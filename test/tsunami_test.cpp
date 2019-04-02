/*
 * Wave Trigger Test
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
 
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include "../wav-trig/wavTrigger.h"

#include "../comm/simUtil.h"

#include <sys/time.h>

using namespace std;

#define MAX_BUF	255

char sioName[MAX_BUF];

int
setTermios(int fd, int speed )
{
	struct termios tty;
	
	memset (&tty, 0, sizeof tty);
	if ( tcgetattr (fd, &tty) != 0)
	{
		perror("tcgetattr" );
		return -1;
	}

	cfsetspeed(&tty, speed);
	cfmakeraw(&tty);
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8 | CLOCAL | CREAD;
	tty.c_iflag &=  ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		perror("tcsetattr");
		return -1;
	}
	return 0;
}

struct termios oldt;
struct termios newt;
int oldf;

struct termios oldtty;

int saveTTY(int sfd)
{
	tcgetattr(sfd, &oldtty);

	return ( 0 );
}
int
restoreTTY(int sfd )
{
	tcsetattr(sfd, TCSANOW, &oldtty);
	return ( 0 );
}	


struct sounds
{
	int index;
	char name[32];
};
/*
struct sounds soundList[] =
{
	{ 1, "Sweeper" },
	{ 2, "Mountain" },
	{ 3, "Dialog1" },
	{ 4, "Dialog2" },
	{ 5, "Horn" },
	{ 6, "Flute C4" },
	{ 7, "Flute E4" },
	{ 8, "Flute G4" },
	{ 9, "A440" },
	{ 0, "" }
};
*/
int
main(int argc, char *argv[] )
{
	int sfd;
	char buffer[MAX_BUF+1];
	int val;
	int c;
	
	wavTrigger wav;
	
	if ( argc < 2 )
	{
		cout << "Usage:\n";
		cout << argv[ 0 ] << " <tty port>\n";
		cout << "eg: " << argv[ 0 ] << " tty1\n";
		return (-1 );
	}
	
	sprintf(sioName, "/dev/%s", argv[1] );
	
	sfd = open(sioName, O_RDWR | O_NOCTTY | O_SYNC );
	if ( sfd < 0 )
	{
		fprintf(stderr, "error %d opening %s: %s\n", errno, sioName, strerror (errno));
		return (-1 );
	}
	saveTTY(sfd );
	setTermios(sfd, B57600); // WAV Trigger defaults to 57600
	
	wav.start(sfd );
	usleep(500000);
	printf("Started\n" );
	val = wav.getVersion(buffer, MAX_BUF );
	printf("WAV Trigger Version: Len %d String %s\n", val, &buffer[1] );

	val = wav.getSysInfo(buffer, MAX_BUF );
	printf("Sys Info: Len %d Voices %d Tracks %d\n", val, buffer[1], buffer[2] );
	
	wav.ampPower(0 );
	wav.masterGain(-8);

#define LEFT_DORSAL_CHAN	4
#define RIGHT_DORSAL_CHAN	5
#define LEFT_FEMORAL_CHAN	2
#define RIGHT_FEMORAL_CHAN	3
#define SPK_1_CHAN			6
#define SPK_2_CHAN			7
#define HEADSET_CHAN		0

	while ( 1 )
	{
		c = getchar();
		wav.stopAllTracks();
		switch ( c )
		{
			case '1':
				printf("Left Dorsal\n" );
				wav.trackPlaySolo(LEFT_DORSAL_CHAN, 1);
				break;
			case '2':
				printf("Right Dorsal\n" );
				wav.trackPlaySolo(RIGHT_DORSAL_CHAN, 1);
				break;
			case '3':
				printf("Left Femoral\n" );
				wav.trackPlaySolo(LEFT_FEMORAL_CHAN, 1);
				break;
			case '4':
				printf("Right Femoral\n" );
				wav.trackPlaySolo(RIGHT_FEMORAL_CHAN, 1);
				break;
			case '5':
				printf("Speaker 1\n" );
				wav.trackPlaySolo(SPK_1_CHAN, 1);
				break;
			case '6':
				printf("Speaker 2\n" );
				wav.trackPlaySolo(SPK_2_CHAN, 1);
				break;
			case '7':
				printf("Headset\n" );
				wav.trackPlaySolo(HEADSET_CHAN, 1);
				break;
			case 'q': case 'Q':
				printf("Quit\n" );
				restoreTTY(sfd );
				exit (0 );
		}
	}
}
