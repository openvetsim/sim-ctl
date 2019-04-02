/*
 * simCtlComm.h
 *
 * This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
 * 
 * Copyright (c) 2019 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
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
 
#ifndef SIMCTLCOMM_H_
#define SIMCTLCOMM_H_

#define SYNC_PORT	50200
#define LISTEN_ACTIVE	1
#define LISTEN_INACTIVE	0

#define SYNC_PULSE			1
#define SYNC_PULSE_VPC		2
#define SYNC_BREATH			3

#define SIM_IP_ADDR_SIZE 32
#define SIM_NAME_SIZE	512

class simCtlComm {

private:
	int commFD;
	int commPort;
	int trySimMgrOpen(char *name );
	
public:
	simCtlComm(int port);
	
	// Support for Sync Port
	int openListen(int active );	// If Active is set, the port stays open. Otherwise, this is simply used to discover the simmgr
	int closeListen(void );
	int wait(const char *syncMessage );
	void show(void );
	
	char simMgrName[SIM_NAME_SIZE];
	char simMgrIPAddr[SIM_IP_ADDR_SIZE];
	virtual ~simCtlComm();
};

struct IPv4
{
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
	unsigned char b4;
};
	
#endif /* SIMCTLCOMM_H_ */