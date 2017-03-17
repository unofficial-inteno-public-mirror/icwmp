/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *
 */


#include <uci.h>
#include <ctype.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "x_inteno_se_ipacccfg.h"

struct ipaccargs cur_ipaccargs = {0};
struct pforwardrgs cur_pforwardrgs = {0};

inline int browseport_forwardingInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int browseAccListInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int init_args_ipacc(struct dmctx *ctx, struct uci_section *s)
{
	struct ipaccargs *args = &cur_ipaccargs;
	ctx->args = (void *)args;
	args->ipaccsection = s;
	return 0;
}

inline int init_args_pforward(struct dmctx *ctx, struct uci_section *s)
{
	struct pforwardrgs *args = &cur_pforwardrgs;
	ctx->args = (void *)args;
	args->forwardsection = s;
	return 0;
}

/************************************************************************************* 
**** function related to get_object_ip_acc_list_cfgobj ****
**************************************************************************************/

int get_x_bcm_com_ip_acc_list_cfgobj_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
		
	dmuci_get_value_by_section_string(accargs->ipaccsection, "enabled", value);
	if ((*value)[0] == '\0') {
		*value = "1";
	}		
	return 0;
}

int set_x_bcm_com_ip_acc_list_cfgobj_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	int check;
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b) {
				value = "";
			}
			else {
				value = "0";
			}
			dmuci_set_value_by_section(accargs->ipaccsection, "enabled", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_cfgobj_address_netmask(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_list *val;
	struct uci_element *e = NULL;
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	struct uci_list *list = NULL;
	
	dmuci_get_value_by_section_list(accargs->ipaccsection, "src_ip", &val);	
	if (val) {
		*value = dmuci_list_to_string(val, ",");
	}
	else
		*value = "";
	if ((*value)[0] == '\0') {
		*value = "0.0.0.0/0";
		return 0;
	}
	return 0;
}

int set_x_inteno_cfgobj_address_netmask(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch, *spch, *val;
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_delete_by_section(accargs->ipaccsection, "src_ip", "");
			val = dmstrdup(value);
			pch = strtok_r(val, " ,", &spch);
			while (pch != NULL) {
				dmuci_add_list_value_by_section(accargs->ipaccsection, "src_ip", pch);
				pch = strtok_r(NULL, " ,", &spch);
			}
			dmfree(val);
			return 0;
	}
	return 0;
}

int get_x_bcm_com_ip_acc_list_cfgobj_acc_port(char *refparam, struct dmctx *ctx, char **value)
{
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	
	dmuci_get_value_by_section_string(accargs->ipaccsection, "dest_port", value);
	return 0;
}

int set_x_bcm_com_ip_acc_list_cfgobj_acc_port(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(accargs->ipaccsection, "dest_port", value);
			return 0;
	}
	return 0;
}



/************************************************************************************* 
**** function related to get_cache_object_port_forwarding ****
**************************************************************************************/

int get_port_forwarding_name(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "name", value);
	return 0;
}

int set_port_forwarding_name(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "name", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "enabled", value);
	return 0;
}

int set_port_forwarding_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	int check;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(forwardargs->forwardsection, "enabled", "1");
			else 
				dmuci_set_value_by_section(forwardargs->forwardsection, "enabled", "0");
			return 0;
	}
	return 0;
}

int get_port_forwarding_loopback(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
		
	dmuci_get_value_by_section_string(forwardargs->forwardsection, "reflection", value);
	if((*value)[0] == '\0') {
		*value = "1";
	}
	return 0;
}

int set_port_forwarding_loopback(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(forwardargs->forwardsection, "reflection", "1");
			else 
				dmuci_set_value_by_section(forwardargs->forwardsection, "reflection", "0");
			return 0;
	}
	return 0;
}

int get_port_forwarding_protocol(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
		
	dmuci_get_value_by_section_string(forwardargs->forwardsection, "proto", value);
	return 0;
}

int set_port_forwarding_protocol(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "proto", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_external_zone(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "src", value);
	return 0;
}

int set_port_forwarding_external_zone(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "src", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_internal_zone(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "dest", value);
	return 0;
}

int set_port_forwarding_internal_zone(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "dest", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_external_port(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "src_dport", value);
	return 0;
}

int set_port_forwarding_external_port(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "src_dport", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_internal_port(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "dest_port", value);
	if ((*value)[0] == '\0') {
		*value = "any";
	}
	return 0;
}

