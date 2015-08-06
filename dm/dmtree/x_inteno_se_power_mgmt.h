/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#ifndef __POWER_MGMT_H
#define __POWER_MGMT_H

int get_pwr_mgmt_value_ethapd(char *refparam, struct dmctx *ctx, char **value);
int get_pwr_mgmt_value_eee(char *refparam, struct dmctx *ctx, char **value);
int get_pwr_nbr_interfaces_up(char *refparam, struct dmctx *ctx, char **value);
int get_pwr_nbr_interfaces_down(char *refparam, struct dmctx *ctx, char **value);
int set_power_mgmt_param_ethapd(char *refparam, struct dmctx *ctx, int action, char *value);
int set_power_mgmt_param_eee(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_X_INTENO_SE_PowerManagement(struct dmctx *ctx);
#endif