/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *
 */

#ifndef _LOG_H_
#define _LOG_H_

enum log_severity_enum {
	EMERG,
	ALERT,
	CRITIC,
	ERROR,
	WARNING,
	NOTICE,
	INFO,
	DEBUG
};

#define DEFAULT_LOG_FILE_SIZE	10240
#define DEFAULT_LOG_FILE_NAME	"/var/log/cwmpd.log"
#define DEFAULT_LOG_SEVERITY	INFO

#ifdef WITH_CWMP_DEBUG
# ifndef CWMP_LOG
#  define CWMP_LOG(SEV,MESSAGE,args...) puts_log(SEV,MESSAGE,##args);
# endif
#else
# define CWMP_LOG(SEV,MESSAGE,args...)
#endif

#ifdef WITH_DEV_DEBUG
# ifndef DD
#  define DD(SEV,MESSAGE,args...) puts_log(SEV,MESSAGE,##args);
# endif
#else
# define DD(SEV,MESSAGE,args...)
#endif

#define DETECT_CRASH(MESSAGE,args...) { \
  const char *A[] = {MESSAGE}; \
  printf("step: %s %s %d\n",__FUNCTION__,__FILE__,__LINE__); fflush(stdout); sleep(1);\
  if(sizeof(A) > 0) \
    printf(*A,##args); \
}

#define TRACE(MESSAGE,args...) { \
  const char *A[] = {MESSAGE}; \
  printf("step: %s %s %d\n",__FUNCTION__,__FILE__,__LINE__); fflush(stdout);\
  if(sizeof(A) > 0) \
    printf(*A,##args); \
}

#endif /* _LOG_H_ */
