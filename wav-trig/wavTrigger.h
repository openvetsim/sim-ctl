/*
 *	sim-ctl Audio "wav": wavTrigger.h
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
//     Filename: wavTrigger.h
// Date Created: 2/23/2014
//
//     Comments: Robertsonics WAV Trigger serial control library
//
// Programmers: Jamie Robertson, jamie@robertsonics.com
//
// **************************************************************
//
// Revision History (Arduino)
//
// Date      Description
// --------  -----------
//
// 02/22/14  First version created.
//           LIMITATIONS: Hard-coded for AltSoftwareSerial Library.
//           Also only supports commands TO the WAV Trigger. Will
//           fix these things.
//
// 05/10/14  Tested with UNO. Added new functions for fades, cross-
//           fades and starting multiple tracks in sample sync.
//
// 04/26/15  Added support for sample-rate / pitch bend control,
//           and compile macro switches for hardware serial ports.
//

#ifndef WAVTRIGGER_H
#define WAVTRIGGER_H

// Board Types
#define BOARD_UNKNOWN			-1
#define BOARD_WAV_TRIGGER		0
#define BOARD_TSUNAMI			1

// Tsunami Modes
#define TSUNAMI_STEREO			0
#define TSUNAMI_MONO			1

// Write Only Commands
#define CMD_TRACK_CONTROL		3
#define CMD_CONTROL_TRACK		3
#define CMD_STOP_ALL			4
#define CMD_MASTER_VOLUME		5
#define CMD_VOLUME				5
#define CMD_TRACK_VOLUME		8
#define CMD_AMP_POWER			9
#define CMD_TRACK_FADE			10
#define CMD_RESUME_ALL_SYNC		11
#define CMD_SAMPLERATE_OFFSET	12
#define CMD_SAMPLERATE			12


// Commands with Data Returned
#define CMD_GET_VERSION			1
#define CMD_GET_SYS_INFO		2
#define CMD_GET_STATUS			7

// From WAV Trigger
#define CMD_VERSION_STRING		0x81		// Return len is 25
#define CMD_SYS_INFO			0x82		// Return len is 8
#define CMD_STATUS				0x83		// Return Len is variable
	// Status:
	// The data is a list of 2-byte track numbers that are currently playing.
	// If there are no tracks playing, the number of data bytes will be 0.
	// Example: 0xf0, 0xaa, 0x09, 0x83, 0x01, 0x00, 0x0e, 0x00, 0x55
	//          start       len   op    trk 0x0001, trk 0x000e  end



#define TRK_PLAY_SOLO	0
#define TRK_PLAY_POLY	1
#define TRK_PAUSE		2
#define TRK_RESUME		3
#define TRK_STOP		4
#define TRK_LOOP_ON		5
#define TRK_LOOP_OFF	6
#define TRK_LOAD		7


class wavTrigger
{
public:
	wavTrigger(void );
	virtual ~wavTrigger();
	
	void start(int sioPort, int index);
	void masterGain(int gain);
	void channelGain(int chan, int gain);
	void stopAllTracks(void);
	void resumeAllInSync(void);
	void trackPlaySolo(int chan, int trk);
	void trackPlayPoly(int chan, int trk);
	void trackLoad(int chan, int trk);
	void trackStop(int chan, int trk);
	void trackPause(int chan, int trk);
	void trackResume(int chan, int trk);
	void trackLoop(int chan, int trk, bool enable);
	void trackGain(int trk, int gain);
	void trackFade(int trk, int gain, int time, bool stopFlag);
	void trackCrossFade(int chan, int trkFrom, int trkTo, int gain, int time);
	void samplerateOffset(int offset);
	void ampPower(int on);
	int getVersion(char *buf, int maxLen );
	int getSysInfo(char *buf, int maxLen );
	int getStatus(char *buf, int maxLen );
	void waitTilDone(int trk );
	int getTracksPlaying(); // Gathers track status and returns the number of tracks playing
	int getTrackStatus(int trk); // Returns 1 if the track is playing, else 0. Gathers track status and returns the status of the indicated track
	int checkTrack(int trk ); // Checks the already gathered status and returns the status for the track
	void show(void );
	int wavIndex;
	
	int boardType;
	char boardFWVersion[32];
	int tsunamiMode;
	
private:
	void trackControl(int chan, int trk, int code);
	int getReturnData(char *buf, int maxLen );
	int	sioPort;	// The current port

};

#endif
