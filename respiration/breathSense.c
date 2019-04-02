/*
 * breathSense.c
 *
 * Detect manual breath (bagging)
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <termios.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include "../comm/simCtlComm.h"
#include "../comm/simUtil.h"
#include "../comm/shmData.h"

using namespace std;

struct shmData *shmData;

char msgbuf[2048];

int type = 0;
int debug = 0;
int isDaemon = 0;
int baseline = 0;
int monitor = 0;

int main(int argc, char *argv[])
{
	int c;
	int ain;
	int sense = 0;
	int activeLoops;
	
	opterr = 0;
	
	while (( c = getopt(argc, argv, "vDm" ) ) != -1 )
	{
		switch ( c )
		{
			case 'D':
				debug++;
				break;
				
			case 'v':
				printf("Usage: %s\n", argv[0] );
				printf("\t-D : Enable debug\n" );
				exit ( 0 );
				break;
				
			case 'm':
				monitor = 1;
				debug = 1;
				break;
				
			case '?':
				if (isprint (optopt))
				  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				  fprintf (stderr,
						   "Unknown option character `\\x%x'.\n",
						   optopt);
				return 1;
				
			 default:
				fprintf (stderr, "Unhandled option `-%c'.\n", c);
				abort ();
		}	
	}
	if ( ! debug )
	{
		daemonize();
		isDaemon = 1;
	}
	initSHM(0 );

	if ( monitor )
	{
		while ( 1 )
		{
			sleep(1 );
			printf("AIN %d, Base %d, Manual %d\n",
				shmData->manual_breath_ain,
				shmData->manual_breath_baseline,
				shmData->respiration.manual_breath );
		}
	}
	while ( baseline == 0 )
	{
		baseline = read_ain(BREATH_AIN_CHANNEL );
	}
	sprintf(msgbuf, "Breath baseline: %d", baseline );
	log_message("", msgbuf); 
	shmData->manual_breath_baseline = baseline;
	
	while ( 1 )
	{
		usleep(10000 );	
		ain = read_ain(BREATH_AIN_CHANNEL );
		shmData->manual_breath_ain = ain;
		if ( ain == 0 )
		{
			continue;
		}
		if ( ain < baseline )
		{
			baseline = ain - 10;
			shmData->manual_breath_baseline = baseline;
		}
		// printf("%d\n", ain );
		switch ( sense )
		{
			case 0:
				if ( ain > (baseline + 30 ) )
				{
					sense = 1;
				}
				break;
			case 1:
				if ( ( ain < ( baseline+10 ) ) || ( activeLoops++ > 200 ) )
				{
					shmData->respiration.manual_breath = 1;
					sprintf(msgbuf, "Breath: %d, Baseline %d", ain, baseline );
					log_message("", msgbuf); 
					sense = 0;
					activeLoops = 0;
				}
				break;
		}
		if ( ain > baseline )
		{
			baseline += 1;
		}
	}
}
