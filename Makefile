CC=g++
CPPFLAGS= -Wall -g -std=c++14 -DBOOST_LOG_DYN_LINK
LDFLAGS= -lpqxx -lpq -lboost_program_options -lboost_unit_test_framework -lboost_log -lboost_filesystem -lboost_thread -lpthread -lboost_system -lboost_log_setup
OBJECTS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
HEADERS := $(wildcard *.h)
PREFIX = /usr/local
LOGDIR = /opt/log/bgp-extrapolator

# compile with optimization if not running tests
all: CPPFLAGS+= -O3

all: $(OBJECTS) $(HEADERS)
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) $(LDFLAGS)

test: CPPFLAGS+= -DRUN_TESTS=1

test: $(OBJECTS) 
	$(CC) $(CPPFLAGS) -o bgp-extrapolator $(OBJECTS) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CC) -c $(CPPFLAGS) $< -o $@

logdir:
	mkdir -p $(LOGDIR)

install: $(OBJECTS) logdir
	install -D bgp-extrapolator $(DESTDIR)$(prefix)/bin/bgp-extrapolator

build: all
build-arch: all
build-indep: all
binary: install
binary-arch: install
binary-indep: install

.PHONY: clean distclean
clean:
	rm *.o bgp-extrapolator || true

distclean: clean
