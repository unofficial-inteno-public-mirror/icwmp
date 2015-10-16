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

struct ip_ping_diagnostic ipping_diagnostic = {0};

int init_ipping_diagnostic()
{	
	ipping_diagnostic.state = strdup("None");
    ipping_diagnostic.interface = NULL;
    ipping_diagnostic.host = NULL;
    ipping_diagnostic.repetition = strdup("5");
    ipping_diagnostic.timeout = strdup("5");
    ipping_diagnostic.size = strdup("56");
    //char *dscp
    ipping_diagnostic.success_count = strdup("0");
    ipping_diagnostic.failure_count = strdup("0");
    ipping_diagnostic.average_response_time = strdup("0");
    ipping_diagnostic.minimum_response_time = strdup("0");
    ipping_diagnostic.maximum_response_time = strdup("0");
    return 0;
}

int exit_ipping_diagnostic()
{	
	free(ipping_diagnostic.state);
    free(ipping_diagnostic.interface);
    free(ipping_diagnostic.host);
    free(ipping_diagnostic.repetition);
    free(ipping_diagnostic.timeout);
    free(ipping_diagnostic.size);
    //char *dscp
    free(ipping_diagnostic.success_count);
    free(ipping_diagnostic.failure_count);
    free(ipping_diagnostic.average_response_time);
    free(ipping_diagnostic.minimum_response_time);
    free(ipping_diagnostic.maximum_response_time);
    return 0;
}

char *set_ping_diagnostic(char *param, char *value)
{	
	free(param);
	return strdup(value); //MUST USE STRDUP 	
}

int extract_ping_statistics(char *str)
{
	char *val = NULL, *failure, *tmp, *pch = NULL, *pch2, *spch, *spch3, *spch2, *stat, *total;
	char delimiter[] = " ";
	int i =0;
	int success;
	val = "packets transmitted";
	pch = strstr(str, val);		
	if (pch) {			
		val = "round-trip";			
		if (i == 0) {
			tmp = strdup(str);			
			pch2 = strtok_r(tmp, ",", &spch2);
			stat = strtok_r(spch2, delimiter, &spch3);
			ipping_diagnostic.success_count = set_ping_diagnostic(ipping_diagnostic.success_count, stat);
			success = atoi(stat);
			total = strtok_r(pch2, delimiter, &spch3);
			asprintf(&failure, "%d", atoi(total) - success);
			ipping_diagnostic.failure_count = set_ping_diagnostic(ipping_diagnostic.failure_count, failure);
			free(failure);
			free(tmp);
			if (success == 0) {
				ipping_diagnostic.state = set_ping_diagnostic(ipping_diagnostic.state, "Error_CannotResolveHostName");
				ipping_diagnostic.minimum_response_time = set_ping_diagnostic(ipping_diagnostic.minimum_response_time, "0");
				ipping_diagnostic.average_response_time = set_ping_diagnostic(ipping_diagnostic.average_response_time, "0");
				ipping_diagnostic.maximum_response_time = set_ping_diagnostic(ipping_diagnostic.maximum_response_time, "0");
				return 0;
			}
			i++;
			pch = strstr(str, val);
		}
		if (pch && i != 0) {
			tmp = strdup(pch);
			pch2 = strtok_r(tmp, "=", &spch2);				
			ipping_diagnostic.state = set_ping_diagnostic(ipping_diagnostic.state, "Complete");
			spch2++;			
			ipping_diagnostic.minimum_response_time = set_ping_diagnostic(ipping_diagnostic.minimum_response_time, strtok_r(spch2, "/", &spch3));
			spch2 = strtok_r(NULL, "/ ", &spch3);
			ipping_diagnostic.average_response_time = set_ping_diagnostic(ipping_diagnostic.average_response_time, spch2);
			spch2 = strtok_r(NULL, "/ ", &spch3);
			ipping_diagnostic.maximum_response_time = set_ping_diagnostic(ipping_diagnostic.maximum_response_time, spch2);
			free(tmp);			
		}
	}		
	return 0;
}

int ping_cmd()
{
	int i = 8;
	char buf[512], *val = NULL, *pch, *spch = NULL, *pch2, *spch2, *dup = NULL;
	int dsl, pp, r;
	buf[0] = '\0';
	
	if ((ipping_diagnostic.host)[0] == '\0') {		
		return 0;
	}
	if (ipping_diagnostic.interface == NULL || (ipping_diagnostic.interface)[0] == '\0')
		pp = dmcmd("ping", i, "-q", "-c", ipping_diagnostic.repetition, "-s", ipping_diagnostic.size, "-w", ipping_diagnostic.timeout, ipping_diagnostic.host);
	else {
		i++;
		pp = dmcmd("ping", i, "-q", "-c", ipping_diagnostic.repetition, "-s", ipping_diagnostic.size, "-w", ipping_diagnostic.timeout, "-I", ipping_diagnostic.interface, ipping_diagnostic.host);
	}	
	if (pp) {
		r = dmcmd_read(pp, buf, 512);
		close(pp);
	}
	if(buf[0] == '\0' || strstr(buf, "bad address")) {
		ipping_diagnostic.state = set_ping_diagnostic(ipping_diagnostic.state, "Complete"); //TO CHECK
		ipping_diagnostic.success_count = set_ping_diagnostic(ipping_diagnostic.success_count, "0");
		ipping_diagnostic.failure_count = set_ping_diagnostic(ipping_diagnostic.failure_count, "0");
		ipping_diagnostic.minimum_response_time = set_ping_diagnostic(ipping_diagnostic.minimum_response_time, "0");
		ipping_diagnostic.average_response_time = set_ping_diagnostic(ipping_diagnostic.average_response_time, "0");
		ipping_diagnostic.maximum_response_time = set_ping_diagnostic(ipping_diagnostic.maximum_response_time, "0");
		return 0;
	}
	for (pch = strtok_r(buf, "\n\r", &spch); pch; pch = strtok_r(NULL, "\n\r", &spch)) {					
		if(strstr(pch, "ping statistics"))		
			break;
	}	
	extract_ping_statistics(spch);
	return 0;
}

int cwmp_ip_ping_diagnostic() 
{
	ping_cmd();
	cwmp_root_cause_event_ipdiagnostic(&cwmp_main);
    return 0;	
}