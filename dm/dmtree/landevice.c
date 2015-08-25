/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#include <ctype.h>
#include <uci.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "landevice.h"
#define DELIMITOR ","
#define TAILLE 10
char *DHCPSTATICADDRESS_DISABLED_CHADDR="00:00:00:00:00:01";

struct ldlanargs
{
	struct uci_section *ldlansection;
};

struct ldipargs
{
	struct uci_section *ldipsection;
};

struct lddhcpargs
{
	struct uci_section *lddhcpsection;
};

struct ldwlanargs
{
	struct uci_section *lwlansection;
	int wlctl_num;
	char *wunit;
	int pki;
};

struct ldethargs
{
	char *eth;
};

struct ldlanargs cur_lanargs = {0};
struct ldipargs cur_ipargs = {0};
struct lddhcpargs cur_dhcpargs = {0};
struct ldwlanargs cur_wlanargs = {0};
struct ldethargs cur_ethargs = {0};

inline int init_ldargs_lan(struct dmctx *ctx, struct uci_section *s)
{
	struct ldlanargs *args = &cur_lanargs;
	ctx->args = (void *)args;
	args->ldlansection = s;
	return 0;
}

inline int init_ldargs_ip(struct dmctx *ctx, struct uci_section *s)
{
	struct ldipargs *args = &cur_ipargs;
	ctx->args = (void *)args;
	args->ldipsection = s;
	return 0;
}

inline int init_ldargs_dhcp(struct dmctx *ctx, struct uci_section *s)
{
	struct lddhcpargs *args = &cur_dhcpargs;
	ctx->args = (void *)args;
	args->lddhcpsection = s;
	return 0;
}

inline int init_ldargs_wlan(struct dmctx *ctx, struct uci_section *s, int wlctl_num, char *wunit, int pki)
{
	struct ldwlanargs *args = &cur_wlanargs;
	ctx->args = (void *)args;
	args->lwlansection = s;
	args->wlctl_num = wlctl_num;
	args->wunit = wunit;
	args->pki = pki;
	return 0;
}

inline int init_ldargs_eth_cfg(struct dmctx *ctx, char *eth)
{
	struct ldethargs *args = &cur_ethargs;
	ctx->args = (void *)args;
	args->eth = eth;
}

int ip_to_int(char *address) //TODO there is already standrard function
{
	unsigned int res;
	char *pch;
	pch = strtok(address,".");
	unsigned int multiple = 256 * 256 * 256;
	while (pch) {
		res += atoi(pch) * multiple;
		multiple /=256;
		pch = strtok(NULL, ".");		
	}
	return res;
}
/************************************************************************** 
**** ****  function related to landevice_lanhostconfigmanagement  **** ****
***************************************************************************/

int get_lan_dns(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	int len;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);
	
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_parse_array(res, "dns-server", -1, NULL, value);
	TRACE("returned value after parse is %s\n", *value);
	len = strlen(*value) - 1;
	if ((*value)[len] == ',')
		(*value)[len] = '\0';
	if ((*value)[0] == '\0') {
		dmuci_get_value_by_section_string(lanargs->ldlansection, "dns", value);
		//TODO REPLACE SPACE BY ','
	}
	return 0;
}

int set_lan_dns(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	/*struct ldlanargs *lanargs = (struct ldlanargs *)args;
	struct *uci_ptr = lanargs->ptr;
	int i;
	char *lan_name = dmstrdup(section_name(lanargs->ldlansection));

	for(i = 0; value[i]; i++) {
		if (value[i] == ',') {
			value[i] = ' ';			
		}
	}
	dmuci_set_value_by_section(&uci_ptr, lanargs->ldlansection, "dns", value);*/
	// delay_service reload "network" "1" //TODO BY IBH
	// delay_service reload "dnsmasq" "1" //TODO BY IBH
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
		//TODO BY IBH
			return 0;
	}
	return 0;
}

int get_lan_dhcp_server_configurable(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_section *s = NULL;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;

	uci_foreach_option_eq("dhcp", "dhcp", "interface", section_name(lanargs->ldlansection), s) {
		if (s != NULL) {
			TRACE("section found s name %s section type is %s \n\n", s->e.name, s->type);
			*value = "1";
			return 0;
		}
	}
	if (s == NULL) {
		TRACE("no section found \n");
		*value = "0";
	}
	return 0;
}

int set_lan_dhcp_server_configurable(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	struct uci_section *s = NULL;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);

	//TODO switch action here VALUECHECK VALUESET????

	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL && value[0] == '0') {
			TRACE("uci_del_section(s)"); //TODO BY FETEN
			break;
		}
	}
	if (s == NULL && value[0] == '1') {
		char *str_value = NULL;
		str_value = "dhcp";
		dmuci_set_value("dhcp",lan_name, NULL, str_value);//check if the allocation for set value is made in uci_set_function
		dmuci_set_value("dhcp", lan_name, "interface", lan_name);
		str_value = "100";
		dmuci_set_value("dhcp", lan_name, "start", str_value);
		str_value = "150";
		dmuci_set_value("dhcp", lan_name, "limit", str_value);
		str_value = "12h";
		dmuci_set_value("dhcp", lan_name, "leasetime", str_value);
	}
	if (value[0] != '1' && value[0] != '0') {
		return 0;
	}		
	return 0;
}

int get_lan_dhcp_server_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_section *s = NULL;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);

	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			dmuci_get_value_by_section_string(s, "ignore", value);
			if ((*value)[0] == '\0')
				*value = "1";
			else
				*value = "0";
		}
		break; //TODO IMEN should explain
	}
	if (s == NULL) {
		*value = "0";
	}
	return 0;
}

int set_lan_dhcp_server_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct uci_section *s = NULL;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);
	
	//TODO switch action here VALUECHECK VALUESET????

	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			if (value[0] == '1')
				dmuci_set_value_by_section(s, "ignore", "");
			else if (value[0] == '0')
				dmuci_set_value_by_section(s, "ignore", "1");
			else
				return 0;
		}
		break;
	}
	return 0;
}

int get_lan_dhcp_interval_address_start(char *refparam, struct dmctx *ctx, char **value)
{	
	json_object *res;
	char *ipaddr, *mask, *start , *limit;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);
	struct uci_section *s = NULL;
	
	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			TRACE("section start found \n");
			dmuci_get_value_by_section_string(s, "start", &start);
			dmuci_get_value_by_section_string(s, "limit", &limit);
			break;			
		}
	}
	if (s == NULL) {
		start = "";
		limit = "";
	}
	if (start[0] == '\0' || limit[0] == '\0') {
		goto end;
	}
	dmuci_get_value_by_section_string(lanargs->ldlansection, "ipaddr", &ipaddr);
	if (ipaddr[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
		if (res)
			json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
	}
	TRACE("start vaut ipaddr %s \n", ipaddr);
	if (ipaddr[0] == '\0') {
		goto end;
	}
	dmuci_get_value_by_section_string(lanargs->ldlansection, "netmask", &mask);
	if (mask[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
		if (res) {
			json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
			mask = "TOCODE :`cidr2netmask $mask` ";
		}	
	}
	TRACE("start vaut mask %s \n", mask);
	if (mask[0] == '\0') {
		goto end;
	}
	*value = "TOCODE"; //ipcalc.sh $ipaddr $mask $start $limit | sed -n "s/START=//p"
	return 0;
end:
	*value = "";
	return 0;
}

int get_lan_dhcp_interval_address_end(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *ipaddr, *mask, *start , *limit;
	struct uci_section *s = NULL;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);
	
	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			TRACE("section start found \n");
			dmuci_get_value_by_section_string(s, "start", &start);
			dmuci_get_value_by_section_string(s, "limit", &limit);
			break;
		}
	}
	TRACE("start vaut %s and limit vaut %s \n", start, limit);
	if (s == NULL) {
		start = "";
		limit = "";
	}
	if (start[0] == '\0' || limit[0] == '\0') {
		goto end;
	}
	dmuci_get_value_by_section_string(lanargs->ldlansection, "ipaddr", &ipaddr);
	if (ipaddr[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
		json_select(res, "ipv4-address", 0, "address", &ipaddr, NULL);
	}
	TRACE("start vaut ipaddr %s \n", ipaddr);
	if (ipaddr[0] == '\0') {
		goto end;
	}
	dmuci_get_value_by_section_string(lanargs->ldlansection, "netmask", &mask);
	if (mask[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
		json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
		mask = "TOCODE :`cidr2netmask $mask` ";
	}
	TRACE("start vaut mask %s \n", mask);
	if (mask[0] == '\0') {
		goto end;
	}
	*value = "TOCODE"; //ipcalc.sh $ipaddr $mask $start $limit | sed -n "s/END=//p"
	return 0;
end:
	*value = "";
	return 0;
}

int set_lan_dhcp_address_start(char *refparam, struct dmctx *ctx, int action, char *value)
{
	//TODO switch action here VALUECHECK VALUESET????
	return 0;
}

int set_lan_dhcp_address_end(char *refparam, struct dmctx *ctx, int action, char *value)
{
	//TODO switch action here VALUECHECK VALUESET????
	return 0;
}

int get_lan_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, char **value) 
{	
	char val[512] = {0};
	struct uci_section *s = NULL;	
	char *min, *max, *ip, *s_n_ip;
	unsigned int n_min, n_max, n_ip;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);
	
	get_lan_dhcp_interval_address_start(refparam, ctx, &min);
	get_lan_dhcp_interval_address_end(refparam, ctx, &max);
	n_min = ip_to_int(min);
	n_max = ip_to_int(max);
	uci_foreach_sections("dhcp", "host", s) {
		if (s != NULL) {
			dmuci_get_value_by_section_string(s, "ip", &ip);
			n_ip = ip_to_int(ip);
			dmasprintf(&s_n_ip, "%u", n_ip);
			if (n_ip >= n_min && n_ip <= n_max) {//CONCAT IT'S BETTER TO ADD FUNCTION FOR THIS TO CHECK
				int len = strlen(val);
				if (len != 0) {
					memcpy(val + len, DELIMITOR, sizeof(DELIMITOR));
					strcpy(val + len + sizeof(DELIMITOR) - 1, s_n_ip);
				}
				else 
					strcpy(val, s_n_ip);
			}
			dmfree(s_n_ip);
		}
	}
	*value = dmstrdup(val); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int set_lan_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, int action, char *value)
{
	//TODO switch action here VALUECHECK VALUESET????
	return 0;
}

