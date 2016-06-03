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
#include "cwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "ip.h"
#include "ipping.h"

struct ip_args cur_ip_args = {0};
struct ipv4_args cur_ipv4_args = {0};

/*************************************************************
 * INIT
/*************************************************************/
inline int init_ip_args(struct dmctx *ctx, struct uci_section *s)
{
	struct ip_args *args = &cur_ip_args;
	ctx->args = (void *)args;
	args->ip_sec = s;
	return 0;
}

inline int init_ipv4_args(struct dmctx *ctx, struct uci_section *s, char *ip_address, char *ip_6address)
{
	struct ipv4_args *args = &cur_ipv4_args;
	ctx->args = (void *)args;
	args->ipv4_sec = s;
	args->ip_address = ip_address;
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
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			if (strcmp(value, "Requested") == 0) {
				IPPING_STOP
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
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			//IPPING_STOP
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
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			TRACE();	
			IPPING_STOP
			dmuci_set_varstate_value("cwmp", "@ippingdiagnostic[0]", "Host", value);
			TRACE();
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
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			IPPING_STOP
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
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			IPPING_STOP
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
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			IPPING_STOP
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
	*value = cur_ipv4_args.ip_address;
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

			adm_entry_get_linker_param(DMROOT"Bridging.Bridge.", linker, value);
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
	adm_entry_get_linker_param(DMROOT"ATM.Link.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"PTM.Link.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"Ethernet.Interface.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"WiFi.SSID.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"PPP.Interface.", linker, value);
	if (*value == NULL)
		*value = "";
	return 0;
}

int set_ip_int_lower_layer(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *linker, *pch, *spch, *dup, *b_key, *proto, *ipaddr, *ip_inst, *ipv4_inst, *p, *type;
	char sec[16];
	struct uci_section *s;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(value, &linker);
			p = strstr(value, ".Port.");
			if (linker && p && strcmp(p, ".Port.1.") == 0)
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

int delete_ip_interface(struct dmctx *ctx)
{
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "type", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "bridge_instance", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip_int_instance", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv4_instance", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv6_instance", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ifname", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipaddr", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip6addr", "");
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

int delete_ipv4(struct dmctx *ctx)
{
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipaddr", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv4_instance", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "");
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

int delete_ipv6(struct dmctx *ctx)
{
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ip6addr", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "ipv6_instance", "");
	dmuci_set_value_by_section(cur_ip_args.ip_sec, "proto", "");
	return 0;
}

/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_ip(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"IP.") {
		DMOBJECT(DMROOT"IP.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"IP.Interface.", ctx, "1", 1, add_ip_interface, NULL, NULL);
		DMOBJECT(DMROOT"IP.Diagnostics.", ctx, "0", 0, NULL, NULL, NULL);
		SUBENTRY(entry_ip_interface, ctx);
		SUBENTRY(entry_ip_ping_diagnostic, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_ip_interface(struct dmctx *ctx)
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
		init_ip_args(ctx, net_sec);
		ip_int = handle_update_instance(1, ctx, &ip_int_last, update_instance_alias, 3, net_sec, "ip_int_instance", "ip_int_alias");
		SUBENTRY(entry_ip_interface_instance, ctx, ip_int);
		SUBENTRY(entry_ip_address, ctx, net_sec, ipv4addr, ipv6addr, ip_int);
	}
	return 0;
}

inline int entry_ip_address(struct dmctx *ctx, struct uci_section *net_sec, char *ipv4addr, char *ipv6addr,char *ip_int)
{
	struct uci_section *ip_sec = NULL;
	char *type, *ifname, *ipv4, *ipv6, *ipv4_inst = NULL, *ipv6_inst = NULL,*ipv4_inst_last = NULL, *ipv6_inst_last = NULL ;

	init_ipv4_args(ctx, net_sec, ipv4addr, ipv6addr);
	if(ipv4addr[0] != '\0') {
		ipv4_inst = handle_update_instance(2, ctx, &ipv4_inst_last, update_instance_alias, 3, net_sec, "ipv4_instance", "ipv4_alias");
		SUBENTRY(entry_ipv4_address_instance, ctx, ip_int, ipv4_inst);
	}
	if (ipv6addr[0] != '\0') {
		ipv6_inst = handle_update_instance(2, ctx, &ipv6_inst_last, update_instance_alias, 3, net_sec, "ipv6_instance", "ipv6_alias");
		SUBENTRY(entry_ipv6_address_instance, ctx, ip_int, ipv6_inst);
	}
	dmasprintf(&ifname, "br-%s", section_name(net_sec));
	uci_foreach_option_eq("network", "interface", "ifname", ifname, ip_sec) {
		dmuci_get_value_by_section_string(ip_sec, "ipaddr", &ipv4);
		dmuci_get_value_by_section_string(ip_sec, "ip6addr", &ipv6);
		if(ipv4[0] != '\0') {
			init_ipv4_args(ctx, ip_sec, ipv4, ipv6);
			ipv4_inst = handle_update_instance(2, ctx, &ipv4_inst_last, update_instance_alias, 3, ip_sec, "ipv4_instance", "ipv4_alias");
			SUBENTRY(entry_ipv4_address_instance, ctx, ip_int, ipv4_inst);
		}
		if(ipv6[0] != '\0') {
			init_ipv4_args(ctx, ip_sec, ipv4, ipv6);
			ipv6_inst = handle_update_instance(2, ctx, &ipv6_inst_last, update_instance_alias, 3, ip_sec, "ipv6_instance", "ipv6_alias");
			SUBENTRY(entry_ipv6_address_instance, ctx, ip_int, ipv6_inst);
		}
	}
	return 0;
}

inline int entry_ip_interface_instance(struct dmctx *ctx, char *int_num)
{
	IF_MATCH(ctx, DMROOT"IP.Interface.%s.", int_num) {
		char linker[32] = "";
		strcat(linker, section_name(cur_ip_args.ip_sec));
		DMOBJECT(DMROOT"IP.Interface.%s.", ctx, "1", 1, NULL, delete_ip_interface, linker, int_num);
		DMPARAM("Alias", ctx, "1", get_ip_int_alias, set_ip_int_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_ip_interface_enable, set_ip_interface_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "0", get_ip_interface_name, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LowerLayers", ctx, "1", get_ip_int_lower_layer, set_ip_int_lower_layer, NULL, 0, 1, UNDEF, NULL);//TODO
		DMOBJECT(DMROOT"IP.Interface.%s.IPv4Address.", ctx, "1", 1, add_ipv4, NULL, NULL, int_num);
		DMOBJECT(DMROOT"IP.Interface.%s.IPv6Address.", ctx, "1", 1, add_ipv6, NULL, NULL, int_num);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_ipv4_address_instance(struct dmctx *ctx, char *int_num, char *ip_inst)
{
	IF_MATCH(ctx, DMROOT"IP.Interface.%s.IPv4Address.%s.", int_num, ip_inst) {
		DMOBJECT(DMROOT"IP.Interface.%s.IPv4Address.%s.", ctx, "1", 1, NULL, delete_ipv4, NULL, int_num, ip_inst);
		DMPARAM("Alias", ctx, "1", get_ipv4_alias, set_ipv4_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "0", get_ip_enable, NULL, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("X_BROADCOM_COM_FirewallEnabled", ctx, "1", get_firewall_enabled, set_firewall_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("IPAddress", ctx, "1", get_ipv4_address, set_ipv4_address, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SubnetMask", ctx, "1", get_ipv4_netmask, set_ipv4_netmask, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("AddressingType", ctx, "1", get_ipv4_addressing_type, set_ipv4_addressing_type, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_ipv6_address_instance(struct dmctx *ctx, char *int_num, char *ip_inst)
{
	IF_MATCH(ctx, DMROOT"IP.Interface.%s.IPv6Address.%s.", int_num, ip_inst) {
		DMOBJECT(DMROOT"IP.Interface.%s.IPv6Address.%s.", ctx, "1", 1, NULL, delete_ipv6, NULL, int_num, ip_inst);
		DMPARAM("Alias", ctx, "1", get_ipv6_alias, set_ipv6_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "0", get_ip_enable, NULL, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("IPAddress", ctx, "1", get_ipv6_address, set_ipv6_address, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Origin", ctx, "1", get_ipv6_addressing_type, set_ipv6_addressing_type, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_ip_ping_diagnostic(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"IP.Diagnostics.IPPingDiagnostics.") {
		DMOBJECT(DMROOT"IP.Diagnostics.IPPingDiagnostics.", ctx, "0", 0, NULL, NULL, NULL);
		DMPARAM("DiagnosticsState", ctx, "1", get_ip_ping_diagnostics_state, set_ip_ping_diagnostics_state, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Interface", ctx, "1", get_ip_ping_interface, set_ip_ping_interface, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Host", ctx, "1", get_ip_ping_host, set_ip_ping_host, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("NumberOfRepetitions", ctx, "1", get_ip_ping_repetition_number, set_ip_ping_repetition_number, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("Timeout", ctx, "1", get_ip_ping_timeout, set_ip_ping_timeout, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("DataBlockSize", ctx, "1", get_ip_ping_block_size, set_ip_ping_block_size, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		//DMPARAM("DSCP", ctx, "1", get_ipping_dscp, set_ipping_dscp, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("SuccessCount", ctx, "0", get_ip_ping_success_count, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("FailureCount", ctx, "0", get_ip_ping_failure_count, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("AverageResponseTime", ctx, "0", get_ip_ping_average_response_time, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MinimumResponseTime", ctx, "0", get_ip_ping_min_response_time, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MaximumResponseTime", ctx, "0", get_ip_ping_max_response_time, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
