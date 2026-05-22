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
#include <sys/time.h>
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

int simMgrSyncTime(void);
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

	loop_count = 30;
	int timeWasSet = 0;
	
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
			loop_count--;
			if ( loop_count < 1 )
			{
				sts = simMgrSyncTime();
				if ( sts == 0 )
				{
					timeWasSet = 1;
				}
				if ( timeWasSet )
				{
					loop_count = (60*60*100);	// 60 minutes
				}
				else
				{
					loop_count = 30;	// 0.3 seconds
				}
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
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:auscultation:side=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				aus.side );
			do_send++;
		}
		else if ( aus.row != shmData->auscultation.row ) 
		{
			aus.row = shmData->auscultation.row;
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:auscultation:row=%d", 
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				aus.row );
			do_send++;
		}
		else if ( aus.col != shmData->auscultation.col )
		{
			aus.col = shmData->auscultation.col;
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:auscultation:col=%d", 
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				aus.col );
			do_send++;
		}
		else if ( pul.right_dorsal != shmData->pulse.right_dorsal ) 
		{
			pul.right_dorsal = shmData->pulse.right_dorsal;
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:right_dorsal=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.right_dorsal );
			do_send++;
		}
		else if ( pul.left_dorsal != shmData->pulse.left_dorsal ) 
		{
			pul.left_dorsal = shmData->pulse.left_dorsal;
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:left_dorsal=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.left_dorsal );
			do_send++;
		}
		else if ( pul.right_femoral != shmData->pulse.right_femoral ) 
		{
			pul.right_femoral = shmData->pulse.right_femoral;
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:right_femoral=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.right_femoral );
			do_send++;
		}
		else if ( pul.left_femoral != shmData->pulse.left_femoral ) 
		{
			pul.left_femoral = shmData->pulse.left_femoral;
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:pulse:left_femoral=%d",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort,
				pul.left_femoral );
			do_send++;
		}

		else if ( shmData->respiration.manual_breath )
		{
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:respiration:manual_breath=1",
				shmData->simMgrIPAddr,
				shmData->simMgrStatusPort );
			shmData->respiration.manual_breath = 0;
			do_send++;
		}
		else if ( cpr.compression != shmData->cpr.compression )
		{
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:cpr:compression=%d",
				shmData->simMgrIPAddr, 
				shmData->simMgrStatusPort, 
				shmData->cpr.compression );
			cpr.compression = shmData->cpr.compression;
			do_send++;
		}
		else if ( cpr.release != shmData->cpr.release )
		{
			sprintf(simctlrWriteCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?set:cpr:release=%d",
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
 * setTimeFromComponents
 *
 * Build a struct tm from caller-supplied fields and apply it via settimeofday().
 * Returns 0 on success, -1 on failure (errno set by settimeofday).
 */
static int
setTimeFromComponents(int year, int month, int day,
                      int hour, int min, int sec,
                      const char *label)
{
	struct tm t;
	struct timeval tv;
	time_t epoch;

	memset(&t, 0, sizeof(t));
	t.tm_year  = year - 1900;
	t.tm_mon   = month - 1;   /* tm_mon is 0-based */
	t.tm_mday  = day;
	t.tm_hour  = hour;
	t.tm_min   = min;
	t.tm_sec   = sec;
	t.tm_isdst = -1;           /* let mktime() determine DST */

	epoch = mktime(&t);
	if ( epoch == (time_t)-1 )
	{
		snprintf(msgbuf, BUF_LEN_MAX, "simMgrSyncTime (%s): mktime failed", label );
		syslog(LOG_DAEMON | LOG_NOTICE, "%s", msgbuf );
		return ( -1 );
	}

	tv.tv_sec  = epoch;
	tv.tv_usec = 0;
	if ( settimeofday(&tv, NULL) != 0 )
	{
		snprintf(msgbuf, BUF_LEN_MAX, "simMgrSyncTime (%s): settimeofday failed: %s",
		         label, strerror(errno) );
		syslog(LOG_DAEMON | LOG_NOTICE, "%s", msgbuf );
		return ( -1 );
	}

	snprintf(msgbuf, BUF_LEN_MAX,
	         "simMgrSyncTime (%s): date set to %04d-%02d-%02d %02d:%02d:%02d",
	         label, year, month, day, hour, min, sec );
	syslog(LOG_DAEMON | LOG_NOTICE, "%s", msgbuf );
	return ( 0 );
}

int
simMgrSyncTime(void)
{
	FILE *pipe1;
	char buff[1024];
	char dbuff[64];
	char name[128];
	char v1[128];
	char v2[128];
	int day = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	int month = 0;   /* C3 fix: always initialised */
	int year = 0;
	int sts;
	int rval = -1;
	int len;
	int i;

	/* The simMgrIPAddr is always set via inet_ntop() and therefore contains
	 * only digits and dots — no shell metacharacters are possible here.      */
	snprintf(buff, sizeof(buff), "curl  %s:%d/cgi-bin/simstatus.cgi?date=1",
	         shmData->simMgrIPAddr, shmData->simMgrStatusPort );

	pipe1 = popen(buff, "r" );
	if ( !pipe1 )
	{
		snprintf(buff, sizeof(buff), "simMgrSyncTime: popen failed: %s", strerror(errno) );
		syslog(LOG_DAEMON | LOG_NOTICE, "%s", buff );
		return ( -1 );
	}

	/* Super-simple parse routine */
	while (fgets(dbuff, (int)sizeof(dbuff), pipe1) != NULL)
	{
		len = (int)strlen(dbuff);

		if ( (len > 20) &&
		     dbuff[0] == '"' &&
		     dbuff[1] == 'd' &&
		     dbuff[2] == 'a' &&
		     dbuff[3] == 't' &&
		     dbuff[4] == 'e' )
		{
			/* Convert quotes to spaces so sscanf sees plain tokens */
			for ( i = 0 ; i <= len ; i++ )
			{
				if ( dbuff[i] == '"' )
					dbuff[i] = ' ';
			}

			/* Try WinVetSim format: date : Fri May 14 11:00:11 2021
			 * Width limits on %s prevent overflow into name/v1/v2 (all 128 bytes). */
			sts = sscanf(dbuff, " %127s : %127s %127s %d %d:%d:%d %d",
			             name, v1, v2, &day, &hour, &min, &sec, &year );

			snprintf(msgbuf, BUF_LEN_MAX, "simMgrSyncTime: sscanf returns %d", sts );
			syslog(LOG_DAEMON | LOG_NOTICE, "%s", msgbuf );

			if ( sts == 8 )
			{
				/* C3 fix: if/else if chain ensures month is always set or
				 * the error path executes — no uninitialized use possible. */
				if      ( strncmp(v2, "Jan", 3) == 0 ) month = 1;
				else if ( strncmp(v2, "Feb", 3) == 0 ) month = 2;
				else if ( strncmp(v2, "Mar", 3) == 0 ) month = 3;
				else if ( strncmp(v2, "Apr", 3) == 0 ) month = 4;
				else if ( strncmp(v2, "May", 3) == 0 ) month = 5;
				else if ( strncmp(v2, "Jun", 3) == 0 ) month = 6;
				else if ( strncmp(v2, "Jul", 3) == 0 ) month = 7;
				else if ( strncmp(v2, "Aug", 3) == 0 ) month = 8;
				else if ( strncmp(v2, "Sep", 3) == 0 ) month = 9;
				else if ( strncmp(v2, "Oct", 3) == 0 ) month = 10;
				else if ( strncmp(v2, "Nov", 3) == 0 ) month = 11;
				else if ( strncmp(v2, "Dec", 3) == 0 ) month = 12;
				else
				{
					snprintf(msgbuf, BUF_LEN_MAX,
					         "simMgrSyncTime: unrecognised month token \"%s\"", v2 );
					syslog(LOG_DAEMON | LOG_NOTICE, "%s", msgbuf );
					break;
				}

				/* C1 fix: call settimeofday() directly — no popen("date ...") */
				if ( setTimeFromComponents(year, month, day,
				                           hour, min, sec, "WVS") == 0 )
				{
					rval = 0;
				}
			}
			else
			{
				/* Linux format: date : 051411032021.26  (MMDDhhmmYYYY.ss)
				 * Re-parse: the two-token form was already consumed above,
				 * v1 holds the date string when sts < 8.                   */
				sts = sscanf(dbuff, " %127s : %127s", name, v1 );
				if ( sts == 2 )
				{
					int lmonth = 0, lday = 0, lhour = 0, lmin = 0, lyear = 0, lsec = 0;
					int parsed = sscanf(v1, "%2d%2d%2d%2d%4d.%2d",
					                    &lmonth, &lday, &lhour, &lmin, &lyear, &lsec );

					if ( parsed == 6 &&
					     lmonth >= 1 && lmonth <= 12 &&
					     lday   >= 1 && lday   <= 31 &&
					     lyear  >= 2000 )
					{
						/* C1 fix: call settimeofday() directly — no popen("date -s ...") */
						if ( setTimeFromComponents(lyear, lmonth, lday,
						                           lhour, lmin, lsec, "Linux") == 0 )
						{
							rval = 0;
						}
					}
					else
					{
						snprintf(msgbuf, BUF_LEN_MAX,
						         "simMgrSyncTime: could not parse Linux date string (parsed=%d)", parsed );
						syslog(LOG_DAEMON | LOG_NOTICE, "%s", msgbuf );
					}
				}
				else
				{
					snprintf(msgbuf, BUF_LEN_MAX,
					         "simMgrSyncTime: sscanf returned %d, cannot extract date token", sts );
					syslog(LOG_DAEMON | LOG_NOTICE, "%s", msgbuf );
				}
			}
		}
	}
	pclose(pipe1 );
	return ( rval );
}

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
	
	sprintf(simctlrReadCmd, "curl  %s:%d/cgi-bin/simstatus.cgi?simctrldata=1", shmData->simMgrIPAddr, shmData->simMgrStatusPort );

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
			// Check for a data line; width limits prevent overflow into 128-byte buffers
			sts = sscanf(msgbuf, "%127s %127s", name, value );
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

	
