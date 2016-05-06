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
#include "ppp.h"

struct ppp_args cur_ppp_args = {0};

/*************************************************************
 * INIT
/*************************************************************/
inline int init_ppp_args(struct dmctx *ctx, struct uci_section *s)
{
	struct ppp_args *args = &cur_ppp_args;
	ctx->args = (void *)args;
	args->ppp_sec = s;
	return 0;
}

/*************************************************************
 * GET SET ALIAS
/*************************************************************/

int get_ppp_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ppp_args.ppp_sec, "ppp_int_alias", value);
	return 0;
}

int set_ppp_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_ppp_args.ppp_sec, "ppp_int_alias", value);
			return 0;
	}
	return 0;
}

/**************************************************************************
* GET & SET PARAMETERS
***************************************************************************/

int get_ppp_enable(char *refparam, struct dmctx *ctx, char **value)
{
	return get_interface_enable_ubus(section_name(cur_ppp_args.ppp_sec), refparam, ctx, value);
}

int set_ppp_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	return set_interface_enable_ubus(section_name(cur_ppp_args.ppp_sec), refparam, ctx, action, value);
}

int get_ppp_name(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup(section_name(cur_ppp_args.ppp_sec));
	return 0;
}

int get_ppp_status(char *refparam, struct dmctx *ctx, char **value)
{
	char *status = NULL;
	char *uptime = NULL;
	char *pending = NULL;
	json_object *res = NULL;
	bool bstatus = false, bpend = false;

	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(cur_ppp_args.ppp_sec)}}, 1, &res);
	DM_ASSERT(res, *value = "");
	if (json_select(res, "up", 0, NULL, &status, NULL) != -1)
	{
		string_to_bool(status, &bstatus);
		if (bstatus) {
			json_select(res, "uptime", 0, NULL, &uptime, NULL);
			json_select(res, "pending", 0, NULL, &pending, NULL);
			string_to_bool(pending, &bpend);
		}
	}
	if (uptime && atoi(uptime) > 0)
		*value = "Connected";
	else if (pending && bpend)
		*value = "Pending Disconnect";
	else
		*value = "Disconnected";
	return 0;
}

int get_ppp_username(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ppp_args.ppp_sec, "username", value);
	return 0;
}

int set_ppp_username(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_ppp_args.ppp_sec, "username", value);
			return 0;
	}
	return 0;
}

int set_ppp_password(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_ppp_args.ppp_sec, "password", value);
			return 0;
	}
	return 0;
}

inline int ubus_get_wan_stats(json_object *res, char **value, char *stat_mod)
{
	char *ifname, *proto;
	dmuci_get_value_by_section_string(cur_ppp_args.ppp_sec, "ifname", &ifname);
	dmuci_get_value_by_section_string(cur_ppp_args.ppp_sec, "proto", &proto);
	if (strcmp(proto, "pppoe") == 0) {
		dmubus_call("network.device", "status", UBUS_ARGS{{"name", ifname}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "statistics", 0, stat_mod, value, NULL);
	}
	return 0;
}

int get_ppp_eth_bytes_received(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "rx_bytes");
	return 0;
}

int get_ppp_eth_bytes_sent(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "tx_bytes");
	return 0;
}

int get_ppp_eth_pack_received(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "rx_packets");
	return 0;
}

int get_ppp_eth_pack_sent(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "tx_packets");
	return 0;
}

int get_ppp_lower_layer(char *refparam, struct dmctx *ctx, char **value)
{
	char *linker;
	dmuci_get_value_by_section_string(cur_ppp_args.ppp_sec, "ifname", &linker);
	adm_entry_get_linker_param(DMROOT"ATM.Link.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"PTM.Link.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"Ethernet.Interface.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"WiFi.SSID.", linker, value);
	if (*value == NULL)
		*value = "";
	return 0;
}

int set_ppp_lower_layer(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *linker;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(value, &linker);
			dmuci_set_value_by_section(cur_ppp_args.ppp_sec, "ifname", linker);
			return 0;
	}
	return 0;
}
/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_ppp(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"PPP.") {
		DMOBJECT(DMROOT"PPP.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"PPP.Interface.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_ppp_interface, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_ppp_interface(struct dmctx *ctx)
{
	struct uci_section *net_sec = NULL;
	char *ppp_int = NULL, *ppp_int_last = NULL;
	char *proto;

	uci_foreach_sections("network", "interface", net_sec) {
		dmuci_get_value_by_section_string(net_sec, "proto", &proto);
		if (!strstr(proto, "ppp"))
			continue;
		init_ppp_args(ctx, net_sec);
		ppp_int = handle_update_instance(1, ctx, &ppp_int_last, update_instance_alias, 3, net_sec, "ppp_int_instance", "ppp_int_alias");
		SUBENTRY(entry_ppp_interface_instance, ctx, ppp_int);
	}
	return 0;
}

inline int entry_ppp_interface_instance(struct dmctx *ctx, char *int_num)
{
	IF_MATCH(ctx, DMROOT"PPP.Interface.%s.", int_num) {
		char linker[32] = "";
		strcat(linker, section_name(cur_ppp_args.ppp_sec));
		DMOBJECT(DMROOT"PPP.Interface.%s.", ctx, "0", 1, NULL, NULL, linker, int_num);
		DMPARAM("Alias", ctx, "1", get_ppp_alias, set_ppp_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_ppp_enable, set_ppp_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "0", get_ppp_name, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LowerLayers", ctx, "1", get_ppp_lower_layer, set_ppp_lower_layer, NULL, 0, 1, UNDEF, NULL);//TODO
		DMPARAM("ConnectionStatus", ctx, "0", get_ppp_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Username", ctx, "1", get_ppp_username, set_ppp_username, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Password", ctx, "1", get_empty, set_ppp_password, NULL, 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"PPP.Interface.%s.Stats.", ctx, "0", 1, NULL, NULL, NULL, int_num);
		DMPARAM("EthernetBytesReceived", ctx, "0", get_ppp_eth_bytes_received, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("EthernetBytesSent", ctx, "0", get_ppp_eth_bytes_sent, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("EthernetPacketsReceived", ctx, "0", get_ppp_eth_pack_received, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("EthernetPacketsSent", ctx, "0", get_ppp_eth_pack_sent,NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
