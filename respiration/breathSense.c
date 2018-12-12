/*
 * breathSense.c
 *
 * Detect manual breath (bagging)
 *
 * Copyright (C) 2016-2018 Terry Kelleher
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
