#include <RTIMULib.h>
#include <gps.h>
#include "control.h"
#include "i2c.h"
#include "pca9685.h"

#define USAGE_STRING "Usage: %s Kp Ki Kd t marks.txt\n"

#define DIRECTION_MID 50
#define DIRECTION_MAX 100	/*TODO*/
#define DIRECTION_MIN 0		/*TODO*/
#define DIRECTION_CHANNEL 99/*TODO*/

#define I2C_DEV "/dev/i2c-1"

RTIMU *imu;
i2c bus;

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
	double last_time;

	clock_gettime(CLOCK_MONOTONIC, &t);

	while(true) {
		/*TODO*/
	}
}

int main(int argc, char *argv[])
{
	int t;
	double Kp, Ki, Kd;
	struct control_args_t control_args;

	if(argc < 5) {
		fprintf(stderr, USAGE_STRING, argv[0]);
		exit(EXIT_FAILURE);
	}

	sscanf(argv[1], "%f", &Kp);
	sscanf(argv[2], "%f", &Ki);
	sscanf(argv[3], "%f", &Kd);
	sscanf(argv[4], "%d", &t);

	/*TODO: check arguments*/

	/*IMU initialization*/
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

	/*TODO: I2C bus initialization*/

	/*TODO: Open/parsing marks.txt*/

	control_args_init(&control_args);
	pid_zoh(&control_args, Kp, Ki, Kd, t);

	plant_config(&control_args, direction, DIRECTION_MID, DIRECTION_MAX, DIRECTION_MIN);
	feedback_config(&control_args, get_angle, imu->IMUGetPollInterval()*1000000);
}