int get_lan_dhcp_subnetmask(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);
	char *mask;
	json_object *res;
	struct uci_section *s = NULL;
	
	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			dmuci_get_value_by_section_string(s, "netmask", value);
			break;
		}
	}
	if (s == NULL)
		*value = "";
	if ((*value)[0] == '\0')
		dmuci_get_value_by_section_string(lanargs->ldlansection, "netmask", value);
	TRACE("next value is %s \n", *value);
	if ((*value)[0] == '\0') {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "ipv4-address", 0, "mask", &mask, NULL);
		*value = "TOCODE `cidr2netmask $mask`";
	}
	return 0;
}

int set_lan_dhcp_subnetmask(char *refparam, struct dmctx *ctx, int action, char *value)
{
	//TODO switch action here VALUECHECK VALUESET????
	return 0;
}
int get_lan_dhcp_iprouters(char *refparam, struct dmctx *ctx, char **value) 
{
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(lanargs->ldlansection, "gateway", value);
	if ((*value)[0] == '\0') {
		dmuci_get_value_by_section_string(lanargs->ldlansection, "ipaddr", value);
	}
	return 0;
}

int set_lan_dhcp_iprouters(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);

	//TODO switch action here VALUECHECK VALUESET????
	dmuci_set_value("network", lan_name, "gateway", value);
	return 0;
}

int get_lan_dhcp_leasetime(char *refparam, struct dmctx *ctx, char **value)
{
	int len, mtime = 60;
	char *ltime = "";
	struct uci_section *s = NULL;
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);

	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			dmuci_get_value_by_section_string(s, "leasetime", &ltime);
			break;
		}		
	}
	if (ltime[0] == '\0') {
		*value = "-1";
		return 0;
	}

	len = strlen(ltime)-1;
	if (ltime[len] != 'm') {
		mtime = 3600;		
		if (ltime[len] != 'h') {
			*value = "0";//TO CHECK IF NO VALUE DO WE HAVE TO SET VALUE TO 0
			return 0;
		}
	}
	ltime[len] = '\0';
	dmasprintf(*value, "%d", (mtime * atoi(ltime)));//TODO to check // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int set_lan_dhcp_leasetime(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct uci_section *s = NULL;
	char buf[64];
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);

	//TODO switch action here VALUECHECK VALUESET????
	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			sprintf(buf, "%dm", (atoi(value) % 60));
			dmuci_set_value_by_section(s, "leasetime",  buf);
			break;
		}		
	}
	return 0;
}

int get_lan_dhcp_domainname(char *refparam, struct dmctx *ctx, char **value) 
{
	char *result, *pch, *str; 
	char *dn = "15";
	struct uci_list *val;
	struct uci_section *s = NULL;	
	struct ldlanargs *lanargs = (struct ldlanargs *)ctx->args;
	char *lan_name = section_name(lanargs->ldlansection);

	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s) {
			dmuci_get_value_by_section_list(s, "dhcp_option", &val); //val is a list
			*value = "parcourir la liste"; //TODO BY IBH
			/*TRACE("result is %s \n", result);
			pch = strtok(result," ");
			TRACE("pch is %s \n", pch);
			while (pch) {
				if (!strstr(pch, dn)) {
					pch = strtok(NULL, " ");
					continue;
				}
				str = dmstrdup(pch+3);
				if(!*value)
					*value = dmstrdup(str);
				else
					dmasprintf(value, "%s --- %s", *value, str);
				TRACE("value ==> %s\n", *value);
				pch = strtok(NULL, " ");
				break;
			}
			goto end;*/
		}
	}
end:
	/*free(str);
	free(dn);
	free(result);*/
	return 0;
}

int set_lan_dhcp_domainname(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *result, *dn, *pch;
	struct uci_list *val;
	struct uci_section *s = NULL;
	char *option = "dhcp_option";
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *lan_name = section_name(ipargs->ldipsection);

	//TODO switch action here VALUECHECK VALUESET????

	uci_foreach_option_eq("dhcp", "dhcp", "interface", lan_name, s) {
		if (s != NULL) {
			dmuci_get_value_by_section_list(s, option, &val); //TODO ADD PARCOUR DE LA LISTE
			/*pch = strtok(result," ");
			while (pch != NULL) {
				dn = dmstrdup(cut_fx(pch, ",", 2)); //TO DO REMOVE SUBSTRING FROM STRING				
				if (strcmp(dn, pch) == 0) {
					pch = strtok(NULL, " ");
					continue;
				}
				//$UCI_DEL_LIST dhcp.$dface.dhcp_option=$dop;//TODO BY FETEN
				//uci_del_list()
				pch = strtok(NULL, " ");
			}*/
		}
	}	
	dmuci_add_list_value("dhcp", lan_name, "dhcp_option", "15,val");//TODO BY FETEN: ADD uci_add_list_section
	// delay_service reload "network" "1"
	// delay_service restart "dnsmasq" "1"
	return 0;
}

int get_lan_host_nbr_entries(char *refparam, struct dmctx *ctx, char **value) 
{
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *lan_name = section_name(ipargs->ldipsection);
	
	//ubus call router clients | grep -c "\"network\": \"$flan\"" //TODO BY IBH
	*value = "TOCODE";
	return 0;
}
/***************************************************************************/
//#define DMINSTANCES(...) ## __VA_ARGS__ 

int filter_lan_device_interface(struct uci_section *s, void *v)
{
	char *ifname = NULL; 
	char *phy_itf = NULL;
	char *pch, *ftype;
	
	dmuci_get_value_by_section_string(s, "type", &ftype);
	if (strcmp(ftype, "alias") != 0) {
		dmuci_get_value_by_section_string(s, "ifname", &ifname);
		//check is physical
		//char *phy_int=`db get hw.board.ethernetLanPorts`; //TODO BY IBH
		//db_get_value("hw", "board", "ethernetLanPorts", &phy_itf);
		//TODO KMD: copy  &phy_itf to a local buf
		phy_itf = "eth1 eth2 eth3 eth4"; //TODO
		phy_itf = dmstrdup(phy_itf);
		TRACE("end db_get_value\n");
		pch = strtok(phy_itf," ");
		while (pch != NULL) {
			if (strstr(ifname, pch)) {
				dmfree(phy_itf);
				return 0;
			}
			pch = strtok(NULL, " ");
		}
		dmfree(phy_itf);
	}
	return -1;
}

int filter_lan_ip_interface(struct uci_section *ss, void *v)
{
	struct uci_section *lds = (struct uci_section *)v;
	char *value, *type;
	dmuci_get_value_by_section_string(ss, "type", &type);
	if (ss == lds) {
		return 0;
	}		
	else if (strcmp(type, "alias") == 0) {
		dmuci_get_value_by_section_string(ss, "ifname", &value);
		if(value[0] == 'b' && value[1] == 'r' && value[2] == '-')
			value += 3;
		if (strcmp(value, section_name(lds)) == 0) 
			return 0;
	}
	return -1;
}


/************************************************************************** 
**** function related to landevice_lanhostconfigmanagement_ipinterface ****
***************************************************************************/

int get_interface_enable_ubus(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *lan_name = section_name(ipargs->ldipsection);
	

	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "up", 0, NULL, value, NULL);
	return 0;
}

int set_interface_enable_ubus(char *refparam, struct dmctx *ctx, int action, char *value)
{
	//TODO switch action here VALUECHECK VALUESET????
	//TODO BY IBH
	return 0;
}

int get_interface_firewall_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *input = "";
	struct uci_section *s = NULL;
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *lan_name = section_name(ipargs->ldipsection);

	uci_foreach_option_cont("firewall", "zone", "network", lan_name, s) {
		if (s != NULL) {
			dmuci_get_value_by_section_string(s, "input", &input);
			if (strcmp(input, "ACCEPT") !=0 && strcmp(input, "forward") !=0) {
				*value = "1";
				return 0;
			}
			break; //TODO TO CHECK
		}
	}
	*value = "0";
	return 0;
}

