#include <RTIMULib.h>
#include "navigation.h"
#include "control.h"
#include "i2c.h"
#include "pca9685.h"

#define USAGE_STRING "Usage: %s Kp Ki Kd t ref\n"

#define DIRECTION_MID 50
#define DIRECTION_MAX 60
#define DIRECTION_MIN 40
#define DIRECTION_CHANNEL 0
#define ESC_CHANNEL 1
#define LED_CHANNEL_A 2
#define LED_CHANNEL_B 3
#define LED_CHANNEL_C 4
#define NMARKS 3
#define BUFSIZE 32

#define I2C_DEV "/dev/i2c-1"

#define WHITE_TRESHOLD 10000

RTIMU *imu;
i2c bus;
struct pos_t mark[NMARKS];

void direction(double input)
{
	PCA9685_setDutyCicle(bus, DIRECTION_CHANNEL, input);
}

double get_angle(void)
{
	RTIMU_DATA imuData = imu->getIMUData();
	return imuData.fusionPose.z();
}

int main(int argc, char *argv[])
{
	int t, i, ret;
	double Kp, Ki, Kd, ref;
	struct control_args_t control_args;
	color currentColor;

	if(argc < 6) {
		fprintf(stderr, USAGE_STRING, argv[0]);
		exit(EXIT_FAILURE);
	}

	sscanf(argv[1], "%lf", &Kp);
	sscanf(argv[2], "%lf", &Ki);
	sscanf(argv[3], "%lf", &Kd);
	sscanf(argv[4], "%d", &t);
	sscanf(argv[5], "%lf", &ref);

	/*TODO: check arguments*/

	/*Open/parsing marks.txt*/
	marks_file = fopen(argv[5], "r");
	if(marks_file < 0) {
		fprintf(stderr, "Cannot open '%s' for read\n", argv[5]);
		exit(EXIT_FAILURE);
	}

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
	bus = i2c_open("/dev/i2c-1");
	if(bus == NULL) {
		perror("i2c_open fail");
		exit(EXIT_FAILURE);
	}

	/*Control system initialization*/
	control_args_init(&control_args);

	/*Set PID constants*/
	pid_zoh(&control_args, Kp, Ki, Kd, t);

	/*Set direction function*/
	plant_config(&control_args, direction, DIRECTION_MID, DIRECTION_MAX, DIRECTION_MIN);

	/*Set feedback function*/
	feedback_config(&control_args, get_angle, imu->IMUGetPollInterval()*1000000);

	/*Set fixed reference*/
	control_set_ref(&control_args, ref);

	/*Configure RGB Sensor*/
	TCS3472_setIntegrationTime(bus, TCS3472_ATIME_24MS);

	/*Enable RGB sensor*/
	TCS3472_powerOn(rgb);

	/*Start controll loop*/
	closed_loop_start(&control_args);

	sleep(1);

	/*Set vel*/
	PCA9685_setDutyCicle(bus, ESC_CHANNEL, 51);

	/*Check for white*/
	TCS3472_getColor(rgb, &currentColor);

	while(currentColor.red < WHITE_TRESHOLD && currentColor.green < WHITE_TRESHOLD && currentColor.blue < WHITE_TRESHOLD) {
		TCS3472_getColor(rgb, &currentColor);
	}

	/*Stop*/
	PCA9685_setDutyCicle(bus, ESC_CHANNEL, 51);

	/*Leds on*/
	PCA9685_setDutyCicle(bus, LED_CHANNEL_A, 100);
	PCA9685_setDutyCicle(bus, LED_CHANNEL_B, 100);
	PCA9685_setDutyCicle(bus, LED_CHANNEL_C, 100);

	sleep(1);

	closed_loop_stop(&control_args);

	i2c_close(bus);

	exit(EXIT_SUCCESS);
}
