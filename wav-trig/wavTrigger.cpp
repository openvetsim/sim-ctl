/*
 *	sim-ctl Audio "wav": wavTrigger.cpp
 *	Generic Linux WAV Trigger serial control library
 *
 *	Programmers: Terry Kelleher, terry@newforce.us
 * 		Based on Robertsonics Arduino library.
 *		Changed to use Linux TTY port
 *		Extended to add additional commands
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

// **************************************************************
//     Filename: wavTrigger.cpp
// Date Created: 2/23/2014
//
//     Comments: Robertsonics WAV Trigger serial control library
//
// Programmers: Jamie Robertson, jamie@robertsonics.com
//
// **************************************************************

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "wavTrigger.h"

#include <syslog.h>

wavTrigger::wavTrigger(void)
{
	sioPort = -1;
	wavIndex = -1;
	boardType = BOARD_UNKNOWN;
}

// **************************************************************
void wavTrigger::start(int port, int index ) {
  sioPort = port;
  wavIndex = index;
  boardType = BOARD_UNKNOWN;
}

// **************************************************************
// For Tsunami, this will set the Volume for Channel 0
void wavTrigger::masterGain(int gain) {

char txbuf[8];
unsigned short vol;
int len;

  if ( sioPort < 0 )
  {
	  return;
  }
  if ( boardType == BOARD_TSUNAMI )
  {
	  channelGain(0, gain );
  }
  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x07;
  txbuf[3] = CMD_MASTER_VOLUME;
  vol = (unsigned short)gain;
  txbuf[4] = (char)vol;
  txbuf[5] = (char)(vol >> 8);
  txbuf[6] = 0x55;
  len = 7;
  
  write(sioPort, txbuf, len );
}

// **************************************************************
// For Tsunami, this will set the Volume for Channel chan
void wavTrigger::channelGain(int chan, int gain) {

char txbuf[8];
unsigned short vol;
int len;
  if ( sioPort < 0 )
  {
	  return;
  }
  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;

  txbuf[2] = 0x08;
  txbuf[3] = CMD_VOLUME;
  txbuf[4] = chan;
  vol = (unsigned short)gain;
  txbuf[5] = (char)vol;
  txbuf[6] = (char)(vol >> 8);
  txbuf[7] = 0x55;
  len = 8;
  
  write(sioPort, txbuf, len );
}
// **************************************************************
void wavTrigger::trackPlaySolo(int chan, int trk) {
  
  trackControl(chan, trk, TRK_LOOP_OFF);
  trackControl(chan, trk, TRK_STOP);
  trackControl(chan, trk, TRK_PLAY_SOLO);
}
extern char *msgbuf;

// **************************************************************
void wavTrigger::trackPlayPoly(int chan, int trk) {
  
  trackControl(chan, trk, TRK_LOOP_OFF);
  trackControl(chan, trk, TRK_STOP);
  trackControl(chan, trk, TRK_PLAY_POLY);
  /*sprintf(msgbuf, "PlayPoly Chan %d Trk %d", 
				 chan, trk	 );
 syslog(LOG_NOTICE, msgbuf);	*/
}

// **************************************************************
void wavTrigger::trackLoad(int chan, int trk) {
  
  trackControl(chan, trk, TRK_LOAD);
}

// **************************************************************
void wavTrigger::trackStop(int chan, int trk) {

  trackControl(chan, trk, TRK_STOP);
}

// **************************************************************
void wavTrigger::trackPause(int chan, int trk) {

  trackControl(chan, trk, TRK_PAUSE);
}

// **************************************************************
void wavTrigger::trackResume(int chan, int trk) {

  trackControl(chan, trk, TRK_RESUME);
}

// **************************************************************
void wavTrigger::trackLoop(int chan, int trk, bool enable) {
 
  if (enable)
    trackControl(chan, trk, TRK_LOOP_ON);
  else
    trackControl(chan, trk, TRK_LOOP_OFF);
}

// **************************************************************
void wavTrigger::trackControl(int chan, int trk, int code) {
  
char txbuf[20];
int len;
int i;
  if ( sioPort < 0 )
  {
	  return;
  }
	txbuf[0] = 0xf0;
	txbuf[1] = 0xaa;
	//txbuf[2] = 0x08;
	txbuf[3] = CMD_TRACK_CONTROL;
	txbuf[4] = (char)code;
	txbuf[5] = (char)trk;
	txbuf[6] = (char)(trk >> 8);
	//txbuf[7] = 0x55;
	//len = 8;
	
	if ( boardType == BOARD_WAV_TRIGGER )
	{
		txbuf[2] = 0x08;
		txbuf[7] = 0x55;
		len = 8;
	}
	else
	{
		txbuf[2] = 0x0A;
		txbuf[7] = chan;
		txbuf[8] =  0; // Flags
		txbuf[9] = 0x55;
		len = 10;
	}
	// printf("WAV %d Trk Control: %d %d %d: ",
		// wavIndex, chan, trk, code );
	// for ( i = 0 ; i < len ; i++ )
	// {
		// printf(" 0x%02x ", txbuf[i] );
	// }
	// printf("\n" );
	
  write(sioPort, txbuf, len);
}

