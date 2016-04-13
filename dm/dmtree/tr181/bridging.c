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
#include <ctype.h>
#include <uci.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "bridging.h"

struct bridging_args cur_bridging_args = {0};
struct bridging_port_args cur_bridging_port_args = {0};
struct bridging_vlan_args cur_bridging_vlan_args = {0};
char *wan_baseifname = NULL;

/**************************************************************************
* INIT
***************************************************************************/
inline int init_bridging_args(struct dmctx *ctx, struct uci_section *s, char *key)
{
	struct bridging_args *args = &cur_bridging_args;
	ctx->args = (void *)args;
	args->bridge_sec = s;
	args->br_key = key;
	return 0;
}

inline int init_bridging_port_args(struct dmctx *ctx, struct uci_section *s, bool vlan, char *ifname)
{
	struct bridging_port_args *args = &cur_bridging_port_args;
	ctx->args = (void *)args;
	args->bridge_port_sec = s;
	args->vlan = vlan;
	args->ifname = ifname;
	return 0;
}

inline int init_bridging_vlan_args(struct dmctx *ctx, struct uci_section *s, char *vlan_port, char *br_inst)
{
	struct bridging_vlan_args *args = &cur_bridging_vlan_args;
	ctx->args = (void *)args;
	args->bridge_vlan_sec = s;
	args->vlan_port = vlan_port;
	args->br_inst = br_inst;
	return 0;
}

/**************************************************************************
* INSTANCE MG
***************************************************************************/
int check_ifname_exist_in_br_ifname_list(char *ifname)
{
	char *br_ifname_list, *br_ifname_dup, *pch, *spch;
	struct uci_section *s;
	uci_foreach_option_eq("network", "interface", "type", "bridge", s) {
		dmuci_get_value_by_section_string(s, "ifname", &br_ifname_list);
		if(br_ifname_list[0] == '\0')
			return 0;
		br_ifname_dup = dmstrdup(br_ifname_list);
		for (pch = strtok_r(br_ifname_dup, " ", &spch); pch != NULL; pch = strtok_r(NULL, " ", &spch)) {
			if (strcmp(pch, ifname) == 0)
				return 1;
		}
	}
	return 0;
}

int get_br_port_last_inst(char *br_key)
{
	char *tmp;
	int r = 0, dr = 0, ds = 0, max;
	struct uci_section *s;
	int buf[BUF_SIZE] = {0};

	uci_foreach_option_eq("dmmap", "bridge_port", "bridge_key", br_key, s) {
		dmuci_get_value_by_section_string(s, "bridge_port_instance", &tmp);
		if (tmp[0] == '\0')
			break;
		buf[0] = atoi(tmp);
	}
	uci_foreach_option_eq("ports", "ethport", "bridge_key", br_key, s) {
		dmuci_get_value_by_section_string(s, "bridge_port_instance", &tmp);
		if (tmp[0] == '\0')
			break;
		buf[1] = atoi(tmp);
	}
	uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "bridge_key", br_key, s) {
		dmuci_get_value_by_section_string(s, "bridge_port_instance", &tmp);
		if (tmp[0] == '\0')
			break;
		buf[2] = atoi(tmp);
	}
	uci_foreach_option_eq("layer2_interface_vdsl", "vdsl_interface", "bridge_key", br_key, s) {
		dmuci_get_value_by_section_string(s, "bridge_port_instance", &tmp);
		if (tmp[0] == '\0')
			break;
		buf[3] = atoi(tmp);
	}
	uci_foreach_option_eq("layer2_interface_ethernet", "ethernet_interface", "bridge_key", br_key, s) {
		dmuci_get_value_by_section_string(s, "bridge_port_instance", &tmp);
		if (tmp[0] == '\0')
			break;
		buf[4] = atoi(tmp);
	}
	uci_foreach_option_eq("wireless", "wifi-iface", "bridge_key", br_key, s) {
		dmuci_get_value_by_section_string(s, "bridge_port_instance", &tmp);
		if (tmp[0] == '\0')
			break;
		buf[5] = atoi(tmp);
	}
	uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "bridge_key", br_key, s) {
		dmuci_get_value_by_section_string(s, "bridge_port_instance", &tmp);
		if (tmp[0] == '\0')
			break;
		buf[6] = atoi(tmp);
	}
	max =  max_array(buf, BUF_SIZE);
	return max;
}

char *br_port_update_instance_alias(int action, char **last_inst, void *argv[])
{
	char *instance, *alias;
	char buf[8] = {0};

	struct uci_section *s = (struct uci_section *) argv[0];
	char *inst_opt = (char *) argv[1];
	char *alias_opt = (char *) argv[2];
	bool *find_max = (bool *) argv[3];
	char *br_key = (char *) argv[4];

	dmuci_get_value_by_section_string(s, inst_opt, &instance);
	if (instance[0] == '\0') {
		if (*find_max) {
			int m = get_br_port_last_inst(br_key);
			sprintf(buf, "%d", m+1);
			*find_max = false;
		}
		else if (last_inst == NULL) {
			sprintf(buf, "%d", 1);
		}
		else {
			sprintf(buf, "%d", atoi(*last_inst)+1);
		}
		instance = dmuci_set_value_by_section(s, inst_opt, buf);
	}
	*last_inst = instance;
	if (action == INSTANCE_MODE_ALIAS) {
		dmuci_get_value_by_section_string(s, alias_opt, &alias);
		if (alias[0] == '\0') {
			sprintf(buf, "cpe-%s", instance);
			alias = dmuci_set_value_by_section(s, alias_opt, buf);
		}
		sprintf(buf, "[%s]", alias);
		instance = dmstrdup(buf);
	}
	return instance;
}