struct uci_section *create_firewall_zone_config(char *fwl, char *iface, char *input, char *forward, char *output)
{
	struct uci_section *s;
	char *value, *name;
	
	dmuci_add_section("firewall", "zone", &s, &value);
	dmasprintf(&name, "%s_%s", fwl, iface);
	dmuci_set_value_by_section(s, "name", name);
	dmuci_set_value_by_section(s, "input", input);
	dmuci_set_value_by_section(s, "forward", forward);
	dmuci_set_value_by_section(s, "output", output);
	dmuci_set_value_by_section(s, "network", iface);
	dmfree(name);
	return s;
}

int set_interface_firewall_enabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int cnt = 0;
	struct uci_section *s = NULL;
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *lan_name = section_name(ipargs->ldipsection);
	
	//TODO switch action here VALUECHECK VALUESET????

	if (value[0] == '1')
		value = "DROP";
	else if (value[0] == '0')
		value = "ACCEPT";
	else
		return 0;
	uci_foreach_option_cont("firewall", "zone", "network", lan_name, s) {
		if (s != NULL) {
			dmuci_set_value_by_section(s, "input", value);
			dmuci_set_value_by_section(s, "forward", value);
			cnt++;
		} 
	}
	if (cnt == 0 && strcmp(value,"DROP") ==0)
		create_firewall_zone_config("fwl", lan_name, "DROP", "DROP", "");
	//delay_service reload "firewall" "1" //TODO BY IBH
	return 0;
}

//TO CHECK BY IBH NEW
int get_interface_ipaddress(char *refparam, struct dmctx *ctx, char **value)
{
	char *proto;
	json_object *res;	
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;//TO CHECK
	char *lan_name = section_name(ipargs->ldipsection);
	
	dmuci_get_value_by_section_string(ipargs->ldipsection, "proto", &proto);
	if (strcmp(proto, "static") == 0)
		dmuci_get_value_by_section_string(ipargs->ldipsection, "ipaddr", value);
	else {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "ipv4-address", 0, "address", value, NULL);		
	}
	return 0;
}

int set_interface_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;

	//TODO switch action here VALUECHECK VALUESET????

	char *lan_name = section_name(ipargs->ldipsection);
	if (value[0] != '\0') {
		dmuci_set_value_by_section(ipargs->ldipsection, "ipaddr", value);
	}
	return 0;
}

int get_interface_subnetmask(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *proto;
	char *val = NULL;
	json_object *res;	
	char *lan_name = section_name(ipargs->ldipsection);
	
	dmuci_get_value_by_section_string(ipargs->ldipsection, "proto", &proto);
	if (strcmp(proto, "static") == 0)
		dmuci_get_value_by_section_string(ipargs->ldipsection, "netmask", value);
	else {
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", lan_name}}, 1, &res);
		json_select(res, "ipv4-address", 0, "mask", &val, NULL);
		*value = "cidr2netmask $val"; //TODO ADD THIS FUNCTION
	}
	return 0;
}

int set_interface_subnetmask(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	
	//TODO switch action here VALUECHECK VALUESET????

	if (value[0] != '\0') {
		dmuci_set_value_by_section(ipargs->ldipsection, "netmask", value);
	}
	return 0;
}

int get_interface_addressingtype (char *refparam, struct dmctx *ctx, char **value)
{
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;

	dmuci_get_value_by_section_string(ipargs->ldipsection, "proto", value);
	if (strcmp(*value, "static") == 0)
		*value = "Static";
	else if (strcmp(*value, "dhcp") == 0)
		*value = "DHCP";
	return 0;
}

int set_interface_addressingtype(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;

	//TODO switch action here VALUECHECK VALUESET????

	if (strcmp(value, "Static") == 0)
		dmuci_set_value_by_section(ipargs->ldipsection, "proto", "static");
	else if (strcmp(value, "DHCP") == 0)
		dmuci_set_value_by_section(ipargs->ldipsection, "proto", "dhcp");
	else
		return 0;
	return 0;
}

/************************************************************************************* 
**** function related to get_landevice_lanhostconfigmanagement_dhcpstaticaddress ****
**************************************************************************************/

int get_dhcpstaticaddress_enable (char *refparam, struct dmctx *ctx, char **value)
{
	char *mac;
	struct lddhcpargs *dhcpargs = (struct lddhcpargs *)ctx->args;

	dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "mac", &mac);
	if (strcmp (mac, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0)
		*value = "0";
	else
		*value = "1";
	return 0;
}

int set_dhcpstaticaddress_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *chaddr;
	struct lddhcpargs *dhcpargs = (struct lddhcpargs *)ctx->args;

	//TODO switch action here VALUECHECK VALUESET????

	dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "mac", &chaddr);
	if (value[0] == '1' && value[1] == '\0') {
		if (strcmp(chaddr, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0) {
			char *orig_chaddr;
			dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "mac_orig", &orig_chaddr);
			dmuci_set_value_by_section(dhcpargs->lddhcpsection, "mac", orig_chaddr);
		} else {
			return 0;
		}
	} else if (value[0] == '0' && value[1] == '\0') {
		if (strcmp(chaddr, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0)
			return 0;
		else {
			char *orig_chaddr;
			dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "mac_orig", &orig_chaddr);
			dmuci_set_value_by_section(dhcpargs->lddhcpsection, "mac_orig", orig_chaddr);
			dmuci_set_value_by_section(dhcpargs->lddhcpsection, "mac", DHCPSTATICADDRESS_DISABLED_CHADDR);
		}
	}
	return 0;
}

int get_dhcpstaticaddress_chaddr(char *refparam, struct dmctx *ctx, char **value)
{
	char *chaddr;
	struct lddhcpargs *dhcpargs = (struct lddhcpargs *)ctx->args;
	
	dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "mac", &chaddr);
	if (strcmp(chaddr, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0)
		dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "mac_orig", value);
	else 
		*value = chaddr;
	return 0;
}

int set_dhcpstaticaddress_chaddr(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	char *chaddr;
	struct lddhcpargs *dhcpargs = (struct lddhcpargs *)ctx->args;
		
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "mac", &chaddr);
			if (strcmp(chaddr, DHCPSTATICADDRESS_DISABLED_CHADDR) == 0)
				dmuci_set_value_by_section(dhcpargs->lddhcpsection, "mac_orig", value);
			else
				dmuci_set_value_by_section(dhcpargs->lddhcpsection, "mac", value);
	}
	return 0;
}

int get_dhcpstaticaddress_yiaddr(char *refparam, struct dmctx *ctx, char **value)
{
	struct lddhcpargs *dhcpargs = (struct lddhcpargs *)ctx->args;
	
	dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "ip", value);
	return 0;
}

int set_dhcpstaticaddress_yiaddr(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct lddhcpargs *dhcpargs = (struct lddhcpargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(dhcpargs->lddhcpsection, "ip", value);
	}
	return 0;
}
/*************************************/


/************************************************************************************* 
**** function related to get_landevice_ethernet_interface_config ****
**************************************************************************************/

int get_lan_ethernet_interfaces(char *ndev, char *iface[], int var, int *length) 
{
	int i = 0;
	int index = 0;
	char *name, *value;	
	json_object *res = NULL;
	
	dmastrcat(&name, "br-", ndev);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", name}}, 1, &res); //TO CHECK WITH FETEN IF NO RES !!!!!!
	//TODO DM UBUS ASSERT
	if (res == NULL) {
		iface[0] = ndev;
		i++;
		return 0;
	} else {
		json_select(res, "bridge-members", -1, NULL, &value, NULL);
		value = dmstrdup(value); // MEM WILL BE FREED IN DMMEMCLEAN
		char *pch = strtok(value,",");
		while (pch != NULL) {
			if (strstr(pch, "eth") && !strstr(pch, ".")) {
				TRACE("pch is %s \n", pch);
				iface[i] = pch;
				i++;
			}
			pch = strtok(NULL, ",");
		}
	}
	*length = i--;
	return 0;
}

int get_lan_eth_iface_cfg_enable(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;
		
	//dmuci_get_value_by_section_string(dhcpargs->lddhcpsection, "ip", value);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", ethargs->eth}}, 1, &res); //TO CHECK WITH FETEN IF NO RES !!!!!!
	DM_ASSERT(res, *value = "");
	int i = json_select(res, "link", -1, NULL, value, NULL);
	if (i == -1)
		*value = "";
	return 0;
}

int set_lan_eth_iface_cfg_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (b) {
				TRACE("ethctl ethargs->eth phy-power up"); //LINUX CMD
			}
			else {
				TRACE("ethctl ethargs->eth phy-power down"); //LINUX CMD
			}
	}
	return 0;
}

int get_lan_eth_iface_cfg_status(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;
		
	get_lan_eth_iface_cfg_enable(refparam, ctx, value);
	if ((*value)[0] == '\0')
		*value = "Disabled";
	else if ((*value)[0] == '1' && (*value)[1] == '\0')
		*value = "Up";
	else 
		*value = "Disabled";
	return 0;
}

