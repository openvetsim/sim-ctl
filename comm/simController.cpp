/*
 * simController.cpp
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
#include <iostream>
#include <string>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <termios.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>

/*
#include <libxml/xmlreader.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
*/

#include "simUtil.h"
#include "shmData.h"

using namespace std;

#define SEC_NONE		0
#define SEC_CARDIAC		1
#define SEC_RESPIRATION	2
#define SEC_GENERAL		3

struct shmData *shmData;
#define BUF_LEN_MAX	4096
char msgbuf[BUF_LEN_MAX+4];
char simctlrReadCmd[BUF_LEN_MAX+4];
char simctlrWriteCmd[BUF_LEN_MAX+4];

void simMgrSyncTime(void);
void simMgrRead(void );
void simMgrWrite(void );
void initializeSensorData(void );

int debug = 0;

#ifdef DO_DEAMON_STARTS
	// This section is as yet untested. I need to create a "clean up" function
	// to handle service stop before implementing.
int pulsePid = 0;
int rfidPid = 0;
int soundPid = 0;
int breathPid = 0;
int cprPid = 0;

pid_t
startProcess(const char *cmd, const char *arg )
{
	pid_t pid = fork(); /* Create a child process */

	switch (pid) {
		case -1: /* Error */
			std::cerr << "Uh-Oh! fork() failed.\n";
			exit(1);
		case 0: /* Child process */
			execl(programPath, arg); /* Execute the program */
			std::cerr << "Uh-Oh! execl() failed!"; /* execl doesn't return unless there's an error */
			exit(1);
		default: /* Parent process */
			break;
	}
	return ( pid );
}
#endif // DO_DEAMON_STARTS

int main(int argc, char *argv[])
{
	int sts;
	int loop_count;
	
	// Do GPIO Pin configurations
	system("config-pin P9.24 uart" );	// UART1 - For rfidScan
    system("config-pin P9.26 uart" );	// UART1 - For rfidScan
	system("config-pin P9.21 uart" );	// UART2 - For soundSense
	system("config-pin P9.22 uart" );	// UART2 - For soundSense

	if ( debug )
	{
		catchFaults();
	}
	else
	{
		daemonize();
	}
	
	sts = initSHM(SHM_CREATE );
	if ( sts )
	{
		snprintf(msgbuf, BUF_LEN_MAX, "SHM Failed (%d) - Exiting", sts );
		log_message("", msgbuf );
		exit ( -1 );
	}

	shmData->cardiac.rate = 80;
	shmData->respiration.awRR = 50;
	shmData->respiration.manual_breath = 0;
	shmData->respiration.active = 0;
	shmData->cpr.last = 0;
	shmData->cpr.compression = 0;
	shmData->cpr.release = 0;
	shmData->cpr.duration = 0;
	shmData->auscultation.heartTrim = 0;
	shmData->auscultation.lungTrim = 0;
	sem_init(&shmData->i2c_sema, 1, 1 ); // pshared =1, value =1
	
	sts = getI2CLock();
	if ( sts )
	{
		log_message("", "getI2CLock returns fail");
	}
	releaseI2CLock();
	
	initializeSensorData();
	
#ifdef DO_DEAMON_STARTS
	// Start the other deamons
	pulsePid = startProcess("/usr/local/bin/pulse" );
	rfidPid = startProcess("/usr/local/bin/rfidScan" );
	soundPid = startProcess("/usr/local/bin/soundSense ttyO2" );
	breathPid = startProcess("/usr/local/bin/breathSense" );
	cprPid = startProcess("/usr/local/bin/cprScan" );
#endif // DO_DEAMON_STARTS

	loop_count = 0;
	while ( 1 )
	{
		if ( shmData->simMgrStatusPort != 0 )
		{
			if ( ( loop_count & 1 ) == 0 )
			{
				simMgrRead();
			}
			else
			{
				simMgrWrite();
			}
			loop_count++;
			if ( loop_count > 3000 )
			{
				// simMgrSyncTime();
				loop_count = 0;
			}
		}
		usleep(100000 );
	}
}
/*
 * look for updates in sensors and send changes
*/
struct auscultation 	aus;
struct pulse 			pul;
struct cpr 				cpr;
struct defibrillation 	def;

void
initializeSensorData(void )
{
	aus.side = 0;
	aus.row = 0;
	aus.col = 0;

	pul.right_dorsal = PULSE_TOUCH_NONE;
	pul.left_dorsal = PULSE_TOUCH_NONE;
	pul.right_femoral = PULSE_TOUCH_NONE;
	pul.left_femoral = PULSE_TOUCH_NONE;
	
	cpr.last = 0;
	cpr.compression = 0;
	cpr.release = 0;
	cpr.duration = 0;
	
	def.last = 0;
	def.energy = 0;
	
}
char ampChar[] = "%26";

void
simMgrWrite(void )
{
	FILE *pipe;
	int do_send;
	while ( 1 ) 
	{
		do_send = 0;
		if ( aus.side != shmData->auscultation.side )
		{
			aus.side = shmData->auscultation.side;
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:auscultation:side=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				aus.side );
			do_send++;
		}
		else if ( aus.row != shmData->auscultation.row ) 
		{
			aus.row = shmData->auscultation.row;
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:auscultation:row=%d", 
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				aus.row );
			do_send++;
		}
		else if ( aus.col != shmData->auscultation.col )
		{
			aus.col = shmData->auscultation.col;
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:auscultation:col=%d", 
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				aus.col );
			do_send++;
		}
		else if ( pul.right_dorsal != shmData->pulse.right_dorsal ) 
		{
			pul.right_dorsal = shmData->pulse.right_dorsal;
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:right_dorsal=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.right_dorsal );
			do_send++;
		}
		else if ( pul.left_dorsal != shmData->pulse.left_dorsal ) 
		{
			pul.left_dorsal = shmData->pulse.left_dorsal;
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:left_dorsal=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.left_dorsal );
			do_send++;
		}
		else if ( pul.right_femoral != shmData->pulse.right_femoral ) 
		{
			pul.right_femoral = shmData->pulse.right_femoral;
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:right_femoral=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.right_femoral );
			do_send++;
		}
		else if ( pul.left_femoral != shmData->pulse.left_femoral ) 
		{
			pul.left_femoral = shmData->pulse.left_femoral;
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:left_femoral=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.left_femoral );
			do_send++;
		}

		else if ( shmData->respiration.manual_breath )
		{
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:respiration:manual_breath=1",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort );
			shmData->respiration.manual_breath = 0;
			do_send++;
		}
		else if ( cpr.compression != shmData->cpr.compression )
		{
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:cpr:compression=%d",
				shmData->simMgrIPAddr, 
				shmData->simMgrStatusPort, 
				shmData->cpr.compression );
			cpr.compression = shmData->cpr.compression;
			do_send++;
		}
		else if ( cpr.release != shmData->cpr.release )
		{
			sprintf(simctlrWriteCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?set:cpr:release=%d",
				shmData->simMgrIPAddr, 
				shmData->simMgrStatusPort,
				shmData->cpr.release );
			cpr.release = shmData->cpr.release;
			do_send++;
		}
#if 0
		else if ( ( def.last != shmData->defibrillation.last ) ||
			 ( def.energy != shmData->defibrillation.energy ) )
		{
			do_send++;
		}
#endif
		if ( do_send )
		{
			//log_message("", simctlrWriteCmd );
			pipe = popen(simctlrWriteCmd, "r" );
			if ( !pipe )
			{
				perror("popen" );
				exit ( 1 );
			}
			else
			{
				while (fgets(msgbuf, BUF_LEN_MAX, pipe) != NULL)
				{
					// Could parse the return, but not really needed.
					//log_message("", msgbuf );
				}
				pclose(pipe );
			}
		}
		else
		{
			break;
		}
	}
}
/*
void
simMgrSyncTime(void)
{
	FILE *pipe1;
	FILE *pipe2;
	char buff[1024];
	char dbuff[64];
	char name[128];
	char v1[128];
	char v2[128];
	char v3[128];
	char v4[128];
	char v5[128];
	char v6[128];
	char v7[128];
	int i;
	int sts;

	sprintf(buff, "simCurl  %s:%d/cgi-bin/simstatus.cgi?date=1", shmData->simMgrIPAddr, shmData->simMgrStatusPort );
	log_message("", buff );
	pipe1 = popen(buff, "r" );
	if ( !pipe1 )
	{
		sprintf(buff, "Get Date fails: %s", strerror(errno ) );
		syslog (LOG_DAEMON | LOG_NOTICE, buff );
	}
	else
	{
// Super-simple parse routine

		while (fgets(dbuff, 64, pipe1) != NULL)
		{
			if ( ( strlen(dbuff) > 20 ) &&
				dbuff[0] == '"' &&
				dbuff[1] == 'd' &&
				dbuff[2] == 'a' &&
				dbuff[3] == 't' &&
				dbuff[4] == 'e' )
			{
				log_message("", dbuff );
			
				// WinVetSim format: "date":"Fri May 14 11:00:11 AM" 
				// Linux format: "date":"051411032021.26"
				sts = sscanf(dbuff, "\"%s\":\"%s %s %s %s:%s:%s %s\"", 
								name, v1, v2, v3, v4, v5, v6, v7 );
				if ( sts == 2 )
				{
					// Linux
					snprintf(buff, 1024, "date -s \"%s\"", v1 );
					pipe2 = popen(buff, "r" );
					if ( !pipe2 )
					{
						snprintf(buff, 1024, "set Date fails: %s", strerror(errno ) );
						syslog (LOG_DAEMON | LOG_NOTICE, buff );
					}
					else
					{
						pclose(pipe2 );
						syslog (LOG_DAEMON | LOG_NOTICE, buff );
					}
				}
				else if ( sts == 7 )
				{
					// WinVetSim
					snprintf(buff, 1024, "date \"%s\"", v1 );
					pipe2 = popen(buff, "r" );
					if ( !pipe2 )
					{
						snprintf(buff, 1024, "set Date fails: %s", strerror(errno ) );
						syslog (LOG_DAEMON | LOG_NOTICE, buff );
					}
					else
					{
						pclose(pipe2 );
						syslog (LOG_DAEMON | LOG_NOTICE, buff );
					}
				}
			}
		}
		pclose(pipe1 );
	}
}
*/
int maxLog = 0;
void
simMgrRead(void )
{
	FILE *pipe;
	int section;
	int i;
	int sts;
	char name[128];
	char value[128];
	
	sprintf(simctlrReadCmd, "simCurl  %s:%d/cgi-bin/simstatus.cgi?simctrldata=1", shmData->simMgrIPAddr, shmData->simMgrStatusPort );

	pipe = popen(simctlrReadCmd, "r" );
	if ( !pipe )
	{
		perror("popen" );
		exit ( 1 );
	}
	else
	{
		// Super-simple parse routine
		section = SEC_NONE;
		while (fgets(msgbuf, BUF_LEN_MAX, pipe) != NULL)
		{
			for ( i = 0 ; msgbuf[i] != 0 ; i++ )
			{
				switch ( msgbuf[i] )
				{
					case ':':
					case '"':
					case '}':
					case '{':
					case ',':
						msgbuf[i] = ' ';
						break;
				}
			}
			// Check for a data line
			sts = sscanf(msgbuf, "%s %s", name, value );
			if ( sts == 1 )
			{
				// printf("Section: '%s'\n", name );
				if ( strcmp(name, "cardiac" ) == 0 )
				{
					section = SEC_CARDIAC;
				}
				else if ( strcmp(name, "respiration" ) == 0 )
				{
					section = SEC_RESPIRATION;
				}
				else
				{
					section = SEC_NONE;
				}
				
			}
			else if ( sts == 2 )
			{
				switch ( section )
				{
					case SEC_NONE:
						if ( debug > 1)
						{
							printf("none: '%s', Value '%s'\n", name, value );
						}
						break;
					case SEC_CARDIAC:
						if ( debug > 1)
						{
							printf("cardiac: '%s', Value '%s'\n", name, value );
						}
						cardiac_parse(name,  value, &shmData->cardiac );
						break;
					case SEC_RESPIRATION:
						if ( debug > 1 )
						{
							printf("respiration: '%s', Value '%s'\n", name, value );
						}
						respiration_parse(name,  value, &shmData->respiration );
						break;
				}
			}

		}
		pclose(pipe );
	}
}

	