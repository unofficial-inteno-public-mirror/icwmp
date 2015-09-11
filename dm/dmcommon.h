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
#include <json/json.h>


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
int get_interface_enable_ubus(char *refparam, struct dmctx *ctx, char **value);
int set_interface_enable_ubus(char *refparam, struct dmctx *ctx, int action, char *value);
int get_interface_firewall_enabled(char *refparam, struct dmctx *ctx, char **value);
struct uci_section *create_firewall_zone_config(char *fwl, char *iface, char *input, char *forward, char *output);
int set_interface_firewall_enabled(char *refparam, struct dmctx *ctx, int action, char *value);
int dmcmd(char *cmd, int n, ...);
int dmcmd_read(int pipe, char *buffer, int size);
int ipcalc(char *ip_str, char *mask_str, char *start_str, char *end_str, char *ipstart_str, char *ipend_str);
int ipcalc_rev_start(char *ip_str, char *mask_str, char *ipstart_str, char *start_str);
int ipcalc_rev_end(char *ip_str, char *mask_str, char *start_str, char *ipend_str, char *end_str);

#endif