int get_lan_eth_iface_cfg_maxbitrate(char *refparam, struct dmctx *ctx, char **value)
{
	char *pch, *v;
	int len;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;
	
	dmuci_get_option_value_string("ports", "@ethports[0]", ethargs->eth, value);
	if ((*value)[0] == '\0')
		return 0;
	else {
		if (strcmp(*value, "auto") == 0)
			*value = "Auto";
		else {
			v = dmstrdup(*value); // MEM WILL BE FREED IN DMMEMCLEAN
			pch = strtok(v, "FHfh");
			len = strlen(pch) + 1;
			if ((*value)[len] == 'D' || (*value)[len] == 'd')
				*value = pch;
		}
	}
	return 0;
}

int set_lan_eth_iface_cfg_maxbitrate(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *val = NULL;
	char *duplex;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			if (strcasecmp(value, "auto") == 0) {
				dmuci_set_value("ports", "@ethports[0]", ethargs->eth, "auto");
				return 0;
			} else {
				dmuci_get_option_value_string("ports", "@ethports[0]", ethargs->eth, &duplex);
				if (strcmp(duplex, "auto") == 0)
					duplex = "FD";
				else {
					if (strstr(duplex, "FD"))
						duplex = "FD";
					else if (strstr(duplex, "HD"))
						duplex = "HD";
					else
						duplex = "FD";
				}
				dmastrcat(&val, value, duplex);
				dmuci_set_value("ports", "@ethports[0]", ethargs->eth, val);
				dmfree(val);
			}

	}
	return 0;
}

int get_lan_eth_iface_cfg_duplexmode(char *refparam, struct dmctx *ctx, char **value)
{
	char *tmp;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;
	 
	dmuci_get_option_value_string("ports", "@ethports[0]", ethargs->eth, &tmp);
	if (strcmp(tmp, "auto") == 0) {
		*value = "Auto";
	}
	else if (tmp[0] == '\0') {
		*value = "";
	}	
	else {
		if (strstr(tmp, "FD"))
			*value = "Full";
		else if (strstr(tmp, "HD"))
			*value = "Half";
		else
			*value = "";
	}
	return 0;
}

int set_lan_eth_iface_cfg_duplexmode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *m, *rate, *val = NULL;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			if (strcasecmp(value, "auto") == 0) {
				dmuci_set_value("ports", "@ethports[0]", ethargs->eth, "auto");
				return 0;
			}
			dmuci_get_option_value_string("ports", "@ethports[0]", ethargs->eth, &m);
			m = dmstrdup(m);
			rate = m;
			if (strcmp(rate, "auto") == 0)
				rate = "100";
			else {
				strtok(rate, "FHfh");
			}
			if (strcmp(value, "full") == 0)
				dmastrcat(&val, rate, "FD");
			else if (strcmp(value, "half") == 0)
				dmastrcat(&val, rate, "HD");
			else {
				dmfree(m);
				return 0;
			}
			dmuci_set_value("ports", "@ethports[0]", ethargs->eth, val);
			dmfree(m);
			dmfree(val);
	}
	return 0;
}

int get_lan_eth_iface_cfg_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", ethargs->eth}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_bytes", value, NULL);	
	return 0;
}

int get_lan_eth_iface_cfg_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", ethargs->eth}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);	
	return 0;
}

int get_lan_eth_iface_cfg_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", ethargs->eth}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	return 0;
}

int get_lan_eth_iface_cfg_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	struct ldethargs *ethargs = (struct ldethargs *)ctx->args;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", ethargs->eth}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);	
	return 0;
}

inline int get_landevice_ethernet_interface_config(struct dmctx *ctx, char *idev, char *ieth) 
{	
	DMOBJECT(DMROOT"LANDevice.%s.LANEthernetInterfaceConfig.%s.", ctx, "0", 1, NULL, NULL, NULL, idev, ieth);
	DMPARAM("Enable", ctx, "1", get_lan_eth_iface_cfg_enable, set_lan_eth_iface_cfg_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("Status", ctx, "0", get_lan_eth_iface_cfg_status, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("MaxBitRate", ctx, "1", get_lan_eth_iface_cfg_maxbitrate, set_lan_eth_iface_cfg_maxbitrate, "", 0, 0, UNDEF, NULL);
	DMPARAM("DuplexMode", ctx, "1", get_lan_eth_iface_cfg_duplexmode, set_lan_eth_iface_cfg_duplexmode, "", 0, 0, UNDEF, NULL);
	DMOBJECT(DMROOT"LANDevice.%s.LANEthernetInterfaceConfig.%s.Stats.", ctx, "0", 1, NULL, NULL, NULL, idev, ieth);
	DMPARAM("BytesSent", ctx, "0", get_lan_eth_iface_cfg_stats_tx_bytes, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("BytesReceived", ctx, "0", get_lan_eth_iface_cfg_stats_rx_bytes, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("PacketsSent", ctx, "0", get_lan_eth_iface_cfg_stats_tx_packets, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("PacketsReceived", ctx, "0", get_lan_eth_iface_cfg_stats_rx_packets, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
}

/*************************************/
int entry_method_root_LANDevice(struct dmctx *ctx)
{
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	struct uci_section *sss = NULL;
	struct uci_ptr ptr = {0};
	char *idev = NULL;
	char *cur_idev = NULL;
	char *ilan = NULL;
	char *cur_ilan = NULL;
	char *idhcp = NULL;
	char *cur_idhcp = NULL;
	char *iwlan = NULL;
	char *cur_iwlan = NULL;
	char* iface[TAILLE];
	int length = 0, ieth;
	char ieth_buf[8] = {0};
	char *network;
	//struct ldlanargs *(ctx->args) = (struct ldlanargs *)(ctx->args); //TO CHECK
	IF_MATCH(ctx, DMROOT"LANDevice.") {
		DMOBJECT(DMROOT"LANDevice.", ctx, "0", 0, NULL, NULL, NULL);
		TRACE("uci_foreach_filter_func start \n");
		uci_foreach_filter_func("network", "interface", NULL, &filter_lan_device_interface, s) {
			TRACE("section type %s section name %s \n\n", s->e.name, s->type);
			//DMOBJECT(DMROOT"LANDevice.%s.", ctx, "0", 1, NULL, NULL, NULL, DMINSTANCES(idev, idev2));
			init_ldargs_lan(ctx, s);
			idev = update_instance(s, cur_idev, "ldinstance");			
			DMOBJECT(DMROOT"LANDevice.%s.", ctx, "0", 0, NULL, NULL, NULL, idev);
			DMOBJECT(DMROOT"LANDevice.%s.LANHostConfigManagement.", ctx, "0", 1, NULL, NULL, NULL, idev);
			DMPARAM("DNSServers", ctx, "1", get_lan_dns, set_lan_dns, "", 0, 0, UNDEF, NULL);
			DMPARAM("DHCPServerConfigurable", ctx, "1", get_lan_dhcp_server_configurable, set_lan_dhcp_server_configurable, "xsd:boolean", 0, 0, UNDEF, NULL);
			DMPARAM("DHCPServerEnable", ctx, "1", get_lan_dhcp_server_enable, set_lan_dhcp_server_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
			DMPARAM("MinAddress", ctx, "1", get_lan_dhcp_interval_address_start, set_lan_dhcp_address_start, "", 0, 0, UNDEF, NULL);
			DMPARAM("MaxAddress", ctx, "1", get_lan_dhcp_interval_address_end, set_lan_dhcp_address_end, "", 0, 0, UNDEF, NULL);
			DMPARAM("ReservedAddresses", ctx, "1", get_lan_dhcp_reserved_addresses, set_lan_dhcp_reserved_addresses, "", 0, 0, UNDEF, NULL);
			DMPARAM("SubnetMask", ctx, "1", get_lan_dhcp_subnetmask, set_lan_dhcp_subnetmask, "", 0, 0, UNDEF, NULL);
			DMPARAM("IPRouters", ctx, "1", get_lan_dhcp_iprouters, set_lan_dhcp_iprouters, "", 0, 0, UNDEF, NULL);
			DMPARAM("DHCPLeaseTime", ctx, "1", get_lan_dhcp_leasetime, set_lan_dhcp_leasetime, "", 0, 0, UNDEF, NULL);
			DMPARAM("DomainName", ctx, "1", get_lan_dhcp_domainname, set_lan_dhcp_domainname, "", 0, 0, UNDEF, NULL);
			DMOBJECT(DMROOT"LANDevice.%s.Hosts.", ctx, "0", 0, NULL, NULL, NULL, idev);
			DMPARAM("HostNumberOfEntries", ctx, "0", get_lan_host_nbr_entries, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
			DMOBJECT(DMROOT"LANDevice.%s.LANHostConfigManagement.IPInterface.", ctx, "0", 1, NULL, NULL, NULL, idev);
			DMOBJECT(DMROOT"LANDevice.%s.LANHostConfigManagement.DHCPStaticAddress.", ctx, "1", 1, NULL, NULL, NULL, idev);
			cur_ilan = NULL;
			uci_foreach_filter_func("network", "interface", s, filter_lan_ip_interface, ss) {
				ilan = update_instance(ss, cur_ilan, "lipinstance");
				init_ldargs_ip(ctx, ss);
				SUBENTRY(get_landevice_lanhostconfigmanagement_ipinterface, ctx, idev, ilan); //ndev is not used //nlan can be passed as
				cur_idhcp = NULL;
				uci_foreach_option_cont("dhcp", "dhcp", "interface", section_name(ss), sss) {
					idhcp = update_instance(sss, cur_idhcp, "ldhcpinstance");
					init_ldargs_dhcp(ctx, sss);
					SUBENTRY(get_landevice_lanhostconfigmanagement_dhcpstaticaddress, ctx, idev, idhcp);
					dmfree(cur_idhcp);
					cur_idhcp = dmstrdup(idhcp);
				}
				dmfree(cur_idhcp);
				dmfree(cur_ilan);
				cur_ilan = dmstrdup(ilan);
			}
			dmfree(cur_ilan);
			DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.", ctx, "0", 0, NULL, NULL, NULL, idev);
			uci_foreach_sections("wireless", "wifi-device", ss) {
				int wlctl_num=0;
				cur_iwlan = NULL;
				uci_foreach_option_eq("wireless", "wifi-iface", "device", section_name(ss), sss) {
					dmuci_get_value_by_section_string(sss, "network", &network);
					if (strcmp(network, section_name(s)) != 0) //CHECK IF ndev is equal to section_name(s)
						continue;
					iwlan = update_instance(sss, cur_iwlan, "lwlaninstance");
					init_ldargs_wlan(ctx, sss, wlctl_num++, section_name(ss), 0);
					SUBENTRY(get_landevice_wlanconfiguration_generic, ctx, idev, iwlan); //TODO IS IT BETTER TO PASS WUNIT AS ARGUMENT  
					dmfree(cur_iwlan);
					cur_iwlan = dmstrdup(iwlan);
				}
				dmfree(cur_iwlan);
			}
			/* TO CHECK */
			DMOBJECT(DMROOT"LANDevice.%s.LANEthernetInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL, idev);
			get_lan_ethernet_interfaces(section_name(s), iface, 10, &length);
			for (ieth = 0; ieth < length; ieth++) {
				init_ldargs_eth_cfg(ctx, iface[ieth]);
				sprintf(ieth_buf, "%d", ieth + 1);
				SUBENTRY(get_landevice_ethernet_interface_config, ctx, idev, ieth_buf);
			}
			dmfree(cur_idev);
			cur_idev = dmstrdup(idev);
		}
		dmfree(cur_idev);
		return 0;
	}
	return FAULT_9005;
}

/************************************************************************************** 
**** **** ****function related to get_landevice_wlanconfiguration_generic **** ********
***************************************************************************************/
int get_wlan_enable(char *refparam, struct dmctx *ctx, char **value)
{
	int i;
	char *val;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "disabled", &val);

	if (val[0] == '0')
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_wlan_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (b)
				value = "0";
			else
				value = "1";
			dmuci_set_value_by_section(wlanargs->lwlansection, "disabled", value);
	}
	return 0;
}

int get_wlan_status (char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	dmuci_get_value_by_section_string(wlanargs->lwlansection, "disabled", value);
	if (*value[0] == '1' && *value[1] == '\0')
		*value = "Disabled";
	else
		*value = "Up";
	return 0;
}

int get_wlan_bssid (char *refparam, struct dmctx *ctx, char **value)
{
	char *wunit, buf[8];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	dmuci_get_value_by_section_string(wlanargs->lwlansection, "device", &wunit); //TODO KMD
	if (wlanargs->wlctl_num != 0) {
		sprintf(buf, "%s.%d", wunit, wlanargs->wlctl_num);
		wunit = buf;
	}
	*value = "`/usr/sbin/wlctl -i $wunit bssid`"; //TODO GET THE EQUIVALENT UBUS CMD OR LINUX CMD
	return 0;
}

int get_wlan_max_bit_rate (char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_option_value_string("wireless", wlanargs->wunit, "hwmode", value);
	return 0;
}

int set_wlan_max_bit_rate(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value("wireless", wlanargs->wunit, "hwmode", value);
	}	
	return 0;
}

int get_wlan_channel(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	dmuci_get_option_value_string("wireless", wlanargs->wunit, "channel", value);
	if (strcmp(*value, "auto") == 0 || (*value)[0] == '\0')
		*value = "`/usr/sbin/wlctl -i $wunit channel|grep \"target channel\"|awk -F ' ' '{print$3}'`"; //TODO LINUX EQUIVALENT COMMAND
	return 0;
}

int set_wlan_channel(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value("wireless", wlanargs->wunit, "channel", value);
	}
	return 0;
}

int get_wlan_auto_channel_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_option_value_string("wireless", wlanargs->wunit, "channel", value);
	if (strcmp(*value, "auto") == 0 || (*value)[0] == '\0')
		*value = "1";
	else 
		*value = "0";
	return 0;
}

int set_wlan_auto_channel_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (b)
				value = "auto";
			else
				value = "`/usr/sbin/wlctl -i $wunit channel|grep \"target channel\"|awk -F ' ' '{print$3}'`";//TODO LINUX EQUIVALENT COMMANDE
			dmuci_set_value("wireless", wlanargs->wunit, "channel", value);
	}
	return 0;
}

int get_wlan_ssid(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "ssid", value);
	return 0;
}

int set_wlan_ssid(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(wlanargs->lwlansection, "ssid", value);
	}
	return 0;
}

