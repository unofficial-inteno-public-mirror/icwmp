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
#include "dmuci.h"
#include "dmcwmp.h"
#include "dmubus.h"
#include "times.h"
#include "dmcommon.h"

int get_time_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *path = dmstrdup("/etc/rc.d/*sysntpd");
	
	if (check_file(path))
		*value = dmstrdup("true");
	else
		*value = dmstrdup("false");
	dmfree(path);
	return 0;
}

int set_time_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	int check; 
	char *pname, *v;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			check = string_to_bool(value, &b);
			if (check == -1)
				return 0;
			if(b) {
				//delay_service restart "sysntpd" "1" //TODO
				///etc/init.d/sysntpd enable
			}
			else {
				pname = dmstrdup("pidof ntpd");
				v = get_pid(pname);
				if (v[0] != '\0') {
					//etc/init.d/sysntpd stop; //TODO
					//etc/init.d/sysntpd disable //TODO
				}
			}
	}
	return 0;
}

//WE CAN WORK WITHOUT FOUND VALUE TO UPDATE
int get_time_ntpserver(char *refparam, struct dmctx *ctx, char **value)
{
	char *pch;
	bool found = 0;
	int element = 0;
	struct uci_list *v;
	struct uci_element *e;
	int occurence;
	
	pch = dmstrdup(strrchr(refparam,'.'));
	pch = pch + 10;
	occurence = atoi(pch);
	dmuci_get_option_value_list("system","ntp","server", &v);
	if (v) {
		uci_foreach_element(v, e) {
			element++;
			if (element == occurence) {
				*value = dmstrdup(e->name);
				found = 1; 
				break;
			}
		}
	}
	if (!found) {
		*value = dmstrdup("");
		return 0;
	}
	if (strcmp(*value, "none") == 0) {
		dmfree(*value);
		*value = dmstrdup("");
	}
	return 0;
}

int set_time_ntpserver(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *pch, *path;
	int check, ntp_num;
	struct uci_list *v;
	struct uci_element *e;
	int count = 1;
	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			pch = dmstrdup(strrchr(refparam,'.'));
			pch = pch + 10;
			ntp_num = atoi(pch);
			dmuci_get_option_value_list("system", "ntp", "server", &v);
			dmuci_del_list_value("system", "ntp", "server", NULL); //TODO CHECK IF WE DON'T HAVE VALUE
			if (v) {
				uci_foreach_element(v, e) {
					if (count == ntp_num) {
						dmuci_add_list_value("system", "ntp", "server", value);
					}
					else {
						dmuci_add_list_value("system", "ntp", "server", e->name);
					}
					count++;
				}
			}
			while (count <= ntp_num) {
				if (count == ntp_num) {
					dmuci_add_list_value("system", "ntp", "server", value);
				}
				else {
					dmuci_add_list_value("system", "ntp", "server", "none");
				}
				count++;
			}
			path = dmstrdup("/etc/rc.d/*sysntpd");
			if (check_file(path)) {
				//delay_service restart "sysntpd" //TODO
				dmfree(path);
			}
			return 0;
	}
	return 0;
}

int entry_method_root_Time(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"Time.") {
		DMOBJECT(DMROOT"Time.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("Enable", ctx, "1", get_time_enable, set_time_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer1", ctx, "1", get_time_ntpserver, set_time_ntpserver, "", 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer2", ctx, "1", get_time_ntpserver, set_time_ntpserver, "", 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer3", ctx, "1", get_time_ntpserver, set_time_ntpserver, "", 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer4", ctx, "1", get_time_ntpserver, set_time_ntpserver, "", 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer5", ctx, "1", get_time_ntpserver, set_time_ntpserver, "", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}