/*
 * simUtil.c
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ctype.h>

#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <termios.h>
#include <syslog.h>
#include <signal.h>
#include <execinfo.h>
#include <string.h>
#include <libgen.h>

#include "simUtil.h"
#include "shmData.h"

extern int debug;
int findAINPath(void );

/*
 * Function: daemonize
 *
 * Convert the task to a daemon. Make it independent of the parent to allow
 * running forever.
 *
 *   Change working directory
 *   Take Lock
 *
 * Parameters: none
 *
 * Returns: none
 */

extern char *program_invocation_name;
extern char *program_invocation_short_name;

#define RUNNING_DIR			"/"
#define LOCK_FILE_DIR       "/var/run/"

void daemonize(void )
{
    pid_t pid;
	int i,lfp;
	char str[10];
	char buffer[128];
	
	if ( getppid()==1 )
	{
		return; /* already a daemon */
	}
    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
	{
		perror("fork: pid < 0:" );
        exit(EXIT_FAILURE);
	}
    /* Success: Let the parent terminate */
    if (pid != 0)
	{
		// perror("fork: pid > 0:" );
        exit(EXIT_SUCCESS);
	}
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
    {
		perror("setsid" );
		exit(EXIT_FAILURE);
	}
	
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	i = open("/dev/null",O_RDWR);
	dup2(i, STDOUT_FILENO );
	dup2(i, STDERR_FILENO ); /* handle standard I/O */
	
	umask(027); /* set newly created file permissions */
	chdir(RUNNING_DIR); /* change running directory */
	
	// Create a Lock File to prevent multiple daemons
	sprintf(buffer, "%s%s.pid", LOCK_FILE_DIR, program_invocation_short_name );
	lfp = open(buffer, O_RDWR|O_CREAT, 0640 );
	if ( lfp < 0 )
	{
		syslog(LOG_DAEMON | LOG_ERR, "Cannot open lock file");
		exit(1); // 
	}
	/*
	if ( lockf(lfp,F_TLOCK,0) < 0 )
	{
		syslog(LOG_DAEMON | LOG_WARNING, "Cannot take lock");
		exit(0); // can not lock
	}
	*/
	
	/* first instance continues */
	sprintf(str, "%d\n", getpid());
	write(lfp, str, strlen(str)); /* record pid to lockfile */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,signal_handler); /* catch hangup signal */
	signal(SIGTERM,signal_handler); /* catch kill signal */
	
	sprintf(buffer, "%s started", program_invocation_short_name );
	syslog (LOG_DAEMON | LOG_NOTICE, buffer );
}


/*
 * Function: log_message
 *
 * Log a message to syslog. The message is also logged to a named file. The file is opened
 * for Append and closed on completion of the write. (The file write will be disabled after
 * development is completed).
 * 
 *
 * Parameters: filename - filename to open for writing
 *             message - Pointer to message string, NULL terminated
 *
 * Returns: none
 */
void log_message(const char *filename, const char* message)
{
#if LOG_TO_FILE == 1
	FILE *logfile;
#endif
	if ( debug )
	{
		printf("%s\n", message );
	}
	else
	{
		syslog(LOG_NOTICE, message);

#if LOG_TO_FILE == 1
		logfile = fopen(filename,"a");
		if ( !logfile )
		{
			return;
		}
		fprintf(logfile, "%s\n", message);
		fclose(logfile );
#endif
	}
}

/*
 * Function: signal_handler
 *
 * Handle inbound signals.
 *		SIGHUP is ignored 
 *		SIGTERM closes the process
 *
 * Parameters: sig - the signal
 *
 * Returns: none
 */
void signal_handler(int sig )
{
	switch(sig) {
	case SIGHUP:
		log_message("","hangup signal catched");
		break;
	case SIGTERM:
		log_message("","terminate signal catched");
		exit(0);
		break;
	}
}
/*
 * Function: signal_fault_handler
 *
 * Handle inbound signals.
 *		SIGHUP is ignored 
 *		SIGTERM closes the process
 *
 * Parameters: sig - the signal
 *
 * Returns: none
 */
void signal_fault_handler(int sig)
{
	int j, nptrs;
#define SIZE 100
	void *buffer[100];
	char **strings;

	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);

	// The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	// would produce similar output to the following:
	//syslog(LOG_NOTICE, "%s", "fault");
	
	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) 
	{
		//syslog(LOG_NOTICE, "fault: %s", "no symbols");
		printf("fault: %s", "no symbols");
		exit(EXIT_FAILURE);
	}
	
	for ( j = 0; j < nptrs; j++ )
	{
		//syslog(LOG_NOTICE, "%d: %s", j, strings[j]);
		printf("%d: %s\n", j, strings[j]);
	}
	free(strings);
	exit ( -1 );
}
void
catchFaults(void )
{
	signal(SIGSEGV,signal_fault_handler );   // install our fault handler
}

