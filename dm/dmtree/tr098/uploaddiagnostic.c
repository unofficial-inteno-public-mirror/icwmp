/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#include <ctype.h>
#include <uci.h>
#include <stdio.h>
#include "cwmp.h"
#include "diagnostic.h"
#include "ubus.h"
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "uploaddiagnostic.h"

static inline char *upload_diagnostic_get(char *option, char *def)
{
	char *tmp;
	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", option, &tmp);
	if(tmp && tmp[0] == '\0')
		return dmstrdup(def);
	else
		return tmp;
}
int get_upload_diagnostics_state(char *refparam, struct dmctx *ctx, char **value)
{
	*value = upload_diagnostic_get("DiagnosticState", "None");
	return 0;
}

int set_upload_diagnostics_state(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			if (strcmp(value, "Requested") == 0) {
				UPLOAD_DIAGNOSTIC_STOP
				curr_section = dmuci_walk_state_section("cwmp", "downloaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
				if(!curr_section)
				{
					dmuci_add_state_section("cwmp", "uploaddiagnostic", &curr_section, &tmp);
				}
				dmuci_set_varstate_value("cwmp", "@uploaddiagnostic[0]", "DiagnosticState", value);
				cwmp_set_end_session(END_SESSION_UPLOAD_DIAGNOSTIC);
			}				
			return 0;
	}
	return 0;
}

int get_upload_diagnostics_interface(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "interface", value);
	return 0;
}

int set_upload_diagnostics_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			UPLOAD_DIAGNOSTIC_STOP
			curr_section = dmuci_walk_state_section("cwmp", "uploaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "uploaddiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@uploaddiagnostic[0]", "interface", value);
			return 0;
	}
	return 0;
}

int get_upload_diagnostics_url(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "url", value);
	return 0;
}

int set_upload_diagnostics_url(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			UPLOAD_DIAGNOSTIC_STOP
			curr_section = dmuci_walk_state_section("cwmp", "uploaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "uploaddiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@uploaddiagnostic[0]", "url", value);
			return 0;
	}
	return 0;
}

int get_upload_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "";
	return 0;
}

int set_upload_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:			
			UPLOAD_DIAGNOSTIC_STOP
			curr_section = dmuci_walk_state_section("cwmp", "uploaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "uploaddiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@uploaddiagnostic[0]", "ethernetpriority", value);
			return 0;
	}
	return 0;
}

int get_upload_diagnostic_romtime(char *refparam, struct dmctx *ctx, char **value)
{
	
	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "ROMtime", value);
	if ((*value)[0] == '\0')
			*value = "0";
	return 0;
}

int get_upload_diagnostic_bomtime(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "BOMtime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_upload_diagnostic_eomtime(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "EOMtime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_upload_diagnostic_test_file_length(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "TestFileLength", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int set_upload_diagnostic_test_file_length(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			UPLOAD_DIAGNOSTIC_STOP
			curr_section = dmuci_walk_state_section("cwmp", "uploaddiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "uploaddiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@uploaddiagnostic[0]", "TestFileLength", value);
			return 0;
	}
	return 0;
}

int get_upload_diagnostic_totalbytes(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "TotalBytesSent", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_upload_diagnostic_tcp_open_request_time(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "TCPOpenRequestTime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int get_upload_diagnostic_tcp_open_response_time(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@uploaddiagnostic[0]", "TCPOpenResponseTime", value);
	if ((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int entry_method_root_Upload_Diagnostics(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"UploadDiagnostics.") {
		DMOBJECT(DMROOT"UploadDiagnostics.", ctx, "0", 0, NULL, NULL, NULL);
		DMPARAM("DiagnosticsState", ctx, "1", get_upload_diagnostics_state, set_upload_diagnostics_state, NULL, 0, 1, UNDEF, NULL);
		//DMPARAM("Interface", ctx, "1", get_upload_diagnostics_interface, set_upload_diagnostics_interface, NULL, 0, 1, UNDEF, NULL); //TODO
		DMPARAM("UploadURL", ctx, "1", get_upload_diagnostics_url, set_upload_diagnostics_url, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("TestFileLength", ctx, "1", get_upload_diagnostic_test_file_length, set_upload_diagnostic_test_file_length, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		//DMPARAM("DSCP", ctx, "1", get_upload_diagnostics_dscp, set_upload_diagnostics_dscp, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("EthernetPriority", ctx, "1", get_upload_diagnostics_ethernet_priority, set_upload_diagnostics_ethernet_priority, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("ROMTime", ctx, "0", get_upload_diagnostic_romtime, NULL, "xsd:dateTime", 0, 1, UNDEF, NULL);
		DMPARAM("BOMTime", ctx, "0", get_upload_diagnostic_bomtime, NULL, "xsd:dateTime", 0, 1, UNDEF, NULL);
		DMPARAM("EOMTime", ctx, "0", get_upload_diagnostic_eomtime, NULL, "xsd:dateTime", 0, 1, UNDEF, NULL);
		DMPARAM("TotalBytesSent", ctx, "0", get_upload_diagnostic_totalbytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("TCPOpenRequestTime", ctx, "0", get_upload_diagnostic_tcp_open_request_time, NULL, "xsd:dateTime", 0, 1, UNDEF, NULL);
		DMPARAM("TCPOpenResponseTime", ctx, "0", get_upload_diagnostic_tcp_open_response_time, NULL, "xsd:dateTime", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
