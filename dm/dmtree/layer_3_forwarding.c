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
#include "layer_3_forwarding.h"


struct routefwdargs cur_routefwdargs = {0};

inline int init_args_rentry(struct dmctx *ctx, struct uci_section *s, char *permission, char *proute)
{
	struct routefwdargs *args = &cur_routefwdargs;
	ctx->args = (void *)args;
	args->permission = dmstrdup(permission);
	args->routefwdsection = s;
	args->proute = dmstrdup(proute);
	return 0;
}

/************************************************************************************* 
**** function related to get_object_layer3 ****
**************************************************************************************/
char *ip_to_hex(char *address)
{
	int i;
	int len = 0;	
	char buf[9];
	char *pch, *val;	
	
	pch = strtok(address,".");	
	while (pch) {
		i = atoi(pch);
		dmasprintf(&val, "%02x", i);
		strcpy(buf +len, val);
		dmfree(val);
		len += 2;
		pch = strtok(NULL, ".");		
	}
	return dmstrdup(buf);
}

char *hex_to_ip(char *address)
{
	int i = 0;
	int len = 0;
	long int dec;
	char *buf, *pch;
	char ip_address[16];
	int cur_len;	
	
	while (i < 8) {
		dmasprintf(&buf, "%c%c", address[i], address[i+1]);
		sscanf(buf, "%x", &dec);
		if (i != 6)
			dmasprintf(&pch, "%u.", dec);
		else
			dmasprintf(&pch, "%u", dec);
		cur_len = strlen(pch);
		strcpy(ip_address +len, pch);
		len += cur_len;
		dmfree(pch);
		i += 2;
	}
	return dmstrdup(ip_address);
}

int proc_get_route_var_by_conf(struct uci_section *s, int name)
{
	char *dest, *mask, *dest_h, *mask_h;
		
	dmuci_get_value_by_section_string(s, "target", &dest);
	dest_h = ip_to_hex(dest);
	dmfree(dest);
	dmuci_get_value_by_section_string(s, "netmask", &mask);
	mask_h = ip_to_hex(mask);
	dmfree(mask);
	//awk '$2=="'"$dest"'" && $8=="'"$mask"'"{print $'"$varn"'}' /proc/net/route //TODO
	return -1;
}

int get_layer3_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *name;
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;

	if (routeargs->routefwdsection == NULL) {
		*value = dmstrdup("1");
		return 0;
	}
	name = dmstrdup(section_name(routeargs->routefwdsection));
	if (strstr(name, "cfg")) {
		TRACE("routeargs->routefwdsection->type %s \n", routeargs->routefwdsection->type);
		if(strcmp(routeargs->routefwdsection->type, "route_disabled") == 0)
			*value = dmstrdup("0");
		else {
			if (proc_get_route_var_by_conf(routeargs->routefwdsection, 1) == 0)
				*value = dmstrdup("0");
			else
				*value = dmstrdup("1");
		}		
	}	
	return 0;
}

/*set_layer3_enable() {
	local route="$1"
	local val="$2"
	local droute sroute="$route"
	local enable=`$UCI_GET network.$route`
	val=`echo $val|tr '[A-Z]' '[a-z]'`
	if [ "$val" = "true" -o "$val" = "1" ]; then
		[ "`$enable" = "route_disabled" ] && droute="route" || return
	elif [ "$val" = "false" -o "$val" = "0" ]; then
		[ "`$enable" = "route_disabled" ] && droute="route_disabled" || return
	else
		return
	fi
	local rc rconfigs=`$UCI_SHOW network.$sroute |sed -n "s/network\.[^.]\+\.//p"`
	route=`$UCI_ADD network $droute`
	for rc in $rconfigs; do
		$UCI_SET network.$route.$rc
	done
	$UCI_DELETE network.$sroute
	delay_service reload "network" "1"
}*/
//TOCODE
int set_layer3_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	char *pch;
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			//TODO
			return 0;
	}
	return 0;
}

int get_layer3_status(char *refparam, struct dmctx *ctx, char **value)
{
	char *eb;
	get_layer3_enable(refparam, ctx, &eb);
	if (eb[0] == '1')
		*value = dmstrdup("Enabled");
	else 
		*value = dmstrdup("Disabled");
	dmfree(eb);
	return 0;	
}

int get_layer3_type(char *refparam, struct dmctx *ctx, char **value)
{
	char *name, *netmask;
	char *delimiter = " \t";
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		name = dmstrdup(section_name(routeargs->routefwdsection));
		if (strstr(name, "cfg"))
			dmuci_get_value_by_section_string(routeargs->routefwdsection, "netmask", value);		
	}	
	else {
		char *proute = dmstrdup(routeargs->proute);
		netmask = dmstrdup(cut_fx(proute, delimiter, 8)); //WE HAVE TO ADD PROUTE TO ARGS LIST		
		*value = dmstrdup(hex_to_ip(netmask));
		dmfree(netmask);
	}
	if (strcmp(*value, "255.255.255.255") == 0 || (*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("Host");
	}
	else {
		dmfree(*value);
		*value = dmstrdup("Network");
	}
	return 0;		
}

int get_layer3_destip(char *refparam, struct dmctx *ctx, char **value)
{
	char *name, *dest;
	char *delimiter = " \t";
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		name = dmstrdup(section_name(routeargs->routefwdsection));
		if (strstr(name, "cfg")) {
			dmuci_get_value_by_section_string(routeargs->routefwdsection, "target", value);
		}
	}
	else {
		char *proute = dmstrdup(routeargs->proute);
		dest = dmstrdup(cut_fx(proute, delimiter, 2));		
		*value = dmstrdup(hex_to_ip(dest));
		dmfree(dest);
	}
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("0.0.0.0");
	}
	return 0;		
}

int set_layer3_destip(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "target", value);
			//delay_service reload "network" "1" //TODO
			return 0;
	}
	return 0;
}

int get_layer3_destmask(char *refparam, struct dmctx *ctx, char **value)
{
	char *name, *netmask;
	char *delimiter = " \t";
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		name = dmstrdup(section_name(routeargs->routefwdsection));
		if (strstr(name, "cfg")) {
			dmuci_get_value_by_section_string(routeargs->routefwdsection, "netmask", value);
		}
	}
	else {
		char *proute = dmstrdup(routeargs->proute);
		netmask = dmstrdup(cut_fx(proute, delimiter, 2));		
		*value = dmstrdup(hex_to_ip(netmask));
		dmfree(netmask);
	}
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("255.255.255.255");
	}
	return 0;
}

int set_layer3_destmask(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "netmask", value);
			//delay_service reload "network" "1" //TODO
			return 0;
	}
	return 0;
}

int get_layer3_src_address(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("0.0.0.0");
	return 0;
}

int get_layer3_src_mask(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("0.0.0.0");
	return 0;
}

int get_layer3_gatewayip(char *refparam, struct dmctx *ctx, char **value)
{
	char *name, *gateway;
	char *delimiter = " \t";
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		name = dmstrdup(section_name(routeargs->routefwdsection));
		if (strstr(name, "cfg")) {
			dmuci_get_value_by_section_string(routeargs->routefwdsection, "gateway", value);
		}
	}
	else {
		char *proute = dmstrdup(routeargs->proute);
		gateway = dmstrdup(cut_fx(proute, delimiter, 2));		
		*value = dmstrdup(hex_to_ip(gateway));
		dmfree(gateway);
	}
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("0.0.0.0");
	}
	dmfree(name);
	return 0;
} 

int set_layer3_gatewayip(char *refparam, struct dmctx *ctx, int action, char *value)
{	
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "gateway", value);
			//delay_service reload "network" "1" //TODO
			return 0;
	}
	return 0;
}

char *get_layer3_interface(struct dmctx *ctx)
{
	json_object *res;
	char *val, *bval, *ifname, *device;
	char *name;
	char *delimiter = " \t";
	struct uci_section *ss;
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	if (routeargs->routefwdsection != NULL)	{
		name = dmstrdup(section_name(routeargs->routefwdsection));
		if (strstr(name, "cfg")) {
			dmuci_get_value_by_section_string(routeargs->routefwdsection, "interface", &val);
			return val;
		}
	}
	else {
		char *proute = dmstrdup(routeargs->proute);
		bval = dmstrdup(cut_fx(proute, delimiter, 1));	
		if (!strstr(bval, "br-")) {			
			uci_foreach_option_cont("network", "interface", "ifname", bval + 3, ss) { //TO CHECK
				if (ss != NULL) {
					ifname = dmstrdup(section_name(ss));
					dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", ifname}}, 1, &res);
					if (res) {
						json_select(res, "device", 0, NULL, &device, NULL);
						if (strcmp(bval, device) == 0) {
							return ifname;
						}
					}
					dmfree(ifname);					
				}								
			}			
		}
	}
	return dmstrdup("");
}