int reset_br_port(char *br_key)
{
	struct uci_section *s, *prev_s = NULL;
	uci_foreach_option_eq("ports", "ethport", "bridge_key", br_key, s) {
		dmuci_set_value_by_section(s, "bridge_port_instance", "");
		dmuci_set_value_by_section(s, "bridge_key", "");
		dmuci_set_value_by_section(s, "penable", "0");
	}
	uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "bridge_key", br_key, s) {
		dmuci_set_value_by_section(s, "bridge_port_instance", "");
		dmuci_set_value_by_section(s, "bridge_key", "");
		dmuci_set_value_by_section(s, "penable", "0");
	}
	uci_foreach_option_eq("layer2_interface_vdsl", "vdsl_interface", "bridge_key", br_key, s) {
		dmuci_set_value_by_section(s, "bridge_port_instance", "");
		dmuci_set_value_by_section(s, "bridge_key", "");
		dmuci_set_value_by_section(s, "penable", "0");
	}
	uci_foreach_option_eq("layer2_interface_ethernet", "ethernet_interface", "bridge_key", br_key, s) {
		dmuci_set_value_by_section(s, "bridge_port_instance", "");
		dmuci_set_value_by_section(s, "bridge_key", "");
		dmuci_set_value_by_section(s, "penable", "0");
	}
	uci_foreach_option_eq("wireless", "wifi-iface", "bridge_key", br_key, s) {
		dmuci_set_value_by_section(s, "bridge_port_instance", "");
		dmuci_set_value_by_section(s, "bridge_key", "");
		dmuci_set_value_by_section(s, "penable", "0");
	}
	uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "bridge_key", br_key, s) {
		if (prev_s)
			dmuci_delete_by_section(prev_s, NULL, NULL);
		prev_s = s;
	}
	if (prev_s)
		dmuci_delete_by_section(prev_s, NULL, NULL);
	return 0;
}

int check_ifname_is_not_lan_port(char *ifname)
{
	struct uci_section *s;
	if (!strstr(ifname, wan_baseifname)) {
		uci_foreach_option_eq("ports", "ethport", "ifname", ifname, s) {
			return 0;
		}
	}
	return 1;
}

int update_port_parameters(char *linker, char *br_key, char *br_pt_inst, char *mg_port)
{
	struct uci_section *s;
	if (check_ifname_is_vlan(linker)) {
		uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", linker, s) {
			dmuci_set_value_by_section(s, "bridge_key", br_key);
			dmuci_set_value_by_section(s, "bridge_port_instance", br_pt_inst);
			dmuci_set_value_by_section(s, "mg_port", mg_port);
			break;
		}
	} else if (strncmp(linker, "ptm", 3) == 0) {
		uci_foreach_option_eq("layer2_interface_vdsl", "vdsl_interface", "ifname", linker, s) {
			dmuci_set_value_by_section(s, "bridge_key", br_key);
			dmuci_set_value_by_section(s, "bridge_port_instance", br_pt_inst);
			dmuci_set_value_by_section(s, "mg_port", mg_port);
			break;
		}
	} else if (strncmp(linker, "atm", 3) == 0) {
		uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", linker, s) {
			dmuci_set_value_by_section(s, "bridge_key", br_key);
			dmuci_set_value_by_section(s, "bridge_port_instance", br_pt_inst);
			dmuci_set_value_by_section(s, "mg_port", mg_port);
			break;
		}
	} else if (strncmp(linker, "wl", 2) == 0) {
		uci_foreach_option_eq("wireless", "wifi-iface", "ifname", linker, s) {
			dmuci_set_value_by_section(s, "bridge_key", br_key);
			dmuci_set_value_by_section(s, "bridge_port_instance", br_pt_inst);
			dmuci_set_value_by_section(s, "mg_port", mg_port);
			break;
		}
	} else {
		uci_foreach_option_eq("ports", "ethport", "ifname", linker, s) {
			dmuci_set_value_by_section(s, "bridge_key", br_key);
			dmuci_set_value_by_section(s, "bridge_port_instance", br_pt_inst);
			dmuci_set_value_by_section(s, "mg_port", mg_port);
			break;
		}
		uci_foreach_option_eq("layer2_interface_ethernet", "ethernet_interface", "ifname", linker, s) {
			dmuci_set_value_by_section(s, "bridge_key", br_key);
			dmuci_set_value_by_section(s, "bridge_port_instance", br_pt_inst);
			dmuci_set_value_by_section(s, "mg_port", mg_port);
			break;
		}
	}
	return 0;
}
/**************************************************************************
*SET & GET BRIDGING PARAMETERS
***************************************************************************/

