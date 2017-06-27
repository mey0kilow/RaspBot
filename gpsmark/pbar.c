#include "pbar.h"

void pbar_init(struct pbar *pb, FILE *fd, float min, float max, float per)
{
	struct winsize w;

	pb->fd = fd;

	ioctl(fileno(fd), TIOCGWINSZ, &w);
	pb->marks = ((w.ws_col - 9)*per);

	pb->min = min;
	pb->max = max;

	pb->start_delimiter = '[';
	pb->end_delimiter = ']';
	pb->fill_char = '=';
	pb->nofill_char = ' ';
}

void pbar_setprogress(struct pbar *pb, float val)
{
	float progress = (val - pb->min)/(pb->max - pb->min);
	int i, fill = (pb->marks*progress);

	fprintf(stderr, "\r\r%c", pb->start_delimiter);
	for(i = fill; i; i--) {
		fprintf(pb->fd, "%c", pb->fill_char);
	}

	for(i = pb->marks - fill; i; i--) {
		fprintf(pb->fd, "%c", pb->nofill_char);
	}

	fprintf(pb->fd, "%c %4.1f%%", pb->end_delimiter, progress*100);
}
