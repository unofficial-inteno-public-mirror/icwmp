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
#include "dhcp.h"

struct dhcp_args cur_dhcp_args = {0};
struct dhcp_static_args cur_dhcp_staticargs = {0};

/*************************************************************
 * INIT
/*************************************************************/
inline int init_dhcp_args(struct dmctx *ctx, struct uci_section *s, char *interface)
{
	struct dhcp_args *args = &cur_dhcp_args;
	ctx->args = (void *)args;
	args->interface = interface;
	args->dhcp_sec = s;
	return 0;
}
inline int init_args_dhcp_host(struct dmctx *ctx, struct uci_section *s)
{
	struct dhcp_static_args *args = &cur_dhcp_staticargs;
	ctx->args = (void *)args;
	args->dhcpsection = s;
	return 0;
}
/*******************ADD-DEL OBJECT*********************/
int add_dhcp_server(struct dmctx *ctx, char **instancepara)
{
	char *value;
	char *instance;
	struct uci_section *s = NULL;
	
	instance = get_last_instance("dhcp", "dhcp", "dhcp_instance");
	dmuci_add_section("dhcp", "dhcp", &s, &value);
	dmuci_set_value_by_section(s, "start", "100");
	dmuci_set_value_by_section(s, "leasetime", "12h");
	dmuci_set_value_by_section(s, "limit", "150");
	*instancepara = update_instance(s, instance, "dhcp_instance");
	return 0;
}

int delete_dhcp_server(struct dmctx *ctx)
{
	dmuci_delete_by_section(cur_dhcp_args.dhcp_sec, NULL, NULL);
	return 0;
}

int delete_dhcp_server_all(struct dmctx *ctx)
{
	int found = 0;
	char *lan_name;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;

	uci_foreach_sections("dhcp", "dhcp", s) {
		if (found != 0)
			dmuci_delete_by_section(ss, NULL, NULL);
		ss = s;
		found++;
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	return 0;	
}

int add_dhcp_staticaddress(struct dmctx *ctx, char **instancepara)
{
	char *value;
	char *instance;
	struct uci_section *s = NULL;
	
	instance = get_last_instance_lev2("dhcp", "host", "ldhcpinstance", "interface", cur_dhcp_args.interface);
	dmuci_add_section("dhcp", "host", &s, &value);
	dmuci_set_value_by_section(s, "mac", DHCPSTATICADDRESS_DISABLED_CHADDR);
	dmuci_set_value_by_section(s, "interface", cur_dhcp_args.interface);
	*instancepara = update_instance(s, instance, "ldhcpinstance");
	return 0;
}

int delete_dhcp_staticaddress_all(struct dmctx *ctx)
{
	int found = 0;
	char *lan_name;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;

	uci_foreach_option_eq("dhcp", "host", "interface", cur_dhcp_args.interface, s) {
		if (found != 0)
			dmuci_delete_by_section(ss, NULL, NULL);
		ss = s;
		found++;
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	return 0;	
}

int delete_dhcp_staticaddress(struct dmctx *ctx)
{
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)ctx->args;
	
	dmuci_delete_by_section(dhcpargs->dhcpsection, NULL, NULL);
	return 0;
}
/*************************************************************
 * GET & SET PARAM
/*************************************************************/
int get_dns_server(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	int len;

	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
	if(res)
	json_parse_array(res, "dns-server", -1, NULL, value);
	else
		*value = "";
	if ((*value)[0] == '\0') {
		dmuci_get_option_value_string("network", cur_dhcp_args.interface, "dns", value);
		*value = dmstrdup(*value); // MEM WILL BE FREED IN DMMEMCLEAN
		char *p = *value;
		while (*p) {
			if (*p == ' ' && p != *value && *(p-1) != ',')
				*p++ = ',';
			else
				p++;
		}
	}
	return 0;
}

int set_dns_server(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *dup, *p;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dup = dmstrdup(value);
			p = dup;
			while (*p) {
				if (*p == ',')
					*p++ = ' ';
				else
					p++;
			}
			dmuci_set_value("network", cur_dhcp_args.interface, "dns", dup);
			dmfree(dup);
			return 0;
	}
	return 0;
}

