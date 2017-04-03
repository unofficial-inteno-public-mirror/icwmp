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
#include "ethernet.h"

struct eth_port_args cur_eth_port_args = {0};
char *wan_ifname = NULL;

/**************************************************************************
* LINKER
***************************************************************************/
int get_linker_val(char *refparam, struct dmctx *dmctx, void *data, char *instance, char **linker) {
	if (cur_eth_port_args.ifname) {
		*linker = cur_eth_port_args.ifname;
		return 0;
	} else {
		*linker = "";
		return 0;
	}
}
/**************************************************************************
* INIT
***************************************************************************/
inline int init_eth_port(struct dmctx *ctx, struct uci_section *s, char *ifname)
{
	struct eth_port_args *args = &cur_eth_port_args;
	ctx->args = (void *)args;
	args->eth_port_sec = s;
	args->ifname = ifname;
	return 0;
}
/**************************************************************************
* SET & GET ALIAS
***************************************************************************/
int get_eth_port_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_eth_port_args.eth_port_sec, "eth_port_alias", value);
	return 0;
}

int set_eth_port_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_eth_port_args.eth_port_sec, "eth_port_alias", value);
			return 0;
	}
	return 0;
}
/**************************************************************************
* GET & SET ETH PARAM
***************************************************************************/
int get_eth_port_enable(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *ifname;

	if (strstr(cur_eth_port_args.ifname, wan_ifname)) {
		ifname = dmstrdup(wan_ifname);
	} else
		ifname = dmstrdup(cur_eth_port_args.ifname);

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "up", -1, NULL, value, NULL);
	dmfree(ifname);
	return 0;
}

int set_eth_port_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *ifname;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (strstr(cur_eth_port_args.ifname, wan_ifname)) {
				ifname = dmstrdup(wan_ifname);
			} else
				ifname = dmstrdup(cur_eth_port_args.ifname);

			if (b) {
				DMCMD("ethctl", 3, ifname, "phy-power", "up"); //TODO wait ubus command
			}
			else {
				DMCMD("ethctl", 3, ifname, "phy-power", "down"); //TODO wait ubus command
			}
			dmfree(ifname);
			return 0;
	}
	return 0;
}

int get_eth_port_status(char *refparam, struct dmctx *ctx, char **value)
{
	bool b;

	get_eth_port_enable(refparam, ctx, value);
	string_to_bool(*value, &b);
	if (b)
		*value = "Up";
	else
		*value = "Down";
	return 0;
}

int get_eth_port_maxbitrate(char *refparam, struct dmctx *ctx, char **value)
{
	char *pch, *spch, *v;

	dmuci_get_value_by_section_string(cur_eth_port_args.eth_port_sec, "speed", value);
	if ((*value)[0] == '\0' || strcmp(*value, "disabled") == 0 )
		return 0;
	else {
		if (strcmp(*value, "auto") == 0)
			*value = "-1";
		else {
			v = dmstrdup(*value);
			pch = strtok_r(v, "FHfh", &spch);
			*value = dmstrdup(pch);
		}
	}
	return 0;
}

int set_eth_port_maxbitrate(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *val = "", *p = "";
	char *duplex;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcasecmp(value, "disabled") == 0 ) {
				dmuci_set_value_by_section(cur_eth_port_args.eth_port_sec, "speed", "disabled");
			} else if (strcasecmp(value, "auto") == 0 || strcmp(value, "-1") == 0) {
				dmuci_set_value_by_section(cur_eth_port_args.eth_port_sec, "speed", "auto");
			} else {
				dmuci_get_value_by_section_string(cur_eth_port_args.eth_port_sec, "speed", &duplex);
				if (strcmp(duplex, "auto") == 0 || strcmp(duplex, "disabled") == 0)
					p = "FDAUTO";
				else {
					p = strchr(duplex, 'F') ? strchr(duplex, 'F') : strchr(duplex, 'H');
				}
				if (p) dmastrcat(&val, value, p);
				dmuci_set_value_by_section(cur_eth_port_args.eth_port_sec, "speed", val);
				dmfree(val);
			}
			return 0;
	}
	return 0;
}

int get_eth_port_name(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_eth_port_args.eth_port_sec, "name", value);
	return 0;
}

int get_eth_port_mac_address(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_eth_port_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "macaddr", -1, NULL, value, NULL);
	return 0;
}

