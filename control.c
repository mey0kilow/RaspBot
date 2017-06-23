#include "control.h"
#include <time.h>
#include <string.h>

void *closed_loop(void *args)
{
	struct control_args_t *arg = (struct control_args_t *) args;
	struct timespec ts;

	/*TODO: stackfault*/

	clock_gettime(CLOCK_MONOTONIC, &ts);

	/*wait a second*/
	ts.tv_sec++;

	while(arg->running) {
		/*Wait for next execution. Sleep again if interrupted*/
		while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL));

		/*Wait for a new error calculation*/
		sem_wait(&arg->error);

		/*Wait for structure lock*/
		pthread_mutex_lock(&arg->mutex);

		/*Calculate the new controller response*/
		/*c(k) = b[0]*c(k-1) + a[0]*e[k] + a[1]*e[k-1] + a[2]*e[k-2]*/
		arg->act = arg->control.b[0]*arg->act
				+ arg->control.a[0]*arg->e[0]
				+ arg->control.a[1]*arg->e[1]
				+ arg->control.a[2]*arg->e[2];

		/*signal that a new controller response is available*/
		sem_post(&arg->action);

		pthread_mutex_unlock(&arg->mutex);

		/*Get the new time. TODO: this  may correct some clock drift, but may*/
		/*cause some performance issue also. If it becomes a problem, a less */
		/*precise clock like CLOCK_MONOTONIC_COARSE could be used, or this   */
		/*line could be ignored, and t will be incremented from the first    */
		/*clock_gettime time. */
		clock_gettime(CLOCK_MONOTONIC, &ts);

		/*Calculate the time of the next execution*/
		ts.tv_nsec += arg->control.t;

		/*Prevent tv_nsec overflow*/
		while(ts.tv_nsec >= 1e9) {
			ts.tv_nsec -= 1e9;
			ts.tv_sec++;
		}
	}

	return NULL;
}

/* Consumer of "action" semaphore, send the control action to the motors*/
void *plant(void *args)
{
	struct control_args_t *arg = (struct control_args_t *)args;
	int val;

	/*TODO: stackfault*/

	while(arg->running) {
		/*Wait for new control action*/
		sem_wait(&arg->action);

		/*Wait for structure lock*/
		pthread_mutex_lock(&arg->plant.mutex);

		/*Send control action to slave microcontroller*/
		val = arg->plant.actuator_bias + arg->act;

		arg->plant.actuator(
				val > arg->plant.actuator_max? arg->plant.actuator_max:
				val < arg->plant.actuator_min? arg->plant.actuator_max:
				val
			);

		pthread_mutex_unlock(&arg->plant.mutex);
	}

	return NULL;
}

void *feedback(void *args)
{
	struct control_args_t *arg = (struct control_args_t *)args;
	struct timespec ts;
	int sem_val;

	/*TODO: stackfault*/

	clock_gettime(CLOCK_MONOTONIC, &ts);

	ts.tv_sec++;

	while(arg->running) {
		while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL));

		pthread_mutex_lock(&arg->mutex);

		/*Rotate buffer*/
		arg->e[2] = arg->e[1];
		arg->e[1] = arg->e[0];

		pthread_mutex_lock(&arg->feedback.mutex);

		/*Calculate new error*/
		arg->e[0] = arg->feedback.ref - arg->feedback.sensor();

		pthread_mutex_unlock(&arg->feedback.mutex);

		sem_getvalue(&arg->error, &sem_val);
		if(sem_val <= 0)
			sem_post(&arg->error);

		pthread_mutex_unlock(&arg->mutex);

		clock_gettime(CLOCK_MONOTONIC, &ts);

		ts.tv_nsec += arg->feedback.poll_interval;
		while(ts.tv_nsec >= 1e9) {
			ts.tv_nsec -= 1e9;
			ts.tv_sec++;
		}

	}

	return NULL;
}

void control_args_init(struct control_args_t *args)
{
	memset(args, '\0', sizeof(struct control_args_t));

	pthread_mutex_init(&args->mutex, NULL);
	pthread_mutex_init(&args->plant.mutex, NULL);
	pthread_mutex_init(&args->feedback.mutex, NULL);

	sem_init(&args->error, 0, 0);
	sem_init(&args->action, 0, 0);

	args->running = false;
}

void pid_zoh(struct control_args_t *args, double Kp, double Ki, double Kd, int t)
{
	double T = ((double)t)/1e9;

	pthread_mutex_lock(&args->mutex);

	/* Continuous PID to digital with zero order holder:
	 * c(k) = c(k-1) + (Kd+Kp)*e[0] + (T*Ki-Kp-2*Kd)*e[1] + Kd*e[2]*/
	args->control.a[0] = Kd + Kp;
	args->control.a[1] = T*Ki - Kp - 2*Kd;
	args->control.a[2] = Kd;
	args->control.b[0] = 1;
	args->control.t = t;

	pthread_mutex_unlock(&args->mutex);
}

void control_set_ref(struct control_args_t *control_args, double ref)
{
	pthread_mutex_lock(&control_args->feedback.mutex);
	control_args->feedback.ref = ref;
	pthread_mutex_unlock(&control_args->feedback.mutex);
}

void plant_config(struct control_args_t *control_args, void (*actuator)(double input), double bias, double max, double min)
{
	pthread_mutex_lock(&control_args->plant.mutex);
	control_args->plant.actuator = actuator;
	control_args->plant.actuator_bias = bias;
	control_args->plant.actuator_max = max;
	control_args->plant.actuator_min = min;
	pthread_mutex_unlock(&control_args->plant.mutex);
}

void feedback_config(struct control_args_t *control_args, double (*sensor)(void), unsigned long int poll_interval)
{
	pthread_mutex_lock(&control_args->feedback.mutex);
	control_args->feedback.sensor = sensor;
	control_args->feedback.poll_interval = poll_interval;
	pthread_mutex_unlock(&control_args->feedback.mutex);
}

int closed_loop_start(struct control_args_t *control_args)
{
	control_args->running = true;
	pthread_create(&control_args->closed_loop_thread,
			NULL, closed_loop, (void *)control_args);
	pthread_create(&control_args->plant_thread,
			NULL, plant, (void *)control_args);
	pthread_create(&control_args->feedback_thread,
			NULL, feedback, (void *)control_args);
	return 0;
}

int closed_loop_stop(struct control_args_t *control_args)
{
	control_args->running = false;
	pthread_join(control_args->closed_loop_thread, NULL);
	pthread_join(control_args->plant_thread, NULL);
	pthread_join(control_args->feedback_thread, NULL);
	return 0;
}
