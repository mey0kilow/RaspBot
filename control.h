#ifndef CONTROL_H
#define CONTROL_H

#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

/*Argument structure*/

//struct control_args_t {
//	/*Controller data*/
//	double a[3], b[1];	/* a0, a1 and a2 are the elements that multiply z at */
//						/* 0, -1 and -2 respectively on the transfer function*/
//						/* numerator. b0 is the element that multiply z at -1*/
//						/* on the transfer function numerator                */
//	double e[3];		/* Error vector. e[0] is the current error, e[1] is  */
//						/* the last error, and so on...                      */
//	double act, t;		/* Control action, discretization time               */
//	bool running;		/* If the control loop is currently running          */
//	double reference;	/* Target angle                                      */
//	struct gps_data_t gps;	/* GPSD structure.*/
//	pthread_mutex_t mutex;	/* A mutex that lock the whole structure         */
//	sem_t error, action;	/* Semaphores for synchronization of new error or*/
//							/* new control action avail.*/
//	/*Plant data*/
//	void (*actuator)(double input);
//	double actuator_bias, actuator_max, actuator_min;
//	/*Feedback data*/
//	double (*sensor)(void);
//};


struct control_data_t {
	double a[3], b[1];	/* a0, a1 and a2 are the elements that multiply z at */
						/* 0, -1 and -2 respectively on the transfer function*/
						/* numerator. b0 is the element that multiply z at -1*/
						/* on the transfer function numerator                */
	double t;			/* Discretization time                               */
};

struct plant_data_t {
	void (*actuator)(double input);	/* Function that actuate over the plant. */
	double actuator_bias,			/* This offset will be added to the      */
									/* control action before send to plant   */
		   actuator_max, actuator_min;	/* Actuator max and min values, after*/
										/* actuator_bias addition.*/
	pthread_mutex_t mutex;
};

struct feedback_data_t {
	double (*sensor)(void);
	double ref;						/* Target reference */
	unsigned long int poll_interval;/* Sensor pooling interval in nanosecs   */
	pthread_mutex_t mutex;
};

struct control_args_t {
	double e[3];		/* Error vector. e[0] is the current error, e[1] is  */
						/* the last error, and so on...                      */
	double act;			/* Control action                                    */
	sem_t error, action;/* Semaphores for synchronization of new error/new   */
						/* control action avail.                             */
	bool running;		/* If the control loop is currently running          */
	struct control_data_t control;		/* Control loop stuff                */
	struct plant_data_t plant;			/* Plant stuff                       */
	struct feedback_data_t feedback;	/* Feedback branch stuff             */
	pthread_mutex_t mutex;	/* A mutex that lock the whole structure         */
	pthread_t closed_loop_thread, plant_thread, feedback_thread;
};

/*Threads*/
void *closed_loop(void *args);
void *plant(void *args);
void *feedback(void *args);

/*Functions*/
void control_args_init(struct control_args_t *control_args);
void pid_zoh(struct control_args_t *args, double Kp, double Ki, double Kd, int t);
void control_set_ref(struct control_args_t *control_args, double ref);
void plant_config(struct control_args_t *control_args, void (*actuator)(double input), double bias, double max, double min);
void feedback_config(struct control_args_t *control_args, double (*sensor)(void), unsigned long int poll_interval);
int closed_loop_start(struct control_args_t *control_args);
int closed_loop_stop(struct control_args_t *control_args);

#endif /*control.h included*/
