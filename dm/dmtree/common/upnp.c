/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *
 */

#include <uci.h>
#include <stdio.h>
#include <ctype.h>
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "upnp.h"

int get_upnp_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("upnpd","config","enabled", value);
	if ((*value)[0] == '\0') {
		*value = "1";
	}
	return 0;
}

int set_upnp_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	int check;
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value("upnpd", "config", "enabled", "");
			else 
				dmuci_set_value("upnpd", "config", "enabled", "0");
			return 0;
	}
	return 0;
}

int get_upnp_status(char *refparam, struct dmctx *ctx, char **value)
{
	pid_t pid = get_pid("miniupnpd");
	
	if (pid < 0) {
		*value = "Down";
	}
	else {
		*value = "Up";
	}
	return 0;
}

DMOBJ tUPnPObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf, linker*/
{"Device", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tUPnPDeviceParams, NULL},
{0}
};

DMLEAF tUPnPDeviceParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"Enable", &DMWRITE, DMT_BOOL, get_upnp_enable, set_upnp_enable, NULL, NULL, NULL},
{"X_INTENO_SE_Status", &DMREAD, DMT_STRING, get_upnp_status, NULL, NULL, NULL, NULL},
{0}
};
