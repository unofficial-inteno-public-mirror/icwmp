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
int entry_method_root_LANDevice(struct dmctx *ctx);
#endif
