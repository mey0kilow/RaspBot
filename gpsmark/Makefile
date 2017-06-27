CC=gcc

OPTS=-std=gnu99
LINKS=-lgps

all: gpsmark

gpsmark: gpsmark.o pbar.o sort.o
	$(CC) gpsmark.o pbar.o sort.o $(OPTS) $(LINKS) -o gpsmark

gpsmark.o: gpsmark.c pbar.h sort.h
	$(CC) gpsmark.c -c $(OPTS)

pbar.o: pbar.c pbar.h
	$(CC) pbar.c -c $(OPTS)

sort.o: sort.c sort.h
	$(CC) sort.c -c $(OPTS)

clean:
	-rm *.o 2>/dev/null || true
