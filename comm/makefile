installTargets=simController simCurl 
targets=simUtil.o simCtlComm.o $(installTargets) 
CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt
	
default:	$(targets)

all:	$(targets)
	
simCurl:	curl.cpp
	g++   $(CFLAGS) -lcurl -o simCurl curl.cpp
	
simUtil.o: simUtil.c simUtil.h
	g++   $(CFLAGS) -c -o simUtil.o simUtil.c

simParse.o: simParse.c shmData.h
	g++   $(CFLAGS) -c -o simParse.o simParse.c

simCtlComm.o: simCtlComm.cpp simCtlComm.h simUtil.h 
	g++   $(CFLAGS) -c -o simCtlComm.o simCtlComm.cpp
	
simController: simController.cpp simUtil.h shmData.h simUtil.o simCtlComm.o simParse.o
	g++   $(CFLAGS) $(LDFLAGS) -o simController simController.cpp simCtlComm.o simUtil.o simParse.o
	
install: $(targets) .FORCE
	sudo cp -u  $(installTargets) /usr/local/bin
	
clean: .FORCE
	rm -f $(targets) *.o
	
.FORCE: