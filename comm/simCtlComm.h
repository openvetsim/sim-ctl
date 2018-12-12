/*
 * simCtlComm.h
 *
 * Definition of a class to for system communications in the simCtl
 *
 * Copyright (C) 2016 Terry Kelleher
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL I
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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