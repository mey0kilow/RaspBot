# Compile opetion
cc=gcc
cxx=g++
OPTS=-ggdb -Wall

# Link option
LINKS=-pthread -lm -lgps

# Compile Objects
OBJECTS=control.o

all: main

main: main.c $(OBJECTS)
	$(CXX) $(OBJECTS) main.c -o main $(LINKS) $(OPTS)

$(OBJECTS): %.o: %.c
	$(CXX) $< -c $(LINKS) $(OPTS) $(LINKS)
