installTargets=pulse
targets=$(installTargets)

CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt

default:	$(targets)

all: $(targets)

pulse: pulse2.c  ../comm/simUtil.h ../comm/shmData.h ../comm/simUtil.o 
	g++ pulse2.c  $(CFLAGS)  ../comm/simUtil.o $(LDFLAGS) -o pulse

pulseI2C.o: pulseI2C.cpp pulseI2C.h
	g++   $(CFLAGS) -c -o pulseI2C.o pulseI2C.cpp

install: $(installTargets) .FORCE
	sudo cp -u $(installTargets) /usr/local/bin

clean: .FORCE
	rm -f $(targets) *.o *.cgi
	
.FORCE: