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
#ifndef __PPP_H
#define __PPP_H

struct ppp_args
{
	struct uci_section *ppp_sec;
};

int entry_method_root_ppp(struct dmctx *ctx);

#endif
