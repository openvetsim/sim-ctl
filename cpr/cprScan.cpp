/*
 * chestScan.cpp
 *
 * Definition of a class to interface with the Simulator Chest Sensors
 *
 * Copyright Terry Kelleher
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

#include "../comm/simCtlComm.h"
#include "../comm/simUtil.h"
#include "../comm/shmData.h"

using namespace std;

struct shmData *shmData;

char msgbuf[2048];

int debug = 0;
#define Z_IDLE		16000
#define Z_COMPRESS	30000
#define Z_RELEASE	5000
#define X_Y_LIMIT	12000
#define CPR_HOLD	10

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
				if ( ( abs(lastX ) > X_Y_LIMIT ) || ( abs(lastY ) > X_Y_LIMIT ) )
				{
					// Large X or Y displacement indication moving the mannequin rather than possible compression
				}
				else if ( abs(lastZ) > Z_COMPRESS  )
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
