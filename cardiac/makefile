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
