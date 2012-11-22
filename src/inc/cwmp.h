/*
    cwmp.h

    cwmp service client in C

--------------------------------------------------------------------------------
cwmp service client
Copyright (C) 2011-2012, Inteno, Inc. All Rights Reserved.

Any distribution, dissemination, modification, conversion, integral or partial
reproduction, can not be made without the prior written permission of Inteno.
--------------------------------------------------------------------------------
Author contact information:

--------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "soapH.h"
#include "list.h"

#define DEFAULT_LOG_FILE_SIZE                               10240
#define DEFAULT_LOG_FILE_NAME                               "/var/log/cwmpd.log"
#define EMERG                                               0
#define ALERT                                               1
#define CRITIC                                              2
#define ERROR                                               3
#define WARNING                                             4
#define NOTICE                                              5
#define INFO                                                6
#define DEBUG                                               7

#define EVENT_RETRY_AFTER_TRANSMIT_FAIL                     0x1
#define EVENT_RETRY_AFTER_REBOOT                            0x2
#define EVENT_RETRY_AFTER_BOOTSTRAP                         0x4
#define EVENT_TYPE_SINGLE                                   0x0
#define EVENT_TYPE_MULTIPLE                                 0x1
#define EVENT_IDX_0BOOTSTRAP                                0
#define EVENT_IDX_1BOOT                                     1
#define EVENT_IDX_2PERIODIC                                 2
#define EVENT_IDX_3SCHEDULED                                3
#define EVENT_IDX_4VALUE_CHANGE                             4
#define EVENT_IDX_5KICKED                                   5
#define EVENT_IDX_6CONNECTION_REQUEST                       6
#define EVENT_IDX_7TRANSFER_COMPLETE                        7
#define EVENT_IDX_8DIAGNOSTICS_COMPLETE                     8
#define EVENT_IDX_9REQUEST_DOWNLOAD                         9
#define EVENT_IDX_10AUTONOMOUS_TRANSFER_COMPLETE            10
#define EVENT_IDX_M_Reboot                                  11
#define EVENT_IDX_M_ScheduleInform                          12
#define EVENT_IDX_M_Download                                13
#define EVENT_IDX_M_Upload                                  14
#define MAX_SIZE_RPC_METHODS_RESPONSE                       14
#define MAX_INT32                                           2147483646
#define MAX_INT_ID                                          MAX_INT32
#define MIN_INT_ID                                          836464
#define PERIOD_INFORM_MIN                                   100
#define PERIOD_INFORM_DEFAULT                               86400
#define CONNECTION_REQUEST_RESTRICT_PERIOD                  10
#define CONNECTION_REQUEST_RESTRICT_REQUEST                 5
#define RPC_ACS_INFORM_IDX                                  1
#define RPC_ACS_GETRPCMETHODS_IDX                           2
#define RPC_ACS_TRANSFERCOMPLETE_IDX                        3


#define FAULT_CPE_ARRAY_SIZE                                21
#define FAULT_CPE_NO_FAULT_IDX                              0
#define FAULT_CPE_METHOD_NOT_SUPPORTED_IDX                  1
#define FAULT_CPE_REQUEST_DENIED_IDX                        2
#define FAULT_CPE_INTERNAL_ERROR_IDX                        3
#define FAULT_CPE_INVALID_ARGUMENTS_IDX                     4
#define FAULT_CPE_RESOURCES_EXCEEDED_IDX                    5
#define FAULT_CPE_INVALID_PARAMETER_NAME_IDX                6
#define FAULT_CPE_INVALID_PARAMETER_TYPE_IDX                7
#define FAULT_CPE_INVALID_PARAMETER_VALUE_IDX               8
#define FAULT_CPE_NON_WRITABLE_PARAMETER_IDX                9
#define FAULT_CPE_NOTIFICATION_REJECTED_IDX                 10
#define FAULT_CPE_DOWNLOAD_FAILURE_IDX                      11
#define FAULT_CPE_UPLOAD_FAILURE_IDX                        12
#define FAULT_CPE_FILE_TRANSFER_AUTHENTICATION_FAILURE_IDX  13
#define FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL_IDX    14
#define FAULT_CPE_DOWNLOAD_FAIL_MULTICAST_GROUP_IDX         15
#define FAULT_CPE_DOWNLOAD_FAIL_CONTACT_SERVER_IDX          16
#define FAULT_CPE_DOWNLOAD_FAIL_ACCESS_FILE_IDX             17
#define FAULT_CPE_DOWNLOAD_FAIL_COMPLETE_DOWNLOAD_IDX       18
#define FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED_IDX          19
#define FAULT_CPE_DOWNLOAD_FAIL_FILE_AUTHENTICATION_IDX     20

#define CWMP_START_BOOT                                     1
#define CWMP_START_PERIODIC                                 2
#define CWMP_START_ICCU										3
#define CWMP_OK                                             0 /* No Error                   */
#define CWMP_GEN_ERR                                        1 /* General Error              */
#define CWMP_MEM_ERR                                        2 /* Memory Error               */
#define CWMP_OWF_ERR                                        3 /* Open Writing File Error    */
#define CWMP_ORF_ERR                                        4 /* Open Reading File Error    */
#define CWMP_MUTEX_ERR                                      5
#define CWMP_FAIL_RPC                                       6
#define CWMP_RETRY_RPC                                      7
#define CWMP_SUCCESS_RPC                                    8
#define CWMP_UNAUTHORIZED_401                               9
#define CWMP_GET_RETRY_8005                                 10
#define CWMP_RETRY_SESSION                                  11
#define CWMP_CONTINUE_SESSION                               12
#define CWMP_SUCCESS_SESSION                                13
#define CWMP_FAULT_CPE                                      14
#define CWMP_EXACT_FOUND                                    15
#define CWMP_CHILD_FOUND                                    16
#define CWMP_PARENT_FOUND                                   17
#define MAX_EVENTS                                          64
#define SOAP_TIMEOUT                                        30

