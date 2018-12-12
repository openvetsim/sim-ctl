installTargets=cprScan 
targets=$(installTargets)
CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt

default:	$(targets)

all: $(targets)

cprScan: cprScan.cpp  cprI2C.o cprI2C.h ../comm/simUtil.o ../comm/simUtil.h
	g++ cprScan.cpp  $(CFLAGS) $(LDFLAGS) cprI2C.o ../comm/simUtil.o -lrt -o cprScan
	
cprI2C.o: cprI2C.cpp cprI2C.h ../comm/simUtil.h ../comm/shmData.h
	g++   $(CFLAGS) -c -o cprI2C.o cprI2C.cpp
	
install: $(installTargets) .FORCE
	sudo cp -u $(installTargets) /usr/local/bin

clean: .FORCE
	rm -f $(targets) *.o *.cgi
	
.FORCE: