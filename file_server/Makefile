CXX = g++ 
CC = g++
CXXFLAGS = -g -Wall -Werror -ansi -pedantic -std=c++11
LDFLAGS = -pthread

all: fserv

## Common:
tcp-utils.o: tcp-utils.h tcp-utils.cc
tokenize.o: tokenize.h tokenize.cc
descriptor.o: descriptor.h descriptor.cc

## multithreaded
fserv.o: tcp-utils.h tokenize.h descriptor.h fserv.cc
fserv: fserv.o tcp-utils.o tokenize.o descriptor.o

clean:
	rm -f fserv *~ *.o *.bak core \#*
