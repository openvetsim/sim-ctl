installTargets=breathSense
targets=$(installTargets)

CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt

default:	$(targets)

all: $(targets)

breathSense: breathSense.c  ../comm/simCtlComm.h  ../comm/simUtil.h ../comm/shmData.h ../comm/simCtlComm.o ../comm/simUtil.o
	g++ breathSense.c  $(CFLAGS)  ../comm/simCtlComm.o ../comm/simUtil.o $(LDFLAGS) -o breathSense

install: $(installTargets) .FORCE
	sudo cp -u $(installTargets) /usr/local/bin

clean: .FORCE
	rm -f $(targets) *.o *.cgi
	
.FORCE: