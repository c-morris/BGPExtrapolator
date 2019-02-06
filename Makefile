CC=g++
CPPFLAGS= -Wall -g -std=c++14 -O3 -pthread
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

# compile with optimization if not running tests
all: CPPFLAGS+= -O3

all: $(OBJECTS)
	$(CC) $(CPPFLAGS) -O3 -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq -pthread

testing: $(OBJECTS) 
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq

test: CPPFLAGS+= -DRUN_TESTS=1

test: testing

%.o: %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o bgp-extrapolator
