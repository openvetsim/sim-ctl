/*
 * simData.h
 *
 */

#ifndef SIMDATA_H_
#define SIMDATA_H_

#include <semaphore.h>

#define SHM_NAME	"shmData"
#define SHM_CREATE	1
#define SHM_OPEN	0

#define SIMMGR_VERSION		1
#define STR_SIZE			64
#define COMMENT_SIZE		1024

#define LUB_DELAY (120*1000*1000) // Delay 120ms (in ns)
#define DUB_DELAY (200*1000*1000) // Delay 200ms (in ns)
#define PULSE_DELAY (120*1000*1000) // Delay 120ms (in ns)

struct cardiac
{
	char rhythm[STR_SIZE];
	char vpc[STR_SIZE];
	int vpc_freq;		// 0-100% - Frequencey of VPC insertions (when vpc is not set to "none")
	char vfib_amplitude[STR_SIZE];	// low, med, high
	int pea;			// Pulseless Electrical Activity
	int rate;			// Heart Rate in Beats per Minute
	char pwave[STR_SIZE];
	int pr_interval;	// PR interval in msec
	int qrs_interval;		// QRS in msec
	int bps_sys;
	int bps_dia;
	int nibp_rate;
	int nibp_read;
	int nibp_freq;
	int right_dorsal_pulse_strength; 	// 0 - None, 1 - Weak, 2 - Normal, 3 - Strong
	int right_femoral_pulse_strength;
	int left_dorsal_pulse_strength;
	int left_femoral_pulse_strength;
	
	char heart_sound[STR_SIZE];
	int heart_sound_volume;
	int heart_sound_mute;
	
};

struct respiration
{
	// Sounds for Inhalation, Exhalation and Background
	char left_lung_sound[STR_SIZE];		// Base Sound 
	int left_lung_sound_volume;
	int left_lung_sound_mute;
	
	char right_lung_sound[STR_SIZE];		// Base Sound
	int right_lung_sound_volume;
	int right_lung_sound_mute;
	
	//char left_lung_sound[STR_SIZE];
	//char right_lung_sound[STR_SIZE];
	
	// Duration of in/ex
	int inhalation_duration;	// in msec
	int exhalation_duration;	// in msec

	// Current respiration rate
	int awRR;	// Computed rate
	int rate;	// defined rate
	
	int chest_movement;
	int manual_breath;
};

struct auscultation
{
	int side;	// 0 - None, 1 - Left, 2 - Right
	int row;	// Row 0 is closest to spine
	int col;	// Col 0 is closets to head
	int heartStrength;
	int leftLungStrength;
	int rightLungStrength;
	char tag[STR_SIZE];
};

#define PULSE_NOT_ACTIVE					0
#define PULSE_RIGHT_DORSAL					1
#define PULSE_RIGHT_FEMORAL					2
#define PULSE_LEFT_DORSAL					3
#define PULSE_LEFT_FEMORAL					4
#define PULSE_POINTS_MAX					5	// Actually, one more than max, as 0 is not used

#define PULSE_TOUCH_NONE					0
#define PULSE_TOUCH_LIGHT					1
#define PULSE_TOUCH_NORMAL					2
#define PULSE_TOUCH_HEAVY					3
#define PULSE_TOUCH_EXCESSIVE				4

struct pulse
{
	int right_dorsal;	// Touch Pressure
	int left_dorsal;	// Touch Pressure
	int right_femoral;	// Touch Pressure
	int left_femoral;	// Touch Pressure
	
	int ain[PULSE_POINTS_MAX];
	int touch[PULSE_POINTS_MAX];
	int base[PULSE_POINTS_MAX];
	int volume[PULSE_POINTS_MAX];
};
struct cpr
{
	int last;			// msec time of last compression
	int	compression;	// 0 to 100%
	int release;		// 0 to 100%
	int duration;
};

struct defibrillation
{
	int last;			// msec time of last shock
	int energy;			// Energy in Joules of last shock
};

struct shmData 
{
	sem_t	i2c_sema;	// Mutex lock - Lock for I2C bus access
	char simMgrIPAddr[32];
	
	// This data is from the sim-mgr, it controls our outputs
	struct cardiac cardiac;
	struct respiration respiration;
	
	// This data is internal to the sim-ctl and is sent to the sim-mgr
	struct auscultation auscultation;
	struct pulse pulse;
	struct cpr cpr;
	struct defibrillation defibrillation;
	int manual_breath_ain;
	int manual_breath_baseline;
};

int cardiac_parse(const char *elem,  const char *value, struct cardiac *card );
int respiration_parse(const char *elem,  const char *value, struct respiration *resp );

#endif /* SIMDATA_H_ */