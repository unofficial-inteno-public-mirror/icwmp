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
#endif
#ifdef DATAMODEL_TR181
#define DMROOT_CWMP "Device"
#endif
#define DMROOT_UPNP ""

#define DMDELIM_UPNP '/'
#define DMDELIM_CWMP '.'

#define DM_PROMPT "icwmp>"

#define UPNP_CFG "tr064"

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

typedef struct dm_upnp_supported_dm_s {
	char *location;
	char *uri;
	char *url;
	char *description;
	char *source_location;
} UPNP_SUPPORTED_DM;

struct set_tmp {
	struct list_head list;
	char *name;
	char *value;
	unsigned int flags;
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

struct dm_upnp_enabled_track {
	struct list_head list;
	char *name;
	char *key;
	char *value;
	unsigned int isobj;
};

struct dm_parameter {
	struct list_head list;
	char *name;
	char *data;
	char *type; 
	char *version;
	unsigned int flags;
};

struct dmctx
{
	bool stop;
	bool match;
	int (*method_param)(DMPARAM_ARGS);
	int (*method_obj)(DMOBJECT_ARGS);
	int (*checkobj)(DMOBJECT_ARGS);
	int (*checkleaf)(DMOBJECT_ARGS);
	struct list_head list_parameter;
	struct list_head set_list_tmp;
	struct list_head list_fault_param;
	DMOBJ *dm_entryobj;
	bool nextlevel;
	int depth;
	int faultcode;
	int setaction;
	char *in_param;
	char *in_notification;
	bool notification_change;
	char *in_value;
	char *addobj_instance;
	char *linker;
	char *linker_param;
	unsigned int dmparam_flags;
	unsigned int alias_register;
	unsigned int nbrof_instance;
	unsigned int amd_version;
	unsigned int instance_mode;
	unsigned int dm_type;
	unsigned int user_mask;
	unsigned char inparam_isparam;
	unsigned char findparam;
	char all_instances[512];
	char *inst_buf[16];
	char *instance_wildchar;
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

struct dm_acl {
	unsigned int flag;
	char *user_access;
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
	CMD_UPNP_GET_SUPPORTED_PARAMETERS,
	CMD_UPNP_GET_INSTANCES,
	CMD_UPNP_GET_SELECTED_VALUES,
	CMD_UPNP_GET_VALUES,
	CMD_UPNP_SET_VALUES,
	CMD_UPNP_GET_ATTRIBUTES,
	CMD_UPNP_SET_ATTRIBUTES,
	CMD_UPNP_DEL_INSTANCE,
	CMD_UPNP_ADD_INSTANCE,
	CMD_UPNP_GET_ACLDATA,
	CMD_UPNP_INIT_STATE_VARIABLES,
	CMD_UPNP_LOAD_ENABLED_PARAMETRS_TRACK,
	CMD_UPNP_GET_CONFIGURATION_UPDATE,
	CMD_UPNP_GET_CURRENT_CONFIGURATION_VERSION,
	CMD_UPNP_GET_SUPPORTED_DATA_MODEL_UPDATE,
	CMD_UPNP_GET_SUPPORTED_PARAMETERS_UPDATE,
	CMD_UPNP_GET_ATTRIBUTE_VALUES_UPDATE,
	CMD_UPNP_GET_ENABLED_PARAMETRS_ALARM,
	CMD_UPNP_GET_ENABLED_PARAMETRS_EVENT,
	CMD_UPNP_GET_ENABLED_PARAMETRS_VERSION,
	CMD_UPNP_CHECK_CHANGED_PARAMETRS_ALARM,
	CMD_UPNP_CHECK_CHANGED_PARAMETRS_EVENT,
	CMD_UPNP_CHECK_CHANGED_PARAMETRS_VERSION,
	CMD_EXTERNAL_COMMAND
};

enum fault_code_enum {
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

enum upnp_fault_code_enum {
	FAULT_UPNP_606 = 606,// Action not authorized
	FAULT_UPNP_701 = 701,// Invalid Argument Syntax
	FAULT_UPNP_702,//Invalid XML Argument
	FAULT_UPNP_703,// No Such Name
	FAULT_UPNP_704,// Invalid Value Type
	FAULT_UPNP_705,// Invalid Value
	FAULT_UPNP_706,// Read Only Violation
	FAULT_UPNP_707,// Multiple Set
	FAULT_UPNP_708,// Resource Temporarily Unavailable
	__FAULT_UPNP_MAX
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

enum dm_param_flags_enum{
	/* UPNP OnChange flags flags */
	DM_PARAM_ALARAM_ON_CHANGE = 1 << 0,
	DM_PARAM_EVENT_ON_CHANGE = 1 << 1,
	/* UPNP type flags */
	NODE_DATA_ATTRIBUTE_INSTANCE = 0x0010,
	NODE_DATA_ATTRIBUTE_MULTIINSTANCE = 0x0020,
	NODE_DATA_ATTRIBUTE_TYPESTRING = 0x0100,
	NODE_DATA_ATTRIBUTE_TYPEINT = 0x0200,
	NODE_DATA_ATTRIBUTE_TYPELONG = 0x0400,
	NODE_DATA_ATTRIBUTE_TYPEBOOL = 0x0800,
	NODE_DATA_ATTRIBUTE_TYPEDATETIME = 0x1000,
	NODE_DATA_ATTRIBUTE_TYPEBASE64 = 0x2000,
	NODE_DATA_ATTRIBUTE_TYPEBIN = 0x4000,
	NODE_DATA_ATTRIBUTE_TYPEPTR = 0x8000,
	NODE_DATA_ATTRIBUTE_TYPEMASK = 0x0000FF00,
	/*ACLRoles*/
	DM_PUBLIC_LIST = 1 << 0,
	DM_PUBLIC_READ = 1 << 1,
	DM_PUBLIC_WRITE = 1 << 2,
	DM_PUBLIC_MASK = DM_PUBLIC_LIST|DM_PUBLIC_READ|DM_PUBLIC_WRITE,
	DM_BASIC_LIST = 1 << 3,
	DM_BASIC_READ = 1 << 4,
	DM_BASIC_WRITE = 1 << 5,
	DM_BASIC_MASK = DM_PUBLIC_MASK|DM_BASIC_LIST|DM_BASIC_READ|DM_BASIC_WRITE,
	DM_XXXADMIN_LIST = 1 << 6,
	DM_XXXADMIN_READ = 1 << 7,
	DM_XXXADMIN_WRITE = 1 << 8,
	DM_XXXADMIN_MASK = DM_BASIC_MASK|DM_XXXADMIN_LIST|DM_XXXADMIN_READ|DM_XXXADMIN_WRITE,
	DM_SUPERADMIN_MASK = DM_XXXADMIN_MASK | (1 << 9),
	DM_LIST_MASK = DM_PUBLIC_LIST|DM_BASIC_LIST|DM_XXXADMIN_LIST,
	DM_READ_MASK = DM_PUBLIC_READ|DM_BASIC_READ|DM_XXXADMIN_READ,
	DM_WRITE_MASK = DM_PUBLIC_WRITE|DM_BASIC_WRITE|DM_XXXADMIN_WRITE,
	DM_FACTORIZED = 1 << 31
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
extern struct list_head list_upnp_enabled_onevent;
extern struct list_head list_upnp_enabled_onalarm;
extern struct list_head list_upnp_enabled_version;
extern struct list_head list_upnp_changed_onevent;
extern struct list_head list_upnp_changed_onalarm;
extern struct list_head list_upnp_changed_version;

extern int end_session_flag;
extern int ip_version;
extern char dm_delim;
extern char DMROOT[64];
extern unsigned int upnp_in_user_mask;

char *update_instance(struct uci_section *s, char *last_inst, char *inst_opt);
char *update_instance_alias(int action, char **last_inst , void *argv[]);
char *update_instance_without_section(int action, char **last_inst, void *argv[]);
int get_empty(char *refparam, struct dmctx *args, char **value);
void add_list_paramameter(struct dmctx *ctx, char *param_name, char *param_data, char *param_type, char *param_version, unsigned int flags);
void del_list_parameter(struct dm_parameter *dm_parameter);
void free_all_list_parameter(struct dmctx *ctx);
void add_set_list_tmp(struct dmctx *ctx, char *param, char *value, unsigned int flags);
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
int dm_entry_upnp_get_instances(struct dmctx *ctx);
int dm_entry_upnp_get_selected_values(struct dmctx *dmctx);
int dm_entry_upnp_get_values(struct dmctx *dmctx);
int dm_entry_upnp_set_values(struct dmctx *dmctx);
int dm_entry_upnp_get_attributes(struct dmctx *dmctx);
int upnp_state_variables_init(struct dmctx *dmctx);
int dm_entry_upnp_tracked_parameters(struct dmctx *dmctx);
int dm_entry_upnp_get_instance_numbers(struct dmctx *dmctx);
char *dm_entry_get_all_instance_numbers(struct dmctx *pctx, char *param);
void free_all_list_enabled_notify();
void free_all_list_upnp_param_track(struct list_head *head);
void dm_update_enabled_notify(struct dm_enabled_notify *p, char *new_value);
void dm_update_enabled_notify_byname(char *name, char *new_value);
char *get_last_instance(char *package, char *section, char *opt_inst);
char *get_last_instance_lev2(char *package, char *section, char *opt_inst, char *opt_check, char *value_check);
char *handle_update_instance(int instance_ranck, struct dmctx *ctx, char **last_inst, char * (*up_instance)(int action, char **last_inst, void *argv[]), int argc, ...);
int dm_add_end_session(struct dmctx *ctx, void(*function)(struct execute_end_session *), int action, void *data);
int apply_end_session();
void cwmp_set_end_session (unsigned int flag);
char *dm_print_path(char *fpath, ...);
void dm_upnp_apply_config(void);
void add_list_upnp_param_track(struct dmctx *dmctx, struct list_head *pchead, char *param, char *key, char *value, unsigned int isobj);


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
