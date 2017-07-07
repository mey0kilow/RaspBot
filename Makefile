# Compile option
CC=gcc
CXX=g++

BASE_DIR=$(shell pwd)

# Include options
INCLUDE_DIRS=-I$(BASE_DIR) -I$(BASE_DIR)/i2c -I$(BASE_DIR)/PCA9685

# Link option
LINKS=-pthread -lm -lgps -lRTIMULib

# CFLAGS and LDFLAGS
CFLAGS+=-ggdb -Wall $(INCLUDE_DIRS)
LDFLAGS+=

# Compile Objects
OBJECTS=control.o i2c/i2c.o PCA9685/pca9685.o navigation.o TCS3472/tcs3472.o

all: main

main: main.c $(OBJECTS)
	$(CXX) $(OBJECTS) main.c -o main $(CFLAGS) $(LDFLAGS) $(LINKS)

$(OBJECTS): %.o: %.c
	$(CXX) $< -c $(CFLAGS) $(LDFLAGS) -o $@

clean:
	-${RM} $(OBJECTS) 2>/dev/null

.PHONY: clean