int get_parameter_by_linker(char *refparam, struct dmctx *ctx, char **value)
{	
	*value = dmstrdup("TOCODE");
	return 0;
}

int get_layer3_interface_linker_parameter(char *refparam, struct dmctx *ctx, char **value)
{
	char *iface, *linker;
			
	iface = get_layer3_interface(ctx);
	if (iface[0] == '\0') {
		*value = dmstrdup("");		
	}		
	else {
		dmastrcat(&linker, "linker_interface:", iface);		
		get_parameter_by_linker(refparam, ctx, value);//TODO		
	}
	return 0;
}

int set_layer3_interface_linker_parameter(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;	
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	TRACE("TO CODE \n");
	return 0;
}

int get_layer3_metric(char *refparam, struct dmctx *ctx, char **value)
{
	char *name;
	char *delimiter = " \t";
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;	
	
	if (routeargs->routefwdsection != NULL)	{
		name = dmstrdup(section_name(routeargs->routefwdsection));
		if (strstr(name, "cfg")) {
			dmuci_get_value_by_section_string(routeargs->routefwdsection, "metric", value);
		}
		dmfree(name);
	}
	else {
		char *proute = dmstrdup(routeargs->proute);
		*value = dmstrdup(cut_fx(proute, delimiter, 7));		
	}
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("0");
	}	
	return 0;
}

int set_layer3_metric(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "metric", value);
			//delay_service reload "network" "1" //TODO
			return 0;
	}
	return 0;
}

int get_layer3_mtu(char *refparam, struct dmctx *ctx, char **value)
{	
	char *name;
	char *delimiter = " \t";
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;

	if (routeargs->routefwdsection != NULL)	{
		name = dmstrdup(section_name(routeargs->routefwdsection));
		if (strstr(name, "cfg")) {
			dmuci_get_value_by_section_string(routeargs->routefwdsection, "mtu", value);
		}
	}
	else {
		char *proute = dmstrdup(routeargs->proute);
		*value = dmstrdup(cut_fx(proute, delimiter, 9));
	}
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("1500");
	}
	dmfree(name);
	return 0;
}

int set_layer3_mtu(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct routefwdargs *routeargs = (struct routefwdargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(routeargs->routefwdsection, "mtu", value);
			//delay_service reload "network" "1" //TODO
			return 0;
	}
	return 0;
}

inline int get_object_layer3(struct dmctx *ctx, char *iroute, char *permission)
{
	DMOBJECT(DMROOT"InternetGatewayDevice.Layer3Forwarding.Forwarding.%s.", ctx, "0", 1, NULL, NULL, NULL, iroute);
	DMPARAM("Enable", ctx, permission, get_layer3_enable, set_layer3_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
	DMPARAM("Status", ctx, 0, get_layer3_status, NULL, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("Type", ctx, 0, get_layer3_type, NULL, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("DestIPAddress", ctx, permission, get_layer3_destip, set_layer3_destip, "xsd:boolean", 0, 1, UNDEF, NULL);
	DMPARAM("DestSubnetMask", ctx, permission, get_layer3_destmask, set_layer3_destmask, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("SourceIPAddress", ctx, 0, get_layer3_src_address, NULL, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("SourceSubnetMask", ctx, 0, get_layer3_src_mask, NULL, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("GatewayIPAddress", ctx, permission, get_layer3_gatewayip, set_layer3_gatewayip, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("Interface", ctx, permission, get_layer3_interface_linker_parameter, set_layer3_interface_linker_parameter, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("ForwardingMetric", ctx, permission, get_layer3_metric, set_layer3_metric, NULL, 0, 1, UNDEF, NULL);
	DMPARAM("MTU", ctx, permission, get_layer3_mtu, set_layer3_mtu, NULL, 0, 1, UNDEF, NULL);	
	return 0;
}

/****************************************/

/*
get_linker_by_parameter() {
	local param="$1"
	local linker=""
	jmsg=`cat "$cache_path/"* $cache_linker_dynamic | grep "\"linker\"" | grep "\"$param\"" | head -1`
	[ "$jmsg" = "" ] && return
	json_load "$jmsg"
	json_get_var linker linker
	echo "$linker"
}
*/

char *get_linker_by_parameter()
{
	//TODO
	return dmstrdup("");
}

int set_layer3_def_conn_serv(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int i;
	char *linker;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			linker = get_linker_by_parameter(value); //TODO
			//TO CHECK
			for (i = 0; i < strlen(linker) -1; i++) {
				if (linker[i] == ':') {
					linker += i+1;
					break;
				}
			}					
			if (linker[0] == '\0')
				return 0;
			dmuci_set_value("cwmp", "cpe", "default_wan_interface", linker);
			//delay_service reload "network" "1" //TODO
			return 0;
	}
	return 0;
}

int get_layer3_nbr_entry(char *refparam, struct dmctx *ctx, char **value)
{
	int pr = 0;
	int found = 0;
	FILE* f = NULL;
	char str[TAILLE_MAX];
	char *delimiter = " \t";
	char *buf;
	char *dest, *dest_ip, *cdest, *mask, *mask_ip, *cmask;
	struct uci_section *s;
	char *str1; 
	
	f = fopen(ROUTE_FILE, "r");
	if ( f != NULL)
	{
		fgets(str, TAILLE_MAX, f);
		while (fgets(str, TAILLE_MAX, f) != NULL ) 
		{			
			if (str[0] == '\n')
				continue;			
			found = 0;
			str1 = dmstrdup(str);			
			dest = cut_fx(str, delimiter, 2);
			dest_ip = hex_to_ip(dest);
			mask = cut_fx(str1, delimiter, 8);			
			mask_ip = hex_to_ip(mask);
			TRACE("dest ip is %s mask ip is %s \n",dest_ip,  mask_ip);
			uci_foreach_sections("network", "route", s) {
				if (s != NULL) {
					dmuci_get_value_by_section_string(s, "target", &cdest);
					if (strcmp(dest_ip, cdest) == 0 || (strcmp(dest_ip, "0.0.0.0") == 0 && cdest[0] == '\0')) {
						dmuci_get_value_by_section_string(s, "mask", &cmask);
						if (strcmp(mask_ip, cmask) == 0 || (strcmp(mask_ip, "255.255.255.255") == 0 && cmask[0] == '\0')) {
							found++;
							dmfree(cmask);
							dmfree(cdest);
							break;
						}							
					}					
				}
			}
			if (!found)
				pr++;
		}
		fclose(f) ;
	}
	uci_foreach_sections("network", "route", s) {
		if (s != NULL) {
			pr++;
		}
	}
	uci_foreach_sections("network", "route_disabled", s) {
		if (s != NULL) {
			pr++;
		}
	}
	dmasprintf(value, "%d", pr);
	return 0;
}

int entry_method_root_layer3_forwarding(struct dmctx *ctx)
{
	char *iroute = NULL;
	char *cur_iroute = NULL;
	char *permission = dmstrdup("1");
	struct uci_section *s = NULL;
	
	IF_MATCH(ctx, DMROOT"Layer3Forwarding.") {
		DMOBJECT(DMROOT"Layer3Forwarding.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"Layer3Forwarding.Forwarding.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("DefaultConnectionService", ctx, "1", get_parameter_by_linker, set_layer3_def_conn_serv, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ForwardNumberOfEntries", ctx, "0", get_layer3_nbr_entry, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		uci_foreach_sections("network", "route", s) {
			if (s != NULL ) {
				init_args_rentry(ctx, s, "1", "");
				iroute = update_instance(s, cur_iroute, "routeinstance");
				SUBENTRY(get_object_layer3, ctx, iroute, permission);
				if (cur_iroute)
					dmfree(cur_iroute);
				cur_iroute = dmstrdup(iroute);
				dmfree(iroute);
			}
			else 
				break;
		}
		dmfree(permission);
		return 0;
	}
	dmfree(permission);
	return FAULT_9005;
}