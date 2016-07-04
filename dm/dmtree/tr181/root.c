/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author MOHAMED Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *	  Author Feten Besbes <feten.besbes@pivasoftware.com>
 *
 */

#include "dmuci.h"
#include "dmcwmp.h"
#include "root.h"
#include "deviceinfo.h"
#include "ip.h"
#include "ethernet.h"
#include "bridging.h"
#include "wifi.h"
#include "wan.h"

int entry_method_root(struct dmctx *ctx)
{
	return FAULT_9005;
}

DMOBJ tEntryObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, NOTIFICATION, nextobj, leaf, linker*/
{DMROOT, &DMREAD, NULL, NULL, NULL, &DMFINFRM, &DMNONE, tRootObj, NULL, NULL},
{0}
};

/* *** Device. *** */
DMOBJ tRootObj[] = {
/* OBJ permission, addobj, delobj, browseinstobj, finform, nextobj, leaf, notification, linker*/
{"DeviceInfo", &DMREAD, NULL, NULL, NULL, &DMFINFRM, &DMNONE,tDeviceInfoObj, tDeviceInfoParams, NULL},
{"Bridging",&DMREAD, NULL, NULL, NULL, NULL, NULL, tBridgObj, NULL, NULL},
{"WiFi",&DMREAD, NULL, NULL, NULL, NULL, NULL, tWifiObj, NULL, NULL},
{"IP",&DMREAD, NULL, NULL, NULL, NULL, NULL, tIPObj, NULL, NULL},
{"Ethernet", &DMREAD, NULL, NULL, NULL, NULL, NULL, tEthernetObj, NULL, NULL},
{"DSL",&DMREAD, NULL, NULL, NULL, NULL, NULL, tDslObj, NULL, NULL},
{"ATM",&DMREAD, NULL, NULL, NULL, NULL, NULL, tAtmObj, NULL, NULL},
{"PTM", &DMREAD, NULL, NULL, NULL, NULL, NULL, tPtmObj, NULL, NULL},
{0}
};
