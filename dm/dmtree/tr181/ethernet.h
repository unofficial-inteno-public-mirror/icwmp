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
#ifndef __ETHERNET_H
#define __ETHERNET_H

struct eth_port_args
{
	struct uci_section *eth_port_sec;
	char *ifname;
};
extern DMOBJ tEthernetObj[];
extern DMOBJ tEthernetStatObj[];
extern DMLEAF tEthernetParams[];
extern DMLEAF tEthernetStatParams[];
inline int browseEthIfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
#endif
