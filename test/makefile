installTargets=ain_air_test ainmon tsunami_test
targets=$(installTargets)

CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt

default:	$(targets)

all: $(targets)

ain_air_test: ain_air_test.c ../comm/simUtil.h  ../comm/simUtil.o
	g++ ain_air_test.c  $(CFLAGS) ../comm/simUtil.o  $(LDFLAGS)  -o ain_air_test

ainmon: ainmon.cpp
	g++ $(CFLAGS) -o ainmon -Wall  ainmon.cpp

tsunami_test: tsunami_test.cpp ../wav-trig/wavTrigger.o
	g++ $(CFLAGS) -o tsunami_test -Wall  ../wav-trig/wavTrigger.o tsunami_test.cpp
	
install: $(installTargets) .FORCE
	sudo cp -u $(installTargets) /usr/local/bin

clean: .FORCE
	rm -f $(targets) *.o *.cgi
	
.FORCE: