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
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "x_inteno_se_logincfg.h"

int set_x_bcm_password(char *refparam, struct dmctx *ctx, int action, char *value, char *user_type)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("passwords", user_type, "password", value);
			return 0;
	}
	return 0;
}

int set_x_bcm_admin_password(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	set_x_bcm_password(refparam, ctx, action, value, "admin");
	return 0;
}

int set_x_bcm_support_password(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	set_x_bcm_password(refparam, ctx, action, value, "support");
	return 0;
}

int set_x_bcm_user_password(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	set_x_bcm_password(refparam, ctx, action, value, "user");
	return 0;
}

int set_x_bcm_root_password(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	set_x_bcm_password(refparam, ctx, action, value, "root");
	return 0;
}

DMLEAF tSe_LoginCfgParam[] = {
{"AdminPassword", &DMWRITE, DMT_STRING, get_empty, set_x_bcm_admin_password, NULL, NULL},
{"SupportPassword", &DMWRITE, DMT_STRING, get_empty, set_x_bcm_support_password, NULL, NULL},
{"UserPassword", &DMWRITE, DMT_STRING, get_empty, set_x_bcm_user_password, NULL, NULL},
{"RootPassword", &DMWRITE, DMT_STRING, get_empty, set_x_bcm_root_password, NULL, NULL},
{0}
};
