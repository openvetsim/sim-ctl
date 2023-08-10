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

installTargets=soundSense
targets=$(installTargets)
CFLAGS=-Wall -ggdb
LDFLAGS=-lrt -lpthread
default: $(targets)

BBB_GPIO=../../BeagleBoneBlack-GPIO

all: $(targets)

soundSense: soundSense.cpp wavTrigger.o wavTrigger.h ../comm/shmData.h ../comm/simCtlComm.h ../comm/simCtlComm.o ../comm/simUtil.h ../comm/simUtil.o
	g++ $(CFLAGS) -o soundSense wavTrigger.o ../comm/simUtil.o ../comm/simCtlComm.o soundSense.cpp $(LDFLAGS)

wavTrigger.o: wavTrigger.cpp wavTrigger.h

install: $(installTargets) .FORCE
	sudo cp -u $(installTargets) /usr/local/bin

factory: $(installTargets) .FORCE
	sudo cp $(installTargets) /usr/local/bin

clean: .FORCE
	rm -f $(targets) *.o *.cgi
	
.FORCE: