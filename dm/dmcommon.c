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

#include <arpa/inet.h>
#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <uci.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "landevice.h"

void compress_spaces(char *str)
{
	char *dst = str;
	for (; *str; ++str) {
		*dst++ = *str;
		if (isspace(*str)) {
			do ++str;
			while (isspace(*str));
			--str;
		}
	}
	*dst = '\0';
}
char *cut_fx(char *str, char *delimiter, int occurence)
{
	int i = 1;
	char *pch, *spch;
	pch = strtok_r(str, delimiter, &spch);
	while (pch != NULL && i<occurence) {
		i++;
		pch = strtok_r(NULL, delimiter, &spch);
	}
	return pch;
}

pid_t get_pid(char *pname)
{
	DIR* dir;
	struct dirent* ent;
	char* endptr;
	char buf[512];

	if (!(dir = opendir("/proc"))) {
		return -1;
	}
	while((ent = readdir(dir)) != NULL) {
		long lpid = strtol(ent->d_name, &endptr, 10);
		if (*endptr != '\0') {
			continue;
		}
		snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
		FILE* fp = fopen(buf, "r");
		if (fp) {
			if (fgets(buf, sizeof(buf), fp) != NULL) {
				char* first = strtok(buf, " ");
				if (strstr(first, pname)) {
					fclose(fp);
					closedir(dir);
					return (pid_t)lpid;
				}
			}
			fclose(fp);
		}
	}
	closedir(dir);
	return -1;
}

int check_file(char *path) 
{
	glob_t globbuf;
	if(glob(path, 0, NULL, &globbuf) == 0) {
		globfree(&globbuf);
		return 1;
	}
	return 0;
}

char *cidr2netmask(int bits)
{
	uint32_t mask;
	struct in_addr ip_addr;
	uint8_t u_bits = (uint8_t)bits;
	char *netmask;
	char tmp[32] = {0};
	
	mask = ((0xFFFFFFFFUL << (32 - u_bits)) & 0xFFFFFFFFUL);
	mask = htonl(mask);
	ip_addr.s_addr = mask;
	return inet_ntoa(ip_addr);
}

void remove_substring(char *s, const char *str_remove)
{
	int len = strlen(str_remove);
	while (s = strstr(s, str_remove)) {
		memmove(s, s+len, 1+strlen(s+len));
    }
}

bool is_strword_in_optionvalue(char *optionvalue, char *str)
{
	int len;
	char *s = optionvalue;
	while ((s = strstr(s, str))) {
		len = strlen(str); //should be inside while, optimization reason
		if(s[len] == '\0' || s[len] == ' ')
			return true;
		s++;
	}
	return false;
}

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
	bool b;
	json_object *res;
	char *ubus_object;
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *lan_name = section_name(ipargs->ldipsection);
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			dmastrcat(&ubus_object, "network.interface.", lan_name);
			if(b) {
				dmubus_call(ubus_object, "up", UBUS_ARGS{}, 0, &res);
			}
			else
				dmubus_call(ubus_object, "down", UBUS_ARGS{}, 0, &res);
			dmfree(ubus_object);
			return 0;
	}	
	return 0;
}

int get_interface_firewall_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *input = "";
	struct uci_section *s = NULL;
	struct ldipargs *ipargs = (struct ldipargs *)ctx->args;
	char *lan_name = section_name(ipargs->ldipsection);

	uci_foreach_option_cont("firewall", "zone", "network", lan_name, s) {
		dmuci_get_value_by_section_string(s, "input", &input);
		if (strcmp(input, "ACCEPT") !=0 && strcmp(input, "forward") !=0) {
			*value = "1";
			return 0;
		}
		break; //TODO TO CHECK
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
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '1')
				value = "DROP";
			else if (value[0] == '0')
				value = "ACCEPT";
			else
				return 0;
			uci_foreach_option_cont("firewall", "zone", "network", lan_name, s) {
				dmuci_set_value_by_section(s, "input", value);
				dmuci_set_value_by_section(s, "forward", value);
				cnt++;
			}
			if (cnt == 0 && strcmp(value,"DROP") ==0)
				create_firewall_zone_config("fwl", lan_name, "DROP", "DROP", "");
			//delay_service reload "firewall" "1" //TODO BY IBH
			return 0;
	}
	return 0;
}