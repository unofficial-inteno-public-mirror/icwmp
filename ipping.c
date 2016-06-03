/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com> *	  
 *
 */

#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include "cwmp.h"
#include "backupSession.h"
#include "xml.h"
#include "log.h"
#include "external.h"	
#include "dmentry.h"
#include "ubus.h"
#include "ipping.h"

int cwmp_ip_ping_diagnostic() 
{
	dmcmd_no_wait("/bin/sh", 2, FUNCTION_PATH, "run");
    return 0;	
}