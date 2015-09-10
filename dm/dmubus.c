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
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <libubus.h>
#include <libubox/blobmsg_json.h>
#include <json/json.h>
#include <stdio.h>
#include "dmubus.h"
#include "dmmem.h"

#define DELIMITOR ","

struct dmubus_ctx dmubus_ctx;
struct ubus_context *ubus_ctx;

static int timeout = 1000;
const char *ubus_socket = NULL;
json_object *json_res = NULL;

static inline bool dmblobmsg_add_object(struct blob_buf *b, json_object *obj)
{
	json_object_object_foreach(obj, key, val) {
	if (!blobmsg_add_json_element(b, key, val))
		return false;
	}
	return true;
}

static inline bool dmblobmsg_json_object_from_uargs(struct blob_buf *b, char *key, char *val)
{
	bool status;
	json_object *jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(val);
	json_object_object_add(jobj,key, jstring);
	status = dmblobmsg_add_object(b, jobj);
	json_object_put(jobj);
	return status;
}

static inline int ubus_arg_cmp(struct ubus_arg *src_args, int src_size, struct ubus_arg dst_args[], int dst_size)
{
	if (src_size != dst_size)
		return -1;
	int i;
	for (i = 0; i < src_size; i++) {
		if (strcmp( src_args[i].key, dst_args[i].key) != 0 || strcmp( src_args[i].val, dst_args[i].val) != 0)
			return -1;
	}
	return 0;
}

static void receive_call_result_data(struct ubus_request *req, int type, struct blob_attr *msg) 
{
	if (!msg)
		return;
	char *str = blobmsg_format_json_indent(msg, true, -1);
	if (!str) {
		json_res = NULL;
		return;
	}
	json_res = json_tokener_parse((const char *)str);
	free(str); //MEM should be free and not dmfree
	if (json_res != NULL && (is_error(json_res))) {
		json_object_put(json_res);
		json_res = NULL;
	}
}

static inline json_object *ubus_call_req(char *obj, char *method, struct ubus_arg u_args[], int u_args_size)
{
	struct blob_buf b = {0};
	uint32_t id;
	int ret;
	json_res = NULL;

	ubus_ctx = ubus_connect(ubus_socket);
	if (!ubus_ctx)
		return NULL;

	blob_buf_init(&b, 0);
	int i=0;
	for (i = 0; i < u_args_size; i++) {
		if (!dmblobmsg_json_object_from_uargs(&b, u_args[i].key, u_args[i].val))
			goto end_error;
	}
	ret = ubus_lookup_id(ubus_ctx, obj, &id);
	ubus_invoke(ubus_ctx, id, method, b.head, receive_call_result_data, NULL, timeout);
	blob_buf_free(&b);
	return json_res;

end_error:
	blob_buf_free(&b);
	return NULL;
}

int dmubus_call(char *obj, char *method, struct ubus_arg u_args[], int u_args_size, json_object **req_res)
{
	struct ubus_obj *i = NULL;
	struct ubus_meth *j = NULL;
	struct ubus_msg *k = NULL;
	json_object **jr;
	bool found = false;
	*req_res = NULL;
	list_for_each_entry(i, &dmubus_ctx.obj_head, list) {
		if (strcmp(obj, i->name) == 0) {
			found = true;
			break;
		}
	}
	if (!found) {
		i = dmcalloc(1, sizeof(struct ubus_obj));
		//init method head
		INIT_LIST_HEAD(&i->method_head);
		i->name = dmstrdup(obj);
		list_add(&i->list, &dmubus_ctx.obj_head);
	}
	found = false;
	list_for_each_entry(j, &i->method_head, list) {
		if (strcmp(method, j->name) == 0) {
			*req_res = j->res;
			found = true;
			break;
		}
	}
	if (!found) {
		j = dmcalloc(1, sizeof(struct ubus_meth));
		//init message head
		INIT_LIST_HEAD(&j->msg_head);
		j->name = dmstrdup(method);
		list_add(&j->list, &i->method_head);
		jr = &j->res;
	}
	// Arguments
	if (u_args_size != 0) {
		found = false;
		int n=0;
		list_for_each_entry(k, &j->msg_head, list) {
			if (ubus_arg_cmp(k->ug, k->ug_size, u_args, u_args_size) == 0) {
				*req_res = k->res;
				found = true;
				break;
			}
		}
		if (!found) {
			k = dmcalloc(1, sizeof(struct ubus_msg));
			list_add(&k->list, &j->msg_head);
			k->ug = dmcalloc(u_args_size, sizeof(struct ubus_arg));
			k->ug_size = u_args_size;
			jr = &k->res;
			int c;
			for (c = 0; c < u_args_size; c++) {
				k->ug[c].key = dmstrdup(u_args[c].key);
				k->ug[c].val = dmstrdup(u_args[c].val);
			}
		}
	}
	if (!found) {
		*jr = ubus_call_req(obj, method, u_args, u_args_size);
		*req_res = *jr;
	}
	return 0;
}

