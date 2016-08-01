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
#include "x_inteno_se_ice.h"

int get_ice_cloud_enable(char *refparam, struct dmctx *ctx, char **value)
{
	bool b;
	dmuci_get_option_value_string("ice", "cloud", "enabled", value);

	string_to_bool(*value, &b);
	if (b)
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_ice_cloud_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value("ice", "cloud", "enabled", "1");
			else
				dmuci_set_value("ice", "cloud", "enabled", "0");
			return 0;
	}
}

int get_ice_cloud_server(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("ice", "cloud", "server", value);
	return 0;
}

int set_ice_cloud_server(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				return 0;
			dmuci_set_value("ice", "cloud", "server", value);
			return 0;
	}
	return 0;
}

DMLEAF tSe_IceParam[] = {
	{"Enable", &DMWRITE, DMT_BOOL, get_ice_cloud_enable, set_ice_cloud_enable, NULL, NULL},
	{"Server", &DMWRITE, DMT_STRING, get_ice_cloud_server, set_ice_cloud_server, NULL, NULL},
	{0}
};
