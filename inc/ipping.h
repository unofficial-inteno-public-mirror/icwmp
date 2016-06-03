/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 */
#ifndef __IPPING__H
#define __IPPING__H

#define FUNCTION_PATH "/usr/share/icwmp/functions/ipping_launch"
#define IPPING_STOP DMCMD("/bin/sh", 2, FUNCTION_PATH, "stop");
int cwmp_ip_ping_diagnostic();
#endif