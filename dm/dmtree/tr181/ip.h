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
	char *ip_4address;
	char *ip_6address;
};

extern DMOBJ tIPObj[];
extern DMOBJ tInterfaceObj[];
extern DMLEAF tIPv4Params[];
extern DMLEAF tIPv6Params[];
extern DMLEAF tIPintParams[];
extern DMLEAF tIpPingDiagParams[];
extern DMOBJ tDiagnosticObj[];
unsigned char get_ipv4_finform(char *refparam, struct dmctx *dmctx, void *data, char *instance);
unsigned char get_ipv6_finform(char *refparam, struct dmctx *dmctx, void *data, char *instance);
int browseIPIfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseIfaceIPv4Inst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseIfaceIPv6Inst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

#endif
