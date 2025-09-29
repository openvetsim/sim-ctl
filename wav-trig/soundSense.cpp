/*
 * soundSense.cpp
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
 *
 * 4.12.2023 - Removed support for Dorsal Pulses
*/

#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <signal.h>
#include <stdint.h>

#include <iomanip>
#include <iostream>
#include <vector>
#include <string>  
#include <cstdlib>
#include <sstream>
#include <cmath>

#define TURN_ON			GPIO_TURN_ON
#define TURN_OFF		GPIO_TURN_OFF


#include "wavTrigger.h"
#include "../cardiac/rfidScan.h"

#include "../comm/simCtlComm.h"
#include "../comm/simUtil.h"
#include "../comm/shmData.h"

wavTrigger wav;
wavTrigger wav2;
wavTrigger *wavPulse;

simCtlComm comm;

struct shmData *shmData;

char msgbuf[1024];

/* str_thdata
	structure to hold data to be passed to a thread
*/
typedef struct str_thdata
{
    int thread_no;
    char message[100];
} thdata;

pthread_t threadInfo1;
pthread_t threadInfo2;
pthread_t threadInfo3;

/* prototype for thread routines */
void *sync_thread ( void *ptr );
void runHeart(void );
void lungFall(int control );
void lungRise(int control );
void runLung(void );
void initialize_timers(void );
timer_t heart_timer;
timer_t breath_timer;
timer_t rise_timer;
struct sigevent heart_sev;
struct sigevent breath_sev;
struct sigevent rise_sev;
	
time_t fallStopTime = 0;

#define HEART_TIMER_SIG		(SIGRTMIN+2)
#define BREATH_TIMER_SIG	(SIGRTMIN+3)
#define RISE_TIMER_SIG		(SIGRTMIN+4)

#define SOUND_LOOP_DELAY	20000	// Delay in usec

using namespace std;

#define MAX_BUF	255

char sioName[2][MAX_BUF];

int debug = 0;
int ldebug = 0;
int monitor = 0;
int soundTest = 0;

void runMonitor(void );

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

#define MIN_VOLUME		-70
#define MAX_VOLUME		5
#define MAX_MAX_VOLUME	8

#define PULSE_VOLUME_OFF		MIN_VOLUME
#define PULSE_VOLUME_VERY_SOFT	MAX_VOLUME // -20
#define PULSE_VOLUME_SOFT		MAX_VOLUME // -10
#define PULSE_VOLUME_ON			5

#define PULSE_TRACK				103
#define PULSE_TRACK_LEFT		104
#define PULSE_TRACK_RIGHT		105

int getPulseVolume(int pressure, int strength );
void doPulse(void);

int heartPlaying = 0;
int lungPlaying = 0;

char trkBuf[256];
	
struct current
{
	int heart_sound_volume;
	int heart_sound_mute;
	int heart_rate;
	char heart_sound[32];
	
	int left_lung_sound_volume;
	int left_lung_sound_mute;
	char left_lung_sound[32];
	int right_lung_sound_volume;
	int right_lung_sound_mute;
	char right_lung_sound[32];
	int respiration_rate;
	
	unsigned int heartCount;
	unsigned int breathCount;
	
	int masterGain;
	int leftLungGain;
	int rightLungGain;
	int heartGain;
	
	int heartStrength;
	int leftLungStrength;
	int rightLungStrength;
	int pea;
};

struct current current;

int lubdub = 0;
int inhL = 0;
int inhR = 0;

#define SOUND_TYPE_UNUSED	0
#define SOUND_TYPE_HEART	1
#define SOUND_TYPE_LUNG		2
#define SOUND_TYPE_PULSE	3
#define SOUND_TYPE_GENERAL	4

#define SOUND_TYPE_LENGTH	8
struct soundType
{
	int type;
	char typeName[SOUND_TYPE_LENGTH];
};
struct soundType soundTypes[] =
{
	{ SOUND_TYPE_UNUSED,	"unused" },
	{ SOUND_TYPE_HEART,		"heart" },
	{ SOUND_TYPE_LUNG,		"lung" },
	{ SOUND_TYPE_PULSE,		"pulse" },
	{ SOUND_TYPE_GENERAL,	"general" }
};

int
typeNameToIndex(char *typeName )
{
	int i;
	
	for ( i = 0 ; i < 5 ; i++ )
	{
		if ( strcmp(typeName, soundTypes[i].typeName ) == 0 )
		{
			return ( soundTypes[i].type );
		}
	}
	return -1;
}
#define SOUND_NAME_LENGTH	32
struct sound
{
	int type;
	int index;
	char name[SOUND_NAME_LENGTH];
	int low_limit;
	int high_limit;
};

// The Tsunami is capable of supporting up to 4096 tracks
#define SOUND_NUM_TRACKS		(4096)
int maxSounds = 0;
struct sound *soundList;
int soundIndex = 0;

int
addSoundToList(int type, int index, const char *name, int low_limit, int high_limit )
{
	int sts;
	if ( soundIndex >= maxSounds )
	{
		snprintf(msgbuf, 1024, "Too many tracks in sound list" );
		log_message("", msgbuf );
		sts = -1;
	}
	else if ( soundList[soundIndex].type == SOUND_TYPE_UNUSED )
	{
		soundList[soundIndex].type = type;
		soundList[soundIndex].index = index;
		memcpy(soundList[soundIndex].name, name, SOUND_NAME_LENGTH );
		soundList[soundIndex].low_limit = low_limit;
		soundList[soundIndex].high_limit = high_limit;
		soundIndex++;
		sts = 0;
	}
	else
	{
		snprintf(msgbuf, 1024, "bad entry in sound list" );
		log_message("", msgbuf );
		sts = -1;
	}
	return ( sts );
}
#define LINE_MAX_LEN	512
int
initSoundList(void )
{
	FILE *file;
	int lines;
	char line[LINE_MAX_LEN];
	char *in;
	char *out;
	char clean[1060];
	struct sound sd;
	char typeName[SOUND_TYPE_LENGTH];
	int sts;
	
	file = fopen("/simulator/soundList.csv", "r" );
	if ( file == NULL )
	{
		if ( debug )
		{
			perror("fopen" );
			fprintf(stderr, "Failed to open /simulator/soundList.csv\n" );
		}
		else
		{
			snprintf(msgbuf, 1024, "Failed to open /simulator/soundList.csv %s", strerror(errno) );
			log_message("", msgbuf);
		}
		exit ( -2 );
	}
	
	for ( lines = 0 ; fgets(line, LINE_MAX_LEN, file ) ; lines++ )
	{
	}
	printf("Found %d lines in file\n", lines );
	maxSounds = lines + 1;
	soundList = (struct sound *)calloc(maxSounds, sizeof(struct sound) );
	
	fseek(file, 0, SEEK_SET );
	for ( lines = 0 ; fgets(line, LINE_MAX_LEN, file ) ; lines++ )
	{
		in = (char *)line;
		out = (char *)clean;
		while ( *in )
		{
			switch ( *in )
			{
				case '\t': case ';': case ',':
					*out = ' ';
					break;
				case ' ':
					*out = '_';
					break;
				default:
					*out = *in;
					break;
			}
			out++;
			in++;
		}
		*out = 0;
		sts = sscanf(clean, "%s %d %s %d %d", 
			typeName,
			&sd.index,
			sd.name,
			&sd.low_limit,
			&sd.high_limit );
		if ( sts == 5 )
		{
			sd.type = typeNameToIndex(typeName );
			addSoundToList( sd.type, sd.index, sd.name, sd.low_limit, sd.high_limit );
		}
		else
		{
			snprintf(msgbuf, 1024, "sscanf returns %d for line: \"%s\"\n", sts, line );
			log_message("", msgbuf);
		}
	}
	return ( 0 );
}

