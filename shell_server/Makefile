CXX = g++
CXXFLAGS = -g -Wall -Werror

<<<<<<< HEAD
all: iserv ciserv cserv
=======
all: ciserv
>>>>>>> 16ba1826d5d804d48970f4552ed95cc8af255b90

## Common:
tcp-utils.o: tcp-utils.h tcp-utils.cc
	$(CXX) $(CXXFLAGS) -c -o tcp-utils.o tcp-utils.cc

## Server:
<<<<<<< HEAD
## (a) iterative server
iserv.o: tcp-utils.h iserv.cc
	$(CXX) $(CXXFLAGS) -c -o iserv.o iserv.cc

iserv: iserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o iserv iserv.o tcp-utils.o

## (b) apparent concurrency
=======
>>>>>>> 16ba1826d5d804d48970f4552ed95cc8af255b90
ciserv.o: tcp-utils.h ciserv.cc
	$(CXX) $(CXXFLAGS) -c -o ciserv.o ciserv.cc

ciserv: ciserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o ciserv ciserv.o tcp-utils.o

<<<<<<< HEAD
## (c) true concurrency
cserv.o: tcp-utils.h cserv.cc
	$(CXX) $(CXXFLAGS) -c -o cserv.o cserv.cc

cserv: cserv.o tcp-utils.o
	$(CXX) $(CXXFLAGS) -o cserv cserv.o tcp-utils.o

clean:
	rm -f iserv ciserv cserv *~ *.o *.bak core \#*
=======
clean:
	rm -f ciserv *~ *.o *.bak core \#*
>>>>>>> 16ba1826d5d804d48970f4552ed95cc8af255b90

