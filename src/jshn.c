/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	Contributed by Inteno Broadband Technology AB
 *
 *	Copyright (C) 2013 Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	Copyright (C) 2013 Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <unistd.h>
#include <libubus.h>
#include <json/json.h>


#include "cwmp.h"
#include "external.h"
#include "log.h"
#include "jshn.h"

static json_object *jshn_obj = NULL;

static int jshn_message_parse(char **policy, int size, char **tb, char *msg)
{
	int i;
	json_object *obj;
	char		*c;

	jshn_obj = json_tokener_parse(msg);
	if (jshn_obj == NULL || is_error(jshn_obj) ||
		json_object_get_type(jshn_obj) != json_type_object)
	{
		jshn_obj = NULL;
		return -1;
	}
	for (i=0; i<size; i++)
	{
		obj = json_object_object_get(jshn_obj, policy[i]);
		if (obj == NULL || is_error(obj) ||
			json_object_get_type(obj) != json_type_string)
			continue;
		c = (char *)json_object_get_string(obj);
		if(c && c[0] != '\0')
			tb[i] = c;
	}
	return 0;
}

static inline void jshn_message_delete()
{
	if(jshn_obj != NULL)
		json_object_put(jshn_obj);
}
enum download_fault {
	DOWNLOAD_FAULT,
	__DOWNLOAD_MAX
};

char *download_fault_policy[] = {
	[DOWNLOAD_FAULT] = "fault_code"
};

int
cwmp_handle_downloadFault(char *msg)
{
	int tmp;
	char *tb[__DOWNLOAD_MAX] = {0};

	jshn_message_parse(download_fault_policy, ARRAYSIZEOF(download_fault_policy), tb, msg);

	if (!tb[DOWNLOAD_FAULT])
		goto error;

	CWMP_LOG(INFO,"triggered handle download fault %s", tb[DOWNLOAD_FAULT]);

	external_downloadFaultResp (tb[DOWNLOAD_FAULT]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}

enum getParamValues {
	GETPARAMVALUES_PARAM,
	GETPARAMVALUES_VALUE,
	GETPARAMVALUES_TYPE,
	GETPARAMVALUES_FAULT,
	__GETPARAMVALUES_MAX
};

char *getParamValues_policy[] = {
	[GETPARAMVALUES_PARAM] = "parameter",
	[GETPARAMVALUES_VALUE] = "value",
	[GETPARAMVALUES_TYPE] = "type",
	[GETPARAMVALUES_FAULT] = "fault_code",
};

int
cwmp_handle_getParamValues(char *msg)
{
	char *tb[__GETPARAMVALUES_MAX] = {0};

	jshn_message_parse(getParamValues_policy, ARRAYSIZEOF(getParamValues_policy), tb, msg);

	if (!tb[GETPARAMVALUES_PARAM])
		goto error;


	CWMP_LOG(INFO, "triggered handle get parameter value of: %s",
			     tb[GETPARAMVALUES_PARAM]);


	external_add_list_parameter(tb[GETPARAMVALUES_PARAM],
			tb[GETPARAMVALUES_VALUE],
			tb[GETPARAMVALUES_TYPE]? tb[GETPARAMVALUES_TYPE] : "xsd:string",
			tb[GETPARAMVALUES_FAULT]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}

enum getParamNames {
	GETPARAMNAMES_PARAM,
	GETPARAMNAMES_WRITABLE,
	GETPARAMNAMES_FAULT,
	__GETPARAMNAMES_MAX
};

char *getParamNames_policy[] = {
	[GETPARAMNAMES_PARAM] = "parameter",
	[GETPARAMNAMES_WRITABLE] = "writable",
	[GETPARAMNAMES_FAULT] = "fault_code",
};

int
cwmp_handle_getParamNames(char *msg)
{
	char *tb[__GETPARAMNAMES_MAX] = {0};

	jshn_message_parse(getParamNames_policy, ARRAYSIZEOF(getParamNames_policy), tb, msg);

	if (!tb[GETPARAMNAMES_PARAM])
		goto error;


	CWMP_LOG(INFO, "triggered handle get parameter name of: %s",
			     tb[GETPARAMNAMES_PARAM]);


	external_add_list_parameter(tb[GETPARAMNAMES_PARAM],
			tb[GETPARAMNAMES_WRITABLE],
			NULL,
			tb[GETPARAMNAMES_FAULT]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}

enum getParamAttributes {
	GETPARAMATTRIBUTES_PARAM,
	GETPARAMATTRIBUTES_NOTIF,
	GETPARAMATTRIBUTES_FAULT,
	__GETPARAMATTRIBUTES_MAX
};

char *getParamAttributes_policy[] = {
	[GETPARAMATTRIBUTES_PARAM] = "parameter",
	[GETPARAMATTRIBUTES_NOTIF] = "notification",
	[GETPARAMATTRIBUTES_FAULT] = "fault_code",
};

int
cwmp_handle_getParamAttributes(char *msg)
{
	char *tb[__GETPARAMATTRIBUTES_MAX] = {0};

	jshn_message_parse(getParamAttributes_policy, ARRAYSIZEOF(getParamAttributes_policy), tb, msg);

	if (!tb[GETPARAMATTRIBUTES_PARAM])
		goto error;

	if (!tb[GETPARAMATTRIBUTES_NOTIF])
		goto error;

	CWMP_LOG(INFO, "triggered handle get parameter attribute of: %s",
			     tb[GETPARAMATTRIBUTES_PARAM]);

	external_add_list_parameter(tb[GETPARAMATTRIBUTES_PARAM],
				      tb[GETPARAMATTRIBUTES_NOTIF],
				      NULL,
				      tb[GETPARAMATTRIBUTES_FAULT]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}

enum setParamAttributes {
	SETPARAMATTRIBUTES_SUCCESS,
	SETPARAMATTRIBUTES_FAULT,
	__SETPARAMATTRIBUTES_MAX
};

char *setParamAttributes_policy[] = {
	[SETPARAMATTRIBUTES_SUCCESS] = "success",
	[SETPARAMATTRIBUTES_FAULT] = "fault_code",
};

int
cwmp_handle_setParamAttributes(char *msg)
{
	char *tb[__SETPARAMATTRIBUTES_MAX] = {0};

	jshn_message_parse(setParamAttributes_policy, ARRAYSIZEOF(setParamAttributes_policy), tb, msg);

	CWMP_LOG(INFO, "triggered handle set parameter attribute");

	external_setParamAttrResp(tb[SETPARAMATTRIBUTES_SUCCESS],
			tb[SETPARAMATTRIBUTES_FAULT]);

	jshn_message_delete();
	return 0;
}

enum setParamValuesFault {
	SETPARAMVALUESFAULT_PARAM,
	SETPARAMVALUESFAULT_FAULT,
	__SETPARAMVALUESFAULT_MAX
};

char *setParamValuesFault_policy[] = {
	[SETPARAMVALUESFAULT_PARAM] = "parameter",
	[SETPARAMVALUESFAULT_FAULT] = "fault_code",
};

static int
cwmp_handle_setParamValuesFault(char *msg)
{
	char *tb[__SETPARAMVALUESFAULT_MAX] = {0};

	jshn_message_parse(setParamValuesFault_policy, ARRAYSIZEOF(setParamValuesFault_policy), tb, msg);

	if (!tb[SETPARAMVALUESFAULT_PARAM])
		goto error;

	if (!tb[SETPARAMVALUESFAULT_FAULT])
		goto error;


	CWMP_LOG(INFO, "triggered handle set parameter value fault (%s) of: %s",
						tb[SETPARAMVALUESFAULT_FAULT],
						tb[SETPARAMVALUESFAULT_PARAM]);


	external_add_list_parameter(tb[SETPARAMVALUESFAULT_PARAM],
						NULL,
						NULL,
						tb[SETPARAMVALUESFAULT_FAULT]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}

enum setParamValuesStatus {
	SETPARAMVALUESSTATUS_STATUS,
	__SETPARAMVALUESSTATUS_MAX
};

char *setParamValuesStatus_policy[] = {
	[SETPARAMVALUESSTATUS_STATUS] = "status",
};

static int
cwmp_handle_setParamValuesStatus(char *msg)
{
	char *tb[__SETPARAMVALUESSTATUS_MAX] = {0};

	jshn_message_parse(setParamValuesStatus_policy, ARRAYSIZEOF(setParamValuesStatus_policy), tb, msg);

	if (!tb[SETPARAMVALUESSTATUS_STATUS])
		goto error;


	CWMP_LOG(INFO, "triggered handle set parameter value status");

	external_setParamValRespStatus(tb[SETPARAMVALUESSTATUS_STATUS]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}

int
cwmp_handle_setParamValues(char *msg)
{
	if (!cwmp_handle_setParamValuesFault(msg))
		return 0;

	if(cwmp_handle_setParamValuesStatus(msg))
		return -1;

	return 0;
}

enum addObject {
	ADDOBJECT_INSTANCE,
	ADDOBJECT_STATUS,
	ADDOBJECT_FAULT,
	__ADDOBJECT_MAX
};

char *addObject_policy[] = {
	[ADDOBJECT_INSTANCE] = "instance",
	[ADDOBJECT_STATUS] = "status",
	[ADDOBJECT_FAULT] = "fault_code",
};

int
cwmp_handle_addObject(char *msg)
{
	char *tb[__ADDOBJECT_MAX] = {0};

	CWMP_LOG(INFO, "triggered handle add object");

	jshn_message_parse(addObject_policy, ARRAYSIZEOF(addObject_policy), tb, msg);

	external_addObjectResp(tb[ADDOBJECT_INSTANCE],
					tb[ADDOBJECT_STATUS],
					tb[ADDOBJECT_FAULT]);
	jshn_message_delete();
	return 0;
}

enum delObject {
	DELOBJECT_STATUS,
	DELOBJECT_FAULT,
	__DELOBJECT_MAX
};

char *delObject_policy[] = {
	[DELOBJECT_STATUS] = "status",
	[DELOBJECT_FAULT] = "fault_code",
};

int
cwmp_handle_delObject(char *msg)
{
	char *tb[__DELOBJECT_MAX] = {0};

	CWMP_LOG(INFO, "triggered handle delete object");

	jshn_message_parse(delObject_policy, ARRAYSIZEOF(delObject_policy), tb, msg);

	external_delObjectResp(tb[DELOBJECT_STATUS],
					tb[DELOBJECT_FAULT]);
	jshn_message_delete();
	return 0;
}
