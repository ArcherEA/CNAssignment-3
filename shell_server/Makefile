CXX = g++
CXXFLAGS = -g -Wall -Werror

all: iserv ciserv cserv

## Common:
tcp-utils.o: tcp-utils.h tcp-utils.cc
	$(CXX) $(CXXFLAGS) -c -o tcp-utils.o tcp-utils.cc

## Server:
## (a) iterative server
iserv.o: tcp-utils.h iserv.cc
	$(CXX) $(CXXFLAGS) -c -o iserv.o iserv.cc

iserv: iserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o iserv iserv.o tcp-utils.o

## (b) apparent concurrency
ciserv.o: tcp-utils.h ciserv.cc
	$(CXX) $(CXXFLAGS) -c -o ciserv.o ciserv.cc

ciserv: ciserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o ciserv ciserv.o tcp-utils.o

## (c) true concurrency
cserv.o: tcp-utils.h cserv.cc
	$(CXX) $(CXXFLAGS) -c -o cserv.o cserv.cc

cserv: cserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o cserv cserv.o tcp-utils.o

clean:
	rm -f iserv ciserv cserv *~ *.o *.bak core \#*

