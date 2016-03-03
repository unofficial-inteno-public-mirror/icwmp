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
inline int entry_laninterface_lan(struct dmctx *ctx);
inline int entry_laninterface_wlan(struct dmctx *ctx);
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
	struct linterfargs *lifargs = (struct linterfargs *)ctx->args;
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
	struct linterfargs *lifargs = (struct linterfargs *)ctx->args;
	
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
	ctx->args = (void *)args;
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
inline int entry_laninterface_lan(struct dmctx *ctx)
{
	char *ei, *ei_last = NULL;
	int i = 0;
	struct linterfargs *args = &cur_linterfargs;
	ctx->args = (void *)args;
	struct uci_section *s = NULL;

	laninterface_lookup(args->eths, &(args->eths_size));
	update_section_list("dmmap","lan_port", NULL, args->eths_size, NULL);
	uci_foreach_sections("dmmap", "lan_port", s) {
		init_lan_interface_args(args->eths[i++], s);
		ei =  handle_update_instance(1, ctx, &ei_last, update_instance_alias, 3, s, "lanportinstance", "lanportalias");
		SUBENTRY(entry_laninterface_lan_instance, ctx, ei);
	}
	return 0;
}

inline int entry_laninterface_wlan(struct dmctx *ctx)
{
	struct uci_section *s = NULL;
	char *wi, *wi_last = NULL;
	uci_foreach_sections("wireless", "wifi-iface", s) {
		init_wifi_iface_args(s);
		wi =  handle_update_instance(1, ctx, &wi_last, update_instance_alias, 3, s, "wifaceinstance", "wifacealias");
		SUBENTRY(entry_laninterface_wlan_instance, ctx, wi);
	}
	return 0;
}
////////////////////////////////////////
int entry_method_root_InternetGatewayDevice_LANInterfaces(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"LANInterfaces.") {
		init_laninterface_lan(ctx);
		DMOBJECT(DMROOT"LANInterfaces.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("LANEthernetInterfaceNumberOfEntries", ctx, "0", get_lan_ethernet_interface_number, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("LANWLANConfigurationNumberOfEntries", ctx, "0", get_lan_wlan_configuration_number, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"LANInterfaces.LANEthernetInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"LANInterfaces.WLANConfiguration.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_laninterface_lan, ctx);
		SUBENTRY(entry_laninterface_wlan, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_laninterface_lan_instance(struct dmctx *ctx, char *li)
{
	DMOBJECT(DMROOT"LANInterfaces.LANEthernetInterfaceConfig.%s.", ctx, "0", 1, NULL, NULL, NULL, li);
	DMPARAM("Alias", ctx, "1", get_lan_eth_int_alias, set_lan_eth_int_alias, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("X_INTENO_COM_EthName", ctx, "0", get_eth_name, NULL, NULL, 0, 1, UNDEF, NULL);
	return 0;
}

inline int entry_laninterface_wlan_instance(struct dmctx *ctx, char  *wli)
{
	DMOBJECT(DMROOT"LANInterfaces.WLANConfiguration.%s.", ctx, "0", 1, NULL, NULL, NULL, wli);
	DMPARAM("Alias", ctx, "1", get_wlan_conf_alias, set_wlan_conf_alias, NULL, 0, 1, UNDEF, NULL);
	return 0;
}