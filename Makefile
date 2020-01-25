CC=g++
CPPFLAGS= -Wall -g -std=c++14
LDFLAGS= -lpqxx -lpq -lboost_program_options -lboost_unit_test_framework
HEADERS := $(wildcard *.h)
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

# compile with optimization if not running tests
all: CPPFLAGS+= -O3

all: $(OBJECTS) $(HEADERS)
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) $(LDFLAGS)

test: CPPFLAGS+= -DRUN_TESTS=1

test: $(OBJECTS) 
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CC) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o bgp-extrapolator
