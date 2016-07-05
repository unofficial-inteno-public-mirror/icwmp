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
#ifndef __DHCP_H
#define __DHCP_H

#include <json-c/json.h>

struct dhcp_args
{
	struct uci_section *dhcp_sec;
	char *interface;
};

struct dhcp_static_args
{
	struct uci_section *dhcpsection;
};

struct client_args
{
	json_object *client;
	char *key;
};


extern DMOBJ tDhcpServerObj[] ;
DMOBJ tDhcpServerPoolObj[];
DMOBJ tDhcpServerPoolAddressObj[];
DMLEAF tDhcpServerPoolParams[];
DMLEAF tDhcpServerPoolAddressParams[];
DMLEAF tDhcpServerPoolClientParams[];

inline int browseDhcpInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int browseDhcpStaticInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int browseDhcpClientInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

#endif