int get_wlan_beacon_type(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", value);
	if (strcmp(*value, "none") == 0)
		*value = "None";
	else if (strcmp(*value, "wep-shared") == 0 || strcmp(*value, "wep-open") == 0)
		*value = "Basic";
	else if (strcmp(*value, "psk") == 0 || strcmp(*value, "psk+") == 0 || strcmp(*value, "wpa") == 0)
		*value = "WPA";
	else if (strcmp(*value, "psk2") == 0 || strcmp(*value, "psk2+") == 0 || strcmp(*value, "wpa2") == 0)
		*value = "11i";
	else if (strcmp(*value, "mixed-psk") == 0 || strcmp(*value, "mixed-psk+") == 0 || strcmp(*value, "mixed-wpa") == 0)
		*value = "WPAand11i";
	return 0;
}

int reset_wlan(struct uci_section *s)
{
	dmuci_delete_by_section(s, "gtk_rekey", NULL);
	dmuci_delete_by_section(s, "wps_pbc", NULL);
	dmuci_delete_by_section(s, "key", NULL);
	dmuci_delete_by_section(s, "key1", NULL);
	dmuci_delete_by_section(s, "key2", NULL);
	dmuci_delete_by_section(s, "key3", NULL);
	dmuci_delete_by_section(s, "key4", NULL);
	dmuci_delete_by_section(s, "radius_server", NULL);
	dmuci_delete_by_section(s, "radius_port", NULL);
	dmuci_delete_by_section(s, "radius_secret", NULL);
	return 0;
}

char *get_nvram_wpakey() {
	FILE* fp = NULL;
	char wpakey[64];
	fp = fopen(NVRAM_FILE, "r");
	if (fp != NULL) {
		fgets(wpakey, 64, fp);
		fclose(fp);
		return dmstrdup(wpakey);
	}
	return NULL;
}

int set_wlan_beacon_type(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *) ctx->args;
	char *encryption, *option;
	char strk64[4][11];

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "None") != 0) {
				value = "none";
				reset_wlan(wlanargs->lwlansection);
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", value);
			} 
			else if (strcmp(value, "Basic") == 0) {
				value = "wep-open";
				if (strcmp(encryption, "wep-shared") != 0 && strcmp(encryption, "wep-open") != 0) {
					reset_wlan(wlanargs->lwlansection);
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", "1");
					wepkey64("Inteno", strk64);
					int i = 0;
					while (i < 4) {
						dmasprintf(&option, "key%d", i + 1);
						dmuci_set_value_by_section(wlanargs->lwlansection, option, strk64[i++]);
						dmfree(option);
						i++;
					}
				}	
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", value);
			}
			else if(strcmp(value, "WPA") == 0) {
				value = "psk";
				char *encryption;
				dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
				if (!strstr(encryption, "psk")){
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", value);
			}
			else if(strcmp(value, "11i") == 0) {
				value = "psk2";
				if (!strstr(encryption, "psk")){
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "1");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "value");
			}
			else if(strcmp(value, "WPAand11i") == 0) {
				value = "mixed-psk";
				if (!strstr(encryption, "psk")){
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "1");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "value");
			}
			//delay_service reload "network" "1" //TODO	BY IBH		
	}	
	return 0;
}

int get_wlan_mac_control_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	char *macfilter;
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "macfilter", &macfilter);
	if (macfilter[0] != '0')
		*value = "1";
	else 
		*value = "0";
	return 0;
}

int set_wlan_mac_control_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (b)
				value = "2";
			else 
				value = "0";
			dmuci_set_value_by_section(wlanargs->lwlansection, "macfilter", value);
	}
	return 0;
}

int get_wlan_standard(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	if (strcmp(wlanargs->wunit, "11b") == 0)
		*value = "b";
	else if (strcmp(wlanargs->wunit, "11bg") == 0)
		*value = "g";
	else if (strcmp(wlanargs->wunit, "11g") == 0 || strcmp(wlanargs->wunit, "11gst") == 0 || strcmp(wlanargs->wunit, "11lrs") == 0)
		*value = "g-only";
	else if (strcmp(wlanargs->wunit, "11n") == 0 || strcmp(wlanargs->wunit, "auto") == 0)
		*value = "n";
	else
		*value = "";
	return 0;
}