#define DEFAULT_ACSURL                                      "http://192.168.1.1:8080/openacs/acs"
#define DEFAULT_CONNECTION_REQUEST_PORT                     7547
#define COUNT_RPC_CPE                                       17
#define COUNT_EVENT                                         15
#define COUNT_ACCESSLIST                                    1
#define DOWNLOADED_CONFIG_FILE                              "/tmp/configuration.cfg"
#define DOWNLOADED_FIRMWARE_FILE                            "/tmp/firmware.img"
#define DOWNLOADED_LAST_VALID_FIRMWARE_FILE                 "/tmp/valid_firmware.img"
#define DOWNLOADED_WEBCONTENT_FILE                          "/tmp/webcontent.ipk"
#define UCI_DM_XML_FILE_LIST                                "cwmp.dm.xml"
#define UCI_DHCP_DISCOVERY_PATH                             "cwmp.acs.dhcp_discovery"
#define UCI_DHCP_ACS_URL_PATH                               "provisioning.iup.tr069url"
#define UCI_STATE_CONNECTION_REQUEST_URL_PATH               "cwmp.acs.cr_url"
#define UCI_ACS_URL_PATH                                    "cwmp.acs.url"
#define UCI_PERIODIC_INFORM_INTERVAL_PATH                   "cwmp.acs.periodic_inform_interval"
#define UCI_PERIODIC_INFORM_ENABLE_PATH                     "cwmp.acs.periodic_inform_enable"
#define UCI_ACS_USERID_PATH                                 "cwmp.acs.userid"
#define UCI_ACS_PASSWD_PATH                                 "cwmp.acs.passwd"
#define UCI_ACS_PARAMETERKEY_PATH                           "cwmp.acs.ParameterKey"
#define UCI_LOG_SEVERITY_PATH                               "cwmp.cpe.log_severity"
#define UCI_CPE_USERID_PATH                                 "cwmp.cpe.userid"
#define UCI_CPE_PASSWD_PATH                                 "cwmp.cpe.passwd"
#define UCI_CPE_PORT_PATH                                   "cwmp.cpe.port"
#define UCI_NOTIFICATION_PASSIVE_PATH                       "cwmp.notification.passive"
#define UCI_NOTIFICATION_ACTIVE_PATH                        "cwmp.notification.active"
#define UCI_NOTIFICATION_DENIED_PATH                        "cwmp.notification.deny"
#define UCI_CPE_LOG_FILE_NAME                               "cwmp.cpe.log_file_name"
#define UCI_CPE_LOG_MAX_SIZE                                "cwmp.cpe.log_max_size"
#define UCI_CPE_ENABLE_STDOUT_LOG                           "cwmp.cpe.log_to_console"
#define UCI_CPE_ENABLE_FILE_LOG                             "cwmp.cpe.log_to_file"

#define UCI_TRACK_CONF                                      "ucitrack"
#define UCI_TRACK_INIT                                      "init"
#define UCI_TRACK_AFFECTS                                   "affects"
#define UCI_TRACK_CONF_CWMP                                 "cwmp"

#define FIRMWARE_FILE_TYPE                                  "1 Firmware Upgrade Image"
#define WEB_CONTENT_FILE_TYPE                               "2 Web Content"
#define CONFIGURATION_FILE_TYPE                             "3 Vendor Configuration File"
#define DOWNLOAD_PROTOCOL_HTTP                              "http://"
#define DOWNLOAD_PROTOCOL_FTP                               "ftp://"
#define	MAX_DOWNLOAD_QUEUE									10
#define ENABLE_CHECK_SIZE				            		0x0
#define DISABLE_CHECK_SIZE		                    		0x1

#define ENCODING_STYLE_URL                                  "http://schemas.xmlsoap.org/soap/encoding/"

typedef enum bool {
    FALSE,
    TRUE
} bool;


typedef struct config {
    char                                *acsurl;
    char                                *confFile;
    char                                *acs_userid;
    char                                *acs_passwd;
    char                                *cpe_userid;
    char                                *cpe_passwd;
    int                                 connection_request_port;
    int                                 period;
    bool                                periodic_enable;
} config;

