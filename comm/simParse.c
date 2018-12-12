/*
 * simParse.cpp
 *
 * Parser for SimCtl status
 * 
 * This file is similar to the parser in SimMgr, but has a smaller command set.
 *
 * Copyright (C) 2016 Terence Kelleher. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include "shmData.h"

extern int debug;

int
cardiac_parse(const char *elem,  const char *value, struct cardiac *card )
{
	int sts = 0;
	int int_val = atoi(value );
	int pulse_val;
	
	if ( ( ! elem ) || ( ! value) || ( ! card ) )
	{
		return ( -11 );
	}
	if ( strcmp(elem, ("rhythm" ) ) == 0 )
	{
		if ( strcmp(value, card->rhythm ) != 0 )
		{
			if ( debug )
			{
				printf("Cardiac rhythm: %s (old %s)\n", value, card->rhythm );
			}
			sprintf(card->rhythm, "%s", value );
		}
	}
	else if ( strcmp(elem, ("vpc" ) ) == 0 )
	{
		if ( strcmp(value, card->vpc ) != 0 )
		{
			if ( debug )
			{
				printf("Cardiac vpc: %s\n", value );
			}
			sprintf(card->vpc, "%s", value );
		}
	}
	else if ( strcmp(elem, ("pea" ) ) == 0 )
	{
		if ( int_val != card->pea )
		{
			if ( debug )
			{
				printf("Cardiac pea: %d\n", int_val );
			}
			card->pea = int_val;
		}
	}
	else if ( strcmp(elem, ("vpc_freq" ) ) == 0 )
	{
		if ( int_val != card->vpc_freq )
		{
			if ( debug )
			{
				printf("Cardiac vpc_freq: %d\n", int_val );
			}
			card->vpc_freq = int_val;
		}
	}
	else if ( strcmp(elem, ("vfib_amplitude" ) ) == 0 )
	{
		if ( strcmp(value, card->vfib_amplitude ) != 0 )
		{
			if ( debug )
			{
				printf("Cardiac vfib_amplitude: %s\n", value );
			}
			sprintf(card->vfib_amplitude, "%s", value );
		}
	}
	else if ( strcmp(elem, ("pwave" ) ) == 0 )
	{
		if ( strcmp(value, card->pwave ) != 0 )
		{
			if ( debug )
			{
				printf("Cardiac pwave: %s\n", value );
			}
			sprintf(card->pwave, "%s", value );
		}
	}
	else if ( strcmp(elem, ("rate" ) ) == 0 )
	{
		if ( int_val != card->rate )
		{
			if ( debug )
			{
				printf("Cardiac rate: %d\n", int_val );
			}
			card->rate = int_val;
		}
	}
	else if ( strcmp(elem, ("pr_interval" ) ) == 0 )
	{
		if ( int_val != card->pr_interval )
		{
			if ( debug )
			{
				printf("Cardiac pr_interval: %d\n", int_val );
			}
			card->pr_interval = int_val;
		}
	}
	else if ( strcmp(elem, ("qrs_interval" ) ) == 0 )
	{
		if ( int_val != card->qrs_interval )
		{
			if ( debug )
			{
				printf("Cardiac qrs_interval: %d\n", int_val );
			}
			card->qrs_interval = int_val;
		}
	}
	else if ( strcmp(elem, ("bps_sys" ) ) == 0 )
	{
		if ( int_val != card->bps_sys )
		{
			if ( debug )
			{
				printf("Cardiac bps_sys: %d\n", int_val );
			}
			card->bps_sys = int_val;
		}
	}
	else if ( strcmp(elem, ("bps_dia" ) ) == 0 )
	{
		if ( int_val != card->bps_dia )
		{
			if ( debug )
			{
				printf("Cardiac bps_dia: %d\n", int_val );
			}
			card->bps_dia = int_val;
		}
	}
	else if ( strcmp(elem, ("nibp_rate" ) ) == 0 )
	{
		if ( int_val != card->nibp_rate )
		{
			if ( debug )
			{
				printf("Cardiac nibp_rate: %d\n", int_val );
			}
			card->nibp_rate = int_val;
		}
	}
	else if ( strcmp(elem, "nibp_read" ) == 0 )
	{
		if ( int_val != card->nibp_read )
		{
			if ( debug )
			{
				printf("Cardiac nibp_read: %d\n", int_val );
			}
			card->nibp_read = int_val;
		}
	}
	else if ( strcmp(elem, "nibp_freq" ) == 0 )
	{
		if ( int_val != card->nibp_freq )
		{
			if ( debug )
			{
				printf("Cardiac nibp_freq: %d\n", int_val );
			}
			card->nibp_freq = int_val;
		}
	}
	
	
	else if ( strcmp(elem, ("heart_sound_volume" ) ) == 0 )
	{
		if ( int_val != card->heart_sound_volume )
		{
			if ( debug )
			{
				printf("Cardiac heart_sound_volume: %d\n", int_val );
			}
			card->heart_sound_volume = int_val;
		}
	}
	
	else if ( strcmp(elem, ("heart_sound_mute" ) ) == 0 )
	{
		if ( int_val != card->heart_sound_mute )
		{
			if ( debug )
			{
				printf("Cardiac heart_sound_mute: %d\n", int_val );
			}
			card->heart_sound_mute = int_val;
		}
		card->heart_sound_mute = atoi(value );
	}
	else if ( strcmp(elem, ("heart_sound" ) ) == 0 )
	{
		if ( strcmp(value, card->heart_sound ) != 0 )
		{
			if ( debug )
			{
				printf("Cardiac heart_sound: %s\n", value );
			}
		}
		sprintf(card->heart_sound, "%s", value );
	}
	else if ( strcmp(elem, ("right_dorsal_pulse_strength" ) ) == 0 )
	{
		if ( (strcmp(value, "none" ) ) == 0 )
		{
			pulse_val = 0;
		}
		else if ( (strcmp(value, "weak" ) ) == 0 )
		{
			pulse_val = 1;
		}
		else if ( (strcmp(value, "medium" ) ) == 0 )
		{
			pulse_val = 2;
		}
		else if ( (strcmp(value, "strong" ) ) == 0 )
		{
			pulse_val = 3;
		}
		else
		{
			sts = 3;
		}
		if ( ( sts != 3 ) && ( card->right_dorsal_pulse_strength != pulse_val ) )
		{
			if ( debug )
			{
				printf("Cardiac right_dorsal_pulse_strength: %d\n", pulse_val );
			}
			card->right_dorsal_pulse_strength = pulse_val;
		}
	}
	else if ( strcmp(elem, ("left_dorsal_pulse_strength" ) ) == 0 )
	{
		if ( (strcmp(value, "none" ) ) == 0 )
		{
			pulse_val = 0;
		}
		else if ( (strcmp(value, "weak" ) ) == 0 )
		{
			pulse_val = 1;
		}
		else if ( (strcmp(value, "medium" ) ) == 0 )
		{
			pulse_val = 2;
		}
		else if ( (strcmp(value, "strong" ) ) == 0 )
		{
			pulse_val = 3;
		}
		else
		{
			sts = 3;
		}
		if ( ( sts != 3 ) && ( card->left_dorsal_pulse_strength != pulse_val ) )
		{
			if ( debug )
			{
				printf("Cardiac left_dorsal_pulse_strength: %d\n", pulse_val );
			}
			card->left_dorsal_pulse_strength = pulse_val;
		}
	}
	else if ( strcmp(elem, ("right_femoral_pulse_strength" ) ) == 0 )
	{
		if ( (strcmp(value, "none" ) ) == 0 )
		{
			pulse_val = 0;
		}
		else if ( (strcmp(value, "weak" ) ) == 0 )
		{
			pulse_val = 1;
		}
		else if ( (strcmp(value, "medium" ) ) == 0 )
		{
			pulse_val = 2;
		}
		else if ( (strcmp(value, "strong" ) ) == 0 )
		{
			pulse_val = 3;
		}
		else
		{
			sts = 3;
		}
		if ( ( sts != 3 ) && ( card->right_femoral_pulse_strength != pulse_val ) )
		{
			if ( debug )
			{
				printf("Cardiac right_femoral_pulse_strength: %d\n", pulse_val );
			}
			card->right_femoral_pulse_strength = pulse_val;
		}
	}
	else if ( strcmp(elem, ("left_femoral_pulse_strength" ) ) == 0 )
	{
		if ( (strcmp(value, "none" ) ) == 0 )
		{
			pulse_val = 0;
		}
		else if ( (strcmp(value, "weak" ) ) == 0 )
		{
			pulse_val = 1;
		}
		else if ( (strcmp(value, "medium" ) ) == 0 )
		{
			pulse_val = 2;
		}
		else if ( (strcmp(value, "strong" ) ) == 0 )
		{
			pulse_val = 3;
		}
		else
		{
			sts = 3;
		}
		if ( ( sts != 3 ) && ( card->left_femoral_pulse_strength != pulse_val ) )
		{
			if ( debug )
			{
				printf("Cardiac left_femoral_pulse_strength: %d\n", pulse_val );
			}
			card->left_femoral_pulse_strength = pulse_val;
		}
	}
	else
	{
		sts = 1;
	}
	return ( sts );
}

int
respiration_parse(const char *elem,  const char *value, struct respiration *resp )
{
	int sts = 0;
	int int_val = atoi(value );
	
	if ( ( ! elem ) || ( ! value) || ( ! resp ) )
	{
		return ( -12 );
	}
	if ( strcmp(elem, "inhalation_duration" ) == 0 )
	{
		if ( int_val != resp->inhalation_duration )
		{
			if ( debug )
			{
				printf("Respiration inhalation_duration: %d\n", int_val );
			}
			resp->inhalation_duration = int_val;
		}
	}
	else if ( strcmp(elem, "exhalation_duration" ) == 0 )
	{
		if ( int_val != resp->exhalation_duration )
		{
			if ( debug )
			{
				printf("Respiration exhalation_duration: %d\n", int_val );
			}
		}
		resp->exhalation_duration = int_val;
	}
	
	else if ( strcmp(elem, "left_lung_sound_volume" ) == 0 )
	{
		if ( int_val != resp->left_lung_sound_volume )
		{
			if ( debug )
			{
				printf("Respiration left_lung_sound_volume: %d\n", int_val );
			}
		}
		resp->left_lung_sound_volume = int_val;
	}
	
	else if ( strcmp(elem, "left_lung_sound_mute" ) == 0 )
	{
		if ( int_val != resp->left_lung_sound_mute )
		{
			if ( debug )
			{
				printf("Respiration left_lung_sound_mute: %d\n", int_val );
			}
			resp->left_lung_sound_mute = int_val;
		}
	}
	
	else if ( strcmp(elem, "right_lung_sound_volume" ) == 0 )
	{
		if ( int_val != resp->right_lung_sound_volume )
		{
			if ( debug )
			{
				printf("Respiration right_lung_sound_volume: %d\n", int_val );
			}
			resp->right_lung_sound_volume = int_val;
		}
	}
	
	else if ( strcmp(elem, "right_lung_sound_mute" ) == 0 )
	{
		if ( int_val != resp->right_lung_sound_mute )
		{
			if ( debug )
			{
				printf("Respiration right_lung_sound_mute: %d\n", int_val );
			}
			resp->right_lung_sound_mute = int_val;
		}
	}
	else if ( strcmp(elem, "left_lung_sound" ) == 0 )
	{
		if ( strcmp(value, resp->left_lung_sound ) != 0 )
		{
			if ( debug )
			{
				printf("Respiration left_lung_sound: %s\n", value );
			}
			sprintf(resp->left_lung_sound, "%s", value );
		}
	}
	else if ( strcmp(elem, "right_lung_sound" ) == 0 )
	{
		if ( strcmp(value, resp->right_lung_sound ) != 0 )
		{
			if ( debug )
			{
				printf("Respiration right_lung_sound: %s\n", value );
			}
			sprintf(resp->right_lung_sound, "%s", value );
		}
	}
	else if ( strcmp(elem, "rate" ) == 0 )
	{
		if ( int_val != resp->rate )
		{
			if ( debug )
			{
				printf("Respiration rate: %d\n", int_val );
			}
			resp->rate = int_val;
		}
	}
	else if ( strcmp(elem, "awRR" ) == 0 )
	{
		if ( int_val != resp->awRR )
		{
			if ( debug )
			{
				printf("Respiration awRR: %d\n", int_val );
			}
			resp->awRR = int_val;
		}
	}
	else if ( strcmp(elem, "chest_movement" ) == 0 )
	{
		if ( int_val != resp->chest_movement )
		{
			if ( debug )
			{
				printf("Respiration chest_movement: %d\n", int_val );
			}
			resp->chest_movement = int_val;
		}
	}
	else
	{
		sts = 1;
	}
	return ( sts );
}
