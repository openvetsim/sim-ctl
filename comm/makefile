#
# This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
# 
# Copyright (c) 2019 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
# 
# This program is free software: you can redistribute it and/or modify  
# it under the terms of the GNU General Public License as published by  
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License 
# along with this program. If not, see <http://www.gnu.org/licenses/>.

installTargets=simController simCurl 
targets=simUtil.o simCtlComm.o $(installTargets) 
cgiTargets=ctlstatus.cgi
CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt
	
default:	$(targets) $(cgiTargets)

all:	$(targets) $(cgiTargets)
	
simCurl:	curl.cpp
	g++   $(CFLAGS) -lcurl -o simCurl curl.cpp
	
simUtil.o: simUtil.c simUtil.h
	g++   $(CFLAGS) -c -o simUtil.o simUtil.c

simParse.o: simParse.c shmData.h
	g++   $(CFLAGS) -c -o simParse.o simParse.c

simCtlComm.o: simCtlComm.cpp simCtlComm.h simUtil.h 
	g++   $(CFLAGS) -c -o simCtlComm.o simCtlComm.cpp
	
simController: simController.cpp simUtil.h shmData.h simUtil.o simParse.o
	g++   $(CFLAGS) $(LDFLAGS) -o simController simController.cpp simUtil.o simParse.o

ctlstatus.cgi: ctlstatus.cpp simUtil.h shmData.h simUtil.o
	g++   $(CFLAGS) $(LDFLAGS) -o ctlstatus.cgi ctlstatus.cpp simUtil.o

install: $(targets) .FORCE $(cgiTargets)
	sudo cp -u  $(installTargets) /usr/local/bin
	sudo cp -u $(cgiTargets) /var/www/cgi-bin
	sudo chmod ug+s /var/www/cgi-bin/*.cgi

factory: $(targets) .FORCE $(cgiTargets)
	sudo cp  $(installTargets) /usr/local/bin
	sudo cp  $(cgiTargets) /var/www/cgi-bin
	sudo chmod ug+s /var/www/cgi-bin/*.cgi
	
clean: .FORCE
	rm -f $(targets) *.o
	
.FORCE: