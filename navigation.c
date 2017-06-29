#include "gps.h"

double azimuth(struct pos_t source, struct pos_t dest)
{
	double h;
	source.latitude *= M_PI/180;
	source.longitude *= M_PI/180;
	dest.latitude *= M_PI/180;
	dest.longitude *= M_PI/180;
	h = atan2(sin((dest.longitude)-(source.longitude))*cos(dest.latitude),cos(source.latitude)*sin(dest.latitude)-sin(source.latitude)*cos(dest.latitude)*cos((dest.longitude)-(source.longitude))); // answer between -180 and +180
	return h < 0? h + 2*M_PI : h;		// answer between 0 and 360
}

double haversine(struct pos_t source, struct pos_t dest)
{
	double a;
	source.latitude *= M_PI/180;
	source.longitude *= M_PI/180;
	dest.latitude *= M_PI/180;
	dest.longitude *= M_PI/180;

	a = sin((dest.latitude - source.latitude)/2)*sin((dest.latitude - source.latitude)/2) + cos(source.latitude) * cos(dest.latitude) * sin((dest.longitude - source.longitude)/2)*sin((dest.longitude - source.longitude)/2);
	return 2*EARTH_RADIUS*atan2(sqrt(a), sqrt(1-a));
}

