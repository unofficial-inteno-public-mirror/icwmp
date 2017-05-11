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

/**************************************************************************
* LINKER
***************************************************************************/
int get_dhcp_client_linker(char *refparam, struct dmctx *dmctx, void *data, char *instance, char **linker) {
	if (((struct client_args *)data)->key) {
		*linker = ((struct client_args *)data)->key;
		return 0;
	} else {
		*linker = "";
		return 0;
	}
}

/*************************************************************
 * INIT
/*************************************************************/
inline int init_dhcp_args(struct dhcp_args *args, struct uci_section *s, char *interface)
{
	args->interface = interface;
	args->dhcp_sec = s;
	return 0;
}
inline int init_args_dhcp_host(struct dhcp_static_args *args, struct uci_section *s)
{
	args->dhcpsection = s;
	return 0;
}

inline int init_dhcp_client_args(struct client_args *args, json_object *client, char *key)
{
	args->client = client;
	args->key = key;
	return 0;
}

/*******************ADD-DEL OBJECT*********************/
int add_dhcp_server(char *refparam, struct dmctx *ctx, void *data, char **instancepara)
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

int delete_dhcp_server(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	int found = 0;
	char *lan_name;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;

	switch (del_action) {
	case DEL_INST:
		dmuci_delete_by_section(((struct dhcp_args *)data)->dhcp_sec, NULL, NULL);
		break;
	case DEL_ALL:
		uci_foreach_sections("dhcp", "dhcp", s) {
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

int add_dhcp_staticaddress(char *refparam, struct dmctx *ctx, void *data, char **instancepara)
{
	char *value;
	char *instance;
	struct uci_section *s = NULL;
	
	instance = get_last_instance_lev2("dhcp", "host", "ldhcpinstance", "interface", ((struct dhcp_args *)data)->interface);
	dmuci_add_section("dhcp", "host", &s, &value);
	dmuci_set_value_by_section(s, "mac", DHCPSTATICADDRESS_DISABLED_CHADDR);
	dmuci_set_value_by_section(s, "interface", ((struct dhcp_args *)data)->interface);
	*instancepara = update_instance(s, instance, "ldhcpinstance");
	return 0;
}

int delete_dhcp_staticaddress(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	int found = 0;
	char *lan_name;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)data;
	
	switch (del_action) {
		case DEL_INST:
			dmuci_delete_by_section(dhcpargs->dhcpsection, NULL, NULL);
			break;
		case DEL_ALL:
			uci_foreach_option_eq("dhcp", "host", "interface", ((struct dhcp_args *)data)->interface, s) {
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
/*************************************************************
 * GET & SET PARAM
/*************************************************************/
int get_dns_server(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	json_object *res;
	int len;

	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
	if(res)
	json_parse_array(res, "dns-server", -1, NULL, value);
	else
		*value = "";
	if ((*value)[0] == '\0') {
		dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "dns", value);
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

int set_dns_server(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
			dmuci_set_value("network", ((struct dhcp_args *)data)->interface, "dns", dup);
			dmfree(dup);
			return 0;
	}
	return 0;
}

int get_dhcp_configurable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *s = NULL;

	uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
		*value = "1";
		return 0;
	}
	*value = "0";
	return 0;
}

int set_dhcp_configurable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
			uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
				if (!b) {
					dmuci_delete_by_section(s, NULL, NULL);
				}
				break;
			}
			if (s == NULL && b) {
				dmuci_set_value("dhcp",((struct dhcp_args *)data)->interface, NULL, "dhcp");
				dmuci_set_value("dhcp", ((struct dhcp_args *)data)->interface, "interface", ((struct dhcp_args *)data)->interface);
				dmuci_set_value("dhcp", ((struct dhcp_args *)data)->interface, "start", "100");
				dmuci_set_value("dhcp", ((struct dhcp_args *)data)->interface, "limit", "150");
				dmuci_set_value("dhcp", ((struct dhcp_args *)data)->interface, "leasetime", "12h");
			}
			return 0;
	}
	return 0;
}

int get_dhcp_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *s = NULL;

	uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
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

int set_dhcp_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
			uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
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

int get_dhcp_interval_address(struct dmctx *ctx, void *data, char *instance, char **value, int option)
{
	json_object *res;
	char *ipaddr = "" , *mask = "", *start , *limit;
	struct uci_section *s = NULL;
	char bufipstart[16], bufipend[16];

	*value = "";

	uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
		dmuci_get_value_by_section_string(s, "start", &start);
		if (option == LANIP_INTERVAL_END)
			dmuci_get_value_by_section_string(s, "limit", &limit);
		break;
	}
	if (s == NULL) {
		return 0;
	}
	dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "ipaddr", &ipaddr);
	if (ipaddr[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
		if (res)
			json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
	}
	if (ipaddr[0] == '\0') {
		return 0;
	}
	dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "netmask", &mask);
	if (mask[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
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
int get_dhcp_interval_address_min(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	get_dhcp_interval_address(ctx, data, instance, value, LANIP_INTERVAL_START);
	return 0;
}

int get_dhcp_interval_address_max(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	get_dhcp_interval_address(ctx, data, instance, value, LANIP_INTERVAL_END);
	return 0;
}

int set_dhcp_address_min(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	json_object *res;
	char *ipaddr = "", *mask = "", *start , *limit, buf[16];
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "ipaddr", &ipaddr);
			if (ipaddr[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
				if (res) {
					json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
				}
			}
			if (ipaddr[0] == '\0')
				return 0;

			dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "netmask", &mask);
			if (mask[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
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
			uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
				dmuci_set_value_by_section(s, "start", buf);
				break;
			}

			return 0;
	}
	return 0;
}

int set_dhcp_address_max(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	int i_val;
	json_object *res;
	char *ipaddr = "", *mask = "", *start, buf[16];
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
				dmuci_get_value_by_section_string(s, "start", &start);
				break;
			}
			if (!s) return 0;

			dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "ipaddr", &ipaddr);
			if (ipaddr[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
				if (res) {
					json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
				}
			}
			if (ipaddr[0] == '\0')
				return 0;

			dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "netmask", &mask);
			if (mask[0] == '\0') {
				dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
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


int get_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char val[512] = {0}, *p;
	struct uci_section *s = NULL;
	char *min, *max, *ip, *s_n_ip;
	unsigned int n_min, n_max, n_ip;
	*value = "";

	get_dhcp_interval_address(ctx, data, instance, &min, LANIP_INTERVAL_START);
	get_dhcp_interval_address(ctx, data, instance, &max, LANIP_INTERVAL_END);
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

int set_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
			get_dhcp_interval_address(ctx, data, instance, &min, LANIP_INTERVAL_START);
			get_dhcp_interval_address(ctx, data, instance, &max, LANIP_INTERVAL_END);
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
					dmuci_set_value_by_section(dhcp_section, "interface", ((struct dhcp_args *)data)->interface);
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

int get_dhcp_subnetmask(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *mask;
	json_object *res;
	struct uci_section *s = NULL;
	char *val;
	*value = "";

	uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
		dmuci_get_value_by_section_string(s, "netmask", value);
		break;
	}
	if (s == NULL || (*value)[0] == '\0')
	dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "netmask", value);
	if ((*value)[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ((struct dhcp_args *)data)->interface}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
		int i_mask = atoi(mask);
		val = cidr2netmask(i_mask);
		*value = dmstrdup(val);// MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
}

int set_dhcp_subnetmask(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct uci_section *s = NULL;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
				dmuci_set_value_by_section(s, "netmask", value);
				return 0;
			}
			return 0;
	}
	return 0;
}

int get_dhcp_iprouters(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "gateway", value);
	if ((*value)[0] == '\0') {
		dmuci_get_option_value_string("network", ((struct dhcp_args *)data)->interface, "ipaddr", value);
	}
	return 0;
}

int set_dhcp_iprouters(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("network", ((struct dhcp_args *)data)->interface, "gateway", value);
			return 0;
	}
	return 0;
}

int get_dhcp_leasetime(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int len, mtime = 0;
	char *ltime = "", *pch, *spch, *ltime_ini, *tmp, *tmp_ini;
	struct uci_section *s = NULL;

	uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
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

int set_dhcp_leasetime(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct uci_section *s = NULL;
	char buf[32];

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
				int val = atoi(value);
				sprintf(buf, "%ds", val);
				dmuci_set_value_by_section(s, "leasetime",  buf);
				break;
			}
			return 0;
	}
	return 0;
}

int get_dhcp_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *linker;
	linker = dmstrdup(((struct dhcp_args *)data)->interface);
	adm_entry_get_linker_param(ctx, dm_print_path("%s%cIP%cInterface%c", DMROOT, dm_delim, dm_delim, dm_delim), linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
	if (*value == NULL)
		*value = "";
	dmfree(linker);
	return 0;
}

int set_dhcp_interface_linker_parameter(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	char *linker;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(ctx, value, &linker);
			if (linker) {
				dmuci_set_value_by_section(((struct dhcp_args *)data)->dhcp_sec, "interface", linker);
				dmfree(linker);
			}
			return 0;
	}
	return 0;
}

int get_dhcp_domainname(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *result, *str;
	struct uci_list *val;
	struct uci_element *e = NULL;
	struct uci_section *s = NULL;
	*value = "";

	uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
		dmuci_get_value_by_section_list(s, "dhcp_option", &val);
		if (val) {
			uci_foreach_element(val, e)
			{
				if ((str = strstr(e->name, "15,"))) {
					*value = dmstrdup(str + sizeof("15,") - 1); //MEM WILL BE FREED IN DMMEMCLEAN
					return 0;
				}
			}
		}
	}
	return 0;
}

int set_dhcp_domainname(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
			uci_foreach_option_eq("dhcp", "dhcp", "interface", ((struct dhcp_args *)data)->interface, s) {
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
	dmuci_add_list_value_by_section(((struct dhcp_args *)data)->dhcp_sec, "dhcp_option", buf);
	return 0;
}

int get_dhcp_static_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string(((struct dhcp_static_args *)data)->dhcpsection, "ldhcpalias", value);
	return 0;
}

int set_dhcp_static_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(((struct dhcp_static_args *)data)->dhcpsection, "ldhcpalias", value);
			return 0;
	}
	return 0;
}

int get_dhcp_staticaddress_chaddr(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *chaddr;
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)data;
	
	dmuci_get_value_by_section_string(dhcpargs->dhcpsection, "mac", &chaddr);
	if (strcmp(chaddr, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0)
		dmuci_get_value_by_section_string(dhcpargs->dhcpsection, "mac_orig", value);
	else 
		*value = chaddr;
	return 0;
}

int set_dhcp_staticaddress_chaddr(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{	
	char *chaddr;
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)data;
		
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

int get_dhcp_staticaddress_yiaddr(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)data;
	
	dmuci_get_value_by_section_string(dhcpargs->dhcpsection, "ip", value);
	return 0;
}

int set_dhcp_staticaddress_yiaddr(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct dhcp_static_args *dhcpargs = (struct dhcp_static_args *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(dhcpargs->dhcpsection, "ip", value);
			return 0;
	}
	return 0;
}

int get_dhcp_client_chaddr(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	json_select(((struct client_args *)data)->client, "macaddr", 0, NULL, value, NULL);
	return 0;
}

/*************************************************************
 * ENTRY METHOD
/*************************************************************/


/*** DHCPv4. ***/
DMOBJ tDhcpv4Obj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf, linker*/
{"Server", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tDhcpv4ServerObj, NULL, NULL},
{0}
};

/*** DHCPv4.Server. ***/
DMOBJ tDhcpv4ServerObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf, linker*/
{"Pool", &DMWRITE, add_dhcp_server, delete_dhcp_server, NULL, browseDhcpInst, NULL, NULL, tDhcpServerPoolObj, tDhcpServerPoolParams, NULL},
{0}
};


/*** DHCPv4.Server.Pool.{i}. ***/
DMOBJ tDhcpServerPoolObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf, linker*/
{"StaticAddress", &DMWRITE, add_dhcp_staticaddress, delete_dhcp_staticaddress, NULL, browseDhcpStaticInst, NULL, NULL, NULL, tDhcpServerPoolAddressParams, NULL},
{"Client", &DMREAD, NULL, NULL, NULL, browseDhcpClientInst, NULL, NULL, NULL, tDhcpServerPoolClientParams, get_dhcp_client_linker},
{0}
};

DMLEAF tDhcpServerPoolParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"DNSServers", &DMWRITE, DMT_STRING,  get_dns_server, set_dns_server, NULL, NULL},
{"X_INTENO_SE_DHCPServerConfigurable", &DMWRITE, DMT_BOOL, get_dhcp_configurable, set_dhcp_configurable, NULL, NULL},
{"Enable", &DMWRITE, DMT_BOOL,  get_dhcp_enable, set_dhcp_enable, NULL, NULL},
{"MinAddress", &DMWRITE, DMT_STRING, get_dhcp_interval_address_min, set_dhcp_address_min, NULL, NULL},
{"MaxAddress", &DMWRITE, DMT_STRING,get_dhcp_interval_address_max, set_dhcp_address_max, NULL, NULL},
{"ReservedAddresses", &DMWRITE, DMT_STRING, get_dhcp_reserved_addresses, set_dhcp_reserved_addresses, NULL, NULL},
{"SubnetMask", &DMWRITE, DMT_STRING,get_dhcp_subnetmask, set_dhcp_subnetmask, NULL, NULL},
{"IPRouters", &DMWRITE, DMT_STRING, get_dhcp_iprouters, set_dhcp_iprouters, NULL, NULL},
{"LeaseTime", &DMWRITE, DMT_STRING, get_dhcp_leasetime, set_dhcp_leasetime, NULL, NULL},
{"DomainName", &DMWRITE, DMT_STRING, get_dhcp_domainname, set_dhcp_domainname, NULL, NULL},
{"Interface", &DMWRITE, DMT_STRING, get_dhcp_interface, set_dhcp_interface_linker_parameter, NULL, NULL},
{0}
};

/*** DHCPv4.Server.Pool.{i}.StaticAddress.{i}. ***/
DMLEAF tDhcpServerPoolAddressParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"Alias", &DMWRITE, DMT_STRING, get_dhcp_static_alias, set_dhcp_static_alias, NULL, NULL},
{"Chaddr", &DMWRITE, DMT_STRING,  get_dhcp_staticaddress_chaddr, set_dhcp_staticaddress_chaddr, NULL, NULL},
{"Yiaddr", &DMWRITE, DMT_STRING,  get_dhcp_staticaddress_yiaddr, set_dhcp_staticaddress_yiaddr, NULL, NULL},
{0}
};

/*** DHCPv4.Server.Pool.{i}.Client.{i}. ***/
DMLEAF tDhcpServerPoolClientParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"Chaddr", &DMREAD, DMT_STRING,  get_dhcp_client_chaddr, NULL, NULL, NULL},
{0}
};

int browseDhcpInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *s;
	char *interface, *idhcp = NULL, *idhcp_last = NULL;
	struct dhcp_args curr_dhcp_args = {0};
	uci_foreach_sections("dhcp","dhcp", s) {
		dmuci_get_value_by_section_string(s, "interface", &interface);
		init_dhcp_args(&curr_dhcp_args, s, interface);
		idhcp = handle_update_instance(1, dmctx, &idhcp_last, update_instance_alias, 3, s, "dhcp_instance", "dhcp_alias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_dhcp_args, idhcp) == DM_STOP)
			break;
	}
	return 0;
}

int browseDhcpStaticInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *sss = NULL;
	char *idhcp = NULL, *idhcp_last = NULL;
	struct dhcp_static_args curr_dhcp_staticargs = {0};

	uci_foreach_option_cont("dhcp", "host", "interface", ((struct dhcp_args *)prev_data)->interface, sss) {
		idhcp = handle_update_instance(2, dmctx, &idhcp_last, update_instance_alias, 3, sss, "ldhcpinstance", "ldhcpalias");
		init_args_dhcp_host(&curr_dhcp_staticargs, sss);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_dhcp_staticargs, idhcp) == DM_STOP)
			break;
	}
	return 0;
}

int browseDhcpClientInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *sss = NULL;
	char *idx = NULL, *idx_last = NULL;
	json_object *res = NULL, *client_obj = NULL;
	char *dhcp, *network;
	int id = 0;
	struct client_args curr_dhcp_client_args = {0};

	dmubus_call("router.network", "clients", UBUS_ARGS{}, 0, &res);
	if (res) {
		json_object_object_foreach(res, key, client_obj) {
			json_select(client_obj, "dhcp", 0, NULL, &dhcp, NULL);
			if(strcmp(dhcp, "true") == 0)
			{
				json_select(client_obj, "network", 0, NULL, &network, NULL);
				if(strcmp(network, ((struct dhcp_args *)prev_data)->interface) == 0)
				{
					init_dhcp_client_args(&curr_dhcp_client_args, client_obj, key);
					idx = handle_update_instance(2, dmctx, &idx_last, update_instance_without_section, 1, ++id);
					if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_dhcp_client_args, idx) == DM_STOP)
						break;
				}
			}
		}
	}
	return 0;
}

