CC=g++
CPPFLAGS= -Wall -g -std=c++14 -O3
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: $(OBJECTS)
	$(CC) $(CPPFLAGS) -O3 -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq

testing: $(OBJECTS) 
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq

test: CPPFLAGS+= -DRUN_TESTS=1

test: testing

%.o: %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o bgp-extrapolator
