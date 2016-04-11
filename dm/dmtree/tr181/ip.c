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

inline int init_ipv4_args(struct dmctx *ctx, struct uci_section *s, char *ip_address)
{
	struct ipv4_args *args = &cur_ipv4_args;
	ctx->args = (void *)args;
	args->ipv4_sec = s;
	args->ip_address = ip_address;
	return 0;
}
/*************************************************************
 * GET & SET PARAM
/*************************************************************/
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
	*value = section_name(cur_ip_args.ip_sec);
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
/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_ip(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"IP.") {
		DMOBJECT(DMROOT"IP.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"IP.Interface.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_ip_interface, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_ip_interface(struct dmctx *ctx)
{
	struct uci_section *net_sec = NULL;
	char *ip_int = NULL, *ip_int_last = NULL;
	char *type, *ipv4 ;

	uci_foreach_sections("network", "interface", net_sec) {
		dmuci_get_value_by_section_string(net_sec, "type", &type);
		if (!strcmp(type,"alias"))
			continue;
		init_ip_args(ctx, net_sec);
		ip_int = handle_update_instance(1, ctx, &ip_int_last, update_instance_alias, 3, net_sec, "ip_int_instance", "ip_int_alias");
		SUBENTRY(entry_ip_interface_instance, ctx, ip_int);
		dmuci_get_value_by_section_string(net_sec, "ipaddr", &ipv4);
		SUBENTRY(entry_ipv4_address, ctx, net_sec, ipv4, ip_int);
	}
	return 0;
}

inline int entry_ipv4_address(struct dmctx *ctx, struct uci_section *net_sec, char *ipv4_address, char *ip_int)
{
	struct uci_section *ipv4_sec = NULL;
	char *type, *ifname, *proto, *ipv4, *ipv4_inst = NULL, *ipv4_inst_last = NULL ;

	if(ipv4_address[0] != '\0') {
		init_ipv4_args(ctx, net_sec, ipv4_address);
		ipv4_inst = handle_update_instance(2, ctx, &ipv4_inst_last, update_instance_alias, 3, net_sec, "ipv4_instance", "ipv4_alias");
		SUBENTRY(entry_ipv4_address_instance, ctx, ip_int, ipv4_inst);
	}
	dmasprintf(&ifname, "br-%s", section_name(net_sec));
	uci_foreach_option_eq("network", "interface", "ifname", ifname, ipv4_sec) {
		dmuci_get_value_by_section_string(net_sec, "ipaddr", &ipv4);
		if(ipv4[0] != '\0') {
			init_ipv4_args(ctx, ipv4_sec, ipv4);
			ipv4_inst = handle_update_instance(2, ctx, &ipv4_inst_last, update_instance_alias, 3, ipv4_sec, "ipv4_instance", "ipv4_alias");
			SUBENTRY(entry_ipv4_address_instance, ctx, ip_int, ipv4_inst);
		}
	}
	return 0;
}
inline int entry_ip_interface_instance(struct dmctx *ctx, char *int_num)
{
	IF_MATCH(ctx, DMROOT"IP.Interface.%s.", int_num) {
		DMOBJECT(DMROOT"IP.Interface.%s.", ctx, "0", 1, NULL, NULL, NULL, int_num);
		DMPARAM("Alias", ctx, "1", get_ip_int_alias, set_ip_int_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_ip_interface_enable, set_ip_interface_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "0", get_ip_interface_name, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LowerLayers", ctx, "0", get_empty, NULL, NULL, 0, 1, UNDEF, NULL);//TODO
		DMOBJECT(DMROOT"IP.Interface.%s.IPv4Address.", ctx, "0", 1, NULL, NULL, NULL, int_num);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_ipv4_address_instance(struct dmctx *ctx, char *int_num, char *ip_inst)
{
	IF_MATCH(ctx, DMROOT"IP.Interface.%s.IPv4Address.%s.", int_num, ip_inst) {
		DMOBJECT(DMROOT"IP.Interface.%s.IPv4Address.%s.", ctx, "0", 1, NULL, NULL, NULL, int_num, ip_inst);
		DMPARAM("Alias", ctx, "1", get_ipv4_alias, set_ipv4_alias, NULL, 0, 1, UNDEF, NULL);
		//DMPARAM("Enable", ctx, "1", get_empty, set_interface_enable_ipinterface, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("X_BROADCOM_COM_FirewallEnabled", ctx, "1", get_firewall_enabled, set_firewall_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("IPAddress", ctx, "1", get_ipv4_address, set_ipv4_address, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SubnetMask", ctx, "1", get_ipv4_netmask, set_ipv4_netmask, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("AddressingType", ctx, "0", get_ipv4_addressing_type, NULL, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

