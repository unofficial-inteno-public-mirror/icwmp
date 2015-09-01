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

struct linterfargs
{
	char *linterf;
};

struct linterfargs cur_linterfargs = {0};

inline int init_lan_interface_args(struct dmctx *ctx, char *lif)
{
	struct linterfargs *args = &cur_linterfargs;
	ctx->args = (void *)args;
	args->linterf = lif;
	return 0;
}

int get_lan_ethernet_interface_number(char *refparam, struct dmctx *ctx, char **value)
{
	int nbr = 0;
	char *pch, *phy_itf, *phy_itf_local;

	db_get_value_string("hw", "board", "ethernetLanPorts", &phy_itf);
	phy_itf_local = dmstrdup(phy_itf);		
	
	pch = strtok(phy_itf_local," ");
	while (pch != NULL) {
		nbr++;
		pch = strtok(NULL, " ");
	}
	dmfree(phy_itf_local);
	dmasprintf(value, "%d", nbr); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int lan_wlan_configuration_number()
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
	dmasprintf(value, "%d", cnt); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_eth_name(char *refparam, struct dmctx *ctx, char **value)
{
	struct linterfargs *lifargs = (struct linterfargs *)ctx->args;
	
	*value = dmstrdup(lifargs->linterf); // MEM WILL BE FREED IN DMMEMCLEAN	 
	return 0;
}

int entry_method_root_InternetGatewayDevice_LANInterfaces(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"LANInterfaces.") {
		int wi=1;
		int nwi;
		char wli[8];
		char *phy_itf, *phy_itf_local;
		
		DMOBJECT(DMROOT"LANInterfaces.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("LANEthernetInterfaceNumberOfEntries", ctx, "0", get_lan_ethernet_interface_number, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("LANWLANConfigurationNumberOfEntries", ctx, "0", get_lan_wlan_configuration_number, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"LANInterfaces.LANEthernetInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL);
		db_get_value_string("hw", "board", "ethernetLanPorts", &phy_itf);
		//TODO KMD: copy  &phy_itf to a local buf
		phy_itf_local = dmstrdup(phy_itf);
		char *pch = strtok(phy_itf_local," ");
		while (pch != NULL) {
			init_lan_interface_args(ctx, pch);
			SUBENTRY(get_lan_interface, ctx, pch+3);
			pch = strtok(NULL, " ");
		}
		dmfree(phy_itf_local);
		DMOBJECT(DMROOT"LANInterfaces.WLANConfiguration.", ctx, "0", 1, NULL, NULL, NULL);
		nwi = lan_wlan_configuration_number();
		while (wi <= nwi) {
			sprintf(wli, "%d", wi);
			SUBENTRY(get_wlan_interface, ctx, wli);
			wi++;
		}
		return 0;
	}
	return FAULT_9005;
}

int get_lan_interface(struct dmctx *ctx, char *li)
{
	DMOBJECT(DMROOT"LANInterfaces.LANEthernetInterfaceConfig.%s.", ctx, "0", 1, NULL, NULL, NULL, li);
	DMPARAM("X_INTENO_COM_EthName", ctx, "0", get_eth_name, NULL, NULL, 0, 1, UNDEF, NULL);
	return 0;
}

int get_wlan_interface(struct dmctx *ctx, char *wli)
{
	DMOBJECT(DMROOT"LANInterfaces.WLANConfiguration.%s", ctx, "0", 1, NULL, NULL, NULL, wli);
	return 0;
}