int get_eth_port_duplexmode(char *refparam, struct dmctx *ctx, char **value)
{
	char *tmp, *name;

	dmuci_get_value_by_section_string(cur_eth_port_args.eth_port_sec, "speed", value);
	if (*value[0] == '\0') {
		*value = "";
	} else if (strcmp(*value, "auto") == 0) {
		*value = "Auto";
	} else {
		if (strchr(*value, 'F'))
			*value = "Full";
		else if (strchr(*value, 'H'))
			*value = "Half";
		else
			*value = "";
	}
	return 0;
}

int set_eth_port_duplexmode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *m, *spch, *rate, *val = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcasecmp(value, "auto") == 0) {
				dmuci_set_value_by_section(cur_eth_port_args.eth_port_sec, "speed", "auto");
				return 0;
			}
			dmuci_get_value_by_section_string(cur_eth_port_args.eth_port_sec, "speed", &m);
			m = dmstrdup(m);
			rate = m;
			if (strcmp(rate, "auto") == 0)
				rate = "100";
			else {
				strtok_r(rate, "FHfh", &spch);
			}
			if (strcasecmp(value, "full") == 0)
				dmastrcat(&val, rate, "FD");
			else if (strcasecmp(value, "half") == 0)
				dmastrcat(&val, rate, "HD");
			else {
				dmfree(m);
				return 0;
			}
			dmuci_set_value_by_section(cur_eth_port_args.eth_port_sec, "speed", val);
			dmfree(m);
			dmfree(val);
			return 0;
	}
	return 0;
}

int get_eth_port_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_eth_port_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_bytes", value, NULL);
	return 0;
}

int get_eth_port_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_eth_port_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	return 0;
}

int get_eth_port_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_eth_port_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	return 0;
}

int get_eth_port_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_eth_port_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	return 0;
}

/*************************************************************
 * ENTRY METHOD
/*************************************************************/

DMOBJ tEthernetObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, nextobj, leaf, linker*/
{"Interface", &DMREAD, NULL, NULL, NULL, browseEthIfaceInst, NULL, NULL, tEthernetStatObj, tEthernetParams, get_linker_val},
{0}
};

DMOBJ tEthernetStatObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, nextobj, leaf, linker*/
{"Stats", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tEthernetStatParams, NULL},
{0}
};

DMLEAF tEthernetParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, NOTIFICATION, linker*/
{"Alias", &DMWRITE, DMT_STRING, get_eth_port_alias, set_eth_port_alias, NULL, NULL},
{"Enable", &DMWRITE, DMT_BOOL, get_eth_port_enable, set_eth_port_enable, NULL, NULL},
{"Status", &DMREAD, DMT_STRING, get_eth_port_status, NULL, NULL, NULL},
{"MaxBitRate", &DMWRITE, DMT_STRING, get_eth_port_maxbitrate, set_eth_port_maxbitrate, NULL, NULL},
{"Name", &DMREAD, DMT_STRING, get_eth_port_name, NULL, NULL, NULL},
{"MACAddress", &DMREAD, DMT_STRING, get_eth_port_mac_address, NULL, NULL, NULL},
{"DuplexMode", &DMWRITE, DMT_STRING, get_eth_port_duplexmode, set_eth_port_duplexmode, NULL, NULL},
{0}
};

DMLEAF tEthernetStatParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, NOTIFICATION, linker*/
{"BytesSent", &DMREAD, DMT_UNINT, get_eth_port_stats_tx_bytes, NULL, NULL, NULL},
{"BytesReceived", &DMREAD, DMT_UNINT, get_eth_port_stats_rx_bytes, NULL, NULL, NULL},
{"PacketsSent", &DMREAD, DMT_UNINT, get_eth_port_stats_tx_packets, NULL, NULL, NULL},
{"PacketsReceived", &DMREAD, DMT_UNINT, get_eth_port_stats_rx_packets, NULL, NULL, NULL},
{0}
};

inline int browseEthIfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *int_num = NULL, *int_num_last = NULL, *ifname;
	struct uci_section *ss = NULL;

	dmuci_get_option_value_string("layer2_interface_ethernet", "Wan", "baseifname", &wan_ifname);
	uci_foreach_sections("ports", "ethport", ss) {
		dmuci_get_value_by_section_string(ss, "ifname", &ifname);
		if (strcmp(ifname, wan_ifname) == 0) {
			dmasprintf(&ifname, "%s.1", ifname);
		}
		init_eth_port(dmctx, ss, ifname);
		int_num =  handle_update_instance(1, dmctx, &int_num_last, update_instance_alias, 3, ss, "eth_port_instance", "eth_port_alias");
		DM_LINK_INST_OBJ(dmctx, parent_node, NULL, int_num);

	}
	return 0;
}

