/*
 * simCtlComm.cpp
 *
 * This file is part of the sim-ctl distribution (https://github.com/OpenVetSimDevelopers/sim-ctl).
 * 
 * Copyright (c) 2019-2024 VetSim, Cornell University College of Veterinary Medicine Ithaca, NY
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

/*
 * This class will find the simmgr and establish a communications link.
 * 
 * The specific host name may be provided by creating a file, /simulator/simmgr that contains the host name or IP address.
 * If the file does not exist or is empty, the local subnet will be scanned to find a simmgr.
*/
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/inet.h>
   
#include <ifaddrs.h>
#include <net/if.h>

#include "simCtlComm.h"
#include "simUtil.h"
#include "version.h"

#define BUF_MAX	4096

using namespace std;
extern int debug;

extern char msgbuf[];


/* Constructor:
 * 		
 */
simCtlComm::simCtlComm()
{
	char name[SIM_NAME_SIZE];
	FILE *fd;
	char *ptr;
	
	commFD = -1;
	commPort = WVS_SYNC_PORT;
	simMgrName[0] = 0;
	simMgrIPAddr[0] = 0;
	simMgrStatusPort = 80;	// Default is standard HTML port. This can be overridden from the SimManager
	barkState = FALSE;	// TRUE when first connection is good.
	connectState = FALSE;	// TRUE when live connection is good.
	
	// Allowed formats for simmgrName file:
	// Hostname or IP address
	//		192.168.1.22
	//      simmgr
	// Hostname or ip with port
	//		192.168.1.22:50200
	//		winvetsim:40844
	// Port only
	//		:50200
	//		:40844

	// 9/22/2025 Note: If the Port Only option is selected and the port specified is either 50200 or 40844, then 
	// we will scan both ports.
	
	fd = fopen("/simulator/simmgrName", "r" );
	if ( fd != NULL )
	{
		memset(name, 0, SIM_NAME_SIZE );
		while ( ( ptr = fgets(name, SIM_NAME_SIZE, fd ) ) != NULL )
		{
			// Strip any leading spaces
			cleanString(ptr );
			if ( strlen(ptr) > 4 && *ptr != '#' )	// Skip comments
			{
				ptr = strchr(name, ':' );
				if ( ptr )
				{
					// Has name and port
					*ptr = 0;
					if ( strlen(name) > 0 )
					{
						sprintf(simMgrName, "%s", name );
					}
					ptr += 1;
					commPort = atoi(ptr );
					if ( commPort != LINUX_SYNC_PORT && commPort != WVS_SYNC_PORT )
					{
						scanBothPorts = false;
					}
				}
				else
				{
					strncpy(simMgrName, name, SIM_NAME_SIZE-2 );
					simMgrName[strcspn(simMgrName, "\n\r")] = 0;
				}
			}
		}
		fclose(fd );
	}
	sprintf(msgbuf, "simCtlComm host \"%s\", port %d\n", simMgrName, commPort );
	log_message("", msgbuf);
}

/* openSimMgr:
 *
 * Scans the local subnet to find the simMgr
 * Opens the SimMgr listen port
*/
int
simCtlComm::openListen(int active )
{
    int fd;
	int sts;

	char hostAddr[32];
	struct IPv4 myIP;
	int i, j;
	struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
	struct sockaddr_in *s4;

	commFD = -1;
	
	// Find our local IPV4 subnet address
	sts = -1;
	while ( sts )
	{
		if ( getifaddrs(&myaddrs) != 0 )
		{
			if ( debug )
			{
				perror("getifaddrs");
				sprintf(msgbuf, "comm.openListen getifaddrs() fails: %s\n", strerror(errno ) );
				log_message("", msgbuf);
			}
		}
		else
		{
			for ( ifa = myaddrs ; ifa != NULL ; ifa = ifa->ifa_next )
			{
				if ( ( ifa->ifa_addr != NULL ) &&
					 ( ifa->ifa_flags & IFF_UP ) &&
					 ( ifa->ifa_addr->sa_family ==  AF_INET ) && 
					 ( strncmp(ifa->ifa_name, "eth0", 4 ) == 0 ) )
				{
					s4 = (struct sockaddr_in *)ifa->ifa_addr;
					in_addr = &s4->sin_addr;
					if ( !inet_ntop(ifa->ifa_addr->sa_family, in_addr, hostAddr, sizeof(hostAddr) ) )
					{
						if ( debug )
						{
							sprintf(msgbuf, "comm.openListen inet_ntop: %s", strerror(errno ) );
							log_message("", msgbuf);
						}
					}
					else
					{
						myIP.b1 = (s4->sin_addr.s_addr & 0xff);
						myIP.b2 = (s4->sin_addr.s_addr & 0xff00) >> 8;
						myIP.b3 = (s4->sin_addr.s_addr & 0xff0000) >> 16;
						myIP.b4 = (s4->sin_addr.s_addr & 0xff000000) >> 24;
						
						sts = 0;
						break;
					}
				}
			}
			freeifaddrs(myaddrs);
		}
		if ( sts )
		{
			// Local IP Address not found
			if ( debug )
			{
				sprintf(msgbuf, "%s", "comm.openListen Local IP Address not found" );
				log_message("", msgbuf);
				sleep(2);	// Delay 2 seconds before next try
			}
		}
	}
	if ( strlen(simMgrName) == 0 )
	{
		sts = -1;
		while ( sts )
		{
			for ( j = 0 ; j < 2 && connectState == false ; j++ )
			{
				if ( scanBothPorts )
				{
					if ( j == 0 )
					{
						commPort = WVS_SYNC_PORT;
					}
					else
					{
						commPort = LINUX_SYNC_PORT;
					}
				}
				// Scan the local subnet looking for the server
				for ( i = 1 ; i < 255 ; i++ )
				{
					if ( i == myIP.b4 ) // Don't scan our own address
					{
						continue;
					}
					sprintf(hostAddr, "%d.%d.%d.%d", myIP.b1, myIP.b2, myIP.b3, i );
					if ( debug )
					{
						printf("Trying \"%s\"\n", hostAddr );
					}
					fd = this->trySimMgrOpen(hostAddr );
					if ( fd <= 0 )
					{
					}
					else
					{
						commFD = fd;
						sts = 0;
						break;
					}

				}
				if ( sts )
				{
					// Not found - Wait and then try again
					usleep(10000 );
				}
			}
		}
	}
	else
	{
		sts = -1;
		while ( sts )
		{
			if ( debug )
			{
				sprintf(msgbuf, "Trying \"%s\"", simMgrName );
				log_message("", msgbuf);
			}
			fd = this->trySimMgrOpen(simMgrName );
			if ( fd <= 0 )
			{
				//sprintf(msgbuf, "trySimMgrOpen(%s) returns %d\n", simMgrName, fd );
				//log_message("", msgbuf);
			}
			else
			{
				commFD = fd;
				sts = 0;
			}
			if ( sts )
			{
				// Not found - Wait and then try again
				sleep(10 );
			}
		}
	}
	if ( commFD < 0 )
	{
		// Since we added the infinite retry loop, this should never occur.
		sprintf(msgbuf, "comm.openListen simMgr not found (%d)", fd );
		log_message("", msgbuf);
		return ( -1 );
	}
	else
	{		
		if ( active != LISTEN_ACTIVE )
		{
			close(commFD );
			commFD = -1;
			sprintf(msgbuf, "comm.openListen simMgr not found (%d)", fd );
			log_message("", msgbuf);
			return ( -1 );
		}
	}
	return ( 0 );
}

#define SM_BUF_MAX	32