int get_br_enable(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *br_name;
	dmastrcat(&br_name, "br-", section_name(cur_bridging_args.bridge_sec));
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", br_name}}, 1, &res);
	DM_ASSERT(res, *value = "false");
	json_select(res, "up", 0, NULL, value, NULL);
	return 0;
}

int get_br_status(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *br_name;
	dmastrcat(&br_name, "br-", section_name(cur_bridging_args.bridge_sec));
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", br_name}}, 1, &res);
	DM_ASSERT(res, *value = "Disabled");
	json_select(res, "up", 0, NULL, value, NULL);
	if(strcmp(*value,"true") == 0)
		*value = "Enabled";
	return 0;
}

int set_br_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *br_name;
	int error = string_to_bool(value, &b);
	switch (action) {
		case VALUECHECK:
			if (error)
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmastrcat(&br_name, "br-", section_name(cur_bridging_args.bridge_sec));
			if (b) {
				dmubus_call_set("network.interface", "up", UBUS_ARGS{{"interface", br_name}}, 1);
			}
			else {
				dmubus_call_set("network.interface", "down", UBUS_ARGS{{"interface", br_name}}, 1);
			}
			return 0;
	}
	return 0;
}

int get_br_associated_interfaces(char *refparam, struct dmctx *ctx, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "ifname", value);
	return 0;
}

int set_br_associated_interfaces(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", value);
			return 0;
	}
	return 0;
}

int get_br_port_status(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "ifname", value);
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", *value}}, 1, &res);
	DM_ASSERT(res, *value = "Down");
	json_select(res, "up", 0, NULL, value, NULL);
	if (strcmp(*value,"true") == 0)
		*value = "Up";
	return 0;
}

int get_br_port_name(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "name", value);
	return 0;
}

int get_br_port_management(char *refparam,struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "mg_port", value);
	return 0;
}

int get_br_port_enable(char *refparam,struct dmctx *ctx, char **value)
{
	*value = "true";
	return 0;
}


/**************************************************************************
* GET STAT
***************************************************************************/
int get_br_port_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "ifname", value);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", *value}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_bytes", value, NULL);
	return 0;
}

int get_br_port_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;

	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "ifname", value);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", *value}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	return 0;
}

int get_br_port_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;

	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "ifname", value);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", *value}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	return 0;
}

int get_br_port_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;

	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "ifname", value);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", *value}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	return 0;
}

int get_br_vlan_enable(char *refparam,struct dmctx *ctx, char **value)
{
	*value = "false";
	char *tmp;
	dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "penable", &tmp);
	if (tmp[0] == '0' || tmp[0] == '\0')
		*value = "false";
	else if (tmp[0] == '1')
		*value = "true";
	return 0;
}

int set_br_vlan_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *vlan_ifname, *br_ifname, *vid, *p;
	char new_ifname[256];
	char pr_linker[32];
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "penable", "1");
			else {
				dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "penable", "0");
				dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "bridge_port_instance", "");
				dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "br_port_linker", "");
			}
			dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "ifname", &vlan_ifname);
			if (vlan_ifname[0] == '\0')
				return 0;
			dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "ifname", &br_ifname);
			if (b) {
				//add vlan ifname to br ifname list
				p = new_ifname;
				if (br_ifname[0] != '\0') {
					dmstrappendstr(p, br_ifname);
					dmstrappendchr(p, ' ');
				}
				dmstrappendstr(p, vlan_ifname);
				dmstrappendend(p);
				dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", new_ifname);
				sprintf(pr_linker,"%s.%s", section_name(cur_bridging_vlan_args.bridge_vlan_sec), vlan_ifname);
				dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "br_port_linker", pr_linker);
			} else {
				//delete vlan ifname from br ifname list
				dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "vlan8021q", &vid);
				remove_interface_from_ifname(vlan_ifname, br_ifname, new_ifname);
				//remove_vid_interfaces_from_ifname(vid, br_ifname, new_ifname);
				dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", new_ifname);
			}
			return 0;
	}
	return 0;
}

int get_br_vlan_name(char *refparam,struct dmctx *ctx, char **value)
{
	*value = "0";
	dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "name", value);
	return 0;
}

int set_br_vlan_name(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "name", value);
			return 0;
	}
	return 0;
}

int get_br_vlan_vid(char *refparam,struct dmctx *ctx, char **value)
{
	*value = "0";
	dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "vlan8021q", value);
	return 0;
}

int set_br_vlan_vid(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "vlan8021q", value);
			return 0;
	}
	return 0;
}

/*************************************************************
 * GET SET ALIAS
/*************************************************************/

int get_br_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "bridge_alias", value);
	return 0;
}

int set_br_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "bridge_alias", value);
			return 0;
	}
	return 0;
}

int get_br_port_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "bridge_port_alias", value);
	return 0;
}

int set_br_port_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_bridging_port_args.bridge_port_sec, "bridge_port_alias", value);
			return 0;
	}
	return 0;
}

