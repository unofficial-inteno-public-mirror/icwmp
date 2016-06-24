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

int entry_method_root(struct dmctx *ctx)
{
	return FAULT_9005;
}

DMOBJ tEntryObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, nextobj, leaf*/
		//{"Manufacturer", "0", get_device_manufacturer, NULL, NULL, 1, 1, UNDEF, NULL, NULL, NULL},

{DMROOT, &DMREAD, NULL, NULL, NULL, &DMFINFRM, tRootObj, NULL},
{0}
};

/* *** Device. *** */
DMOBJ tRootObj[] = {
/* OBJ permission, addobj, delobj, browseinstobj, finform, nextobj, leaf*/
{"DeviceInfo", &DMREAD, NULL, NULL, NULL, &DMFINFRM, tDeviceInfoObj, tDeviceInfoParams},
//{"ManagementServer", &DMREAD, NULL, NULL, NULL, &DMFINFRM, NULL, tMgmtServerParams},
//{"Time", &DMREAD, NULL, NULL, NULL, NULL, NULL, tTimeParams},
//{"Services", &DMREAD, NULL, NULL, NULL, NULL, tServiceObj, NULL},
//{"IP", &DMREAD, NULL, NULL, NULL, &DMFINFRM, tIPObj, NULL},
//{"PPP", &DMREAD, NULL, NULL, NULL, &DMFINFRM, tPPPObj, NULL},
{0}
};