int set_wlan_standard(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			if (strcmp(value, "b") == 0)
				value = "11b";
			else if (strcmp(value, "g") == 0)
				value = "11bg";
			else if (strcmp(value, "g-only") == 0)
				value = "11g";
			else if (strcmp(value, "n") == 0)
				value = "auto";
			dmuci_set_value("wireless", wlanargs->wunit, "hwmode", value);
	}	
	return 0;
}

int get_wlan_wep_key_index(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	char *encryption;
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	if (strcmp(encryption, "wep-shared") == 0)
		dmuci_get_value_by_section_string(wlanargs->lwlansection, "key", value);
	else 
		*value = "";
	return 0;
}

int set_wlan_wep_key_index(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	char *option, *encryption;
	char strk64[4][11];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			//delay_service reload "network" "1" TODO BY IBH
			if (strcmp(encryption, "wep-shared") != 0 && strcmp(encryption, "wep-open") != 0)
			{
				reset_wlan(wlanargs->lwlansection);
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "wep-open");
				wepkey64("Inteno", strk64);
				int i = 0;
				while (i < 4) {
					dmasprintf(&option, "key%d", i + 1);
					dmuci_set_value_by_section(wlanargs->lwlansection, option, strk64[i]);
					dmfree(option);
					i++;
				}
			}
			dmuci_set_value_by_section(wlanargs->lwlansection, "key", value);
	}	
	return 0;
}

int set_wlan_key_passphrase(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	char *option, *encryption;
	char strk64[4][11];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(encryption, "wep-shared") == 0 || strcmp(encryption, "wep-open") == 0) {
				wepkey64("Inteno", strk64);
				int i = 0;
				while (i < 4) {
					dmasprintf(&option, "key%d", i + 1);
					dmuci_set_value_by_section(wlanargs->lwlansection, option, strk64[i]);
					dmfree(option);
					i++;
				}
			} else if (strcmp(encryption, "none") == 0)
				return -1; //TODO CHECK
			else 
				set_wlan_pre_shared_key(refparam, ctx, action, value); //TODO CHECK
	}
	return 0;	
}

int get_wlan_wep_encryption_level(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "40-bit, 104-bit";
	return 0;
}

int get_wlan_basic_encryption_modes(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	if (strcmp(encryption, "wep-shared") == 0 || strcmp(encryption, "wep-open") == 0)
		*value = "WEPEncryption";
	else 
		*value = "None";
	return 0;
}

int set_wlan_basic_encryption_modes(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *option, *encryption;
	char strk64[4][11];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "WEPEncryption") == 0) {
				if (strcmp(encryption, "wep-shared") != 0 && strcmp(encryption, "wep-open") != 0) {
					reset_wlan(wlanargs->lwlansection);
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", "1");
					int i = 0;
					wepkey64("Inteno", strk64);
					while (i < 4) {
						dmasprintf(&option, "key%d", i + 1);
						dmuci_set_value_by_section(wlanargs->lwlansection, option, strk64[i]);
						dmfree(option);
						i++;
					}	
					dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", value);
				}
			} else if (strcmp(value, "None") == 0) {
				reset_wlan(wlanargs->lwlansection);
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "none");
			}
	}	
	return 0;
}

int get_wlan_basic_authentication_mode(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	if (strcmp(encryption, "wep-shared") == 0)
		*value = "SharedAuthentication";
	else 
		*value = "None";
	return 0;
}

int set_wlan_basic_authentication_mode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *encryption, *option;
	char strk64[4][11];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "SharedAuthentication") == 0) {
				if (strcmp(encryption, "wep-shared") != 0 && strcmp(encryption, "wep-open") != 0) {
					reset_wlan(wlanargs->lwlansection);
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", "1");
					int i = 0;
					wepkey64("Inteno", strk64);
					while (i < 4) {
						dmasprintf(&option, "key%d", i + 1);
						dmuci_set_value_by_section(wlanargs->lwlansection, option, strk64[i]);
						dmfree(option);
						i++;
					}
					dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "wep-open");
				}
				
			} else if (strcmp(value, "None")) {
				reset_wlan(wlanargs->lwlansection);
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "none");
			}
	}
	return 0;
}

int get_wlan_wpa_encryption_modes(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	if (strcmp(encryption, "psk+tkip") == 0 || strcmp(encryption, "mixed-psk+tkip") == 0)
		*value = "TKIPEncryption";
	else if (strcmp(encryption, "psk+ccmp") == 0 || strcmp(encryption, "mixed-psk+ccmp") == 0)
		*value = "AESEncryption";
	else if (strcmp(encryption, "psk+tkip+ccmp") == 0 || strcmp(encryption, "mixed-psk+tkip+ccmp") == 0)
		*value = "TKIPandAESEncryption";
	return 0;
}

int set_wlan_wpa_encryption_modes(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "TKIPEncryption") == 0) {
				if(!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk+tkip");
			}
			else if (strcmp(value, "AESEncryption") == 0) {
				if(!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk+ccmp");
			}
			else if (strcmp(value, "TKIPandAESEncryption") == 0) {
				if(!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk+tkip+ccmp");
			}
	}	
	return 0;
}

int get_wlan_wpa_authentication_mode(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	*value = "";
	if (strcmp(encryption, "psk") == 0 || strcmp(encryption, "psk+") == 0 || strcmp(encryption, "mixed-psk") == 0)
		*value = "PSKAuthentication";
	else if (strcmp(encryption, "wpa") == 0 || strcmp(encryption, "mixed-wpa") == 0)
		*value = "EAPAuthentication";
	return 0;
}

int set_wlan_wpa_authentication_mode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "PSKAuthentication") == 0) {
				if (!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk");
			}
			else if (strcmp(value, "EAPAuthentication") == 0) {
				if(strcmp(encryption, "wpa") != 0 && strcmp(encryption, "wpa2") != 0 && strcmp(encryption, "mixed-wpa") != 0) {
					reset_wlan(wlanargs->lwlansection);
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_server", "");
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_port", "1812");
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_secret", "");
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "wpa");
			}
	}	
	return 0;
}

int get_wlan_ieee_11i_encryption_modes(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	*value = "";
	if (strcmp(encryption, "psk2+tkip") == 0 || strcmp(encryption, "mixed-psk+tkip") == 0)
		*value = "TKIPEncryption";
	else if (strcmp(encryption, "psk2+ccmp") == 0 || strcmp(encryption, "mixed-psk+ccmp") == 0)
		*value = "AESEncryption";
	else if (strcmp(encryption, "psk2+tkip+ccmp") == 0 || strcmp(encryption, "mixed-psk+tkip+ccmp") == 0)
		*value = "TKIPandAESEncryption";
	return 0;
}

int set_wlan_ieee_11i_encryption_modes(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "TKIPEncryption") == 0) {
				if (!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "1");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk2+tkip");
			}
			else if (strcmp(value, "AESEncryption") == 0) {
				if (!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "1");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk2+ccmp");
			}
			else if (strcmp(value, "TKIPandAESEncryption") == 0) {
				if (!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "1");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk2+tkip+ccmp");
			}
	}	
	return 0;
}

int get_wlan_ieee_11i_authentication_mode(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	*value = "";
	if (strcmp(*value, "psk2") == 0 || strcmp(*value, "psk2+") == 0 || strcmp(*value, "mixed-psk") == 0 )
		*value = "PSKAuthentication";
	else if (strcmp(encryption, "wpa2") == 0 || strcmp(encryption, "mixed-wpa") == 0)
		*value = "EAPAuthentication";
	return 0;
}

int set_wlan_ieee_11i_authentication_mode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "PSKAuthentication") == 0) {
				if (!strstr(encryption, "psk")) {
					reset_wlan(wlanargs->lwlansection);
					char *gnw = get_nvram_wpakey();
					dmuci_set_value_by_section(wlanargs->lwlansection, "key", gnw);
					dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
					dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "1");
					dmfree(gnw);
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk2");
			}
			else if (strcmp(value, "EAPAuthentication") == 0) {
				if (strcmp(value, "wpa") != 0 && strcmp(value, "wpa2") != 0 && strcmp(value, "mixed-wpa") != 0) {
					reset_wlan(wlanargs->lwlansection);
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_server", "");
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_port", "1812");
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_secret", "");
				}
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "wpa2");
			}
	}	
	return 0;
}

int get_wlan_radio_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	int val;
	char *wunit, buf[8];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	*value = "";
	if (wlanargs->wlctl_num != 0) {
		sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
		wunit = buf;
	}
	else {
		wunit = wlanargs->wunit;
	}
	char *radio = "`/usr/sbin/wlctl -i $wunit radio`"; //TODO GET THE EQUIVALENT UBUS CMD OR LINUX CMD
	//convert hex to decimal 
	sscanf(radio, "%x", &val);
	if (val == 0)
		*value = "1";
	else if (val == 1)
		*value = "0";
	return 0;
}

int set_wlan_radio_enabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	char *wunit, buf[8];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (!b)
				value = "off";
			else
				value = "on";
			if (wlanargs->wlctl_num != 0) {
				sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
				wunit = buf;
			}
			else 
				wunit = wlanargs->wunit;
			// /usr/sbin/wlctl -i $wunit radio $val //TODO EQUIVALENT LINUX COMMAND
	}
	return 0;
}

