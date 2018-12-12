/*
 * pulseI2C.h
 * Definition of a class to interface with the Simulator Pulse Generator
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

#ifndef PULSEI2C_H_
#define PULSEI2C_H_
#define PULSE_I2C_BUFFER 0x80
#define PULSE_I2C_MAX_DEVS 1
#define MAX_BUS 64
#define PULSE_BASE_ADDR		0x5A

class pulseI2C {

private:
	int I2CBus;
	char I2CdataBuffer[PULSE_I2C_BUFFER];
	char I2Cnamebuf[MAX_BUS];
	

public:
	int deviceID;
	int I2Cfile;
	pulseI2C(int bus);
	int readRegister(int reg );
	int writeRegister(int reg, unsigned char val );
	virtual ~pulseI2C();
};

// Register Map
#define	PULSE_REG_STATUS		0x00
	#define STATUS_DEVICE_ID_BITS		0xE0	// 4 - DRV2604 (RAM, No ROM)
												// 3 - DRV2605 (Licensed ROM, No RAM)
												// 6 - DRV2604L Low Voltage Version
												// 7 - DRV2605L Low Voltage Version
	#define STATUS_DIAG_RESULT			0x08	// 0 - OK, 1 - Not OK
	#define STATUS_OVER_TEMP			0x02	// Over Temp Detected
	#define STATUS_OC_DETECT			0x01	// Overcurrent Detected
	
#define PULSE_REG_MODE			0x01
	#define MODE_RESET					0x80	// Set to reset device
	#define MODE_STANDBY				0x40	// Set to enter standby mode
	#define MODE_MODE					0x07	// 0 - Internal Trigger
												// 1 - External Trigger - Edge (IN/TRIG Pin)
												// 2 - External Trigger - Level (IN/TRIG Pin)
												// 3 - PWM input & Analog Input
												// 4 - reserved
												// 5 - Real-Time Playback (RTP)
												// 6 - Diagnostics
												// 7 - Auto Calibration
												
#define PULSE_REG_RTP_INPUT		0x02
	// Entry point for Real Time Playback
	
#define PULSE_REG_HI_Z			0x03
	// Set bit 0x10 to set outputs to Hi-Z mode
#define PULSE_REG_LIBRARY		0x03


// Waveform Sequence Pointer to RAM library
#define PULSE_REG_WAV_FRM_SEQ_1	0x04
#define PULSE_REG_WAV_FRM_SEQ_2	0x05
#define PULSE_REG_WAV_FRM_SEQ_3	0x06
#define PULSE_REG_WAV_FRM_SEQ_4	0x07
#define PULSE_REG_WAV_FRM_SEQ_5	0x08
#define PULSE_REG_WAV_FRM_SEQ_6	0x09
#define PULSE_REG_WAV_FRM_SEQ_7	0x0A
#define PULSE_REG_WAV_FRM_SEQ_8	0x0B

#define PULSE_REG_GO			0x0C
	// Set 0x01 to GO (Fire Sequence)
	
#define PULSE_REG_OTO			0x0D	// Overdrive Time Offset
#define PULSE_REG_SPT			0x0E	// Sustain Time Offset, Positive
#define PULSE_REG_SNT			0x0F	// Sustain Time Offset, Negative
#define PULSE_REG_BRT			0x10	// Brake Time Offset

// 2605 Only
#define ATH_CONTROL				0x11	// Auto-to-Vibe Control
#define	ATH_MIN_INPUT			0x12	 
#define	ATH_MAX_INPUT			0x13
#define	ATH_MIN_DRIVE			0x14
#define	ATH_MAX_DRIVE			0x15

#define PULSE_REG_RATED_VOLTAGE	0x16	// reference voltage for full-scale output during closed-loop operation
#define PULSE_REG_OD_CLAMP		0x17
#define PULSE_REG_A_CAL_COMP	0x18
#define PULSE_REG_A_CAL_BEMF	0x19
#define PULSE_REG_FEEDBACK_CTL	0x1A
	// Must set ERM_LRA bit to indicate LRA
	
#define PULSE_REG_CONTROL_1		0x1B
#define PULSE_REG_CONTROL_2		0x1C
#define PULSE_REG_CONTROL_3		0x1D
#define PULSE_REG_CONTROL_4		0x1E
#define PULSE_REG_CONTROL_5		0x1F
#define PULSE_REG_OL_LRA_PERIOD	0x20	// Open Loop Period
#define PULSE_REG_VBAT			0x21	// Voltage monitor Vdd = (VBAT) x 5.6V / 255
#define PULSE_REG_LRA_PERIOD	0x22	// Resonance Perior

// 2604 Only
#define PULSE_REG_RAM_ADDR_UB	0xFD	// RAM Address, Upper Byte
#define PULSE_REG_RAM_ADDR_LB	0xFE	// RAM Address, Lower Byte
#define PULSE_REG_RAM_DATA		0xFF	// RAM Data Byte Register

/* Library definitions
	Library		Rated Voltage	Overdrive Voltage	Rise Time	Brake Time
	A			1.3V			3V					40-60ms		20-40ms
	B			3V				3V					40-60ms		5-15ms
	C			3V				3V					60-80ms		10-20ms
	D			3V				3V					100-140ms	15-25ms
	E			3V				3V					> 140ms		> 30ms
	F			4.5V			5V					35-45ms		10-20ms
	(Library F is for LRAs, 1-E for ERMs)
	
	Select Library in Register 0x03
*/

#endif /* PULSEI2C_H_ */
