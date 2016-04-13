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

#include <uci.h>
#include <stdio.h>
#include <ctype.h>
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "ppp.h"

struct ppp_args cur_ppp_args = {0};

/*************************************************************
 * INIT
/*************************************************************/
inline int init_ppp_args(struct dmctx *ctx, struct uci_section *s)
{
	struct ppp_args *args = &cur_ppp_args;
	ctx->args = (void *)args;
	args->ppp_sec = s;
	return 0;
}

/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_ppp(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"PPP.") {
		DMOBJECT(DMROOT"PPP.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"PPP.Interface.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_ppp_interface, ctx);
		return 0;
	}
	return FAULT_9005;
}


inline int entry_ppp_interface(struct dmctx *ctx)
{
	struct uci_section *net_sec = NULL;
	char *ppp_int = NULL, *ppp_int_last = NULL;
	char *proto;

	uci_foreach_sections("network", "interface", net_sec) {
		dmuci_get_value_by_section_string(net_sec, "proto", &proto);
		if (strcmp(proto,"ppp"))
			continue;
		init_ppp_args(ctx, net_sec);
		ppp_int = handle_update_instance(1, ctx, &ppp_int_last, update_instance_alias, 3, net_sec, "ppp_int_instance", "ppp_int_alias");
		SUBENTRY(entry_ppp_interface_instance, ctx, ppp_int);
	}
	return 0;
}
inline int entry_ppp_interface_instance(struct dmctx *ctx, char *int_num)
{
	IF_MATCH(ctx, DMROOT"PPP.Interface.%s.", int_num) {
		DMOBJECT(DMROOT"PPP.Interface.%s.", ctx, "0", 1, NULL, NULL, NULL, int_num);
		DMPARAM("Alias", ctx, "1", get_empty, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_empty, NULL, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "0", get_empty, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LowerLayers", ctx, "0", get_empty, NULL, NULL, 0, 1, UNDEF, NULL);//TODO
		return 0;
	}
	return FAULT_9005;
}