int get_dhcp_configurable(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_section *s = NULL;

	uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
		*value = "1";
		return 0;
	}
	*value = "0";
	return 0;
}

int set_dhcp_configurable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
				if (!b) {
					dmuci_delete_by_section(s, NULL, NULL);
				}
				break;
			}
			if (s == NULL && b) {
				dmuci_set_value("dhcp",cur_dhcp_args.interface, NULL, "dhcp");
				dmuci_set_value("dhcp", cur_dhcp_args.interface, "interface", cur_dhcp_args.interface);
				dmuci_set_value("dhcp", cur_dhcp_args.interface, "start", "100");
				dmuci_set_value("dhcp", cur_dhcp_args.interface, "limit", "150");
				dmuci_set_value("dhcp", cur_dhcp_args.interface, "leasetime", "12h");
			}
			return 0;
	}
	return 0;
}

int get_dhcp_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_section *s = NULL;

	uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
		dmuci_get_value_by_section_string(s, "ignore", value);
		if ((*value)[0] == '\0')
			*value = "1";
		else
			*value = "0";
		return 0;
	}
	*value = "0";
	return 0;
}

int set_dhcp_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
				if (b)
					dmuci_set_value_by_section(s, "ignore", "");
				else
					dmuci_set_value_by_section(s, "ignore", "1");
				break;
			}
			return 0;
	}
	return 0;
}

enum enum_lanip_interval_address {
	LANIP_INTERVAL_START,
	LANIP_INTERVAL_END
};

int get_dhcp_interval_address(struct dmctx *ctx, char **value, int option)
{
	json_object *res;
	char *ipaddr = "" , *mask = "", *start , *limit;
	struct uci_section *s = NULL;
	char bufipstart[16], bufipend[16];

	*value = "";

	uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
		dmuci_get_value_by_section_string(s, "start", &start);
		if (option == LANIP_INTERVAL_END)
			dmuci_get_value_by_section_string(s, "limit", &limit);
		break;
	}
	if (s == NULL) {
		return 0;
	}
	dmuci_get_option_value_string("network", cur_dhcp_args.interface, "ipaddr", &ipaddr);
	if (ipaddr[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
		if (res)
			json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
	}
	if (ipaddr[0] == '\0') {
		return 0;
	}
	dmuci_get_option_value_string("network", cur_dhcp_args.interface, "netmask", &mask);
	if (mask[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
		if (res) {
			json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
			if (mask[0] == '\0') {
				return 0;
			}
			mask = cidr2netmask(atoi(mask));
		}
	}
	if (mask[0] == '\0') {
		mask = "255.255.255.0";
	}
	if (option == LANIP_INTERVAL_START) {
		ipcalc(ipaddr, mask, start, NULL, bufipstart, NULL);
		*value = dmstrdup(bufipstart); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	else {
		ipcalc(ipaddr, mask, start, limit, bufipstart, bufipend);
		*value = dmstrdup(bufipend); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
}
int get_dhcp_interval_address_min(char *refparam, struct dmctx *ctx, char **value)
{
	get_dhcp_interval_address(ctx, value, LANIP_INTERVAL_START);
	return 0;
}

int get_dhcp_interval_address_max(char *refparam, struct dmctx *ctx, char **value)
{
	get_dhcp_interval_address(ctx, value, LANIP_INTERVAL_END);
	return 0;
}

int set_dhcp_address_min(char *refparam, struct dmctx *ctx, int action, char *value)
{
	json_object *res;
	char *ipaddr = "", *mask = "", *start , *limit, buf[16];
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_option_value_string("network", cur_dhcp_args.interface, "ipaddr", &ipaddr);
			if (ipaddr[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
				if (res) {
					json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
				}
			}
			if (ipaddr[0] == '\0')
				return 0;

			dmuci_get_option_value_string("network", cur_dhcp_args.interface, "netmask", &mask);
			if (mask[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
				if (res) {
					json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
					if (mask[0] == '\0')
						return 0;
					mask = cidr2netmask(atoi(mask));
				}
			}
			if (mask[0] == '\0')
				mask = "255.255.255.0";

			ipcalc_rev_start(ipaddr, mask, value, buf);
			uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
				dmuci_set_value_by_section(s, "start", buf);
				break;
			}

			return 0;
	}
	return 0;
}

int set_dhcp_address_max(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int i_val;
	json_object *res;
	char *ipaddr = "", *mask = "", *start, buf[16];
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
				dmuci_get_value_by_section_string(s, "start", &start);
				break;
			}
			if (!s) return 0;

			dmuci_get_option_value_string("network", cur_dhcp_args.interface, "ipaddr", &ipaddr);
			if (ipaddr[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
				if (res) {
					json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
				}
			}
			if (ipaddr[0] == '\0')
				return 0;

			dmuci_get_option_value_string("network", cur_dhcp_args.interface, "netmask", &mask);
			if (mask[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
				if (res) {
					json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
					if (mask[0] == '\0')
						return 0;
					mask = cidr2netmask(atoi(mask));
				}
			}
			if (mask[0] == '\0')
				mask = "255.255.255.0";

			ipcalc_rev_end(ipaddr, mask, start, value, buf);
			dmuci_set_value_by_section(s, "limit", buf);
			return 0;
	}
	return 0;
}


int get_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, char **value)
{
	char val[512] = {0}, *p;
	struct uci_section *s = NULL;
	char *min, *max, *ip, *s_n_ip;
	unsigned int n_min, n_max, n_ip;
	*value = "";

	get_dhcp_interval_address(ctx, &min, LANIP_INTERVAL_START);
	get_dhcp_interval_address(ctx, &max, LANIP_INTERVAL_END);
	if (min[0] == '\0' || max[0] == '\0')
		return 0;
	n_min = inet_network(min);
	n_max = inet_network(max);
	p = val;
	uci_foreach_sections("dhcp", "host", s) {
		dmuci_get_value_by_section_string(s, "ip", &ip);
		if (ip[0] == '\0')
			continue;
		n_ip = inet_network(ip);
		if (n_ip >= n_min && n_ip <= n_max) {
			if (val[0] != '\0')
				dmstrappendchr(p, ',');
			dmstrappendstr(p, ip);
		}
	}
	dmstrappendend(p);
	*value = dmstrdup(val); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int set_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct uci_section *s = NULL;
	struct uci_section *dhcp_section = NULL;
	char *min, *max, *ip, *val, *local_value;
	char *pch, *spch;
	unsigned int n_min, n_max, n_ip;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			get_dhcp_interval_address(ctx, &min, LANIP_INTERVAL_START);
			get_dhcp_interval_address(ctx, &max, LANIP_INTERVAL_END);
			n_min = inet_network(min);
			n_max = inet_network(max);
			local_value = dmstrdup(value);

			for (pch = strtok_r(local_value, ",", &spch);
				pch != NULL;
				pch = strtok_r(NULL, ",", &spch)) {
				uci_foreach_option_eq("dhcp", "host", "ip", pch, s) {
					continue;
				}
				n_ip = inet_network(pch);
				if (n_ip < n_min && n_ip > n_max)
					continue;
				else {
					dmuci_add_section("dhcp", "host", &dhcp_section, &val);
					dmuci_set_value_by_section(dhcp_section, "mac", DHCPSTATICADDRESS_DISABLED_CHADDR);
					dmuci_set_value_by_section(dhcp_section, "interface", cur_dhcp_args.interface);
					dmuci_set_value_by_section(dhcp_section, "ip", pch);
				}
			}
			dmfree(local_value);
			uci_foreach_sections("dhcp", "host", s) {
				dmuci_get_value_by_section_string(s, "ip", &ip);
				n_ip =	inet_network(ip);
				if (n_ip >= n_min && n_ip <= n_max)
					dmuci_delete_by_section(s, "ip", NULL);
			}
			return 0;
	}
	return 0;
}

int get_dhcp_subnetmask(char *refparam, struct dmctx *ctx, char **value)
{
	char *mask;
	json_object *res;
	struct uci_section *s = NULL;
	char *val;
	*value = "";

	uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
		dmuci_get_value_by_section_string(s, "netmask", value);
		break;
	}
	if (s == NULL || (*value)[0] == '\0')
	dmuci_get_option_value_string("network", cur_dhcp_args.interface, "netmask", value);
	if ((*value)[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", cur_dhcp_args.interface}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
		int i_mask = atoi(mask);
		val = cidr2netmask(i_mask);
		*value = dmstrdup(val);// MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
}

int set_dhcp_subnetmask(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
				dmuci_set_value_by_section(s, "netmask", value);
				return 0;
			}
			return 0;
	}
	return 0;
}

int get_dhcp_iprouters(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("network", cur_dhcp_args.interface, "gateway", value);
	if ((*value)[0] == '\0') {
		dmuci_get_option_value_string("network", cur_dhcp_args.interface, "ipaddr", value);
	}
	return 0;
}

int set_dhcp_iprouters(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("network", cur_dhcp_args.interface, "gateway", value);
			return 0;
	}
	return 0;
}

int get_dhcp_leasetime(char *refparam, struct dmctx *ctx, char **value)
{
	int len, mtime = 0;
	char *ltime = "", *pch, *spch, *ltime_ini, *tmp, *tmp_ini;
	struct uci_section *s = NULL;

	uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
		dmuci_get_value_by_section_string(s, "leasetime", &ltime);
		break;
	}
	if (ltime[0] == '\0') {
		*value = "-1";
		return 0;
	}
	ltime = dmstrdup(ltime);
	ltime_ini = dmstrdup(ltime);
	tmp = ltime;
	tmp_ini = ltime_ini;
	pch = strtok_r(ltime, "h", &spch);
	if (strcmp(pch, ltime_ini) != 0) {
		mtime = 3600 * atoi(pch);
		if(spch[0] != '\0') {
			ltime += strlen(pch)+1;
			ltime_ini += strlen(pch)+1;
			pch = strtok_r(ltime, "m", &spch);
			if (strcmp(pch, ltime_ini) != 0) {
				mtime += 60 * atoi(pch);
				if(spch[0] !='\0') {
					ltime += strlen(pch)+1;
					ltime_ini += strlen(pch)+1;
					pch = strtok_r(ltime, "s", &spch);
					if (strcmp(pch, ltime_ini) != 0) {
						mtime += atoi(pch);
					}
				}
			} else {
				pch = strtok_r(ltime, "s", &spch);
	if (strcmp(pch, ltime_ini) != 0)
				mtime +=  atoi(pch);
			}
		}
	}
	else {
		pch = strtok_r(ltime, "m", &spch);
		if (strcmp(pch, ltime_ini) != 0) {
		mtime += 60 * atoi(pch);
			if(spch[0] !='\0') {
				ltime += strlen(pch)+1;
				ltime_ini += strlen(pch)+1;
				pch = strtok_r(ltime, "s", &spch);
				if (strcmp(pch, ltime_ini) != 0) {
					mtime += atoi(pch);
				}
			}
		} else {
			pch = strtok_r(ltime, "s", &spch);
			if (strcmp(pch, ltime_ini) != 0)
				mtime +=  atoi(pch);
		}
	}
	dmfree(tmp);
	dmfree(tmp_ini);

	dmasprintf(value, "%d", mtime); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int set_dhcp_leasetime(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct uci_section *s = NULL;
	char buf[32];

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
				sprintf(buf, "%dm", (atoi(value) / 60));
				dmuci_set_value_by_section(s, "leasetime",  buf);
				break;
			}
			return 0;
	}
	return 0;
}

