/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2016 Inteno Broadband Technology AB
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 *
 */
#ifndef __WAN_H
#define __WAN_H


struct dsl_line_args
{
	struct uci_section *line_sec;
	char *type;
};

struct dsl_channel_args
{
	struct uci_section *chanel_sec;
};

struct atm_args
{
	struct uci_section *atm_sec;
	char *ifname;
};
struct ptm_args
{
	struct uci_section *ptm_sec;
	char *ifname;
};
int entry_method_root_wan_dsl(struct dmctx *ctx);
int entry_method_root_wan_ptm(struct dmctx *ctx);
int entry_method_root_wan_atm(struct dmctx *ctx);

#endif
