/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2016 Inteno Broadband Technology AB
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 *
 */

#include <uci.h>
#include <stdio.h>
#include <ctype.h>
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "ip.h"
#include "diagnostic.h"

struct ip_args cur_ip_args = {0};
struct ipv4_args cur_ipv4_args = {0};

struct dm_forced_inform_s IPv4INFRM = {0, get_ipv4_finform};
struct dm_forced_inform_s IPv6INFRM = {0, get_ipv6_finform};

unsigned char get_ipv4_finform(char *refparam, struct dmctx *dmctx, void *data, char *instance)
{
	return 1;
	//TODO
}
unsigned char get_ipv6_finform(char *refparam, struct dmctx *dmctx, void *data, char *instance)
{
	return 1;
	//TODO
}
/*************************************************************
 * INIT
/*************************************************************/
inline int init_ip_args(struct dmctx *ctx, struct uci_section *s, char *ip_4address, char *ip_6address)
{
	struct ip_args *args = &cur_ip_args;
	args->ip_sec = s;
	args->ip_4address = ip_4address;
	args->ip_6address = ip_6address;
	return 0;
}

inline int init_ipv4_args(struct dmctx *ctx, struct uci_section *s, char *ip_4address, char *ip_6address)
{
	struct ipv4_args *args = &cur_ipv4_args;
	args->ipv4_sec = s;
	args->ip_4address = ip_4address;
	args->ip_6address = ip_6address;
	return 0;
}
/*************************************************************
 * GET & SET PARAM
/*************************************************************/
static inline char *ipping_get(char *option, char *def)
{
	char *tmp;
	dmuci_get_varstate_string("cwmp", "@ippingdiagnostic[0]", option, &tmp);
	if(tmp && tmp[0] == '\0')
		return dmstrdup(def);
	else
		return tmp;
}

int get_ip_ping_diagnostics_state(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("DiagnosticState", "None");
	return 0;
}	

int set_ip_ping_diagnostics_state(char *refparam, struct dmctx *ctx, int action, char *value)
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

int get_ip_ping_interface(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_varstate_string("cwmp", "@ippingdiagnostic[0]", "interface", value);	
	return 0;
}

int set_ip_ping_interface(char *refparam, struct dmctx *ctx, int action, char *value)
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

int get_ip_ping_host(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_varstate_string("cwmp", "@ippingdiagnostic[0]", "Host", value);
	return 0;
}

int set_ip_ping_host(char *refparam, struct dmctx *ctx, int action, char *value)
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

int get_ip_ping_repetition_number(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("NumberOfRepetitions", "3");
	return 0;
}

int set_ip_ping_repetition_number(char *refparam, struct dmctx *ctx, int action, char *value)
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

int get_ip_ping_timeout(char *refparam, struct dmctx *ctx, char **value)
{
	
	*value = ipping_get("Timeout", "1000");	
	return 0;
}

int set_ip_ping_timeout(char *refparam, struct dmctx *ctx, int action, char *value)
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
			dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "Timeout", value);
			return 0;
	}
	return 0;
}

int get_ip_ping_block_size(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("DataBlockSize", "64");
	
	return 0;
}

int set_ip_ping_block_size(char *refparam, struct dmctx *ctx, int action, char *value)
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

int get_ip_ping_success_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("SuccessCount", "0");
	
	return 0;
}

int get_ip_ping_failure_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("FailureCount", "0");
	
	return 0;
}

int get_ip_ping_average_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("AverageResponseTime", "0");
	return 0;
}

int get_ip_ping_min_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("MinimumResponseTime", "0");
	
	return 0;
}

int get_ip_ping_max_response_time(char *refparam, struct dmctx *ctx, char **value)
{
	*value = ipping_get("MaximumResponseTime", "0");	
	
	return 0;
}


int get_ip_interface_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *lan_name = section_name(cur_ip_args.ip_sec);
	get_interface_enable_ubus(lan_name, refparam, ctx, value);
	return 0;
}

