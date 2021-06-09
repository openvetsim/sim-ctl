#
# This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
# 
# Copyright (c) 2019-2021 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
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

# Top level makefile
#
# Each subdir, targets for "all", "install" and "clean" should be provided.

# pulse
SUBDIRS =  comm cardiac cpr  respiration pulse wav-trig initialization test www
MAKEFLAGS = 
#--no-print-directory
default:
#	@test -s ../BeagleBoneBlack-GPIO || { echo "Did not find BeagleBoneBlack-GPIO! Exiting..."; exit 1; }

	@for dir in $(SUBDIRS); do \
		$(MAKE) $(MAKEFLAGS) -C $$dir ; \
		done
	
build:
	@mkdir -p build

all clean install factory:
	@for dir in $(SUBDIRS); do \
		$(MAKE) $(MAKEFLAGS) -C $$dir  $@; \
		done
	
.PHONY: build $(SUBDIRS)