void
showSounds(void )
{
	int i;
	
	for ( i = 0 ; i < maxSounds ; i++ )
	{
		if ( soundList[i].type != SOUND_TYPE_UNUSED )
		{
			printf("%s,%d,%s,%d,%d\n",
				soundTypes[soundList[i].type].typeName, soundList[i].index, soundList[i].name, soundList[i].low_limit, soundList[i].high_limit );
		}
	}
}

void
getHeartFiles(void )
{
	int hr = shmData->cardiac.rate;
	int i;
	int new_lubdub = -1;
	struct sound *sound;
	
	for ( i = 0 ;  i < maxSounds ; i++ )
	{
		sound = &soundList[i];
		if ( ( sound->type == SOUND_TYPE_HEART ) && ( strcmp(sound->name, current.heart_sound ) == 0 ) && ( sound->low_limit <= hr ) && ( sound->high_limit >= hr ) )
		{
			new_lubdub = sound->index;
			break;
		}
	}
	if ( new_lubdub == -1 )
	{
		snprintf(msgbuf, 1024, "No lubdub file for %s %d", current.heart_sound, shmData->cardiac.rate );
		log_message("", msgbuf);
	}
	else
	{
		lubdub = new_lubdub;
	}
	snprintf(msgbuf, 1024, "Get Heart Files %s : %d", 
		current.heart_sound, lubdub );
	log_message("", msgbuf);
}
void
getLungFiles(void )
{
	int breathRate = shmData->respiration.rate;
	int i;
	int new_inhL = -1;
	int new_inhR = -1;
	struct sound *sound;
	
	for ( i = 0 ;  i < maxSounds ; i++ )
	{
		sound = &soundList[i];
		if ( ( sound->type == SOUND_TYPE_LUNG ) && (strcmp(sound->name, current.left_lung_sound ) == 0 ) && ( sound->low_limit <= breathRate ) && ( sound->high_limit >= breathRate ) )
		{
			new_inhL = sound->index;
			break;
		}
	}
	for ( i = 0 ;  i < maxSounds ; i++ )
	{
		sound = &soundList[i];
		if ( ( sound->type == SOUND_TYPE_LUNG ) && (strcmp(sound->name, current.right_lung_sound ) == 0 ) && ( sound->low_limit <= breathRate ) && ( sound->high_limit >= breathRate ) )
		{
			new_inhR = sound->index;
			break;
		}
	}
	if ( new_inhL == -1 )
	{
		snprintf(msgbuf, 1024, "No inhL file for %s %d", current.left_lung_sound, shmData->respiration.rate );
		log_message("", msgbuf);
	}
	else
	{
		inhL = new_inhL;
	}
	if ( new_inhR == -1 )
	{
		snprintf(msgbuf, 1024, "No inhR file for %s %d", current.right_lung_sound, shmData->respiration.rate );
		log_message("", msgbuf);
	}
	else
	{
		inhR = new_inhR;
	}
	snprintf(msgbuf, 1024, "Get Lung Files %s : %d, %s : %d", 
		current.left_lung_sound, inhL, current.right_lung_sound, inhR );
	log_message("", msgbuf);
}
unsigned int heartLast = 0;
int heartState = 0;
unsigned int lungLast = 0;
int lungState = 0;

FILE *riseLPin;
FILE *riseRPin;
FILE *fallPin;
FILE *pulsePin;

int pumpOnOff;
int riseOnOff;
int fallOnOff;

void doReport(void )
{
	snprintf(msgbuf, 1024, "Counts: %d, %d, Heart State %d, Heart Gain %d Lung State %d Lung Gain R-%d L-%d Master Gain %d  heart %d inh R-%d L-%d", 
				current.heartCount, current.breathCount, 
				heartState, current.heartGain, 
				lungState, current.rightLungGain, current.leftLungGain, 
				current.masterGain,
				lubdub, inhR, inhL );
	
	log_message("", msgbuf);
}

// Volume is a range of 0 to 10, 
// Strength is a range of 0 to 10
// Range of Gain of Tsunami is -70 to 10 
// But -30 is barely audible, so we use -40 as our bottom

#define MAX_CALC_GAIN	10
#define MIN_CALC_GAIN	-40
#define GAIN_CALC_RANGE (MAX_CALC_GAIN - ( MIN_CALC_GAIN ) )
int volumeToGain(int volume, int strength )
{
	int gain;
	int s1;
	int g1;
	int g2;
	
	s1 = (strength + volume)*100;
	g1 = (s1 * GAIN_CALC_RANGE );
	g2 = g1 / 2000;
	gain = g2 + MIN_CALC_GAIN;
	return ( gain );
}

void allAirOff(int quiet )
{
	gpioPinSet(riseLPin, TURN_OFF );
	gpioPinSet(riseRPin, TURN_OFF );
	gpioPinSet(fallPin, TURN_OFF );
	if ( ! quiet )
	{
		shmData->respiration.riseState = 0;
		shmData->respiration.fallState = 0;
		fallStopTime = 0;
		lungState = 0;
	}
}
/*
 * Function: ss_signal_handler
 *
 * Handle inbound signals.
 *		SIGHUP is ignored 
 *		SIGTERM closes the process
 *
 * Parameters: sig - the signal
 *
 * Returns: none
 */
int doStop;

void ss_signal_handler(int sig )
{
	switch(sig) {
	case SIGHUP:
		allAirOff(0);
		log_message("","hangup signal caught");
		break;
	case SIGTERM:
		allAirOff(0);
		log_message("","terminate signal caught");
		
		exit(0);
		break;
	}
}


