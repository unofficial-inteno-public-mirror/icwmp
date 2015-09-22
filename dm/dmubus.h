/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 *		Author: Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 */

#ifndef __UBUS_H
#define __UBUS_H

#include <libubox/list.h>
#include <libubox/blobmsg_json.h>
#include <json/json.h>

#define UBUS_ARGS (struct ubus_arg[])
#define SIMPLE_OUTPUT -1
#define INDENT_OUTPUT 0
#define JSON_OUTPUT SIMPLE_OUTPUT

struct dmubus_ctx {
	struct list_head obj_head;
};

struct ubus_obj {
	struct list_head list;
	struct list_head method_head;
	char *name;
};

struct ubus_meth {
	struct list_head list;
	struct list_head msg_head;
	char *name;
	json_object *res;
};

struct ubus_msg {
	struct list_head list;
	struct ubus_arg *ug; // ubus method param
	int ug_size;
	json_object *res;
};

struct ubus_arg{
	char *key;
	char *val;
};

extern struct dmubus_ctx dmubus_ctx;
extern struct ubus_context *ubus_ctx;

int dmubus_call(char *obj, char *method, struct ubus_arg u_args[], int u_args_size, json_object **req_res);
int dmubus_call_set(char *obj, char *method, struct ubus_arg u_args[], int u_args_size);
void dmubus_ctx_free(struct dmubus_ctx *ctx);
void json_parse_array( json_object *jobj, char *key, int index, char *next_key, char **value);
int json_select(json_object *jobj, char *search, int index, char *next_key, char **value, json_object **jobjres);
#endif
