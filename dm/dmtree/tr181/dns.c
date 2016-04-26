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
#include "dns.h"

struct dns_args cur_dns_args = {0};

/*************************************************************
 * INIT
/*************************************************************/
inline int init_dns_args(struct dmctx *ctx, struct uci_section *int_sec, char *dns_ip, int num)
{
	struct dns_args *args = &cur_dns_args;
	ctx->args = (void *)args;
	args->int_sec = int_sec;
	args->dns_ip = dns_ip;
	args->num = num;
	return 0;
}
/*************************************************************
 * GET & SET PARAM
/*************************************************************/
int get_dns_enable(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "1";
	return 0;
}

int get_dns_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmasprintf(value, "cpe-%d", cur_dns_args.num);
	return 0;
}

int get_dns_server_ip(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup(cur_dns_args.dns_ip);
	return 0;
}

int set_dns_server_ip(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *dup, *p, *proto;

	switch (action) {
		case VALUECHECK:
			dmuci_get_value_by_section_string(cur_dns_args.int_sec, "proto", &proto);
			if (strcmp(proto, "static"))
				return FAULT_9001;
			return 0;
		case VALUESET:
			dup = dmstrdup(value);
			for (p = dup; *p ; p++) {
				if (*p == ',')
					*p = ' ';
			}
			dmuci_set_value_by_section(cur_dns_args.int_sec, "dns", dup);
			dmfree(dup);
			return 0;
	}
	return 0;
}

int get_dns_interface(char *refparam, struct dmctx *ctx, char **value)
{
	char *linker;
	linker = dmstrdup(section_name(cur_dns_args.int_sec));
	adm_entry_get_linker_param(DMROOT"IP.Interface.", linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
	if (*value == NULL)
		*value = "";
	dmfree(linker);
	return 0;
}
/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_dns(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"DNS.") {
		DMOBJECT(DMROOT"DNS.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"DNS.Client.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"DNS.Client.Server.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_dns, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_dns(struct dmctx *ctx)
{
	struct uci_section *net_sec = NULL;
	char *dns_inst;
	char *idx, *idx_last = NULL;
	json_object *res;
	int id = 0;
	char *tmp = NULL, *pch, *spch, *dns, *p;

	uci_foreach_sections("network", "interface", net_sec) {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(net_sec)}}, 1, &res);
		DM_ASSERT(res, tmp = "");
		json_parse_array(res, "dns-server", -1, NULL, &tmp);
		if (tmp[0] == '\0') {
			dmuci_get_value_by_section_string(net_sec, "dns", &dns);
			if (dns[0] == '\0')
				continue;
			tmp = dmstrdup(dns); // MEM WILL BE FREED IN DMMEMCLEAN
			for (p = tmp; *p ; p++) {
				if (*p == ' ')
					*p = ',';
			}
		}
		for (pch = strtok_r(tmp, ",", &spch); pch != NULL; pch = strtok_r(NULL, ",", &spch)) {
			idx = handle_update_instance(3, ctx, &idx_last, update_instance_without_section, 1, ++id);
			init_dns_args(ctx, net_sec, pch, id);
			SUBENTRY(entry_dns_instance, ctx, idx);
		}
	}
	return 0;
}

inline int entry_dns_instance(struct dmctx *ctx, char *int_num)
{
	IF_MATCH(ctx, DMROOT"DNS.Client.Server.%s.", int_num) {
		DMOBJECT(DMROOT"DNS.Client.Server.%s.", ctx, "0", NULL, NULL, NULL, NULL, int_num);
		DMPARAM("Enable", ctx, "0", get_dns_enable, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_dns_enable, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("Alias", ctx, "0", get_dns_alias, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("DNSServer", ctx, "1", get_dns_server_ip, set_dns_server_ip, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("Interface", ctx, "0", get_dns_interface, NULL, NULL, 0, 0, UNDEF, NULL);

		return 0;
	}
	return FAULT_9005;
}
