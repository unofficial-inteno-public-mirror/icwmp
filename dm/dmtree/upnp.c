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
	static bool b;
	int check;	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if(b)
				dmuci_set_value("upnpd", "config", "enabled", "1");
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

int entry_method_root_upnp(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"UPnP.") {
		DMOBJECT(DMROOT"UPnP.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"UPnP.Device.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("Enable", ctx, "1", get_upnp_enable, set_upnp_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_Status", ctx, "0", get_upnp_status, NULL, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}			