int
simCtlComm::wait(void )
{
	int len;
	char buffer[SM_BUF_MAX];
	int syncState = SYNC_NONE;
	while ( 1 )
	{
		memset(buffer, 0, SM_BUF_MAX );
		len = read(commFD, buffer, SM_BUF_MAX-1 );
		if ( len > 0 )
		{
			if ( debug > 1 )
			{
				printf("%s", buffer );
			}
			syncState = SYNC_NONE;
			if ( strstr(buffer, "pulse" ) )
			{
				syncState |= SYNC_PULSE;
			}
			if ( strstr(buffer, "pulseVPC" ) )
			{
				syncState |= SYNC_PULSE_VPC;
			}
			if (  strstr(buffer, "breath" )  )
			{
				syncState |= SYNC_BREATH;
			}
			if (  strstr(buffer, "statusPort" )  )
			{
				char *ptr;
				
				ptr = strchr(buffer, ':' );
				if ( ptr )
				{
					ptr++;
					this->simMgrStatusPort = atoi(ptr );
					syncState |= SYNC_STATUS_PORT;
				}
			}
			if (  strstr(buffer, "version" )  )
			{
				// Write back the version
				sprintf(buffer, "%s", SIMCTL_VERSION );
				len = write(commFD, buffer, strlen(buffer) );
			}
			if ( syncState != SYNC_NONE )
			{
				return ( syncState );
			}
		}
		else
		{
			// Check if closed and try to reopen
			buffer[0] = 'P';
			
			len = write(commFD, buffer, 1 );
			if ( ( len < 0 )  ||  ( errno == EPIPE ) )
			{
				// Leave barkState as TRUE, to prevent additional barks.
				connectState = FALSE;
				
				//close(commFD );
				
				sprintf(msgbuf, "Closed - Reopen Pipe" );
				log_message("", msgbuf);
				int sts = this->reopen();
				if ( sts )
				{
					sprintf(msgbuf, "Reopen Failed - Rescan" );
					log_message("", msgbuf);
				
					sts = this->openListen(1);
					sprintf(msgbuf, "openListen returns %d", sts );
					log_message("", msgbuf);
				}
			}
		}
		usleep(1000);
	}
}
int
simCtlComm::reopen(void)
{
	int fd = commFD;
	struct sockaddr_in addr;
	long arg;
	int res;
	struct timeval tv;
	fd_set myset; 
	socklen_t lon;
	int valopt;
	int ldebug = debug;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(commPort );	
	addr.sin_addr.s_addr = inet_addr(currentHostAddr); 
		
// Trying to connect with timeout 
	res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	if ( res < 0 )
	{
		if (errno == EINPROGRESS)
		{ 
			//sprintf(msgbuf, (stderr, "EINPROGRESS in connect() - selecting\n"); 
			//log_message("", msgbuf);
			
			tv.tv_sec = 0; 
			tv.tv_usec = 10000; 
			FD_ZERO(&myset); 
			FD_SET(fd, &myset); 
			res = select(fd+1, NULL, &myset, NULL, &tv); 
			if (res < 0 && errno != EINTR)
			{ 
				//printf(msgbuf, "comm.openListen select(): %s", strerror(errno ) );
				//log_message("", msgbuf);
			} 
			else if (res > 0)
			{ 
				// Socket selected for write 
				lon = sizeof(int); 
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
				{ 
					// fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
				} 
				else
				{
					// Check the value returned... 
					if (valopt) 
					{
						// This just means the polled address is not a SimManager. It did not answer on the port.
						// sprintf(msgbuf, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) );
						// log_message("", msgbuf);
					}
					else
					{
						sprintf(msgbuf, "reopened simMgr at %s\n", currentHostAddr );
						log_message("", msgbuf);
						barkState = TRUE;
						int enableKeepAlive = 1;
						setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enableKeepAlive, sizeof(enableKeepAlive));

						return ( fd );
					}
				}
			} 
			else 
			{ 
				if ( ldebug )
				{
					sprintf(msgbuf, "Timeout in select()\n" );
					log_message("", msgbuf);
				}
				// fprintf(stderr, "Timeout in select()\n");
			} 
		} 
		else
		{ 
			fprintf(stderr, "Error Reconnecting %d - %s\n", errno, strerror(errno));  
			sprintf(msgbuf, "comm.openListen Error connecting): %s", strerror(errno ) );
			log_message("", msgbuf);
			return -1;
		}
	}
	else
	{
		if ( ldebug )
		{
			printf("res is %d\n", res );
		}
	}
	// Set to blocking mode again... 
	if( (arg = fcntl(fd, F_GETFL, NULL)) < 0)
	{ 
		fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
		sprintf(msgbuf, "comm.openListen fcntl(F_GETFL) fails: %s", strerror(errno ) );
		log_message("", msgbuf);
	} 
	arg &= (~O_NONBLOCK); 
	if( fcntl(fd, F_SETFL, arg) < 0) 
	{ 
		fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
		sprintf(msgbuf, "comm.openListen fcntl(F_SETFL) fails: %s", strerror(errno ) );
		log_message("", msgbuf);
	}
	return ( 0 );
}
int
simCtlComm::closeListen(void)
{
	if ( commFD )
	{
		close(commFD );
		commFD = 0;
	}
	return ( 0 );
}

simCtlComm::~simCtlComm()
{
	this->closeListen();
}

