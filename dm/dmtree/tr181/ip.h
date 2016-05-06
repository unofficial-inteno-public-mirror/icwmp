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
#ifndef __IP_H
#define __IP_H

extern struct ip_ping_diagnostic ipping_diagnostic; 
struct ip_args
{
	struct uci_section *ip_sec;
};

struct ipv4_args
{
	struct uci_section *ipv4_sec;
	char *ip_address;
};


int entry_method_root_ip(struct dmctx *ctx);

#endif