int get_br_vlan_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "bridge_vlan_alias", value);
	return 0;
}

int set_br_vlan_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "bridge_vlan_alias", value);
			return 0;
	}
	return 0;
}
/*************************************************************
 * ADD DELETE OBJECT
/*************************************************************/

int add_bridge(struct dmctx *ctx, char **instance)
{
	char *last_inst;
	char bridge_name[16], ib[8];
	char *p = bridge_name;

	last_inst = get_last_instance_lev2("network", "interface", "bridge_instance", "type", "bridge");
	sprintf(ib, "%d", last_inst ? atoi(last_inst)+1 : 1);
	dmstrappendstr(p, "bridge_0_");
	dmstrappendstr(p, ib);
	dmstrappendend(p);
	dmuci_set_value("network", bridge_name, "", "interface");
	dmuci_set_value("network", bridge_name, "type", "bridge");
	dmuci_set_value("network", bridge_name, "proto", "dhcp");
	*instance = dmuci_set_value("network", bridge_name, "bridge_instance", ib);
	update_section_list("dmmap","bridge_port", "bridge_key", 1, ib, "mg_port", "true", "bridge_port_instance", "1");
	return 0;
}

int delete_bridge(struct dmctx *ctx)
{
	struct uci_section *s = NULL, *prev_s = NULL;

	dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "type", "");
	dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "bridge_instance", "");
	dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ip_int_instance", "");
	dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ipv4_instance", "");
	uci_foreach_option_eq("dmmap", "bridge_port", "bridge_key", cur_bridging_args.br_key, s) {
		if (prev_s)
			dmuci_delete_by_section(prev_s, NULL, NULL);
		prev_s = s;
	}
	if (prev_s)
		dmuci_delete_by_section(prev_s, NULL, NULL);
	reset_br_port( cur_bridging_args.br_key);
	dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", "");
	return 0;
}

int delete_bridge_all(struct dmctx *ctx)
{
	struct uci_section *bridge_s, *vlan_s, *prev_s = NULL;
	char *bridgekey = NULL;

	uci_foreach_option_eq("network", "interface", "type", "bridge", bridge_s) {
		dmuci_set_value_by_section(bridge_s, "type", "");
		dmuci_set_value_by_section(bridge_s, "bridge_instance", "");
	}
	uci_foreach_sections("layer2_interface_vlan", "vlan_interface", vlan_s) {
		if(prev_s != NULL && bridgekey[0] != '\0')
			dmuci_delete_by_section(prev_s, NULL, NULL);
		prev_s = vlan_s;
		dmuci_get_value_by_section_string(vlan_s, "bridge_key", &bridgekey);
	}
	if(prev_s != NULL && bridgekey[0] != '\0')
		dmuci_delete_by_section(prev_s, NULL, NULL);
	return 0;
}

int add_br_vlan(struct dmctx *ctx, char **instance)
{
	char *value, *last_instance, *ifname;
	struct uci_section *vlan_s;
	char buf[16];
	char *v_name = buf;

	last_instance = get_last_instance_lev2("layer2_interface_vlan", "vlan_interface", "bridge_vlan_instance", "bridge_key", cur_bridging_args.br_key);
	dmuci_add_section("layer2_interface_vlan", "vlan_interface", &vlan_s, &value);
	dmuci_set_value_by_section(vlan_s, "bridge_key", cur_bridging_args.br_key);
	*instance = update_instance(vlan_s, last_instance, "bridge_vlan_instance");
	dmstrappendstr(v_name, "vlan_");
	dmstrappendstr(v_name, cur_bridging_args.br_key);
	dmstrappendchr(v_name, '.');
	dmstrappendstr(v_name, *instance);
	dmstrappendend(v_name);
	dmuci_set_value_by_section(vlan_s, "name", v_name);
	dmuci_set_value_by_section(vlan_s, "penable", "0");
	return 0;
}

int delete_br_vlan(struct dmctx *ctx)
{
	char *vid, *ifname;
	char new_ifname[128];

	dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "ifname", &ifname);
	dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "vlan8021q", &vid);
	if(ifname[0] != '\0' && vid[0] != '\0'){
		remove_vid_interfaces_from_ifname(vid, ifname, new_ifname);
		dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", new_ifname);
	}
	dmuci_delete_by_section(cur_bridging_vlan_args.bridge_vlan_sec, NULL, NULL);
	return 0;
}

int delete_br_vlan_all(struct dmctx *ctx)
{
	char *vid, *ifname;
	struct uci_section *vlan_s, *prev_s = NULL ;
	char new_ifname[128];

	uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "bridge_key", cur_bridging_args.br_key, vlan_s) {
		dmuci_get_value_by_section_string(vlan_s, "vlan8021q", &vid);
		dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "ifname", &ifname);
		if(ifname[0] != '\0' && vid[0] != '\0'){
			remove_vid_interfaces_from_ifname(vid, ifname, new_ifname);
			dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", new_ifname);
		}
		if (prev_s != NULL)
			dmuci_delete_by_section(prev_s, NULL, NULL);
		prev_s = vlan_s;
	}
	if (prev_s != NULL)
		dmuci_delete_by_section(prev_s, NULL, NULL);
	return 0;
}

