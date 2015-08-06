/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 */

#ifndef __DEVICE_INFO_H
#define __DEVICE_INFO_H
#include "dmcwmp.h"

#define BASE_MAC_ADDR "/proc/nvram/BaseMacAddr"
#define UPTIME "/proc/uptime"

char *get_deviceid_manufacturer();
char *get_deviceid_manufactureroui();
char *get_deviceid_productclass();
char *get_deviceid_serialnumber();
char *get_softwareversion();
int get_spec_version(char *refparam, struct dmctx *ctx, char **value);
int get_device_manufacturer(char *refparam, struct dmctx *ctx, char **value);
int get_device_manufactureroui(char *refparam, struct dmctx *ctx, char **value);
int get_device_routermodel(char *refparam, struct dmctx *ctx, char **value);
int get_device_productclass(char *refparam, struct dmctx *ctx, char **value);
int get_device_serialnumber(char *refparam, struct dmctx *ctx, char **value);
int get_device_hardwareversion(char *refparam, struct dmctx *ctx, char **value);
int get_device_softwareversion(char *refparam, struct dmctx *ctx, char **value);
int get_device_info_uptime(char *refparam, struct dmctx *ctx, char **value);
int get_device_devicelog (char *refparam, struct dmctx *ctx, char **value);
int get_device_specversion(char *refparam, struct dmctx *ctx, char **value);
int get_device_provisioningcode(char *refparam, struct dmctx *ctx, char **value);
int set_device_provisioningcode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_base_mac_addr(char *refparam, struct dmctx *ctx, char **value);
int get_catv_enabled(char *refparam, struct dmctx *ctx, char **value);
int set_device_catvenabled(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_DeviceInfo(struct dmctx *ctx);

#endif
