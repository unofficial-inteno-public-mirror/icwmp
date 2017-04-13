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
#define DMROOT_CWMP "InternetGatewayDevice"
#define DMROOT_UPNP "/InternetGatewayDevice"
#endif
#ifdef DATAMODEL_TR181
#define DMROOT_CWMP "Device"
#define DMROOT_UPNP "/Device"
#endif

#define DMDELIM_UPNP '/'
#define DMDELIM_CWMP '.'

#define DM_PROMPT "icwmp>"

#ifdef UNDEF
#undef UNDEF
#endif
#define UNDEF -1

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifndef FREE
#define FREE(x) do { free(x); x = NULL; } while (0)
#endif

extern struct dm_permession_s DMREAD;
extern struct dm_permession_s DMWRITE;
extern struct dm_forced_inform_s DMFINFRM;

extern struct dm_notif_s DMNONE;
extern struct dm_notif_s DMACTIVE;
extern struct dm_notif_s DMPASSIVE;

#define DMPARAM_ARGS \
	struct dmctx *dmctx, \
	struct dmnode *node, \
	char *lastname, \
	struct dm_permession_s *permission, \
	int type, \
	int (*get_cmd)(char *refparam, struct dmctx *dmctx, char **value), \
	int (*set_cmd)(char *refparam, struct dmctx *dmctx, int action, char *value), \
	struct dm_forced_inform_s *forced_inform, \
	struct dm_notif_s *notification, \
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
	int (*get_linker)(char *refparam, struct dmctx *dmctx, void *data, char *instance, char **linker), \
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
	int (*setvalue)(char *refparam, struct dmctx *dmctx, int action, char *value);
	struct dm_forced_inform_s *forced_inform;
	struct dm_notif_s *notification;
} DMLEAF;

typedef struct dm_obj_s {
	/* OBJ, permission, addobj, delobj, browseinstobj, forced_inform, notification, nextobj, leaf, linker(10)*/
	char *obj;
	struct dm_permession_s *permission;
	int (*addobj)(struct dmctx *dmctx, char **instance);
	int (*delobj)(struct dmctx *dmctx, unsigned char del_action);
	bool (*checkobj)(struct dmctx *dmctx, void *data);
	int (*browseinstobj)(struct dmctx *dmctx, struct dmnode *node, void *data, char *instance);
	struct dm_forced_inform_s *forced_inform;
	struct dm_notif_s *notification;
	struct dm_obj_s *nextobj;
	struct dm_leaf_s *leaf;
	int (*get_linker)(char *refparam, struct dmctx *dmctx, void *data, char *instance, char **linker);
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
	unsigned int dm_type;
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

typedef struct execute_end_session {
	struct list_head list;
	int action;
	unsigned int dm_type;
	unsigned int amd_version;
	unsigned int instance_mode;
	void *data;
	void (*function)(struct execute_end_session *);
} execute_end_session;

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

enum end_session_enum {
	END_SESSION_REBOOT = 1,
	END_SESSION_EXTERNAL_ACTION = 1<<1,
	END_SESSION_RELOAD = 1<<2,
	END_SESSION_FACTORY_RESET = 1<<3,
	END_SESSION_IPPING_DIAGNOSTIC = 1<<4,
	END_SESSION_DOWNLOAD_DIAGNOSTIC = 1<<5,
	END_SESSION_UPLOAD_DIAGNOSTIC = 1<<6,
};

enum dm_browse_enum {
	DM_ERROR = -1,
	DM_OK = 0,
	DM_STOP = 1
};

enum dmt_type_enum {
	DMT_STRING,
	DMT_UNINT,
	DMT_INT,
	DMT_LONG,
	DMT_BOOL,
	DMT_TIME,
};

enum amd_version_enum{
	AMD_1 = 1,
	AMD_2,
	AMD_3,
	AMD_4,
	AMD_5,
};

enum dm_type_enum{
	DM_CWMP,
	DM_UPNP,
};

#define DM_CLEAN_ARGS(X) memset(&(X), 0, sizeof(X))
static inline int DM_LINK_INST_OBJ(struct dmctx *dmctx, DMNODE *parent_node, void *data, char *instance)
{
	dmctx->faultcode = dm_link_inst_obj(dmctx, parent_node, data, instance);
	if (dmctx->stop)
		return DM_STOP;
	return DM_OK;
}

extern struct list_head list_enabled_notify;
extern struct list_head list_enabled_lw_notify;
extern struct list_head list_execute_end_session;
extern int end_session_flag;
extern int ip_version;
extern char dm_delim;
extern char DMROOT[64];

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
int dm_entry_enabled_notify(struct dmctx *ctx);
int dm_entry_get_linker(struct dmctx *ctx);
int dm_entry_get_linker_value(struct dmctx *ctx);
void free_all_list_enabled_notify();
void dm_update_enabled_notify(struct dm_enabled_notify *p, char *new_value);
void dm_update_enabled_notify_byname(char *name, char *new_value);
char *get_last_instance(char *package, char *section, char *opt_inst);
char *get_last_instance_lev2(char *package, char *section, char *opt_inst, char *opt_check, char *value_check);
char *handle_update_instance(int instance_ranck, struct dmctx *ctx, char **last_inst, char * (*up_instance)(int action, char **last_inst, void *argv[]), int argc, ...);
int dm_add_end_session(struct dmctx *ctx, void(*function)(struct execute_end_session *), int action, void *data);
int apply_end_session();
void cwmp_set_end_session (unsigned int flag);
char *dm_print_path(char *fpath, ...);

#ifndef TRACE
#define TRACE_TYPE 0
static inline void trace_empty_func()
{
}
#if TRACE_TYPE == 2
#define TRACE(MESSAGE,args...) do { \
	const char *A[] = {MESSAGE}; \
	fprintf(stderr, "TRACE: %s %s %d ",__FUNCTION__,__FILE__,__LINE__); \
	if(sizeof(A) > 0) \
		fprintf(stderr, *A,##args); \
	fprintf(stderr, "\n"); \
	fflush(stderr); \
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
