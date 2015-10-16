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
#define CONNECTION_REQUEST_RESTRICT_PERIOD	5
#define CONNECTION_REQUEST_RESTRICT_REQUEST	50
#define DEFAULT_CONNECTION_REQUEST_PORT		7547

#define DEFAULT_ACSURL						"http://192.168.1.1:8080/openacs/acs"
#define UCI_DHCP_DISCOVERY_PATH				"icwmp.acs.dhcp_discovery"
#define UCI_DHCP_ACS_URL_PATH				"icwmp.acs.dhcp_url_path"
#define UCI_ACS_URL_PATH					"icwmp.acs.url"
#define UCI_PERIODIC_INFORM_TIME_PATH		"icwmp.acs.periodic_inform_time"
#define UCI_PERIODIC_INFORM_INTERVAL_PATH	"icwmp.acs.periodic_inform_interval"
#define UCI_PERIODIC_INFORM_ENABLE_PATH		"icwmp.acs.periodic_inform_enable"
#define UCI_ACS_USERID_PATH					"icwmp.acs.userid"
#define UCI_ACS_PASSWD_PATH					"icwmp.acs.passwd"
#define UCI_ACS_PARAMETERKEY_PATH			"icwmp.acs.ParameterKey"
#define UCI_ACS_SSL_CAPATH					"icwmp.acs.ssl_capath"
#define UCI_ACS_SSL_VERSION			 		"icwmp.acs.ssl_version"
#define UCI_LOG_SEVERITY_PATH				"icwmp.cpe.log_severity"
#define UCI_CPE_USERID_PATH					"icwmp.cpe.userid"
#define UCI_CPE_PASSWD_PATH					"icwmp.cpe.passwd"
#define UCI_CPE_INTERFACE_PATH              "icwmp.cpe.interface"
#define UCI_CPE_UBUS_SOCKET_PATH			"icwmp.cpe.ubus_socket"
#define UCI_CPE_PORT_PATH					"icwmp.cpe.port"
#define UCI_CPE_LOG_FILE_NAME				"icwmp.cpe.log_file_name"
#define UCI_CPE_LOG_MAX_SIZE				"icwmp.cpe.log_max_size"
#define UCI_CPE_ENABLE_STDOUT_LOG			"icwmp.cpe.log_to_console"
#define UCI_CPE_ENABLE_FILE_LOG				"icwmp.cpe.log_to_file"
#define DM_SOFTWARE_VERSION_PATH			"InternetGatewayDevice.DeviceInfo.SoftwareVersion"


enum end_session {
	END_SESSION_REBOOT = 1,
	END_SESSION_EXTERNAL_ACTION = 1<<1,
	END_SESSION_RELOAD = 1<<2,
	END_SESSION_FACTORY_RESET = 1<<3,
	END_SESSION_IPPING_DIAGNOSTIC = 1<<4,
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
    struct list_head                    head_dm_parameter;
    int                                 id;
} event_container;

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
	char								*acs_ssl_capath;
    char 								*acs_ssl_version;
    char                                *cpe_userid;
    char                                *cpe_passwd;
    char                                *dhcp_url_path;
    char								*ip;
    char								*interface;
    char                                *ubus_socket;
    int                                 connection_request_port;
    int                                 period;
    time_t                              time;
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

typedef struct session_status {
    time_t last_start_time;
    time_t last_end_time;
    int last_status;
    time_t next_periodic;
    time_t next_retry;
    unsigned int success_session;
    unsigned int failure_session;
} session_status;

enum enum_session_status {
    SESSION_WAITING,
    SESSION_RUNNING,
    SESSION_FAILURE,
    SESSION_SUCCESS
};

struct deviceid {
	char *manufacturer;
	char *oui;
	char *serialnumber;
	char *productclass;
	char *softwareversion;
};

typedef struct cwmp {
    struct env			env;
    struct config		conf;
    struct deviceid		deviceid;
    struct list_head	head_session_queue;
    pthread_mutex_t		mutex_session_queue;
    struct session		*session_send;
    pthread_mutex_t		mutex_session_send;
    pthread_cond_t		threshold_session_send;
    pthread_mutex_t		mutex_periodic;
    pthread_cond_t		threshold_periodic;
    pthread_mutex_t		mutex_handle_notify;
    pthread_cond_t		threshold_handle_notify;
    int					count_handle_notify;
    int					retry_count_session;
    struct list_head	*head_event_container;
    int					pid_file;
    time_t              start_time;
    struct session_status session_status;
    unsigned int cwmp_id;
    int cr_socket_desc;
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
    struct list_head	*list_set_value_fault;
} rpc;

#define ARRAYSIZEOF(a)  (sizeof(a) / sizeof((a)[0]))
#define FREE(x) do { free(x); x = NULL; } while (0)

extern struct cwmp	cwmp_main;
extern const struct EVENT_CONST_STRUCT	EVENT_CONST [__EVENT_IDX_MAX];
extern struct list_head list_value_change;
extern pthread_mutex_t mutex_value_change;

struct rpc *cwmp_add_session_rpc_cpe (struct session *session, int type);
struct session *cwmp_add_queue_session (struct cwmp *cwmp);
struct rpc *cwmp_add_session_rpc_acs (struct session *session, int type);
struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);
int event_remove_all_event_container(struct session *session, int rem_from);
void cwmp_save_event_container (struct cwmp *cwmp,struct event_container *event_container);
void *thread_event_periodic (void *v);
void cwmp_add_notification(void);
int netlink_init(void);
char * mix_get_time(void);
char * mix_get_time_of(time_t t_time);
void *thread_exit_program (void *v);
void connection_request_ip_value_change(struct cwmp *cwmp);
void connection_request_port_value_change(struct cwmp *cwmp, int port);
void add_dm_parameter_tolist(struct list_head *head, char *param_name, char *param_data, char *param_type);
void cwmp_set_end_session (unsigned int end_session_flag);
void *thread_handle_notify(void *v);

#endif /* _CWMP_H__ */