int get_dhcp_domainname(char *refparam, struct dmctx *ctx, char **value)
{
	char *result, *str;
	struct uci_list *val;
	struct uci_element *e = NULL;
	struct uci_section *s = NULL;
	*value = "";

	uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
		dmuci_get_value_by_section_list(s, "dhcp_option", &val);
		if (val) {
			uci_foreach_element(val, e)
			{
				if (str = strstr(e->name, "15,")) {
					*value = dmstrdup(str + sizeof("15,") - 1); //MEM WILL BE FREED IN DMMEMCLEAN
					return 0;
				}
			}
		}
	}
	return 0;
}

int set_dhcp_domainname(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *result, *dn, *pch;
	struct uci_list *val;
	struct uci_section *s = NULL;
	struct uci_element *e = NULL, *tmp;
	char *option = "dhcp_option", buf[64];

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("dhcp", "dhcp", "interface", cur_dhcp_args.interface, s) {
				dmuci_get_value_by_section_list(s, option, &val);
				if (val) {
					uci_foreach_element_safe(val, e, tmp)
					{
						if (strstr(tmp->name, "15,")) {
							dmuci_del_list_value_by_section(s, "dhcp_option", tmp->name); //TODO test it
						}
					}
				}
				break;
			}
			goto end;
	}
end:
	sprintf(buf, "15,%s", value);
	dmuci_add_list_value("dhcp", cur_dhcp_args.interface, "dhcp_option", buf);
	return 0;
}

int get_dhcp_static_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dhcp_staticargs.dhcpsection, "ldhcpalias", value);
	return 0;
}

int set_dhcp_static_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_dhcp_staticargs.dhcpsection, "ldhcpalias", value);
			return 0;
	}
	return 0;
}

int get_dhcp_staticaddress_chaddr(char *refparam, struct dmctx *ctx, char **value)
{
	char *chaddr;
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)ctx->args;
	
	dmuci_get_value_by_section_string(dhcpargs->dhcpsection, "mac", &chaddr);
	if (strcmp(chaddr, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0)
		dmuci_get_value_by_section_string(dhcpargs->dhcpsection, "mac_orig", value);
	else 
		*value = chaddr;
	return 0;
}

int set_dhcp_staticaddress_chaddr(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	char *chaddr;
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)ctx->args;
		
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(dhcpargs->dhcpsection, "mac", &chaddr);
			if (strcmp(chaddr, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0)
				dmuci_set_value_by_section(dhcpargs->dhcpsection, "mac_orig", value);
			else
				dmuci_set_value_by_section(dhcpargs->dhcpsection, "mac", value);
			return 0;
	}
	return 0;
}

int get_dhcp_staticaddress_yiaddr(char *refparam, struct dmctx *ctx, char **value)
{
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)ctx->args;
	
	dmuci_get_value_by_section_string(dhcpargs->dhcpsection, "ip", value);
	return 0;
}

