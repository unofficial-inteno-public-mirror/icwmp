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
extern DMLEAF tDeviceInfoParams[];
extern DMLEAF tCatTvParams[];
extern DMLEAF tVcfParams[];
extern DMOBJ tDeviceInfoObj[];
struct dev_vcf
{
	struct uci_section *vcf_sec;
};

char *get_deviceid_manufacturer();
char *get_deviceid_manufactureroui();
char *get_deviceid_productclass();
char *get_deviceid_serialnumber();
char *get_softwareversion();
int lookup_vcf_name(char *instance, char **value);
int browseVcfInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

#endif