// **************************************************************
void wavTrigger::stopAllTracks(void) {

char txbuf[10];
  if ( sioPort < 0 )
  {
	  return;
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x05;
  txbuf[3] = CMD_STOP_ALL;
  txbuf[4] = 0x55;
  write(sioPort, txbuf, 5);
}

// **************************************************************
void wavTrigger::resumeAllInSync(void) {

char txbuf[10];
  if ( sioPort < 0 )
  {
	  return;
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x05;
  txbuf[3] = CMD_RESUME_ALL_SYNC;
  txbuf[4] = 0x55;
  write(sioPort, txbuf, 5);
}

// **************************************************************
void wavTrigger::trackGain(int trk, int gain) {

char txbuf[10];
unsigned short vol;
  if ( sioPort < 0 )
  {
	  return;
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x09;
  txbuf[3] = CMD_TRACK_VOLUME;
  txbuf[4] = (char)trk;
  txbuf[5] = (char)(trk >> 8);
  vol = (unsigned short)gain;
  txbuf[6] = (char)vol;
  txbuf[7] = (char)(vol >> 8);
  txbuf[8] = 0x55;
  write(sioPort, txbuf, 9);
}

// **************************************************************
void wavTrigger::trackFade(int trk, int gain, int time, bool stopFlag) {

char txbuf[20];
unsigned short vol;
  if ( sioPort < 0 )
  {
	  return;
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x0c;
  txbuf[3] = CMD_TRACK_FADE;
  txbuf[4] = (char)trk;
  txbuf[5] = (char)(trk >> 8);
  vol = (unsigned short)gain;
  txbuf[6] = (char)vol;
  txbuf[7] = (char)(vol >> 8);
  txbuf[8] = (char)time;
  txbuf[9] = (char)(time >> 8);
  txbuf[10] = stopFlag;
  txbuf[11] = 0x55;
  write(sioPort, txbuf, 12);
}

// **************************************************************
void wavTrigger::trackCrossFade(int chan, int trkFrom, int trkTo, int gain, int time) {

char txbuf[20];
unsigned short vol;
  if ( sioPort < 0 )
  {
	  return;
  }

  // Start the To track with -40 dB gain
  trackGain(trkTo, -40);
  trackPlayPoly(chan, trkTo);

  // Start a fade-in to the target volume
  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x0c;
  txbuf[3] = CMD_TRACK_FADE;
  txbuf[4] = (char)trkTo;
  txbuf[5] = (char)(trkTo >> 8);
  vol = (unsigned short)gain;
  txbuf[6] = (char)vol;
  txbuf[7] = (char)(vol >> 8);
  txbuf[8] = (char)time;
  txbuf[9] = (char)(time >> 8);
  txbuf[10] = 0x00;
  txbuf[11] = 0x55;
  write(sioPort, txbuf, 12);

  // Start a fade-out on the From track
  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x0c;
  txbuf[3] = CMD_TRACK_FADE;
  txbuf[4] = (char)trkFrom;
  txbuf[5] = (char)(trkFrom >> 8);
  vol = (unsigned short)-40;
  txbuf[6] = (char)vol;
  txbuf[7] = (char)(vol >> 8);
  txbuf[8] = (char)time;
  txbuf[9] = (char)(time >> 8);
  txbuf[10] = 0x01;
  txbuf[11] = 0x55;
  write(sioPort, txbuf, 12);
}

// **************************************************************
void wavTrigger::samplerateOffset(int offset) {

char txbuf[8];
unsigned short off;
  if ( sioPort < 0 )
  {
	  return;
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x07;
  txbuf[3] = CMD_SAMPLERATE_OFFSET;
  off = (unsigned short)offset;
  txbuf[4] = (char)off;
  txbuf[5] = (char)(off >> 8);
  txbuf[6] = 0x55;
  write(sioPort, txbuf, 7);
}

// **************************************************************
void wavTrigger::ampPower(int on) {

char txbuf[8];
unsigned short off;
  if ( sioPort < 0 )
  {
	  return;
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x06;
  txbuf[3] = CMD_AMP_POWER;
  txbuf[4] = on;
  txbuf[5] = 0x55;
  write(sioPort, txbuf, 6);
}

// **************************************************************
int wavTrigger::getVersion(char *buf, int maxLen) {
char txbuf[10];
int len;
int i;
float ver;
char cc;
int year;
  if ( sioPort < 0 )
  {
	  return ( -1 );
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x05;
  txbuf[3] = CMD_GET_VERSION;
  txbuf[4] = 0x55;
  write(sioPort, txbuf, 6);
  len = getReturnData(buf, maxLen );
  
  if ( len == 0x19 || len == 21)
  {
	  // From Wav Trigger
	  boardType = BOARD_WAV_TRIGGER;
  }
  else if ( len == 0x1b || len == 23)
  {
	  boardType = BOARD_TSUNAMI;
	  i = sscanf(&buf[1], "Tsunami v%f%c (c)%d", &ver, &cc, &year );
	  if ( cc == 'm' )
	  {
		  tsunamiMode = TSUNAMI_MONO;
	  }
	  else
	  {
		  tsunamiMode = TSUNAMI_STEREO;
	  }
  }
  else
  {
	  boardType = BOARD_UNKNOWN;
	  sprintf(&buf[1], "Unknown Board 0x%02x 0x%02x %s", buf[0], buf[1], &buf[3] );
  }
  sprintf( boardFWVersion, "%.*s", len-1, &buf[1] );
  return(len);
}

// **************************************************************
int wavTrigger::getSysInfo(char *buf, int maxLen) {
char txbuf[8];
unsigned short off;
  if ( sioPort < 0 )
  {
	  return ( -1 );
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x05;
  txbuf[3] = CMD_GET_SYS_INFO;
  txbuf[4] = 0x55;
  write(sioPort, txbuf, 6);
  return(getReturnData(buf, maxLen ) );
}

// **************************************************************
int wavTrigger::getStatus(char *buf, int maxLen) {
char txbuf[8];
unsigned short off;
  if ( sioPort < 0 )
  {
	  return ( -1 );
  }

  txbuf[0] = 0xf0;
  txbuf[1] = 0xaa;
  txbuf[2] = 0x05;
  txbuf[3] = CMD_GET_STATUS;
  txbuf[4] = 0x55;
  write(sioPort, txbuf, 6);
  return(getReturnData(buf, maxLen ) );
}

// **************************************************************
int wavTrigger::getReturnData(char *buf, int maxLen) {
	int i;
	int state = 0;
	int sts;
	int len;
	int rec = 0;
	int loops = 0;
	
	if ( sioPort < 0 )
	{
	  return ( -1 );
	}
	memset(buf, 0, maxLen );
	// Read input until we get a Start (0xF0, 0xAA)
	for ( i = 0 ; i < ( maxLen + 4 ) ;  loops++ )
	{
		sts = read(sioPort, &buf[i], 1 );
		if ( sts > 0 )
		{
			switch ( state )
			{
				case 0:  // Read Start Byte 0
					if ( buf[i] == 0xF0 )
					{
						state++;
						//printf ("F0 " );
					}
					break;
				case 1: // Read Start Byte 1
					if ( buf[i] == 0xAA )
					{
						state++;
						//printf ("AA " );
					}
					break;
				case 2:  // Read Length
					len = buf[i] - 4;  // Number of Data bytes is the length minus Start, Len and Stop
					//printf ("%02x ", buf[i] );
					state++;
					break;
				case 3:
				    //printf ("%02x ", buf[i] );
					rec++;
					if ( rec < maxLen )
					{
						i++;
					}
					if ( rec == len )
					{
						state++;
					}
					break;
				case 4: // Read in Stop
				    //printf ("(STOP) %02x ", buf[i] );
					// Should be the end
					//printf("\n" );
					return ( rec );
					break;
			}
		}
		else
		{
			if ( loops++ > 20 )	// Timeout Check
			{
				//printf("\n" );
				return ( -1 );
			}
		}
	}
	return ( -1 );
}			

#define MAX_BUF 255

char trkStatusBuffer[MAX_BUF+1];
int tracksReturned = 0;

int wavTrigger::getTracksPlaying() {
	int tracks;
	int val;

	val = getStatus(trkStatusBuffer, MAX_BUF );
	tracks = (val-1)/2;
	tracksReturned = tracks;
	return ( tracks );
}

int wavTrigger::getTrackStatus(int trk) {
	int tracks;

	tracks = getTracksPlaying();
	return ( checkTrack(trk ) );
}

int wavTrigger::checkTrack(int trk )
{
	int i;
	int val;
	
	for ( i = 0 ; i < tracksReturned ; i++ )
	{
		val = ( trkStatusBuffer[2+(i*2)] << 8 ) + trkStatusBuffer[1+(i*2)];
		if ( val == trk )
		{
			return ( 1 );
		}
	}
	return ( 0 );
}
void wavTrigger::show(void) {
	printf("Board: " );
	switch ( boardType )
	{
		case BOARD_WAV_TRIGGER:
			printf("WAV Trigger " );
			break;
		case BOARD_TSUNAMI:
			printf("Tsunami (%s) ", tsunamiMode == 0  ? "Stereo" : "Mono" );
			
			break;
		default:
			printf("Unknown " );
			break;
	}
	printf("\nFW: %s\n", boardFWVersion );
}
wavTrigger::~wavTrigger(void)
{
	
}





