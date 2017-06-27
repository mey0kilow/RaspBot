#ifndef GPS_H
#define GPS_H

#include <gps.h>
#include <math.h>

#define EARTH_RADIUS 6371000
#define GPS_POLL_TIME 1e9

struct pos_t {
	double latitude, longitude;
};

double azimuth(struct pos_t source, struct pos_t dest);
double haversine(struct pos_t source, struct pos_t dest);

#endif
