/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author MOHAMED Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *	  Author Feten Besbes <feten.besbes@pivasoftware.com>
 *
 */
#ifndef __DMCWMP_H__
#define __DMCWMP_H__
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libubox/list.h>
#include "dmuci.h"
#include "dmmem.h"

#define DMROOT "InternetGatewayDevice."
#ifdef UNDEF
#undef UNDEF
#endif

#define UNDEF -1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define IF_MATCH(ctx, prefix, ...)																				\
	(ctx)->match = (ctx)->tree; 																				\
	if (!(ctx)->tree) {																							\
		sprintf((ctx)->current_obj, prefix, ##__VA_ARGS__);														\
		if (strstr((ctx)->in_param, (ctx)->current_obj) || strstr((ctx)->current_obj, (ctx)->in_param)) {		\
			(ctx)->match = true; 																				\
		}																										\
	}																											\
	if ((ctx)->match)

#define DMOBJECT(name, ctx, permission, notif_permission, addobj, delobj, linker, ...) {	\
	sprintf((ctx)->current_obj, name, ##__VA_ARGS__);										\
	int error = ctx->method_obj(ctx, permission, notif_permission, addobj, delobj, linker);	\
	if ((ctx)->stop) return error;															\
}

#define DMPARAM(lastname, ctx, permission, get_cmd, set_cmd, type, forced_inform, notif_permission, forced_notify, linker) {					\
	int error = ctx->method_param(lastname, ctx, permission, get_cmd, set_cmd, type, forced_inform, notif_permission, forced_notify, linker);	\
	if ((ctx)->stop) return error;																												\
}

#define SUBENTRY(f, ctx, ...) {			\
	int error = f(ctx, ## __VA_ARGS__);	\
	if ((ctx)->stop) return error;		\
}

#define DMPARAM_API_ARGS \
	char *lastname, \
	struct dmctx *ctx, \
	char *permission, \
	int (*get_cmd)(char *refparam, struct dmctx *ctx, char **value), \
	int (*set_cmd)(char *refparam, struct dmctx *ctx, int action, char *value), \
	char *type, \
	bool forced_inform, \
	bool notif_permission, \
	int forced_notify, \
	char *linker

#define DMOBJECT_API_ARGS \
	struct dmctx *ctx, \
	char *permission, \
	bool notif_permission, \
	int (*addobj)(struct dmctx *ctx, char **instance), \
	int (*delobj)(struct dmctx *ctx), \
	char *linker

#define TAILLE_MAX 1024
	
struct set_tmp {
	struct list_head list;
	char *name;
	char *value;
};

struct param_fault {
	struct list_head list;
	char *name;
	int fault;
};

struct dm_parameter {
	struct list_head list;
	char *name;
	char *data;
	char *type; 
};

struct dmctx
{
	bool stop;
	bool tree;
	bool match;
	int (*method_param)(DMPARAM_API_ARGS);
	int (*method_obj)(DMOBJECT_API_ARGS);
	void *args;
	struct list_head list_parameter;
	struct list_head set_list_tmp;
	struct list_head list_fault_param;
	bool nextlevel;
	int faultcode;
	int setaction;
	char *in_param;
	char *in_notification; 
	char *in_value;
	char *addobj_instance;
	char current_obj[512];
};

struct prefix_method {
	const char *prefix_name;
	bool enable;
	bool (*set_enable)(void);
	bool forced_inform;
	int (*method)(struct dmctx *ctx);
};

struct notification {
	char *value;
	char *type;
};

enum set_value_action {
	VALUECHECK,
	VALUESET
};

enum {
	CMD_GET_VALUE,
	CMD_GET_NAME,
	CMD_GET_NOTIFICATION,
	CMD_SET_VALUE,
	CMD_SET_NOTIFICATION,
	CMD_ADD_OBJECT,
	CMD_DEL_OBJECT,
	CMD_INFORM,
};

enum fault_code {
	FAULT_9000 = 9000,// Method not supported
	FAULT_9001,// Request denied
	FAULT_9002,// Internal error
	FAULT_9003,// Invalid arguments
	FAULT_9004,// Resources exceeded
	FAULT_9005,// Invalid parameter name
	FAULT_9006,// Invalid parameter type
	FAULT_9007,// Invalid parameter value
	FAULT_9008,// Attempt to set a non-writable parameter
	FAULT_9009,// Notification request rejected
	FAULT_9010,// Download failure
	FAULT_9011,// Upload failure
	FAULT_9012,// File transfer server authentication failure
	FAULT_9013,// Unsupported protocol for file transfer
	FAULT_9014,// Download failure: unable to join multicast group
	FAULT_9015,// Download failure: unable to contact file server
	FAULT_9016,// Download failure: unable to access file
	FAULT_9017,// Download failure: unable to complete download
	FAULT_9018,// Download failure: file corrupted
	FAULT_9019,// Download failure: file authentication failure
	__FAULT_MAX
};

char *update_instance(struct uci_section *s, char *last_inst, char *inst_opt);
char *max_instance(char *package, char *stype, char *option, char *inst_option, char *value);
int get_empty(char *refparam, struct dmctx *args, char **value);
void add_list_paramameter(struct dmctx *ctx, char *param_name, char *param_data, char *param_type);
void del_list_parameter(struct dm_parameter *dm_parameter);
void free_all_list_parameter(struct dmctx *ctx);
void add_set_list_tmp(struct dmctx *ctx, char *param, char *value);
void del_set_list_tmp(struct set_tmp *set_tmp);
void free_all_set_list_tmp(struct dmctx *ctx);
void add_list_fault_param(struct dmctx *ctx, char *param, int fault);
void del_list_fault_param(struct param_fault *param_fault);
void free_all_list_fault_param(struct dmctx *ctx);
int string_to_bool(char *v, bool *b);
int dm_entry_get_value(struct dmctx *ctx);
int dm_entry_get_name(struct dmctx *ctx);
int dm_entry_get_notification(struct dmctx *ctx);
int dm_entry_inform(struct dmctx *ctx);
int dm_entry_add_object(struct dmctx *ctx);
int dm_entry_delete_object(struct dmctx *ctx);
int dm_entry_set_value(struct dmctx *ctx);
int dm_entry_set_notification(struct dmctx *ctx);
int dm_entry_set_prefix_methods_enable(void);

#ifndef TRACE
#define TRACE_TYPE 0
static inline void trace_empty_func()
{
}
#if TRACE_TYPE == 2
#define TRACE(MESSAGE,args...) do { \
	const char *A[] = {MESSAGE}; \
	printf("TRACE: %s %s %d\n",__FUNCTION__,__FILE__,__LINE__); fflush(stdout);\
	if(sizeof(A) > 0) \
		printf(*A,##args); \
} while(0)
#elif TRACE_TYPE == 1
#define TRACE(MESSAGE, ...) printf(MESSAGE, ## __VA_ARGS__)
#else
#define TRACE(MESSAGE, ...) trace_empty_func()
#endif
#endif



#ifndef DETECT_CRASH
#define DETECT_CRASH(MESSAGE,args...) { \
	const char *A[] = {MESSAGE}; \
	printf("DETECT_CRASH: %s %s %d\n",__FUNCTION__,__FILE__,__LINE__); fflush(stdout);\
	if(sizeof(A) > 0) \
		printf(*A,##args); \
	sleep(1); \
}
#endif

#endif
