/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *		Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *
 */

#include <uci.h>
#include <stdio.h>
#include <ctype.h>
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "x_inteno_syslog.h"

int get_server_ip_address(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("system", "@system[0]", "log_ip", value);
	return 0;
}

int set_server_ip_address(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("system", "@system[0]", "log_ip", value);
			return 0;
	}
	return 0;
}
	
int get_server_port_number(char *refparam, struct dmctx *ctx, char **value)
{
	char *tmp;
	dmuci_get_option_value_string("system", "@system[0]", "log_port", &tmp);
	if (tmp[0] == '\0')
		*value = "514";
	else
		*value = tmp;
	return 0;
}

int set_server_port_number(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("system", "@system[0]", "log_port", value);
			return 0;
	}
	return 0;
}

int get_remote_log_level(char *refparam, struct dmctx *ctx, char **value)
{
	char *tmp;
	dmuci_get_option_value_string("system", "@system[0]", "conloglevel", &tmp);
	if (tmp[0] == '\0')
		*value = "7";
	else
		*value = tmp;
	return 0;
}

int set_remote_log_level(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("system", "@system[0]", "conloglevel", value);
			return 0;
	}
	return 0;
}

int entry_method_root_syslog(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_SyslogCfg.") {
		DMOBJECT(DMROOT"X_INTENO_SE_SyslogCfg.", ctx, "0", 0, NULL, NULL, NULL);
		DMPARAM("ServerIPAddress", ctx, "1", get_server_ip_address, set_server_ip_address, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ServerPortNumber", ctx, "1", get_server_port_number, set_server_port_number, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("RemoteLogLevel", ctx, "1", get_remote_log_level, set_remote_log_level, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}