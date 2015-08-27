/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *
 */ 

#include <uci.h>
#include <stdio.h>
#include <ctype.h>
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "x_inteno_se_igmp.h"


int get_igmp_dscp_mark(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_dscp_mark", value); 
	return 0;
}

int set_igmp_dscp_mark(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_dscp_mark", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_proxy_interface(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_proxy_interfaces", value);
	//echo ${value// /,} TODO REPLACE SPACE BY ','
	return 0;
}

int set_igmp_proxy_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int i;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				return 0;
			int ln = strlen(value); 
			for (i = 0; i< strlen(value); i++) {
				if (value[i] == ',') {
					value[i] = ' ';
				}
			}
			compress_spaces(value);
			dmuci_set_value("mcpd", "mcpd", "igmp_proxy_interfaces", value);
			//delay_service restart "mcpd" "1" //TODO
			return 0;
	}
	return 0;
}

int get_igmp_default_version(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_default_version", value); 
	return 0;
} 

int set_igmp_default_version(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_default_version", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_query_interval(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_query_interval", value); 
	return 0;
} 

int set_igmp_query_interval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_query_interval", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_query_response_interval(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_query_response_interval", value); 
	return 0;
} 

int set_igmp_query_response_interval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_query_response_interval", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_last_member_queryinterval(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_last_member_query_interval", value); 
	return 0;
} 

int set_igmp_last_member_queryinterval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_last_member_query_interval", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_robustness_value(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_robustness_value", value); 
	return 0;
} 

int set_igmp_robustness_value(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_robustness_value", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_multicast_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_lan_to_lan_multicast", value);
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("0");
	}
	return 0;
}

int set_igmp_multicast_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_lan_to_lan_multicast", value);
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_lan_to_lan_multicast", "");			
			//delay_service restart "mcpd" "1" //TODO
			return 0;
	}
	return 0;
}

int get_igmp_fastleave_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_fast_leave", value);
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("0");
	}
	return 0;
}

int set_igmp_fastleave_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_fast_leave", value);
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_fast_leave", "");
			//delay_service restart "mcpd" "1" //TODO
			return 0;
	}
	return 0;
}

int get_igmp_joinimmediate_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_join_immediate", value);
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("0");
	}
	return 0;
}

int set_igmp_joinimmediate_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_join_immediate", value);
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_join_immediate", "");
			//delay_service restart "mcpd" "1" //TODO
			return 0;
	}
	return 0;
}

int get_igmp_proxy_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_proxy_enable", value);
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("0");
	}
	return 0;
}

int set_igmp_proxy_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_proxy_enable", value);
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_proxy_enable", "");
			//delay_service restart "mcpd" "1" //TODO
			return 0;
	}
	return 0;
}

int get_igmp_maxgroup(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_max_groups", value); 
	return 0;
} 

int set_igmp_maxgroup(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_max_groups", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_maxsources(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_max_sources", value); 
	return 0;
} 

int set_igmp_maxsources(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_max_sources", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_maxmembers(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_max_members", value);
	return 0;
}

int set_igmp_maxmembers(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_max_members", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_snooping_mode(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_snooping_enable", value);
	return 0;
} 

int set_igmp_snooping_mode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_snooping_enable", value);
			//delay_service restart "mcpd" "1" TODO
			return 0;
	}
	return 0;
}

int get_igmp_snooping_interface(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_snooping_interfaces", value);
	// echo ${value// /,} TODO
	return 0;
}

int set_igmp_snooping_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int i;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				return 0;
			int ln = strlen(value);
			for (i = 0; i< strlen(value); i++) {
				if (value[i] == ',') {
					value[i] = ' ';
				}
			}
			compress_spaces(value);
			dmuci_set_value("mcpd", "mcpd", "igmp_snooping_interfaces", value);
			//delay_service restart "mcpd" "1" //TODO
			return 0;
	}
	return 0;
}

int entry_method_root_X_INTENO_SE_IGMP(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_IGMP.") {
		DMOBJECT(DMROOT"X_INTENO_SE_IGMP.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("DifferentiateService", ctx, "1", get_igmp_dscp_mark, set_igmp_dscp_mark, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ProxyInterface", ctx, "1", get_igmp_proxy_interface, set_igmp_proxy_interface, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("DefaultVersion", ctx, "1", get_igmp_default_version, set_igmp_default_version, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("QueryInterval", ctx, "1", get_igmp_query_interval, set_igmp_query_interval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("QueryResponseInterval", ctx, "1", get_igmp_query_response_interval, set_igmp_query_response_interval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("LastMemberQueryInterval", ctx, "1", get_igmp_last_member_queryinterval, set_igmp_last_member_queryinterval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("RobustnessValue", ctx, "1", get_igmp_robustness_value, set_igmp_robustness_value, "xsd:int", 0, 1, UNDEF, NULL);
		DMPARAM("LanToLanMulticastEnable", ctx, "1", get_igmp_multicast_enable, set_igmp_multicast_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("MaxGroup", ctx, "1", get_igmp_maxgroup, set_igmp_maxgroup, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MaxSources", ctx, "1", get_igmp_maxsources, set_igmp_maxsources, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MaxMembers", ctx, "1", get_igmp_maxmembers, set_igmp_maxmembers, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("FastLeaveEnable", ctx, "1", get_igmp_fastleave_enable, set_igmp_fastleave_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("JoinImmediateEnable", ctx, "1", get_igmp_joinimmediate_enable, set_igmp_joinimmediate_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("ProxyEnable", ctx, "1", get_igmp_proxy_enable, set_igmp_proxy_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("SnoopingMode", ctx, "1", get_igmp_snooping_mode, set_igmp_snooping_mode, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SnoopingInterfaces", ctx, "1", get_igmp_snooping_interface, set_igmp_snooping_interface, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
