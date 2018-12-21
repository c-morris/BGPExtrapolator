CC=g++
CPPFLAGS= -Wall -g -std=c++14
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

all: $(OBJECTS)
	$(CC) $(CPPFLAGS) -O3 -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq

# recompile main.o to run tests
test: $(OBJECTS)
	$(CC) -c $(CPPFLAGS) -DRUN_TESTS=1 -o main.o main.cpp
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) -lpqxx -lpq

%.o: %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o bgp-extrapolator