typedef struct env {
    unsigned short                      boot;
    unsigned short                      periodic;
    unsigned short                      iccu;
    long int							max_firmware_size;
} env;

typedef struct  api_value_change {
    struct list_head                    parameter_list;
    int                                 parameter_size;
    pthread_mutex_t                     mutex;
} api_value_change;

typedef struct cwmp {
    struct env                          env;
    struct config                       conf;
    struct list_head                    head_session_queue;
    pthread_mutex_t                     mutex_session_queue;
    struct session                      *session_send;
    pthread_mutex_t                     mutex_session_send;
    pthread_cond_t                      threshold_session_send;
    pthread_mutex_t                     mutex_periodic;
    pthread_cond_t                      threshold_periodic;
    int                                 retry_count_session;
    struct list_head                    *head_event_container;
    struct  api_value_change            api_value_change;
    int                                 error;
} cwmp;

typedef struct session {
    struct list_head                    list;
    char                                acs_url[256];
    struct list_head                    head_event_container;
    int                                 event_size;
    int                                 parameter_size;
    unsigned int                        single_event_flag;
    struct list_head                    head_rpc_cpe;
    struct list_head                    head_rpc_acs;
    struct list_head                    head_session_end_func;
    struct soap                         soap;
    bool                                hold_request;
    bool                                digest_auth;
    int                                 error;
} session;

typedef struct session_end_func {
    struct list_head                    list;
    int                                 (*func)(struct cwmp *cwmp, void *input);
    void                                *input;
} session_end_func;


typedef struct event_container {
    struct list_head                    list;
    struct cwmp1__EventStruct           event;
    struct list_head                    head_paramater_container;
    int                                 id;
    int                                 idx;
} event_container;

typedef struct paramater_container {
    struct list_head                    list;
    struct cwmp1__ParameterValueStruct  paramater;
} paramater_container;

typedef struct soap_cwmp1_methods__rpc
{
    void                                (*soap_serialize_cwmp1__send_data)(struct soap *soap, void *data);
    int                                 (*soap_put_cwmp1__send_data)(struct soap *soap, void *data, char *envelope, char *action);
    void                                *(*soap_get_cwmp1__rpc_received_data)(struct soap *, void *data, char *envelope_response, char *action);
    char                                *envelope;
    char                                *envelope_response;
} soap_cwmp1_methods__rpc;
typedef struct fault
{
    int 								code_idx;
    char 								*parameter_cause;
} fault;
typedef struct rpc_acs {
    struct list_head                    list;
    void                                *method_data;
    int                                 (*method_data_init)(struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
    int                                 (*method_remote_call)(struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
    void                                *method_response_data;
    int                                 (*method_response)(struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
    int                                 (*method_end)(struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
    int                                 (*destructor)(struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
    struct soap_cwmp1_methods__rpc      soap_methods;
    struct fault                        fault;
    int                                 type;
    int                                 error;
} rpc_acs;

typedef struct rpc_cpe {
    struct list_head                    list;
    void                                *method_data;
    int                                 (*method)(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
    void                                *method_response_data;
    int                                 (*method_response_data_init)(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
    int                                 (*method_response)(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
    int                                 (*method_end)(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
    int                                 (*destructor)(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
    struct soap_cwmp1_methods__rpc      soap_methods;
    struct fault                        fault;
    int                                 error;
} rpc_cpe;

typedef struct CPE_METHOD_CONSTRUCTORS
{
    char                                *METHOD;
    struct rpc_cpe                      *(*CONSTRUCTOR)(struct session *session);
} CPE_METHOD_CONSTRUCTORS;

typedef struct FAULT_CPE
{
    char                                *CODE;
    int                                 TYPE;
    char                                *DESCRIPTION;
} FAULT_CPE;

typedef struct EVENT_CONST_STRUCT
{
    char                                *CODE;
    unsigned int                        TYPE;
    unsigned short                      RETRY;

} EVENT_CONST_STRUCT;

typedef struct schedule_inform {
    struct list_head                    list;
    time_t                              scheduled_time;
    char                                *commandKey;
} schedule_inform;

typedef struct download {
    struct list_head                    list;
    time_t                              scheduled_time;
	char 								*CommandKey;
	char 								*FileType;
	char 								*URL;
	char 								*Username;
	char 								*Password;
} download;

struct download_end_func {
    int                                 (*func)(struct cwmp *cwmp, void *input);
    void                                *input;
};

#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))
typedef struct config_uci_list {
    struct list_head                    list;
    char                                *value;
} config_uci_list;

typedef struct ACCESSLIST_CONST_STRUCT
{
    char                                *NAME;
    char                                *UCI_ACCESSLIST_PATH;
} ACCESSLIST_CONST_STRUCT;

#ifdef WITH_CWMP_DEBUG
# ifndef CWMP_LOG
#  define CWMP_LOG(SEV,MESSAGE,args...) puts_log(SEV,MESSAGE,##args);
# endif
#else
# define CWMP_LOG(SEV,MESSAGE,args...)
#endif
