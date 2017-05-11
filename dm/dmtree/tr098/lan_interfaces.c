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

struct ethrnet_ifaces_s ethrnet_ifaces_g = {0};
static inline void laninterface_lookup(char *eths[], int *size);
int browselaninterface_lanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browselaninterface_wlanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
//////////////////////INIT ARGS////////////////////////////////
int init_lan_interface_args(struct linterfargs *args, char *lif, struct uci_section *port_sec)
{
	args->linterf = lif;
	args->port_sec = port_sec;
	return 0;
}

int get_lan_ethernet_interface_number(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmasprintf(value, "%d", ethrnet_ifaces_g.eths_size);// MEM WILL BE FREED IN DMMEMCLEAN
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

int get_lan_wlan_configuration_number(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int cnt = lan_wlan_configuration_number();
	
	dmasprintf(value, "%d", cnt);// MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_eth_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct linterfargs *lifargs = (struct linterfargs *)data;
	
	*value = lifargs->linterf;
	return 0;
}

static inline void laninterface_lookup(char *eths[], int *size)
{
	static char eths_buf[64];
	char *phy_itf;
	char *savepch;
	int n = 0;

	if (*size)
		return;
	db_get_value_string("hw", "board", "ethernetLanPorts", &phy_itf);
	strcpy(eths_buf, phy_itf);
	eths[n] = strtok_r(eths_buf, " ", &savepch);
	while (eths[n] != NULL) {
		eths[++n] = strtok_r(NULL, " ", &savepch);
	}
	*size = n;
}

inline void init_laninterface_lan(struct dmctx *ctx, void *data)
{
	laninterface_lookup(ethrnet_ifaces_g.eths, &(ethrnet_ifaces_g.eths_size));
}

////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_lan_eth_int_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct linterfargs *args = (struct linterfargs *)data;
	dmuci_get_value_by_section_string(args->port_sec, "lanportalias", value);
	return 0;
}

int set_lan_eth_int_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct linterfargs *args = (struct linterfargs *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(args->port_sec, "lanportalias", value);
			return 0;
	}
	return 0;
}

int get_wlan_conf_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *wiface_sec = (struct uci_section *)data;
	dmuci_get_value_by_section_string(wiface_sec, "wifacealias", value);
	return 0;
}

int set_wlan_conf_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct uci_section *wiface_sec = (struct uci_section *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(wiface_sec, "wifacealias", value);
			return 0;
	}
	return 0;
}
/////////////SUB ENTRIES///////////////
bool check_laninterfaces(struct dmctx *dmctx, void *data)
{
	init_laninterface_lan(dmctx, data);
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
	struct linterfargs curr_linterfargs = {0};
	struct linterfargs *args = &curr_linterfargs;
	struct uci_section *s = NULL;

	update_section_list("dmmap","lan_port", NULL, ethrnet_ifaces_g.eths_size, NULL, NULL, NULL, NULL, NULL);
	uci_foreach_sections("dmmap", "lan_port", s) {
		init_lan_interface_args(args, ethrnet_ifaces_g.eths[i++], s);
		ei =  handle_update_instance(1, dmctx, &ei_last, update_instance_alias, 3, s, "lanportinstance", "lanportalias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)args, ei) == DM_STOP)
			break;
	}
	return 0;
}

int browselaninterface_wlanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *s = NULL;
	char *wi, *wi_last = NULL;
	uci_foreach_sections("wireless", "wifi-iface", s) {
		wi =  handle_update_instance(1, dmctx, &wi_last, update_instance_alias, 3, s, "wifaceinstance", "wifacealias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)s, wi) == DM_STOP)
			break;
	}
	return 0;
}