void dmubus_ctx_free(struct dmubus_ctx *ctx)
{
	struct ubus_obj *i, *_i;
	struct ubus_meth *j, *_j;
	struct ubus_msg *k, *_k;

	list_for_each_entry_safe(i, _i, &ctx->obj_head, list) {
		list_for_each_entry_safe(j, _j, &i->method_head, list) {
			list_for_each_entry_safe(k, _k, &j->msg_head, list) {
				if(k->ug_size != 0) {
					int c;
					for (c = 0; c < k->ug_size; c++) {
						dmfree(k->ug[c].key);
						dmfree(k->ug[c].val);
					}
					dmfree(k->ug);
				}
				list_del(&k->list);
				if(k->res)
					json_object_put(k->res);
				dmfree(k);
			}
			list_del(&j->list);
			if(j->res)
				json_object_put(j->res);
			dmfree(j->name);
			dmfree(j);
		}
		list_del(&i->list);
		dmfree(i->name);
		dmfree(i);
	}
}

static inline void print_json_value(json_object *jobj, char **value)
{
	*value = (char *)json_object_get_string(jobj);
}

void json_parse_array( json_object *jobj, char *key, int index, char *next_key, char **value)
{
	enum json_type type;
	int i;
	int len;
	char val[512] = {0};
	json_object *jarray = jobj;/*Simply get the array*/

	*value = "";
	if(key) {
		jarray = json_object_object_get(jobj, key);/*Getting the array if it is a key value pair*/
	}
	if(!jarray) {
		return;
	}
	int arraylen = json_object_array_length(jarray);/*Getting the length of the array*/
	json_object *jvalue;
	//read all tab
	if (index == -1) {
		for (i=0; i< arraylen; i++) {
			//jvalue = json_object_array_get_idx(jarray, i); /*Getting the array element at position i*/
			json_parse_array( jarray, NULL, i, NULL, value);
			//CONCAT
			len = strlen(val);
			if (len != 0) {
				memcpy(val + len, DELIMITOR, sizeof(DELIMITOR));
				strcpy(val + len + sizeof(DELIMITOR) - 1, *value);
			}
			else 
				strcpy(val, *value);
		}
		*value = dmstrdup(val); // MEM WILL BE FREED IN DMMEMCLEAN
	} else if (index < arraylen) {
		jvalue = json_object_array_get_idx(jarray, index);/*Getting the array element at position i*/
		type = json_object_get_type(jvalue);		
		if (type == json_type_array) {
			json_parse_array(jvalue, NULL, index, NULL, value);
		}
		else if (type == 0) {
			*value = "";
		}
		else if (type != json_type_object) {
			print_json_value(jvalue, value);
		}
		else {
			json_select(jvalue, next_key, index, next_key, value, NULL);
		}
	}
	else {
		*value = "";
	}
}

/*Parsing the json object*/
int json_select(json_object *jobj, char *search, int index, char *next_key, char **value, json_object **jobjres)
{
	enum json_type type;
	json_object *jobj1 = NULL;
	if (value != NULL) *value = "";
	if (jobjres != NULL) *jobjres = NULL;
	json_object_object_foreach(jobj, key, val) {
		if (strcmp(key, search) == 0) {
			type = json_object_get_type(val);
			switch (type) {
				case json_type_boolean:
				case json_type_double:
				case json_type_int:
				case json_type_string:
					print_json_value(val, value);
					return 0;
				case json_type_object:
					jobj1 = json_object_object_get(jobj, key);
					if (next_key == NULL) {
						*jobjres = json_object_object_get(jobj, key);
						return 0;
					}
					else {
						json_select(jobj1, next_key, index, NULL, value, jobjres);
						return 0;
					}
				case json_type_array:
					jobj1 = json_object_object_get(jobj, key);
					json_parse_array(jobj1, NULL, index, next_key, value);
					return 0;
			}
			return 0;
		}
	}
	return -1;
}
