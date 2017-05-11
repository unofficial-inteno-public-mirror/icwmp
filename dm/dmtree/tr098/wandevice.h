/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#ifndef __WAN_DEVICE_H
#define __WAN_DEVICE_H
#include <libubox/blobmsg_json.h>
#include <json-c/json.h>

struct wanargs
{
/* WANDevice */
	struct uci_section *wandevsection;
	int instance;
	char *fdev;
/* WANConnectionDevice */
	struct uci_section *wancdsection;
	char *fwan;
	char *iwan;
	char *wan_ifname;
/* WAN###Connection */
	struct uci_section *wancprotosection;
};

extern struct dm_notif_s DMWANConnectionDevicenotif;
extern DMOBJ tWANDeviceObj[];
extern DMLEAF tWANDeviceParam[];
extern DMOBJ tWANConnectionDeviceObj[];
extern DMLEAF tWANConnectionDeviceParam[];
extern DMOBJ tWANConnectionObj[];
extern DMLEAF tWANIPConnectionParam[];
extern DMLEAF tWANPPPConnectionParam[];
extern DMLEAF tWANConnectionStatsParam[];
extern DMLEAF tWANConnection_VLANParam[];
extern DMLEAF tWANDSLLinkConfigParam[];
extern DMLEAF tWANCommonInterfaceConfigParam[];
extern DMLEAF tWANDSLInterfaceConfigParam[];
extern DMOBJ tWANEthernetInterfaceConfigObj[];
extern DMLEAF tWANEthernetInterfaceConfigParam[];
extern DMLEAF tWANEthernetInterfaceConfigStatsParam[];
int browsewandeviceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
unsigned char get_wan_protocol_connection_forced_inform(char *refparam, struct dmctx *dmctx, void *data, char *instance);
#endif
