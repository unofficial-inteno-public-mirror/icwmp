/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_EXTERNAL_H__
#define _FREECWMP_EXTERNAL_H__
#include <libubox/list.h>

#ifdef DUMMY_MODE
static char *fc_script = "./ext/openwrt/scripts/freecwmp.sh";
#else
static char *fc_script = "/usr/sbin/freecwmp";
#endif
static char *fc_script_set_actions = "/tmp/freecwmp_set_action.sh";
static char *fc_script_get_actions = "/tmp/freecwmp_get_action.sh";

struct external_parameter {
	struct list_head list;
	char *name;
	char *data; /* notification for GetParameterAttribute; writable for GetParameterNames; value for GetParameterValues*/
	char *type;
	char *fault_code;
};

void external_setParamValRespStatus (char *status);
void external_fetch_setParamValRespStatus (char **status);
void external_setParamAttrResp (char *status, char *fault);
void external_fetch_setParamAttrResp (char **status, char **fault);
void external_addObjectResp (char *instance, char *status, char *fault);
void external_fetch_addObjectResp (char **instance, char **status, char **fault);
void external_delObjectResp (char *status, char *fault);
void external_fetch_delObjectResp (char **status, char **fault);
int external_get_action(char *action, char *name, char *arg);
int external_get_action_write(char *action, char *name, char *arg);
int external_get_action_execute();
int external_set_action_write(char *action, char *name, char *value, char *change);
int external_set_action_execute(char *action);
int external_simple(char *arg);
int external_download(char *url, char *size);
void external_add_list_paramameter(char *param_name, char *param_data, char *param_type, char *fault_code);
void external_free_list_parameter();

#endif

