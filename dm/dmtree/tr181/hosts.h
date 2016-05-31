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
#ifndef __HOSTS_H
#define __HOSTS_H

struct host_args
{
	json_object *client;
	char *key;
};

int entry_method_root_hosts(struct dmctx *ctx);

#endif
