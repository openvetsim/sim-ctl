/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 * 
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * HTTP PUT upload with authentiction using "any" method. libcurl picks the
 * one the server supports/wants.
 * </DESC>
 */
#include <stdio.h>
#include <fcntl.h>
#ifdef WIN32
#  include <io.h>
#else
#  ifdef __VMS
     typedef int intptr_t;
#  endif
#  if !defined(_AIX) && !defined(__sgi) && !defined(__osf__)
#    include <stdint.h>
#  endif
#  include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include "curl.h"

#ifdef _MSC_VER
#  ifdef _WIN64
     typedef __int64 intptr_t;
#  else
     typedef int intptr_t;
#  endif
#endif

#include <curl/curl.h>

#if LIBCURL_VERSION_NUM < 0x070c03
#error "upgrade your libcurl to no less than 7.12.3"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#if defined(_AIX) || defined(__sgi) || defined(__osf__)
#ifndef intptr_t
#define intptr_t long
#endif
#endif


int
main(int argc, char **argv )
{
	if ( argc != 2 )
	{
		fprintf (stderr, "Invalid args (%d)\n", argc );
		fprintf (stderr, "usage: %s [url]\n", argv[0] );
		return -1;
	}
	return ( curl( argv[1] ) );
}

int curl(char *url )
{
	CURL *curl;
	CURLcode res;

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	/* get a curl handle */
	curl = curl_easy_init();
	if(curl) 
	{
		/* set the ioctl function */
		// curl_easy_setopt(curl, CURLOPT_IOCTLFUNCTION, my_ioctl);
		
	
		/* specify target URL, and note that this URL should also include a file
		   name, not only a directory (as you can do with GTP uploads) */
		curl_easy_setopt(curl, CURLOPT_URL, url);
	
		/* Now run off and do what you've been told! */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		}
		/* always cleanup */
		curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  return 0;
}
