/*
 * pulseI2C.cpp
 * Definition of a class to interface with the Simulator Pulse units
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
#include "pulseI2C.h"
#include "../comm/simUtil.h"
#include <iostream>
#include <math.h>
#include <string.h>
#include <errno.h>

using namespace std;

/*
	I2C Operation
	7-bit Slave, address is 0x5A 
		0xB4 for Writing
		0xB5 for Reading
	
*/

char emsgbuf[2048];

pulseI2C::pulseI2C(int bus )
{
	int addr;
	int res;
	
	I2CBus = bus;
	deviceID = 0x0000;

	snprintf(I2Cnamebuf, sizeof(I2Cnamebuf), "/dev/i2c-%d", I2CBus);

    if ((I2Cfile = open(I2Cnamebuf, O_RDWR)) < 0)
	{
		perror("pulseI2C open");
    }
	else
	{
		addr = PULSE_BASE_ADDR;
		
		if ( ioctl(I2Cfile, I2C_SLAVE, addr ) < 0 )
		{
			perror("pulseI2C ioctl" );
		}
		else
		{
			res = readRegister(PULSE_REG_STATUS );
			if ( res < 0 )
			{
				perror("pulseI2C readRegister" );
			}
			else
			{
				switch ( ( res & 0xE0 ) >> 5 )
				{
					case 7:
					case 3:
						deviceID = 0x2605;
						break;
					case 6:
					case 4:
						deviceID = 0x2604;
						break;
					default:
						deviceID = 0x0000;
						break;
				}
			}
		}
	}
}

int 
pulseI2C::readRegister(int reg )
{
	int status;
	struct i2c_msg i2cMsg[2];
	struct i2c_rdwr_ioctl_data ioctl_data;
	__u8 in_buf[4];
	__u8 out_buf[4];
	
	//struct i2c_rdwr_ioctl_data {
	//	struct i2c_msg *msgs;  /* ptr to array of simple messages */
	//	int nmsgs;             /* number of messages to exchange */
	//}

	out_buf[0] = reg;
    i2cMsg[0].addr = PULSE_BASE_ADDR;
	i2cMsg[0].flags = 0;
	i2cMsg[0].len = 1;
	i2cMsg[0].buf = out_buf;
    i2cMsg[1].addr = PULSE_BASE_ADDR;
	i2cMsg[1].flags = I2C_M_RD;
	i2cMsg[1].len = 1;
	i2cMsg[1].buf = in_buf;
	ioctl_data.nmsgs = 2;
	ioctl_data.msgs = &i2cMsg[0];
	//sts = getI2CLock();
	//if ( sts )
	//{
	//	printf("readRegister Could not get I2C Lock\n" );
	//	return ( -1 );
	//}
	status = ioctl(I2Cfile, I2C_RDWR, &ioctl_data );
	releaseI2CLock();
	if ( status < 0 )
	{
		perror("ioctl" );
		sprintf(emsgbuf, "pulseI2C::readRegister: I2C_M_RD Addr 0x%x returns %d, errno %d - %s\n", PULSE_BASE_ADDR, status, errno, strerror(errno) );
		log_message(NULL, emsgbuf );
		
		return ( -1 );
	}
	return ( (int)in_buf[0] );
}

int 
pulseI2C::writeRegister(int reg, unsigned char val )
{
	int status;
	struct i2c_msg i2cMsg[1];
	struct i2c_rdwr_ioctl_data ioctl_data;
	__u8 out_buf[4];
	
	out_buf[0] = (__u8)reg;
	out_buf[1] = val;
    i2cMsg[0].addr = PULSE_BASE_ADDR;
	i2cMsg[0].flags = 0;
	i2cMsg[0].len = 2;
	i2cMsg[0].buf = out_buf;
	ioctl_data.nmsgs = 1;
	ioctl_data.msgs = &i2cMsg[0];
	
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

pulseI2C::~pulseI2C()
{
	close(I2Cfile);
}