int
main(int argc, char *argv[] )
{
	int sfd;
	int sfd2;
	char buffer[MAX_BUF+1];
	int i;
	int c;
	int val;
	int sts;
	struct sigaction new_action;
	int changed;
	int listenState = FALSE;
	
	while (( c = getopt(argc, argv, "smdth" ) ) != -1 )
	{
		switch ( c )
		{
			case 'd':
				debug = 1;
				break;
			case 'm':
				monitor = 1;
				debug = 1;
				break;
			case 't':
				monitor = 1;
				debug = 2;
				break;
			case 's':
				monitor = 0;
				debug = 3;
				break;
			case 'h':
				cout << "Usage:\n";
				cout << argv[ 0 ] << " [-d] [-m][-t] [tty port 1] [tty port 2]\n";
				cout << "eg: " << argv[ 0 ] << " ttyO2 ttyO4\n";
				return (0 );
		}
	}
	// detect if TTY files are name tttOn or ttySn
	// In earlier debian releases, ttyOn was used.
	// In debian 10, the default name switched to ttySn, and symlinks allowed using ttyOn
	// In debian 11, the symlinks are gone. 
	struct stat sb;
	sprintf(sioName[0], "/dev/ttyO2" );
	sprintf(sioName[1], "/dev/ttyO4" );
	printf("Checking %s\n", sioName[0] );
	if (lstat(sioName[0], &sb) == -1)
	{
        // File not found, try ttsS2
		sprintf(sioName[0], "/dev/ttyS2" );
		sprintf(sioName[1], "/dev/ttyS4" );
		if (lstat(sioName[0], &sb) == -1)
		{
			// No tty files
		}
		else
		{
			// Found ttyS2
		}
    }
	else
	{
		// Found tty O2
	}
	
	if ( monitor == 0 )
	{
		if ( optind < argc )
		{
			snprintf(sioName[0], MAX_BUF, "/dev/%s", argv[optind] );
			optind++;
			if ( optind < argc )
			{
				snprintf(sioName[1], MAX_BUF, "/dev/%s", argv[optind] );
			}
		}
	}
	printf("Debug %d, Monitor %d, sio '%s' '%s'\n", debug, monitor, sioName[0], sioName[1] );
	if ( monitor )
	{
		sts = initSHM(SHM_OPEN );
		if ( sts  )
		{
			perror("initSHM" );
			return (-1 );
		}
		runMonitor();
	}
	initSoundList();
	if ( debug && debug < 3 )
	{
		printf("Show Sounds:\n" );
		showSounds();
	}

	// Controls for Chest Rise/Fall
	riseLPin = gpioPinOpen(23, GPIO_OUTPUT );	// P8_13
	riseRPin = gpioPinOpen(67, GPIO_OUTPUT );	// P8_8
	fallPin = gpioPinOpen(68, GPIO_OUTPUT );	// P8_10
	pulsePin = gpioPinOpen(66, GPIO_OUTPUT );	// P8_7

	allAirOff(1 );
	if ( ( debug < 1 ) && ( ldebug == 0 ) )
	{
		if ( debug > 0 )
		{
			printf("Calling daemonize\n" );
		}
		daemonize();
	}
	else
	{
		printf("Skip daemonize\n" );
	}
	if ( debug > 1 )
	{
		printf("Starting Timer\n" );
	}
	new_action.sa_handler = ss_signal_handler;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction (SIGPIPE, &new_action, NULL);
	signal(SIGHUP,ss_signal_handler); /* catch hangup signal */
	signal(SIGTERM,ss_signal_handler); /* catch kill signal */
	if ( debug > 0 )
	{
		printf("ldebug is %d\n", ldebug );
	}
	if ( !ldebug )
	{
		if ( debug > 0 )
		{
			printf("Calling initSHM\n" );
		}
		sts = initSHM(SHM_OPEN );
		if ( debug > 0 )
		{
			printf("initSHM returned\n" );
		}
		if ( sts  )
		{
			perror("initSHM");
			allAirOff(1);
			return (-1 );
		}
	}
	
	if ( debug > 1 )
	{
		printf("Looking for WAV Trigger\n" );
	}	
	// When booted, the SIO port may not yet be available. Try every 1 second for 20 sec.
	for ( i = 0 ; i <  20 ; i++ )
	{
		sfd = open(sioName[0], O_RDWR | O_NOCTTY | O_SYNC );
		if ( sfd < 0 )
		{
			if ( debug > 1 )
			{
				perror("open" );
				printf("Try %d\n", i );
			}
			sleep(1 );
		}
		else
		{
			break;
		}
	}
	if ( debug > 1 )
	{
		printf("Shut Off air\n" );
	}
	allAirOff(0);
	if ( sfd < 0 )
	{
		snprintf(msgbuf, 1024, "No SIO Port. Running Silent" );
		log_message("", msgbuf);
	}
	else
	{
		if ( debug > 1 )
		{
			printf("Set TERMIOS\n" );
		}
		setTermios(sfd, B57600); // WAV Trigger defaults to 57600
		
		if ( debug > 1 )
		{
			printf("Start Wav\n" );
		}
	}
	wav.start(sfd, 0 );
	usleep(500000);
	
	if ( debug < 4 )
	{
		if ( debug > 1 )
		{
			printf("initialize_timers\n" );
		}
		initialize_timers();
	}
	if ( sfd < 0 )
	{
	}
	else
	{
		if ( debug > 1 )
		{
			printf("Check Tsunami Version\n" );
		}
		val = wav.getVersion(buffer, MAX_BUF );
		
		if ( debug > 0 )
		{
			printf("WAV Trigger Version: Len %d String '%.*s'\n", val, val-1, &buffer[1] );
		}
		else
		{
			snprintf(msgbuf, 1024, "WAV Trigger Version: Len %d String %.*s", val, val-1, &buffer[1] );
			log_message("", msgbuf);
		}
		val = wav.getSysInfo(buffer, MAX_BUF );
		if ( debug > 0 )
		{
			printf("Sys Info: Len %d Voices %d Tracks %d\n", val, buffer[1], buffer[2] );
		}
		else
		{
			snprintf(msgbuf, 1024, "Sys Info: Len %d Voices %d Tracks %d", val, buffer[1], buffer[2] );
			log_message("", msgbuf);
		}
		if ( wav.boardType == BOARD_WAV_TRIGGER )
		{
			wavPulse = &wav2;
			// If Wav Trigger (not Tsunami), then we try to open the second port
			if ( strlen(sioName[1]) > 0 )
			{
				sfd2 = open(sioName[1], O_RDWR | O_NOCTTY | O_SYNC );
				if ( sfd2 < 0  )
				{
					// Failed - No second board
					snprintf(msgbuf, 1024, "tty failed for second Wav Trigger on %s", sioName[1] );
					log_message("", msgbuf);
					sfd2 = 0;
				}
				else
				{
					setTermios(sfd2, B57600); // WAV Trigger defaults to 57600
					wav2.start(sfd2, 1 );
					usleep(500000);
					val = wav2.getVersion(buffer, MAX_BUF );
					snprintf(msgbuf, 1024, "WAV2 Trigger Version: Len %d String %.*s", val, val-1, &buffer[1] );
					log_message("", msgbuf);
					val = wav2.getSysInfo(buffer, MAX_BUF );
					snprintf(msgbuf, 1024, "Sys Info: Len %d Voices %d Tracks %d", val, buffer[1], buffer[2] );
					log_message("", msgbuf);
				}
			}
		}
		else
		{
			wavPulse = &wav;
		}
		if ( wav.boardType == BOARD_TSUNAMI && wav.tsunamiMode != TSUNAMI_MONO )
		{
			sfd2 = sfd; // Send all commands to Tsunami
			if ( debug > 0 )
			{
				printf("Tsunami is running Stereo Mode. Must be Mono\n" );
			}
			else
			{
				snprintf(msgbuf, 1024, "Tsunami is running Stereo Mode. Must be Mono");
				log_message("", msgbuf);
			}
		}
	}
	//wav.show();
	wav.ampPower(0 );
	wav.stopAllTracks();
	wav.masterGain(0);
	if ( sfd2 )
	{
		wavPulse->ampPower(1 );
		wavPulse->stopAllTracks();
		wavPulse->masterGain(0);
	}
	current.masterGain = 0;
	
	if ( wav.boardType == BOARD_TSUNAMI )
	{
		for ( i = 0 ; i < 8 ; i++ )
		{
			wav.channelGain(i,0 );
		}
	}
	wavPulse->trackGain(PULSE_TRACK, MAX_MAX_VOLUME );
	wav.trackGain(113, 0 );  // Track 113 is Sinus_120
	wav.trackGain(5, 0 );  // Track 5 is Bark
	wav.stopAllTracks();
	wavPulse->stopAllTracks();

	snprintf(msgbuf, 1024, "Initial Bark");
	log_message("", msgbuf);	
	wav.trackPlaySolo(0, 5);	// Bark
	while ( wav.getTracksPlaying() > 0 )
	{
		usleep(10000);
	}
	if ( debug == 3 )
	{
		wav.trackPlaySolo(0, 1);	// Play Cassiopeia
		while ( wav.getTracksPlaying() > 0 )
		{
			usleep(10000);
		}
	}
	if ( debug > 3 )
	{
		val = getPulseVolume(PULSE_TOUCH_NORMAL, 2 );
		printf("Setting Pulse Volumes to %d\n", val );
		for ( i = 2 ; i < 6 ; i++ )
		{
			wavPulse->channelGain(i, val );
		}
		for ( i = 0 ; i < 2000 ; i++ )
		{
			wavPulse->trackPlayPoly(0, 113); // Pulse
			wavPulse->trackPlayPoly(2, PULSE_TRACK); // Pulse
			wavPulse->trackPlayPoly(3, PULSE_TRACK); // Pulse
			wavPulse->trackPlayPoly(4, PULSE_TRACK); // Pulse
			wavPulse->trackPlayPoly(5, PULSE_TRACK); // Pulse
			usleep(100000);
			while ( wavPulse->getTracksPlaying() > 0 )
			{
				usleep(10000);
			}
		
			switch ( i % 5 )
			{
				case 0:
					gpioPinSet(fallPin, TURN_ON );
					break;
				case 1:
					gpioPinSet(riseLPin, TURN_ON );
					break;
				case 2:
					gpioPinSet(riseRPin, TURN_ON );
					break;
				case 4:
					gpioPinSet(riseLPin, TURN_OFF );
					gpioPinSet(riseRPin, TURN_OFF );
					gpioPinSet(fallPin, TURN_OFF );
					gpioPinSet(pulsePin, TURN_OFF );
					break;
			}

			printf("%d\n", i );
		}
		
		snprintf(msgbuf, 1024, "Debug Bark");
		log_message("", msgbuf);
		wav.trackPlaySolo(0, 5); //Bark
		allAirOff(0);
		exit ( 0 );
	}
	//wav.masterGain(MIN_VOLUME);
	//current.masterGain = MIN_VOLUME;
	current.heartCount = 0;
	current.breathCount = 0;
	current.leftLungGain = -65;
	current.rightLungGain = -65;
	current.heartGain = -65;
	
	wavPulse->trackGain(PULSE_TRACK, MAX_MAX_VOLUME );
	pthread_create (&threadInfo1, NULL, &sync_thread,(void *) NULL );
	
	// Main loop monitors the volumes and keeps them set
	// Also gets the track info updated

	if ( debug )
	{
		snprintf(msgbuf, 1024, "Running" );
		if ( debug > 1 )
		{
			printf("%s\n", msgbuf );
		}
		else
		{
			log_message("", msgbuf);
		}
	}

	while ( 1 )
	{
		// Master off based on active auscultation
		if ( soundTest )
		{
			if ( current.masterGain != MAX_VOLUME )
			{
				wav.channelGain(0, MAX_VOLUME);
				current.masterGain = MAX_VOLUME;
			}
			shmData->auscultation.col  = 1;
			shmData->auscultation.row  = 1;
			shmData->auscultation.side = 1;
			shmData->auscultation.heartStrength = 10;
			shmData->auscultation.leftLungStrength = 10;
			shmData->auscultation.rightLungStrength = 0;
		}
		else
		{
			if ( listenState != comm.barkState )
			{
				// listenState has changed. If new barkState is TRUE then bark
				listenState = comm.barkState;
				if ( listenState == TRUE )
				{
					int savedVolume = current.masterGain;
					wav.channelGain(0, 0);
					current.masterGain = 0;
					while ( wav.getTracksPlaying() > 0 )
					{
						usleep(10000);
					}
					//wav.trackGain(5, 0 );
					wav.stopAllTracks();
					snprintf(msgbuf, 1024, "Enter Listen State Bark");
					log_message("", msgbuf);
					wav.trackPlaySolo(0, 5);	// Bark
					while ( wav.getTracksPlaying() > 0 )
					{
						usleep(10000);
					}
					wav.channelGain(0, savedVolume);
				}
			}
			if ( ( shmData->auscultation.side == 0 ) && ( current.masterGain != MIN_VOLUME ) )
			{
				wav.channelGain(0, MIN_VOLUME);
				current.masterGain = MIN_VOLUME;
				if ( debug )
				{
					printf("Master Off\n" );
				}
				snprintf(msgbuf, 1024, "Set Off: %d, %d, Heart Gain %d, Lung Gains %d / %d, Master Gain %d", 
					current.heartCount, current.breathCount, current.heartGain, current.rightLungGain, current.leftLungGain, current.masterGain );
				log_message("", msgbuf);
			}
			else if ( ( shmData->auscultation.side != 0 ) && ( current.masterGain != MAX_VOLUME ) )
			{
				wav.channelGain(0, MAX_VOLUME);
				current.masterGain = MAX_VOLUME;
				if ( debug  )
				{
					printf("Master On\n" );
				}
				snprintf(msgbuf, 1024, "Set On: %d, %d, Heart Gain %d, Lung Gains %d / %d (%d), Master Gain %d", 
					current.heartCount, current.breathCount, current.heartGain, current.rightLungGain, current.leftLungGain, shmData->respiration.left_lung_sound_volume, current.masterGain );
				log_message("", msgbuf);
			}
		}
		
		changed = 0;
		// Check for heart/lung changes
		if ( ( current.heart_rate != shmData->cardiac.rate ) || 
			 ( strcmp(current.heart_sound, shmData->cardiac.heart_sound) != 0 ) )
		{
			snprintf(msgbuf, 1024, "Cardiac %d:%d, %s, %s", 
				 current.heart_rate, shmData->cardiac.rate,
				 current.heart_sound, shmData->cardiac.heart_sound	 );
			log_message("", msgbuf);		
			current.heart_rate = shmData->cardiac.rate;
			memcpy(current.heart_sound, shmData->cardiac.heart_sound, 32 );
			changed = 1;
		}
		if ( changed )
		{
			getHeartFiles();
			doReport();
		}
		
		changed = 0;
		if ( ( current.respiration_rate != shmData->respiration.rate ) ||
			 ( strcmp(current.left_lung_sound, shmData->respiration.left_lung_sound) != 0 ) ||
			 ( strcmp(current.right_lung_sound, shmData->respiration.right_lung_sound) != 0 ) )
		{
			snprintf(msgbuf, 1024, "Resp %d:%d, %s, %s, %s, %s", 
				 current.respiration_rate, shmData->respiration.rate,
				 current.left_lung_sound, shmData->respiration.left_lung_sound,
				 current.right_lung_sound, shmData->respiration.right_lung_sound );
			log_message("", msgbuf);
			current.respiration_rate = shmData->respiration.rate;
			memcpy(current.left_lung_sound, shmData->respiration.left_lung_sound, 32 );
			memcpy(current.right_lung_sound, shmData->respiration.right_lung_sound, 32 );
			changed = 1;
		}
		if ( changed )
		{
			getLungFiles();
			doReport();
		}
		
		runLung();
		runHeart();
		usleep(SOUND_LOOP_DELAY );
	}
}

