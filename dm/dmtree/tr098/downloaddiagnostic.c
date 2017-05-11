/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2016 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#include <ctype.h>
#include <uci.h>
#include <stdio.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "downloaddiagnostic.h"


DMLEAF tDownloadDiagnosticsParam[] = {
{"DiagnosticsState", &DMWRITE, DMT_STRING, get_download_diagnostics_state, set_download_diagnostics_state, NULL, NULL},
{"DownloadURL", &DMWRITE, DMT_STRING, get_download_diagnostics_url, set_download_diagnostics_url, NULL, NULL},
{"EthernetPriority", &DMWRITE, DMT_UNINT, get_download_diagnostics_ethernet_priority,set_download_diagnostics_ethernet_priority, NULL, NULL},
{"ROMTime", &DMREAD, DMT_TIME, get_download_diagnostic_romtime, NULL, NULL, NULL},
{"BOMTime", &DMREAD, DMT_TIME, get_download_diagnostic_bomtime, NULL, NULL, NULL},
{"EOMTime", &DMREAD, DMT_TIME, get_download_diagnostic_eomtime, NULL, NULL, NULL},
{"TestBytesReceived", &DMREAD, DMT_UNINT, get_download_diagnostic_testbytes, NULL, NULL, NULL},
{"TotalBytesReceived", &DMREAD, DMT_UNINT, get_download_diagnostic_totalbytes, NULL, NULL, NULL},
{"TCPOpenRequestTime", &DMREAD, DMT_TIME, get_download_diagnostic_tcp_open_request_time, NULL, NULL, NULL},
{"TCPOpenResponseTime", &DMREAD, DMT_TIME, get_download_diagnostic_tcp_open_response_time, NULL, NULL, NULL},
//{"DSCP", &DMWRITE, DMT_UNINT, get_download_diagnostics_dscp, set_download_diagnostics_dscp, NULL, NULL},
//{"Interface", &DMWRITE, DMT_UNINT, get_download_diagnostics_interface, set_download_diagnostics_interface, NULL, NULL},
{0}
};

static inline char *download_diagnostic_get(char *option, char *def)
{
	char *tmp;
	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", option, &tmp);
	if(tmp && tmp[0] == '\0')
		return dmstrdup(def);
	else
		return tmp;
}
int get_download_diagnostics_state(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = download_diagnostic_get("DiagnosticState", "None");
	return 0;
}

int set_download_diagnostics_state(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			if (strcmp(value, "Requested") == 0) {
				DOWNLOAD_DIAGNOSTIC_STOP
				curr_section = dmuci_walk_state_section("cwmp", "downloaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
				if(!curr_section)
				{
					dmuci_add_state_section("cwmp", "downloaddiagnostic", &curr_section, &tmp);
				}
				dmuci_set_varstate_value("cwmp", "@downloaddiagnostic[0]", "DiagnosticState", value);
				cwmp_set_end_session(END_SESSION_DOWNLOAD_DIAGNOSTIC);
			}				
			return 0;
	}
	return 0;
}

int get_download_diagnostics_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "interface", value);
	return 0;
}

int set_download_diagnostics_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			DOWNLOAD_DIAGNOSTIC_STOP
			curr_section = (struct uci_section *)dmuci_walk_state_section("cwmp", "downloaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "downloaddiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@downloaddiagnostic[0]", "interface", value);
			return 0;
	}
	return 0;
}

int get_download_diagnostics_url(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{

	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "url", value);
	return 0;
}

int set_download_diagnostics_url(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			DOWNLOAD_DIAGNOSTIC_STOP
			curr_section = (struct uci_section *)dmuci_walk_state_section("cwmp", "downloaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "downloaddiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@downloaddiagnostic[0]", "url", value);
			return 0;
	}
	return 0;
}

int get_download_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "";
	return 0;
}

int set_download_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:			
			DOWNLOAD_DIAGNOSTIC_STOP
			curr_section = (struct uci_section *)dmuci_walk_state_section("cwmp", "downloaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "downloaddiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@downloaddiagnostic[0]", "ethernetpriority", value);
			return 0;
	}
	return 0;
}

int get_download_diagnostic_romtime(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	
	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "ROMtime", value);
	if ((*value)[0] == '\0')
			*value = "0";
	return 0;
}

int get_download_diagnostic_bomtime(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{

	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "BOMtime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_download_diagnostic_eomtime(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{

	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "EOMtime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_download_diagnostic_testbytes(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{

	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "TestBytesReceived", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_download_diagnostic_totalbytes(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{

	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "TotalBytesReceived", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_download_diagnostic_tcp_open_request_time(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{

	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "TCPOpenRequestTime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_download_diagnostic_tcp_open_response_time(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{

	dmuci_get_varstate_string("cwmp", "@downloaddiagnostic[0]", "TCPOpenResponseTime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}
