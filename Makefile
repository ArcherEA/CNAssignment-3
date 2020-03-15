CXX = g++
CXXFLAGS = -g -Wall -Werror

all: iserv ciserv cserv

## Common:
tcp-utils.o: tcp-utils.h tcp-utils.cc
	$(CXX) $(CXXFLAGS) -c -o tcp-utils.o tcp-utils.cc
tokenize.o: tokenize.h tokenize.cc
	$(CXX) $(CXXFLAGS) -c -o tokenize.o tokenize.cc

## Server:
## (a) iterative server
iserv.o: tcp-utils.h iserv.cc
	$(CXX) $(CXXFLAGS) -c -o iserv.o iserv.cc

iserv: iserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o iserv iserv.o tcp-utils.o

## (b) apparent concurrency
cserv.o: tcp-utils.h ciserv.cc
	$(CXX) $(CXXFLAGS) -c -o ciserv.o ciserv.cc

cserv: ciserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o ciserv ciserv.o tcp-utils.o

## (c) true concurrency
ciserv.o: tcp-utils.h ciserv.cc tokenize.h
	$(CXX) $(CXXFLAGS) -c -o ciserv.o ciserv.cc

ciserv: cserv.o tcp-utils.o tokenize.o
	$(CXX) $(CXXFLAGS) -o ciserv ciserv.o tcp-utils.o tokenize.o

clean:
	rm -f iserv ciserv cserv *~ *.o *.bak core \#*