void 
simCtlComm::show(void )
{
	printf("Port %d, FD %d\n", this->commPort, this->commFD );
}
int
simCtlComm::trySimMgrOpen(char *hostName )
{
	int fd;
	struct sockaddr_in addr;
	long arg;
	int res;
	struct timeval tv;
	fd_set myset; 
	socklen_t lon;
	int valopt;
	struct hostent *he;
    struct in_addr **addr_list;
	char hostAddr[32];
	int a =0;
	int b = 0;
	int c = 0;
	int d = 0;
	int ldebug = debug;
	
	// If an IP address is passed in, use it directly. Otherwise, get the IP address from the hostname.
    res = sscanf(hostName, "%d.%d.%d.%d", &a, &b, &c, &d );
	if ( res == 4 )
	{
		if ( ldebug )
		{
			sprintf(msgbuf, "simCtlComm::trySimMgrOpen: IP Addr \"%s\" (res %d)\n", hostName, res );
			log_message("", msgbuf);
		}
		strncpy(hostAddr, hostName, 32 );
	}
	else
	{
		if ( ldebug )
		{
			sprintf(msgbuf, "simCtlComm::trySimMgrOpen: hostName \"%s\"\n", hostName );
			log_message("", msgbuf);
		}
		if ( (he = gethostbyname( hostName ) ) == NULL) 
		{
			// No IP address found
			if ( ldebug )
			{
				printf("simCtlComm::trySimMgrOpen: No IP found for host \"%s\"\n", hostName );
			}
			sprintf(msgbuf, "%s - res %d (%d.%d.%d.%d)", hostName, res, a,b,c,d );
			log_message("", msgbuf);
			return -4;
		}
		
		addr_list = (struct in_addr **) he->h_addr_list;
		if ( addr_list[0] == NULL )
		{
			// Host found but no IP listed
			return -2;
		}
		strncpy(hostAddr , inet_ntoa(*addr_list[0]), 32 );
		if ( ldebug )
		{
			sprintf(msgbuf, "simCtlComm::trySimMgrOpen: IP found for host \"%s\" is \"%s\"\n", hostName, hostAddr );
			log_message("", msgbuf);
		}
	}
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if ( fd < 0 )
	{
		if ( ldebug )
		{
			perror("socket" );
			sprintf(msgbuf, "comm.openListen socket: %s", strerror(errno ) );
			log_message("", msgbuf);
		}
	}
	else
	{
		addr.sin_family = AF_INET;
		addr.sin_port = htons(commPort );
		
		addr.sin_addr.s_addr = inet_addr(hostAddr); 
		
		// Set non-blocking 
		if( (arg = fcntl(fd, F_GETFL, NULL)) < 0)
		{ 
			if ( ldebug )
			{
				sprintf(msgbuf, "comm.openListen fcntl(F_GETFL): %s", strerror(errno ) );
				log_message("", msgbuf);
				fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
			}
		}
		else
		{
			arg |= O_NONBLOCK; 
			if( fcntl(fd, F_SETFL, arg) < 0)
			{ 
				if ( ldebug )
				{
					sprintf(msgbuf, "comm.openListen fcntl(F_SETFL): %s", strerror(errno ) );
					log_message("", msgbuf);
					fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
				}
			} 
			else
			{
				// Trying to connect with timeout 
				res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
				if ( res < 0 )
				{
					if (errno == EINPROGRESS)
					{ 
						// fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
						
						tv.tv_sec = 0; 
						tv.tv_usec = 10000; 
						FD_ZERO(&myset); 
						FD_SET(fd, &myset); 
						res = select(fd+1, NULL, &myset, NULL, &tv); 
						if (res < 0 && errno != EINTR)
						{ 
							//printf(msgbuf, "comm.openListen select(): %s", strerror(errno ) );
							//log_message("", msgbuf);
						} 
						else if (res > 0)
						{ 
							// Socket selected for write 
							lon = sizeof(int); 
							if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
							{ 
								// fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
							} 
							else
							{
								// Check the value returned... 
								if (valopt) 
								{
									// This just means the polled address is not a SimManager. It did not answer on the port.
									// sprintf(msgbuf, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) );
									// log_message("", msgbuf);
								}
								else
								{
									memcpy(simMgrIPAddr, hostAddr, SIM_IP_ADDR_SIZE );
									memcpy(currentHostAddr, hostAddr, SIM_IP_ADDR_SIZE  );
		
									sprintf(msgbuf, "Found simMgr at %s\n", hostAddr );
									log_message("", msgbuf);
									barkState = TRUE;
									int enableKeepAlive = 1;
									setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&enableKeepAlive, sizeof(enableKeepAlive));

									return ( fd );
								}
							}
						} 
						else 
						{ 
							if ( ldebug )
							{
								sprintf(msgbuf, "Timeout in select()\n" );
								log_message("", msgbuf);
							}
							// fprintf(stderr, "Timeout in select()\n");
						} 
					} 
					else
					{ 
						if ( ldebug )
						{
							fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));  
							sprintf(msgbuf, "comm.openListen Error connecting): %s", strerror(errno ) );
							log_message("", msgbuf);
						}
					}
				}
				else
				{
					if ( ldebug )
					{
						printf("res is %d\n", res );
					}
				}
				// Set to blocking mode again... 
				if( (arg = fcntl(fd, F_GETFL, NULL)) < 0)
				{ 
					fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
					sprintf(msgbuf, "comm.openListen fcntl(F_GETFL) fails: %s", strerror(errno ) );
					log_message("", msgbuf);
				} 
				arg &= (~O_NONBLOCK); 
				if( fcntl(fd, F_SETFL, arg) < 0) 
				{ 
					fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
					sprintf(msgbuf, "comm.openListen fcntl(F_SETFL) fails: %s", strerror(errno ) );
					log_message("", msgbuf);
				}
			}
		}
		close(fd);
	}
	return 0;
}