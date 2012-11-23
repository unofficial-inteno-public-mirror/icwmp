/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <unistd.h>
#include <libubus.h>


#include "cwmp.h"
#include "ubus.h"
#include "external.h"

void cwmp_add_notification (char *name, char *value, char *type);

static struct ubus_context *ctx = NULL;

static enum notify {
	NOTIFY_PARAM,
	NOTIFY_VALUE,
	NOTIFY_TYPE,
	__NOTIFY_MAX
};

static const struct blobmsg_policy notify_policy[] = {
	[NOTIFY_PARAM] = { .name = "parameter", .type = BLOBMSG_TYPE_STRING },
	[NOTIFY_VALUE] = { .name = "value", .type = BLOBMSG_TYPE_STRING },
	[NOTIFY_TYPE] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_notify(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__NOTIFY_MAX];

	blobmsg_parse(notify_policy, ARRAY_SIZE(notify_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[NOTIFY_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;

	CWMP_LOG(INFO, "triggered ubus notification parameter %s",
						     blobmsg_data(tb[NOTIFY_PARAM]));
	cwmp_add_notification(blobmsg_data(tb[NOTIFY_PARAM]),
			tb[NOTIFY_VALUE]? blobmsg_data(tb[NOTIFY_VALUE]) : NULL,
			tb[NOTIFY_TYPE]? blobmsg_data(tb[NOTIFY_TYPE]) : NULL);

	return 0;
}

static enum inform {
	INFORM_EVENT,
	__INFORM_MAX
};

static const struct blobmsg_policy inform_policy[] = {
	[INFORM_EVENT] = { .name = "event", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_inform(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	int tmp;
	struct blob_attr *tb[__INFORM_MAX];

	blobmsg_parse(inform_policy, ARRAY_SIZE(inform_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[INFORM_EVENT])
		return UBUS_STATUS_INVALID_ARGUMENT;

	CWMP_LOG(INFO,"triggered ubus inform %s",
			     blobmsg_data(tb[INFORM_EVENT]));
//	tmp = freecwmp_int_event_code(blobmsg_data(tb[INFORM_EVENT]));
//	cwmp_connection_request(tmp);

	return 0;
}

static int
freecwmpd_handle_reload(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	CWMP_LOG(INFO, "triggered ubus reload");
//	freecwmp_reload();

	return 0;
}

static enum getParamValues {
	GETPARAMVALUES_PARAM,
	GETPARAMVALUES_VALUE,
	GETPARAMVALUES_TYPE,
	GETPARAMVALUES_FAULT,
	__GETPARAMVALUES_MAX
};

static const struct blobmsg_policy getParamValues_policy[] = {
	[GETPARAMVALUES_PARAM] = { .name = "parameter", .type = BLOBMSG_TYPE_STRING },
	[GETPARAMVALUES_VALUE] = { .name = "value", .type = BLOBMSG_TYPE_STRING },
	[GETPARAMVALUES_TYPE] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
	[GETPARAMVALUES_FAULT] = { .name = "fault_code", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_getParamValues(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__GETPARAMVALUES_MAX];

	blobmsg_parse(getParamValues_policy, ARRAY_SIZE(getParamValues_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[GETPARAMVALUES_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;


	CWMP_LOG(INFO, "triggered ubus GetParameterValues parameter %s",
			     blobmsg_data(tb[GETPARAMVALUES_PARAM]));


	external_add_list_paramameter(blobmsg_data(tb[GETPARAMVALUES_PARAM]),
			tb[GETPARAMVALUES_VALUE]? blobmsg_data(tb[GETPARAMVALUES_VALUE]) : NULL,
			tb[GETPARAMVALUES_TYPE]? blobmsg_data(tb[GETPARAMVALUES_TYPE]) : "xsd:string",
			tb[GETPARAMVALUES_FAULT]? blobmsg_data(tb[GETPARAMVALUES_FAULT]) : NULL);

	return 0;
}

static enum getParamNames {
	GETPARAMNAMES_PARAM,
	GETPARAMNAMES_WRITABLE,
	GETPARAMNAMES_FAULT,
	__GETPARAMNAMES_MAX
};

static const struct blobmsg_policy getParamNames_policy[] = {
	[GETPARAMNAMES_PARAM] = { .name = "parameter", .type = BLOBMSG_TYPE_STRING },
	[GETPARAMNAMES_WRITABLE] = { .name = "writable", .type = BLOBMSG_TYPE_STRING },
	[GETPARAMNAMES_FAULT] = { .name = "fault_code", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_getParamNames(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__GETPARAMNAMES_MAX];

	blobmsg_parse(getParamNames_policy, ARRAY_SIZE(getParamNames_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[GETPARAMNAMES_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;


	CWMP_LOG(INFO, "triggered ubus GetParameterNames parameter %s",
			     blobmsg_data(tb[GETPARAMNAMES_PARAM]));


	external_add_list_paramameter(blobmsg_data(tb[GETPARAMNAMES_PARAM]),
			tb[GETPARAMNAMES_WRITABLE]? blobmsg_data(tb[GETPARAMNAMES_WRITABLE]) : NULL,
			NULL,
			tb[GETPARAMNAMES_FAULT]? blobmsg_data(tb[GETPARAMNAMES_FAULT]) : NULL);

	return 0;
}

static enum getParamAttributes {
	GETPARAMATTRIBUTES_PARAM,
	GETPARAMATTRIBUTES_NOTIF,
	GETPARAMATTRIBUTES_FAULT,
	__GETPARAMATTRIBUTES_MAX
};

static const struct blobmsg_policy getParamAttributes_policy[] = {
	[GETPARAMATTRIBUTES_PARAM] = { .name = "parameter", .type = BLOBMSG_TYPE_STRING },
	[GETPARAMATTRIBUTES_NOTIF] = { .name = "notification", .type = BLOBMSG_TYPE_STRING },
	[GETPARAMATTRIBUTES_FAULT] = { .name = "fault_code", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_getParamAttributes(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__GETPARAMATTRIBUTES_MAX];

	blobmsg_parse(getParamAttributes_policy, ARRAY_SIZE(getParamAttributes_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[GETPARAMATTRIBUTES_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (!tb[GETPARAMATTRIBUTES_NOTIF])
		return UBUS_STATUS_INVALID_ARGUMENT;

	CWMP_LOG(INFO, "triggered ubus GetParameterAttributes parameter %s",
			     blobmsg_data(tb[GETPARAMATTRIBUTES_PARAM]));

	external_add_list_paramameter(blobmsg_data(tb[GETPARAMATTRIBUTES_PARAM]),
				      blobmsg_data(tb[GETPARAMATTRIBUTES_NOTIF]),
				      NULL,
				      blobmsg_data(tb[GETPARAMATTRIBUTES_FAULT]));

	return 0;
}

static enum setParamAttributes {
	SETPARAMATTRIBUTES_FAULT,
	__SETPARAMATTRIBUTES_MAX
};

static const struct blobmsg_policy setParamAttributes_policy[] = {
	[SETPARAMATTRIBUTES_FAULT] = { .name = "fault_code", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_setParamAttributes(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__SETPARAMATTRIBUTES_MAX];

	blobmsg_parse(setParamAttributes_policy, ARRAY_SIZE(setParamAttributes_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[SETPARAMATTRIBUTES_FAULT])
		return UBUS_STATUS_INVALID_ARGUMENT;

	CWMP_LOG(INFO, "triggered ubus SetParameterAttributes");

	external_setParamAttrRespFault(blobmsg_data(tb[SETPARAMATTRIBUTES_FAULT]));

	return 0;
}

static enum setParamValuesFault {
	SETPARAMVALUESFAULT_PARAM,
	SETPARAMVALUESFAULT__FAULT,
	__SETPARAMVALUESFAULT_MAX
};

static const struct blobmsg_policy setParamValuesFault_policy[] = {
	[SETPARAMVALUESFAULT_PARAM] = { .name = "parameter", .type = BLOBMSG_TYPE_STRING },
	[SETPARAMVALUESFAULT__FAULT] = { .name = "fault_code", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_setParamValuesFault(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__SETPARAMVALUESFAULT_MAX];

	blobmsg_parse(setParamValuesFault_policy, ARRAY_SIZE(setParamValuesFault_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[SETPARAMVALUESFAULT_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (!tb[SETPARAMVALUESFAULT__FAULT])
		return UBUS_STATUS_INVALID_ARGUMENT;


	CWMP_LOG(INFO, "triggered ubus SetParameterValues FAULT for the parameter %s",
			     blobmsg_data(tb[SETPARAMVALUESFAULT_PARAM]));


	external_add_list_paramameter(blobmsg_data(tb[SETPARAMVALUESFAULT_PARAM]),
						NULL,
						NULL,
						blobmsg_data(tb[SETPARAMVALUESFAULT__FAULT]));

	return 0;
}

static enum setParamValuesStatus {
	SETPARAMVALUESSTATUS_STATUS,
	__SETPARAMVALUESSTATUS_MAX
};

static const struct blobmsg_policy setParamValuesStatus_policy[] = {
	[SETPARAMVALUESSTATUS_STATUS] = { .name = "status", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_setParamValuesStatus(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__SETPARAMVALUESSTATUS_MAX];

	blobmsg_parse(setParamValuesStatus_policy, ARRAY_SIZE(setParamValuesStatus_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[SETPARAMVALUESSTATUS_STATUS])
		return UBUS_STATUS_INVALID_ARGUMENT;


	CWMP_LOG(INFO, "triggered ubus SetParameterValues status for the parameter");

	external_setParamValRespStatus(blobmsg_data(tb[SETPARAMVALUESSTATUS_STATUS]));

	return 0;
}

static enum addObject {
	ADDOBJECT_INSTANCE,
	ADDOBJECT_STATUS,
	ADDOBJECT_FAULT,
	__ADDOBJECT_MAX
};

static const struct blobmsg_policy addObject_policy[] = {
	[ADDOBJECT_INSTANCE] = { .name = "instance", .type = BLOBMSG_TYPE_STRING },
	[ADDOBJECT_STATUS] = { .name = "status", .type = BLOBMSG_TYPE_STRING },
	[ADDOBJECT_FAULT] = { .name = "fault_code", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_addObject(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__ADDOBJECT_MAX];

	blobmsg_parse(addObject_policy, ARRAY_SIZE(addObject_policy), tb,
		      blob_data(msg), blob_len(msg));

	external_addObjectResp(tb[ADDOBJECT_INSTANCE] ? blobmsg_data(tb[ADDOBJECT_INSTANCE]): NULL,
					tb[ADDOBJECT_STATUS] ? blobmsg_data(tb[ADDOBJECT_STATUS]): NULL,
					tb[ADDOBJECT_FAULT] ? blobmsg_data(tb[ADDOBJECT_FAULT]): NULL);
	return 0;
}

static enum delObject {
	DELOBJECT_STATUS,
	DELOBJECT_FAULT,
	__DELOBJECT_MAX
};

static const struct blobmsg_policy delObject_policy[] = {
	[DELOBJECT_STATUS] = { .name = "status", .type = BLOBMSG_TYPE_STRING },
	[DELOBJECT_FAULT] = { .name = "fault_code", .type = BLOBMSG_TYPE_STRING },
};

static int
freecwmpd_handle_delObject(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__DELOBJECT_MAX];

	blobmsg_parse(delObject_policy, ARRAY_SIZE(delObject_policy), tb,
		      blob_data(msg), blob_len(msg));

	external_delObjectResp(tb[DELOBJECT_STATUS] ? blobmsg_data(tb[DELOBJECT_STATUS]): NULL,
					tb[DELOBJECT_FAULT] ? blobmsg_data(tb[DELOBJECT_FAULT]): NULL);
	return 0;
}

static const struct ubus_method freecwmp_methods[] = {
	UBUS_METHOD("notify", freecwmpd_handle_notify, notify_policy),
	UBUS_METHOD("inform", freecwmpd_handle_inform, inform_policy),
	UBUS_METHOD("GetParameterValues", freecwmpd_handle_getParamValues, getParamValues_policy),
	UBUS_METHOD("SetParameterValuesFault", freecwmpd_handle_setParamValuesFault, setParamValuesFault_policy),
	UBUS_METHOD("SetParameterValuesStatus", freecwmpd_handle_setParamValuesStatus, setParamValuesStatus_policy),
	UBUS_METHOD("GetParameterNames", freecwmpd_handle_getParamNames, getParamNames_policy),
	UBUS_METHOD("GetParameterAttributes", freecwmpd_handle_getParamAttributes, getParamAttributes_policy),
	UBUS_METHOD("SetParameterAttributes", freecwmpd_handle_setParamAttributes, setParamAttributes_policy),
	UBUS_METHOD("AddObject", freecwmpd_handle_addObject, addObject_policy),
	UBUS_METHOD("DelObject", freecwmpd_handle_delObject, delObject_policy),
	{ .name = "reload", .handler = freecwmpd_handle_reload },
};

static struct ubus_object_type main_object_type =
	UBUS_OBJECT_TYPE("freecwmpd", freecwmp_methods);

static struct ubus_object main_object = {
	.name = "tr069",
	.type = &main_object_type,
	.methods = freecwmp_methods,
	.n_methods = ARRAY_SIZE(freecwmp_methods),
};

int
ubus_init(struct cwmp *cwmp)
{
	uloop_init();
	ctx = ubus_connect(cwmp->conf.ubus_socket);
	if (!ctx) return -1;

	ubus_add_uloop(ctx);

	if (ubus_add_object(ctx, &main_object)) return -1;
	uloop_run();
	return 0;
}

void
ubus_exit(void)
{
	if (ctx) ubus_free(ctx);
}