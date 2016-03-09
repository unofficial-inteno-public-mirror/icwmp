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

int get_ipping_diagnostics_state(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.state;
	return 0;
}	

int set_ipping_diagnostics_state(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			if (strcmp(value, "Requested") == 0) {
				ipping_diagnostic.state = set_ping_diagnostic(ipping_diagnostic.state, value);
				cwmp_set_end_session(END_SESSION_IPPING_DIAGNOSTIC);
			}				
			return 0;
	}
	return 0;
}

int get_ipping_interface(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.interface;
	return 0;
}

int set_ipping_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:			
			ipping_diagnostic.interface = set_ping_diagnostic(ipping_diagnostic.interface, value);			
			return 0;
	}
	return 0;
}

int get_ipping_host(char *refparam, struct dmctx *ctx, char **value)
{

	*value = ipping_diagnostic.host;
	return 0;
}

int set_ipping_host(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			TRACE();	
			ipping_diagnostic.host = set_ping_diagnostic(ipping_diagnostic.host, value);
			TRACE();
			return 0;
	}
	return 0;
}

int get_ipping_repetition_number(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.repetition;
	return 0;
}

int set_ipping_repetition_number(char *refparam, struct dmctx *ctx, int action, char *value)
{
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:			
			ipping_diagnostic.repetition = set_ping_diagnostic(ipping_diagnostic.repetition, value);
			return 0;
	}
	return 0;
}

int get_ipping_timeout(char *refparam, struct dmctx *ctx, char **value)
{
	
	*value = ipping_diagnostic.timeout;	
	return 0;
}

int set_ipping_timeout(char *refparam, struct dmctx *ctx, int action, char *value)
{
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:			
			ipping_diagnostic.timeout = set_ping_diagnostic(ipping_diagnostic.timeout, value);
			return 0;
	}
	return 0;
}

int get_ipping_block_size(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.size;
	
	return 0;
}

int set_ipping_block_size(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:			
			ipping_diagnostic.size = set_ping_diagnostic(ipping_diagnostic.size, value);
	}
	return 0;
}

int get_ipping_success_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.success_count;
	
	return 0;
}

int get_ipping_failure_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.failure_count;
	
	return 0;
}

int get_ipping_average_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.average_response_time;	
	return 0;
}

int get_ipping_min_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.minimum_response_time;
	
	return 0;
}

int get_ipping_max_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_diagnostic.maximum_response_time;
	
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