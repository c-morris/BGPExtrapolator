SOURCE_FILES := cpp
HEADER_FILES := hpp
OBJECT_FILES := o

CC       := g++
CPPFLAGS := -std=c++14 -O3 -Wall
LDFLAGS  := -lssl -lcrypto

EXE_NAME := mac-testing
MAIN_CPP := main.cpp

all:
	$(CC) $(CPPFLAGS) $(MAIN_CPP) -o $(EXE_NAME) $(LDFLAGS)

.PHONY: clean distclean
clean:
	rm -r -f $(EXE_NAME) || true

distclean: clean