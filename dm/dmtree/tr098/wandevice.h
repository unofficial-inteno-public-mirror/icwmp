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
#include <json/json.h>

struct wancprotoargs
{
	struct uci_section *wancprotosection;
	struct uci_ptr *ptr;
};

struct wancdevargs
{
	struct uci_section *wandevsection;
	int index;
	char *fwan;
	char *iwan;
	char *wan_ifname;
};

struct wanargs
{
	struct uci_section *wandevsection;
	int instance;
	char *fdev;
};
int entry_method_root_WANDevice(struct dmctx *ctx);
#endif