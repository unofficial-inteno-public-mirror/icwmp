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
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "lan_interfaces.h"

struct linterfargs cur_linterfargs = {0};
struct wifaceargs cur_wifaceargs = {0};
static inline void laninterface_lookup(char *eths[], int *size);
inline int browselaninterface_lanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int browselaninterface_wlanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
//////////////////////INIT ARGS////////////////////////////////
static inline int init_lan_interface_args(char *lif, struct uci_section *port_sec)
{
	cur_linterfargs.linterf = lif;
	cur_linterfargs.port_sec = port_sec;
	return 0;
}

static inline int init_wifi_iface_args(struct uci_section *s)
{
	cur_wifaceargs.wiface_sec = s;
	return 0;
}

int get_lan_ethernet_interface_number(char *refparam, struct dmctx *ctx, char **value)
{
	struct linterfargs *lifargs = &cur_linterfargs;
	dmasprintf(value, "%d", lifargs->eths_size);// MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

static inline int lan_wlan_configuration_number()
{
	int cnt = 0;
	struct uci_section *s;
	char *pch, *phy_itf, *phy_itf_local;

	uci_foreach_sections("wireless", "wifi-iface", s) {
		cnt++;
	}
	return cnt;
}

int get_lan_wlan_configuration_number(char *refparam, struct dmctx *ctx, char **value)
{
	int cnt = lan_wlan_configuration_number();
	
	dmasprintf(value, "%d", cnt);// MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_eth_name(char *refparam, struct dmctx *ctx, char **value)
{
	struct linterfargs *lifargs = &cur_linterfargs;
	
	*value = lifargs->linterf;
	return 0;
}

static inline void laninterface_lookup(char *eths[], int *size)
{
	static char eths_buf[64];
	char *phy_itf;
	char *savepch;
	int n = 0;

	db_get_value_string("hw", "board", "ethernetLanPorts", &phy_itf);
	strcpy(eths_buf, phy_itf);
	eths[n] = strtok_r(eths_buf, " ", &savepch);
	while (eths[n] != NULL) {
		eths[++n] = strtok_r(NULL, " ", &savepch);
	}
	*size = n;
}

inline void init_laninterface_lan(struct dmctx *ctx)
{
	struct linterfargs *args = &cur_linterfargs;
	laninterface_lookup(args->eths, &(args->eths_size));
}

////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_lan_eth_int_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_linterfargs.port_sec, "lanportalias", value);
	return 0;
}

int set_lan_eth_int_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_linterfargs.port_sec, "lanportalias", value);
			return 0;
	}
	return 0;
}

int get_wlan_conf_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifaceargs.wiface_sec, "wifacealias", value);
	return 0;
}

int set_wlan_conf_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifaceargs.wiface_sec, "wifacealias", value);
			return 0;
	}
	return 0;
}
/////////////SUB ENTRIES///////////////
bool check_laninterfaces(struct dmctx *dmctx, void *data)
{
	init_laninterface_lan(dmctx);
	return true;
}

DMLEAF tlaninterface_lanParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_lan_eth_int_alias, set_lan_eth_int_alias, NULL, NULL},
{"X_INTENO_COM_EthName", &DMREAD, DMT_STRING, get_eth_name, NULL, NULL, NULL},
{0}
};

DMLEAF tlaninterface_wlanParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_wlan_conf_alias, set_wlan_conf_alias, NULL, NULL},
{0}
};

DMLEAF tLANInterfacesParam[] = {
{"LANEthernetInterfaceNumberOfEntries", &DMREAD, DMT_UNINT, get_lan_ethernet_interface_number, NULL, NULL, NULL},
{"LANWLANConfigurationNumberOfEntries", &DMREAD, DMT_UNINT, get_lan_wlan_configuration_number, NULL, NULL, NULL},
{0}
};

DMOBJ tLANInterfacesObj[] = {
{"LANEthernetInterfaceConfig", &DMREAD, NULL, NULL, NULL, browselaninterface_lanInst, NULL, NULL, NULL, tlaninterface_lanParam, NULL},
{"WLANConfiguration", &DMREAD, NULL, NULL, NULL, browselaninterface_wlanInst, NULL, NULL, NULL, tlaninterface_wlanParam, NULL},
{0}
};

int browselaninterface_lanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *ei, *ei_last = NULL;
	int i = 0;
	struct linterfargs *args = &cur_linterfargs;
	struct uci_section *s = NULL;

	laninterface_lookup(args->eths, &(args->eths_size));
	update_section_list("dmmap","lan_port", NULL, args->eths_size, NULL, NULL, NULL, NULL, NULL);
	uci_foreach_sections("dmmap", "lan_port", s) {
		init_lan_interface_args(args->eths[i++], s);
		ei =  handle_update_instance(1, dmctx, &ei_last, update_instance_alias, 3, s, "lanportinstance", "lanportalias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, ei) == DM_STOP)
			break;
	}
	DM_CLEAN_ARGS(cur_linterfargs);
	return 0;
}

int browselaninterface_wlanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *s = NULL;
	char *wi, *wi_last = NULL;
	uci_foreach_sections("wireless", "wifi-iface", s) {
		init_wifi_iface_args(s);
		wi =  handle_update_instance(1, dmctx, &wi_last, update_instance_alias, 3, s, "wifaceinstance", "wifacealias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, wi) == DM_STOP)
			break;
	}
	DM_CLEAN_ARGS(cur_wifaceargs);
	return 0;
}
