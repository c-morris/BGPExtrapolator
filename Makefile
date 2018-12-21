CC=g++
CFLAGS= -Wall -g -std=c++14
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: $(OBJECTS)
	$(CC) $(CFLAGS) -O3 -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq

test: $(OBJECTS)
	$(CC) $(CFLAGS) -DRUN_TESTS -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq

%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o bgp-extrapolator
