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
#include <iostream>
#include <math.h>
#include <string.h>
using namespace std;

struct pulseData pulseRTPStd[] =
{   { 5000, 255 },
	{ 500, 0 },
	{ 500, 255 },
	{ 0,    0 },
};
struct pulseData pulseRTPWeak[] =
{   { 5000, 160 },
	{ 500, 0 },
	{ 500, 160 },
	{ 0,    0 },
};

/*
	I2C Operation
	7-bit Slave, address is 0x5A 
		0xB4 for Writing
		0xB5 for Reading
	
*/
pulseI2C::pulseI2C(int bus )
{
	int addr;
	I2CBus = bus;
	int res;
	
	deviceID = 0x0000;
	
	snprintf(I2Cnamebuf, sizeof(I2Cnamebuf), "/dev/i2c-%d", I2CBus);
	sts = getI2CLock();
	if ( sts )
	{
		log_message("", "Could not get I2C Lock");
		return;
	}
    if ((I2Cfile = open(I2Cnamebuf, O_RDWR)) < 0)
	{
		perror("open");
    }

	addr = PULSE_BASE_ADDR;
	//printf("Left: Addr %05xh", addr );
	
	if ( ioctl(I2Cfile, I2C_SLAVE, addr ) < 0 )
	{
		printf(" NO Dev\n" );
	}
	printf("I2Cfile is %d\n", I2Cfile );
	
	res = readRegister(PULSE_REG_STATUS );
	if ( res < 0 )
	{
		perror("readRegister" );
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
				break;
		}
	}
	releaseI2CLock();
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
	sts = getI2CLock();
	if ( sts )
	{
		log_message("", "Could not get I2C Lock");
		return;
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
	sts = getI2CLock();
	if ( sts )
	{
		log_message("", "Could not get I2C Lock");
		return;
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

pulseI2C::autoCalibration(void )
{
	int res;
	int i;

	sts = getI2CLock();
	if ( sts )
	{
		log_message("", "Could not get I2C Lock");
		return (-1 );
	}
	printf("Starting Auto-Calibration\n" );

	/*	The following instructions list the step-by-step register configuration for auto-calibration. For additional details see
		the Register Map section.
		1. Apply the supply voltage to the DRV2605L device, and pull the EN pin high. The supply voltage should allow
		for adequate drive voltage of the selected actuator.	*/
	res = pulse.readRegister(PULSE_REG_STATUS );
	if ( res < 0 )
	{
		perror("Status Read" );
		releaseI2CLock();
		return ( -1 );
	}
	printf("Device Status is 0x%02x\n", res );

	res = pulse.readRegister(PULSE_REG_MODE );
	if ( res < 0 )
	{
		perror("Mode Read" );
		releaseI2CLock();
		return ( -1 );
	}
	printf("Device Mode is 0x%02x\n", res );

	/*	2. Write a value of 0x07 to register 0x01. This value moves the DRV2605L device out of STANDBY and places
		the MODE[2:0] bits in auto-calibration mode. */
	res = pulse.writeRegister(PULSE_REG_MODE, 0x07 );
	if ( res < 0 )
	{
		perror("Mode Write" );
		releaseI2CLock();
		return ( -1 );
	}

	/* 3. Populate the input parameters required by the auto-calibration engine: 
	   (a) ERM_LRA — selection will depend on desired actuator.
	   (b) FB_BRAKE_FACTOR[2:0] — A value of 2 is valid for most actuators.
	   (c) LOOP_GAIN[1:0] — A value of 2 is valid for most actuators. */
	res = pulse.writeRegister(PULSE_REG_FEEDBACK_CTL, 0x80 | ( 0x2 <<4 ) | ( 0x2 << 2 ) | 0x2 );
	if ( res < 0 )
	{
		perror("FB Write" );
		releaseI2CLock();
		return ( -1 );
	}

	/* (d) RATED_VOLTAGE[7:0] — See the Rated Voltage Programming section for calculating the correct
		register value. */
	res = pulse.writeRegister(PULSE_REG_RATED_VOLTAGE, 0x53 ); // From DRV2605 Setup Guide - 2Vrms
	if ( res < 0 )
	{
		perror("Rated Voltage Write" );
		releaseI2CLock();
		return ( -1 );
	}
	/* (e) OD_CLAMP[7:0] — See the Overdrive Voltage-Clamp Programming section for calculating the correct
		register value. */
	res = pulse.writeRegister(PULSE_REG_OD_CLAMP, 0x89 ); // From DRV2605 Setup Guide - 3 Vpeak
	if ( res < 0 )
	{
		perror("OD_CLAMP Write" );
		releaseI2CLock();
		return ( -1 );
	}

	/* (f) AUTO_CAL_TIME[1:0] — A value of 3 is valid for most actuators. 
	   (k) ZC_DET_TIME[1:0] — A value of 0 is valid for most actuators. */
	res = pulse.writeRegister(PULSE_REG_CONTROL_4, (0<<6) | (0x3<<4) ) ;
	if ( res < 0 )
	{
		perror("CONTROL_4 Write" );
		releaseI2CLock();
		return ( -1 );
	}

	/* (g) DRIVE_TIME[3:0] — See the Drive-Time Programming for calculating the correct register value. */
	res = pulse.writeRegister(PULSE_REG_CONTROL_1, 19 ); // 2.4 msec for Freq of 205Hz.
	if ( res < 0 )
	{
		perror("CONTROL_1 Write" );
		releaseI2CLock();
		return ( -1 );
	}

	/* (h) SAMPLE_TIME[1:0] — A value of 3 is valid for most actuators.
	   (i) BLANKING_TIME[3:0] — A value of 1 is valid for most actuators.
	   (j) IDISS_TIME[3:0] — A value of 1 is valid for most actuators. */
	res = pulse.writeRegister(PULSE_REG_CONTROL_2, (3<<4) | (1<<2) | 1 ); 
	if ( res < 0 )
	{
		perror("CONTROL_2 Write" );
		releaseI2CLock();
		return ( -1 );
	}


	/* 4. Set the GO bit (write 0x01 to register 0x0C) to start the auto-calibration process. When auto calibration is
	complete, the GO bit automatically clears. The auto-calibration results are written in the respective registers
	as shown in Figure 25. */
	res = pulse.writeRegister(PULSE_REG_GO,  1 );
	if ( res < 0 )
	{
		perror("GO Write" );
		releaseI2CLock();
		return ( -1 );
	}
	
	// Wait for completion before checking status
	for ( i = 0 ; i < 2000 ; i++ )
	{
		usleep(1000000 ); // 1 sec
		res = pulse.readRegister(PULSE_REG_GO );
		if ( res == 0 )
		{
			break;
		}
	}
	/* 5. Check the status of the DIAG_RESULT bit (in register 0x00) to ensure that the auto-calibration routine is
	complete without faults. */
	res = pulse.readRegister(PULSE_REG_STATUS );
	if ( res < 0 )
	{
		perror("Completion Status Read" );
		releaseI2CLock();
		return ( -1 );
	}
	printf("Device Status is 0x%02x\n", res );
	if ( res & 0x8 )
	{
		printf("Auto Calibration Failed\n" );
		releaseI2CLock();
		return ( -1 );
	}
	printf("Auto Calibration Succeeds!\n" );
	
	// Set mode to INTERNAL TRIGGER
	res = pulse.writeRegister(PULSE_REG_MODE, 0x0 );
	if ( res < 0 )
	{
		perror("Mode Write" );
		releaseI2CLock();
		return ( -1 );
	}
	releaseI2CLock();
	return ( 0 );
	
	/* 6. Evaluate system performance with the auto-calibrated settings. Note that the evaluation should occur during
	the final assembly of the device because the auto-calibration process can affect actuator performance and
	behavior. If any adjustment is needed, the inputs can be modified and this sequence can be repeated. If the
	performance is satisfactory, the user can do any of the following:
	(a) Repeat the calibration process upon subsequent power ups.
	(b) Store the auto-calibration results in host processor memory and rewrite them to the DRV2605L device
	upon subsequent power ups. The device retains these settings when in STANDBY mode or when the EN
	pin is low.
	(c) Program the results permanently in nonvolatile, on-chip OTP memory. Even when a device power cycle
	occurs, the device retains the auto-calibration settings. See the Programming On-Chip OTP Memory
	section for additional information.
	*/
}

pulseI2C::read_sense(void )
{
	char buf[8];
	int fd;
	int val;
	int on;
	
	fd = open (SENSE_PATH, O_RDONLY );
	if ( fd < 0 )
	{
		fprintf(stderr, "Failed to open AIN1" );
		perror("open" );
		return ( 0 );
	}
	
	read(fd, &buf, 4 );
	close(fd);
	val = atoi(buf );

	if ( ( val < TOUCH_HI_LIMIT ) && ( val >= TOUCH_MID_LIMIT ) )
	{
		on = 1;
	}
	else if ( ( val < TOUCH_LO_LIMIT ) && ( val > TOUCH_LO_LIMIT ) )
	{
		on = 2;
	}
	else
	{
		on = 0;
	}
	return ( on );
}

pulseI2C::runRTP(int sequence )
{
	int i;
	int res;
	struct pulseData *pulseRTP;
	
	sts = getI2CLock();
	if ( sts )
	{
		log_message("", "Could not get I2C Lock");
		return (-1 );
	}
	
	if ( sequence == 1 )
		pulseRTP = pulseRTPStd;
	else
		pulseRTP = pulseRTPWeak;
	
	// Set RTP Unsigned
	res = pulse.readRegister(PULSE_REG_CONTROL_3 );
	if ( res < 0 )
	{
		perror("Read Control 3" );
		releaseI2CLock();
		return ( -1 );
	}
	if ( ( res & 0x08 ) == 0 )
	{
		res |= 0x08;
		res = pulse.writeRegister(PULSE_REG_CONTROL_3, res );
		if ( res < 0 )
		{
			perror("Control 3 Write" );
			releaseI2CLock();
			return ( -1 );
		}
	}
	pulse.writeRegister(PULSE_REG_RTP_INPUT, 0 );
	// Set Mode to RTP
	res = pulse.writeRegister(PULSE_REG_MODE, 0x05 );
	if ( res < 0 )
	{
		perror("Mode Write" );
		releaseI2CLock();
		return ( -1 );
	}
	
	for ( i = 0 ; ; i++ )
	{
		pulse.writeRegister(PULSE_REG_RTP_INPUT, pulseRTP[i]->val );
		if ( pulseRTP[i]->time )
		{
			usleep(pulseRTP[i]->time );
		}
		else
		{
			break;
		}

	}
	
	// Set to standby mode
	res = pulse.writeRegister(PULSE_REG_MODE, 0x40 );
	if ( res < 0 )
	{
		perror("Mode Write" );
		releaseI2CLock();
		return ( -1 );
	}
	releaseI2CLock();
	return ( 0 );
}

pulseI2C::~pulseI2C()
{
	close(I2Cfile);
}

