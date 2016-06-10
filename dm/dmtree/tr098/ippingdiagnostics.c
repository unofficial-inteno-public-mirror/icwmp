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
#include "ipping.h" 
#include "ubus.h"
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "ippingdiagnostics.h"
#include "ipping.h"

static inline char *ipping_get(char *option, char *def)
{
	char *tmp;
	dmuci_get_varstate_string("cwmp", "@ippingdiagnostic[0]", option, &tmp);
	if(tmp && tmp[0] == '\0')
		return dmstrdup(def);
	else
		return tmp;
}
int get_ipping_diagnostics_state(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("DiagnosticState", "None");
	return 0;
}

int set_ipping_diagnostics_state(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			if (strcmp(value, "Requested") == 0) {
				IPPING_STOP
				curr_section = dmuci_walk_state_section("cwmp", "ippingdiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
				if(!curr_section)
				{
					dmuci_add_state_section("cwmp", "ippingdiagnostic", &curr_section, &tmp);
				}
				dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "DiagnosticState", value);
				cwmp_set_end_session(END_SESSION_IPPING_DIAGNOSTIC);
			}				
			return 0;
	}
	return 0;
}

int get_ipping_interface(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_varstate_string("cwmp", "@ippingdiagnostic[0]", "interface", value);	
	return 0;
}

int set_ipping_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			//IPPING_STOP
			curr_section = dmuci_walk_state_section("cwmp", "ippingdiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "ippingdiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "interface", value);
			return 0;
	}
	return 0;
}

int get_ipping_host(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@ippingdiagnostic[0]", "Host", value);
	return 0;
}

int set_ipping_host(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			IPPING_STOP
			curr_section = dmuci_walk_state_section("cwmp", "ippingdiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "ippingdiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "Host", value);
			return 0;
	}
	return 0;
}

int get_ipping_repetition_number(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("NumberOfRepetitions", "3");
	return 0;
}

int set_ipping_repetition_number(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:			
			IPPING_STOP
			curr_section = dmuci_walk_state_section("cwmp", "ippingdiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "ippingdiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "NumberOfRepetitions", value);
			return 0;
	}
	return 0;
}

int get_ipping_timeout(char *refparam, struct dmctx *ctx, char **value)
{
	
	*value = ipping_get("Timeout", "1000");	
	return 0;
}

int set_ipping_timeout(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct uci_section *curr_section = NULL;
	char *tmp;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			IPPING_STOP
			curr_section = dmuci_walk_state_section("cwmp", "ippingdiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "ippingdiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "Timeout", value);
			return 0;
	}
	return 0;
}

int get_ipping_block_size(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("DataBlockSize", "64");
	
	return 0;
}

int set_ipping_block_size(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *tmp;
	struct uci_section *curr_section = NULL;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			IPPING_STOP
			curr_section = dmuci_walk_state_section("cwmp", "ippingdiagnostic", NULL, NULL, CMP_SECTION, NULL, NULL, GET_FIRST_SECTION);
			if(!curr_section)
			{
				dmuci_add_state_section("cwmp", "ippingdiagnostic", &curr_section, &tmp);
			}
			dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "DataBlockSize", value);
	}
	return 0;
}

int get_ipping_success_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("SuccessCount", "0");
	
	return 0;
}

int get_ipping_failure_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("FailureCount", "0");
	
	return 0;
}

int get_ipping_average_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("AverageResponseTime", "0");
	return 0;
}

int get_ipping_min_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("MinimumResponseTime", "0");
	
	return 0;
}

int get_ipping_max_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("MaximumResponseTime", "0");	
	
	return 0;
}

int entry_method_root_IPPingDiagnostics(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"IPPingDiagnostics.") {
		DMOBJECT(DMROOT"IPPingDiagnostics.", ctx, "0", 0, NULL, NULL, NULL);
		DMPARAM("DiagnosticsState", ctx, "1", get_ipping_diagnostics_state, set_ipping_diagnostics_state, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Interface", ctx, "1", get_ipping_interface, set_ipping_interface, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Host", ctx, "1", get_ipping_host, set_ipping_host, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("NumberOfRepetitions", ctx, "1", get_ipping_repetition_number, set_ipping_repetition_number, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("Timeout", ctx, "1", get_ipping_timeout, set_ipping_timeout, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("DataBlockSize", ctx, "1", get_ipping_block_size, set_ipping_block_size, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		//DMPARAM("DSCP", ctx, "1", get_ipping_dscp, set_ipping_dscp, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("SuccessCount", ctx, "0", get_ipping_success_count, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("FailureCount", ctx, "0", get_ipping_failure_count, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("AverageResponseTime", ctx, "0", get_ipping_average_response_time, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MinimumResponseTime", ctx, "0", get_ipping_min_response_time, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MaximumResponseTime", ctx, "0", get_ipping_max_response_time, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}