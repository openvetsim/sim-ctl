/*
 * pulse.c
 *
 * Functions used for pulse
 *
 * Second version. Uses Tsunami audio and Bone Conduction speakers.
 * In this version, the pulse sound is handled in the soundSense daemon. 
 * This process monitors the pulse touch sensors.
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
//#include "../comm/simCtlComm.h"
#include "../comm/simUtil.h"
#include "../comm/shmData.h"



using namespace std;

struct shmData *shmData;

void init_touch_sensors(void );
void read_touch_sensors(void );
void read_touch_sensor(int chan );

char msgbuf[2048];

int type = 0;
int debug = 0;
int isDaemon = 0;

/*
 * Auto Calibration for Pulse Touch Sensors
 *
 * 1: At startup, the initial reading of the Touch Sensor is used as a baseline.
 * 2: The Sensor reading decreases as pressure increases.
 * 3: An excessive touch is detected as a reading that is SENSE_EXCESS less than the baseline
 * 4: A heavy touch is detected as a reading that is SENSE_HI less than the baseline
 * 5: A normal touch is detected as reading that is SENSE_MID less than the baseline
 * 6: A light touch is detected as reading that is SENSE_LO less than the baseline
 * 7: The baseline is adjusted by 1 every sample, to track any shifts in the baseline
*/

#define SENSE_EXCESS		1200
#define SENSE_HI			1000
#define SENSE_MID			500
#define SENSE_LO			250
#define SENSE_OFFSET_ADJUST	20

struct senseChans
{
	int ainChannel;
	int position;
	int last;
	int ain;
	int baseline;
};

struct senseChans senseChannels [] =
{
	{ TOUCH_SENSE_AIN_CHANNEL_1, PULSE_LEFT_FEMORAL,  0, 0, 0 },
	{ TOUCH_SENSE_AIN_CHANNEL_2, PULSE_RIGHT_FEMORAL, 0, 0, 0 },
	{ TOUCH_SENSE_AIN_CHANNEL_3, PULSE_LEFT_DORSAL,   0, 0, 0 },
	{ TOUCH_SENSE_AIN_CHANNEL_4, PULSE_RIGHT_DORSAL,  0, 0, 0 } 
};

int main(int argc, char *argv[])
{
	int sts;
	int c;
	int loops;
	
	opterr = 0;
	
	while (( c = getopt(argc, argv, "hD" ) ) != -1 )
	{
		switch ( c )
		{
			case 'D':
				debug++;
				break;
				
			case 'h':
				printf("Usage: %s [-D]\n", argv[0] );
				printf("\t-D : Enable debug\n" );
				exit ( 0 );
				break;
				
			case '?':
				if (optopt == 'c')
				  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
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
	
	
		sts = initSHM(SHM_OPEN );
		if ( sts  )
		{
			perror("initSHM");
			return (-1 );
		}
	}
	init_touch_sensors();
	
	if ( debug )
	{
		printf("Starting Loop\n");
	}
	
	while ( 1 )
	{
		read_touch_sensors();

		if ( debug && ( loops++ >= 10 ) )
		{
			sprintf(msgbuf, "sense %d %d %d %d %d %d %d %d", 
					senseChannels[0].ain,
					senseChannels[1].ain,
					senseChannels[2].ain,
					senseChannels[3].ain,
					senseChannels[0].last,
					senseChannels[1].last,
					senseChannels[2].last,
					senseChannels[3].last );
			if ( debug )
				printf("%s\n", msgbuf );
			else
				log_message("", msgbuf );
	
			loops = 0;
		}
		usleep(200000 );
	}
	if ( isDaemon )
	{
		sprintf(msgbuf, "Exited Loop: %s\n", strerror(errno ) );
		log_message("", msgbuf);
	}
	printf("Exited Loop: %s\n", strerror(errno ) );
	return 0;
}
void 
init_touch_sensors(void )
{
	int chan;
	int sensor;
	int position;
	
	for ( chan = 0 ; chan < 4 ; chan++ )
	{
		sensor = read_ain(senseChannels[chan].ainChannel );
		position = senseChannels[chan].position;
		if ( ! debug )
		{
			senseChannels[chan].baseline = sensor;
			shmData->pulse.base[position] = sensor;
			shmData->pulse.ain[position] = sensor;
			shmData->pulse.touch[position] = 0;
		}
		if ( debug )
		{
			printf("Chan %d, Baseline %d\n",
				chan, senseChannels[chan].baseline );
		}
	}
}
const char *positions[] = {
	"None",
	"Right Dorsal",
	"Right Femoral",
	"Left Dorsal",
	"Left Femoral" 
};

const char *touches[] = {
	"None",
	"Light",
	"Normal",
	"Heavy",
	"Excessive" 
};

void 
read_touch_sensors(void )
{
	int chan;
	int pressure;
	
	for ( chan = 0 ; chan < 4 ; chan++ )
	{
		read_touch_sensor(chan );
	
		pressure = senseChannels[chan].last;
		if ( ! debug )
		{
			switch ( senseChannels[chan].position )
			{
				case PULSE_RIGHT_DORSAL:
					shmData->pulse.right_dorsal = pressure;
					break;
				case PULSE_LEFT_DORSAL:
					shmData->pulse.left_dorsal = pressure;
					break;
				case PULSE_RIGHT_FEMORAL:
					shmData->pulse.right_femoral = pressure;
					break;
				case PULSE_LEFT_FEMORAL:
					shmData->pulse.left_femoral = pressure;
					break;
				default:
					break;
			}
		}
	}
}
void
read_touch_sensor(int chan )
{
	int sensor;
	int diff;
	int on;
	int ainChan = senseChannels[chan].ainChannel ;
	int position = senseChannels[chan].position ;
	
	sensor = read_ain(ainChan );
	senseChannels[chan].ain = sensor;
	
	if ( ! debug )
	{
		shmData->pulse.ain[position] = sensor;
	}
	if ( ( sensor > 4096 ) || ( sensor < 0 ) )
	{
		sprintf(msgbuf, "bad sensor read %d", sensor );
		if ( debug )
		{
			printf("%s\n", msgbuf );
		}

		senseChannels[chan].last = PULSE_TOUCH_NONE;
	}
	else
	{
		// Adjustments to the baseline
		if ( sensor > senseChannels[chan].baseline )
		{
			senseChannels[chan].baseline = sensor;
		}
		else if ( sensor < ( senseChannels[chan].baseline - SENSE_OFFSET_ADJUST ) )
		{
			senseChannels[chan].baseline -= 5;
		}
		
		diff = senseChannels[chan].baseline - sensor;

		if ( diff > SENSE_EXCESS )	// Excessive touch
		{
			on = PULSE_TOUCH_EXCESSIVE;
		}
		else if ( diff > SENSE_HI )		// Heavy touch
		{
			on = PULSE_TOUCH_HEAVY ;
		}
		else if ( diff > SENSE_MID  )	// Good touch
		{
			on = PULSE_TOUCH_NORMAL;
		}
		else if (  diff > SENSE_LO )	// light touch
		{
			on = PULSE_TOUCH_LIGHT;
		}
		else	// No touch
		{
			on = PULSE_TOUCH_NONE;
		}
		senseChannels[chan].last = on;
	
		if ( ! debug )
		{
			shmData->pulse.base[position] = senseChannels[chan].baseline;
		}
	}
	if ( ! debug )
	{
		shmData->pulse.touch[position] = senseChannels[chan].last;
	}
}
