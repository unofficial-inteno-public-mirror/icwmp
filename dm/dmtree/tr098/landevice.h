/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#ifndef __LAN_DEVICE_H
#define __LAN_DEVICE_H
#include <libubox/blobmsg_json.h>
#include <json-c/json.h>
#define NVRAM_FILE "/proc/nvram/WpaKey"

extern DMLEAF tLanhost_Config_ManagementParam[];
extern DMLEAF tDHCPStaticAddressParam[];
extern DMLEAF tIPInterfaceParam[];
extern DMLEAF tlanethernetinterfaceconfigParam[];
extern DMOBJ tLanhost_Config_ManagementObj[];
extern DMLEAF tWlanConfigurationParam[];
extern DMLEAF tWPSParam[];
extern DMLEAF tWepKeyParam[];
extern DMLEAF tpresharedkeyParam[];
extern DMLEAF tassociateddeviceParam[];
extern DMOBJ tWlanConfigurationObj[];
extern DMOBJ tLANDeviceinstObj[];
extern DMOBJ tLANDeviceObj[];
extern DMOBJ tlanethernetinterfaceconfigObj[];
extern DMLEAF tlanethernetinterfaceStatsParam[];
extern DMLEAF tLANDeviceParam[];
extern DMLEAF tlandevice_hostParam[];
extern DMOBJ tlandevice_hostObj[];
struct wl_clientargs
{
	char *mac;
	char *wiface;
};

struct clientargs
{
	json_object *client;
	char *lan_name;
};

struct ldlanargs
{
	struct uci_section *ldlansection;
	char *ldinstance;
};

struct ldipargs
{
	struct uci_section *ldipsection;
};

struct lddhcpargs
{
	struct uci_section *lddhcpsection;
};

struct ldwlanargs
{
	struct uci_section *lwlansection;
	int wlctl_num;
	struct uci_section *device_section;
	char *wunit;
	char *wiface;
	json_object *res;
	int pki;
};

struct ldethargs
{
	struct uci_section *lan_ethsection;
	char *eth;
};

struct wlan_psk
{
	struct uci_section *wlanpsk;
};

struct wlan_wep
{
	struct uci_section *wlanwep;
	unsigned int key_index;
};

typedef struct dhcp_param
{
	char *interface;
	char *state_sec;
}dhcp_param;

inline int browselandeviceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
#endif
