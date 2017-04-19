/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#ifndef __DM_COMMON_H
#define __DM_COMMON_H
#include <sys/types.h>
#include <libubox/blobmsg_json.h>
#include <json-c/json.h>
#include <unistd.h>
#include "dmcwmp.h"
#define NVRAM_FILE "/proc/nvram/WpaKey"
#define MAX_DHCP_LEASES 256
#define ARP_FILE "/proc/net/arp"
#define DMMAP "dmmap"
#define DHCPSTATICADDRESS_DISABLED_CHADDR "00:00:00:00:00:01"
#define DM_ASSERT(X, Y) \
do { \
	if(!(X)) { \
		Y; \
		return -1; \
	} \
} while(0)

#define dmstrappendstr(dest, src) \
do { \
	int len = strlen(src); \
	memcpy(dest, src, len); \
	dest += len; \
} while(0)

#define dmstrappendchr(dest, c) \
do { \
	*dest = c; \
	dest += 1; \
} while(0)

#define dmstrappendend(dest) \
do { \
	*dest = '\0'; \
} while(0)


#define DMCMD(CMD, N, ...) \
do { \
	int mpp = dmcmd(CMD, N, ## __VA_ARGS__); \
	if (mpp) close (mpp); \
} while (0)

void compress_spaces(char *str);
char *cut_fx(char *str, char *delimiter, int occurence);
pid_t get_pid(char *pname);
int check_file(char *path);
char *cidr2netmask(int bits);
void remove_substring(char *s, const char *str_remove);
bool is_strword_in_optionvalue(char *optionvalue, char *str);
int get_interface_enable_ubus(char *iface, char *refparam, struct dmctx *ctx, char **value);
int set_interface_enable_ubus(char *iface, char *refparam, struct dmctx *ctx, int action, char *value);
int get_interface_firewall_enabled(char *iface, char *refparam, struct dmctx *ctx, char **value);
struct uci_section *create_firewall_zone_config(char *fwl, char *iface, char *input, char *forward, char *output);
int set_interface_firewall_enabled(char *iface, char *refparam, struct dmctx *ctx, int action, char *value);
int dmcmd(char *cmd, int n, ...);
int dmcmd_read(int pipe, char *buffer, int size);
int dmcmd_no_wait(char *cmd, int n, ...);
int ipcalc(char *ip_str, char *mask_str, char *start_str, char *end_str, char *ipstart_str, char *ipend_str);
int ipcalc_rev_start(char *ip_str, char *mask_str, char *ipstart_str, char *start_str);
int ipcalc_rev_end(char *ip_str, char *mask_str, char *start_str, char *ipend_str, char *end_str);
int network_get_ipaddr(char **value, char *iface);
void remove_vid_interfaces_from_ifname(char *vid, char *ifname, char *new_ifname);
void update_section_option_list(char *config, char *section, char *option, char *option_2,char *val, char *val_2, char *name);
void update_section_list(char *config, char *section, char *option, int number, char *filter, char *option1, char *val1,  char *option2, char *val2);
char *get_nvram_wpakey();
int reset_wlan(struct uci_section *s);
int get_cfg_layer2idx(char *pack, char *section_type, char *option, int shift);
int wan_remove_dev_interface(struct uci_section *interface_setion, char *dev);
int filter_lan_device_interface(struct uci_section *s, void *v);
void update_remove_vlan_from_bridge_interface(char *bridge_key, struct uci_section *vb);
int filter_lan_ip_interface(struct uci_section *ss, void *v);
void remove_interface_from_ifname(char *iface, char *ifname, char *new_ifname);
int max_array(int a[], int size);
int check_ifname_is_vlan(char *ifname);
int set_uci_dhcpserver_option(struct dmctx *ctx, struct uci_section *s, char *option, char *value);
int update_uci_dhcpserver_option(struct dmctx *ctx, struct uci_section *s, char *option, char * new_option, char *value);

#endif
