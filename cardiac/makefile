installTargets=rfidScan 
targets= $(installTargets)

CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt

default:	rfidScan

all: $(targets)
	
rfidScan: rfidScan.cpp  rfidScan.h ../comm/shmData.h ../comm/simCtlComm.h ../comm/simUtil.h
	g++ rfidScan.cpp  $(CFLAGS) $(LDFLAGS)  -I/usr/include/libxml2 -lrt -lxml2 ../comm/simUtil.o -o rfidScan

install: $(installTargets) .FORCE
	sudo cp -u $(installTargets) /usr/local/bin
	
clean: .FORCE
	rm -f $(targets) *.o *.cgi gpio
	
.FORCE:
