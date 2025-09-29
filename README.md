# sim-ctl
## Simulation Controller Project

Project Directories for the SimCtl.

All projects within the archive are licensed under GLP v3.0. See LICENSE.

| Directory  | Description  | 
| ------------: | :---------------------------------------|
| audio   | Support for the Auscultation system. This project includes the SimCtl code for management of the Wave Trigger. |
| build   | Destination file for built files.
| cardiac | Manage the parameters for the Cardiac support and actions taken on Pulse Syncs. |
| comm    | Common files including communications with the Sim Manager. |
| cpr     | Chest compression sensor support including both Accelerometer and Time-of-Flight. |
| initialization | Default configuration files and service files. |
| pulse   | Support for touch sensors on Pulse units. |
| respiration | Support for manual breath sensor. |
| test    | Test code used in development.  |
| wav-trig | Sound Sense and Wave Trigger/Tsunami communications. Controls and synchronizes pulse, auscultation sounds and chest rise. |
| www     | Embedded Web Site for Disgnostics. |

## Release Notes:

### Version 1.1.9:
	Added scan of both WVS and Linux SimManagers, to simplify upgrade and to allow use of both host types without reconfiguration.
	
### Version 1.1.8:
	Added Fast Scan support to detect WinVetSim faster

### Version 1.1.7:
	Remove Tank support from Air Controls. Add support for ICStation RFID Reader.
 	Additional chages for Debian 11
 
### Version 1.1.6:
	Changes to prepare for build with Debian 11
	
### Version 1.1.5:
	Limit Limit breath inhalation time to 1.5 seconds, to better sync chest movement to sounds at low RR
	Auto recover from CPR sensor temprarily removed from SimCtl.
	
### Version 1.1.4:
	Fixed missing respiration/breathSense.cpp. Left out from 1.1.3 release

### Version 1.1.3:

	Adjustment in loop delays to reduce CPU usage
	
### Version 1.1.2:
	
	Added support for ToF sensor, VL6180x for compression distance

### Version 1.1.1:

	Reverted Rise disable action to pre 1.1.0
