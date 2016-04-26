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
#ifndef __DNS_H
#define __DNS_H

struct dns_args
{
	struct uci_section *int_sec;
	char *dns_ip;
	int num;
};

int entry_method_root_dns(struct dmctx *ctx);

#endif
