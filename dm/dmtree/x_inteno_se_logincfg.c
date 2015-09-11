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

int set_x_bcm_admin_password(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_x_bcm_password(refparam, ctx, action, value, "admin");
	return 0;
}

int set_x_bcm_support_password(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_x_bcm_password(refparam, ctx, action, value, "support");
	return 0;
}

int set_x_bcm_user_password(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_x_bcm_password(refparam, ctx, action, value, "user");
	return 0;
}

int set_x_bcm_root_password(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_x_bcm_password(refparam, ctx, action, value, "root");
	return 0;
}

int entry_method_root_X_INTENO_SE_LOGIN_CFG(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_LoginCfg.") {
		DMOBJECT(DMROOT"X_INTENO_SE_LoginCfg.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("AdminPassword", ctx, "1", get_empty, set_x_bcm_admin_password, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SupportPassword", ctx, "1", get_empty, set_x_bcm_support_password, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("UserPassword", ctx, "1", get_empty, set_x_bcm_user_password, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("RootPassword", ctx, "1", get_empty, set_x_bcm_root_password, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}