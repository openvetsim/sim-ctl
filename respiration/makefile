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

installTargets=breathSense
targets=$(installTargets)

CFLAGS=-pthread -Wall -g -ggdb
LDFLAGS=-lrt

default:	$(targets)

all: $(targets)

breathSense: breathSense.c  ../comm/simUtil.h ../comm/shmData.h ../comm/simUtil.o
	g++ breathSense.c  $(CFLAGS) ../comm/simUtil.o $(LDFLAGS) -o breathSense

install: $(installTargets) .FORCE
	sudo cp -u $(installTargets) /usr/local/bin

install: $(installTargets) .FORCE
	sudo cp $(installTargets) /usr/local/bin

clean: .FORCE
	rm -f $(targets) *.o *.cgi
	
.FORCE: