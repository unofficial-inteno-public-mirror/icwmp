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
#include "hosts.h"

struct host_args cur_host_args = {0};
inline int browsehostInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

/*************************************************************
 * INIT
/*************************************************************/
inline int init_host_args(struct dmctx *ctx, json_object *clients, char *key)
{
	struct host_args *args = &cur_host_args;
	args->client = clients;
	args->key = key;
	return 0;
}
/*************************************************************
 * GET & SET PARAM
/*************************************************************/
int get_host_ipaddress(char *refparam, struct dmctx *ctx, char **value)
{
	json_select(cur_host_args.client, "ipaddr", 0, NULL, value, NULL);
	return 0;
}

int get_host_hostname(char *refparam, struct dmctx *ctx, char **value)
{
	json_select(cur_host_args.client, "hostname", 0, NULL, value, NULL);
	return 0;
}

int get_host_active(char *refparam, struct dmctx *ctx, char **value)
{
	json_select(cur_host_args.client, "connected", 0, NULL, value, NULL);
	return 0;
}

int get_host_phy_address(char *refparam, struct dmctx *ctx, char **value)
{
	json_select(cur_host_args.client, "macaddr", 0, NULL, value, NULL);
	return 0;
}

int get_host_address_source(char *refparam, struct dmctx *ctx, char **value) {
	char *dhcp;

	json_select(cur_host_args.client, "dhcp", 0, NULL, &dhcp, NULL);
	if (strcasecmp(dhcp, "true") == 0)
		*value = "DHCP";
	else
		*value = "Static";
	return 0;
}

int get_host_leasetime_remaining(char *refparam, struct dmctx *ctx, char **value)
{
	char buf[80], *dhcp;
	FILE *fp;
	char line[MAX_DHCP_LEASES];
	struct tm ts;
	char *leasetime, *mac_f, *mac, *line1;
	char delimiter[] = " \t";

	json_select(cur_host_args.client, "dhcp", 0, NULL, &dhcp, NULL);
	if (strcmp(dhcp, "false") == 0) {
		*value = "0";
	}
	else {
		json_select(cur_host_args.client, "macaddr", 0, NULL, &mac, NULL);
		//
		fp = fopen(ARP_FILE, "r");
		if ( fp != NULL)
		{
			while (fgets(line, MAX_DHCP_LEASES, fp) != NULL )
			{
				if (line[0] == '\n')
					continue;
				line1 = dmstrdup(line);
				leasetime = cut_fx(line, delimiter, 1);
				mac_f = cut_fx(line1, delimiter, 2);
				if (strcasecmp(mac, mac_f) == 0) {
					int rem_lease = atoi(leasetime) - time(NULL);
					if (rem_lease < 0)
						*value = "-1";
					else
						dmasprintf(value, "%d", rem_lease); // MEM WILL BE FREED IN DMMEMCLEAN
					fclose(fp) ;
					return 0;
				}
			}
			fclose(fp);
		}
	}
	return 0;
}

int  get_host_dhcp_client(char *refparam, struct dmctx *ctx, char **value)
{
	char *iface, *linker;
		dmastrcat(&linker, "linker_dhcp:", cur_host_args.key);
		adm_entry_get_linker_param(ctx, dm_print_path("%s%cDHCPv4%c", DMROOT, dm_delim, dm_delim), linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
		if (*value == NULL) {
			*value = "";
		}
		dmfree(linker);
	return 0;
}

char *get_interface_type(char *mac, char *ndev)
{
	json_object *res;
	int wlctl_num;
	struct uci_section *s, *d;
	char *network, *device, *value, *wunit;
	char buf[8], *p;

	uci_foreach_sections("wireless", "wifi-device", d) {
		wlctl_num = 0;
		wunit = section_name(d);
		uci_foreach_option_eq("wireless", "wifi-iface", "device", wunit, s) {
			dmuci_get_value_by_section_string(s, "network", &network);
			if (strcmp(network, ndev) == 0) {
				if (wlctl_num != 0) {
					sprintf(buf, "%s.%d", wunit, wlctl_num);
					p = buf;
				}
				else {
					p = wunit;
				}
				dmubus_call("router.wireless", "stas", UBUS_ARGS{{"vif", p}}, 1, &res);
				if(res) {
					json_object_object_foreach(res, key, val) {
						json_select(val, "macaddr", 0, NULL, &value, NULL);
						if (strcasecmp(value, mac) == 0)
							return "802.11";
					}
				}
				wlctl_num++;
			}
		}
	}
	return "Ethernet";
}

int get_host_interfacetype(char *refparam, struct dmctx *ctx, char **value)
{
	char *mac, *network;
	
	json_select(cur_host_args.client, "macaddr", 0, NULL, &mac, NULL);
	json_select(cur_host_args.client, "network", 0, NULL, &network, NULL);
	*value = get_interface_type(mac, network);
	return 0;
}

int get_host_nbr_entries(char *refparam, struct dmctx *ctx, char **value)
{
	int entries = 0;
	json_object *res;

	dmubus_call("router.network", "clients", UBUS_ARGS{}, 0, &res);
	DM_ASSERT(res, *value = "0");
	json_object_object_foreach(res, key, client_obj) {
		entries++;
	}
	dmasprintf(value, "%d", entries); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

DMLEAF thostsParam[] = {
{"HostNumberOfEntries", &DMREAD, DMT_UNINT, get_host_nbr_entries, NULL, NULL, NULL},
{0}
};

DMLEAF thostParam[] = {
{"IPAddress", &DMREAD, DMT_STRING, get_host_ipaddress, NULL, NULL, &DMNONE},
{"HostName", &DMREAD, DMT_STRING, get_host_hostname, NULL, NULL, &DMNONE},
{"Active", &DMREAD, DMT_BOOL, get_host_active, NULL, NULL, &DMNONE},
{"PhysAddress", &DMREAD, DMT_STRING, get_host_phy_address, NULL, NULL, &DMNONE},
{"X_INTENO_SE_InterfaceType", &DMREAD, DMT_STRING, get_host_interfacetype, NULL, NULL, &DMNONE},
{"AddressSource", &DMREAD, DMT_STRING, get_host_address_source, NULL, NULL, &DMNONE},
{"LeaseTimeRemaining", &DMREAD, DMT_STRING, get_host_leasetime_remaining, NULL, NULL, &DMNONE},
{"DHCPClient", &DMREAD, DMT_STRING, get_host_dhcp_client, NULL, NULL, NULL},
{0}
};

DMOBJ thostsObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"Host", &DMREAD, NULL, NULL, NULL, browsehostInst, NULL, NULL, NULL, thostParam, NULL},
{0}
};

/*************************************************************
 * ENTRY METHOD
/*************************************************************/
inline int browsehostInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	json_object *res, *client_obj;
	char *idx, *idx_last = NULL;
	int id = 0;
	dmubus_call("router.network", "clients", UBUS_ARGS{}, 0, &res);
	if (res) {
		json_object_object_foreach(res, key, client_obj) {
			init_host_args(dmctx, client_obj, key);
			idx = handle_update_instance(2, dmctx, &idx_last, update_instance_without_section, 1, ++id);
			if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, idx) == DM_STOP)
				break;
		}
	}
	DM_CLEAN_ARGS(cur_host_args);
	return 0;
}

