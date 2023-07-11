/*
 * chestScan.cpp
 *
 * This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
 * 
 * Copyright (c) 2019-2023 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
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
#include <unistd.h>
#include "cprI2C.h"
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

#define SUPPORT_TOF 1
#ifdef SUPPORT_TOF

#include "vl6180x.h"
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define ADDRESS_DEFAULT 0x29

void startTOF(void);
void runTOF(void);
int tofGetModel(int *model, int *revision);
int file_i2c;
VL6180X tof;

#endif

#include "../comm/simCtlComm.h"
#include "../comm/simUtil.h"
#include "../comm/shmData.h"


using namespace std;

struct shmData *shmData;

char msgbuf[2048];

int debug = 0;
#define Z_IDLE		16000
#define Z_COMPRESS	19000
#define Z_RELEASE	5000
#define X_Y_LIMIT	10000
#define CPR_HOLD	20

int main(int argc, char *argv[])
{
	int sts;
	int newData;
	int lastZ, diffZ;
	int lastX, lastY;
	int cummZ;
	int count = 0;
	int compressed = 0;
	int loop = 0;
	
	if ( ! debug )
	{
		daemonize();
	}
	else
	{
		catchFaults();
	}
	sts = initSHM(0 );
	if ( sts )
	{
		sprintf(msgbuf, "SHM Failed (%d) - Exiting", sts );
		log_message("", msgbuf );
		exit ( -1 );
	}
#ifdef SUPPORT_TOF
	startTOF();
#endif
	cprI2C cprSense(0 );
	if ( cprSense.present == 0 )
	{
		log_message("","No cprSense Found on bus - Waiting" );
		while ( 1 )
		{
			// We loop here to keep the deamon open. Makes for a cleaner shutdown.
			sleep(60);
		}
		log_message("","cprSense Found Sensor" );
	}
	
	// shmData->present = cprSense.present;
	newData = cprSense.readSensor();
	diffZ = cprSense.readingZ;
	lastX = cprSense.readingX;
	lastY = cprSense.readingY;
	lastZ = cprSense.readingZ;
	cummZ = 0;
	
	//printf("%3d:\t%05d:\t%05d\t%05d\t: %05d  %d\n", count, loop, lastZ, diffZ, cummZ, compressed );
	printf("%05d\t%05d\t%05d\t%05d  %d\n", loop, lastX, lastY, lastZ, compressed );
	usleep(10000);
	while ( 1 )
	{
		while ( ! ( newData = cprSense.readSensor() ) )
		{
			usleep(5000);
		}
		if ( newData )
		{
			loop++;
			diffZ = cprSense.readingZ - lastZ;
			lastZ = cprSense.readingZ;
			lastX = cprSense.readingX;
			lastY = cprSense.readingY;
			cummZ += diffZ;
#if 0
			if ( compressed )
			{
				count++;
				if ( count > CPR_HOLD )
				{
					compressed = 0;
					shmData->cpr.compression = 0;
					shmData->cpr.release = 0;
				}
				/*
				if ( lastZ < Z_RELEASE )
				{
					// released
					compressed = 0;
					shmData->cpr.compression = 0;
					shmData->cpr.release = 100;
				} */
			}
			else
#endif
			{
				/*
				if ( ( abs(lastX ) > X_Y_LIMIT ) || ( abs(lastY ) > X_Y_LIMIT ) )
				{
					// Large X or Y displacement indication moving the mannequin rather than possible compression
				}
				else if ( abs(lastZ) > Z_COMPRESS  )
					*/
				if ( ( abs(lastX ) > X_Y_LIMIT ) || ( abs(lastY ) > X_Y_LIMIT ) ||  abs(lastZ) > Z_COMPRESS )
				{
					compressed = 1;
					shmData->cpr.compression = 1;
					shmData->cpr.release = 0;
					count = 0;
				}
				else
				{
					// If we are short of the Z_COMPRESS threshold, limit the compression to 200 ms.
					count++;
					if ( count > CPR_HOLD )
					{
						compressed = 0;
						shmData->cpr.compression = 0;
						shmData->cpr.release = 50;
					}
				}
			}
			if (  debug &&  ( compressed || ( abs(diffZ) > 1000 ) ) )
			{
				//printf("%3d:\t%05d:\t%05d\t%05d\t: %05d  %d\n", count, loop, lastZ, diffZ, cummZ, compressed );
				printf("%05d\t%05d\t%05d\t%05d  %d\n", loop, lastX, lastY, lastZ, compressed );
			}
			shmData->cpr.x = lastX;
			shmData->cpr.y = lastY;
			shmData->cpr.z = lastZ;
		}
		usleep(20000);
	}

	return 0;
}

#ifdef SUPPORT_TOF
void
startTOF(void)
{
	char filename[32];
	int model = 0;
	int revision = 0;
	int sts;
	
	shmData->cpr.tof_present = 0;
	sprintf(filename,"/dev/i2c-%d", 2);
	
	sts = getI2CLock();
	if ( sts )
	{
		releaseI2CLock();
		printf("runTOF::readRegister: Could not get I2C Lock\n" );
		return;
	}
	if ((file_i2c = open(filename, O_RDWR)) < 0)
	{
		// No ToF Found
		releaseI2CLock();
		return;
	}
	if (ioctl(file_i2c, I2C_SLAVE, ADDRESS_DEFAULT) < 0)
	{
		fprintf(stderr, "Failed to acquire bus access or talk to slave\n");
		releaseI2CLock();
		close(file_i2c);
		file_i2c = -1;
		return;
	}
	
	tof.setDev(file_i2c );
	tof.setAddress((uint8_t)ADDRESS_DEFAULT );
	revision = tof.readReg32Bit((uint16_t)0x0001);
	model = tof.readReg((uint16_t)0x000 );
	releaseI2CLock();
	
	sprintf(msgbuf, "VL63L0X: Model %02xh Revision %02xh", model, revision );
	log_message("", msgbuf );
	
	if ( model != 0xB4 )
	{
		// Not a VL6810
		sprintf(msgbuf, "VL63L0X: Model %02xh is not B4h", model );
		log_message("", msgbuf );
		return;
	}
	shmData->cpr.tof_present = 1;
	shmData->cpr.distance = 0;
	shmData->cpr.maxDistance = 0;
	
	pid_t pid = fork(); /* Create a child process */

	switch (pid) {
		case -1: /* Error */
			exit(1);
			
		case 0: /* Child process */
			runTOF();
			break;
			
		default: /* Parent process */
			break;
	}
}

void
runTOF(void)
{
	int distance;
	int sts;

	usleep(100000);
	sts = getI2CLock();
	tof.init();
	tof.configureDefault();
	tof.setPtpOffset(22);
	tof.setTimeout(500);
	releaseI2CLock();
	
	while ( 1 ) // read values 20 times a second
	{
		sts = getI2CLock();
		if ( sts == 0 )
		{
			distance = tof.readRangeSingle();
			releaseI2CLock();
			if ( tof.timeoutOccurred() )
			{
				shmData->cpr.tof_present += 1;
			}
			else
			{
				//if (distance > 0 && distance < 255) // valid range?
				{
					shmData->cpr.distance = distance;
					if ( distance > shmData->cpr.maxDistance )
					{
						shmData->cpr.maxDistance = distance;
					}
				}
			}
		}
		usleep(50000); // 50ms
	}
} 
#endif
