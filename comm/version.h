/*
 * version.h
 *
 * This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
 * 
 * Copyright (c) 2021-2023 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#define SIMCTL_VERSION "1.1.11"

/*
Version 1.1.10:
	Fix fail to recover lost connection, introduced in last release.
	Add immediate reopen of current connection. This eliminates dropped pulses.
	
Version 1.1.9:
	Changed SimManager scan to use both WVS_SYNC_PORT and LINUX_SYNC_PORT rather than require setting in the simmgrName file
	
Version 1.1.8:
	Add update package for installing without needing internet
	Change default SYNC_PORT to WinVetSeim (40844)
	
Version 1.1.7:
	Remove Tank support from Air Controls. Add support for ICStation RFID Reader.
	
Version 1.1.6:
	Changes to prepare for build with Debian 11
	
Version 1.1.5:
	Limit Limit breath inhalation time to 1.5 seconds, to better sync chest movement to sounds at low RR
	Auto recover from CPR sensor temprarily removed from SimCtl.
	
Version 1.1.4:
	Fixed missing respiration/breathSense.cpp. Left out from 1.1.3 release

Version 1.1.3:

	Adjustment in loop delays to reduce CPU usage
	
Version 1.1.2:
	
	Added support for ToF sensor, VL6180x for compression distance

Version 1.1.1:

	Reverted Rise disable action to pre 1.1.0

Changes since 1.0.4:

This version supports an upcoming change to the Sim Controller PCB (PCB version 3.1.x), which does not
have ADC or Audio channels connected for Dorsal Pulses.

This version also supports using two Wav Trigger cards in place of a single Tsunami. This is to provide an
alternative if Tsunami boards are unavailable.

Software Changes:

	Update JQuery to version 3.6.4
	
	Add support for dual WavTrigger sound cards in place of Tsunami
	(Tsunami is still the default configuration)

	Remove support for Dorsal Pulse
	
	Change to service "stop" process
	
	Debug output changes
*/
