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


int get_igmp_dscp_mark(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_dscp_mark", value); 
	return 0;
}

int set_igmp_dscp_mark(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_dscp_mark", value);
			return 0;
	}
	return 0;
}

int get_igmp_proxy_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *p;
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_proxy_interfaces", value);
	*value = dmstrdup(*value);  // MEM WILL BE FREED IN DMMEMCLEAN
	p = *value;
	while (*p++) {
		if (*p == ' ') *p = ',';
	}
	return 0;
}

int set_igmp_proxy_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	int i;
	char *p;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				return 0;
			value = dmstrdup(value);
			p = value;
			while (*p++) {
				if (*p == ',') *p = ' ';
			}
			compress_spaces(value);
			dmuci_set_value("mcpd", "mcpd", "igmp_proxy_interfaces", value);
			dmfree(value);
			return 0;
	}
	return 0;
}

int get_igmp_default_version(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_default_version", value);
	return 0;
} 

int set_igmp_default_version(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_default_version", value);
			return 0;
	}
	return 0;
}

int get_igmp_query_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_query_interval", value); 
	return 0;
} 

int set_igmp_query_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_query_interval", value);
			return 0;
	}
	return 0;
}

int get_igmp_query_response_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_query_response_interval", value);
	return 0;
} 

int set_igmp_query_response_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_query_response_interval", value);
			return 0;
	}
	return 0;
}

int get_igmp_last_member_queryinterval(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_last_member_query_interval", value);
	return 0;
} 

int set_igmp_last_member_queryinterval(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_last_member_query_interval", value);
			return 0;
	}
	return 0;
}

int get_igmp_robustness_value(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_robustness_value", value);
	return 0;
} 

int set_igmp_robustness_value(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_robustness_value", value);
			return 0;
	}
	return 0;
}

int get_igmp_multicast_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_lan_to_lan_multicast", value);
	if ((*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int set_igmp_multicast_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_lan_to_lan_multicast", "1");
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_lan_to_lan_multicast", "");
			return 0;
	}
	return 0;
}

int get_igmp_fastleave_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_fast_leave", value);
	if ((*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int set_igmp_fastleave_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_fast_leave", "1");
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_fast_leave", "");
			return 0;
	}
	return 0;
}

int get_igmp_joinimmediate_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_join_immediate", value);
	if ((*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int set_igmp_joinimmediate_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_join_immediate", "1");
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_join_immediate", "");
			return 0;
	}
	return 0;
}

int get_igmp_proxy_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_proxy_enable", value);
	if ((*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int set_igmp_proxy_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value("mcpd", "mcpd", "igmp_proxy_enable", "1");
			else
				dmuci_set_value("mcpd", "mcpd", "igmp_proxy_enable", "");
			return 0;
	}
	return 0;
}

int get_igmp_maxgroup(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_max_groups", value); 
	return 0;
} 

int set_igmp_maxgroup(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_max_groups", value);
			return 0;
	}
	return 0;
}

int get_igmp_maxsources(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_max_sources", value);
	return 0;
} 

int set_igmp_maxsources(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_max_sources", value);
			return 0;
	}
	return 0;
}

int get_igmp_maxmembers(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_max_members", value);
	return 0;
}

int set_igmp_maxmembers(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_max_members", value);
			return 0;
	}
	return 0;
}

int get_igmp_snooping_mode(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_snooping_enable", value);
	return 0;
}

int set_igmp_snooping_mode(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("mcpd", "mcpd", "igmp_snooping_enable", value);
			return 0;
	}
	return 0;
}

int get_igmp_snooping_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *p;

	dmuci_get_option_value_string("mcpd", "mcpd", "igmp_snooping_interfaces", value);
	*value = dmstrdup(*value);  // MEM WILL BE FREED IN DMMEMCLEAN
	p = *value;
	while (*p++) {
		if (*p == ' ') *p = ',';
	}

	return 0;
}

int set_igmp_snooping_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	int i;
	char *p;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				return 0;
			value = dmstrdup(value);
			p = value;
			while (*p++) {
				if (*p == ',') *p = ' ';
			}
			compress_spaces(value);
			dmuci_set_value("mcpd", "mcpd", "igmp_snooping_interfaces", value);
			dmfree(value);
			return 0;
	}
	return 0;
}

DMLEAF tSe_IgmpParam[] = {
{"DifferentiateService", &DMWRITE, DMT_STRING, get_igmp_dscp_mark, set_igmp_dscp_mark, NULL, NULL},
{"ProxyInterface", &DMWRITE, DMT_STRING, get_igmp_proxy_interface, set_igmp_proxy_interface, NULL, NULL},
{"DefaultVersion", &DMWRITE, DMT_STRING, get_igmp_default_version, set_igmp_default_version, NULL, NULL},
{"QueryInterval", &DMWRITE, DMT_UNINT, get_igmp_query_interval, set_igmp_query_interval, NULL, NULL},
{"QueryResponseInterval", &DMWRITE, DMT_UNINT, get_igmp_query_response_interval, set_igmp_query_response_interval, NULL, NULL},
{"LastMemberQueryInterval", &DMWRITE, DMT_UNINT, get_igmp_last_member_queryinterval, set_igmp_last_member_queryinterval, NULL, NULL},
{"RobustnessValue", &DMWRITE, DMT_INT, get_igmp_robustness_value, set_igmp_robustness_value, NULL, NULL},
{"LanToLanMulticastEnable", &DMWRITE, DMT_BOOL, get_igmp_multicast_enable, set_igmp_multicast_enable, NULL, NULL},
{"MaxGroup", &DMWRITE, DMT_UNINT, get_igmp_maxgroup, set_igmp_maxgroup, NULL, NULL},
{"MaxSources", &DMWRITE, DMT_UNINT, get_igmp_maxsources, set_igmp_maxsources, NULL, NULL},
{"MaxMembers", &DMWRITE, DMT_UNINT, get_igmp_maxmembers, set_igmp_maxmembers, NULL, NULL},
{"FastLeaveEnable", &DMWRITE, DMT_BOOL, get_igmp_fastleave_enable, set_igmp_fastleave_enable, NULL, NULL},
{"JoinImmediateEnable", &DMWRITE, DMT_BOOL, get_igmp_joinimmediate_enable, set_igmp_joinimmediate_enable, NULL, NULL},
{"ProxyEnable", &DMWRITE, DMT_BOOL, get_igmp_proxy_enable, set_igmp_proxy_enable, NULL, NULL},
{"SnoopingMode", &DMWRITE, DMT_STRING, get_igmp_snooping_mode, set_igmp_snooping_mode, NULL, NULL},
{"SnoopingInterfaces", &DMWRITE, DMT_STRING, get_igmp_snooping_interface, set_igmp_snooping_interface, NULL, NULL},
{0}
};
