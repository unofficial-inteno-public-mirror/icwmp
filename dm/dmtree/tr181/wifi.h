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
#ifndef __WIFI_H
#define __WIFI_H

struct wifi_radio_args
{
	struct uci_section *wifi_radio_sec;
};

struct wifi_ssid_args
{
	struct uci_section *wifi_ssid_sec;
	char *ifname;
	char *linker;
};
struct wifi_acp_args
{
	struct uci_section *wifi_acp_sec;
	char *ifname;
};

extern DMOBJ tWifiObj[];
extern DMOBJ tWifiRadioStatsObj[];
extern DMOBJ tAcessPointSecurityObj[];
extern DMOBJ tWifiSsidStatsObj[];
extern DMLEAF tWifiAcessPointParams[];
extern DMLEAF tWifiSsidParams[];
extern DMLEAF tWifiRadioParams[];
extern DMLEAF tWifiAcessPointSecurityParams[];
extern DMLEAF tWifiRadioStatsParams[];
extern DMLEAF tWifiSsidStatsParams[];
inline int browseWifiSsidInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int browseWifiAccessPointInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int browseWifiRadioInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
#endif