void *
sync_thread ( void *ptr )
{
	int sts;
	
	sts = comm.openListen(LISTEN_ACTIVE );
	if ( sts != 0 )
	{
		perror("comm.openListen" );
		exit ( -4 );
	}
	shmData->simMgrStatusPort = comm.simMgrStatusPort;
	sprintf(shmData->simMgrIPAddr, "%s", comm.simMgrIPAddr );

	while ( 1 )
	{
		sts = comm.wait();
		if ( sts & (SYNC_PULSE | SYNC_PULSE_VPC ) )
		{
			current.heartCount += 1;
		}
		if ( sts & SYNC_BREATH )
		{
			current.breathCount += 1;
			allAirOff(0 );
		}
		if( sts & SYNC_STATUS_PORT )
		{
			shmData->simMgrStatusPort = comm.simMgrStatusPort;
		}
	}
}
void
setHeartVolume(int force )
{
	int gain = current.heartGain;
	
	if ( force ||
		 ( current.heart_sound_mute != shmData->cardiac.heart_sound_mute ) ||
		 ( current.heart_sound_volume != shmData->cardiac.heart_sound_volume ) ||
		 ( current.heartStrength != shmData->auscultation.heartStrength ) ||
		 ( current.pea != shmData->cardiac.pea ) )
	{
		current.heart_sound_mute = shmData->cardiac.heart_sound_mute;
		current.heart_sound_volume = shmData->cardiac.heart_sound_volume;
		current.heartStrength  = shmData->auscultation.heartStrength;
		current.pea = shmData->cardiac.pea;
		//if ( current.heart_sound_mute )
		//{
		//	current.heartGain = MIN_VOLUME;
		//}
		//else
		{
			if ( current.pea )
			{
				current.heartGain = MIN_VOLUME;
			}
			else
			{
				current.heartGain = volumeToGain(current.heart_sound_volume + shmData->auscultation.heartTrim, current.heartStrength );
			}
		}
	}
	if ( force || ( gain != current.heartGain ) )
	{
		wav.trackGain(lubdub, current.heartGain );
	}
}
void
setLeftLungVolume(int force )
{
	int gain = current.leftLungGain;
	
	if ( force ||
		 ( current.left_lung_sound_mute != shmData->respiration.left_lung_sound_mute ) ||
		 ( current.left_lung_sound_volume != shmData->respiration.left_lung_sound_volume ) ||
		 ( current.leftLungStrength != shmData->auscultation.leftLungStrength ) )
	{
		current.left_lung_sound_mute = shmData->respiration.left_lung_sound_mute;
		current.left_lung_sound_volume = shmData->respiration.left_lung_sound_volume;
		current.leftLungStrength = shmData->auscultation.leftLungStrength;
		//if ( current.left_lung_sound_mute )
		//{
		//	current.leftLungGain = MIN_VOLUME;
		//}
		//else
		{
			current.leftLungGain = volumeToGain(current.left_lung_sound_volume + shmData->auscultation.lungTrim, current.leftLungStrength );
		}
	}
	
	if ( force || ( gain != current.leftLungGain ) )
	{
		if ( shmData->auscultation.side != 2 )
		{
			wav.trackGain(inhL, current.leftLungGain );
		}
	}
}
void
setRightLungVolume(int force )
{
	int gain = current.rightLungGain;
	
	if ( force ||
		 ( current.right_lung_sound_mute != shmData->respiration.right_lung_sound_mute ) ||
		 ( current.right_lung_sound_volume != shmData->respiration.right_lung_sound_volume ) ||
		 ( current.rightLungStrength != shmData->auscultation.rightLungStrength ) )
	{
		current.left_lung_sound_mute = shmData->respiration.left_lung_sound_mute;
		current.right_lung_sound_volume = shmData->respiration.right_lung_sound_volume;
		current.rightLungStrength = shmData->auscultation.rightLungStrength;
		//if ( current.right_lung_sound_mute )
		//{
		//	current.rightLungGain = MIN_VOLUME;
		//}
		//else
		{
			current.rightLungGain = volumeToGain(current.right_lung_sound_volume + shmData->auscultation.lungTrim, current.rightLungStrength );
		}
	}
	if ( force || ( gain != current.rightLungGain ) )
	{
		if ( shmData->auscultation.side != 1 )
		{
			wav.trackGain(inhR, current.rightLungGain );
		}
	}
}

