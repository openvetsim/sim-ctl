/*
 * rfidScan.cpp
 *
 * Definition of a class to interface with the Simulator RFID Chest Sensor
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

#ifndef RFIDSCAN_H_
#define RFIDSCAN_H_

#define RFID_SHM_NAME	"rfidSense"
#define MAX_RFID_TAGS	128

// The data for the rfid tags will be pulled from a .ini file
struct rfidTag
{
	uint64_t tagId;			// 40-bit ID of the tag
	int side;				// 0 is left side, 1 is right side
	// Volume levels are -70 to +10
	int heartStrength;		// Strength of Heart sound
	int leftLungStrength;	// Strength of Left Lung sound
	int rightLungStrength;	// Strength of Right Lung sound
	int xPosition;
	int yPosition;
};
	
struct rfidData 
{
	unsigned int tagCount;		// Count of configured tags
	int tagDetected;			// 0 is no current detection, 1 is detected
	int lastTagDetected;		// Index of the most recent tag detected
	struct rfidTag	tags[MAX_RFID_TAGS];	// Tag Data
};

// For Parsing the XML:
#define PARAMETER_NAME_LENGTH	128
struct xml_level
{
	int num;
	char name[PARAMETER_NAME_LENGTH];
};


#endif /* RFIDSCAN_H_ */