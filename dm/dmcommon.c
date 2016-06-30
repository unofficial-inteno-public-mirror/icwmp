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
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <uci.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"

char *array_notifcation_char[__MAX_notification] = {
	[notification_none] = "0",
	[notification_passive] = "1",
	[notification_active] = "2",
};

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

int get_interface_enable_ubus(char *iface, char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", iface}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "up", 0, NULL, value, NULL);
	return 0;
}

int set_interface_enable_ubus(char *iface, char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *ubus_object;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			dmastrcat(&ubus_object, "network.interface.", iface);
			if(b) {
				dmubus_call_set(ubus_object, "up", UBUS_ARGS{}, 0);
			}
			else
				dmubus_call_set(ubus_object, "down", UBUS_ARGS{}, 0);
			dmfree(ubus_object);
			return 0;
	}	
	return 0;
}

int get_interface_firewall_enabled(char *iface, char *refparam, struct dmctx *ctx, char **value)
{
	char *input = "", *forward = "";
	struct uci_section *s = NULL;

	uci_foreach_option_cont("firewall", "zone", "network", iface, s) {
		dmuci_get_value_by_section_string(s, "input", &input);
		dmuci_get_value_by_section_string(s, "forward", &forward);
		if (strcmp(input, "ACCEPT") !=0 && strcmp(forward, "ACCEPT") !=0) {
			*value = "1";
			return 0;
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

int set_interface_firewall_enabled(char *iface, char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	int cnt = 0;
	struct uci_section *s = NULL;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				value = "DROP";
			else
				value = "ACCEPT";
			uci_foreach_option_cont("firewall", "zone", "network", iface, s) {
				dmuci_set_value_by_section(s, "input", value);
				dmuci_set_value_by_section(s, "forward", value);
				cnt++;
			}
			if (cnt == 0 && b)
				create_firewall_zone_config("fwl", iface, "DROP", "DROP", "ACCEPT");
			return 0;
	}
	return 0;
}

int dmcmd(char *cmd, int n, ...)
{
	va_list arg;
	int i, pid;
	static int dmcmd_pfds[2];
	char *argv[n+2];

	argv[0] = cmd;

	va_start(arg,n);
	for (i=0; i<n; i++)
	{
		argv[i+1] = va_arg(arg, char*);
	}
	va_end(arg);

	argv[n+1] = NULL;

	if (pipe(dmcmd_pfds) < 0)
		return -1;

	if ((pid = fork()) == -1)
		return -1;

	if (pid == 0) {
		/* child */
		close(dmcmd_pfds[0]);
		dup2(dmcmd_pfds[1], 1);
		close(dmcmd_pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);
	} else if (pid < 0)
		return -1;

	/* parent */
	close(dmcmd_pfds[1]);

	int status;
	while (waitpid(pid, &status, 0) != pid);

	return dmcmd_pfds[0];
}

int dmcmd_no_wait(char *cmd, int n, ...)
{
	va_list arg;
	int i, pid;
	static int dmcmd_pfds[2];
	char *argv[n+2];

	argv[0] = cmd;
	va_start(arg,n);
	for (i=0; i<n; i++)
	{
		argv[i+1] = strdup(va_arg(arg, char*));
	}
	va_end(arg);

	argv[n+1] = NULL;

	if ((pid = fork()) == -1)
		return -1;

	if (pid == 0) {
		execvp(argv[0], (char **) argv);
		exit(ESRCH);
	} else if (pid < 0)
		return -1;
	return 0;
}
int dmcmd_read(int pipe, char *buffer, int size)
{
	int rd;
	if (size < 2) return -1;
	if ((rd = read(pipe, buffer, (size-1))) > 0) {
		buffer[rd] = '\0';
		return (rd + 1);
	}
	else {
		buffer[0] = '\0';
		return -1;
	}
	return -1;
}

int ipcalc(char *ip_str, char *mask_str, char *start_str, char *end_str, char *ipstart_str, char *ipend_str)
{
//TODO test it in accordance with inteno issue #7467
	struct in_addr ip;
	struct in_addr mask;
	struct in_addr ups;
	struct in_addr upe;
	int start, end;
	unsigned int umask;
	unsigned int addr;

	inet_aton(ip_str, &ip);
	inet_aton(mask_str, &mask);

	start = atoi(start_str);

	ups.s_addr = htonl(ntohl(ip.s_addr & mask.s_addr) + start);
	strcpy(ipstart_str, inet_ntoa(ups));

	if (end_str) {
		end = atoi(end_str);
		upe.s_addr = htonl(ntohl(ups.s_addr) + end);
		strcpy(ipend_str, inet_ntoa(upe));
	}
	return 0;
}

int ipcalc_rev_start(char *ip_str, char *mask_str, char *ipstart_str, char *start_str)
{
//TODO test it in accordance with inteno issue #7467
	struct in_addr ip;
	struct in_addr mask;
	struct in_addr ups;
	struct in_addr upe;
	int start;
	unsigned int umask;
	unsigned int addr;

	inet_aton(ip_str, &ip);
	inet_aton(mask_str, &mask);
	inet_aton(ipstart_str, &ups);

	start = ntohl(ups.s_addr) - ntohl(ip.s_addr & mask.s_addr);
	sprintf(start_str, "%d", start);
	return 0;
}

int ipcalc_rev_end(char *ip_str, char *mask_str, char *start_str, char *ipend_str, char *end_str)
{
//TODO test it in accordance with inteno issue #7467
	struct in_addr ip;
	struct in_addr mask;
	struct in_addr upe;
	int end;

	inet_aton(ip_str, &ip);
	inet_aton(mask_str, &mask);
	inet_aton(ipend_str, &upe);

	end = ntohl(upe.s_addr) - ntohl(ip.s_addr & mask.s_addr) - atoi(start_str);
	sprintf(end_str, "%d", end);
	return 0;
}

int network_get_ipaddr(char **value, char *iface)
{
	json_object *res;
	
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", iface}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "ipv4-address", 0, "address", value, NULL);
	return 0;
}

void remove_vid_interfaces_from_ifname(char *vid, char *ifname, char *new_ifname)
{
	char *sv, *pch, *p = new_ifname, *spch;
	new_ifname[0] = '\0';
	bool append;

	ifname = dmstrdup(ifname);
	pch = strtok_r(ifname, " ", &spch);
	while (pch != NULL) {
		append = false;
		char *sv = strchr(pch, '.');
		if (sv) {
			sv++;
			if (strcmp(sv, vid) != 0) {
				append = true;
			}
		}
		else {
			append = true;
		}
		if (append) {
			if (p == new_ifname) {
				dmstrappendstr(p, pch);
			}
			else {
				dmstrappendchr(p, ' ');
				dmstrappendstr(p, pch);
			}
		}
		pch = strtok_r(NULL, " ", &spch);
	}
	dmstrappendend(p);
	dmfree(ifname);
}

void update_section_option_list(char *config, char *section, char *option, char *option_2,char *val, char *val_2, char *name)
{
	char *add_value;
	int i = 0;
	char *baseifname;
	struct uci_section *prev_s= NULL, *s;
	char *instance = NULL, *last_instance  = NULL, *value;
	bool add_sec = true;
	
	if (name[0] == '\0') {
		add_sec = false;
	}
	uci_foreach_option_eq(config, section, option_2, val_2, s) {
		dmuci_get_value_by_section_string(s, option, &baseifname);
		if (!strstr(name, baseifname))
		{
			//delete section if baseifname  does not belong to ifname
			if (prev_s) {
				dmuci_delete_by_section(prev_s, NULL, NULL);
			}
			prev_s = s;
		} else if (strstr(name, baseifname) && (strcmp(baseifname,val) ==0)) {
			//do not add baseifname if exist
			add_sec = false;
		}
	}
	if (prev_s) {
		dmuci_delete_by_section(prev_s, NULL, NULL);
	}
	if (add_sec) {
		dmuci_add_section(config, section, &s, &add_value);
		dmuci_set_value_by_section(s, option, val);
		dmuci_set_value_by_section(s, option_2, val_2);
	}
}


void update_section_list(char *config, char *section, char *option, int number, char *filter, char *option1, char *val1,  char *option2, char *val2)
{
	char *add_value;
	struct uci_section *s = NULL;
	int i = 0;

	if (option) {
		uci_foreach_option_eq(config, section, option, filter, s) {
			return;
		}
	} else {
		uci_foreach_sections(config, section, s) {
			return;
		}
	}
	while (i < number) {
		dmuci_add_section(config, section, &s, &add_value);
		if (option)dmuci_set_value_by_section(s, option, filter);
		if (option1)dmuci_set_value_by_section(s, option1, val1);
		if (option2)dmuci_set_value_by_section(s, option2, val2);
		i++;
	}
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

int get_cfg_layer2idx(char *pack, char *section_type, char *option, int shift)
{
	char *si, *value;
	int idx = 0, max = -1;
	struct uci_section *s = NULL;

	uci_foreach_sections(pack, section_type, s) {
		dmuci_get_value_by_section_string(s, option, &value);
		si = value + shift;
		idx = atoi(si);
		if (idx > max)
			max = idx;
	}
	return (max + 1);
}

int wan_remove_dev_interface(struct uci_section *interface_setion, char *dev)
{
	char *ifname, new_ifname[64], *p, *pch, *spch;
	new_ifname[0] = '\0';
	p = new_ifname;
	dmuci_get_value_by_section_string(interface_setion, "ifname", &ifname);
	ifname = dmstrdup(ifname);
	for (pch = strtok_r(ifname, " ", &spch); pch; pch = strtok_r(NULL, " ", &spch)) {
		if (!strstr(pch, dev)) {
			if (new_ifname[0] != '\0') {
				dmstrappendchr(p, ' ');
			}
			dmstrappendstr(p, pch);
		}
	}
	dmstrappendend(p);
	dmfree(ifname);
	if (new_ifname[0] == '\0') {
		dmuci_delete_by_section(interface_setion, NULL, NULL);
	}
	else {
		dmuci_set_value_by_section(interface_setion, "ifname", new_ifname);
	}
	return 0;
}

int filter_lan_device_interface(struct uci_section *s, void *v)
{
	char *ifname = NULL;
	char *phy_itf = NULL, *phy_itf_local;
	char *pch, *spch, *ftype, *islan;

	dmuci_get_value_by_section_string(s, "type", &ftype);
	if (strcmp(ftype, "alias") != 0) {
		dmuci_get_value_by_section_string(s, "is_lan", &islan);
		if (islan[0] == '1' && strcmp(section_name(s), "loopback") != 0 )
			return 0;
		dmuci_get_value_by_section_string(s, "ifname", &ifname);
		db_get_value_string("hw", "board", "ethernetLanPorts", &phy_itf);
		phy_itf_local = dmstrdup(phy_itf);
		for (pch = strtok_r(phy_itf_local, " ", &spch); pch != NULL; pch = strtok_r(NULL, " ", &spch)) {
			if (strstr(ifname, pch)) {
				dmfree(phy_itf_local);
				return 0;
			}
		}
		dmfree(phy_itf_local);
	}
	return -1;
}

void update_remove_vlan_from_bridge_interface(char *bridge_key, struct uci_section *vb)
{
	char *ifname,*vid;
	char new_ifname[128];
	struct uci_section *s;

	uci_foreach_option_eq("network", "interface", "bridge_instance", bridge_key, s)
	{
		break;
	}
	if (!s) return;
	dmuci_get_value_by_section_string(vb, "vid", &vid);
	dmuci_get_value_by_section_string(s, "ifname", &ifname);
	remove_vid_interfaces_from_ifname(vid, ifname, new_ifname);
	dmuci_set_value_by_section(s, "ifname", new_ifname);
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
		if(strncmp(value, "br-", 3) == 0)
			value += 3;
		if (strcmp(value, section_name(lds)) == 0)
			return 0;
	}
	return -1;
}

void remove_interface_from_ifname(char *iface, char *ifname, char *new_ifname)
{
	char *pch, *spch, *p = new_ifname;
	new_ifname[0] = '\0';

	ifname = dmstrdup(ifname);
	pch = strtok_r(ifname, " ", &spch);
	while (pch != NULL) {
		if (strcmp(pch, iface) != 0) {
			if (p == new_ifname) {
				dmstrappendstr(p, pch);
			}
			else {
				dmstrappendchr(p, ' ');
				dmstrappendstr(p, pch);
			}
		}
		pch = strtok_r(NULL, " ", &spch);
	}
	dmstrappendend(p);
	dmfree(ifname);
}

int max_array(int a[], int size)
{
	int i, max = 0;
	for (i = 0; i< size; i++)
	{
		if(a[i] > max )
		max = a[i];
	}
	return max;
}

int check_ifname_is_vlan(char *ifname)
{
	char *pch = NULL;
	pch = strrchr(ifname, '.');
	if (pch && atoi(pch+1) >= 2)
		return 1;
	return 0;
}

int dmcommon_check_notification_value(char *value)
{
	int i;
	for (i = 0; i< __MAX_notification; i++) {
		if (strcmp(value, array_notifcation_char[i]) == 0)
			return 0;
	}
	return -1;
}
