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
#include "routing.h"

enum enum_route_type {
	ROUTE_STATIC,
	ROUTE_DYNAMIC,
	ROUTE_DISABLED
};

struct routingfwdargs cur_routefwdargs = {0};
struct router_args cur_router_args = {0};

inline int entry_router_ipv4forwarding_instance(struct dmctx *ctx, char *irouter, char *iroute, char *permission);
inline int init_args_ipv4forward(struct dmctx *ctx, struct uci_section *s, char *permission, struct proc_routing *proute, int type)
{
	struct routingfwdargs *args = &cur_routefwdargs;
	ctx->args = (void *)args;
	args->permission = permission;
	args->routefwdsection = s;
	args->proute = proute;
	args->type = type;
	return 0;
}

inline int init_router_args(struct dmctx *ctx, struct uci_section *section)
{
	struct router_args *args = &cur_router_args;
	ctx->args = (void *)args;
	args->router_section = section;
	return 0;
}
/************************************************************************************* 
**** function related to get_object_router_ipv4forwarding ****
**************************************************************************************/
void ip_to_hex(char *address, char *ret) //TODO Move to the common.c
{
	int i;
	int ip[4] = {0};
	
	sscanf(address, "%d.%d.%d.%d", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]));
	sprintf(ret, "%02X%02X%02X%02X", ip[0], ip[1], ip[2], ip[3]);
}

void hex_to_ip(char *address, char *ret) //TODO Move to the common.c
{
	int i;
	int ip[4] = {0};
	sscanf(address, "%2x%2x%2x%2x", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]));
	if (htonl(13) == 13) {
		sprintf(ret, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	} else {
		sprintf(ret, "%d.%d.%d.%d", ip[3], ip[2], ip[1], ip[0]);
	}
}

void parse_proc_route_line(char *line, struct proc_routing *proute)
{
	char *pch, *spch;
	proute->iface = strtok_r(line, " \t", &spch);

	pch = strtok_r(NULL, " \t", &spch);
	hex_to_ip(pch, proute->destination);
	pch = strtok_r(NULL, " \t", &spch);
	hex_to_ip(pch, proute->gateway);
	proute->flags = strtok_r(NULL, " \t", &spch);
	proute->refcnt = strtok_r(NULL, " \t", &spch);
	proute->use = strtok_r(NULL, " \t", &spch);
	proute->metric = strtok_r(NULL, " \t", &spch);
	pch = strtok_r(NULL, " \t", &spch);
	hex_to_ip(pch, proute->mask);
	proute->mtu = strtok_r(NULL, " \t", &spch);
	proute->window = strtok_r(NULL, " \t", &spch);
	proute->irtt = strtok_r(NULL, " \t\n\r", &spch);
}

bool is_proute_static(struct proc_routing *proute)
{
	char *mask;
	struct uci_section *s;
	uci_foreach_option_eq("network", "route", "target", proute->destination, s) {
		dmuci_get_value_by_section_string(s, "netmask", &mask);
		if (mask[0] == '\0' || strcmp(proute->mask, mask) == 0)
			return true;
	}
	uci_foreach_option_eq("network", "route_disabled", "target", proute->destination, s) {
		dmuci_get_value_by_section_string(s, "netmask", &mask);
		if (mask[0] == '\0' || strcmp(proute->mask, mask) == 0)
			return true;
	}
	return false;
}

bool is_cfg_route_active(struct uci_section *s)
{
	FILE *fp;
	char line[MAX_PROC_ROUTING];
	struct proc_routing proute;
	char *dest, *mask;

	dmuci_get_value_by_section_string(s, "target", &dest);
	dmuci_get_value_by_section_string(s, "netmask", &mask);

	fp = fopen(ROUTING_FILE, "r");
	if ( fp != NULL)
	{
		fgets(line, MAX_PROC_ROUTING, fp);
		while (fgets(line, MAX_PROC_ROUTING, fp) != NULL )
		{
			if (line[0] == '\n')
				continue;
			parse_proc_route_line(line, &proute);
			if (strcmp(dest, proute.destination) == 0 &&
				(mask[0] == '\0' || strcmp(mask, proute.mask) == 0)) {
				fclose(fp) ;
				return true;
			}
		}
		fclose(fp) ;
	}
	return false;
}

int get_forwarding_last_inst()
{
	char *rinst = NULL, *drinst = NULL, *dsinst = NULL, *tmp;
	int r = 0, dr = 0, ds = 0, max;
	struct uci_section *s;
	int cnt = 0;
	FILE *fp;
	char line[MAX_PROC_ROUTING];
	struct proc_routing proute;

	uci_foreach_sections("network", "route", s) {
		dmuci_get_value_by_section_string(s, "routeinstance", &tmp);
		if (tmp[0] == '\0')
			break;
		rinst = tmp;
	}
	uci_foreach_sections("network", "route_disabled", s) {
		dmuci_get_value_by_section_string(s, "routeinstance", &tmp);
		if (tmp[0] == '\0')
			break;
		dsinst = tmp;
	}
	uci_path_foreach_sections(icwmpd, "dmmap", "route_dynamic", s) {
		dmuci_get_value_by_section_string(s, "routeinstance", &tmp);
		if (tmp[0] == '\0')
			break;
		drinst = tmp;
	}
	if (rinst) r = atoi(rinst);
	if (dsinst) ds = atoi(dsinst);
	if (drinst) dr = atoi(drinst);
	max = (r>ds&&r>dr?r:ds>dr?ds:dr);
	return max;
}

char *forwarding_update_instance_alias(int action, char **last_inst, void *argv[])
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
			int m = get_forwarding_last_inst();
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

struct uci_section *update_route_dynamic_section(struct proc_routing *proute)
{
	struct uci_section *s = NULL;
	char *name, *mask;
	uci_path_foreach_option_eq(icwmpd, "dmmap", "route_dynamic", "target", proute->destination, s) {
		dmuci_get_value_by_section_string(s, "netmask", &mask);
		if (strcmp(proute->mask, mask) == 0){
			return s;
		}
	}
	if (!s) {
		DMUCI_ADD_SECTION(icwmpd, "dmmap", "route_dynamic", &s, &name);
		DMUCI_SET_VALUE_BY_SECTION(icwmpd, s, "target", proute->destination);
		DMUCI_SET_VALUE_BY_SECTION(icwmpd, s, "netmask", proute->mask);
	}
	return s;
}

int get_router_ipv4forwarding_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;

	if (routeargs->routefwdsection == NULL) {
		*value = "1";
		return 0;
	}
	if(strcmp(routeargs->routefwdsection->type, "route_disabled") == 0)
		*value = "0";
	else {
		*value = "1";
	}
	return 0;
}

int set_router_ipv4forwarding_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *pch;
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b) {
				if (routeargs->type == ROUTE_STATIC)
					return 0;
				dmuci_set_value_by_section(routeargs->routefwdsection, NULL, "route"); //TODO test
			}
			else {
				if (routeargs->type == ROUTE_DISABLED)
					return 0;
				dmuci_set_value_by_section(routeargs->routefwdsection, NULL, "route_disabled"); //TODO test
			}
			return 0;
	}
	return 0;
}

int get_router_ipv4forwarding_status(char *refparam, struct dmctx *ctx, char **value)
{
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;

	if (routeargs->routefwdsection == NULL) {
		*value = "Enabled";
		return 0;
	}
	if(strcmp(routeargs->routefwdsection->type, "route_disabled") == 0) {
		*value = "Disabled";
	} else {
		if (is_cfg_route_active(routeargs->routefwdsection))
			*value = "Enabled";
		else
			*value = "Error";
	}
	return 0;	
}

int get_router_ipv4forwarding_destip(char *refparam, struct dmctx *ctx, char **value)
{
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		dmuci_get_value_by_section_string(routeargs->routefwdsection, "target", value);
		if ((*value)[0] == '\0') {
			*value = "0.0.0.0";
		}
	}
	else {
		struct proc_routing *proute = routeargs->proute;
		*value = dmstrdup(proute->destination); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;		
}

int set_router_ipv4forwarding_destip(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "target", value);
			return 0;
	}
	return 0;
}

int get_router_ipv4forwarding_destmask(char *refparam, struct dmctx *ctx, char **value)
{
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		dmuci_get_value_by_section_string(routeargs->routefwdsection, "netmask", value);
		if ((*value)[0] == '\0') {
			*value = "255.255.255.255";
		}
	}
	else {
		struct proc_routing *proute = routeargs->proute;
		*value = dmstrdup(proute->mask); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
}

int set_router_ipv4forwarding_destmask(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "netmask", value);
			return 0;
	}
	return 0;
}

int get_router_ipv4forwarding_src_address(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "0.0.0.0";
	return 0;
}

int get_router_ipv4forwarding_src_mask(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "0.0.0.0";
	return 0;
}

int get_router_ipv4forwarding_gatewayip(char *refparam, struct dmctx *ctx, char **value)
{
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		dmuci_get_value_by_section_string(routeargs->routefwdsection, "gateway", value);
		if ((*value)[0] == '\0') {
			*value = "0.0.0.0";
		}
	}
	else {
		struct proc_routing *proute = routeargs->proute;
		*value = dmstrdup(proute->gateway); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
} 

int set_router_ipv4forwarding_gatewayip(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "gateway", value);
			return 0;
	}
	return 0;
}

char *get_router_ipv4forwarding_interface(struct dmctx *ctx)
{
	json_object *res;
	char *val, *bval, *ifname, *device;
	char *name;
	struct uci_section *ss;
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		dmuci_get_value_by_section_string(routeargs->routefwdsection, "interface", &val);
		return val;
	}
	else {
		struct proc_routing *proute = routeargs->proute;
		bval = proute->iface;
		val = bval;
		if (!strstr(bval, "br-")) {
			uci_foreach_option_cont("network", "interface", "ifname", bval, ss) {
				ifname = section_name(ss);
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ifname, String}}, 1, &res);
				if (res) {
					json_select(res, "device", 0, NULL, &device, NULL);
					if (strcmp(bval, device) == 0) {
						return ifname;
					}
				}
			}			
		}
		else {
			val = bval + sizeof("br-") - 1;
		}
		return val;
	}
	return "";
}

int get_router_ipv4forwarding_interface_linker_parameter(char *refparam, struct dmctx *ctx, char **value)
{
	char *iface, *linker;
			
	iface = get_router_ipv4forwarding_interface(ctx);
	if (iface[0] != '\0') {
		dmasprintf(&linker, "%s", iface);
		adm_entry_get_linker_param(DMROOT"IP.Interface.", linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
		if (*value == NULL)
			*value = "";
		dmfree(linker);
	}
	return 0;
}

int set_router_ipv4forwarding_interface_linker_parameter(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *linker;
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(value, &linker);
			if (linker) {
				dmuci_set_value_by_section(routeargs->routefwdsection, "interface", linker);
				dmfree(linker);
			}
			return 0;
	}
	return 0;
}

int get_router_ipv4forwarding_metric(char *refparam, struct dmctx *ctx, char **value)
{
	char *name;
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;	
	
	if (routeargs->routefwdsection != NULL)	{
		dmuci_get_value_by_section_string(routeargs->routefwdsection, "metric", value);
	}
	else {
		struct proc_routing *proute = routeargs->proute;
		*value = dmstrdup(proute->metric); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	if ((*value)[0] == '\0') {
		*value = "0";
	}	
	return 0;
}

int set_router_ipv4forwarding_metric(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct routingfwdargs *routeargs = (struct routingfwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "metric", value);
			return 0;
	}
	return 0;
}

int get_router_nbr_entry(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_section *s;
	int cnt = 0;
	FILE *fp;
	char line[MAX_PROC_ROUTING];
	struct proc_routing proute;

	uci_foreach_sections("network", "route", s) {
		cnt++;
	}
	uci_foreach_sections("network", "route_disabled", s) {
		cnt++;
	}
	fp = fopen(ROUTING_FILE, "r");
	if ( fp != NULL)
	{
		fgets(line, MAX_PROC_ROUTING, fp);
		while (fgets(line, MAX_PROC_ROUTING, fp) != NULL )
		{
			if (line[0] == '\n')
				continue;
			parse_proc_route_line(line, &proute);
			if (is_proute_static(&proute))
				continue;
			cnt++;
		}
		fclose(fp) ;
	}
	dmasprintf(value, "%d", cnt); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

/*************************************************************
 * SET AND GET ALIAS FOR ROUTER OBJ
/*************************************************************/

int get_router_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_router_args.router_section, "router_alias", value);
	return 0;
}

int set_router_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_router_args.router_section, "router_alias", value);
			return 0;
	}
	return 0;
}

int get_router_ipv4forwarding_alias(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "";
	if (cur_routefwdargs.routefwdsection) dmuci_get_value_by_section_string(cur_routefwdargs.routefwdsection, "routealias", value);
	return 0;
}

int set_router_ipv4forwarding_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (cur_routefwdargs.routefwdsection) dmuci_set_value_by_section(cur_routefwdargs.routefwdsection, "routealias", value);
			return 0;
	}
	return 0;
}

/*************************************************************
 * SUB ENTRIES
/*************************************************************/

inline int entry_router_ipv4forwarding(struct dmctx *ctx, char *irouter)
{
	char *iroute = NULL, *iroute_last = NULL;
	char *permission = "1";
	struct uci_section *s = NULL, *ss = NULL;
	FILE* fp = NULL;
	char line[MAX_PROC_ROUTING];
	struct proc_routing proute = {0};
	bool find_max = true;
	uci_foreach_sections("network", "route", s) {
		init_args_ipv4forward(ctx, s, "1", NULL, ROUTE_STATIC);
		iroute =  handle_update_instance(1, ctx, &iroute_last, forwarding_update_instance_alias, 4, s, "routeinstance", "routealias", &find_max);
		SUBENTRY(entry_router_ipv4forwarding_instance, ctx, irouter, iroute, permission);
	}
	uci_foreach_sections("network", "route_disabled", s) {
		init_args_ipv4forward(ctx, s, "1", NULL, ROUTE_DISABLED);
		iroute =  handle_update_instance(1, ctx, &iroute_last, forwarding_update_instance_alias, 4, s, "routeinstance", "routealias", &find_max);
		SUBENTRY(entry_router_ipv4forwarding_instance, ctx, irouter, iroute, permission);
	}
	fp = fopen(ROUTING_FILE, "r");
	if ( fp != NULL)
	{
		fgets(line, MAX_PROC_ROUTING, fp);
		while (fgets(line, MAX_PROC_ROUTING, fp) != NULL )
		{
			if (line[0] == '\n')
				continue;
			parse_proc_route_line(line, &proute);
			if (is_proute_static(&proute))
				continue;
			ss = update_route_dynamic_section(&proute);
			init_args_ipv4forward(ctx, ss, "0", &proute, ROUTE_DYNAMIC);
			iroute =  handle_update_instance(1, ctx, &iroute_last, forwarding_update_instance_alias, 4, ss, "routeinstance", "routealias", &find_max);
			SUBENTRY(entry_router_ipv4forwarding_instance, ctx, irouter, iroute, "0");
		}
		fclose(fp) ;
	}
	return 0;
}

inline int entry_method_router(struct dmctx *ctx)
{
	struct uci_section *s = NULL;
	char *r = NULL, *r_last = NULL;
	
	update_section_list(DMMAP,"router", NULL, 1, NULL, NULL, NULL, NULL, NULL);
	uci_path_foreach_sections(icwmpd, "dmmap", "router", s) {
		init_router_args(ctx, s);
		r = handle_update_instance(1, ctx, &r_last, update_instance_alias_icwmpd, 3, s, "router_instance", "router_alias");
		SUBENTRY(entry_method_root_router_sub, ctx, r);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_router_ipv4forwarding_instance(struct dmctx *ctx, char *irouter, char *iroute, char *permission)
{	
	IF_MATCH(ctx, DMROOT"Routing.Router.%s.IPv4Forwarding.%s.", irouter, iroute) {
		DMOBJECT(DMROOT"Routing.Router.%s.IPv4Forwarding.%s.", ctx, "0", 1, NULL, NULL, NULL, irouter, iroute);
		DMPARAM("Enable", ctx, permission, get_router_ipv4forwarding_enable, set_router_ipv4forwarding_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_router_ipv4forwarding_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Alias", ctx, "1", get_router_ipv4forwarding_alias, set_router_ipv4forwarding_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("DestIPAddress", ctx, permission, get_router_ipv4forwarding_destip, set_router_ipv4forwarding_destip, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("DestSubnetMask", ctx, permission, get_router_ipv4forwarding_destmask, set_router_ipv4forwarding_destmask, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SourceIPAddress", ctx, "0", get_router_ipv4forwarding_src_address, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SourceSubnetMask", ctx, "0", get_router_ipv4forwarding_src_mask, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("GatewayIPAddress", ctx, permission, get_router_ipv4forwarding_gatewayip, set_router_ipv4forwarding_gatewayip, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Interface", ctx, permission, get_router_ipv4forwarding_interface_linker_parameter, set_router_ipv4forwarding_interface_linker_parameter, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ForwardingMetric", ctx, permission, get_router_ipv4forwarding_metric, set_router_ipv4forwarding_metric, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

int entry_method_root_routing(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"Routing.") {
		DMOBJECT(DMROOT"Routing.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"Routing.Router.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("RouterNumberOfEntries", ctx, "0", get_router_nbr_entry, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		SUBENTRY(entry_method_router, ctx);
		return 0;
	}
	return FAULT_9005;
}

int entry_method_root_router_sub(struct dmctx *ctx, char *irouter)
{
	IF_MATCH(ctx, DMROOT"Routing.Router.%s.", irouter) {
		DMOBJECT(DMROOT"Routing.Router.%s.", ctx, "0", 1, NULL, NULL, NULL, irouter);
		DMPARAM("Alias", ctx, "1", get_router_alias, set_router_alias, NULL, 0, 1, UNDEF, NULL); //TODO
		DMOBJECT(DMROOT"Routing.Router.%s.IPv4Forwarding.", ctx, "0", 1, NULL, NULL, NULL, irouter);
		SUBENTRY(entry_router_ipv4forwarding, ctx, irouter);
	}
	return 0;
}




