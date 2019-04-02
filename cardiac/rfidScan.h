/*
 * rfidScan.cpp
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