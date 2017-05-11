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
#ifndef __BRIDGING_H
#define __BRIDGING_H


struct bridging_args
{
	struct uci_section *bridge_sec;
	char *br_key;
	char *ifname;
	char *br_inst;
};

struct bridging_port_args
{
	struct uci_section *bridge_port_sec;
	struct uci_section *bridge_sec;
	bool vlan;
	char *ifname;
};

struct bridging_vlan_args
{
	struct uci_section *bridge_vlan_sec;
	struct uci_section *bridge_sec;
	char *vlan_port;
	char *br_inst;
	char *ifname;
};

#define BUF_SIZE 7
extern DMOBJ tBridgingObj[];
extern DMOBJ tDridgingBridgeObj[];
extern DMLEAF tDridgingBridgeParams[];
extern DMLEAF tBridgeVlanParams[];
extern DMLEAF tBridgePortParams[];
extern DMLEAF tBridgeVlanPortParams[];
extern DMOBJ tBridgePortObj[];
extern DMLEAF tBridgePortStatParams[];

#endif
