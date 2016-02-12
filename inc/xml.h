/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 *
 */

#ifndef _FREECWMP_XML_H__
#define _FREECWMP_XML_H__

#include <microxml.h>
#include <libubox/list.h>

#define CWMP_MXML_TAB_SPACE			"  "
#define DOWNLOAD_PROTOCOL_HTTP		"http://"
#define DOWNLOAD_PROTOCOL_FTP		"ftp://"
#define	MAX_DOWNLOAD_QUEUE			10
#define	MAX_SCHEDULE_INFORM_QUEUE	10

struct cwmp_namespaces
{
	char *soap_env;
	char *soap_enc;
	char *xsd;
	char *xsi;
	char *cwmp;
} ns;

enum rpc_cpe_methods_idx {
	RPC_CPE_GET_RPC_METHODS = 1,
	RPC_CPE_SET_PARAMETER_VALUES,
	RPC_CPE_GET_PARAMETER_VALUES,
	RPC_CPE_GET_PARAMETER_NAMES,
	RPC_CPE_SET_PARAMETER_ATTRIBUTES,
	RPC_CPE_GET_PARAMETER_ATTRIBUTES,
	RPC_CPE_ADD_OBJECT,
	RPC_CPE_DELETE_OBJECT,
	RPC_CPE_REBOOT,
	RPC_CPE_DOWNLOAD,
	RPC_CPE_FACTORY_RESET,
	RPC_CPE_SCHEDULE_INFORM,
	RPC_CPE_FAULT,
	__RPC_CPE_MAX
};


enum rpc_acs_methods_idx {
	RPC_ACS_INFORM = 1,
	RPC_ACS_GET_RPC_METHODS,
	RPC_ACS_TRANSFER_COMPLETE,
	__RPC_ACS_MAX
};

enum fault_cpe_idx {
	FAULT_CPE_NO_FAULT,
	FAULT_CPE_METHOD_NOT_SUPPORTED,
	FAULT_CPE_REQUEST_DENIED,
	FAULT_CPE_INTERNAL_ERROR,
	FAULT_CPE_INVALID_ARGUMENTS,
	FAULT_CPE_RESOURCES_EXCEEDED,
	FAULT_CPE_INVALID_PARAMETER_NAME,
	FAULT_CPE_INVALID_PARAMETER_TYPE,
	FAULT_CPE_INVALID_PARAMETER_VALUE,
	FAULT_CPE_NON_WRITABLE_PARAMETER,
	FAULT_CPE_NOTIFICATION_REJECTED,
	FAULT_CPE_DOWNLOAD_FAILURE,
	FAULT_CPE_UPLOAD_FAILURE,
	FAULT_CPE_FILE_TRANSFER_AUTHENTICATION_FAILURE,
	FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL,
	FAULT_CPE_DOWNLOAD_FAIL_MULTICAST_GROUP,
	FAULT_CPE_DOWNLOAD_FAIL_CONTACT_SERVER,
	FAULT_CPE_DOWNLOAD_FAIL_ACCESS_FILE,
	FAULT_CPE_DOWNLOAD_FAIL_COMPLETE_DOWNLOAD,
	FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED,
	FAULT_CPE_DOWNLOAD_FAIL_FILE_AUTHENTICATION,
	__FAULT_CPE_MAX
};

enum {
	FAULT_CPE_TYPE_CLIENT,
	FAULT_CPE_TYPE_SERVER
};

struct rpc_cpe_method {
	const char *name;
	int (*handler)(struct session *session, struct rpc *rpc);
};

struct rpc_acs_method {
	const char *name;
	int (*prepare_message)(struct cwmp *cwmp, struct session *session, struct rpc *rpc);
	int (*parse_response)(struct cwmp *cwmp, struct session *session, struct rpc *rpc);
	int (*extra_clean)(struct session *session, struct rpc *rpc);
};

typedef struct FAULT_CPE
{
    char                                *CODE;
    int                                	ICODE;
    int                                 TYPE;
    char                                *DESCRIPTION;
} FAULT_CPE;

typedef struct schedule_inform {
    struct list_head                    list;
    time_t                              scheduled_time;
    char                                *commandKey;
} schedule_inform;

typedef struct download {
    struct list_head                    list;
    time_t                              scheduled_time;
    int									file_size;
	char 								*command_key;
	char 								*file_type;
	char 								*url;
	char 								*username;
	char 								*password;
} download;

typedef struct transfer_complete {
	int									fault_code;
	char 								*command_key;
	char 								*start_time;
	char 								*complete_time;
	char 								*old_software_version;
} transfer_complete;

#define MXML_DELETE(X)  do {if (X) { mxmlDelete(X); X = NULL; } } while(0)

extern struct list_head		list_schedule_inform;
extern struct list_head		list_download;
extern int					count_download_queue;
extern const struct rpc_cpe_method rpc_cpe_methods[__RPC_CPE_MAX];
extern const struct rpc_acs_method rpc_acs_methods[__RPC_ACS_MAX];


void xml_exit(void);

int cwmp_handle_rpc_cpe_get_rpc_methods(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_set_parameter_values(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_get_parameter_values(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_get_parameter_names(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_set_parameter_attributes(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_get_parameter_attributes(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_add_object(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_delete_object(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_reboot(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_download(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_factory_reset(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_schedule_inform(struct session *session, struct rpc *rpc);
int cwmp_handle_rpc_cpe_fault(struct session *session, struct rpc *rpc);

int cwmp_rpc_acs_prepare_message_inform(struct cwmp *cwmp, struct session *session, struct rpc *rpc);
int cwmp_rpc_acs_parse_response_inform(struct cwmp *cwmp, struct session *session, struct rpc *rpc);
int cwmp_rpc_acs_prepare_get_rpc_methods(struct cwmp *cwmp, struct session *session, struct rpc *rpc);
int cwmp_rpc_acs_prepare_transfer_complete(struct cwmp *cwmp, struct session *session, struct rpc *rpc);
int cwmp_rpc_acs_destroy_data_inform(struct session *session, struct rpc *rpc);
int cwmp_rpc_acs_destroy_data_transfer_complete(struct session *session, struct rpc *rpc);

int xml_handle_message(struct session *session);
int xml_prepare_msg_out(struct session *session);
int xml_prepare_lwnotification_message(char **msg_out);
int cwmp_create_fault_message(struct session *session, struct rpc *rpc_cpe, int fault_code);
int cwmp_get_fault_code (int fault_code);
int cwmp_scheduleInform_remove_all();
int cwmp_scheduledDownload_remove_all();
struct transfer_complete *cwmp_set_data_rpc_acs_transferComplete();
void *thread_cwmp_rpc_cpe_scheduleInform (void *v);
void *thread_cwmp_rpc_cpe_download (void *v);


const char *whitespace_cb(mxml_node_t *node, int where);

#endif

