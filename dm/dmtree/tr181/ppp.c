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
inline int browseInterfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

/*************************************************************
 * INIT
/*************************************************************/
inline int init_ppp_args(struct dmctx *ctx, struct uci_section *s)
{
	struct ppp_args *args = &cur_ppp_args;
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
	adm_entry_get_linker_param(ctx, dm_print_path("%s%cATM%cLink%c", DMROOT, dm_delim, dm_delim, dm_delim), linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(ctx, dm_print_path("%s%cPTM%cLink%c", DMROOT, dm_delim, dm_delim, dm_delim), linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(ctx, dm_print_path("%s%cEthernet%cInterface%c", DMROOT, dm_delim, dm_delim), dm_delim, linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(ctx, dm_print_path("%s%cWiFi%cSSID%c", DMROOT, dm_delim, dm_delim, dm_delim), linker, value);
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
			adm_entry_get_linker_value(ctx, value, &linker);
			dmuci_set_value_by_section(cur_ppp_args.ppp_sec, "ifname", linker);
			return 0;
	}
	return 0;
}
/**************************************************************************
* LINKER
***************************************************************************/
int get_linker_ppp_interface(char *refparam, struct dmctx *dmctx, void *data, char *instance, char **linker) {

	if(cur_ppp_args.ppp_sec) {
		dmasprintf(linker,"%s", section_name(cur_ppp_args.ppp_sec));
		return 0;
	}
	*linker = "";
	return 0;
}

DMLEAF tpppInterfaceParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_ppp_alias, set_ppp_alias, NULL, NULL},
{"Enable", &DMWRITE, DMT_BOOL, get_ppp_enable, set_ppp_enable, NULL, NULL},
{"Name", &DMREAD, DMT_STRING, get_ppp_name, NULL, NULL, NULL},
{"LowerLayers", &DMWRITE, DMT_STRING, get_ppp_lower_layer, set_ppp_lower_layer, NULL, NULL},
{"ConnectionStatus", &DMREAD, DMT_STRING, get_ppp_status, NULL, NULL, NULL},
{"Username", &DMWRITE, DMT_STRING, get_ppp_username, set_ppp_username, NULL, NULL},
{"Password", &DMWRITE, DMT_STRING, get_empty, set_ppp_password, NULL, NULL},
{0}
};

DMLEAF tStatsParam[] = {
{"EthernetBytesReceived", &DMREAD, DMT_UNINT, get_ppp_eth_bytes_received, NULL, NULL, NULL},
{"EthernetBytesSent", &DMWRITE, DMT_UNINT, get_ppp_eth_bytes_sent, NULL, NULL, NULL},
{"EthernetPacketsReceived", &DMREAD, DMT_UNINT, get_ppp_eth_pack_received, NULL, NULL, NULL},
{"EthernetPacketsSent", &DMREAD, DMT_UNINT, get_ppp_eth_pack_sent, NULL, NULL, NULL},
{0}
};

DMOBJ tpppInterfaceObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"Stats", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tStatsParam, NULL},
{0}
};

DMOBJ tpppObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"Interface", &DMREAD, NULL, NULL, NULL, browseInterfaceInst, NULL, NULL, tpppInterfaceObj, tpppInterfaceParam, get_linker_ppp_interface},
{0}
};
/*************************************************************
 * ENTRY METHOD
/*************************************************************/
inline int browseInterfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *net_sec = NULL;
	char *ppp_int = NULL, *ppp_int_last = NULL;
	char *proto;

	uci_foreach_sections("network", "interface", net_sec) {
		dmuci_get_value_by_section_string(net_sec, "proto", &proto);
		if (!strstr(proto, "ppp"))
			continue;
		init_ppp_args(dmctx, net_sec);
		ppp_int = handle_update_instance(1, dmctx, &ppp_int_last, update_instance_alias, 3, net_sec, "ppp_int_instance", "ppp_int_alias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, ppp_int) == DM_STOP)
			break;
	}
	DM_CLEAN_ARGS(cur_ppp_args);
	return 0;
}


