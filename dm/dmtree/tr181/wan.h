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

extern DMOBJ tDslObj[];
extern DMOBJ tAtmObj[];
extern DMOBJ tPtmObj[];
extern DMOBJ tAtmLinkStatsObj[];
extern DMOBJ tPtmLinkStatsObj[];
extern DMLEAF tDslLineParams[];
extern DMLEAF tDslChanelParams[] ;
extern DMLEAF tAtmLineParams[];
extern DMLEAF tAtmLinkStatsParams[] ;
extern DMLEAF tPtmLinkStatsParams[];
extern DMLEAF tPtmLineParams[];

int browseDslLineInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseDslChannelInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseAtmLinkInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browsePtmLinkInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

#endif