int shmFile;
extern struct shmData *shmData;

int
initSHM(int create )
{
	void *space;
	int mmapSize;
	int pageSize;
	int allocSize;
	int mode;
	int perm;
	
	mmapSize = sizeof(struct shmData );
	// Round up size to integral number of pages
	pageSize = getpagesize();
	allocSize = (mmapSize+pageSize-1) & ~(pageSize-1);

	// Open the Shared Memory space
	if ( create )
	{
		mode = O_CREAT | O_RDWR;
		perm = S_IRUSR | S_IWUSR;
	}
	else
	{
		mode = O_RDWR;
		perm = 0;
	}
	
	shmFile = shm_open(SHM_NAME, mode, perm );
	if ( shmFile < 0 )
	{
		fprintf(stderr, "Failed to open %s. Rval is %d\n", SHM_NAME, shmFile );
		perror("shm_open" );
		return ( -3 );
	}
	if ( create )
	{
		// Set file size
		if ( ftruncate(shmFile, allocSize) == -1)
		{
			perror("ftruncate" );
			
			return ( -3 );
		}
	}
	space = mmap((caddr_t)0,
				mmapSize, 
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				shmFile,
				0 );
	if ( space == MAP_FAILED )
	{
		perror("mmap" );
		return ( -4 );
	}
	
	shmData = (struct shmData *)space;
	
	return ( 0 );
}

#define PATH_MAX	512
char ain_path[PATH_MAX];
int ain_path_found = 0;
int ain_new_names = 0;

int findAINPath(void )
{
	FILE *fp;
	struct stat sb;
	int sts;
	
	fp = popen("find /sys/devices -name AIN0", "r" );
	
	while ( fgets(ain_path, PATH_MAX, fp) != NULL)
	{
		sprintf(ain_path, "%s", dirname(ain_path ) );	// Return the directory
		if ( debug > 1 )
		{
			printf("%s\n", ain_path);
		}
	}
	pclose(fp );
	
	if ( strlen(ain_path ) > 0 )
	{
		ain_path_found = 1;
		if ( debug )
		{
			printf("AIN Path is %s\n", ain_path );
		}
	}
	else
	{
		// Check for the 'newer' path to the  in_voltageN_raw devices
		sts = stat("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", &sb );
		if ( sts == 0 )
		{
			sprintf(ain_path, "%s", "/sys/bus/iio/devices/iio:device0" );	// Return the directory
			if ( debug > 1 )
			{
				printf("%s\n", ain_path);
			}
		}
		if ( strlen(ain_path ) > 0 )
		{
			ain_path_found = 1;
			ain_new_names = 1;
			if ( debug )
			{
				printf("AIN Path is %s\n", ain_path );
			}
		}
		else
		{
			printf("No AIN Path Found\n" );
		}
	}
	return ( 0 );
}
	
int
read_ain(int chan )
{
	int fd;
	int val = 0;
	char name[256];
	int sts;
	char buf[8];
	
	if ( ain_path_found == 0 )
	{
		findAINPath();
	}
	if ( ain_path_found == 1 )
	{
		if ( ain_new_names )
		{
			sprintf(name, "%s/in_voltage%d_raw", ain_path, chan );
		}
		else
		{
			sprintf(name, "%s/AIN%d", ain_path, chan);
		}
		fd = open (name, O_RDONLY );
		if ( fd < 0 )
		{
			if ( debug )
			{
				fprintf(stderr, "Failed to open %s", name );
			}
			perror("open" );
			exit ( -2 );
		}
		
		sts = read(fd, buf, 4 );
		if ( sts == 4 )
		{
			val = atoi(buf );
		}
		close(fd);
	}

	return ( val );
}

/**
 * cleanString
 *
 * remove leading and trailing spaces. Reduce internal spaces to single. Remove tabs, newlines, CRs.
*/
#define WHERE_START		0
#define WHERE_TEXT		1
#define WHERE_SPACE		2

void
cleanString(char *strIn )
{
	char *in;
	char *out;
	int where = WHERE_START;
	
	in = (char *)strIn;
	out = (char *)strIn;
	while ( *in )
	{
		if ( isspace( *in ) )
		{
			switch ( where )
			{
				case WHERE_START:
				case WHERE_SPACE:
					break;
				
				case WHERE_TEXT:
					*out = ' ';
					out++;
					where = WHERE_SPACE;
					break;
			}
		}
		else
		{
			where = WHERE_TEXT;
			*out = *in;
			out++;
		}
		in++;
	}
	*out = 0;
}

