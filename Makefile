SRC_DIR := ./src/
BIN_DIR := ./bin/
HEADER_DIR := ./include/

SOURCE_FILES := cpp
HEADER_FILES := h
OBJECT_FILES := o

CC       := g++
CPPFLAGS := -std=c++14 -O3 -Wall -DBOOST_LOG_DYN_LINK -I $(HEADER_DIR)
LDFLAGS  := -lpqxx -lpq -lboost_program_options -lboost_unit_test_framework -lboost_log -lboost_filesystem -lboost_thread -lpthread -lboost_system -lboost_log_setup

SOURCES := $(shell find $(SRC_DIR) -name "*.$(SOURCE_FILES)")
HEADERS := $(shell find $(HEADER_DIR) -name "*.$(HEADER_FILES)")
OBJECTS := $(patsubst $(SRC_DIR)%.$(SOURCE_FILES), $(BIN_DIR)%.$(OBJECT_FILES), $(SOURCES))

PREFIX := /usr/local
LOGDIR := /opt/log/bgp-extrapolator

EXE_NAME := bgp-extrapolator
MAIN_CPP := main.cpp

all: $(OBJECTS) $(HEADERS)
	$(CC) $(CPPFLAGS) $(MAIN_CPP) -o $(EXE_NAME) $(OBJECTS) $(LDFLAGS)

test: CPPFLAGS+= -g -DRUN_TESTS=1

test: $(OBJECTS) $(HEADERS)
	$(CC) $(CPPFLAGS) $(MAIN_CPP) -o $(EXE_NAME) $(OBJECTS) $(LDFLAGS)

$(BIN_DIR)%$(OBJECT_FILES): $(SRC_DIR)%$(SOURCE_FILES) $(HEADERS)
	@mkdir -p $(@D)

	$(CC) $(CPPFLAGS) -c $< -o $@

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
	rm -r -f $(BIN_DIR)* $(EXE_NAME) || true

distclean: clean