int add_br_port(struct dmctx *ctx, char **instance)
{
	char *value;
	struct uci_section *br_port_s;
	char buf[16];

	int m = get_br_port_last_inst(cur_bridging_args.br_key);
	dmasprintf(instance, "%d", m+1);
	dmuci_add_section("dmmap", "bridge_port", &br_port_s, &value);
	dmuci_set_value_by_section(br_port_s, "bridge_key", cur_bridging_args.br_key);
	dmuci_set_value_by_section(br_port_s, "bridge_port_instance", *instance);
	dmuci_set_value_by_section(br_port_s, "mg_port", "false");
	return 0;
}

int delete_br_port(struct dmctx *ctx)
{
	char *iface, *ifname, *linker;
	char new_ifname[128];
	struct uci_section *vlan_s;

	dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "ifname", &ifname);
	if(ifname[0] != '\0'){
		remove_interface_from_ifname(cur_bridging_port_args.ifname, ifname, new_ifname);
		dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", new_ifname);
		dmuci_set_value_by_section(cur_bridging_port_args.bridge_port_sec, "bridge_port_instance", "");
		dmuci_set_value_by_section(cur_bridging_port_args.bridge_port_sec, "bridge_key", "");
		dmuci_set_value_by_section(cur_bridging_port_args.bridge_port_sec, "penable", "0");
		return 0;
	}
	dmasprintf(&linker, "%s.%s", section_name(cur_bridging_port_args.bridge_port_sec), cur_bridging_port_args.ifname);
	uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "br_port_linker", linker, vlan_s) {
		dmuci_set_value_by_section(vlan_s, "br_port_linker", "");
	}
	dmuci_delete_by_section(cur_bridging_port_args.bridge_port_sec, NULL, NULL);//del port from dmmap
	dmfree(linker);
	return 0;
}


int delete_br_port_all(struct dmctx *ctx)
{
	struct uci_section *s = NULL, *prev_s = NULL;
	uci_foreach_option_eq("dmmap", "bridge_port", "bridge_key", cur_bridging_args.br_key, s) {
		if (prev_s)
			dmuci_delete_by_section(prev_s, NULL, NULL);
		prev_s = s;
	}
	if (prev_s)
		dmuci_delete_by_section(prev_s, NULL, NULL);
	reset_br_port(cur_bridging_args.br_key);
	dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", ""); // TO CHECK
	return 0;
}

/*************************************************************
 * LOWER LAYER
/*************************************************************/
int get_port_lower_layer(char *refparam, struct dmctx *ctx, char **value)
{
	char *linker= "";
	char *mg_port;
	char buf[16];

	dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "mg_port", &mg_port);
	if (strcmp(mg_port, "true") ==  0) {
		*value = "";
		return 0;
	} else {
		dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "ifname", &linker);
		if(cur_bridging_port_args.vlan) {
			strncpy(buf, linker, 5);
			buf[5] = '\0';
			strcat(buf, "1");
			linker = buf;
		}
	}
	adm_entry_get_linker_param(DMROOT"Ethernet.Interface.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"WiFi.SSID.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"ATM.Link.", linker, value);
	if (*value == NULL)
		adm_entry_get_linker_param(DMROOT"PTM.Link.", linker, value);

	if (*value == NULL)
		*value = "";
	return 0;

}

int set_port_lower_layer(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *linker, *iface, *ifname, *p, *br_key, *br_pt_inst = "", *mg_port = "false", *br_port_ifname, *vid = NULL;
	char new_ifname[256];
	char tmp[16];
	char pr_linker[32];
	struct uci_section *s;
	switch (action) {
		case VALUECHECK:
			adm_entry_get_linker_value(value, &linker);
			if (linker && check_ifname_exist_in_br_ifname_list(linker))
				return FAULT_9001;
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(value, &linker);
			 //check ifname(linker) doesn't exit in bridges
			if (linker && !check_ifname_exist_in_br_ifname_list(linker)) {
				//save param of current port and copy it to new port
				dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "bridge_key", &br_key);
				dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "bridge_port_instance", &br_pt_inst);
				dmuci_get_value_by_section_string(cur_bridging_port_args.bridge_port_sec, "mg_port", &mg_port);
				//remove old port (ifname) from bridge
				if (cur_bridging_port_args.ifname[0] != 0 && strcmp(cur_bridging_port_args.ifname, linker) != 0) {
					delete_br_port(ctx);
				}
				// check if the current port is already linked with VLAN
				sprintf(pr_linker,"%s.%s", section_name(cur_bridging_port_args.bridge_port_sec), cur_bridging_port_args.ifname);
				uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "br_port_linker", pr_linker, s) {
					dmuci_get_value_by_section_string(s, "vlan8021q", &vid);
					break;
				}
				dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "ifname", &ifname);
				p = new_ifname;
				if (ifname[0] != '\0') {
					dmstrappendstr(p, ifname);
					dmstrappendchr(p, ' ');
				}
				if(vid && check_ifname_is_not_lan_port(linker) && !strstr (linker, "wl")) {
					strncpy(tmp, linker, 5);
					tmp[5] = '\0';
					strcat(tmp, vid);
					linker = tmp;
					dmstrappendstr(p, tmp);
					dmstrappendend(p);
					uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "br_port_linker", pr_linker, s) {
						sprintf(pr_linker,"%s.%s", section_name(s), linker);
						dmuci_set_value_by_section(s, "br_port_linker", pr_linker);
						dmuci_set_value_by_section(s, "ifname", linker);
						dmuci_set_value_by_section(s, "penable", "1");
					}
				} else {
					dmstrappendstr(p, linker);
					dmstrappendend(p);
				}
				dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", new_ifname);
				//remove old br_port param to the new one
				update_port_parameters(linker, br_key, br_pt_inst, mg_port);
				if(cur_bridging_port_args.ifname[0] == '\0')
					dmuci_delete_by_section(cur_bridging_port_args.bridge_port_sec, NULL, NULL);// delete dmmap section after remove br_port_instance to adequate config
			}
	return 0;
	}
}

