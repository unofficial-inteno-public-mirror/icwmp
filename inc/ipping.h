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

struct ip_ping_diagnostic {
    char *state;
    char *interface;
    char *host;
    char *repetition;
    char *timeout;
    char *size;
    //char *dscp
    char *success_count;
    char *failure_count;
    char *average_response_time;
    char *minimum_response_time;
    char *maximum_response_time;  
};

int init_ipping_diagnostic();
int exit_ipping_diagnostic();
int cwmp_ip_ping_diagnostic();
char *set_ping_diagnostic(char *param, char *value);

#endif