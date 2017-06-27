#ifndef PBAR_H
#define PBAR_H

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

struct pbar {
	FILE *fd;
	unsigned int marks;
	float min, max;
	char start_delimiter, end_delimiter, fill_char, nofill_char;
};

void pbar_init(struct pbar *pb, FILE *fd, float min, float max, float per);
void pbar_start(struct pbar *pb);
#define pbar_start(pb) { fprintf((pb)->fd, "\n"); pbar_setprogress((pb), 0); }
void pbar_setprogress(struct pbar *pb, float val);
void pbar_custom(struct pbar *pb, char start_delimiter, char end_delimiter,
		 char fill_char, char nofill_char);
#define pbar_custom(pb, sd, ed, fc, nc) {(pb)->start_delimiter = (sd); (pb)->end_delimiter = (ed); (pb)->fill_char = (fc); (pb)->nofill_char = (nc);}
void pbar_end(struct pbar *pb);
#define pbar_end(pb) {(pb)->progress = 0; fprintf((pb)->fd, "\n");}

#endif // pbar.h included