int get_vlan_port_vlan_ref(char *refparam, struct dmctx *ctx, char **value)
{
	char linker[8];
	char *name;
	dmasprintf(&name,DMROOT"Bridging.Bridge.%s.",cur_bridging_args.br_key);
	sprintf(linker,"vlan%s_%s", cur_bridging_vlan_args.vlan_port, cur_bridging_args.br_key);
	adm_entry_get_linker_param(DMROOT"Bridging.Bridge.", linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
	if (*value == NULL)
		*value = "";
	return 0;
}

int get_vlan_port_port_ref(char *refparam, struct dmctx *ctx, char **value)
{
	char *linker;
	char *name;
	dmasprintf(&name,DMROOT"Bridging.Bridge.%s.", cur_bridging_args.br_key);
	dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "br_port_linker", &linker);
	adm_entry_get_linker_param(name, linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
	if (*value == NULL)
		*value = "";
	dmfree(name);
	return 0;
}

int set_vlan_port_port_ref(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *linker = NULL, *vid, *enable, *vifname, tmp[8], *pch, *p, *br_ifname;
	char new_ifname[16];

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(value, &linker);
			if (!linker) {
				dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "br_port_linker", "");
				set_br_vlan_enable(refparam, ctx, action, "false");
				return 0;
			}
			dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "br_port_linker", linker);
			dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "vlan8021q", &vid);
			//dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "ifname", &vifname);
			pch = strchr(linker, '.') + 1;
			if (pch[0] == '\0') {
				dmfree(linker);
				return 0;
			}
			if (vid[0] == '\0') {
				if (strstr(pch, "atm") || strstr(pch, "ptm") || strstr(pch, wan_baseifname)) {
					strncpy(tmp, pch, 4);
					tmp[4] ='\0';
					dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "baseifname", tmp);
				}
			} else {
				if (strstr(pch, "atm") || strstr(pch, "ptm") || strstr(pch, wan_baseifname)) {
					p = new_ifname;
					strncpy(tmp, pch, 4);
					tmp[4] ='\0';
					dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "baseifname", tmp);
					dmstrappendstr(p,  tmp);
					dmstrappendchr(p, '.');
					dmstrappendstr(p, vid);
					dmstrappendend(p);
					dmuci_set_value_by_section(cur_bridging_vlan_args.bridge_vlan_sec, "ifname", new_ifname);
					dmuci_get_value_by_section_string(cur_bridging_vlan_args.bridge_vlan_sec, "penable", &enable);///TO CHECK
					// add to bridge ifname if enable = 1
					if (enable[0] == '1') {
						vifname = dmstrdup(new_ifname);
						dmuci_get_value_by_section_string(cur_bridging_args.bridge_sec, "ifname", &br_ifname);
						p = new_ifname;
						if (br_ifname[0] != '\0') {
							dmstrappendstr(p, br_ifname);
							dmstrappendchr(p, ' ');
						}
						dmstrappendstr(p, vifname);
						dmstrappendend(p);
						dmuci_set_value_by_section(cur_bridging_args.bridge_sec, "ifname", new_ifname);
						dmfree(vifname);
					}
				}
			}
			dmfree(linker);
			return 0;
	}
	return 0;
}
/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_bridging(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"Bridging.") {
		DMOBJECT(DMROOT"Bridging.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"Bridging.Bridge.", ctx, "1", 0, add_bridge, delete_bridge_all, NULL);
		SUBENTRY(entry_bridging_sub, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_bridging_sub(struct dmctx *ctx)
{
	struct uci_section *br_s = NULL;
	char *br_inst = NULL, *br_inst_last = NULL, *ifname;

	dmuci_get_option_value_string("layer2_interface_ethernet", "Wan", "baseifname", &wan_baseifname);
	uci_foreach_option_eq("network", "interface", "type", "bridge", br_s) {
		br_inst = handle_update_instance(1, ctx, &br_inst_last, update_instance_alias, 3, br_s, "bridge_instance", "bridge_alias");
		init_bridging_args(ctx, br_s, br_inst);
		dmuci_get_value_by_section_string(br_s, "ifname", &ifname);
		SUBENTRY(entry_bridge_instance, ctx, br_inst);
		SUBENTRY(entry_bridge_vlan_instance_sub, ctx, br_inst);
		SUBENTRY(entry_bridge_port_sub, ctx, ifname, br_inst);

	}
	return 0;
}

inline int entry_bridge_port_sub(struct dmctx *ctx, char *ifname, char *idev)
{
	struct uci_section *eth_s = NULL, *atm_s = NULL, *ptm_s = NULL, *wl_s = NULL, *vlan_s = NULL, *w_eth_s = NULL, *m_port = NULL, *new_port = NULL;
	char *port = NULL, *port_last = NULL, *vlan = NULL, *vlan_last = NULL;
	char *ifname_dup, *pch, *spch, *vid;
	bool find_max = true;

	update_section_list("dmmap","bridge_port", "bridge_key", 1, idev, "mg_port", "true", "bridge_port_instance", "1");
	uci_foreach_option_eq("dmmap", "bridge_port", "bridge_key", idev, new_port) {
		init_bridging_port_args(ctx, new_port, false, "");
		port = handle_update_instance(2, ctx, &port_last, update_instance_alias, 3, new_port, "bridge_port_instance", "bridge_port_alias");
		SUBENTRY(entry_bridge_port_instance, ctx, idev, port);
	}
	if (ifname[0] == '\0')
		return 0;
	ifname_dup = dmstrdup(ifname);
	for (pch = strtok_r(ifname_dup, " ", &spch); pch != NULL; pch = strtok_r(NULL, " ", &spch)) {
		uci_foreach_option_eq("ports", "ethport", "ifname", pch, eth_s) {
			dmuci_set_value_by_section(eth_s, "bridge_key", idev);
			dmuci_set_value_by_section(eth_s, "mg_port", "false");
			dmuci_set_value_by_section(eth_s, "penable", "1");
			init_bridging_port_args(ctx, eth_s, false, pch);
			port = handle_update_instance(2, ctx, &port_last, br_port_update_instance_alias, 5, eth_s, "bridge_port_instance", "bridge_port_alias", &find_max, idev);
			SUBENTRY(entry_bridge_port_instance, ctx, idev, port);
			break;
		}
		uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", pch, atm_s) {
			dmuci_set_value_by_section(atm_s, "bridge_key", idev);
			dmuci_set_value_by_section(atm_s, "mg_port", "false");
			dmuci_set_value_by_section(atm_s, "penable", "1");
			init_bridging_port_args(ctx, atm_s, false, pch);
			port = handle_update_instance(2, ctx, &port_last, update_instance_alias, 3, atm_s, "bridge_port_instance", "bridge_port_alias");
			SUBENTRY(entry_bridge_port_instance, ctx, idev, port);
			break;
		}
		uci_foreach_option_eq("layer2_interface_vdsl", "vdsl_interface", "ifname", pch, ptm_s) {
			dmuci_set_value_by_section(ptm_s, "bridge_key", idev);
			dmuci_set_value_by_section(ptm_s, "mg_port", "false");
			dmuci_set_value_by_section(ptm_s, "penable", "1");
			init_bridging_port_args(ctx, ptm_s, false, pch);
			port = handle_update_instance(2, ctx, &port_last, update_instance_alias, 3, ptm_s, "bridge_port_instance", "bridge_port_alias");
			SUBENTRY(entry_bridge_port_instance, ctx, idev, port);
			break;
		}
		uci_foreach_option_eq("layer2_interface_ethernet", "ethernet_interface", "ifname", pch, w_eth_s) {
			dmuci_set_value_by_section(w_eth_s, "bridge_key", idev);
			dmuci_set_value_by_section(w_eth_s, "mg_port", "false");
			dmuci_set_value_by_section(w_eth_s, "penable", "1");
			init_bridging_port_args(ctx, w_eth_s, false, pch);
			port = handle_update_instance(2, ctx, &port_last, update_instance_alias, 3, w_eth_s, "bridge_port_instance", "bridge_port_alias");
			SUBENTRY(entry_bridge_port_instance, ctx, idev, port);
			break;
		}
		uci_foreach_option_eq("wireless", "wifi-iface", "ifname", pch, wl_s) {
			dmuci_set_value_by_section(wl_s, "bridge_key", idev);
			dmuci_set_value_by_section(wl_s, "mg_port", "false");
			dmuci_set_value_by_section(wl_s, "penable", "1");
			init_bridging_port_args(ctx, wl_s, false, pch);
			port = handle_update_instance(2, ctx, &port_last, update_instance_alias, 3, wl_s, "bridge_port_instance", "bridge_port_alias");
			SUBENTRY(entry_bridge_port_instance, ctx, idev, port);
			break;
		}
		uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", pch, vlan_s) {
			dmuci_set_value_by_section(vlan_s, "bridge_key", idev);
			dmuci_set_value_by_section(vlan_s, "mg_port", "false");
			dmuci_set_value_by_section(vlan_s, "penable", "1");
			init_bridging_port_args(ctx, vlan_s, true, pch);
			port =  handle_update_instance(2, ctx, &port_last, update_instance_alias, 3, vlan_s, "bridge_port_instance", "bridge_port_alias");
			SUBENTRY(entry_bridge_port_instance, ctx, idev, port);
			break;
		}
	}
	dmfree(ifname_dup);
	return 0;
}
inline int entry_bridge_vlan_instance_sub(struct dmctx *ctx, char *idev)
{
	struct uci_section *vlan_s = NULL;
	char *vlan = NULL, *vlan_last = NULL;
	char *type, *ipv4 ;

	uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "bridge_key", idev, vlan_s) {
		vlan =  handle_update_instance(2, ctx, &vlan_last, update_instance_alias, 3, vlan_s, "bridge_vlan_instance", "bridge_vlan_alias");
		init_bridging_vlan_args(ctx, vlan_s, vlan, idev);
		SUBENTRY(entry_bridge_vlan_instance, ctx, idev, vlan);
		SUBENTRY(entry_bridge_vlan_port_instance, ctx, idev, vlan);
	}
	return 0;
}