#define NO_I2C_LOCK	1
int
getI2CLock(void )
{
#ifndef NO_I2C_LOCK
	int trycount = 0;
	int sts;

	while ( ( sts = sem_trywait(&shmData->i2c_sema) ) != 0 )
	{
		if ( trycount++ > 200 )
		{
			// Could not get lock soon enough. Try again next time.
			log_message("", "Failed to take i2c_sema");
			return ( -1 );
		}
		usleep(10000 );
	}
#endif
	return ( 0 );
}

void
releaseI2CLock(void )
{
	
#ifndef NO_I2C_LOCK
	sem_post(&shmData->i2c_sema);
#endif
}

/**
 * gpioPinOpen
 *
 * Open a GPIO Pin and set its direction. Return a FILE * for access to the value.
*/

FILE *
gpioPinOpen(int pin, int direction )
{
	char name[512];
	struct stat sb;
	FILE *io, *iodir, *ioval;
	int sts;
	
	printf("gpioPinOpen(%d, %d)\n", pin, direction );
	sprintf(name, "/sys/class/gpio/gpio%d", pin );
	sts = stat(name, &sb );
	if ( sts == 0 )
	{
		//printf("gpio%d exists\n", pin );
	}
	else
	{
		io = fopen("/sys/class/gpio/export", "w");
		fseek(io, 0, SEEK_SET);
		fprintf(io, "%d", pin);
		fflush(io);
		fclose(io);
	}
	
	sprintf(name, "/sys/class/gpio/gpio%d/direction", pin );
	iodir = fopen(name, "w");
    fseek(iodir ,0, SEEK_SET);
	if ( direction == GPIO_OUTPUT )
	{
		fprintf(iodir, "out");
	}
	else
	{
		fprintf(iodir, "in");
	}
    fflush(iodir);
	fclose(iodir);
	
	sprintf(name, "/sys/class/gpio/gpio%d/value", pin );
	if ( direction == GPIO_OUTPUT )
	{
		ioval = fopen(name, "w");
	}
	else
	{
		ioval = fopen(name, "r");
	}
    fseek(ioval,0,SEEK_SET);
	
	return ( ioval );
}

/**
 * gpioPinSet
 *
 * Set the GPIO Pin HI or LO
*/
void
gpioPinSet(FILE *ioval, int val )
{
	if ( val != 0 )
	{
		val = 1;
	}
	
	fprintf(ioval, "%d", val);
    fflush(ioval);
}

/**
 * gpioPinRead
 *
 * Read the state of a GPIO Input Pin.
*/
int 
gpioPinRead(int pin, int *value)
{
	FILE *ioval;
	char name[512];
	char ch;
	int sts;
	
	snprintf(name, sizeof(name), "/sys/class/gpio/gpio%d/value", pin);

	ioval = fopen(name, "r");
	if (ioval < 0) {
		perror("gpio/value");
		return ( -1 );
	}
	sts = fread(&ch, 1, 1, ioval );
	fclose(ioval );
	if ( sts == 1 )
	{
		if (ch != '0')
		{
			*value = 1;
		}
		else
		{
			*value = 0;
		}
		return 0;
	}
	else
	{
		perror("fread" );
		printf("%p, %p\n", ioval,value );
	}
	return ( -1 );
}

/**
 * gpioPinGet
 *
 * Read the state of a GPIO Input Pin.
*/
int 
gpioPinGet(FILE *ioval, int *value)
{
	//int fd;
	//char buf[MAX_BUF];
	char ch;
	int sts;
	
	/*
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}

	close(fd);
	*/
	sts = fread(&ch, 1, 1, ioval );
	if ( sts == 1 )
	{
		if (ch != '0')
		{
			*value = 1;
		}
		else
		{
			*value = 0;
		}
		return 0;
	}
	return ( -1 );
}

// Implementation of itoa() 
// Base 10 only, so a little more efficient
char itoaNumbers[12] = "0123456789";
char itoaString[34] = { 0, };

char* itoa(int num ) 
{
	char *t = &itoaString[34];
	int isNeg;
	int digit;
	
	if ( num >= 0 )
	{
		isNeg = false;
	}
	else
	{
		isNeg = true;
		num = -num;
	}
	do 
	{
		digit = num % 10;
		*t-- = itoaNumbers[digit];
	} while ( num /= 10, num > 0 );
	
	if ( isNeg )
	{
		*t='-';
	}
	else
	{
		++t;
	}
	return t;
}