/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 *
 */

#ifndef _FREECWMP_EXTERNAL_H__
#define _FREECWMP_EXTERNAL_H__
#include <libubox/list.h>

#ifdef DUMMY_MODE
static char *fc_script = "./ext/openwrt/scripts/freecwmp.sh";
#else
static char *fc_script = "/usr/sbin/freecwmp";
#endif

extern pthread_mutex_t external_mutex_value_change;
extern struct list_head external_list_value_change;
extern struct list_head external_list_parameter;

void external_downloadFaultResp (char *fault_code);
void external_fetch_downloadFaultResp (char **fault_code);
void external_setParamValRespStatus (char *status);
void external_fetch_setParamValRespStatus (char **status);
void external_setParamAttrResp (char *status, char *fault);
void external_fetch_setParamAttrResp (char **status, char **fault);
void external_addObjectResp (char *instance, char *status, char *fault);
void external_fetch_addObjectResp (char **instance, char **status, char **fault);
void external_delObjectResp (char *status, char *fault);
void external_fetch_delObjectResp (char **status, char **fault);
int external_get_action(char *action, char *name, char *next_level);
int external_set_action(char *action, char *name, char *value, char *change);
int external_object_action(char *command, char *name);
int external_simple(char *command, char *arg);
int external_download(char *url, char *size, char *type, char *user, char *pass);
int external_apply(char *action, char *type);
int external_handle_action(int (*external_handler)(char *msg));
void external_add_list_paramameter(char *param_name, char *param_data, char *param_type, char *fault_code);
void external_free_list_parameter();
void external_init();
void external_exit();

#endif