int set_dhcp_staticaddress_yiaddr(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(dhcpargs->dhcpsection, "ip", value);
			return 0;
	}
	return 0;
}
/*************************************************************
 * ENTRY METHOD
/*************************************************************/
int entry_method_root_dhcp(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"DHCPv4.") {
		DMOBJECT(DMROOT"DHCPv4.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"DHCPv4.Server.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"DHCPv4.Server.Pool.", ctx, "0", 1, add_dhcp_server, delete_dhcp_server_all, NULL);
		SUBENTRY(entry_dhcp, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_dhcp(struct dmctx *ctx)
{
	struct uci_section *s;
	char *interface, *idhcp = NULL, *idhcp_last = NULL;
	uci_foreach_sections("dhcp","dhcp", s) {
		dmuci_get_value_by_section_string(s, "interface", &interface);
		init_dhcp_args(ctx, s, interface);
		idhcp = handle_update_instance(1, ctx, &idhcp_last, update_instance_alias, 3, s, "dhcp_instance", "dhcp_alias");
		SUBENTRY(entry_dhcp_instance, ctx, interface, idhcp);
	}

	return 0;
}

inline int entry_dhcp_instance(struct dmctx *ctx, char *interface, char *int_num)
{
	IF_MATCH(ctx, DMROOT"DHCPv4.Server.Pool.%s.", int_num) {
		DMOBJECT(DMROOT"DHCPv4.Server.Pool.%s.", ctx, "0", NULL, NULL, delete_dhcp_server, NULL, int_num);
		DMPARAM("DNSServers", ctx, "1", get_dns_server, set_dns_server, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_DHCPServerConfigurable", ctx, "1", get_dhcp_configurable, set_dhcp_configurable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_dhcp_enable, set_dhcp_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("MinAddress", ctx, "1", get_dhcp_interval_address_min, set_dhcp_address_min, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("MaxAddress", ctx, "1", get_dhcp_interval_address_max, set_dhcp_address_max, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ReservedAddresses", ctx, "1", get_dhcp_reserved_addresses, set_dhcp_reserved_addresses, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SubnetMask", ctx, "1", get_dhcp_subnetmask, set_dhcp_subnetmask, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("IPRouters", ctx, "1", get_dhcp_iprouters, set_dhcp_iprouters, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LeaseTime", ctx, "1", get_dhcp_leasetime, set_dhcp_leasetime, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("DomainName", ctx, "1", get_dhcp_domainname, set_dhcp_domainname, NULL, 0, 1, UNDEF, NULL);
		//DMPARAM("Interface", ctx, "1", get_lan_dhcp_domainname, set_lan_dhcp_domainname, NULL, 0, 1, UNDEF, NULL); // refer to  IP.Interface
		DMOBJECT(DMROOT"DHCPv4.Server.Pool.%s.StaticAddress.", ctx, "0", NULL, add_dhcp_staticaddress, delete_dhcp_staticaddress_all, NULL, int_num); //TODO
		SUBENTRY(entry_dhcp_static_address, ctx, interface, int_num);		
		return 0;
	}
	return FAULT_9005;
}


inline int entry_dhcp_static_address(struct dmctx *ctx, char *interface, char *idev)
{
	struct uci_section *sss = NULL;
	char *idhcp = NULL, *idhcp_last = NULL;
	uci_foreach_option_cont("dhcp", "host", "interface", interface, sss) {
		idhcp = handle_update_instance(2, ctx, &idhcp_last, update_instance_alias, 3, sss, "ldhcpinstance", "ldhcpalias");
		init_args_dhcp_host(ctx, sss);
		SUBENTRY(entry_dhcp_static_address_instance, ctx, idev, idhcp);
	}
}

inline int entry_dhcp_static_address_instance(struct dmctx *ctx, char *int_num, char *st_address)
{
	IF_MATCH(ctx, DMROOT"DHCPv4.Server.Pool.%s.StaticAddress.%s.", int_num, st_address) {
		DMOBJECT(DMROOT"DHCPv4.Server.Pool.%s.StaticAddress.%s.", ctx, "1", NULL, NULL, delete_dhcp_staticaddress, NULL, int_num, st_address);
		DMPARAM("Alias", ctx, "1", get_dhcp_static_alias, set_dhcp_static_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Chaddr", ctx, "1", get_dhcp_staticaddress_chaddr, set_dhcp_staticaddress_chaddr, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Yiaddr", ctx, "1", get_dhcp_staticaddress_yiaddr, set_dhcp_staticaddress_yiaddr, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
