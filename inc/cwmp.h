/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *
 */

#ifndef _CWMP_H__
#define _CWMP_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <microxml.h>
#include <libubox/list.h>

#define MAX_EVENTS							64
#define MAX_INT32							2147483646
#define MAX_INT_ID							MAX_INT32
#define MIN_INT_ID							836464
#define PERIOD_INFORM_MIN					60
#define PERIOD_INFORM_DEFAULT				86400
#define CONNECTION_REQUEST_RESTRICT_PERIOD	10
#define CONNECTION_REQUEST_RESTRICT_REQUEST	10
#define DEFAULT_CONNECTION_REQUEST_PORT		7547

#define DEFAULT_ACSURL						"http://192.168.1.1:8080/openacs/acs"
#define UCI_DHCP_DISCOVERY_PATH				"cwmp.acs.dhcp_discovery"
#define UCI_DHCP_ACS_URL_PATH				"cwmp.acs.dhcp_url_path"
#define UCI_ACS_URL_PATH					"cwmp.acs.url"
#define UCI_PERIODIC_INFORM_INTERVAL_PATH	"cwmp.acs.periodic_inform_interval"
#define UCI_PERIODIC_INFORM_ENABLE_PATH		"cwmp.acs.periodic_inform_enable"
#define UCI_ACS_USERID_PATH					"cwmp.acs.userid"
#define UCI_ACS_PASSWD_PATH					"cwmp.acs.passwd"
#define UCI_ACS_PARAMETERKEY_PATH			"cwmp.acs.ParameterKey"
#define UCI_LOG_SEVERITY_PATH				"cwmp.cpe.log_severity"
#define UCI_CPE_USERID_PATH					"cwmp.cpe.userid"
#define UCI_CPE_PASSWD_PATH					"cwmp.cpe.passwd"
#define UCI_CPE_INTERFACE_PATH              "cwmp.cpe.interface"
#define UCI_CPE_UBUS_SOCKET_PATH			"cwmp.cpe.ubus_socket"
#define UCI_CPE_PORT_PATH					"cwmp.cpe.port"
#define UCI_CPE_LOG_FILE_NAME				"cwmp.cpe.log_file_name"
#define UCI_CPE_LOG_MAX_SIZE				"cwmp.cpe.log_max_size"
#define UCI_CPE_ENABLE_STDOUT_LOG			"cwmp.cpe.log_to_console"
#define UCI_CPE_ENABLE_FILE_LOG				"cwmp.cpe.log_to_file"

enum end_session {
	END_SESSION_REBOOT = 1,
	END_SESSION_EXTERNAL_ACTION = 1<<1,
	END_SESSION_RELOAD = 1<<2,
	END_SESSION_FACTORY_RESET = 1<<3
};

enum cwmp_start {
	CWMP_START_BOOT  = 1,
	CWMP_START_PERIODIC = 2
};

enum cwmp_ret_err {
	CWMP_OK,			/* No Error */
	CWMP_GEN_ERR, 		/* General Error */
	CWMP_MEM_ERR,  		/* Memory Error */
	CWMP_MUTEX_ERR,
	CWMP_RETRY_SESSION
};

enum event_retry_after_enum {
	EVENT_RETRY_AFTER_TRANSMIT_FAIL = 0x1,
	EVENT_RETRY_AFTER_REBOOT = 0x2,
	EVENT_RETRY_AFTER_BOOTSTRAP = 0x4
};

enum event_type_enum {
	EVENT_TYPE_SINGLE = 0x0,
	EVENT_TYPE_MULTIPLE = 0x1
};

enum event_idx_enum {
	EVENT_IDX_0BOOTSTRAP,
	EVENT_IDX_1BOOT,
	EVENT_IDX_2PERIODIC,
	EVENT_IDX_3SCHEDULED,
	EVENT_IDX_4VALUE_CHANGE,
	EVENT_IDX_5KICKED,
	EVENT_IDX_6CONNECTION_REQUEST,
	EVENT_IDX_7TRANSFER_COMPLETE,
	EVENT_IDX_8DIAGNOSTICS_COMPLETE,
	EVENT_IDX_9REQUEST_DOWNLOAD,
	EVENT_IDX_10AUTONOMOUS_TRANSFER_COMPLETE,
	EVENT_IDX_M_Reboot,
	EVENT_IDX_M_ScheduleInform,
	EVENT_IDX_M_Download,
	EVENT_IDX_M_Upload,
	__EVENT_IDX_MAX
};

typedef struct event_container {
    struct list_head                    list;
    int 								code;	/* required element of type xsd:string */
	char 								*command_key;
    struct list_head                    head_parameter_container;
    int                                 id;
} event_container;

typedef struct parameter_container {
	struct list_head list;
	char *name;
	char *data;
	char *type;
	char *fault_code;
} parameter_container;

typedef struct EVENT_CONST_STRUCT
{
    char                                *CODE;
    unsigned int                        TYPE;
    unsigned short                      RETRY;

} EVENT_CONST_STRUCT;

typedef struct config {
    char                                *acsurl;
    char                                *acs_userid;
    char                                *acs_passwd;
    char                                *cpe_userid;
    char                                *cpe_passwd;
    char                                *dhcp_url_path;
    char								*ip;
    char								*interface;
    char                                *ubus_socket;
    int                                 connection_request_port;
    int                                 period;
    bool                                periodic_enable;
} config;

typedef struct env {
    unsigned short                      boot;
    unsigned short                      periodic;
    long int							max_firmware_size;
} env;

typedef struct config_uci_list {
    struct list_head                    list;
    char                                *value;
} config_uci_list;

typedef struct cwmp {
    struct env			env;
    struct config		conf;
    struct list_head	head_session_queue;
    pthread_mutex_t		mutex_session_queue;
    struct session		*session_send;
    pthread_mutex_t		mutex_session_send;
    pthread_cond_t		threshold_session_send;
    pthread_mutex_t		mutex_periodic;
    pthread_cond_t		threshold_periodic;
    int					retry_count_session;
    struct list_head	*head_event_container;
    int					pid_file;
} cwmp;

typedef struct session {
    struct list_head	list;
    struct list_head	head_event_container;
    struct list_head	head_rpc_cpe;
    struct list_head	head_rpc_acs;
    mxml_node_t			*tree_in;
    mxml_node_t			*tree_out;
    mxml_node_t			*body_in;
    bool				hold_request;
    bool				digest_auth;
    unsigned int		end_session;
    int					fault_code;
    int					error;
} session;

typedef struct rpc {
    struct list_head	list;
    int					type;
    void				*extra_data;
} rpc;

#define ARRAYSIZEOF(a)  (sizeof(a) / sizeof((a)[0]))
#define FREE(x) do { free(x); x = NULL; } while (0)

extern struct cwmp	cwmp_main;
extern const struct EVENT_CONST_STRUCT	EVENT_CONST [__EVENT_IDX_MAX];

struct rpc *cwmp_add_session_rpc_cpe (struct session *session, int type);
struct session *cwmp_add_queue_session (struct cwmp *cwmp);
struct rpc *cwmp_add_session_rpc_acs (struct session *session, int type);
struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);
int event_remove_all_event_container(struct session *session, int rem_from);
void cwmp_save_event_container (struct cwmp *cwmp,struct event_container *event_container);
void *thread_event_periodic (void *v);
void cwmp_add_notification (char *name, char *value, char *attribute, char *type);
int netlink_init(void);
char * mix_get_time(void);
void *thread_exit_program (void *v);

#endif /* _CWMP_H__ */
