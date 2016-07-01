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

#ifdef DATAMODEL_TR098
#define DMROOT "InternetGatewayDevice"
#endif
#ifdef DATAMODEL_TR181
#define DMROOT "Device"
#endif
#ifdef UNDEF
#undef UNDEF
#endif

#define UNDEF -1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define IF_MATCH(ctx, prefix, ...)									\
	(ctx)->match = (ctx)->tree; 									\
	if (!(ctx)->tree) {												\
		sprintf((ctx)->current_obj, prefix, ##__VA_ARGS__);			\
		if (strstr((ctx)->in_param, (ctx)->current_obj) || 			\
			(strstr((ctx)->current_obj, (ctx)->in_param) && 		\
			(ctx)->in_param[strlen((ctx)->in_param)-1] == '.')) {	\
			(ctx)->match = true; 									\
		}															\
	}																\
	if ((ctx)->match)

#define IF_MATCH_ROOT(ctx)											\
	(ctx)->match = (ctx)->tree;										\
	if (!(ctx)->tree) {												\
		if (strstr((ctx)->in_param, DMROOT) &&						\
			!strchr(((ctx)->in_param + sizeof(DMROOT) - 1), '.')) {	\
			(ctx)->match = true;									\
		}															\
	}																\
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
// TO REMOVE JUST FOR COMPILE
#define SUBENTRY(f, ctx, ...) {			\
	int error = f(ctx, ## __VA_ARGS__);	\
	if ((ctx)->stop) return error;		\
}
// new sub entry
#define DM_LINK_INST_OBJ(dmctx, parent_node, data, instance) \
	do { \
		(dmctx)->faultcode = dm_link_inst_obj(dmctx, parent_node, data, instance); \
		if ((dmctx)->stop) \
			return (dmctx)->faultcode; \
	} while(0)

extern struct dm_permession_s DMREAD;
extern struct dm_permession_s DMWRITE;
extern struct dm_forced_inform_s DMFINFRM;

extern struct dm_notif_s DMNONE;
extern struct dm_notif_s DMACTIVE;
extern struct dm_notif_s DMPASSIVE;

enum dmt_type_enum {
	DMT_STRING,
	DMT_UNINT,
	DMT_INT,
	DMT_BOOL,
	DMT_TIME,
};

#define DMPARAM_ARGS \
	struct dmctx *dmctx, \
	struct dmnode *node, \
	char *lastname, \
	struct dm_permession_s *permission, \
	int type, \
	int (*get_cmd)(char *refparam, struct dmctx *dmctx, char **value), \
	int (*set_cmd)(char *refparam, struct dmctx *dmctx, char *value, int action), \
	struct dm_forced_inform_s *forced_inform, \
	struct dm_notif_s *notification, \
	char *(*get_linker)(char *refparam, struct dmctx *dmctx, void *data, char *instance), \
	void *data, \
	char *instance

#define DMOBJECT_ARGS \
	struct dmctx *dmctx, \
	struct dmnode *node, \
	struct dm_permession_s *permission, \
	int (*addobj)(struct dmctx *dmctx, char **instance), \
	int (*delobj)(struct dmctx *dmctx, unsigned char del_action), \
	struct dm_forced_inform_s *forced_inform, \
	struct dm_notif_s *notification, \
	char *(*get_linker)(char *refparam, struct dmctx *dmctx, void *data, char *instance), \
	void *data, \
	char *instance

#define TAILLE_MAX 1024

struct dm_forced_inform_s;
struct dm_permession_s;
struct dm_parameter;
struct dm_leaf_s;
struct dm_obj_s;
struct dmnode;
struct dmctx;
struct dm_notif_s;

struct dm_permession_s {
	char *val;
	char *(*get_permission)(char *refparam, struct dmctx *dmctx, void *data, char *instance);
};

struct dm_forced_inform_s {
	unsigned char val;
	unsigned char (*get_forced_inform)(char *refparam, struct dmctx *dmctx, void *data, char *instance);
};

struct dm_notif_s {
	char *val;
	char *(*get_notif)(char *refparam, struct dmctx *dmctx, void *data, char *instance);
};

typedef struct dm_leaf_s {
	/* PARAM, permission, type, getvalue, setvalue, forced_inform, notification(7)*/
	char *parameter;
	struct dm_permession_s *permission;
	int type;
	int (*getvalue)(char *refparam, struct dmctx *dmctx, char **value);
	int (*setvalue)(char *refparam, struct dmctx *dmctx, char *value, int action);
	struct dm_forced_inform_s *forced_inform;
	struct dm_notif_s *notification;
} DMLEAF;

typedef struct dm_obj_s {
	/* OBJ, permission, addobj, delobj, browseinstobj, forced_inform, notification, nextobj, leaf, linker(10)*/
	char *obj;
	struct dm_permession_s *permission;
	int (*addobj)(struct dmctx *dmctx, char **instance);
	int (*delobj)(struct dmctx *dmctx, unsigned char del_action);
	int (*browseinstobj)(struct dmctx *dmctx, struct dmnode *node, void *data, char *instance);
	struct dm_forced_inform_s *forced_inform;
	struct dm_notif_s *notification;
	struct dm_obj_s *nextobj;
	struct dm_leaf_s *leaf;
	char *(*get_linker)(struct dmctx *dmctx);
} DMOBJ;


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

struct dm_enabled_notify {
	struct list_head list;
	char *name;
	char *notification;
	char *value;
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
	int (*method_param)(DMPARAM_ARGS);
	int (*method_obj)(DMOBJECT_ARGS);
	int (*checkobj)(DMOBJECT_ARGS);
	int (*checkleaf)(DMOBJECT_ARGS);
	void *args;
	struct list_head list_parameter;
	struct list_head set_list_tmp;
	struct list_head list_fault_param;
	bool nextlevel;
	int faultcode;
	int setaction;
	char *in_param;
	char *in_notification;
	bool notification_change;
	char *in_value;
	char *addobj_instance;
	char *linker;
	char *linker_param;
	unsigned int alias_register;
	unsigned int nbrof_instance;
	unsigned int amd_version;
	unsigned int instance_mode;
	unsigned char inparam_isparam;
	unsigned char findobj;
	char current_obj[512];
	char *inst_buf[16];
};


typedef struct dmnode {
	DMOBJ *obj;
	struct dmnode *parent;
	char *current_object;
	unsigned char instance_level;
	unsigned char matched;
	unsigned char is_instanceobj;
} DMNODE;

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

enum del_action_enum {
	DEL_INST,
	DEL_ALL
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
	FAULT_9020,// Download failure: unable to complete download
	FAULT_9021,// Cancelation of file transfer not permitted
	FAULT_9022,// Invalid UUID format
	FAULT_9023,// Unknown Execution Environment
	FAULT_9024,// Disabled Execution Environment
	FAULT_9025,// Diployment Unit to Execution environment mismatch
	FAULT_9026,// Duplicate Deployment Unit
	FAULT_9027,// System Ressources Exceeded
	FAULT_9028,// Unknown Deployment Unit
	FAULT_9029,// Invalid Deployment Unit State
	FAULT_9030,// Invalid Deployment Unit Update: Downgrade not permitted
	FAULT_9031,// Invalid Deployment Unit Update: Version not specified
	FAULT_9032,// Invalid Deployment Unit Update: Version already exist
	__FAULT_MAX
};

enum {
	INSTANCE_UPDATE_NUMBER,
	INSTANCE_UPDATE_ALIAS
};

enum instance_mode {
	INSTANCE_MODE_NUMBER,
	INSTANCE_MODE_ALIAS
};

extern struct list_head list_enabled_notify;
extern struct list_head list_enabled_lw_notify;

char *update_instance(struct uci_section *s, char *last_inst, char *inst_opt);
char *update_instance_alias(int action, char **last_inst , void *argv[]);
char *update_instance_without_section(int action, char **last_inst, void *argv[]);
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
int dm_entry_enabled_notify(struct dmctx *ctx);
int dm_entry_get_linker(struct dmctx *ctx);
int dm_entry_get_linker_value(struct dmctx *ctx);
void free_all_list_enabled_notify();
void dm_update_enabled_notify(struct dm_enabled_notify *p, char *new_value);
void dm_update_enabled_notify_byname(char *name, char *new_value);
char *get_last_instance(char *package, char *section, char *opt_inst);
char *get_last_instance_lev2(char *package, char *section, char *opt_inst, char *opt_check, char *value_check);
char *handle_update_instance(int instance_ranck, struct dmctx *ctx, char **last_inst, char * (*up_instance)(int action, char **last_inst, void *argv[]), int argc, ...);

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
