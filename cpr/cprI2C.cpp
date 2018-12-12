/*
 * cprI2C.cpp
 * Definition of a class to interface with the Simulator Chest Sensors
 *
 * Copyright (C) Terry Kelleher
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <stdio.h>
#include "cprI2C.h"
#include <iostream>
#include <math.h>
#include <string.h>
#include "../comm/simUtil.h"
#include "../comm/shmData.h"
extern int debug;

using namespace std;

cprI2C::cprI2C(int dummy )
{
	present = 0;
	
	(void)scanForSensor();
}

int cprI2C::scanForSensor(void )
{
	int cc;
	unsigned char reg;
	
	for ( I2CBus = 1 ; I2CBus < 3 ; I2CBus++ )
	{
		snprintf(I2Cnamebuf, sizeof(I2Cnamebuf), "/dev/i2c-%d", I2CBus);
		if ((I2Cfile = open(I2Cnamebuf, O_RDWR)) < 0)
		{
			continue;
		}
		for ( I2CAddr = CPR_BASE_ADDR ; I2CAddr <= CPR_MAX_ADDR ; I2CAddr++ )
		{
			if ( ioctl(I2Cfile, I2C_SLAVE, I2CAddr ) >= 0 )
			{
				cc = readRegister(WHO_AM_I );
				if ( debug )
				{
					printf("WHO_AM_I returns 0x%x\n", cc );
				}
				if ( cc != 0x33 )
				{
					if ( debug ) 
					{	
						printf("Device is not LIS3DH\n" );
					}
					releaseI2CLock();
					present = 0;
				}
				else
				{
					// Found! Reset device 
					cc = writeRegister(CTRL_REG5, CTRL_REG5 );
					
					// Wait 5 msec (or more)
					usleep(20000 );
					
					// Set mode
					reg = ( CR1_ODR_100Hz << 4 )  | CR1_ZEN | CR1_YEN | CR1_XEN;
					cc = writeRegister(CTRL_REG1, reg );
					
					// Read back to see if it took
					cc = readRegister(CTRL_REG1 );
					if ( cc != (int)reg )
					{
						printf("write to CTRL_REG1 failed\n" );
						present = 0;
					}
					else
					{
						present = 1;
					
						// Enable Block Data Update
						reg = ( CR4_BDU );
						cc = writeRegister(CTRL_REG4, reg );
						
						// Enable Temp
						//reg = (TEMP_ADC_PD | TEMP_TEMP_EN );
						//cc = writeRegister(TEMP_CFG_REG, reg );
						//temperature = -1;
						return ( present );
					}
				}
			}
		}
	}
	return ( present );
}

int cprI2C::readRegister(int reg )
{
	int status;
	struct i2c_msg i2cMsg[2];
	struct i2c_rdwr_ioctl_data ioctl_data;
	__u8 in_buf[4];
	__u8 out_buf[4];
	int sts;
	
	//struct i2c_rdwr_ioctl_data {
	//	struct i2c_msg *msgs;  /* ptr to array of simple messages */
	//	int nmsgs;             /* number of messages to exchange */
	//}

	out_buf[0] = reg;
    i2cMsg[0].addr = I2CAddr;
	i2cMsg[0].flags = 0;
	i2cMsg[0].len = 1;
	i2cMsg[0].buf = out_buf;
    i2cMsg[1].addr = I2CAddr;
	i2cMsg[1].flags = I2C_M_RD;
	i2cMsg[1].len = 1;
	i2cMsg[1].buf = in_buf;
	ioctl_data.nmsgs = 2;
	ioctl_data.msgs = &i2cMsg[0];
	sts = getI2CLock();
	if ( sts )
	{
		printf("cprI2C::readRegister: Could not get I2C Lock\n" );
		return ( -1 );
	}
	status = ioctl(I2Cfile, I2C_RDWR, &ioctl_data );
	releaseI2CLock();
	if ( status < 0 )
	{
		perror("ioctl" );
		printf("I2Cfile is %d\n", I2Cfile );
		return ( -1 );
	}
	return ( (int)in_buf[0] );
}
int cprI2C::readRegister16(int reg )
{
	int status;
	struct i2c_msg i2cMsg[2];
	struct i2c_rdwr_ioctl_data ioctl_data;
	__u8 in_buf[4];
	__u8 out_buf[4];
	int sts;
	
	//struct i2c_rdwr_ioctl_data {
	//	struct i2c_msg *msgs;  /* ptr to array of simple messages */
	//	int nmsgs;             /* number of messages to exchange */
	//}

	out_buf[0] = reg;
    i2cMsg[0].addr = I2CAddr | 0x80; // Set bit x80 for Auto increment
	i2cMsg[0].flags = 0;
	i2cMsg[0].len = 1;
	i2cMsg[0].buf = out_buf;
    i2cMsg[1].addr = I2CAddr;
	i2cMsg[1].flags = I2C_M_RD;
	i2cMsg[1].len = 2;
	i2cMsg[1].buf = in_buf;
	ioctl_data.nmsgs = 2;
	ioctl_data.msgs = &i2cMsg[0];
	sts = getI2CLock();
	if ( sts )
	{
		return ( -1 );
	}
	status = ioctl(I2Cfile, I2C_RDWR, &ioctl_data );
	releaseI2CLock();
	if ( status < 0 )
	{
		perror("ioctl" );
		printf("I2Cfile is %d\n", I2Cfile );
		return ( -1 );
	}
	return ( (int)in_buf[0] | ( (int)in_buf[1] << 8 ) );
}
int cprI2C::writeRegister(int reg, unsigned char val )
{
	int status;
	struct i2c_msg i2cMsg[1];
	struct i2c_rdwr_ioctl_data ioctl_data;
	__u8 out_buf[4];
	int sts;
	
	out_buf[0] = (__u8)reg;
	out_buf[1] = val;
    i2cMsg[0].addr = I2CAddr;
	i2cMsg[0].flags = 0;
	i2cMsg[0].len = 2;
	i2cMsg[0].buf = out_buf;
	ioctl_data.nmsgs = 1;
	ioctl_data.msgs = &i2cMsg[0];
	sts = getI2CLock();
	if ( sts )
	{
		return ( -1 );
	}
	status = ioctl(I2Cfile, I2C_RDWR, &ioctl_data );
	releaseI2CLock();
	if ( status < 0 )
	{
		perror("ioctl" );
		printf("I2Cfile is %d\n", I2Cfile );
		return ( -1 );
	}
	return ( 0 );
}

