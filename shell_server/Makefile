CXX = g++
CXXFLAGS = -g -Wall -Werror

all: ciserv

## Common:
tcp-utils.o: tcp-utils.h tcp-utils.cc
	$(CXX) $(CXXFLAGS) -c -o tcp-utils.o tcp-utils.cc

## Server:
ciserv.o: tcp-utils.h ciserv.cc
	$(CXX) $(CXXFLAGS) -c -o ciserv.o ciserv.cc

ciserv: ciserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o ciserv ciserv.o tcp-utils.o

clean:
	rm -f ciserv *~ *.o *.bak core \#*