int get_wlan_device_operation_mode(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "mode", value);
	if (strcmp(*value, "ap") == 0)
		*value = "InfrastructureAccessPoint";
	else 
		*value = "";
	return 0;
}

int set_wlan_device_operation_mode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			if (strcmp(value, "InfrastructureAccessPoint") == 0)
				dmuci_set_value_by_section(wlanargs->lwlansection, "mode", "ap");
	}
	return 0;
}

int get_wlan_authentication_service_mode(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	if (strcmp(encryption, "wpa") == 0 || strcmp(encryption, "wpa2") == 0 || strcmp(encryption, "mixed-wpa") == 0)
		*value = "RadiusClient";
	else 
		*value = "None";
	return 0;
}

int set_wlan_authentication_service_mode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (strcmp(value, "None") == 0) {
				reset_wlan(wlanargs->lwlansection);
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "none");
			}
			else if (strcmp(value, "RadiusClient") == 0) {
				if (strcmp(encryption, "wpa") == 0 || strcmp(encryption, "wpa2") == 0 || strcmp(encryption, "mixed-wpa") == 0) {
					reset_wlan(wlanargs->lwlansection);
					dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "wpa");
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_server", "");
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_port", "1812");
					dmuci_set_value_by_section(wlanargs->lwlansection, "radius_secret", "");
				}
			}
	}
	return 0;
}

int get_wlan_total_associations(char *refparam, struct dmctx *ctx, char **value)
{
	char *wunit, buf[8];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	if (wlanargs->wlctl_num != 0) {
		sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
		wunit = buf;
	}
	else 
		wunit = wlanargs->wunit;
	*value = "`/usr/sbin/wlctl -i $wunit assoclist | grep -c 'assoclist'`";//TODO ADD LINUX CMD
	return 0;
}

int get_wlan_devstatus_statistics_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;	
	json_object *res;
	char *wunit, buf[8];
	
	if (wlanargs->wlctl_num != 0) {
		sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
		wunit = buf;
	}
	else 
		wunit = wlanargs->wunit;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wunit}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", -1, "tx_bytes", value, NULL);
	return 0;
}

int get_wlan_devstatus_statistics_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *wunit, buf[8];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	if (wlanargs->wlctl_num != 0) {
		sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
		wunit = buf;
	}
	else 
		wunit = wlanargs->wunit;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wunit}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	return 0;
}

int get_wlan_devstatus_statistics_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{	
	json_object *res;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	char *wunit, buf[8];
	
	if (wlanargs->wlctl_num != 0) {
		sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
		wunit = buf;
	}
	else 
		wunit = wlanargs->wunit;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wunit}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	return 0;
}

int get_wlan_devstatus_statistics_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *val = NULL;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	char *wunit, buf[8];
	
	if (wlanargs->wlctl_num != 0) {
		sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
		wunit = buf;
	}
	else
		wunit = wlanargs->wunit;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wunit}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	
	return 0;
}

int get_wlan_ssid_advertisement_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *hidden;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "hidden", &hidden);
	if (hidden[0] == '1' && hidden[1] == '\0')
		*value = "0";
	else
		*value = "1";
	return 0;
}

int set_wlan_ssid_advertisement_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (b)
				dmuci_set_value_by_section(wlanargs->lwlansection, "hidden", "");
			else
				dmuci_set_value_by_section(wlanargs->lwlansection, "hidden", "1");

	}
	return 0;
}

int get_wlan_wps_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *wps_pbc;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "wps_pbc", &wps_pbc);
	if (wps_pbc[0] == '1' && wps_pbc[1] == '\0')
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_wlan_wps_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (b)
				dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "1");
			else
				dmuci_set_value_by_section(wlanargs->lwlansection, "wps_pbc", "");
	}
	return 0;
}

int get_x_inteno_se_channelmode(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_option_value_string("wireless", wlanargs->wunit, "channel", value);
	if (strcmp(*value, "auto") || (*value)[0] == '\0')
		*value = "Auto";
	else
		*value = "Manual";
	return 0;
}

int set_x_inteno_se_channelmode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *channel;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			if (strcmp(value, "Auto"))
				dmuci_set_value("wireless", wlanargs->wunit, "channel", "auto");
			else if (strcmp(value, "Manual"))
				channel = "`/usr/sbin/wlctl -i $wunit channel |grep \"target channel\" |awk -F ' ' '{print$3}'`"; //TODO GET LINUX CMD
				dmuci_set_value("wireless", wlanargs->wunit, "channel", channel);
	}

	return 0;
}

int get_x_inteno_se_supported_standard(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	char *status = "";// `wlctl -i wlanargs->wunit status |awk  '$1==""Chanspec:" {print$2}'` //TODO LINUX EQUIVALENT CMD
	if (strcmp(status, "5GHz") == 0)
		*value = "a, n, ac";
	else
		*value = "b, g, n, gst, lrs";
	return 0;
}

int get_x_inteno_se_operating_channel_bandwidth(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	dmuci_get_option_value_string("wireless", wlanargs->wunit, "bandwidth", value);
	if (value[0] == '\0')
		*value = "LINUX EQUIVALENT CMD"; //`wlctl -i wlanargs->wunit status |awk  '$1=="Chanspec:" {print$5}'` //TODO LINUX EQUIVALENT CMD
	//echo "${val//[^0-9]/}""MHz" //TODO LINUX EQUIVALENT CMD 
	return 0;
}

int set_x_inteno_se_operating_channel_bandwidth(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int x;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			x = (int)strtol(value, (char **)NULL, 10);
			dmasprintf(&value, "%d", x);//TODO CHECK
			if ((value[0] == '0' && value[1] == '\0') || value[0] == '\0') {
				dmfree(value);
				return 0;
			}
			dmuci_set_value("wireless", wlanargs->wunit, "bandwidth", value);
			dmfree(value);
	}	
	return 0;
}

int get_x_inteno_se_maxssid(char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "bss_max", value);
	return 0;
}

int set_x_inteno_se_maxssid(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_set_value_by_section(wlanargs->lwlansection, "bss_max", value);
	}

	return 0;
}

//TODO TO CODE
int set_wlan_wep_key(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			//uci_set_value_section(wlanargs->lwlansection, "bss_max", value);
			return 0;
	}
	return 0;
}

/****************************************************************************************/

/*int filter_wlan_interface (struct uci_section *sss, void *v)
{
	struct uci_section *lds = (struct uci_section *)v;
	dmuci_get_value_by_section_string(sss, "device", &device);
	dmuci_get_value_by_section_string(sss, "network", &network);
	if (strcmp(device, section_name(lds)) == 0)
			return 0;
	return -1;
}*/

inline int get_landevice_lanhostconfigmanagement_ipinterface (struct dmctx *ctx, char *idev, char *ilan) //TODO CAN WE USE TYPE VOID
{
	//ctx->args = (void *)args; //TO CHECK 
	struct ldipargs *ipargs = (struct ldipargs *)(ctx->args);
	
	DMOBJECT(DMROOT"LANDevice.%s.LANHostConfigManagement.IPInterface.%s.", ctx, "0", 1, NULL, NULL, section_name(ipargs->ldipsection), idev, ilan);//TO CHECK "linker_interface:$nlan"
	DMPARAM("Enable", ctx, "1", get_interface_enable_ubus, set_interface_enable_ubus, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("X_BROADCOM_COM_FirewallEnabled", ctx, "1", get_interface_firewall_enabled, set_interface_firewall_enabled, "", 0, 0, UNDEF, NULL);
	DMPARAM("IPInterfaceIPAddress", ctx, "1", get_interface_ipaddress, set_interface_ipaddress, "", 0, 0, UNDEF, NULL);
	DMPARAM("IPInterfaceSubnetMask", ctx, "1", get_interface_subnetmask, set_interface_subnetmask, "", "0", 0, UNDEF, NULL);
	DMPARAM("IPInterfaceAddressingType", ctx, "1", get_interface_addressingtype, set_interface_addressingtype, "", 0, 0, UNDEF, NULL);
	return 0;
}

inline int get_landevice_lanhostconfigmanagement_dhcpstaticaddress(struct dmctx *ctx, char *idev, char *idhcp) //TODO CAN WE USE TYPE VOID
{
	DMOBJECT(DMROOT"LANDevice.%s.LANHostConfigManagement.DHCPStaticAddress.%s.", ctx, "1", 1, NULL, NULL, NULL, idev, idhcp);
	DMPARAM("Enable", ctx, "1", get_dhcpstaticaddress_enable, set_dhcpstaticaddress_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("Chaddr", ctx, "1", get_dhcpstaticaddress_chaddr, set_dhcpstaticaddress_chaddr, "", 0, 0, UNDEF, NULL);
	DMPARAM("Yiaddr", ctx, "1", get_dhcpstaticaddress_yiaddr, set_dhcpstaticaddress_yiaddr, "", 0, 0, UNDEF, NULL);
	return 0;
}

inline int get_landevice_wlanconfiguration_generic(struct dmctx *ctx, char *idev,char *iwlan)
{
	int pki = 0;
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.", ctx, "0", 0, NULL, NULL, NULL, idev, iwlan);
	DMPARAM("Enable", ctx, "1", get_wlan_enable, set_wlan_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("Status", ctx, "0", get_wlan_status, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("BSSID", ctx, "0", get_wlan_bssid, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("MaxBitRate", ctx, "1", get_wlan_max_bit_rate, set_wlan_max_bit_rate, "", 0, 0, UNDEF, NULL);
	DMPARAM("Channel", ctx, "1", get_wlan_channel, set_wlan_channel, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("AutoChannelEnable", ctx, "1", get_wlan_auto_channel_enable, set_wlan_auto_channel_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("SSID", ctx, "1", get_wlan_ssid, set_wlan_ssid, "", 0, 0, UNDEF, NULL);
	DMPARAM("BeaconType", ctx, "1", get_wlan_beacon_type, set_wlan_beacon_type, "", 0, 0, UNDEF, NULL);
	DMPARAM("MACAddressControlEnabled", ctx, "1", get_wlan_mac_control_enable, set_wlan_mac_control_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("Standard", ctx, "1", get_wlan_standard, set_wlan_standard, "", "0", 0, UNDEF, NULL);
	DMPARAM("WEPKeyIndex", ctx, "1", get_wlan_wep_key_index, set_wlan_wep_key_index, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("KeyPassphrase", ctx, "1", get_empty, set_wlan_key_passphrase, "", 0, 0, UNDEF, NULL);
	DMPARAM("WEPEncryptionLevel", ctx, "0", get_wlan_wep_encryption_level, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("BasicEncryptionModes", ctx, "1", get_wlan_basic_encryption_modes, set_wlan_basic_encryption_modes, "", 0, 0, UNDEF, NULL);
	DMPARAM("BasicAuthenticationMode", ctx, "1", get_wlan_basic_authentication_mode, set_wlan_basic_authentication_mode, "", 0, 0, UNDEF, NULL);
	DMPARAM("WPAEncryptionModes", ctx, "1", get_wlan_wpa_encryption_modes, set_wlan_wpa_encryption_modes, "", 0, 0, UNDEF, NULL);
	DMPARAM("WPAAuthenticationMode", ctx, "1", get_wlan_wpa_authentication_mode, set_wlan_wpa_authentication_mode, "", 0, 0, UNDEF, NULL);
	DMPARAM("IEEE11iEncryptionModes", ctx, "1", get_wlan_ieee_11i_encryption_modes, set_wlan_ieee_11i_encryption_modes, "", 0, 0, UNDEF, NULL);
	DMPARAM("IEEE11iAuthenticationMode", ctx, "1", get_wlan_ieee_11i_authentication_mode, set_wlan_ieee_11i_authentication_mode, "", 0, 0, UNDEF, NULL);
	DMPARAM("RadioEnabled", ctx, "1", get_wlan_radio_enabled, set_wlan_radio_enabled, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("DeviceOperationMode", ctx, "1", get_wlan_device_operation_mode, set_wlan_device_operation_mode, "", 0, 0, UNDEF, NULL);
	DMPARAM("AuthenticationServiceMode", ctx, "1", get_wlan_authentication_service_mode, set_wlan_authentication_service_mode, "", 0, 0, UNDEF, NULL);
	DMPARAM("TotalAssociations", ctx, "0", get_wlan_total_associations, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("ChannelsInUse", ctx, "1", get_wlan_channel, set_wlan_channel, "", 0, 0, UNDEF, NULL);
	DMPARAM("TotalBytesSent", ctx, "0", get_wlan_devstatus_statistics_tx_bytes, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("TotalBytesReceived", ctx, "0", get_wlan_devstatus_statistics_rx_bytes, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("TotalPacketsSent", ctx, "0", get_wlan_devstatus_statistics_tx_packets, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("TotalPacketsReceived", ctx, "0", get_wlan_devstatus_statistics_rx_packets, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("SSIDAdvertisementEnabled", ctx, "1", get_wlan_ssid_advertisement_enable, set_wlan_ssid_advertisement_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("X_INTENO_SE_ChannelMode", ctx, "1", get_x_inteno_se_channelmode, set_x_inteno_se_channelmode, "", 0, 0, UNDEF, NULL);
	DMPARAM("X_INTENO_SE_SupportedStandards", ctx, "0", get_x_inteno_se_supported_standard, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("X_INTENO_SE_OperatingChannelBandwidth", ctx, "1", get_x_inteno_se_operating_channel_bandwidth, set_x_inteno_se_operating_channel_bandwidth, "", 0, 0, UNDEF, NULL);
	DMPARAM("X_INTENO_SE_MaxSSID", ctx, "1", get_x_inteno_se_maxssid, set_x_inteno_se_maxssid, "", 0, 0, UNDEF, NULL);
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.WPS.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan); //Check if we can move it 
	DMPARAM("Enable", ctx, "1", get_wlan_wps_enable, set_wlan_wps_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.WEPKey.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan);
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.WEPKey.1.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan);
	DMPARAM("WEPKey", ctx, "1", get_empty, set_wlan_wep_key, "", 0, 0, UNDEF, NULL);
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.WEPKey.2.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan);
	DMPARAM("WEPKey", ctx, "1", get_empty, set_wlan_wep_key, "", 0, 0, UNDEF, NULL);
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.WEPKey.3.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan);
	DMPARAM("WEPKey", ctx, "1", get_empty, set_wlan_wep_key, "", 0, 0, UNDEF, NULL);
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.WEPKey.4.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan);
	DMPARAM("WEPKey", ctx, "1", get_empty, set_wlan_wep_key, "", 0, 0, UNDEF, NULL); //TODO CHECK PARAM ORDER
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.PreSharedKey.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan);
	while (pki++ != 10) { 
		SUBENTRY(get_landevice_wlanconfiguration_presharedkey, ctx, pki, idev, iwlan); //"$wunit" "$wlctl_num" "$uci_num" are not needed
	}
	return 0;
}

int set_wlan_pre_shared_key(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *encryption;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	switch (action) {
		VALUECHECK:
			return 0;
		VALUESET:
			dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
			if (!strstr(encryption, "psk")) {
				reset_wlan(wlanargs->lwlansection);
				dmuci_set_value_by_section(wlanargs->lwlansection, "gtk_rekey", "3600");
				dmuci_set_value_by_section(wlanargs->lwlansection, "encryption", "psk");
			}
			dmuci_set_value_by_section(wlanargs->lwlansection, "key", value);
	}
	return 0;
}

/*int set_wlan_key_passphrase(char *refparam, struct dmctx *ctx, char *value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)args;
	char *encryption; 
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);
	int i=0;
	//delay_service reload "network" "1" //TODO BY IBH
	if (strcmp(encryption, "wep-shared") == 0 || strcmp(encryption, "wep-open") == 0) {
		/*for key in `/usr/sbin/wepkeygen 64 $val`;do
			$UCI_SET wireless.@wifi-iface[$num].key$i="$key"
			let i++
		done*/
		/*char *keys=dmstrdup("`/usr/sbin/wepkeygen 64 $val`"); //TODO BY IBH
		char *pch = strtok(keys," ");
		while (pch != NULL) {
			dmuci_set_value_by_section();
			pch = strtok(NULL, " ");
		}			
	}		
	} else if (strcmp(encryption, "none") == 0) {
		//echo error //EQUIVALENT 
		return 0;
	} else {
		set_wlan_pre_shared_key(refparam, args, value); //TO CHECK
	}
	return 0;
}*/

int get_wlan_psk_assoc_MACAddress(char *refparam, struct dmctx *ctx, char **value)
{
	char *wunit, *encryption, buf[8];
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "encryption", &encryption);	
	if (strstr(encryption, "psk")) {
		if (wlanargs->wlctl_num != 0) {
			sprintf(buf, "%s.%d", wlanargs->wunit, wlanargs->wlctl_num);
			wunit = buf;
		}
		else 
			wunit = wlanargs->wunit;
		TRACE("pki vaut %d \n", wlanargs->pki);
		wlanargs->pki = wlanargs->pki + 1;
		TRACE("pki vaut %d \n", wlanargs->pki);
		*value = "TO CODE:`/usr/sbin/wlctl -i $wunit assoclist|awk -F' ' '{print $2}'`";
	}
	return 0;
}

inline int get_landevice_wlanconfiguration_presharedkey(struct dmctx *ctx, int pki, char *idev, char *iwlan)
{
	char *pki_c;
	dmasprintf(&pki_c, "%d", pki);
	DMOBJECT(DMROOT"LANDevice.%s.WLANConfiguration.%s.PreSharedKey.%s.", ctx, "0", 1, NULL, NULL, NULL, idev, iwlan, pki_c);
	DMPARAM("PreSharedKey", ctx, "1", get_empty, set_wlan_pre_shared_key, "", 0, 0, UNDEF, NULL);
	DMPARAM("KeyPassphrase", ctx, "1", get_empty, set_wlan_key_passphrase, "", 0, 0, UNDEF, NULL);
	DMPARAM("AssociatedDeviceMACAddress", ctx, "0", get_wlan_psk_assoc_MACAddress, NULL, "", 0, 0, UNDEF, NULL);
	dmfree(pki_c);
	return 0;
}
