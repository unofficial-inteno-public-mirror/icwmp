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
	static bool b;
	int check;
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
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
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	struct uci_list *list = NULL;
	
	dmuci_get_value_by_section_string(accargs->ipaccsection, "src_ip", value);
	if ((*value)[0] == '\0') {
		*value = "0.0.0.0/0";
		return 0;
	}
	return 0;
}

int set_x_inteno_cfgobj_address_netmask(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct ipaccargs *accargs = (struct ipaccargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_delete_by_section(accargs->ipaccsection, "src_ip", NULL); //TODO CHECK
			value = dmstrdup(value);
			pch = strtok (value, ",");
			while (pch != NULL) {
				dmuci_add_list_value_by_section(accargs->ipaccsection, "src_ip", pch);
				pch = strtok(NULL, ",");
			}
			dmfree(value);
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
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(accargs->ipaccsection, "dest_port", value);
			return 0;
	}
	return 0;
}

inline int get_object_ip_acc_list_cfgobj(struct dmctx *ctx, char *irule)
{
	DMOBJECT(DMROOT"X_INTENO_SE_IpAccCfg.X_INTENO_SE_IpAccListCfgObj.%s.", ctx, "0", 1, NULL, NULL, NULL, irule);
	DMPARAM("Enable", ctx, "1", get_x_bcm_com_ip_acc_list_cfgobj_enable, set_x_bcm_com_ip_acc_list_cfgobj_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
	DMPARAM("AccAddressAndNetMask", ctx, "1", get_x_inteno_cfgobj_address_netmask, set_x_inteno_cfgobj_address_netmask, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("AccPort", ctx, "1", get_x_bcm_com_ip_acc_list_cfgobj_acc_port, set_x_bcm_com_ip_acc_list_cfgobj_acc_port, NULL, 0, 1, UNDEF, NULL);
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
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "name", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "enabled", value);
	if((*value)[0] == '\0') {
		*value = "1";
	}
	return 0;
}

int set_port_forwarding_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	int check;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if(b)
				dmuci_set_value_by_section(forwardargs->forwardsection, "enabled", "");
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
	static bool b;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if(b)
				dmuci_set_value_by_section(forwardargs->forwardsection, "reflection", "");
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
		VALUECHECK:
			return 0;
		VALUESET:
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
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "src", value);
			return 0;
	}
	return 0;
}

int get_port_forwarding_internal_zone(char *refparam, struct dmctx *ctx, char **value)
{
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "src", value);
	return 0;
}

int set_port_forwarding_internal_zone(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "src", value);
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
		VALUECHECK:
			return 0;
		VALUESET:
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
		VALUECHECK:
			return 0;
		VALUESET:
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
		VALUECHECK:
			return 0;
		VALUESET:
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

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "src_ip", value);
	return 0;
}

int set_port_forwarding_internal_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(forwardargs->forwardsection, "src_ip", value);
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
		VALUECHECK:
			return 0;
		VALUESET:
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
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	dmuci_get_value_by_section_string(forwardargs->forwardsection, "src_ip", value);
	if ((*value)[0] == '\0') {
		*value = "any";
	}
	return 0;
}

int set_port_forwarding_source_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			if (strcasecmp(value, "any") == 0) {
				value = "";
			}
			dmuci_set_value_by_section(forwardargs->forwardsection, "src_ip", value);
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
	char *pch;
	struct pforwardrgs *forwardargs = (struct pforwardrgs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_del_list_value_by_section(forwardargs->forwardsection, "src_mac", NULL);
			value = dmstrdup(value);
			pch = strtok (value, " ");
			while (pch != NULL) {
				dmuci_add_list_value_by_section(forwardargs->forwardsection, "src_mac", pch);
				pch = strtok(NULL, " ");
			}
			dmfree(value);
			return 0;
	}
	return 0;
}

inline int get_object_port_forwarding(struct dmctx *ctx, char *iforward)
{
	DMOBJECT(DMROOT"X_INTENO_SE_IpAccCfg.X_INTENO_SE_PortForwarding.%s.", ctx, "0", 1, NULL, NULL, NULL, iforward);
	DMPARAM("Name", ctx, "1", get_port_forwarding_name, set_port_forwarding_name, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("Enable", ctx, "1", get_port_forwarding_enable, set_port_forwarding_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
	DMPARAM("EnalbeNatLoopback", ctx, "1", get_port_forwarding_loopback, set_port_forwarding_loopback, "xsd:boolean", 0, 1, UNDEF, NULL);
	DMPARAM("Protocol", ctx, "1", get_port_forwarding_protocol, set_port_forwarding_protocol, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("ExternalZone", ctx, "1", get_port_forwarding_external_zone, set_port_forwarding_external_zone, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("InternalZone", ctx, "1", get_port_forwarding_internal_zone, set_port_forwarding_internal_zone, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("ExternalPort", ctx, "1", get_port_forwarding_external_port, set_port_forwarding_external_port, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("InternalPort", ctx, "1", get_port_forwarding_internal_port, set_port_forwarding_internal_port, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("SourcePort", ctx, "1", get_port_forwarding_source_port, set_port_forwarding_source_port, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("InternalIpAddress", ctx, "1", get_port_forwarding_internal_ipaddress, set_port_forwarding_internal_ipaddress, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("ExternalIpAddress", ctx, "1", get_port_forwarding_external_ipaddress, set_port_forwarding_external_ipaddress, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("SourceIpAddress", ctx, "1", get_port_forwarding_source_ipaddress, set_port_forwarding_source_ipaddress, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("SourceMacAddress", ctx, "1", get_port_forwarding_src_mac, set_port_forwarding_src_mac, NULL, 0, 1, UNDEF, NULL);
	return 0;
}

int entry_method_root_X_INTENO_SE_IpAccCfg(struct dmctx *ctx)
{
	char *irule = NULL;
	char *cur_irule = NULL;
	char *iforward = NULL;
	char *cur_iforward = NULL;
	struct uci_section *s = NULL;
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_IpAccCfg.") {
		DMOBJECT(DMROOT"X_INTENO_SE_IpAccCfg.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"X_INTENO_SE_IpAccCfg.X_INTENO_SE_IpAccListCfgObj.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"X_INTENO_SE_IpAccCfg.X_INTENO_SE_PortForwarding.", ctx, "1", 1, NULL, NULL, NULL);
		uci_foreach_sections("firewall", "rule", s) {
			if (s != NULL ) {
				init_args_ipacc(ctx, s);
				irule = update_instance(s, cur_irule, "fruleinstance");
				SUBENTRY(get_object_ip_acc_list_cfgobj, ctx, irule);
				dmfree(cur_irule);
				cur_irule = dmstrdup(irule);
			}
			else 
				break;
		}
		dmfree(cur_irule);
		uci_foreach_option_eq("firewall", "redirect", "target", "DNAT", s) {
			if (s != NULL ) {
				init_args_pforward(ctx, s);
				iforward = update_instance(s, cur_iforward, "forwardinstance");
				SUBENTRY(get_object_port_forwarding, ctx, iforward);
				if (cur_iforward)
				dmfree(cur_iforward);
				cur_iforward = dmstrdup(iforward);
			}
			else 
				break;
		}
		dmfree(cur_iforward);
		return 0;
	}
	return FAULT_9005;
}
