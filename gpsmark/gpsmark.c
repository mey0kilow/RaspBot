#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <gps.h>		// link with -lgps
#include <signal.h>
#include <stdbool.h>

#include "sort.h"
#include "pbar.h"

#define READS_DEFAULT_VALUE 500
#define DELAY_DEFAULT_VALUE 1e6		// 1s default delay -> 1Hz defaul freq.

static bool interrupted = false, quiet = false;
static double delay = DELAY_DEFAULT_VALUE;
static unsigned long reads = READS_DEFAULT_VALUE;
static char *file_name = NULL;
static int verbosity = 0;

void sort(double *vec, size_t size);
void signalHandler(int signo);
void args_parser(int argc, char *argv[]) __attribute__((constructor));
void help(const char *name);

void signalHandler(int signo)
{
	interrupted = true;
}

void args_parser(int argc, char *argv[])
{
	int opt, ret;
	double freq;

	while((opt = getopt(argc, argv, "f:r:qvs")) != -1) {
		switch(opt) {
			case 'f':
				if(sscanf(optarg, "%lf", &freq) != 1) {
					fprintf(stderr, "Invalid argument for \"%s\".\n", optarg);
					help(argv[0]);
				}
				delay = 1e6/freq;
				break;
			case 'r':
				if(sscanf(optarg, "%lu", &reads) != 1) {
					fprintf(stderr, "Invalid number of reads \"%s\".\n", optarg);
					help(argv[0]);
				}
				break;
			case 's':
			case 'q':
				verbosity--;
				break;
			case 'v':
				verbosity++;
				break;
			default:
				help(argv[0]);
		}
	}
	if(optind < argc)
		file_name = argv[optind];
}

void help(const char *name)
{
				fprintf(stderr,
						"Usage: %s [options] [FILE_NAME]\n\t-f FREQ\t\tset read frequency (default is %.2fHz)\n\t-r READS\tset number of reads (default is %d reads)\n\t-s\t\tLess verbose\n\t-q\t\tAlias to -s\n\t-v\t\tMore verbose\n\n\tAll parameters are optionals",
						name,
						1e6/DELAY_DEFAULT_VALUE,
						READS_DEFAULT_VALUE
					   );
				exit(-1);
}

int main(int argc, char *argv[])
{
	struct gps_data_t gps_data;
	struct pbar pb;
	double *latitude, *longitude,					/*Coordinates vectors*/
			latmed, lonmed, latsum = 0, lonsum = 0,/*Coordinates sum and time of last acquisition*/
			last_time = 0;
	unsigned long int i;

	if(file_name)
		if(freopen(file_name, "a", stdout) == NULL) {
			perror("Cannot access file");
			exit(-1);
		}

	signal(SIGINT, signalHandler);

	if(gps_open(GPSD_SHARED_MEMORY, NULL, &gps_data)) {		/*GPSd through shared memory*/
		perror("Cannot connect to gpsd daemon");
		exit(-2);
	}

	latitude = (double*)malloc(sizeof(double)*reads);

	if(latitude == NULL) {
		perror("Cannot allocate memory");
		gps_close(&gps_data);
		exit(-3);
	}

	longitude = (double*)malloc(sizeof(double)*reads);

	if(longitude == NULL) {
		perror("Cannot allocate memory");
		free(latitude);
		gps_close(&gps_data);
		exit(-3);
	}

	if(verbosity >= 0) {
		pbar_init(&pb, stderr, 0, reads, 1);
		if(verbosity > 0) {
			fprintf(stderr, "Reading %lu points", reads);

			if(verbosity > 1)
				fprintf(stderr, " at %.2fHz", 1e6/delay);

			fprintf(stderr, "\n");
		}
		pbar_setprogress(&pb, 0);
	}

	for(i = reads; i && !interrupted;) {

		if(gps_read(&gps_data) < 0) {
			perror("gps_read call failed");
		} else {
			if(gps_data.set && gps_data.status && gps_data.fix.time > last_time) {		/*if initialized, fixed and the fix is newer than the last*/

				last_time = gps_data.fix.time;

				latitude[reads-i] = gps_data.fix.latitude;
				latsum += gps_data.fix.latitude;

				longitude[reads-i] = gps_data.fix.longitude;
				lonsum += gps_data.fix.longitude;

				i--;

				if(verbosity >= 0) {
					pbar_setprogress(&pb, reads-i);
				}
			}
		}
		usleep(delay);
	}

	gps_close(&gps_data);

	reads = reads - i;		/*Update how many reads was really taken, in case the loop exit due to interrupt*/

	if(reads != 0) {
		sort(latitude, reads);	/*Merge Sort on the reads, for median*/
		sort(longitude, reads);

		if(reads%2) {
			latmed = (latitude[reads/2]+latitude[1+reads/2])/2.0;
			lonmed = (longitude[reads/2]+longitude[1+reads/2])/2.0;
		} else {
			latmed = latitude[reads/2];
			lonmed = longitude[reads/2];
		}

		fprintf(stdout, "Median: %f,%f\n", latmed, lonmed);
		fprintf(stdout, "Average: %f,%f\n", latsum/reads, lonsum/reads);

		for(unsigned long int i = reads; i; i--)
			fprintf(stdout, "%lu: %f,%f\n", reads-i, latitude[reads-i], longitude[reads-i]);

		if(verbosity >= 0) {
			fprintf(stderr, "\n");
			if(verbosity > 1) {
				fprintf(stderr, "Read %lu points:\n", reads);
				fprintf(stderr, "Median: %f,%f\n", latmed, lonmed);
				fprintf(stderr, "Average: %f,%f\n", latsum/reads, lonsum/reads);
			}
		}
	} else {
		if(verbosity >= 0) {
			fprintf(stderr, "\nNo reads");
		}
	}

	fclose(stdout);
}