int set_port_forwarding_internal_port(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcasecmp(value, "any") == 0) {
				value = "";
			}
			dmuci_set_value_by_section(forwardargs->forwardsection, "dest_port", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_source_port(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "src_port", value);
	if ((*value)[0] == '\0') {
		*value = "any";
	}
	return 0;
}

int set_port_forwarding_source_port(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcasecmp(value, "any") == 0) {
				value = "";
			}
			dmuci_set_value_by_section(forwardargs->forwardsection, "src_port", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_internal_ipaddress(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "dest_ip", value);
	return 0;
}

int set_port_forwarding_internal_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "dest_ip", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_external_ipaddress(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "src_dip", value);
	if ((*value)[0] == '\0') {
		*value = "any";
	}
	return 0;
}

int set_port_forwarding_external_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcasecmp(value, "any") == 0) {
				value = "";
			}
			dmuci_set_value_by_section(forwardargs->forwardsection, "src_dip", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_source_ipaddress(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_list *val;
	struct uci_element *e = NULL;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_list(forwardargs->forwardsection, "src_ip", &val);
	if (val) {
		*value = dmuci_list_to_string(val, ",");
	}
	else {
		*value = "";
	}
	if ((*value)[0] == '\0') {
		*value = "any";
	}
	return 0;
}

int set_port_forwarding_source_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch, *val, *spch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcasecmp(value, "any") == 0) {
				dmuci_delete_by_section(forwardargs->forwardsection, "src_ip", "");
			}
			else {
				dmuci_delete_by_section(forwardargs->forwardsection, "src_ip", "");
				val = dmstrdup(value);
				pch = strtok_r(val, " ,", &spch);
				while (pch != NULL) {
					dmuci_add_list_value_by_section(forwardargs->forwardsection, "src_ip", pch);
					pch = strtok_r(NULL, " ,", &spch);
				}
				dmfree(val);
			}						
			return 0;
	}
	return 0;
}

int get_port_forwarding_src_mac(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_list *list = NULL;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_list(forwardargs->forwardsection, "src_mac", &list);
	*value = dmuci_list_to_string(list, " ");
	return 0;
}

int set_port_forwarding_src_mac(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch, *spch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_delete_by_section(forwardargs->forwardsection, "src_mac", NULL);
			value = dmstrdup(value);
			pch = strtok_r(value, " ", &spch);
			while (pch != NULL) {
				dmuci_add_list_value_by_section(forwardargs->forwardsection, "src_mac", pch);
				pch = strtok_r(NULL, " ", &spch);
			}
			dmfree(value);
			return 0;
	}
	return 0;
}

/***** ADD DEL OBJ *******/
int add_ipacccfg_port_forwarding(struct dmctx *ctx, char **instancepara)
{
	char *value;
	char *instance;
	struct uci_section *redirect = NULL;	
	
	instance = get_last_instance_lev2("firewall", "redirect", "forwardinstance", "target", "DNAT");
	dmuci_add_section("firewall", "redirect", &redirect, &value);
	dmuci_set_value_by_section(redirect, "enabled", "0");
	dmuci_set_value_by_section(redirect, "target", "DNAT");
	dmuci_set_value_by_section(redirect, "proto", "tcp udp");
	*instancepara = update_instance(redirect, instance, "forwardinstance");
	return 0;
}


int delete_ipacccfg_port_forwarding(struct dmctx *ctx, unsigned char del_action)
{
	int found = 0;
	struct pforwardrgs *forwardargs;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	
	switch (del_action) {
		case DEL_INST:
			forwardargs = (struct pforwardrgs *)ctx->args;
			dmuci_delete_by_section(forwardargs->forwardsection, NULL, NULL);
			break;
		case DEL_ALL:
			uci_foreach_option_eq("firewall", "redirect", "target", "DNAT", s) {
				if (found != 0)
					dmuci_delete_by_section(ss, NULL, NULL);
				ss = s;
				found++;
			}
			if (ss != NULL)
				dmuci_delete_by_section(ss, NULL, NULL);
			break;
	}
	return 0;
}

////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_x_inteno_cfgobj_address_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_ipaccargs.ipaccsection, "frulealias", value);
	return 0;
}

int set_x_inteno_cfgobj_address_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_ipaccargs.ipaccsection, "frulealias", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_pforwardrgs.forwardsection, "forwardalias", value);
	return 0;
}

int set_port_forwarding_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_pforwardrgs.forwardsection, "forwardalias", value);
			return 0;
	}
	return 0;
}

DMLEAF tSe_PortForwardingParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_port_forwarding_alias, set_port_forwarding_alias, NULL, NULL},
{"Name", &DMWRITE, DMT_STRING, get_port_forwarding_name, set_port_forwarding_name, NULL, NULL},
{"Enable", &DMWRITE, DMT_BOOL, get_port_forwarding_enable, set_port_forwarding_enable, NULL, NULL},
{"EnalbeNatLoopback", &DMWRITE, DMT_BOOL, get_port_forwarding_loopback, set_port_forwarding_enable, NULL, NULL},
{"Protocol", &DMWRITE, DMT_STRING, get_port_forwarding_protocol, set_port_forwarding_protocol, NULL, NULL},
{"ExternalZone", &DMWRITE, DMT_STRING, get_port_forwarding_external_zone, set_port_forwarding_external_zone, NULL, NULL},
{"InternalZone", &DMWRITE, DMT_STRING, get_port_forwarding_internal_zone, set_port_forwarding_internal_zone, NULL, NULL},
{"ExternalPort", &DMWRITE, DMT_STRING, get_port_forwarding_external_port, set_port_forwarding_external_port, NULL, NULL},
{"InternalZone", &DMWRITE, DMT_STRING, get_port_forwarding_internal_zone, set_port_forwarding_internal_zone, NULL, NULL},
{"SourcePort", &DMWRITE, DMT_STRING, get_port_forwarding_source_port, set_port_forwarding_source_port, NULL, NULL},
{"InternalIpAddress", &DMWRITE, DMT_STRING, get_port_forwarding_internal_ipaddress, set_port_forwarding_internal_ipaddress, NULL, NULL},
{"ExternalIpAddress", &DMWRITE, DMT_STRING, get_port_forwarding_external_ipaddress, set_port_forwarding_external_ipaddress, NULL, NULL},
{"SourceIpAddress", &DMWRITE, DMT_STRING, get_port_forwarding_source_ipaddress, set_port_forwarding_source_ipaddress, NULL, NULL},
{"SourceMacAddress", &DMWRITE, DMT_STRING, get_port_forwarding_src_mac, set_port_forwarding_src_mac, NULL, NULL},
{0}
};

DMLEAF tSe_IpAccCfgParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_x_inteno_cfgobj_address_alias, set_x_inteno_cfgobj_address_alias, NULL, NULL},
{"Enable", &DMWRITE, DMT_BOOL, get_x_bcm_com_ip_acc_list_cfgobj_enable, set_x_bcm_com_ip_acc_list_cfgobj_enable, NULL, NULL},
{"AccAddressAndNetMask", &DMWRITE, DMT_STRING, get_x_inteno_cfgobj_address_netmask, set_x_inteno_cfgobj_address_netmask, NULL, NULL},
{"AccPort", &DMWRITE, DMT_STRING, get_x_bcm_com_ip_acc_list_cfgobj_acc_port, set_x_bcm_com_ip_acc_list_cfgobj_acc_port, NULL, NULL},
{0}
};

DMOBJ tSe_IpAccObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"X_INTENO_SE_IpAccListCfgObj", &DMREAD, NULL, NULL, NULL, browseAccListInst, NULL, NULL, NULL, tSe_IpAccCfgParam, NULL},
{"X_INTENO_SE_PortForwarding", &DMWRITE, add_ipacccfg_port_forwarding, delete_ipacccfg_port_forwarding, NULL, browseport_forwardingInst, NULL, NULL, NULL, tSe_PortForwardingParam, NULL},
{0}
};

inline int browseAccListInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *irule = NULL, *irule_last = NULL;
	struct uci_section *s = NULL;

	uci_foreach_sections("firewall", "rule", s) {
		init_args_ipacc(dmctx, s);
		irule =  handle_update_instance(1, dmctx, &irule_last, update_instance_alias, 3, s, "fruleinstance", "frulealias");
		DM_LINK_INST_OBJ(dmctx, parent_node, NULL, irule);
	}
	return 0;
}

inline int browseport_forwardingInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *iforward = NULL, *iforward_last = NULL;
	struct uci_section *s = NULL;
	uci_foreach_option_eq("firewall", "redirect", "target", "DNAT", s) {
		init_args_pforward(dmctx, s);
		iforward =  handle_update_instance(1, dmctx, &iforward_last, update_instance_alias, 3, s, "forwardinstance", "forwardalias");
		DM_LINK_INST_OBJ(dmctx, parent_node, NULL, iforward);
	}
	return 0;
}

