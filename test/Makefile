# Compile opetion
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
OBJECTS=../control.o ../i2c/i2c.o ../PCA9685/pca9685.o ../navigation.o

all: direction

direction: direction.c $(OBJECTS) $(OBJECTS:.o=.h)
	$(CXX) $(OBJECTS) direction.c -o direction $(CFLAGS) $(LDFLAGS) $(LINKS)

clean:
	-${RM} direction 2>/dev/null

.PHONY: clean
