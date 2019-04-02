/*
 * simUtil.h
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

#ifndef SIMUTIL_H_
#define SIMUTIL_H_

void daemonize(void );
void log_message(const char *filename, const char* message);
void signal_handler(int sig );
void catchFaults(void );

int initSHM(int create );

// Analog Input Assignments
#define BREATH_AIN_CHANNEL			0
#define TOUCH_SENSE_AIN_CHANNEL_1	1
#define AIR_PRESSURE_AIN_CHANNEL	2
#define TOUCH_SENSE_AIN_CHANNEL_2	3
#define TOUCH_SENSE_AIN_CHANNEL_3	4
#define TOUCH_SENSE_AIN_CHANNEL_4	5

int read_ain(int chan );		// Read Analog Input Channel
int getI2CLock(void );
void releaseI2CLock(void );
void cleanString(char *strIn );
char* itoa(int num );

// GPIO Access
#define GPIO_TURN_ON	1
#define GPIO_TURN_OFF	0
#define GPIO_INPUT		1
#define GPIO_OUTPUT		0

FILE *gpioPinOpen(int pin, int direction );
void gpioPinSet(FILE *ioval, int val );
int gpioPinGet(FILE *ioval, int *value);
int gpioPinRead(int pin, int *value);

#endif /* SIMUTIL_H_ */