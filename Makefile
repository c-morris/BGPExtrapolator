CC=g++
CPPFLAGS= -Wall -g -std=c++14 -DBOOST_LOG_DYN_LINK
LDFLAGS= -lpqxx -lpq -lboost_program_options -lboost_unit_test_framework -lboost_log -lboost_filesystem -lboost_thread -lpthread -lboost_system -lboost_log_setup
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

# compile with optimization if not running tests
all: CPPFLAGS+= -O3

all: $(OBJECTS)
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) $(LDFLAGS)

test: CPPFLAGS+= -DRUN_TESTS=1

test: $(OBJECTS) 
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o bgp-extrapolator