inline int entry_bridge_instance(struct dmctx *ctx, char *dev)
{
	IF_MATCH(ctx, DMROOT"Bridging.Bridge.%s.", dev) {
		DMOBJECT(DMROOT"Bridging.Bridge.%s.", ctx, "1", NULL, NULL, delete_bridge, NULL, dev);
		DMPARAM("Alias", ctx, "1", get_br_alias, set_br_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_br_status, set_br_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_br_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_COM_AssociatedInterfaces", ctx, "1", get_br_associated_interfaces, set_br_associated_interfaces, NULL, 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"Bridging.Bridge.%s.VLAN.", ctx, "1", NULL, add_br_vlan, delete_br_vlan_all, NULL, dev);
		DMOBJECT(DMROOT"Bridging.Bridge.%s.Port.", ctx, "1", NULL, add_br_port, delete_br_port_all, NULL, dev); // TODO ADD DEL OBJ
		DMOBJECT(DMROOT"Bridging.Bridge.%s.VLANPort.", ctx, "0", NULL, NULL, NULL, NULL, dev);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_bridge_port_instance(struct dmctx *ctx, char *br, char *port)
{
	IF_MATCH(ctx, DMROOT"Bridging.Bridge.%s.Port.%s.", br, port) {
		char linker[32];
		sprintf(linker,"%s.%s", section_name(cur_bridging_port_args.bridge_port_sec), cur_bridging_port_args.ifname);
		DMOBJECT(DMROOT"Bridging.Bridge.%s.Port.%s.", ctx, "1", NULL, NULL, delete_br_port, linker, br, port);
		DMPARAM("Alias", ctx, "1", get_br_port_alias, set_br_port_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "0", get_br_port_enable, NULL, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_br_port_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "0", get_br_port_name, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LowerLayers", ctx, "1", get_port_lower_layer, set_port_lower_layer, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ManagementPort", ctx, "0", get_br_port_management, NULL, NULL, 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"Bridging.Bridge.%s.Port.%s.Stats.", ctx, "0", 1, NULL, NULL, NULL, br, port);
		DMPARAM("BytesSent", ctx, "0", get_br_port_stats_tx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("BytesReceived", ctx, "0", get_br_port_stats_rx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsSent", ctx, "0", get_br_port_stats_tx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsReceived", ctx, "0", get_br_port_stats_rx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_bridge_vlan_instance(struct dmctx *ctx, char *br, char *vlan)
{
	IF_MATCH(ctx, DMROOT"Bridging.Bridge.%s.VLAN.%s.", br, vlan) {
		char linker[8];
		sprintf(linker,"vlan%s_%s", vlan, br);
		DMOBJECT(DMROOT"Bridging.Bridge.%s.VLAN.%s.", ctx, "1", NULL, NULL, delete_br_vlan, linker, br, vlan);
		DMPARAM("Alias", ctx, "1", get_br_vlan_alias, set_br_vlan_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_br_vlan_enable, set_br_vlan_enable, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "1", get_br_vlan_name, set_br_vlan_name, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("VLANID", ctx, "1", get_br_vlan_vid, set_br_vlan_vid, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}


inline int entry_bridge_vlan_port_instance(struct dmctx *ctx, char *br, char *vlan_port)
{
	IF_MATCH(ctx, DMROOT"Bridging.Bridge.%s.VLANPort.%s.", br, vlan_port) {
		DMOBJECT(DMROOT"Bridging.Bridge.%s.VLANPort.%s.", ctx, "0", NULL, NULL, NULL, NULL, br, vlan_port);
		DMPARAM("Alias", ctx, "1", get_br_vlan_alias, set_br_vlan_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "0", get_br_vlan_enable, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("VLAN", ctx, "0", get_vlan_port_vlan_ref, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Port", ctx, "1", get_vlan_port_port_ref, set_vlan_port_port_ref, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
