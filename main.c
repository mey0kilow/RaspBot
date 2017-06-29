#include <RTIMULib.h>
#include "gps.h"
#include "control.h"
#include "i2c/i2c.h"
#include "pca9685.h"

#define USAGE_STRING "Usage: %s Kp Ki Kd t marks.txt\n"

#define DIRECTION_MID 50
#define DIRECTION_MAX 100	/*TODO*/
#define DIRECTION_MIN 0		/*TODO*/
#define DIRECTION_CHANNEL 99/*TODO*/
#define NMARKS 3
#define BUFSIZE 32

#define I2C_DEV "/dev/i2c-1"

RTIMU *imu;
i2c bus;
struct pos_t mark[NMARKS];
int actual_mark;
struct gps_data_t gps;

void direction(double input)
{
	PCA9685_setDutyCicle(bus, DIRECTION_CHANNEL, input);
}

double get_angle(void)
{
	RTIMU_DATA imuData = imu->getIMUData();
	return imuData.fusionPose.z();
}

void *gps_monitor(void *args)
{
	struct control_args_t *arg = (struct control_args_t *)args;
	struct timespec t;
	struct gps_data_t actual_pos;
	double last_time;

	clock_gettime(CLOCK_MONOTONIC, &t);

	while(actual_mark < NMARKS) {
		while(arg->running) {
			/*Wait for next execution. Sleep again if interrupted*/
			while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL));

			/*get time*/
			clock_gettime(CLOCK_MONOTONIC, &ts);

			/*Read */
			if(gps_read(&gps) < 0) {
				perror("GPS read fail");
			} else {
				if(gps.set && gps.status) {
					/*Get actual position*/
					actual_pos.latitude = gps.fix.latitude;
					actual_pos.longitude = gps.fix.longitude;

					/*Lock feedback structure*/
					pthread_mutex_lock(arg->feedback.mutex);

					/*Set the new reference*/
					arg->feedback.ref = azimuth(actual_pos, mark[actual_mark]);

					/*Unlock feedback structure*/
					pthread_mutex_unlock(arg->feedback.mutex);

					/*TODO: check if it is close enough to start the camera with haversine function*/
				} else {
					fprintf(stderr, "GPS read fail: no fix yet");
				}
			}

			/*Calculate the time of the next execution*/
			ts.tv_nsec += GPS_POLL_TIME;

			/*Prevent tv_nsec overflow*/
			while(ts.tv_nsec >= 1e9) {
				ts.tv_nsec -= 1e9;
				ts.tv_sec++;
			}
		}

		actual_mark++;
	}
}

int main(int argc, char *argv[])
{
	int t, i, ret, marks_file;
	double Kp, Ki, Kd;
	struct control_args_t control_args;
	char buf[BUFSIZE];

	if(argc < 6) {
		fprintf(stderr, USAGE_STRING, argv[0]);
		exit(EXIT_FAILURE);
	}

	sscanf(argv[1], "%f", &Kp);
	sscanf(argv[2], "%f", &Ki);
	sscanf(argv[3], "%f", &Kd);
	sscanf(argv[4], "%d", &t);

	/*TODO: check arguments*/

	/*Open/parsing marks.txt*/
	marks_file = open(argv[5], O_RDONLY);
	if(marks_file < 0) {
		fprintf(stderr, "Cannot open '%s' for read\n", argv[5]);
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < NMARKS;) {
		/*read file until '\n'*/
		fgets(buf, BUFSIZE, marks_file);

		/*Scan a "median" line*/
		ret = sscanf(buf, "Median: %f,%f\n", &mark[i].latitude, &mark[i].longitude);

		/*If reach the end of the file*/
		if(ret == EOF) {
			/*Something is wrong*/
			fprintf(stderr, "Fail to read %d marks from %s\n", NMARKS, argv[5]);
			close(marks_file);
			exit(EXIT_FAILURE);
		}

		/*If it have at least one match*/
		if(ret > 0) {
			/*Advance*/
			i++;
		}
	}

	close(marks_file);
	actual_mark = 0;

	/*IMU initialization*/
	/*TODO: RTIMULib should use SPI*/
	imu = RTIMU::createIMU(new RTIMUSettings("RTIMULib"));
	if((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL)) {
		fprintf(stderr, "No IMU found\n");
		exit(EXIT_FAILURE);
	}

	imu->IMUInit();

	imu->setSlerpPower(0.02);
	imu->setGyroEnable(true);
	imu->setAccelEnable(true);
	imu->setCompassEnable(true);

	/*I2C bus initialization*/
	bus = i2cOpen("/dev/i2c-1");
	if(bus == NULL) {
		perror("i2cOpen fail");
		exit(EXIT_FAILURE);
	}

	/*GPS initialization*/
	if(gps_open(GPSD_SHARED_MEMORY, NULL, &gps)) {
		perror("Cannot connect to gpsd daemon");
		i2cClose(bus);
		exit(EXIT_FAILURE);
	}

	control_args_init(&control_args);
	pid_zoh(&control_args, Kp, Ki, Kd, t);

	plant_config(&control_args, direction, DIRECTION_MID, DIRECTION_MAX, DIRECTION_MIN);
	feedback_config(&control_args, get_angle, imu->IMUGetPollInterval()*1000000);
}
