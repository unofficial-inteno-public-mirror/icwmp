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
#include <json/json.h>
#define NVRAM_FILE "/proc/nvram/WpaKey"

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
};

int entry_method_root_LANDevice(struct dmctx *ctx);
#endif
