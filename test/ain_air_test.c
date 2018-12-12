/*
 * ain.c
 *
 * Test AIN and Lung Control
 * 
 *
 * Copyright (C) 2016-2017 Terry Kelleher
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
#include "../comm/simUtil.h"

using namespace std;
struct shmData *shmData;
int debug = 2;
char msgbuf[2048];

int main(int argc, char *argv[])
{
	int c;
	int ain0;
	int ain1;
	int ain2;
	int ain3;
	int ain4;
	int ain5;
	FILE *tankPin;
	FILE *riseLPin;
	FILE *riseRPin;
	FILE *fallPin;
	int testPhase = 0;
	
	opterr = 0;
	
	while (( c = getopt(argc, argv, "vD" ) ) != -1 )
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
	
	// Controls for Chest Rise/Fall
	tankPin = gpioPinOpen(45, GPIO_OUTPUT );
	riseLPin = gpioPinOpen(23, GPIO_OUTPUT );
	riseRPin = gpioPinOpen(67, GPIO_OUTPUT );
	fallPin = gpioPinOpen(68, GPIO_OUTPUT );

	while ( 1 )
	{
		
		usleep(250000 );	
		
		// Test Air Control Output Lines
		gpioPinSet(tankPin, GPIO_TURN_OFF );
		gpioPinSet(riseLPin, GPIO_TURN_OFF );
		gpioPinSet(riseRPin, GPIO_TURN_OFF );
		gpioPinSet(fallPin, GPIO_TURN_OFF );
		switch ( testPhase )
		{
			case 0:
				gpioPinSet(tankPin, GPIO_TURN_ON );
				break;
			case 1:
				gpioPinSet(riseLPin, GPIO_TURN_ON );
				break;
			case 2:
				gpioPinSet(riseRPin, GPIO_TURN_ON );
				break;
			case 3:
				gpioPinSet(fallPin, GPIO_TURN_ON );
				break;
			case 4:
				// Test All AIN lines
				ain0 = read_ain(BREATH_AIN_CHANNEL );
				ain1 = read_ain(TOUCH_SENSE_AIN_CHANNEL_1 );
				ain2 = read_ain(AIR_PRESSURE_AIN_CHANNEL );
				ain3 = read_ain(TOUCH_SENSE_AIN_CHANNEL_2 );
				ain4 = read_ain(TOUCH_SENSE_AIN_CHANNEL_3 );
				ain5 = read_ain(TOUCH_SENSE_AIN_CHANNEL_4 );
				printf("%d\t%d\t%d\t%d\t%d\t%d\n", ain0, ain1, ain2, ain3, ain4, ain5 );
				break;
		}
		if ( testPhase++ > 4 )
		{
			testPhase = 0;
		}
	}
}