int cprI2C::readSensor()
{
	int status;
	unsigned int value;
	short reading;

	//float degreeC;
	//float degreeF;
	
	status = readRegister(STATUS_REG_AUX );
	if ( ( status & (SRA_3DA) ) == (SRA_3DA) )
	{
		value = readRegister(OUT_ADC3_L );
		value |= ( readRegister(OUT_ADC3_H ) << 8 );
		reading = (short)value;
		// Assuming temp is degrees C, convert to F
		//degreeC = (float)reading / 100;
		//degreeF = ( (degreeC * 9 )/5 ) + 32;
		//temperature = degreeF;
	}
	
	// See if Data is present
	status = readRegister(STATUS_REG );
	if ( ( status & (SR_ZDA|SR_YDA|SR_XDA) ) != (SR_ZDA|SR_YDA|SR_XDA) )
	{
		return ( 0 );
	}
	value = readRegister(OUT_X_L );
	value |= ( readRegister(OUT_X_H ) << 8 );
	reading = (short)value;
	readingX = reading;
	
	value = readRegister(OUT_Y_L );
	value |= ( readRegister(OUT_Y_H ) << 8 );
	reading = (short)value;
	readingY = reading;
	
	value = readRegister(OUT_Z_L );
	value |= ( readRegister(OUT_Z_H ) << 8 );
	reading = (short)value;
	readingZ = reading;
	
	return ( status );
}

cprI2C::~cprI2C()
{
	close(I2Cfile);
}

