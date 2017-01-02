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
	char *path = "/etc/rc.d/*sysntpd";
	
	if (check_file(path))
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_time_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	int check; 
	pid_t pid;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b) {
				DMCMD("/etc/rc.common", 2, "/etc/init.d/sysntpd", "enable"); //TODO wait ubus command
				pid = get_pid("ntpd");
				if (pid < 0) {
					DMCMD("/etc/rc.common", 2, "/etc/init.d/sysntpd", "start"); //TODO wait ubus command
				}
			}
			else {
				DMCMD("/etc/rc.common", 2, "/etc/init.d/sysntpd", "disable"); //TODO wait ubus command
				pid = get_pid("ntpd");
				if (pid > 0) {
					DMCMD("/etc/rc.common", 2, "/etc/init.d/sysntpd", "stop"); //TODO may be should be updated with ubus call uci
				}
			}
			return 0;
	}
	return 0;
}

//WE CAN WORK WITHOUT FOUND VALUE TO UPDATE
int get_time_ntpserver(char *refparam, struct dmctx *ctx, char **value, int index)
{
	char *pch;
	bool found = 0;
	int element = 0;
	struct uci_list *v;
	struct uci_element *e;
	
	dmuci_get_option_value_list("system","ntp","server", &v);
	if (v) {
		uci_foreach_element(v, e) {
			element++;
			if (element == index) {
				*value = dmstrdup(e->name); // MEM WILL BE FREED IN DMMEMCLEAN
				found = 1; 
				break;
			}
		}
	}
	if (!found) {
		*value = "";
		return 0;
	}
	if (strcmp(*value, "none") == 0) {
		*value = "";
	}
	return 0;
}

int get_time_ntpserver1(char *refparam, struct dmctx *ctx, char **value)
{
	return get_time_ntpserver(refparam, ctx, value, 1);
}

int get_time_ntpserver2(char *refparam, struct dmctx *ctx, char **value)
{
	return get_time_ntpserver(refparam, ctx, value, 2);
}

int get_time_ntpserver3(char *refparam, struct dmctx *ctx, char **value)
{
	return get_time_ntpserver(refparam, ctx, value, 3);
}

int get_time_ntpserver4(char *refparam, struct dmctx *ctx, char **value)
{
	return get_time_ntpserver(refparam, ctx, value, 4);
}

int get_time_ntpserver5(char *refparam, struct dmctx *ctx, char **value)
{
	return get_time_ntpserver(refparam, ctx, value, 5);
}

int set_time_ntpserver(char *refparam, struct dmctx *ctx, int action, char *value, int index)
{
	char *pch, *path;
	int check;
	struct uci_list *v;
	struct uci_element *e;
	int count = 0;
	int i = 0;
	char *ntp[5] = {0};
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_option_value_list("system", "ntp", "server", &v);
			if (v) {
				uci_foreach_element(v, e) {
					if ((count+1) == index) {
						ntp[count] = dmstrdup(value);
					}
					else {
					ntp[count] = dmstrdup(e->name);
				}
					count++;
					if (count > 4)
						break;
				}
			}
			if (index > count) {
				ntp[index-1] = dmstrdup(value);
				count = index;
				}
			for (i = 0; i < 5; i++) {
				if (ntp[i] && (*ntp[i]) != '\0')
					count = i+1;
			}
			dmuci_delete("system", "ntp", "server", NULL);
			for (i = 0; i < count; i++) {
				dmuci_add_list_value("system", "ntp", "server", ntp[i] ? ntp[i] : "");
				dmfree(ntp[i]);
			}
			return 0;
	}
	return 0;
}

int set_time_ntpserver1(char *refparam, struct dmctx *ctx, int action, char *value)
{
	return set_time_ntpserver(refparam, ctx, action, value, 1);
}

int set_time_ntpserver2(char *refparam, struct dmctx *ctx, int action, char *value)
{
	return set_time_ntpserver(refparam, ctx, action, value, 2);
}

int set_time_ntpserver3(char *refparam, struct dmctx *ctx, int action, char *value)
{
	return set_time_ntpserver(refparam, ctx, action, value, 3);
}

int set_time_ntpserver4(char *refparam, struct dmctx *ctx, int action, char *value)
{
	return set_time_ntpserver(refparam, ctx, action, value, 4);
}

int set_time_ntpserver5(char *refparam, struct dmctx *ctx, int action, char *value)
{
	return set_time_ntpserver(refparam, ctx, action, value, 5);
}

int entry_method_root_Time(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"Time.") {
		DMOBJECT(DMROOT"Time.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("Enable", ctx, "1", get_time_enable, set_time_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer1", ctx, "1", get_time_ntpserver1, set_time_ntpserver1, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer2", ctx, "1", get_time_ntpserver2, set_time_ntpserver2, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer3", ctx, "1", get_time_ntpserver3, set_time_ntpserver3, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer4", ctx, "1", get_time_ntpserver4, set_time_ntpserver4, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("NTPServer5", ctx, "1", get_time_ntpserver5, set_time_ntpserver5, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
