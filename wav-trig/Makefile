installTargets=soundSense
targets=$(installTargets)
CFLAGS=-Wall
default: $(targets)

BBB_GPIO=../../BeagleBoneBlack-GPIO

all: $(targets)

soundSense: soundSense.cpp wavTrigger.o wavTrigger.h ../comm/shmData.h ../comm/simCtlComm.h ../comm/simCtlComm.o ../comm/simUtil.h ../comm/simUtil.o
	g++ $(CFLAGS) -o soundSense -lrt -lrt -lpthread -Wall wavTrigger.o ../comm/simUtil.o ../comm/simCtlComm.o soundSense.cpp
	
pulseI2C.o: pulseI2C.cpp pulseI2C.h

wavTrigger.o: wavTrigger.cpp wavTrigger.h

install: $(installTargets) .FORCE
	sudo cp  $(installTargets) /usr/local/bin

clean: .FORCE
	rm -f $(targets) *.o *.cgi
	
.FORCE: