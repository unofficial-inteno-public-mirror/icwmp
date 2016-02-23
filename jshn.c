/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <unistd.h>
#include <libubus.h>

#ifdef _AADJ
#include <json-c/json.h>
#else
#include <json/json.h>
#endif

#include "cwmp.h"
#include "external.h"
#include "log.h"
#include "jshn.h"

static json_object *jshn_obj = NULL;

static int jshn_message_parse(char **policy, int size, char **tb, char *msg)
{
	int i;
	json_object *obj;

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
		tb[i] = (char *)json_object_get_string(obj);
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

	DD(INFO,"triggered handle download fault %s", tb[DOWNLOAD_FAULT]);

	external_downloadFaultResp (tb[DOWNLOAD_FAULT]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}

enum upload_fault {
	UPLOAD_FAULT,
	__UPLOAD_MAX
};

char *upload_fault_policy[] = {
	[UPLOAD_FAULT] = "fault_code"
};
int
cwmp_handle_uploadFault(char *msg)
{
	int tmp;
	char *tb[__UPLOAD_MAX] = {0};

	jshn_message_parse(upload_fault_policy, ARRAYSIZEOF(upload_fault_policy), tb, msg);

	if (!tb[UPLOAD_FAULT])
		goto error;

	DD(INFO,"triggered handle upload fault %s", tb[UPLOAD_FAULT]);

	external_uploadFaultResp (tb[UPLOAD_FAULT]);

	jshn_message_delete();
	return 0;

error:
	jshn_message_delete();
	return -1;
}