void 
runHeart ( void )
{
	struct itimerspec its;
	
	if ( shmData->auscultation.side != 0 && shmData->cardiac.pea == 0 )
	{
		setHeartVolume(1 );	// Force volume setting as the track may have changed
	}
	else
	{
		setHeartVolume(1 );	// Set volume only if a change occurred
	}
	switch ( heartState )
	{
		case 0:
			if ( heartLast != current.heartCount )
			{
				heartLast = current.heartCount;
				gpioPinSet(pulsePin, TURN_ON );
				//if ( shmData->auscultation.side != 0 )
				//{
					its.it_interval.tv_sec = 0;
					its.it_interval.tv_nsec = 0;
					
					// Set First expiration as the interval plus the delay
					its.it_value.tv_sec = 0;
					its.it_value.tv_nsec = LUB_DELAY;
					if (timer_settime(heart_timer, 0, &its, NULL) == -1)
					{
						perror("runHeart: timer_settime");
						//snprintf(msgbuf, 1024, "runHeart: timer_settime: %s", strerror(errno) );
						//log_message("", msgbuf );
						exit ( -1 );
					}
				//}
			}
			break;
		case 1:
			gpioPinSet(pulsePin, TURN_OFF );
			if ( shmData->cardiac.pea == 0 )
			{
				//if ( shmData->auscultation.side != 0 )
				//{
					// gpioPinSet(pulsePin, TURN_OFF );
					wav.trackPlayPoly(0, lubdub);
					//snprintf(msgbuf, 1024, "runHeart: lub (%d) Gain is %d", lub, heartGain );
					//log_message("", msgbuf );
					heartState = 0;
					heartPlaying = 1;
				//}
				//else
				//{
				//	heartState = 0;
				//	heartPlaying = 0;
				//}
				
				// Check pulse palpation
				doPulse();
			}
			break;
			
		default:
			break;
	}
}

static void
delay_handler(int sig, siginfo_t *si, void *uc)
{
	if ( sig == HEART_TIMER_SIG )
	{
		if ( heartState == 0 )
		{
			heartState = 1;
		}
		else if ( heartState == 2 )
		{
			heartState = 3;
		}
	}
	else if ( sig == BREATH_TIMER_SIG )
	{
		if ( lungState == 0 )
		{
			lungState = 1;
		}
	}	
}
#define EXH_LIMIT 400
int exhLimit = EXH_LIMIT;
#define INH_LIMIT 1.5
double inhLimit = INH_LIMIT;

static void
rise_handler(int sig, siginfo_t *si, void *uc)
{
	if ( shmData->respiration.chest_movement )
	{
		// Stop rise
		lungRise(TURN_OFF );
		riseOnOff = 0;
		usleep(10000);	// Delay 10 MSEC before fall
		lungFall(TURN_ON );
		fallOnOff = 1;
	}
	else
	{
		// When chest movement is disabled, 
		lungRise(TURN_OFF );
		lungFall(TURN_ON);
		usleep(10000);
		lungFall(TURN_OFF );
		riseOnOff = 0;
		fallOnOff = 0;
	}
	exhLimit = EXH_LIMIT;
}

void
initialize_timers(void )
{
	struct sigaction new_action;
	sigset_t mask;
	
	printf("\n\ninitialize_timers\n\n" );
	// Pulse Timer Setup
	new_action.sa_flags = SA_SIGINFO;
	new_action.sa_sigaction = delay_handler;
	sigemptyset(&new_action.sa_mask);
	if (sigaction(HEART_TIMER_SIG, &new_action, NULL) == -1)
	{
		perror("sigaction");
		snprintf(msgbuf, 1024, "sigaction() fails for Pulse Timer: %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Block timer signal temporarily
	sigemptyset(&mask);
	sigaddset(&mask, HEART_TIMER_SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		perror("sigprocmask");
		snprintf(msgbuf, 1024, "sigprocmask() fails for Pulse Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Create the Timer
	heart_sev.sigev_notify = SIGEV_SIGNAL;
	heart_sev.sigev_signo = HEART_TIMER_SIG;
	heart_sev.sigev_value.sival_ptr = &heart_timer;
	
	if ( timer_create(CLOCK_MONOTONIC, &heart_sev, &heart_timer ) == -1 )
	{
		perror("timer_create" );
		snprintf(msgbuf, 1024, "timer_create() fails for Pulse Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit (-1);
	}
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
		perror("sigprocmask");
		snprintf(msgbuf, 1024, "sigprocmask() fails for Pulse Timer%s ", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	
	// Breath Timer Setup
	new_action.sa_flags = SA_SIGINFO;
	new_action.sa_sigaction = delay_handler;
	sigemptyset(&new_action.sa_mask);
	if (sigaction(BREATH_TIMER_SIG, &new_action, NULL) == -1)
	{
		perror("sigaction");
		snprintf(msgbuf, 1024, "sigaction() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit(-1 );
	}
	// Block timer signal temporarily
	sigemptyset(&mask);
	sigaddset(&mask, BREATH_TIMER_SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		perror("sigprocmask");
		snprintf(msgbuf, 1024, "sigprocmask() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Create the Timer
	breath_sev.sigev_notify = SIGEV_SIGNAL;
	breath_sev.sigev_signo = BREATH_TIMER_SIG;
	breath_sev.sigev_value.sival_ptr = &breath_timer;
	
	if ( timer_create(CLOCK_MONOTONIC, &breath_sev, &breath_timer ) == -1 )
	{
		perror("timer_create" );
		snprintf(msgbuf, 1024, "timer_create() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit (-1);
	}
	
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
		perror("sigprocmask");
		snprintf(msgbuf, 1024, "sigprocmask() fails for Breath Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	
	// Rise Timer Setup
	new_action.sa_flags = SA_SIGINFO;
	new_action.sa_sigaction = rise_handler;
	sigemptyset(&new_action.sa_mask);
	if (sigaction(RISE_TIMER_SIG, &new_action, NULL) == -1)
	{
		perror("sigaction");
		snprintf(msgbuf, 1024, "sigaction() fails for Rise Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit(-1 );
	}
	// Block timer signal temporarily
	sigemptyset(&mask);
	sigaddset(&mask, RISE_TIMER_SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
	{
		perror("sigprocmask");
		snprintf(msgbuf, 1024, "sigprocmask() fails for Rise Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
	// Create the Timer
	rise_sev.sigev_notify = SIGEV_SIGNAL;
	rise_sev.sigev_signo = RISE_TIMER_SIG;
	rise_sev.sigev_value.sival_ptr = &rise_timer;
	
	if ( timer_create(CLOCK_MONOTONIC, &rise_sev, &rise_timer ) == -1 )
	{
		perror("timer_create" );
		snprintf(msgbuf, 1024, "timer_create() fails for Rise Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit (-1);
	}
	
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
    {
		perror("sigprocmask");
		snprintf(msgbuf, 1024, "sigprocmask() fails for Rise Timer %s", strerror(errno) );
		log_message("", msgbuf );
		exit ( -1 );
	}
}

void
lungFall(int control )
{
	gpioPinSet(fallPin, control );

	if ( control == TURN_ON )
		shmData->respiration.fallState = 1;
	else
		shmData->respiration.fallState = 0;
}

void
lungRise(int control )
{
	if ( control == TURN_ON )
	{
		shmData->respiration.riseState = 1;
	}
	else
	{
		shmData->respiration.riseState = 0;
	}
	if ( ! shmData->respiration.chest_movement  )
	{
		control = 0;
	}
	gpioPinSet(riseLPin, control );
	gpioPinSet(riseRPin, control );
}
/* Lung State:
	0 - Idle. Waiting for Sync. When Sync Received:
		Start Inflation. 
		Start timer to end inflation.
		Start 40 ms timer to wait for sound
	1 - Play Sound
	
*/
	
void 
runLung( void )
{
	struct itimerspec its;
	long int delayTime;	// Delay in ns
	double periodSeconds;
	double inhTime;
	double fractional;
	double integer;
	time_t now;
	
	if ( ! shmData->respiration.chest_movement )
	{
		allAirOff(0);
	}

	if ( shmData->auscultation.side != 0  )
	{
		current.respiration_rate = shmData->respiration.rate;
		setLeftLungVolume(1 );	// Force volume setting as the track may have changed
		setRightLungVolume(1 );	// Force volume setting as the track may have changed
	}
	else
	{
		setLeftLungVolume(0 );	// Set volume only if a change occurred
		setRightLungVolume(0 );	// Set volume only if a change occurred
	}
	if ( shmData->respiration.active ) // && shmData->respiration.chest_movement )
	{
		// Manual Respiration
		lungFall(TURN_OFF );
		if ( shmData->respiration.chest_movement )
		{
			lungRise(TURN_ON );
		}
	}
	else
	{
		switch ( lungState )
		{
			case 0:
				if ( fallStopTime )
				{
					now = time(NULL );
					if ( now >= fallStopTime )
					{
						lungFall(TURN_OFF );
						fallStopTime = 0;
					}
				}
				if ( lungLast != current.breathCount )
				{
					lungLast = current.breathCount;
				// Breath Timer
					// Clear the interval, run single execution
					its.it_interval.tv_sec = 0;
					its.it_interval.tv_nsec = 0;
					
					// Set First expiration as the interval plus the delay
					its.it_value.tv_sec = 0;
					delayTime = 40000000;	// Delay in ns
					its.it_value.tv_nsec = delayTime;
					if (timer_settime(breath_timer, 0, &its, NULL) == -1)
					{
						perror("runLung: timer_settime");
						snprintf(msgbuf, 1024, "runLung: timer_settime: %s", strerror(errno) );
						log_message("", msgbuf );
						exit ( -1 );
					}
					lungFall(TURN_OFF );
					fallOnOff = 0;
					usleep(10000);

					if ( shmData->respiration.chest_movement )
					{
						if ( debug ) printf("ON\n" );
						lungRise(TURN_ON );
					}
					riseOnOff = 1;
				// Rise Timer
					// Clear the interval, run single execution
					its.it_interval.tv_sec = 0;
					its.it_interval.tv_nsec = 0;
					
					// The duration should be 30% of the respiration period
#define INH_PERCENT		(0.30)
					if ( shmData->respiration.rate > 0 )
					{
						periodSeconds = ( 1 / (double)shmData->respiration.rate ) * 60;
					}
					else
					{
						periodSeconds = 2;
					}
					inhTime = periodSeconds * INH_PERCENT;
					if ( inhTime > inhLimit )
					{
						inhTime = inhLimit;
					}
					
					if ( inhTime < 0 )
					{
						snprintf(msgbuf, 1024, "runLung: rise inhTime (%f) is negative. period %f rate %d", inhTime, periodSeconds, shmData->respiration.rate );
						inhTime = inhLimit;
						log_message("", msgbuf );
					}
					fractional = modf(inhTime, &integer );
					its.it_value.tv_sec = (int)integer;
					delayTime = fractional * 1000000000; //Seconds to nanoseconds

					if ( delayTime < 0 )
					{
						snprintf(msgbuf, 1024, "runLung: rise delayTime is negative for inhTime %f rate %d", inhTime, shmData->respiration.rate );
						delayTime = 0;
						log_message("", msgbuf );
					}
					its.it_value.tv_nsec = delayTime;
					
					if (timer_settime(rise_timer, 0, &its, NULL) == -1)
					{
						//perror("runLung: rise timer_settime");
						snprintf(msgbuf, 1024, "runLung: rise timer_settime: %s", strerror(errno) );
						log_message("", msgbuf );
						snprintf(msgbuf, 1024, "runLung: periodSeconds %f, (%ld : %ld)", periodSeconds, its.it_value.tv_sec, its.it_value.tv_nsec );
						log_message("", msgbuf );
						exit ( -1 );
					}
				}
				else if ( current.respiration_rate == 0 )
				{
					if ( exhLimit-- == 0 )
					{
						if ( debug ) printf("OFF\n" );
						lungFall(TURN_OFF );
						fallOnOff = 0;
						lungRise(TURN_OFF );
						snprintf(msgbuf, 1024, "runLung: exhLimit Hit" );
						log_message("", msgbuf );
					}
				}
				break;
			case 1:
				if ( shmData->auscultation.side > 0 &&  shmData->auscultation.side < 4 )
				{
					if ( shmData->auscultation.side == 1 )
					{
						wav.trackPlayPoly(0, inhL);
					}
					else
					{
						wav.trackPlayPoly(0, inhR);
					}

					if ( debug > 1 )
					{
						printf("inh: %d-%d\n", inhR, inhL );
					}
					lungState = 0;
					lungPlaying = 1;
				}
				else
				{
					lungState = 0;
					lungPlaying = 0;
				}
				now = time(NULL );
				fallStopTime = now + 10;
				break;
#if 0
			case 2: // No longer used
				if ( shmData->auscultation.side == 0 )
				{
					wav.trackStop(inh );
					lungState = 0;
					lungPlaying = 0;
				}
				else
				{
					// INH playing. Check for completion
					if ( wav.checkTrack(inh ) == 0 )
					{
						// inh done
						lungState = 0;
						lungPlaying = 0;
					}
				}
				break;
#endif	
			default:
				break;
		}
	}
}

int getPulseVolume(int pressure, int strength )
{
	int pulseVolume;
	
	switch ( pressure )
	{
		case PULSE_TOUCH_NONE:
		default:
			pulseVolume = PULSE_VOLUME_OFF;
			break;
		case PULSE_TOUCH_EXCESSIVE:
			pulseVolume = PULSE_VOLUME_VERY_SOFT;
			break;
		case PULSE_TOUCH_HEAVY:
			pulseVolume = PULSE_VOLUME_SOFT;
			break;
		case PULSE_TOUCH_NORMAL:
			pulseVolume = PULSE_VOLUME_ON;
			break;
		case PULSE_TOUCH_LIGHT:
			pulseVolume = PULSE_VOLUME_SOFT;
			break;
	}
	switch ( strength )
	{
		case 0: // None
			pulseVolume = PULSE_VOLUME_OFF;
			break;
		case 1: // Weak
			pulseVolume -= 10;
			break;
		case 2: // Normal
			break;
		case 3: // Strong
		default:
			pulseVolume += 10;
			break;
	}
	return ( pulseVolume );
}

void doPulse(void )
{
	int pulseVolume;
	
	if ( shmData->cardiac.pea )
	{
		return;
	}
	if ( wavPulse->boardType == BOARD_TSUNAMI )
	{
		if ( shmData->pulse.right_femoral && shmData->cardiac.right_femoral_pulse_strength > 0 )
		{
			pulseVolume = getPulseVolume(shmData->pulse.right_femoral, shmData->cardiac.right_femoral_pulse_strength );
			pulseVolume = pulseVolume - 25;
			shmData->pulse.volume[PULSE_RIGHT_FEMORAL] = pulseVolume;
			wavPulse->channelGain(3, pulseVolume );
			wavPulse->trackPlayPoly(3, PULSE_TRACK);
		}
		else
		{
			wavPulse->channelGain(3, PULSE_VOLUME_OFF );
			shmData->pulse.volume[PULSE_RIGHT_FEMORAL] = PULSE_VOLUME_OFF;
		}
		if ( shmData->pulse.left_femoral && shmData->cardiac.left_femoral_pulse_strength > 0 )
		{
			pulseVolume = getPulseVolume(shmData->pulse.left_femoral, shmData->cardiac.left_femoral_pulse_strength );
			pulseVolume = pulseVolume - 25;
			shmData->pulse.volume[PULSE_LEFT_FEMORAL] = pulseVolume;
			wavPulse->channelGain(2, pulseVolume );
			wavPulse->trackPlayPoly(2, PULSE_TRACK);
		}
		else
		{
			wavPulse->channelGain(2, PULSE_VOLUME_OFF );
			shmData->pulse.volume[PULSE_LEFT_FEMORAL] = PULSE_VOLUME_OFF;
		}
	}
	else
	{
		// For WAV Trigger board.
		if ( shmData->pulse.right_femoral && shmData->cardiac.right_femoral_pulse_strength > 0 ) 
		{
			pulseVolume = getPulseVolume(shmData->pulse.right_femoral, shmData->cardiac.right_femoral_pulse_strength );
			pulseVolume = pulseVolume - 25;
			shmData->pulse.volume[PULSE_RIGHT_FEMORAL] = pulseVolume;
			wavPulse->trackGain(PULSE_TRACK_RIGHT, pulseVolume );
			wavPulse->trackPlayPoly(0, PULSE_TRACK_RIGHT);
		}
		else
		{
			wavPulse->trackGain(PULSE_TRACK_RIGHT, PULSE_VOLUME_OFF );
		}
		if ( shmData->pulse.left_femoral && shmData->cardiac.left_femoral_pulse_strength > 0 )
		{
			pulseVolume = getPulseVolume(shmData->pulse.left_femoral, shmData->cardiac.left_femoral_pulse_strength );
			pulseVolume = pulseVolume - 25;
			shmData->pulse.volume[PULSE_LEFT_FEMORAL] = pulseVolume;
			wavPulse->trackGain(PULSE_TRACK_LEFT, pulseVolume );
			wavPulse->trackPlayPoly(0, PULSE_TRACK_LEFT);
		}
		else
		{
			wavPulse->trackGain(PULSE_TRACK_LEFT, PULSE_VOLUME_OFF );
		}
		wavPulse->masterGain(PULSE_VOLUME_ON );
	}
	/*
	switch ( shmData->pulse.position )
	{
		case PULSE_NOT_ACTIVE:
		default:
			pulseChannel = 0;
			break;
			
		case PULSE_LEFT_FEMORAL:
			pulseChannel = 2;
			pulseStrength = shmData->cardiac.left_femoral_pulse_strength;
			break;
		
		case PULSE_RIGHT_FEMORAL:
			pulseChannel = 3;
			pulseStrength = shmData->cardiac.right_femoral_pulse_strength;
			break;
		
	}
		
	if ( pulseChannel > 0 )
	{
		pulseVolume = PULSE_VOLUME_ON;
	
		switch ( shmData->pulse.pressure )
		{
			case PULSE_TOUCH_NONE:
				pulseVolume = PULSE_VOLUME_OFF;
				break;
			case PULSE_TOUCH_EXCESSIVE:
				pulseVolume = PULSE_VOLUME_VERY_SOFT;
				break;
			case PULSE_TOUCH_HEAVY:
				pulseVolume = PULSE_VOLUME_SOFT;
				break;
			case PULSE_TOUCH_NORMAL:
				pulseVolume = PULSE_VOLUME_ON;
				break;
			case PULSE_TOUCH_LIGHT:
				pulseVolume = PULSE_VOLUME_SOFT;
				break;
		}
		switch ( pulseStrength )
		{
			case 0: // None
				pulseVolume = PULSE_VOLUME_OFF;
				break;
			case 1: // Very Weak
				pulseVolume -= 20;
				break;
			case 2: // Weak
				pulseVolume -= 10;
				break;
			case 3: // Strong
			default:
				break;
		}
		
		if ( debug )
		{
			snprintf(msgbuf, 1024, "Pulse: Channel %d Strength %d Volume %d", 
				pulseChannel,
				pulseStrength,
				pulseVolume );
			if ( debug > 1 )
			{
				printf("%s\n", msgbuf );
			}
			else
			{
				log_message("", msgbuf);
			}
		}
		if ( pulseVolume > PULSE_VOLUME_OFF )
		{
			wavPulse->channelGain(pulseChannel, pulseVolume );
			wavPulse->trackPlayPoly(pulseChannel, PULSE_TRACK);
		}
	}
	*/
}

char lastTag[STR_SIZE];

void
runMonitor(void )
{
	
	while ( 1 )
	{
		if ( debug > 1 )
		{
			if ( strcmp(shmData->auscultation.tag, lastTag ) != 0 )
			{
				memcpy(lastTag, shmData->auscultation.tag, STR_SIZE );
				printf("Tag %s\n", lastTag );
			}
		}
		else
		{
			printf( "sense %d:%d:%d:%d  %d:%d:%d:%d  %d:%d:%d:%d  %d:%d:%d:%d  Tag: '%s', %d/%d %s\n", 
					shmData->pulse.base[1], shmData->pulse.ain[1], shmData->pulse.touch[1], shmData->pulse.volume[1],
					shmData->pulse.base[2], shmData->pulse.ain[2], shmData->pulse.touch[2], shmData->pulse.volume[2],
					shmData->pulse.base[3], shmData->pulse.ain[3], shmData->pulse.touch[3], shmData->pulse.volume[3],
					shmData->pulse.base[4], shmData->pulse.ain[4], shmData->pulse.touch[4], shmData->pulse.volume[4], 
					shmData->auscultation.tag, 
					shmData->manual_breath_ain, shmData->manual_breath_baseline, shmData->respiration.manual_breath ? " - Breath" : "" );
		}
		usleep(500000 );
	}
}
