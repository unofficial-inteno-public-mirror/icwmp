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
#include "nat.h"

struct nat_args
{
	struct uci_section *int_sec;
};

struct nat_args cur_nat_args = {0};

inline int init_nat_args(struct dmctx *ctx, struct uci_section *int_sec)
{
	struct nat_args *args = &cur_nat_args;
	ctx->args = (void *)args;
	args->int_sec = int_sec;	
	return 0;
}

int get_nat_enable(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "1";
	return 0;
}

int get_nat_alias(char *refparam, struct dmctx *ctx, char **value)
{
	struct nat_args *natargs = (struct nat *)ctx->args;
	dmuci_get_value_by_section_string(natargs->int_sec, "natalias", value);
	return 0;
}

int set_nat_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct nat_args *natargs = (struct nat *)ctx->args;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(natargs->int_sec, "natalias", value);
			return 0;
	}
	return 0;
}
int get_nat_interface(char *refparam, struct dmctx *ctx, char **value)
{
	char *linker;
	linker = dmstrdup(section_name(cur_nat_args.int_sec));
	adm_entry_get_linker_param(DMROOT"IP.Interface.", linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
	if (*value == NULL)
		*value = "";
	dmfree(linker);
	return 0;
}

/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_nat(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"NAT.") {
		DMOBJECT(DMROOT"NAT.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"NAT.InterfaceSetting.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_nat, ctx);
		return 0;
	}
	return FAULT_9005;
}

int get_nat_last_inst()
{
	char *rinst = "0", *tmp;
	int max;
	struct uci_section *s;
	int cnt = 0;
	
	uci_foreach_sections("network", "interface", s) {
		dmuci_get_value_by_section_string(s, "natinstance", &tmp);
		if (tmp[0] == '\0')
			continue;
		else if (atoi(rinst) < atoi(tmp))
		{
			rinst = tmp;
		}		
	}
	max = atoi(rinst);
	return max;
}

char *nat_update_instance_alias(int action, char **last_inst, void *argv[])
{
	char *instance, *alias;
	char buf[8] = {0};

	struct uci_section *s = (struct uci_section *) argv[0];
	char *inst_opt = (char *) argv[1];
	char *alias_opt = (char *) argv[2];
	bool *find_max = (bool *) argv[3];

	dmuci_get_value_by_section_string(s, inst_opt, &instance);
	if (instance[0] == '\0') {
		if (*find_max) {
			int m = get_nat_last_inst();
			sprintf(buf, "%d", m+1);
			*find_max = false;
		}
		else if (last_inst == NULL) {
			sprintf(buf, "%d", 1);
		}
		else {
			sprintf(buf, "%d", atoi(*last_inst)+1);
		}
		instance = dmuci_set_value_by_section(s, inst_opt, buf);
	}
	*last_inst = instance;
	if (action == INSTANCE_MODE_ALIAS) {
		dmuci_get_value_by_section_string(s, alias_opt, &alias);
		if (alias[0] == '\0') {
			sprintf(buf, "cpe-%s", instance);
			alias = dmuci_set_value_by_section(s, alias_opt, buf);
		}
		sprintf(buf, "[%s]", alias);
		instance = dmstrdup(buf);
	}
	return instance;
}

inline int entry_nat(struct dmctx *ctx)
{
	struct uci_section *net_sec = NULL, *s = NULL;
	char *nat = NULL;
	char *nati, *nati_last = NULL;
	int id = 0;
	bool find_max = true;
	uci_foreach_sections("network", "interface", net_sec) {
		uci_foreach_list_cont("firewall", "zone", "network", section_name(net_sec), s) {
			dmuci_get_value_by_section_string(s, "masq", &nat);
			if(nat[0] == '1') {
				nati =  handle_update_instance(1, ctx, &nati_last, nat_update_instance_alias, 4, net_sec, "natinstance", "natalias", &find_max);
				init_nat_args(ctx, net_sec);
				SUBENTRY(entry_nat_instance, ctx, nati);
				break;
			}			
		}
	}	
	return 0;
}


inline int entry_nat_instance(struct dmctx *ctx, char *int_num)
{
	IF_MATCH(ctx, DMROOT"NAT.InterfaceSetting.%s.", int_num) {
		DMOBJECT(DMROOT"NAT.InterfaceSetting.%s.", ctx, "0", NULL, NULL, NULL, NULL, int_num);
		DMPARAM("Enable", ctx, "0", get_nat_enable, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("Alias", ctx, "1", get_nat_alias, set_nat_alias, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("Interface", ctx, "0", get_nat_interface, NULL, NULL, 0, 0, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