int set_ip_interface_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *lan_name = section_name(cur_ip_args.ip_sec);
	set_interface_enable_ubus(lan_name, refparam, ctx, action, value);
	return 0;
}

int get_ip_interface_name(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup(section_name(cur_ip_args.ip_sec));
	return 0;
}

int get_firewall_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	get_interface_firewall_enabled(section_name(cur_ip_args.ip_sec), refparam, ctx, value);
	return 0;
}

int set_firewall_enabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_interface_firewall_enabled(section_name(cur_ip_args.ip_sec), refparam, ctx, action, value);
	return 0;
}


int get_ipv4_address(char *refparam, struct dmctx *ctx, char **value)
{
	*value = cur_ipv4_args.ip_4address;
	return 0;
}

int set_ipv4_address(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "proto", &proto);
			if(strcmp(proto, "static") == 0)
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ipaddr", value);
			return 0;
	}
	return 0;
}

int get_ipv4_netmask(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "netmask", value);
	return 0;
}

int set_ipv4_netmask(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "proto", &proto);
			if(strcmp(proto, "static") == 0)
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "netmask", value);
			return 0;
	}
	return 0;
}

int get_ipv4_addressing_type (char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "proto", value);
	if (strcmp(*value, "static") == 0)
		*value = "Static";
	else if (strcmp(*value, "dhcp") == 0)
		*value = "DHCP";
	else
		*value = "";
	return 0;
}

int set_ipv4_addressing_type(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if(strcasecmp(value, "static") == 0) {
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "proto", "static");
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ipaddr", "0.0.0.0");
			}
			if(strcasecmp(value, "dhcp") == 0) {
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "proto", "dhcp");
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ipaddr", "");
			}
			return 0;
	}
	return 0;
}

int get_ipv6_addressing_type (char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "proto", value);
	if (strcmp(*value, "static") == 0)
		*value = "Static";
	else if (strcmp(*value, "dhcpv6") == 0)
		*value = "DHCPv6";
	else
		*value = "";
	return 0;
}

int set_ipv6_addressing_type(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if(strcasecmp(value, "static") == 0) {
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "proto", "static");
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ip6addr", "0.0.0.0");
			}
			if(strcasecmp(value, "dhcpv6") == 0) {
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "proto", "dhcpv6");
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ip6addr", "");
			}
			return 0;
	}
	return 0;
}
int get_ip_int_lower_layer(char *refparam, struct dmctx *ctx, char **value)
{
	char *wifname, *wtype, *br_inst, *mg, *device, *proto;
	struct uci_section *port;
	json_object *res;
	char buf[8];
	char linker[64] = "";

	dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "type", &wtype);
	if (strcmp(wtype, "bridge") == 0) {
		dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "bridge_instance", &br_inst);
		uci_foreach_option_eq("dmmap", "bridge_port", "bridge_key", br_inst, port) {
			dmuci_get_value_by_section_string(port, "mg_port", &mg);
			if (strcmp(mg, "true") == 0)
				sprintf(linker, "%s+", section_name(port));

			adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cBridging%cBridge%c", dm_delim, dm_delim, dm_delim), linker, value);
			if (*value == NULL)
				*value = "";
			return 0;
		}
	} else if (wtype[0] == '\0' || strcmp(wtype, "anywan") == 0) {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(cur_ip_args.ip_sec)}}, 1, &res);
		dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "ifname", &wifname);
		strcpy (linker, wifname);
		if (res) {
			json_select(res, "device", -1, NULL, &device, NULL);
			strcpy(linker, device);
			if(device[0] == '\0') {
				strncpy(buf, wifname, 6);
				buf[6]='\0';
				strcpy(linker, buf);
			}
		}
		dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "proto", &proto);
		if (strstr(proto, "ppp")) {
			sprintf(linker, "%s", section_name(cur_ip_args.ip_sec));
		}
	}
	adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cATM%cLink%c", dm_delim, dm_delim, dm_delim), linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cPTM%cLink%c", dm_delim, dm_delim, dm_delim), linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cEthernet%cInterface%c", dm_delim, dm_delim, dm_delim), linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cWiFi%cSSID%c", dm_delim, dm_delim, dm_delim), linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cPPP%cInterface%c", dm_delim, dm_delim, dm_delim), linker, value);
	if (*value == NULL)
		*value = "";
	return 0;
}

int set_ip_int_lower_layer(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *linker, *pch, *spch, *dup, *b_key, *proto, *ipaddr, *ip_inst, *ipv4_inst, *p, *type;
	char sec[16];
	char pat[32] = "";
	struct uci_section *s;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(ctx, value, &linker);
			sprintf(pat, "%cPort%c1%c", dm_delim, dm_delim, dm_delim);
			if (linker && strstr(value, pat))
			{
				strncpy(sec, linker, strlen(linker) - 1);
				sec[strlen(linker) - 1] = '\0';
				dmuci_get_option_value_string("dmmap", sec, "bridge_key", &b_key);
				dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "proto", &proto);
				dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "ipaddr", &ipaddr);
				dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "ip_int_instance", &ip_inst);
				dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "ipv4_instance", &ipv4_inst);
				uci_foreach_option_eq("network", "interface", "bridge_instance", b_key, s) {
					dmuci_set_value_by_section(s, "proto", proto);
					dmuci_set_value_by_section(s, "ipaddr", ipaddr);
					dmuci_set_value_by_section(s, "ip_int_instance", ip_inst);
					dmuci_set_value_by_section(s, "ipv4_instance", ipv4_inst);
					dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "type", &type);
					if (strcmp (type, "bridge"))
						dmuci_delete_by_section(cur_ip_args.ip_sec, NULL, NULL);
				}
				return 0;
			}
			if (linker)
				dmuci_set_value_by_section(cur_ip_args.ip_sec, "ifname", linker);

			return 0;
	}
	return 0;
}

int get_ipv6_address(char *refparam, struct dmctx *ctx, char **value)
{
	*value = cur_ipv4_args.ip_6address;
	return 0;
}

int set_ipv6_address(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "proto", &proto);
			if(strcmp(proto, "static") == 0)
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ip6addr", value);
			return 0;
	}
	return 0;
}

int get_ip_enable(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "true";
	return 0;
}
/*************************************************************
 * GET & SET ALIAS
/*************************************************************/
int get_ip_int_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "ip_int_alias", value);
	return 0;
}

int set_ip_int_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip_int_alias", value);
			return 0;
	}
	return 0;
}

int get_ipv4_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "ipv4_alias", value);
	return 0;
}

int set_ipv4_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "proto", &proto);
			if(strcmp(proto, "static") == 0)
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ipv4_alias", value);
			return 0;
	}
	return 0;
}

int get_ipv6_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "ipv6_alias", value);
	return 0;
}

int set_ipv6_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_ipv4_args.ipv4_sec, "proto", &proto);
			if(strcmp(proto, "static") == 0)
				dmuci_set_value_by_section(cur_ipv4_args.ipv4_sec, "ipv6_alias", value);
			return 0;
	}
	return 0;
}
/*************************************************************
 * ADD & DEL OBJ
/*************************************************************/
char *get_last_instance_cond(char *package, char *section, char *opt_inst, char *opt_cond, char *cond_val, char *opt_filter, char *filter_val)
{
	struct uci_section *s;
	char *inst = NULL, *val, *val_f;

	uci_foreach_sections(package, section, s) {
		if (opt_cond) dmuci_get_value_by_section_string(s, opt_cond, &val);
		if (opt_filter) dmuci_get_value_by_section_string(s, opt_filter, &val_f);
		if(opt_cond && opt_filter && (strcmp(val, cond_val) == 0 || strcmp(val_f, filter_val) == 0))
			continue;
		inst = update_instance(s, inst, opt_inst);
	}
	return inst;
}

int add_ip_interface(struct dmctx *ctx, char **instance)
{
	char *last_inst;
	char ip_name[32], ib[8];
	char *p = ip_name;

	last_inst = get_last_instance_cond("network", "interface", "ip_int_instance", "type", "alias", "proto", "");
	sprintf(ib, "%d", last_inst ? atoi(last_inst)+1 : 1);
	dmstrappendstr(p, "ip_interface_");
	dmstrappendstr(p, ib);
	dmstrappendend(p);
	dmuci_set_value("network", ip_name, "", "interface");
	dmuci_set_value("network", ip_name, "proto", "dhcp");
	*instance = dmuci_set_value("network", ip_name, "ip_int_instance", ib);
	return 0;
}

int delete_ip_interface(struct dmctx *ctx, unsigned char del_action)
{
	switch (del_action) {
	case DEL_INST:
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "type", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "bridge_instance", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip_int_instance", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv4_instance", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv6_instance", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ifname", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipaddr", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip6addr", "");
		break;
	case DEL_ALL:
		return FAULT_9005;
	}
	return 0;
}

int add_ipv4(struct dmctx *ctx, char **instancepara)
{
	char *instance;

	dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "ipv4_instance", &instance);
	*instancepara = update_instance(cur_ip_args.ip_sec, instance, "ipv4_instance");
	if(instance[0] == '\0') {
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv4_instance", *instancepara);
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipaddr", "0.0.0.0");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "static");
	}
	return 0;
}

int delete_ipv4(struct dmctx *ctx, unsigned char del_action)
{
	switch (del_action) {
	case DEL_INST:
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipaddr", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv4_instance", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "");
		break;
	case DEL_ALL:
		return FAULT_9005;
	}
	return 0;
}

int add_ipv6(struct dmctx *ctx, char **instancepara)
{
	char *instance;

	dmuci_get_value_by_section_string(cur_ip_args.ip_sec, "ipv6_instance", &instance);
	*instancepara = update_instance(cur_ip_args.ip_sec, instance, "ipv6_instance");
	if(instance[0] == '\0') {
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv6_instance", *instancepara);
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip6addr", "::");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "static");
	}
	return 0;
}

int delete_ipv6(struct dmctx *ctx, unsigned char del_action)
{
	switch (del_action) {
	case DEL_INST:
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip6addr", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv6_instance", "");
		dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "");
		break;
	case DEL_ALL:
		return FAULT_9005;
	}
	return 0;
}

/**************************************************************************
* LINKER
***************************************************************************/
int get_linker_ip_interface(char *refparam, struct dmctx *dmctx, void *data, char *instance, char **linker) {

	if(cur_ip_args.ip_sec) {
		dmasprintf(linker,"%s", section_name(cur_ip_args.ip_sec));
		return 0;
	} else {
		*linker = "";
		return 0;
	}
}
/*************************************************************
 * ENTRY METHOD
/*************************************************************/

/* *** Device.IP. *** */
DMOBJ tIPObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"Interface", &DMWRITE, add_ip_interface, delete_ip_interface, NULL, browseIPIfaceInst, NULL, NULL, tInterfaceObj, tIPintParams, get_linker_ip_interface},
{"Diagnostics", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tDiagnosticObj, NULL, NULL},
{0}
};

DMLEAF tIPintParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"Enable", &DMWRITE, DMT_BOOL, get_ip_interface_enable, set_ip_interface_enable, NULL, NULL},
{"Name", &DMREAD, DMT_STRING, get_ip_interface_name, set_ipv4_address, NULL, NULL},
{"LowerLayers", &DMWRITE, DMT_STRING, get_ip_int_lower_layer, set_ip_int_lower_layer, NULL, NULL},
{0}
};
/* *** Device.IP.Interface. *** */
DMOBJ tInterfaceObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, nextobj, leaf*/
{"IPv4Address", &DMREAD, add_ipv4, delete_ipv4, NULL, browseIfaceIPv4Inst, NULL, NULL, NULL, tIPv4Params, NULL},
{"IPv6Address", &DMREAD, add_ipv6, delete_ipv6, NULL, browseIfaceIPv6Inst, NULL, NULL, NULL, tIPv6Params, NULL},
{0}
};

DMOBJ tDiagnosticObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, nextobj, leaf*/
{"IPPingDiagnostics", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tIpPingDiagParams, NULL},
{0}
};
/* *** Device.IP.Interface.{i}.IPv4Address.{i}. *** */
DMLEAF tIPv4Params[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform*/
{"Alias", &DMWRITE, DMT_STRING, get_ipv4_alias, set_ipv4_alias, &IPv4INFRM, NULL},
{"Enable", &DMREAD, DMT_BOOL, get_ip_enable, NULL, &IPv4INFRM, NULL},
{"X_BROADCOM_COM_FirewallEnabled", &DMWRITE, DMT_BOOL, get_firewall_enabled, set_firewall_enabled, &IPv4INFRM, NULL},
{"IPAddress", &DMWRITE, DMT_STRING, get_ipv4_address, set_ipv4_address, &IPv4INFRM, NULL},
{"SubnetMask", &DMWRITE, DMT_STRING, get_ipv4_netmask, set_ipv4_netmask, &IPv4INFRM, NULL},
{"AddressingType", &DMWRITE, DMT_STRING, get_ipv4_addressing_type, set_ipv4_addressing_type, &IPv4INFRM, NULL},
{0}
};


/* *** Device.IP.Interface.{i}.IPv6Address.{i}. *** */
DMLEAF tIPv6Params[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform*/
{"Alias", &DMWRITE, DMT_STRING, get_ipv6_alias, set_ipv6_alias, &IPv6INFRM, NULL},
{"Enable", &DMREAD, DMT_BOOL, get_ip_enable, NULL, &IPv6INFRM, NULL},
{"IPAddress", &DMWRITE, DMT_STRING, get_ipv6_address, set_ipv6_address, &IPv6INFRM, NULL},
{"Origin", &DMWRITE, DMT_STRING, get_ipv6_addressing_type, set_ipv6_addressing_type, &IPv6INFRM, NULL},
{0}
};

DMLEAF tIpPingDiagParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"DiagnosticsState", &DMWRITE, DMT_STRING, get_ip_ping_diagnostics_state, set_ip_ping_diagnostics_state, NULL, NULL},
{"Interface", &DMWRITE, DMT_BOOL, get_ip_ping_interface, set_ip_ping_diagnostics_state, NULL, NULL},
{"Host", &DMWRITE, DMT_STRING, get_ip_ping_host, set_ip_ping_host, NULL, NULL},
{"NumberOfRepetitions", &DMWRITE, DMT_UNINT, get_ip_ping_repetition_number, set_ip_ping_repetition_number, NULL, NULL},
{"Timeout", &DMWRITE, DMT_UNINT, get_ip_ping_timeout, set_ip_ping_timeout, NULL, NULL},
{"DataBlockSize", &DMWRITE, DMT_UNINT, get_ip_ping_block_size, set_ip_ping_block_size, NULL, NULL},
{"SuccessCount", &DMREAD, DMT_UNINT, get_ip_ping_success_count, NULL, NULL},
{"FailureCount", &DMREAD, DMT_UNINT, get_ip_ping_failure_count, set_ipv6_addressing_type, NULL, NULL},
{"AverageResponseTime", &DMREAD, DMT_UNINT, get_ip_ping_average_response_time, NULL, NULL, NULL},
{"MinimumResponseTime", &DMREAD, DMT_UNINT, get_ip_ping_min_response_time, NULL, NULL, NULL},
{"MaximumResponseTime", &DMREAD, DMT_UNINT, get_ip_ping_max_response_time, NULL, NULL, NULL},
{0}
};

int browseIPIfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *net_sec = NULL;
	char *ip_int = NULL, *ip_int_last = NULL;
	char *type, *ipv4addr = "", *ipv6addr = "", *proto, *inst;
	json_object *res;

	uci_foreach_sections("network", "interface", net_sec) {
		dmuci_get_value_by_section_string(net_sec, "type", &type);
		if (!strcmp(type, "alias") || !strcmp(section_name(net_sec), "loopback"))
			continue;
		dmuci_get_value_by_section_string(net_sec, "ipaddr", &ipv4addr);
		if (ipv4addr[0] == '\0') {
			dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(net_sec)}}, 1, &res);
			if (res)
				json_select(res, "ipv4-address", 0, "address", &ipv4addr, NULL);
		}
		dmuci_get_value_by_section_string(net_sec, "ip6addr", &ipv6addr);
		if (ipv6addr[0] == '\0') {
			dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(net_sec)}}, 1, &res);
			if (res)
				json_select(res, "ipv6-address", 0, "address", &ipv6addr, NULL);
		}
		dmuci_get_value_by_section_string(net_sec, "proto", &proto);
		dmuci_get_value_by_section_string(net_sec, "ip_int_instance", &inst);
		if (ipv4addr[0] == '\0' && ipv6addr[0] == '\0' && strcmp(proto, "dhcp") != 0 && strcmp(proto, "dhcpv6") != 0 && strcmp(inst, "") == 0) {
			continue;
		}
		init_ip_args(dmctx, net_sec, ipv4addr, ipv6addr);
		ip_int = handle_update_instance(1, dmctx, &ip_int_last, update_instance_alias, 3, net_sec, "ip_int_instance", "ip_int_alias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, ip_int) == DM_STOP)
			break;
	}
	DM_CLEAN_ARGS(cur_ip_args);
	return 0;
}

int browseIfaceIPv4Inst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *ip_sec = NULL;
	char *type, *ifname, *ipv4, *ipv6, *ipv4_inst = NULL, *ipv6_inst = NULL,*ipv4_inst_last = NULL, *ipv6_inst_last = NULL ;

	if(cur_ip_args.ip_4address[0] != '\0') {
		ipv4_inst = handle_update_instance(2, dmctx, &ipv4_inst_last, update_instance_alias, 3, cur_ip_args.ip_sec, "ipv4_instance", "ipv4_alias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, ipv4_inst) == DM_STOP)
			goto end;
	}
	dmasprintf(&ifname, "br-%s", section_name(cur_ip_args.ip_sec));
	uci_foreach_option_eq("network", "interface", "ifname", ifname, ip_sec) {
		dmuci_get_value_by_section_string(ip_sec, "ipaddr", &ipv4);
		if(ipv4[0] != '\0') {
			init_ipv4_args(dmctx, ip_sec, cur_ip_args.ip_4address, NULL);
			ipv4_inst = handle_update_instance(2, dmctx, &ipv4_inst_last, update_instance_alias, 3, ip_sec, "ipv4_instance", "ipv4_alias");
			if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, ipv4_inst) == DM_STOP)
				goto end;
		}
	}
end:
	DM_CLEAN_ARGS(cur_ipv4_args);
	return 0;
}

int browseIfaceIPv6Inst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *ip_sec = NULL;
	char *type, *ifname, *ipv4, *ipv6, *ipv4_inst = NULL, *ipv6_inst = NULL,*ipv4_inst_last = NULL, *ipv6_inst_last = NULL ;

	if (cur_ip_args.ip_6address[0] != '\0') {
		ipv6_inst = handle_update_instance(2, dmctx, &ipv6_inst_last, update_instance_alias, 3, cur_ip_args.ip_sec, "ipv6_instance", "ipv6_alias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, ipv6_inst) == DM_STOP)
			goto end;
	}
	dmasprintf(&ifname, "br-%s", section_name(cur_ip_args.ip_sec));
	uci_foreach_option_eq("network", "interface", "ifname", ifname, ip_sec) {
		dmuci_get_value_by_section_string(ip_sec, "ip6addr", &ipv6);
		if(ipv6[0] != '\0') {
			init_ipv4_args(dmctx, ip_sec, NULL, cur_ip_args.ip_6address);
			ipv6_inst = handle_update_instance(2, dmctx, &ipv6_inst_last, update_instance_alias, 3, ip_sec, "ipv6_instance", "ipv6_alias");
			if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, ipv6_inst) == DM_STOP)
				goto end;
		}
	}
end:
	DM_CLEAN_ARGS(cur_ipv4_args);
	return 0;
}
