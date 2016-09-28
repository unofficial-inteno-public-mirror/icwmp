/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 *	Copyright (C) 2012 Jonas Gorski <jonas.gorski@gmail.com>
 */

#include <stdbool.h>
#include <stdint.h>
#include <microxml.h>
#include <time.h>
#include "cwmp.h"
#include "xml.h"
#include "external.h"
#include "messages.h"
#include "backupSession.h"
#include "log.h"
#include "jshn.h"
#include "dmentry.h"
#include "deviceinfo.h"
#include "dmcwmp.h"
#include "softwaremodules.h"

LIST_HEAD(list_download);
LIST_HEAD(list_upload);
LIST_HEAD(list_schedule_download);
LIST_HEAD(list_apply_schedule_download);
LIST_HEAD(list_change_du_state);
static pthread_mutex_t		mutex_download = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		threshold_download;
static pthread_mutex_t		mutex_change_du_state = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		threshold_change_du_state;
static pthread_mutex_t		mutex_schedule_download = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		threshold_schedule_download;
static pthread_mutex_t		mutex_apply_schedule_download = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		threshold_apply_schedule_download;
static pthread_mutex_t		mutex_upload = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		threshold_upload;
int							count_download_queue = 0;
int							count_schedule_inform_queue = 0;

LIST_HEAD(list_schedule_inform);
static pthread_mutex_t      mutex_schedule_inform = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       threshold_schedule_inform;

const static char *soap_env_url = "http://schemas.xmlsoap.org/soap/envelope/";
const static char *soap_enc_url = "http://schemas.xmlsoap.org/soap/encoding/";
const static char *xsd_url = "http://www.w3.org/2001/XMLSchema";
const static char *xsi_url = "http://www.w3.org/2001/XMLSchema-instance";

typedef struct DEVICE_ID_STRUCT {
	char *device_id_name;
	char *parameter_name;
} DEVICE_ID_STRUCT;

const static char *cwmp_urls[] = {
		"urn:dslforum-org:cwmp-1-0",
		"urn:dslforum-org:cwmp-1-1",
		"urn:dslforum-org:cwmp-1-2",
		"urn:dslforum-org:cwmp-1-3",
		"urn:dslforum-org:cwmp-1-4",
		NULL };

struct FAULT_CPE FAULT_CPE_ARRAY [] = {
    [FAULT_CPE_METHOD_NOT_SUPPORTED]                = {"9000", FAULT_9000, FAULT_CPE_TYPE_SERVER, "Method not supported"},
    [FAULT_CPE_REQUEST_DENIED]                      = {"9001", FAULT_9001, FAULT_CPE_TYPE_SERVER, "Request denied (no reason specified)"},
    [FAULT_CPE_INTERNAL_ERROR]                      = {"9002", FAULT_9002, FAULT_CPE_TYPE_SERVER, "Internal error"},
    [FAULT_CPE_INVALID_ARGUMENTS]                   = {"9003", FAULT_9003, FAULT_CPE_TYPE_CLIENT, "Invalid arguments"},
    [FAULT_CPE_RESOURCES_EXCEEDED]                  = {"9004", FAULT_9004, FAULT_CPE_TYPE_SERVER, "Resources exceeded"},
    [FAULT_CPE_INVALID_PARAMETER_NAME]              = {"9005", FAULT_9005, FAULT_CPE_TYPE_CLIENT, "Invalid parameter name"},
    [FAULT_CPE_INVALID_PARAMETER_TYPE]              = {"9006", FAULT_9006, FAULT_CPE_TYPE_CLIENT, "Invalid parameter type"},
    [FAULT_CPE_INVALID_PARAMETER_VALUE]             = {"9007", FAULT_9007, FAULT_CPE_TYPE_CLIENT, "Invalid parameter value"},
    [FAULT_CPE_NON_WRITABLE_PARAMETER]              = {"9008", FAULT_9008, FAULT_CPE_TYPE_CLIENT, "Attempt to set a non-writable parameter"},
    [FAULT_CPE_NOTIFICATION_REJECTED]               = {"9009", FAULT_9009, FAULT_CPE_TYPE_SERVER, "Notification request rejected"},
    [FAULT_CPE_DOWNLOAD_FAILURE]                    = {"9010", FAULT_9010, FAULT_CPE_TYPE_SERVER, "Download failure"},
    [FAULT_CPE_UPLOAD_FAILURE]                      = {"9011", FAULT_9011, FAULT_CPE_TYPE_SERVER, "Upload failure"},
    [FAULT_CPE_FILE_TRANSFER_AUTHENTICATION_FAILURE]= {"9012", FAULT_9012, FAULT_CPE_TYPE_SERVER, "File transfer server authentication failure"},
    [FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL]  = {"9013", FAULT_9013, FAULT_CPE_TYPE_SERVER, "Unsupported protocol for file transfer"},
    [FAULT_CPE_DOWNLOAD_FAIL_MULTICAST_GROUP]       = {"9014", FAULT_9014, FAULT_CPE_TYPE_SERVER, "Download failure: unable to join multicast group"},
    [FAULT_CPE_DOWNLOAD_FAIL_CONTACT_SERVER]        = {"9015", FAULT_9015, FAULT_CPE_TYPE_SERVER, "Download failure: unable to contact file server"},
    [FAULT_CPE_DOWNLOAD_FAIL_ACCESS_FILE]           = {"9016", FAULT_9016, FAULT_CPE_TYPE_SERVER, "Download failure: unable to access file"},
    [FAULT_CPE_DOWNLOAD_FAIL_COMPLETE_DOWNLOAD]     = {"9017", FAULT_9017, FAULT_CPE_TYPE_SERVER, "Download failure: unable to complete download"},
    [FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED]        = {"9018", FAULT_9018, FAULT_CPE_TYPE_SERVER, "Download failure: file corrupted"},
    [FAULT_CPE_DOWNLOAD_FAIL_FILE_AUTHENTICATION]   = {"9019", FAULT_9019, FAULT_CPE_TYPE_SERVER, "Download failure: file authentication failure"},
	[FAULT_CPE_DOWNLOAD_FAIL_WITHIN_TIME_WINDOW]   = {"9020", FAULT_9020, FAULT_CPE_TYPE_SERVER, "Download failure: unable to complete download"},
	[FAULT_CPE_DUPLICATE_DEPLOYMENT_UNIT]   		= {"9026", FAULT_9026, FAULT_CPE_TYPE_SERVER, "Duplicate deployment unit"},
	[FAULT_CPE_SYSTEM_RESOURCES_EXCEEDED]   		= {"9027", FAULT_9027, FAULT_CPE_TYPE_SERVER, "System ressources exceeded"},
	[FAULT_CPE_UNKNOWN_DEPLOYMENT_UNIT]   			= {"9028", FAULT_9028, FAULT_CPE_TYPE_SERVER, "Unknown deployment unit"},
	[FAULT_CPE_INVALID_DEPLOYMENT_UNIT_STATE]   	= {"9029", FAULT_9029, FAULT_CPE_TYPE_SERVER, "Invalid deployment unit state"},
	[FAULT_CPE_INVALID_DOWNGRADE_REJECTED]   		= {"9030", FAULT_9030, FAULT_CPE_TYPE_SERVER, "Invalid deployment unit Update: Downgrade not permitted"},
	[FAULT_CPE_INVALID_UPDATE_VERSION_UNSPECIFIED]  = {"9031", FAULT_9031, FAULT_CPE_TYPE_SERVER, "Invalid deployment unit Update: Version not specified"},
	[FAULT_CPE_INVALID_UPDATE_VERSION_EXIST]  		= {"9031", FAULT_9032, FAULT_CPE_TYPE_SERVER, "Invalid deployment unit Update: Version already exist"}
};

const struct rpc_cpe_method rpc_cpe_methods[] = {
	[RPC_CPE_GET_RPC_METHODS] 				= {"GetRPCMethods", cwmp_handle_rpc_cpe_get_rpc_methods, AMD_1},
	[RPC_CPE_SET_PARAMETER_VALUES] 			= {"SetParameterValues", cwmp_handle_rpc_cpe_set_parameter_values, AMD_1},
	[RPC_CPE_GET_PARAMETER_VALUES] 			= {"GetParameterValues", cwmp_handle_rpc_cpe_get_parameter_values, AMD_1},
	[RPC_CPE_GET_PARAMETER_NAMES] 			= {"GetParameterNames", cwmp_handle_rpc_cpe_get_parameter_names, AMD_1},
	[RPC_CPE_SET_PARAMETER_ATTRIBUTES] 		= {"SetParameterAttributes", cwmp_handle_rpc_cpe_set_parameter_attributes, AMD_1},
	[RPC_CPE_GET_PARAMETER_ATTRIBUTES] 		= {"GetParameterAttributes", cwmp_handle_rpc_cpe_get_parameter_attributes, AMD_1},
	[RPC_CPE_ADD_OBJECT] 					= {"AddObject", cwmp_handle_rpc_cpe_add_object, AMD_1},
	[RPC_CPE_DELETE_OBJECT] 				= {"DeleteObject", cwmp_handle_rpc_cpe_delete_object, AMD_1},
	[RPC_CPE_REBOOT] 						= {"Reboot", cwmp_handle_rpc_cpe_reboot, AMD_1},
	[RPC_CPE_DOWNLOAD] 						= {"Download", cwmp_handle_rpc_cpe_download, AMD_1},
	[RPC_CPE_UPLOAD] 						= {"Upload", cwmp_handle_rpc_cpe_upload, AMD_1},
	[RPC_CPE_FACTORY_RESET] 				= {"FactoryReset", cwmp_handle_rpc_cpe_factory_reset, AMD_1},
	[RPC_CPE_CANCEL_TRANSFER] 				= {"CancelTransfer", cwmp_handle_rpc_cpe_cancel_transfer, AMD_3},	
	[RPC_CPE_SCHEDULE_INFORM] 				= {"ScheduleInform", cwmp_handle_rpc_cpe_schedule_inform, AMD_1},
	[RPC_CPE_SCHEDULE_DOWNLOAD] 			= {"ScheduleDownload", cwmp_handle_rpc_cpe_schedule_download, AMD_3},
	[RPC_CPE_CHANGE_DU_STATE] 				= {"ChangeDUState", cwmp_handle_rpc_cpe_change_du_state, AMD_3},
	[RPC_CPE_FAULT] 						= {"Fault", cwmp_handle_rpc_cpe_fault, AMD_1}
};

const struct rpc_acs_method rpc_acs_methods[] = {
	[RPC_ACS_INFORM] 			= {"Inform", cwmp_rpc_acs_prepare_message_inform, cwmp_rpc_acs_parse_response_inform, cwmp_rpc_acs_destroy_data_inform},
	[RPC_ACS_GET_RPC_METHODS] 	= {"GetRPCMethods", cwmp_rpc_acs_prepare_get_rpc_methods, NULL, NULL},
	[RPC_ACS_TRANSFER_COMPLETE] = {"TransferComplete", cwmp_rpc_acs_prepare_transfer_complete,	NULL, cwmp_rpc_acs_destroy_data_transfer_complete},
	[RPC_ACS_DU_STATE_CHANGE_COMPLETE] = {"DUStateChangeComplete", cwmp_rpc_acs_prepare_du_state_change_complete,	NULL, cwmp_rpc_acs_destroy_data_du_state_change_complete}

};

static int xml_recreate_namespace(mxml_node_t *tree)
{
	const char *cwmp_urn;
	char *c;
	int i;
	mxml_node_t *b = tree;

	do
	{
		FREE(ns.soap_env);
		FREE(ns.soap_enc);
		FREE(ns.xsd);
		FREE(ns.xsi);
		FREE(ns.cwmp);

		c = (char *) mxmlElementGetAttrName(b, soap_env_url);
		if (c && *(c + 5) == ':') {
			ns.soap_env = strdup((c + 6));
		} else {
			continue;
		}

		c = (char *) mxmlElementGetAttrName(b, soap_enc_url);
		if (c && *(c + 5) == ':') {
			ns.soap_enc = strdup((c + 6));
		} else {
			continue;
		}

		c = (char *) mxmlElementGetAttrName(b, xsd_url);
		if (c && *(c + 5) == ':') {
			ns.xsd = strdup((c + 6));
		} else {
			continue;
		}

		c = (char *) mxmlElementGetAttrName(b, xsi_url);
		if (c && *(c + 5) == ':') {
			ns.xsi = strdup((c + 6));
		} else {
			continue;
		}

		for (i = 0; cwmp_urls[i] != NULL; i++) {
			cwmp_urn = cwmp_urls[i];
			c = (char *) mxmlElementGetAttrName(b, cwmp_urn);
			if (c && *(c + 5) == ':') {
				ns.cwmp = strdup((c + 6));
				break;
			}
		}

		if (!ns.cwmp) continue;

		return 0;
	} while(b = mxmlWalkNext(b, tree, MXML_DESCEND));

	return -1;
}

void xml_exit(void)
{
	FREE(ns.soap_env);
	FREE(ns.soap_enc);
	FREE(ns.xsd);
	FREE(ns.xsi);
	FREE(ns.cwmp);
}

int xml_send_message(struct cwmp *cwmp, struct session *session, struct rpc *rpc)
{
	char *s, *c, *msg_out = NULL, *msg_in = NULL;
	int msg_out_len = 0, f, r = 0;
	mxml_node_t	*b;

	if (session->tree_out) {

		unsigned char *zmsg_out;
		msg_out = mxmlSaveAllocString(session->tree_out, whitespace_cb);
		CWMP_LOG(DEBUG,"Message OUT \n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n%s\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>",msg_out);
		if (cwmp->conf.compression != COMP_NONE) {
		    if (zlib_compress(msg_out, &zmsg_out, &msg_out_len, cwmp->conf.compression)) {
		        return -1;
		    }
            FREE(msg_out);
            msg_out = (char *) zmsg_out;
		} else {
		    msg_out_len = strlen(msg_out);
		}
	}
	while (1) {
		f = 0;
		if (http_send_message(cwmp, msg_out, msg_out_len,&msg_in)) {
			goto error;
		}
		if (msg_in) {
			CWMP_LOG(DEBUG,"Message IN \n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n%s\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<",msg_in);
			if (s = strstr(msg_in, "<FaultCode>"))
				sscanf(s, "<FaultCode>%d</FaultCode>",&f);
			if (f) {
				if (f == 8005) {
					r++;
					if (r<5) {
						FREE(msg_in);
						continue;
					}
					goto error;
				} else if (rpc && rpc->type != RPC_ACS_INFORM) {
					break;
				} else {
					goto error;
				}
			}
			else {
				break;
			}
		} else {
			goto end;
		}
	}

	session->tree_in = mxmlLoadString(NULL, msg_in, MXML_NO_CALLBACK);
	if (!session->tree_in) goto error;

	xml_recreate_namespace(session->tree_in);

	/* get NoMoreRequests or HolRequest*/
	session->hold_request = false;
	if (asprintf(&c, "%s:%s", ns.cwmp, "NoMoreRequests") == -1)
		goto error;
	b = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);
	if (b) {
		b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
		if (b && b->type == MXML_TEXT  && b->value.text.string)
			session->hold_request = atoi(b->value.text.string);
	} else {
		if (asprintf(&c, "%s:%s", ns.cwmp, "HoldRequests") == -1)
			goto error;

		b = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
		FREE(c);
		if (b) {
			b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
			if (b && b->type == MXML_TEXT && b->value.text.string)
				session->hold_request = atoi(b->value.text.string);
		}
	}

end:
	FREE(msg_out);
	FREE(msg_in);
	return 0;

error:
	FREE(msg_out);
	FREE(msg_in);
	return -1;
}

int xml_prepare_msg_out(struct session *session)
{
	struct cwmp   *cwmp = &cwmp_main;
	struct config   *conf;
	conf = &(cwmp->conf);
	mxml_node_t *n;
#ifdef DUMMY_MODE
	FILE *fp;
	fp = fopen("./ext/soap_msg_templates/cwmp_response_message.xml", "r");
	session->tree_out = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
#else
	session->tree_out = mxmlLoadString(NULL, CWMP_RESPONSE_MESSAGE, MXML_NO_CALLBACK);
	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Envelope", NULL, NULL, MXML_DESCEND);
	if(!n) { return -1;}
	mxmlElementSetAttr(n, "xmlns:cwmp", cwmp_urls[(conf->amd_version)-1]);
#endif
	if (!session->tree_out) return -1;

	return 0;
}

int xml_set_cwmp_id(struct session *session)
{
    char        *c;
    mxml_node_t *b;

    /* define cwmp id */
    if (asprintf(&c, "%u", ++(cwmp_main.cwmp_id)) == -1)
        return -1;

    b = mxmlFindElement(session->tree_out, session->tree_out, "cwmp:ID", NULL, NULL, MXML_DESCEND);
    if (!b) return -1;

    b = mxmlNewText(b, 0, c);
    FREE(c);
    if (!b) return -1;

    return 0;
}

int xml_set_cwmp_id_rpc_cpe(struct session *session)
{
	char		*c;
	mxml_node_t	*b;

	/* handle cwmp:ID */
	if (asprintf(&c, "%s:%s", ns.cwmp, "ID") == -1)
		return -1;

	b = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);


	if (b) {
	    /* ACS send ID parameter */
		b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
		if (!b || b->type != MXML_TEXT || !b->value.text.string) return 0;
		c = strdup(b->value.text.string);

		b = mxmlFindElement(session->tree_out, session->tree_out, "cwmp:ID", NULL, NULL, MXML_DESCEND);
		if (!b) return -1;

		b = mxmlNewText(b, 0, c);
		FREE(c);
		if (!b) return -1;
	} else {
	    /* ACS does not send ID parameter */
	    int r = xml_set_cwmp_id(session);
	    return r;
	}
	return 0;
}

int xml_handle_message(struct session *session)
{
	struct rpc	*rpc_cpe;
	char		*c;
	int 		i;
	mxml_node_t	*b;
	struct cwmp   *cwmp = &cwmp_main;
	struct config   *conf;
	conf = &(cwmp->conf);

	/* get method */

	if (asprintf(&c, "%s:%s", ns.soap_env, "Body") == -1) {
		CWMP_LOG (INFO,"Internal error");
		session->fault_code = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}

	b = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if(!b) {
		CWMP_LOG (INFO,"Invalid received message");
		session->fault_code = FAULT_CPE_REQUEST_DENIED;
		goto fault;
	}
	session->body_in = b;

	while (1) {
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND_FIRST);
		if (!b) goto error;
		if (b->type == MXML_ELEMENT) break;
	}

	c = b->value.element.name;
	/* convert QName to localPart, check that ns is the expected one */
	if (strchr(c, ':')) {
		char *tmp = strchr(c, ':');
		size_t ns_len = tmp - c;

		if (strlen(ns.cwmp) != ns_len) {
			CWMP_LOG (INFO,"Invalid received message");
			session->fault_code = FAULT_CPE_REQUEST_DENIED;
			goto fault;
		}

		if (strncmp(ns.cwmp, c, ns_len)){
			CWMP_LOG (INFO,"Invalid received message");
			session->fault_code = FAULT_CPE_REQUEST_DENIED;
			goto fault;
		}

		c = tmp + 1;
	} else {
		CWMP_LOG (INFO,"Invalid received message");
		session->fault_code = FAULT_CPE_REQUEST_DENIED;
		goto fault;
	}
	CWMP_LOG (INFO,"SOAP RPC message: %s", c);
	rpc_cpe = NULL;
	for (i = 1; i < __RPC_CPE_MAX; i++) {
		if (i!= RPC_CPE_FAULT && strcmp(c, rpc_cpe_methods[i].name) == 0 && rpc_cpe_methods[i].amd <= conf->amd_version) {
			CWMP_LOG (INFO,"%s RPC is supported",c);
			rpc_cpe = cwmp_add_session_rpc_cpe(session, i);
			if (rpc_cpe == NULL) goto error;
			break;
		}
	}
	if (!rpc_cpe) {
		CWMP_LOG (INFO,"%s RPC is not supported",c);
		session->fault_code = FAULT_CPE_METHOD_NOT_SUPPORTED;
		goto fault;
	}
	return 0;
fault:
	rpc_cpe = cwmp_add_session_rpc_cpe(session, RPC_CPE_FAULT);
	if (rpc_cpe == NULL) goto error;
	return 0;
error:
	return -1;
}

const char *whitespace_cb(mxml_node_t *node, int where)
{
    static char tab_space[10 * sizeof(CWMP_MXML_TAB_SPACE) + 1];

    if (node->type != MXML_ELEMENT)
        return NULL;

    switch (where) {
        case MXML_WS_BEFORE_CLOSE:
            if (node->child && node->child->type != MXML_ELEMENT)
                return NULL;
        case MXML_WS_BEFORE_OPEN:
            tab_space[0] = '\0';
            while (node = node->parent)
                strcat(tab_space, CWMP_MXML_TAB_SPACE);
            return tab_space;
        case MXML_WS_AFTER_OPEN:
            return ((!node->child || node->child->type == MXML_ELEMENT) ? "\n" : NULL);
        case MXML_WS_AFTER_CLOSE:
            return "\n";
        default:
            return NULL;
    }
}


/*
 * [RPC ACS]: Inform
 */

static int xml_prepare_events_inform(struct session *session, mxml_node_t *tree)
{
	mxml_node_t *node, *b1, *b2;
	char *c=NULL;
	int n = 0;
	struct list_head *ilist;
	struct event_container *event_container;

	b1 = mxmlFindElement(tree, tree, "Event", NULL, NULL, MXML_DESCEND);
	if (!b1) return -1;

	list_for_each(ilist,&(session->head_event_container))
	{
		event_container = list_entry(ilist, struct event_container, list);
		node = mxmlNewElement (b1, "EventStruct");
		if (!node) goto error;
		b2 = mxmlNewElement (node, "EventCode");
		if (!b2) goto error;
		b2 = mxmlNewText(b2, 0, EVENT_CONST[event_container->code].CODE);
		if (!b2) goto error;
		b2 = mxmlNewElement (node, "CommandKey");
		if (!b2) goto error;
		if (event_container->command_key) {
			b2 = mxmlNewText(b2, 0, event_container->command_key);
			if (!b2) goto error;
		}
		mxmlAdd(b1, MXML_ADD_AFTER, MXML_ADD_TO_PARENT, node);
		n++;
	}
	if (n) {
		if (asprintf(&c, "cwmp:EventStruct[%u]", n) == -1)
			return -1;
		mxmlElementSetAttr(b1, "xsi:type", "soap_enc:Array");
		mxmlElementSetAttr(b1, "soap_enc:arrayType", c);
		free(c);
	}
	return 0;

error:
	return -1;
}

static int xml_prepare_parameters_inform(struct dmctx *dmctx, struct dm_parameter *dm_parameter, mxml_node_t *parameter_list, int *size)
{
	mxml_node_t *node, *b;
	int found;

	b = mxmlFindElementText(parameter_list, parameter_list, dm_parameter->name, MXML_DESCEND);
	if(b && dm_parameter->data != NULL)
	{
		node = b->parent->parent;
		b = mxmlFindElement(node, node, "Value", NULL, NULL, MXML_DESCEND_FIRST);
		if(!b) return 0;
		if (b->child && strcmp(dm_parameter->data, b->child->value.text.string)==0)
			return 0;
		mxmlDelete(b);
		(*size)--;

		goto create_value;
	}
	else if (b && dm_parameter->data == NULL)
	{
		return 0;
	}
	else if (!b && dm_parameter->data == NULL)
	{
		dm_entry_param_method(dmctx, CMD_GET_VALUE, dm_parameter->name, NULL, NULL);
		return 0;
	}
	node = mxmlNewElement (parameter_list, "ParameterValueStruct");
	if (!node) return -1;

	b = mxmlNewElement(node, "Name");
	if (!b) return -1;

	b = mxmlNewText(b, 0, dm_parameter->name);
	if (!b) return -1;

create_value:
	b = mxmlNewElement(node, "Value");
	if (!b) return -1;

#ifdef ACS_MULTI
	mxmlElementSetAttr(b, "xsi:type", (dm_parameter->type && dm_parameter->type[0] != '\0')? dm_parameter->type : "xsd:string");
#endif
	b = mxmlNewText(b, 0, dm_parameter->data);
	if (!b) return -1;

	(*size)++;
	return 0;
}

static int xml_prepare_lwnotifications(mxml_node_t *parameter_list)
{
	mxml_node_t *b, *n;

	struct list_head *p;
	struct dm_parameter *lw_notification;
	list_for_each(p, &list_lw_value_change) {
		lw_notification = list_entry(p, struct dm_parameter, list);

		n = mxmlNewElement(parameter_list, "Param");
		if (!n) goto error;

		b = mxmlNewElement(n, "Name");
		if (!b) goto error;

		b = mxmlNewText(b, 0, lw_notification->name);
		if (!b) goto error;
		
		b = mxmlNewElement(n, "Value");
		if (!b) goto error;
		#ifdef ACS_MULTI
				mxmlElementSetAttr(b, "xsi:type", lw_notification->type);
		#endif
		b = mxmlNewText(b, 0, lw_notification->data);
		if (!b) goto error;		
	}
	return 0;

error:
	return -1;
}

int xml_prepare_lwnotification_message(char **msg_out)
{
	mxml_node_t *tree, *b, *n, *parameter_list;
	struct external_parameter *external_parameter;
	struct cwmp   *cwmp = &cwmp_main;
	struct config   *conf;
	conf = &(cwmp->conf);
	char *c = NULL;
	int counter = 0;

	tree = mxmlLoadString(NULL, CWMP_LWNOTIFICATION_MESSAGE, MXML_NO_CALLBACK);
	if (!tree) goto error;

	b = mxmlFindElement(tree, tree, "TS", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	if (asprintf(&c, "%ld", time(NULL)) == -1)
		goto error;
	b = mxmlNewText(b, 0,c);
	free(c);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "UN", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, conf->acs_userid);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "CN", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	c = calculate_lwnotification_cnonce();
	b = mxmlNewText(b, 0,c);
	free(c);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "OUI", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, cwmp->deviceid.oui);
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "ProductClass", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, cwmp->deviceid.productclass ? cwmp->deviceid.productclass : "");
	if (!b) goto error;

	b = mxmlFindElement(tree, tree, "SerialNumber", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, cwmp->deviceid.serialnumber ? cwmp->deviceid.serialnumber : "");
	if (!b) goto error;

	parameter_list = mxmlFindElement(tree, tree, "Notification", NULL, NULL, MXML_DESCEND);
	if (!parameter_list) goto error;
	if (xml_prepare_lwnotifications(parameter_list))
		goto error;

	*msg_out = mxmlSaveAllocString(tree, whitespace_cb);

	mxmlDelete(tree);
	return 0;

error:
	return -1;
}

char* xml_get_cwmp_version (int version) 
{
	int k;	
	char tmp[10]  = "";
	static char versions[60] = "";
	versions[0] = '\0';

        for (k=0; k < version; k++) {
            if (k == 0)
                sprintf(tmp, "1.%d", k);   
            else 
                sprintf(tmp, ", 1.%d", k);
            strcat(versions, tmp);
        }
	return versions;
}

int cwmp_rpc_acs_prepare_message_inform (struct cwmp *cwmp, struct session *session, struct rpc *this)
{
    struct dm_parameter *dm_parameter;
    struct event_container *event_container;
    mxml_node_t *tree, *b, *node, *parameter_list;
    char *c = NULL;
    int size = 0, i, error;
    struct list_head *ilist,*jlist;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

    if (session==NULL || this==NULL)
    {
        return -1;
    }

#ifdef DUMMY_MODE
	FILE *fp;
	fp = fopen("./ext/soap_msg_templates/cwmp_inform_message.xml", "r");
	tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
#else
	tree = mxmlLoadString(NULL, CWMP_INFORM_MESSAGE, MXML_NO_CALLBACK);
#endif
	if (!tree) goto error;
	b = mxmlFindElement(tree, tree, "soap_env:Envelope", NULL, NULL, MXML_DESCEND);
	if(!b) { goto error;}
	mxmlElementSetAttr(b, "xmlns:cwmp", cwmp_urls[(cwmp->conf.supported_amd_version)-1]);
	if ( cwmp->conf.supported_amd_version >= 4 ) {
		b = mxmlFindElement(tree, tree, "soap_env:Header", NULL, NULL, MXML_DESCEND);
		if (!b) goto error;
		node = mxmlNewElement(b, "cwmp:SessionTimeout");
		if (!node) goto error;
		mxmlElementSetAttr(node, "soap_env:mustUnderstand", "0");
		node = mxmlNewInteger(node, cwmp->conf.session_timeout);
		if (!node) goto error;
	}
	if ( cwmp->conf.supported_amd_version >= 5 ) {
		node = mxmlNewElement(b, "cwmp:SupportedCWMPVersions");
		if (!node) goto error;
		mxmlElementSetAttr(node, "soap_env:mustUnderstand", "0");
		node = mxmlNewText(node, 0,  xml_get_cwmp_version(cwmp->conf.supported_amd_version)); 
		if (!node) goto error;
	} 
	b = mxmlFindElement(tree, tree, "RetryCount", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewInteger(b, cwmp->retry_count_session);
	if (!b) goto error;

	if (xml_prepare_events_inform(session, tree))
		goto error;

	b = mxmlFindElement(tree, tree, "CurrentTime", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewText(b, 0, mix_get_time());
	if (!b) goto error;

	parameter_list = mxmlFindElement(tree, tree, "ParameterList", NULL, NULL, MXML_DESCEND);
   	if (!parameter_list) goto error;

    list_for_each(ilist, &(session->head_event_container))
    {
    	event_container = list_entry(ilist, struct event_container, list);
        list_for_each(jlist, &(event_container->head_dm_parameter))
        {
        	dm_parameter = list_entry(jlist, struct dm_parameter, list);
        	if (xml_prepare_parameters_inform(&dmctx, dm_parameter, parameter_list, &size))
        	    goto error;
        }
    }

    b = mxmlFindElement(tree, tree, "OUI", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;
	b = mxmlNewText(b, 0, cwmp->deviceid.oui ? cwmp->deviceid.oui : "");
	if (!b) goto error;

    b = mxmlFindElement(tree, tree, "Manufacturer", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;
	b = mxmlNewText(b, 0, cwmp->deviceid.manufacturer ? cwmp->deviceid.manufacturer : "");
	if (!b) goto error;

    b = mxmlFindElement(tree, tree, "ProductClass", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;
	b = mxmlNewText(b, 0, cwmp->deviceid.productclass ? cwmp->deviceid.productclass : "");
	if (!b) goto error;

    b = mxmlFindElement(tree, tree, "SerialNumber", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;
	b = mxmlNewText(b, 0, cwmp->deviceid.serialnumber ? cwmp->deviceid.serialnumber : "");
	if (!b) goto error;

    dm_entry_param_method(&dmctx, CMD_INFORM, NULL, NULL, NULL);

    while (dmctx.list_parameter.next != &dmctx.list_parameter) {
    	dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);
    	if (xml_prepare_parameters_inform(&dmctx, dm_parameter, parameter_list, &size))
    		goto error;
    	del_list_parameter(dm_parameter);
    }

    if (asprintf(&c, "cwmp:ParameterValueStruct[%d]", size) == -1)
		goto error;

    mxmlElementSetAttr(parameter_list, "xsi:type", "soap_enc:Array");
    mxmlElementSetAttr(parameter_list, "soap_enc:arrayType", c);

	free(c);
	session->tree_out = tree;

	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

int cwmp_rpc_acs_parse_response_inform (struct cwmp *cwmp, struct session *session, struct rpc *this)
{
	mxml_node_t *tree, *b;
	int i = -1;	
	char *c;
	const char *cwmp_urn;

	tree = session->tree_in;
	if (!tree) goto error;
	b = mxmlFindElement(tree, tree, "MaxEnvelopes", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;
	b = mxmlWalkNext(b, tree, MXML_DESCEND_FIRST);
	if (!b || b->type != MXML_TEXT || !b->value.text.string)
		goto error;
	if(cwmp->conf.supported_amd_version == 1) {
		cwmp->conf.amd_version = 1;
		return 0;
	}
	b = mxmlFindElement(tree, tree, "UseCWMPVersion", NULL, NULL, MXML_DESCEND);
	if (b && cwmp->conf.supported_amd_version >= 5) { //IF supported version !=5 acs response dosen't contain UseCWMPVersion
		b = mxmlWalkNext(b, tree, MXML_DESCEND_FIRST);
		if (!b || b->type != MXML_TEXT || !b->value.text.string)
			goto error;
		c = (char *)(b->value.text.string);
		if (c && *(c + 1) == '.') {
			c+=2;
			cwmp->conf.amd_version = atoi(c) + 1;
			return 0;
		}
		goto error;
	}
	for (i = 0; cwmp_urls[i] != NULL; i++) {
		cwmp_urn = cwmp_urls[i];
		c = (char *) mxmlElementGetAttrName(tree, cwmp_urn);
		if (c && *(c + 5) == ':') {
			break;
		}
	}
	if (i == 0) {
		cwmp->conf.amd_version = i+1;
	}
	else if ( i >= 1 && i <= 3) {
		switch (cwmp->conf.supported_amd_version)
        {
            case 1:
				cwmp->conf.amd_version = 1; //Already done
				break;
			case 2:
			case 3:
			case 4:
				//MIN ACS CPE
				if (cwmp->conf.supported_amd_version <= i+1)
					cwmp->conf.amd_version = cwmp->conf.supported_amd_version;
				else
					cwmp->conf.amd_version = i+1;
				break;
				//(cwmp->supported_conf.amd_version < i+1) ?"cwmp->conf.amd_version":"i+1";
			case 5:
				cwmp->conf.amd_version = i+1;
				break;			
		}
	}
	else if ( i >= 4 ) {
		cwmp->conf.amd_version = cwmp->conf.supported_amd_version;
	}
	return 0;

error:
	return -1;
}

int cwmp_rpc_acs_destroy_data_inform(struct session *session, struct rpc *rpc)
{
	event_remove_all_event_container(session,RPC_SEND);
	return 0;
}

/*
 * [RPC ACS]: GetRPCMethods
 */

int cwmp_rpc_acs_prepare_get_rpc_methods(struct cwmp *cwmp, struct session *session, struct rpc *rpc)
{
	mxml_node_t *tree, *n;

	tree = mxmlLoadString(NULL, CWMP_RESPONSE_MESSAGE, MXML_NO_CALLBACK);

	n = mxmlFindElement(tree, tree, "soap_env:Envelope", NULL, NULL, MXML_DESCEND);
	if(!n) { return -1;}
	mxmlElementSetAttr(n, "xmlns:cwmp", cwmp_urls[(cwmp->conf.amd_version)-1]);
	n = mxmlFindElement(tree, tree, "soap_env:Body",
					NULL, NULL, MXML_DESCEND);
	if (!n) return -1;

	n = mxmlNewElement(n, "cwmp:GetRPCMethods");
	if (!n) return -1;

	session->tree_out = tree;

	return 0;
}

int cwmp_rpc_acs_parse_response_get_rpc_methods (struct cwmp *cwmp, struct session *session, struct rpc *this)
{
	mxml_node_t *tree, *b;

	tree = session->tree_in;
	if (!tree) goto error;
	b = mxmlFindElement(tree, tree, "MethodList", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;
	b = mxmlWalkNext(b, tree, MXML_DESCEND_FIRST);
	if (!b || b->type != MXML_TEXT || !b->value.text.string)
		goto error;
	return 0;

error:
	return -1;
}

/*
 * [RPC ACS]: TransferComplete
 */

int cwmp_rpc_acs_prepare_transfer_complete(struct cwmp *cwmp, struct session *session, struct rpc *rpc)
{
	mxml_node_t *tree, *n;
	struct transfer_complete *p;

	p = (struct transfer_complete *)rpc->extra_data;
	tree = mxmlLoadString(NULL, CWMP_RESPONSE_MESSAGE, MXML_NO_CALLBACK);
	n = mxmlFindElement(tree, tree, "soap_env:Envelope", NULL, NULL, MXML_DESCEND);
	if(!n) { goto error;}
	mxmlElementSetAttr(n, "xmlns:cwmp", cwmp_urls[(cwmp->conf.amd_version)-1]);

	n = mxmlFindElement(tree, tree, "soap_env:Body",
					NULL, NULL, MXML_DESCEND);
	if (!n) goto error;

	n = mxmlNewElement(n, "cwmp:TransferComplete");
	if (!n) goto error;

	n = mxmlNewElement(n, "CommandKey");
	if (!n) goto error;

	n = mxmlNewText(n, 0, p->command_key);
	if (!n) goto error;

	n = n->parent->parent;
	n = mxmlNewElement(n, "StartTime");
	if (!n) goto error;

	n = mxmlNewText(n, 0, p->start_time);
	if (!n) goto error;

	n = n->parent->parent;
	n = mxmlNewElement(n, "CompleteTime");
	if (!n) goto error;

	n = mxmlNewText(n, 0, mix_get_time());
	if (!n) goto error;

	n = n->parent->parent;
	n = mxmlNewElement(n, "FaultStruct");
	if (!n) goto error;

	n = mxmlNewElement(n, "FaultCode");
	if (!n) goto error;

	n = mxmlNewText(n, 0, p->fault_code?FAULT_CPE_ARRAY[p->fault_code].CODE:"0");
	if (!n) goto error;

	n = n->parent->parent;
	n = mxmlNewElement(n, "FaultString");
	if (!n) goto error;

	n = mxmlNewText(n, 0, p->fault_code?FAULT_CPE_ARRAY [p->fault_code].DESCRIPTION:"");
	if (!n) goto error;

	session->tree_out = tree;

	return 0;

error:
	return -1;
}

int cwmp_rpc_acs_destroy_data_transfer_complete(struct session *session, struct rpc *rpc)
{
	struct transfer_complete *p;
	if(rpc->extra_data != NULL)
	{
		p = (struct transfer_complete *)rpc->extra_data;
		bkp_session_delete_transfer_complete (p);
		bkp_session_save();
		FREE(p->command_key);
		FREE(p->start_time);
		FREE(p->complete_time);
		FREE(p->old_software_version);
	}
	FREE(rpc->extra_data);
	return 0;
}

/*
 * [RPC ACS]: DUStateChangeComplete
 */

int cwmp_rpc_acs_prepare_du_state_change_complete(struct cwmp *cwmp, struct session *session, struct rpc *rpc)
{
	mxml_node_t *tree, *n;
	struct du_state_change_complete *p;

	p = (struct du_state_change_complete *)rpc->extra_data;
	tree = mxmlLoadString(NULL, CWMP_RESPONSE_MESSAGE, MXML_NO_CALLBACK);
	n = mxmlFindElement(tree, tree, "soap_env:Envelope", NULL, NULL, MXML_DESCEND);
	if(!n) { goto error;}
	mxmlElementSetAttr(n, "xmlns:cwmp", cwmp_urls[(cwmp->conf.amd_version)-1]);
	n = mxmlFindElement(tree, tree, "soap_env:Body",
					NULL, NULL, MXML_DESCEND);
	if (!n) goto error;

	n = mxmlNewElement(n, "cwmp:DUStateChangeComplete");
	if (!n) goto error;

	n = mxmlNewElement(n, "CommandKey");
	if (!n) goto error;

	n = mxmlNewText(n, 0, p->command_key);
	if (!n) goto error;
	n = n->parent->parent;
	n = mxmlNewElement(n, "Results");
	if (!n) goto error;
	
	mxml_node_t *b, *t;
	char *c;
	struct opresult *q;
	list_for_each_entry(q, &(p->list_opresult), list) {
		t = mxmlNewElement(n, "OpResultStruct");		
		if (!t) goto error;
		b = mxmlNewElement(t, "UUID");
		if (!b) goto error;
		c = q->uuid ? strdup(q->uuid) : strdup("");
		b = mxmlNewText(b, 0, c);
		FREE(c);
		if (!b) goto error;
		b = mxmlNewElement(t, "DeploymentUnitRef");
		if (!b) goto error;
		c = q->du_ref ? strdup(q->du_ref) : strdup("");
		b = mxmlNewText(b, 0, c);
		FREE(c);
		if (!b) goto error;
		b = mxmlNewElement(t, "Version");
		if (!b) goto error;
		c = q->version ? strdup(q->version) : strdup("");
		b = mxmlNewText(b, 0, c);
		FREE(c);
		b = mxmlNewElement(t, "CurrentState");
		if (!b) goto error;
		c = q->current_state ? strdup(q->current_state) : strdup("");
		b = mxmlNewText(b, 0, c);
		FREE(c);		
		if (!b) goto error;
		b = mxmlNewElement(t, "FaultStruct");
		if (!b) goto error;

		b = mxmlNewElement(b, "FaultCode");
		if (!n) goto error;

		b = mxmlNewText(b, 0, q->fault?FAULT_CPE_ARRAY[q->fault].CODE:"0");
		if (!n) goto error;

		b = b->parent->parent;
		b = mxmlNewElement(b, "FaultString");
		if (!b) goto error;

		b= mxmlNewText(b, 0, q->fault?FAULT_CPE_ARRAY [q->fault].DESCRIPTION:"");
		if (!b) goto error;
		
	}
	session->tree_out = tree;

	return 0;

error:
	return -1;
}

int cwmp_rpc_acs_destroy_data_du_state_change_complete(struct session *session, struct rpc *rpc)
{
	struct du_state_change_complete *p;
	if(rpc->extra_data != NULL)
	{
		p = (struct du_state_change_complete *)rpc->extra_data;
		bkp_session_delete_du_state_change_complete (p);
		bkp_session_save();
		FREE(p->command_key);
	}
	return 0;
}


/*
 * [RPC CPE]: GetParameterValues
 */

int cwmp_handle_rpc_cpe_get_parameter_values(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *parameter_list, *b;
	struct dm_parameter *dm_parameter;
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	char *c = NULL;
	int counter = 0, fault_code = FAULT_CPE_INTERNAL_ERROR;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

	b = session->body_in;
	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
			    NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:GetParameterValuesResponse");
	if (!n) goto fault;

	parameter_list = mxmlNewElement(n, "ParameterList");
	if (!parameter_list) goto fault;

#ifdef ACS_MULTI
	mxmlElementSetAttr(parameter_list, "xsi:type", "soap_enc:Array");
#endif

	while (b) {
		if (b && b->type == MXML_TEXT &&
		    b->value.text.string &&
		    b->parent->type == MXML_ELEMENT &&
		    !strcmp(b->parent->value.element.name, "string")) {
			parameter_name = b->value.text.string;
		}
		if (b && b->type == MXML_ELEMENT && /* added in order to support GetParameterValues with empty string*/
			!strcmp(b->value.element.name, "string") &&
			!b->child) {
			parameter_name = "";
		}
		if (parameter_name) {
			int e = dm_entry_param_method(&dmctx, CMD_GET_VALUE, parameter_name, NULL, NULL);
			if (e) {
				fault_code = cwmp_get_fault_code(e);
				goto fault;
			}
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
		parameter_name = NULL;
	}

    while (dmctx.list_parameter.next != &dmctx.list_parameter) {
    	dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);

    	n = mxmlNewElement(parameter_list, "ParameterValueStruct");
		if (!n) goto fault;

		n = mxmlNewElement(n, "Name");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, dm_parameter->name);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "Value");
		if (!n) goto fault;

#ifdef ACS_MULTI
		mxmlElementSetAttr(n, "xsi:type", dm_parameter->type);
#endif
		n = mxmlNewText(n, 0, dm_parameter->data? dm_parameter->data : "");
		if (!n) goto fault;

		counter++;

		del_list_parameter(dm_parameter);
	}
#ifdef ACS_MULTI
	b = mxmlFindElement(session->tree_out, session->tree_out, "ParameterList",
			    NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	if (asprintf(&c, "cwmp:ParameterValueStruct[%d]", counter) == -1)
		goto fault;

	mxmlElementSetAttr(b, "soap_enc:arrayType", c);
	FREE(c);
#endif

	dm_ctx_clean(&dmctx);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

/*
 * [RPC CPE]: GetParameterNames
 */

int cwmp_handle_rpc_cpe_get_parameter_names(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *parameter_list, *b = session->body_in;
	struct dm_parameter *dm_parameter;
	char *parameter_name = NULL;
	char *NextLevel = NULL;
	char *c;
	int counter = 0, fault_code = FAULT_CPE_INTERNAL_ERROR;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:GetParameterNamesResponse");
	if (!n) goto fault;

	parameter_list = mxmlNewElement(n, "ParameterList");
	if (!parameter_list) goto fault;

#ifdef ACS_MULTI
	mxmlElementSetAttr(parameter_list, "xsi:type", "soap_enc:Array");
#endif

	while (b) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "ParameterPath")) {
			parameter_name = b->value.text.string;
		}
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "ParameterPath") &&
			!b->child) {
			parameter_name = "";
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "NextLevel")) {
			NextLevel = b->value.text.string;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}
	if (parameter_name && NextLevel) {
		int e = dm_entry_param_method(&dmctx, CMD_GET_NAME, parameter_name, NextLevel, NULL);
		if (e) {
			fault_code = cwmp_get_fault_code(e);
			goto fault;
		}
	}

    while (dmctx.list_parameter.next != &dmctx.list_parameter) {
    	dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);

		n = mxmlNewElement(parameter_list, "ParameterInfoStruct");
		if (!n) goto fault;

		n = mxmlNewElement(n, "Name");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, dm_parameter->name);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "Writable");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, dm_parameter->data);
		if (!n) goto fault;

		counter++;

		del_list_parameter(dm_parameter);
	}

#ifdef ACS_MULTI
	b = mxmlFindElement(session->tree_out, session->tree_out, "ParameterList",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	if (asprintf(&c, "cwmp:ParameterInfoStruct[%d]", counter) == -1)
		goto fault;

	mxmlElementSetAttr(b, "soap_enc:arrayType", c);
	FREE(c);
#endif

	dm_ctx_clean(&dmctx);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

/*
 * [RPC CPE]: GetParameterAttributes
 */

int cwmp_handle_rpc_cpe_get_parameter_attributes(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *parameter_list, *b;
	struct dm_parameter *dm_parameter;
	char *parameter_name = NULL;
	char *c=NULL;
	int counter = 0, fault_code = FAULT_CPE_INTERNAL_ERROR;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

	b = session->body_in;
	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:GetParameterAttributesResponse");
	if (!n) goto fault;

	parameter_list = mxmlNewElement(n, "ParameterList");
	if (!parameter_list) goto fault;

#ifdef ACS_MULTI
	mxmlElementSetAttr(parameter_list, "xsi:type", "soap_enc:Array");
#endif

	while (b) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "string")) {
			parameter_name = b->value.text.string;
		}
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "string") &&
			!b->child) {
			parameter_name = "";
		}
		if (parameter_name) {
			int e = dm_entry_param_method(&dmctx, CMD_GET_NOTIFICATION, parameter_name, NULL, NULL);
			if (e) {
				fault_code = cwmp_get_fault_code(e);
				goto fault;
			}
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
		parameter_name = NULL;
	}

    while (dmctx.list_parameter.next != &dmctx.list_parameter) {
    	dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);

		n = mxmlNewElement(parameter_list, "ParameterAttributeStruct");
		if (!n) goto fault;

		n = mxmlNewElement(n, "Name");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, dm_parameter->name);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "Notification");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, dm_parameter->data);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "AccessList");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, "");
		if (!n) goto fault;

		counter++;

		del_list_parameter(dm_parameter);
	}
#ifdef ACS_MULTI
	b = mxmlFindElement(session->tree_out, session->tree_out, "ParameterList",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	if (asprintf(&c, "cwmp:ParameterAttributeStruct[%d]", counter) == -1)
		goto fault;

	mxmlElementSetAttr(b, "soap_enc:arrayType", c);
	FREE(c);
#endif

	dm_ctx_clean(&dmctx);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

/*
 * [RPC CPE]: SetParameterValues
 */

static int is_duplicated_parameter(mxml_node_t *param_node, struct session *session)
{
	mxml_node_t *b = param_node;
	while(b = mxmlWalkNext(b, session->body_in, MXML_DESCEND)) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Name")) {
			if(strcmp(b->value.text.string, param_node->value.text.string)==0)
				return -1;
		}
	}
	return 0;
}

int cwmp_handle_rpc_cpe_set_parameter_values(struct session *session, struct rpc *rpc)
{
	mxml_node_t *b, *n;
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	char *parameter_key = NULL;
	char *status = NULL;
	char *v, *c = NULL;
	char buf[128];
	int fault_code = FAULT_CPE_INTERNAL_ERROR;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

	b = mxmlFindElement(session->body_in, session->body_in, "ParameterList", NULL, NULL, MXML_DESCEND);
	if(!b) {
		fault_code = FAULT_CPE_REQUEST_DENIED;
		goto fault;
	}

	while (b) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Name")) {
			parameter_name = b->value.text.string;
			if (is_duplicated_parameter(b,session)) {
				fault_code = FAULT_CPE_INVALID_ARGUMENTS;
				goto fault;
			}
			FREE(parameter_value);
		}

		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "Name") &&
			!b->child) {
			parameter_name = "";
		}

		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Value")) {
			int whitespace;
			parameter_value = strdup((char *)mxmlGetText(b, &whitespace));
			n = b->parent;
			while (b = mxmlWalkNext(b, n, MXML_DESCEND)) {
				v = (char *)mxmlGetText(b, &whitespace);
				if (!whitespace) break;
				asprintf(&c, "%s %s", parameter_value, v);
				FREE(parameter_value);
				parameter_value = c;
			}
			b = n->last_child;
		}

		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "Value") &&
			!b->child) {
			parameter_value = strdup("");
		}
		if (parameter_name && parameter_value) {
			int e = dm_entry_param_method(&dmctx, CMD_SET_VALUE, parameter_name, parameter_value, NULL);
			if (e) {
				fault_code = FAULT_CPE_INVALID_ARGUMENTS;
			}
			parameter_name = NULL;
			FREE(parameter_value);
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}

	if (fault_code == FAULT_CPE_INVALID_ARGUMENTS) {
		goto fault;
	}

	b = mxmlFindElement(session->body_in, session->body_in, "ParameterKey", NULL, NULL, MXML_DESCEND);
	if(!b) {
		fault_code = FAULT_CPE_REQUEST_DENIED;
		goto fault;
	}

	b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
	if (b && b->type == MXML_TEXT && b->value.text.string)
		parameter_key = b->value.text.string;

	int f = dm_entry_apply(&dmctx, CMD_SET_VALUE, parameter_key ? parameter_key : "", NULL);
	if (f) {
		fault_code = cwmp_get_fault_code(f);
		goto fault;
	}

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:SetParameterValuesResponse");
	if (!b) goto fault;

	b = mxmlNewElement(b, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "1");
	if (!b) goto fault;

success:
	dm_ctx_clean(&dmctx);
	return 0;

fault:
	rpc->list_set_value_fault = &dmctx.list_fault_param;
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

/*
 * [RPC CPE]: SetParameterAttributes
 */

int cwmp_handle_rpc_cpe_set_parameter_attributes(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *b = session->body_in;
	char *c, *parameter_name = NULL, *parameter_notification = NULL, *attr_notification_update = NULL;
	int fault_code = FAULT_CPE_INTERNAL_ERROR;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

	/* handle cwmp:SetParameterAttributes */
	if (asprintf(&c, "%s:%s", ns.cwmp, "SetParameterAttributes") == -1)
		goto fault;

	n = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if (!n) goto fault;
	b = n;

	while (b != NULL) {
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "SetParameterAttributesStruct")) {
			attr_notification_update = NULL;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Name")) {
			parameter_name = b->value.text.string;
		}
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "Name") &&
			!b->child) {
			parameter_name = "";
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "NotificationChange")) {
			attr_notification_update = b->value.text.string;
		}
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "NotificationChange") &&
			!b->child) {
			attr_notification_update = "";
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Notification")) {
			parameter_notification = b->value.text.string;
		}
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->value.element.name, "Notification") &&
			!b->child) {
			parameter_notification = "";
		}
		if (attr_notification_update && parameter_name && parameter_notification) {
			int e = dm_entry_param_method(&dmctx, CMD_SET_NOTIFICATION, parameter_name, parameter_notification, attr_notification_update);
			if (e) {
				fault_code = cwmp_get_fault_code(e);
				goto fault;
			}
			attr_notification_update = NULL;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}

	int f = dm_entry_apply(&dmctx, CMD_SET_NOTIFICATION, NULL, NULL);
	if (f) {
		fault_code = cwmp_get_fault_code(f);
		goto fault;
	}

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:SetParameterAttributesResponse");
	if (!b) goto fault;

end_success:
	dm_ctx_clean(&dmctx);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

/*
 * [RPC CPE]: AddObject
 */

int cwmp_handle_rpc_cpe_add_object(struct session *session, struct rpc *rpc)
{
	mxml_node_t *b;
	struct paramameter_container *paramameter_container;
	char buf[128];
	char *object_name = NULL;
	char *parameter_key = NULL;
	int fault_code = FAULT_CPE_INTERNAL_ERROR;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

	b = session->body_in;
	while (b) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "ObjectName")) {
			object_name = b->value.text.string;
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "ParameterKey")) {
			parameter_key = b->value.text.string;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}

	if (object_name) {
		int e = dm_entry_param_method(&dmctx, CMD_ADD_OBJECT, object_name, parameter_key ? parameter_key : "", NULL);
		if (e) {
			fault_code = cwmp_get_fault_code(e);
			goto fault;
		}
	} else {
		fault_code = FAULT_CPE_INVALID_PARAMETER_NAME;
		goto fault;
	}

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:AddObjectResponse");
	if (!b) goto fault;

	b = mxmlNewElement(b, "InstanceNumber");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, dmctx.addobj_instance);
	if (!b) goto fault;

	b = b->parent->parent;
	b = mxmlNewElement(b, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "1");
	if (!b) goto fault;

success:
	dm_ctx_clean(&dmctx);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

/*
 * [RPC CPE]: DeleteObject
 */

int cwmp_handle_rpc_cpe_delete_object(struct session *session, struct rpc *rpc)
{
	mxml_node_t *b;
	char buf[128];
	char *object_name = NULL;
	char *parameter_key = NULL;
	int fault_code = FAULT_CPE_INTERNAL_ERROR;
	struct dmctx dmctx = {0};

    dm_ctx_init(&dmctx);

	b = session->body_in;
	while (b) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "ObjectName")) {
			object_name = b->value.text.string;
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "ParameterKey")) {
			parameter_key = b->value.text.string;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}

	if (object_name) {
		int e = dm_entry_param_method(&dmctx, CMD_DEL_OBJECT, object_name, parameter_key ? parameter_key : "", NULL);
		if (e) {
			fault_code = cwmp_get_fault_code(e);
			goto fault;
		}
	} else {
		fault_code = FAULT_CPE_INVALID_PARAMETER_NAME;
		goto fault;
	}


	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:DeleteObjectResponse");
	if (!b) goto fault;

	b = mxmlNewElement(b, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "1");
	if (!b) goto fault;

success:
	dm_ctx_clean(&dmctx);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	dm_ctx_clean(&dmctx);
	return 0;

error:
	dm_ctx_clean(&dmctx);
	return -1;
}

/*
 * [RPC CPE]: GetRPCMethods
 */

int cwmp_handle_rpc_cpe_get_rpc_methods(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *method_list, *b = session->body_in;
	char *c = NULL;
	int i,counter = 0;

	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:GetRPCMethodsResponse");
	if (!n) goto fault;

	method_list = mxmlNewElement(n, "MethodList");
	if (!method_list) goto fault;

	for (i=1; i<__RPC_CPE_MAX; i++)
	{
		if (i!= RPC_CPE_FAULT)
		{
			n = mxmlNewElement(method_list, "string");
			if (!n) goto fault;

			n = mxmlNewText(n, 0, rpc_cpe_methods[i].name);
			if (!n) goto fault;

			counter++;
		}
	}
#ifdef ACS_MULTI
	b = mxmlFindElement(session->tree_out, session->tree_out, "MethodList",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	mxmlElementSetAttr(b, "xsi:type", "soap_enc:Array");
	if (asprintf(&c, "xsd:string[%d]", counter) == -1)
		goto fault;

	mxmlElementSetAttr(b, "soap_enc:arrayType", c);
	FREE(c);
#endif

	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, FAULT_CPE_INTERNAL_ERROR))
		goto error;
	return 0;

error:
	return -1;
}

/*
 * [RPC CPE]: FactoryReset
 */

int cwmp_handle_rpc_cpe_factory_reset(struct session *session, struct rpc *rpc)
{
	mxml_node_t *b;

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:FactoryResetResponse");
	if (!b) goto fault;

	cwmp_set_end_session(END_SESSION_FACTORY_RESET);

	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, FAULT_CPE_INTERNAL_ERROR))
		goto error;
	return 0;

error:
	return -1;
}

/*
 * [RPC CPE]: CancelTransfer
 */

int cwmp_handle_rpc_cpe_cancel_transfer(struct session *session, struct rpc *rpc)
{
	mxml_node_t *b;
	char *command_key = NULL;

	b = session->body_in;
	while (b) {
		if (b && b->type == MXML_TEXT &&
		b->value.text.string &&
		b->parent->type == MXML_ELEMENT &&
		!strcmp(b->parent->value.element.name, "CommandKey")) {
			command_key = b->value.text.string;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}
	if(command_key) {
		cancel_transfer(command_key);	
	}
	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:CancelTransferResponse");
	if (!b) goto fault;
	return 0;
fault:
	if (cwmp_create_fault_message(session, rpc, FAULT_CPE_INTERNAL_ERROR))
		goto error;
	return 0;

error:
	return -1;
}

int cancel_transfer(char * key) {
	struct upload			*pupload;
	struct download			*pdownload;   
	struct list_head		*ilist, *q;
	
	if(list_download.next!=&(list_download)) {
		list_for_each_safe(ilist,q,&(list_download))
		{
			pdownload = list_entry(ilist, struct download, list);
			if(strcmp(pdownload->command_key, key) == 0 )
			{
				pthread_mutex_lock (&mutex_download);
				bkp_session_delete_download(pdownload);
				bkp_session_save();
				list_del (&(pdownload->list));
				if(pdownload->scheduled_time != 0)
				    count_download_queue--;
				cwmp_free_download_request(pdownload);
				pthread_mutex_unlock (&mutex_download);
			}       
	    }
	}
	if(list_upload.next!=&(list_upload)) {
		list_for_each_safe(ilist,q,&(list_upload))
		{
			pupload = list_entry(ilist, struct upload, list);
			if(strcmp(pupload->command_key, key) == 0 )
			{
				pthread_mutex_lock (&mutex_upload);
				bkp_session_delete_upload(pupload);
				bkp_session_save(); //is it needed
				list_del (&(pupload->list));
				if(pupload->scheduled_time != 0)
				    count_download_queue--;
				cwmp_free_upload_request(pupload);
				pthread_mutex_unlock (&mutex_upload);
			}       
	    }
	}
	// Cancel schedule download
	return CWMP_OK;
}


/*
 * [RPC CPE]: Reboot
 */

int cwmp_handle_rpc_cpe_reboot(struct session *session, struct rpc *rpc)
{
	mxml_node_t *b;
	struct event_container  *event_container;
	char *command_key = NULL;

	b = session->body_in;

	while (b) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "CommandKey")) {
			command_key = b->value.text.string;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}

	pthread_mutex_lock (&(cwmp_main.mutex_session_queue));
	event_container = cwmp_add_event_container (&cwmp_main, EVENT_IDX_M_Reboot, command_key);
	if (event_container == NULL)
	{
		pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));
		goto fault;
	}
	cwmp_save_event_container (&cwmp_main,event_container);
	pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:RebootResponse");
	if (!b) goto fault;

	cwmp_set_end_session(END_SESSION_REBOOT);

	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, FAULT_CPE_INTERNAL_ERROR))
		goto error;
	return 0;

error:
	return -1;
}

/*
 * [RPC CPE]: ScheduleInform
 */

void *thread_cwmp_rpc_cpe_scheduleInform (void *v)
{
    struct cwmp                     *cwmp = (struct cwmp *)v;
    struct event_container          *event_container;
    struct schedule_inform          *schedule_inform;
    struct timespec                 si_timeout = {0, 0};
    time_t                          current_time, stime;
    bool                            add_event_same_time = false;

    for(;;)
    {
        if(list_schedule_inform.next!=&(list_schedule_inform)) {
            schedule_inform = list_entry(list_schedule_inform.next,struct schedule_inform, list);
            stime = schedule_inform->scheduled_time;
            current_time    = time(NULL);
            if (current_time >= schedule_inform->scheduled_time)
            {
                if (add_event_same_time)
                {
                    pthread_mutex_lock (&mutex_schedule_inform);
                    list_del (&(schedule_inform->list));
                    if (schedule_inform->commandKey!=NULL)
                    {
                        bkp_session_delete_schedule_inform(schedule_inform->scheduled_time,schedule_inform->commandKey);
                        free (schedule_inform->commandKey);
                    }
                    free(schedule_inform);
                    pthread_mutex_unlock (&mutex_schedule_inform);
                    continue;
                }
                pthread_mutex_lock (&(cwmp->mutex_session_queue));
                CWMP_LOG(INFO,"Schedule Inform thread: add ScheduleInform event in the queue");
                event_container = cwmp_add_event_container (cwmp, EVENT_IDX_3SCHEDULED, "");
                if (event_container != NULL)
                {
                    cwmp_save_event_container (cwmp,event_container);
                }
                event_container = cwmp_add_event_container (cwmp, EVENT_IDX_M_ScheduleInform, schedule_inform->commandKey);
                if (event_container != NULL)
                {
                    cwmp_save_event_container (cwmp,event_container);
                }
                pthread_mutex_unlock (&(cwmp->mutex_session_queue));
                pthread_cond_signal(&(cwmp->threshold_session_send));
                pthread_mutex_lock (&mutex_schedule_inform);
                list_del (&(schedule_inform->list));
                if (schedule_inform->commandKey != NULL)
                {
                    bkp_session_delete_schedule_inform(schedule_inform->scheduled_time,schedule_inform->commandKey);
                    free (schedule_inform->commandKey);
                }
                free(schedule_inform);
                count_schedule_inform_queue--;
                pthread_mutex_unlock (&mutex_schedule_inform);
                add_event_same_time = true;
                continue;
            }
            bkp_session_save();
        add_event_same_time = false;
        pthread_mutex_lock (&mutex_schedule_inform);
        si_timeout.tv_sec = stime;
        pthread_cond_timedwait(&threshold_schedule_inform, &mutex_schedule_inform, &si_timeout);
        pthread_mutex_unlock (&mutex_schedule_inform);
        } else {
    bkp_session_save();
            add_event_same_time = false;
            pthread_mutex_lock (&mutex_schedule_inform);
            pthread_cond_wait(&threshold_schedule_inform, &mutex_schedule_inform);
            pthread_mutex_unlock (&mutex_schedule_inform);
        }
    }

    return NULL;
}

int cwmp_scheduleInform_remove_all()
{
	struct schedule_inform          *schedule_inform;

	pthread_mutex_lock (&mutex_schedule_inform);
	while (list_schedule_inform.next!=&(list_schedule_inform))
    {
        schedule_inform = list_entry(list_schedule_inform.next,struct schedule_inform, list);

		list_del (&(schedule_inform->list));
		if (schedule_inform->commandKey!=NULL)
		{
			bkp_session_delete_schedule_inform(schedule_inform->scheduled_time,schedule_inform->commandKey);
			free (schedule_inform->commandKey);
		}
		free(schedule_inform);
    }
	bkp_session_save();
	pthread_mutex_unlock (&mutex_schedule_inform);

	return CWMP_OK;
}

int cwmp_handle_rpc_cpe_schedule_inform(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *method_list, *b = session->body_in;
	char *c = NULL, *command_key = NULL;
	int i,counter = 0;
    struct event_container          *event_container;
    struct schedule_inform          *schedule_inform;
    time_t                          scheduled_time;
    struct list_head                *ilist;
    bool                            cond_signal=false;
    pthread_t                       scheduleInform_thread;
    int                             error,fault = FAULT_CPE_NO_FAULT;
    unsigned int					delay_seconds = 0;

    pthread_mutex_lock (&mutex_schedule_inform);

    while (b) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "CommandKey")) {
			command_key = b->value.text.string;
		}

		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "DelaySeconds")) {
			delay_seconds = atoi(b->value.text.string);
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}

    if (delay_seconds <= 0) {
    	fault = FAULT_CPE_INVALID_ARGUMENTS;
		pthread_mutex_unlock (&mutex_schedule_inform);
		goto fault;
    }

    if(count_schedule_inform_queue>=MAX_SCHEDULE_INFORM_QUEUE)
	{
		fault = FAULT_CPE_RESOURCES_EXCEEDED;
		pthread_mutex_unlock (&mutex_schedule_inform);
		goto fault;
	}
    count_schedule_inform_queue++;

    scheduled_time = time(NULL) + delay_seconds;
    list_for_each(ilist,&(list_schedule_inform))
    {
        schedule_inform = list_entry(ilist,struct schedule_inform, list);
        if (schedule_inform->scheduled_time >= scheduled_time)
        {
            break;
        }
    }

    n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
                NULL, NULL, MXML_DESCEND);
    if (!n) goto fault;

    n = mxmlNewElement(n, "cwmp:ScheduleInformResponse");
    if (!n) goto fault;


    CWMP_LOG(INFO,"Schedule inform event will start in %us",delay_seconds);
    schedule_inform = calloc (1,sizeof(struct schedule_inform));
    if (schedule_inform==NULL)
    {
        pthread_mutex_unlock (&mutex_schedule_inform);
        goto fault;
    }
    schedule_inform->commandKey     = strdup(command_key);
    schedule_inform->scheduled_time = scheduled_time;
    list_add (&(schedule_inform->list), ilist->prev);
    bkp_session_insert_schedule_inform(schedule_inform->scheduled_time,schedule_inform->commandKey);
    bkp_session_save();
    pthread_mutex_unlock (&mutex_schedule_inform);
        pthread_cond_signal(&threshold_schedule_inform);

success:
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault?fault:FAULT_CPE_INTERNAL_ERROR))
		goto error;
	goto success;

error:
	return -1;
}

/*
 * [RPC CPE]: Download
 */

int cwmp_launch_download(struct download *pdownload, struct transfer_complete **ptransfer_complete)
{
    int							i, error = FAULT_CPE_NO_FAULT;
    char						*download_startTime;
    struct transfer_complete	*p;
    char						*fault_code;
    char						file_size[128];

    download_startTime = mix_get_time();

    bkp_session_delete_download(pdownload);
    bkp_session_save();

    sprintf(file_size,"%d",pdownload->file_size);
    external_download(pdownload->url, file_size, pdownload->file_type,
    		pdownload->username, pdownload->password, 0);
    external_handle_action(cwmp_handle_downloadFault);
    external_fetch_downloadFaultResp(&fault_code);

    if(fault_code != NULL)
    {
    	if(fault_code[0]=='9')
    	{
			for(i=1;i<__FAULT_CPE_MAX;i++)
			{
				if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
				{
					error = i;
					break;
				}
			}
    	}
    	free(fault_code);
    }
    /*else {
    	error = FAULT_CPE_INTERNAL_ERROR;
    }*/

	p = calloc (1,sizeof(struct transfer_complete));
	if(p == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		return error;
	}

	p->command_key			= strdup(pdownload->command_key);
	p->start_time 			= strdup(download_startTime);
	p->complete_time		= strdup(mix_get_time());
	if(error != FAULT_CPE_NO_FAULT)
	{
		p->fault_code 		= error;
	}

	*ptransfer_complete = p;

    return error;
}

int cwmp_launch_schedule_download(struct schedule_download *pdownload, struct transfer_complete **ptransfer_complete)
{
    int							i, error = FAULT_CPE_NO_FAULT;
    char						*download_startTime;
    struct transfer_complete	*p;
    char						*fault_code;
    char						file_size[128];

    download_startTime = mix_get_time();

    bkp_session_delete_schedule_download(pdownload);
    bkp_session_save();

    sprintf(file_size,"%d",pdownload->file_size);
    external_download(pdownload->url, file_size, pdownload->file_type,
    		pdownload->username, pdownload->password, pdownload->timewindowstruct[0].windowstart);
    external_handle_action(cwmp_handle_downloadFault);
    external_fetch_downloadFaultResp(&fault_code);

    if(fault_code != NULL)
    {
    	if(fault_code[0]=='9')
    	{
			for(i=1;i<__FAULT_CPE_MAX;i++)
			{
				if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
				{
					error = i;
					break;
				}
			}
    	}
    	free(fault_code);
    }
    /*else {
    	error = FAULT_CPE_INTERNAL_ERROR;
    }*/

	p = calloc (1,sizeof(struct transfer_complete));
	if(p == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		return error;
	}

	p->command_key			= strdup(pdownload->command_key);
	p->start_time 			= strdup(download_startTime);
	p->complete_time		= strdup(mix_get_time());
	p->type					= TYPE_SCHEDULE_DOWNLOAD;
	if(error != FAULT_CPE_NO_FAULT)
	{
		p->fault_code 		= error;
	}

	*ptransfer_complete = p;

    return error;
}

int cwmp_launch_upload(struct upload *pupload, struct transfer_complete **ptransfer_complete)
{
	int							i, error = FAULT_CPE_NO_FAULT;
	char						*upload_startTime;
	struct transfer_complete	*p;
	char						*fault_code;
	char *name = "";
	struct dmctx dmctx = {0};
	upload_startTime = mix_get_time();

	bkp_session_delete_upload(pupload);
	bkp_session_save();

	dm_ctx_init(&dmctx);
	if (pupload->file_type[0] == '3' && pupload->f_instance && isdigit(pupload->f_instance[0])) {
		lookup_vcf_name(pupload->f_instance, &name);
	}
	external_upload(pupload->url, pupload->file_type,
			pupload->username, pupload->password, name);
	dm_ctx_clean(&dmctx);
	external_handle_action(cwmp_handle_uploadFault);
	external_fetch_uploadFaultResp(&fault_code);

	if(fault_code != NULL)
	{
		if(fault_code[0]=='9')
		{
			for(i=1;i<__FAULT_CPE_MAX;i++)
			{
				if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
				{
					error = i;
					break;
				}
			}
		}
		free(fault_code);
	}
	p = calloc (1,sizeof(struct transfer_complete));
	if(p == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		return error;
	}

	p->command_key			= strdup(pupload->command_key);
	p->start_time 			= strdup(upload_startTime);
	p->complete_time		= strdup(mix_get_time());
	if(error != FAULT_CPE_NO_FAULT)
	{
		p->fault_code 		= error;
	}

	*ptransfer_complete = p;

	return error;
}

void *thread_cwmp_rpc_cpe_download (void *v)
{
    struct cwmp                     			*cwmp = (struct cwmp *)v;
    struct download          					*pdownload;
    struct timespec                 			download_timeout = {0, 0};
    time_t                          			current_time, stime;
    int											i,error = FAULT_CPE_NO_FAULT;
    struct transfer_complete					*ptransfer_complete;
    long int									time_of_grace = 3600,timeout;
    char										*fault_code;

    for(;;)
    {
        if (list_download.next!=&(list_download)) {
            pdownload = list_entry(list_download.next,struct download, list);
            stime = pdownload->scheduled_time;
            current_time    = time(NULL);
            if(pdownload->scheduled_time != 0)
                timeout = current_time - pdownload->scheduled_time;
            else
                timeout = 0;
            if((timeout >= 0)&&(timeout > time_of_grace))
            {
                pthread_mutex_lock (&mutex_download);
                bkp_session_delete_download(pdownload);
                ptransfer_complete = calloc (1,sizeof(struct transfer_complete));
                if(ptransfer_complete != NULL)
                {
                    error = FAULT_CPE_DOWNLOAD_FAILURE;

                    ptransfer_complete->command_key		= strdup(pdownload->command_key);
                    ptransfer_complete->start_time 		= strdup(mix_get_time());
                    ptransfer_complete->complete_time	= strdup(ptransfer_complete->start_time);
                    ptransfer_complete->fault_code		= error;

                    ptransfer_complete->type = TYPE_DOWNLOAD;
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                }
                list_del (&(pdownload->list));
                if(pdownload->scheduled_time != 0)
                    count_download_queue--;
                cwmp_free_download_request(pdownload);
                pthread_mutex_unlock (&mutex_download);
                continue;
            }
            if((timeout >= 0)&&(timeout <= time_of_grace))
            {
                pthread_mutex_lock (&(cwmp->mutex_session_send));
                external_init();
                CWMP_LOG(INFO,"Launch download file %s",pdownload->url);
                error = cwmp_launch_download(pdownload,&ptransfer_complete);
                if(error != FAULT_CPE_NO_FAULT)
                {
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    bkp_session_save();
                    cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                    bkp_session_delete_transfer_complete(ptransfer_complete);
                }
                else
                {
                    if (pdownload->file_type[0] == '1') {
                    	ptransfer_complete->old_software_version = cwmp->deviceid.softwareversion;
                    }
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    bkp_session_save();
                    external_apply("download", pdownload->file_type, 0);
                    external_handle_action(cwmp_handle_downloadFault);
                    external_fetch_downloadFaultResp(&fault_code);
                    if(fault_code != NULL)
                    {
                        if(fault_code[0]=='9')
                        {
                            for(i=1;i<__FAULT_CPE_MAX;i++)
                            {
                                if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
                                {
                                    error = i;
                                    break;
                                }
                            }
                        }
                        free(fault_code);
                        if((error == FAULT_CPE_NO_FAULT) &&
                            (pdownload->file_type[0] == '1' || pdownload->file_type[0] == '3'))
                        {
                            exit(EXIT_SUCCESS);
                        }
                        bkp_session_delete_transfer_complete(ptransfer_complete);
                        ptransfer_complete->fault_code = error;
                        bkp_session_insert_transfer_complete(ptransfer_complete);
                        bkp_session_save();
                        cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                    }
                }
                external_exit();
                pthread_mutex_unlock (&(cwmp->mutex_session_send));
                pthread_cond_signal (&(cwmp->threshold_session_send));
                pthread_mutex_lock (&mutex_download);
                list_del (&(pdownload->list));
                if(pdownload->scheduled_time != 0)
                    count_download_queue--;
                cwmp_free_download_request(pdownload);
                pthread_mutex_unlock (&mutex_download);
                continue;
            }
        pthread_mutex_lock (&mutex_download);
        download_timeout.tv_sec = stime;
        pthread_cond_timedwait(&threshold_download, &mutex_download, &download_timeout);
        pthread_mutex_unlock (&mutex_download);
        } else {
            pthread_mutex_lock (&mutex_download);
            pthread_cond_wait(&threshold_download, &mutex_download);
            pthread_mutex_unlock (&mutex_download);
        }
    }
    return NULL;
}

void *thread_cwmp_rpc_cpe_schedule_download (void *v)
{
    struct cwmp                     			*cwmp = (struct cwmp *)v;
    struct schedule_download          			*pschedule_download;
    struct timespec                 			download_timeout = {0, 0};
    time_t                          			current_time, stime;
    int											i,error = FAULT_CPE_NO_FAULT;
    struct transfer_complete					*ptransfer_complete;
    long int									timeout;
    char										*fault_code;
	int min_time = 0;
	struct schedule_download *current_download = NULL;
	struct schedule_download *p, *_p;
	for(;;)
    {
		current_time = time(NULL);
		if(list_schedule_download.next != &(list_schedule_download))
		{
			list_for_each_entry_safe(p, _p, &(list_schedule_download), list)
			{
				if (min_time == 0)
				{
					if (p->timewindowstruct[0].windowend >= current_time)
					{
						min_time = p->timewindowstruct[0].windowstart;
						current_download = p;
					}
					else if (p->timewindowstruct[1].windowend >= current_time)
					{
						min_time = p->timewindowstruct[1].windowstart;
						current_download = p;
					}
					else 
					{
						pthread_mutex_lock (&mutex_schedule_download);
				        bkp_session_delete_schedule_download(p);
				        ptransfer_complete = calloc (1,sizeof(struct transfer_complete));
				        if(ptransfer_complete != NULL)
				        {
				            error = FAULT_CPE_DOWNLOAD_FAIL_WITHIN_TIME_WINDOW;
							ptransfer_complete->command_key		= strdup(p->command_key);
				            ptransfer_complete->start_time 		= strdup(mix_get_time());
				            ptransfer_complete->complete_time	= strdup(ptransfer_complete->start_time);
				            ptransfer_complete->fault_code		= error;
				            ptransfer_complete->type = TYPE_SCHEDULE_DOWNLOAD;
				            bkp_session_insert_transfer_complete(ptransfer_complete);
				            cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
				        }
				        list_del (&(p->list));
				        if(p->timewindowstruct[0].windowstart != 0)
				            count_download_queue--;
				        cwmp_free_schedule_download_request(p);
				        pthread_mutex_unlock (&mutex_schedule_download);
				        continue;
					}
				}
				else
				{
					if(p->timewindowstruct[0].windowend >= current_time )
					{
						if(p->timewindowstruct[0].windowstart < min_time)
						{
							min_time = p->timewindowstruct[0].windowstart;
							current_download = p;
						}
				
					}
					else if (p->timewindowstruct[1].windowend >= current_time)
					{
						if(p->timewindowstruct[1].windowstart < min_time)
						{
							min_time = p->timewindowstruct[1].windowstart;
							current_download = p;
						}
					}
					else
					{
						pthread_mutex_lock (&mutex_schedule_download);
				        bkp_session_delete_schedule_download(p);
				        ptransfer_complete = calloc (1,sizeof(struct transfer_complete));
				        if(ptransfer_complete != NULL)
				        {
				            error = FAULT_CPE_DOWNLOAD_FAIL_WITHIN_TIME_WINDOW;
							ptransfer_complete->command_key		= strdup(p->command_key);
				            ptransfer_complete->start_time 		= strdup(mix_get_time());
				            ptransfer_complete->complete_time	= strdup(ptransfer_complete->start_time);
				            ptransfer_complete->fault_code		= error;
				            ptransfer_complete->type = TYPE_SCHEDULE_DOWNLOAD;
				            bkp_session_insert_transfer_complete(ptransfer_complete);
				            cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
				        }
				        list_del (&(p->list));
				        if(p->timewindowstruct[0].windowstart != 0)
				            count_download_queue--;
				        cwmp_free_schedule_download_request(p);
				        pthread_mutex_unlock (&mutex_schedule_download);
						continue;
					}
				}
			}
		}
		else
		{
			pthread_mutex_lock (&mutex_schedule_download);
			pthread_cond_wait(&threshold_schedule_download, &mutex_schedule_download);
			pthread_mutex_unlock (&mutex_schedule_download);
		}
		if (min_time == 0)
		{
			continue;
		}
		else if (min_time <= current_time) {
			if ( (min_time == current_download->timewindowstruct[0].windowstart && (current_download->timewindowstruct[0].windowmode)[0] == '2') ||  (min_time == current_download->timewindowstruct[1].windowstart && (current_download->timewindowstruct[1].windowmode)[0] == '2') )
			{
			pthread_mutex_lock (&mutex_schedule_download);
			external_init();
            ptransfer_complete = calloc (1,sizeof(struct transfer_complete));
			ptransfer_complete->type					= TYPE_SCHEDULE_DOWNLOAD;
            error = cwmp_launch_schedule_download(current_download, &ptransfer_complete);
			if(error != FAULT_CPE_NO_FAULT)
            {				
	            bkp_session_insert_transfer_complete(ptransfer_complete);
                bkp_session_save();
                cwmp_root_cause_TransferComplete (cwmp, ptransfer_complete);
                bkp_session_delete_transfer_complete(ptransfer_complete);
            }
            else
            {
				external_exit();
				pthread_mutex_unlock (&mutex_schedule_download);
				if(pthread_mutex_trylock(&(cwmp->mutex_session_send)) == 0)
				{
					pthread_mutex_lock (&mutex_apply_schedule_download);
					pthread_mutex_lock (&mutex_schedule_download);
				    external_init();
					if (current_download->file_type[0] == '1') {
				    	ptransfer_complete->old_software_version = cwmp->deviceid.softwareversion;
				    }
				    bkp_session_insert_transfer_complete(ptransfer_complete);
				    bkp_session_save();
				    external_apply("download", current_download->file_type, current_download->timewindowstruct[0].windowstart);
					external_handle_action(cwmp_handle_downloadFault);
				    external_fetch_downloadFaultResp(&fault_code);
				    if(fault_code != NULL)
				    {
				        if(fault_code[0]=='9')
				        {
				            for(i=1;i<__FAULT_CPE_MAX;i++)
				            {
				                if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
				                {
				                    error = i;
				                    break;
				                }
				            }
				        }
				        free(fault_code);
				        if((error == FAULT_CPE_NO_FAULT) &&
				            (current_download->file_type[0] == '1' || current_download->file_type[0] == '3'))
				        {
				            exit(EXIT_SUCCESS);
				        }
				        bkp_session_delete_transfer_complete(ptransfer_complete);
				        ptransfer_complete->fault_code = error;
				        bkp_session_insert_transfer_complete(ptransfer_complete);
				        bkp_session_save();
				        cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
				    }
					external_exit();
				    pthread_mutex_unlock (&mutex_schedule_download);
					pthread_mutex_unlock (&mutex_apply_schedule_download);
				    pthread_mutex_unlock(&(cwmp->mutex_session_send));
				    pthread_cond_signal(&(cwmp->threshold_session_send));
				}
				else 
				{
					cwmp_add_apply_schedule_download(current_download, ptransfer_complete->start_time);
				}
			}
			pthread_mutex_lock (&mutex_schedule_download);
			bkp_session_delete_schedule_download(current_download);
			bkp_session_save();			
			list_del(&(current_download->list));
			cwmp_free_schedule_download_request(current_download);
			pthread_mutex_unlock (&mutex_schedule_download);
			min_time = 0;
			current_download = NULL;
			continue;
		}//AT ANY TIME OR WHEN IDLE
		else 
		{			
				pthread_mutex_lock (&(cwmp->mutex_session_send));
                external_init();
                CWMP_LOG(INFO,"Launch download file %s",current_download->url);
                error = cwmp_launch_schedule_download(current_download,&ptransfer_complete);
                if(error != FAULT_CPE_NO_FAULT)
                {
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    bkp_session_save();
                    cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                    bkp_session_delete_transfer_complete(ptransfer_complete);
                }
                else
                {
                    if (current_download->file_type[0] == '1') {
                    	ptransfer_complete->old_software_version = cwmp->deviceid.softwareversion;
                    }
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    bkp_session_save();
                    external_apply("download", current_download->file_type, 0);
                    external_handle_action(cwmp_handle_downloadFault);
                    external_fetch_downloadFaultResp(&fault_code);
                    if(fault_code != NULL)
                    {
                        if(fault_code[0]=='9')
                        {
                            for(i=1;i<__FAULT_CPE_MAX;i++)
                            {
                                if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
                                {
                                    error = i;
                                    break;
                                }
                            }
                        }
                        free(fault_code);
                        if((error == FAULT_CPE_NO_FAULT) &&
                            (current_download->file_type[0] == '1' || current_download->file_type[0] == '3'))
                        {
                            exit(EXIT_SUCCESS);
                        }
                        bkp_session_delete_transfer_complete(ptransfer_complete);
                        ptransfer_complete->fault_code = error;
                        bkp_session_insert_transfer_complete(ptransfer_complete);
                        bkp_session_save();
                        cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                    }
                }
                external_exit();
                pthread_mutex_unlock (&(cwmp->mutex_session_send));
                pthread_cond_signal (&(cwmp->threshold_session_send));
                pthread_mutex_lock (&mutex_schedule_download);
                list_del (&(current_download->list));
                if(current_download->timewindowstruct[0].windowstart != 0)
                    count_download_queue--;
                cwmp_free_schedule_download_request(current_download);
                pthread_mutex_unlock (&mutex_schedule_download);
                continue;
		}
		}
		else
		{
			if (min_time == current_download->timewindowstruct[0].windowstart)
			{
				pthread_mutex_lock (&mutex_schedule_download);
				download_timeout.tv_sec = min_time;
				pthread_cond_timedwait(&threshold_schedule_download, &mutex_schedule_download, &download_timeout);
				pthread_mutex_unlock (&mutex_schedule_download);
			}
			else if (min_time == current_download->timewindowstruct[1].windowstart)
			{
				pthread_mutex_lock (&mutex_schedule_download);
				download_timeout.tv_sec = min_time;
				pthread_cond_timedwait(&threshold_schedule_download, &mutex_schedule_download, &download_timeout);
				pthread_mutex_unlock (&mutex_schedule_download);
			}
		}
        
    }
    return NULL;
}

void *thread_cwmp_rpc_cpe_apply_schedule_download (void *v)
{
    struct cwmp                     			*cwmp = (struct cwmp *)v;
    struct schedule_download          			*pschedule_download;
    struct timespec                 			apply_timeout = {0, 0};
    time_t                          			current_time, stime;
    int											i,error = FAULT_CPE_NO_FAULT;
    struct transfer_complete					*ptransfer_complete;
    long int									timeout;
    char										*fault_code;
	int min_time = 0;
	struct apply_schedule_download *apply_download = NULL;
	struct apply_schedule_download *p, *_p;
	for(;;)
    {
		current_time = time(NULL);
		if(list_apply_schedule_download.next != &(list_apply_schedule_download))
		{
			list_for_each_entry_safe(p, _p, &(list_apply_schedule_download), list)
			{
				if (min_time == 0)
				{
					if (p->timeintervals[0].windowend >= current_time)
					{
						min_time = p->timeintervals[0].windowstart;
						apply_download = p;
					}
					else if (p->timeintervals[1].windowend >= current_time)
					{
						min_time = p->timeintervals[1].windowstart;
						apply_download = p;
					}
					else 
					{
						pthread_mutex_lock (&mutex_apply_schedule_download);
				        bkp_session_delete_apply_schedule_download(p);
				        ptransfer_complete = calloc (1,sizeof(struct transfer_complete));
				        if(ptransfer_complete != NULL)
				        {
				            error = FAULT_CPE_DOWNLOAD_FAIL_WITHIN_TIME_WINDOW;
							ptransfer_complete->command_key		= strdup(p->command_key);
				            ptransfer_complete->start_time 		= strdup(mix_get_time());
				            ptransfer_complete->complete_time	= strdup(ptransfer_complete->start_time);
				            ptransfer_complete->fault_code		= error;
				            ptransfer_complete->type = TYPE_SCHEDULE_DOWNLOAD;
				            bkp_session_insert_transfer_complete(ptransfer_complete);
				            cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
				        }
				        list_del (&(p->list));
				        if(p->timeintervals[0].windowstart != 0)
				            count_download_queue--;
				        cwmp_free_apply_schedule_download_request(p);
				        pthread_mutex_unlock (&mutex_apply_schedule_download);
				        continue;
					}
				}
				else
				{
					if(p->timeintervals[0].windowend >= current_time )
					{
						if(p->timeintervals[0].windowstart < min_time)
						{
							min_time = p->timeintervals[0].windowstart;
							apply_download = p;
						}
				
					}
					else if (p->timeintervals[1].windowend >= current_time)
					{
						if(p->timeintervals[1].windowstart < min_time)
						{
							min_time = p->timeintervals[1].windowstart;
							apply_download = p;
						}
					}
					else
					{
						pthread_mutex_lock (&mutex_apply_schedule_download);
				        bkp_session_delete_apply_schedule_download(p);
				        ptransfer_complete = calloc (1,sizeof(struct transfer_complete));
				        if(ptransfer_complete != NULL)
				        {
				            error = FAULT_CPE_DOWNLOAD_FAIL_WITHIN_TIME_WINDOW;
							ptransfer_complete->command_key		= strdup(p->command_key);
				            ptransfer_complete->start_time 		= strdup(mix_get_time());
				            ptransfer_complete->complete_time	= strdup(ptransfer_complete->start_time);
				            ptransfer_complete->fault_code		= error;
				            ptransfer_complete->type = TYPE_SCHEDULE_DOWNLOAD;
				            bkp_session_insert_transfer_complete(ptransfer_complete);
				            cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
				        }
				        list_del (&(p->list));
				        /*if(p->timewindowintervals[0].windowstart != 0)
				            count_download_queue--;*/
				        cwmp_free_apply_schedule_download_request(p);
				        pthread_mutex_unlock (&mutex_apply_schedule_download);
						continue;
					}
				}
			}
		}
		else
		{
			pthread_mutex_lock (&mutex_apply_schedule_download);
			pthread_cond_wait(&threshold_apply_schedule_download, &mutex_apply_schedule_download);
			pthread_mutex_unlock (&mutex_apply_schedule_download);
		}
		if (min_time == 0)
		{
			continue;
		}
		else if (min_time <= current_time ) 
		{	
			pthread_mutex_lock (&(cwmp->mutex_session_send));
			pthread_mutex_lock (&mutex_schedule_download);
			external_init();
			bkp_session_delete_apply_schedule_download(apply_download);
			bkp_session_save();
			ptransfer_complete = calloc (1,sizeof(struct transfer_complete));	
			if (apply_download->file_type[0] == '1') {
            	ptransfer_complete->old_software_version = cwmp->deviceid.softwareversion;
            }
			ptransfer_complete->command_key		= strdup(apply_download->command_key);
			ptransfer_complete->start_time 		= strdup(apply_download->start_time);
		    ptransfer_complete->complete_time	= strdup(mix_get_time());
			ptransfer_complete->fault_code		= error;
			ptransfer_complete->type = TYPE_SCHEDULE_DOWNLOAD;
            bkp_session_insert_transfer_complete(ptransfer_complete);
            bkp_session_save();
            external_apply("download", apply_download->file_type, apply_download->timeintervals[0].windowstart);
			external_handle_action(cwmp_handle_downloadFault);
            external_fetch_downloadFaultResp(&fault_code);
            if(fault_code != NULL)
            {
                if(fault_code[0]=='9')
                {
                    for(i=1;i<__FAULT_CPE_MAX;i++)
                    {
                        if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
                        {
                            error = i;
                            break;
                        }
                    }
                }
                free(fault_code);
                if((error == FAULT_CPE_NO_FAULT) &&
                    (apply_download->file_type[0] == '1' || apply_download->file_type[0] == '3'))
                {
                    exit(EXIT_SUCCESS);
                }
                bkp_session_delete_transfer_complete(ptransfer_complete);
                ptransfer_complete->fault_code = error;
                bkp_session_insert_transfer_complete(ptransfer_complete);
                bkp_session_save();
                cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
            }
			external_exit();
			pthread_mutex_unlock (&mutex_schedule_download);
            pthread_mutex_unlock (&(cwmp->mutex_session_send));
            pthread_cond_signal (&(cwmp->threshold_session_send));
            pthread_mutex_lock (&mutex_apply_schedule_download);
            list_del (&(apply_download->list));
            /*if(pdownload->timeintervals[0].windowstart != 0)
                count_download_queue--;*/
            cwmp_free_apply_schedule_download_request(apply_download);
            pthread_mutex_unlock (&mutex_apply_schedule_download);
            continue;
		}
		else
		{
			if (min_time == apply_download->timeintervals[0].windowstart)
			{
				pthread_mutex_lock (&mutex_apply_schedule_download);
				apply_timeout.tv_sec = min_time;
				pthread_cond_timedwait(&threshold_apply_schedule_download, &mutex_schedule_download, &apply_timeout);
				pthread_mutex_unlock (&mutex_apply_schedule_download);				
			}
			else if (min_time == apply_download->timeintervals[1].windowstart)
			{
				pthread_mutex_lock (&mutex_apply_schedule_download);
				apply_timeout.tv_sec = min_time;
				pthread_cond_timedwait(&threshold_schedule_download, &mutex_schedule_download, &apply_timeout);
				pthread_mutex_unlock (&mutex_schedule_download);				
			}
		}
        
    }
	return NULL;
}

void *thread_cwmp_rpc_cpe_change_du_state (void *v)
{
    struct cwmp                     			*cwmp = (struct cwmp *)v;
    struct change_du_state          			*pchange_du_state;
    struct timespec                 			download_timeout = {50, 0};
    time_t                          			current_time, stime;
    int											i,error = FAULT_CPE_NO_FAULT;
    struct du_state_change_complete				*pdu_state_change_complete;
    long int									time_of_grace = 216000,timeout;
    char										*fault_code;
	char										*package_version;
	char										*package_name;
	struct operations							*p, *q;
	struct opresult								*res;
	char 										*operation_endTime;
	char 										*du_instance;
	struct dmctx dmctx = {0};
	char du_ref[16];
	char *cur_uuid = NULL;
	char *cur_url = NULL;
	char *cur_user = NULL;
	char *cur_pass = NULL;
	char *cur_instance = NULL;
	char *cur_name = NULL;
	char *cur_version = NULL;
	int uuid;
    
    for(;;)
    {
        if (list_change_du_state.next!=&(list_change_du_state)) {
            pchange_du_state = list_entry(list_change_du_state.next,struct change_du_state, list);
			
            stime = pchange_du_state->timeout;
            current_time    = time(NULL);
            timeout = current_time - pchange_du_state->timeout;
            if((timeout >= 0)&&(timeout > time_of_grace))
            {
                pthread_mutex_lock (&mutex_change_du_state);
				pdu_state_change_complete = calloc (1,sizeof(struct du_state_change_complete));
				if (pdu_state_change_complete != NULL)
				{
					error = FAULT_CPE_DOWNLOAD_FAILURE; //TO UPDATE
					INIT_LIST_HEAD(&(pdu_state_change_complete->list_opresult));
					pdu_state_change_complete->command_key = strdup(pchange_du_state->command_key);
					pdu_state_change_complete->timeout = pchange_du_state->timeout;
					list_for_each_entry_safe(p, q, &pchange_du_state->list_operation, list) {
						res = calloc(1, sizeof(struct opresult));
						list_add_tail(&(res->list), &(pdu_state_change_complete->list_opresult));					
						res->uuid = strdup(p->uuid);
						res->version = strdup(p->version);
						res->current_state = strdup("Failed");
						res->start_time = strdup(mix_get_time());
						res->complete_time = strdup(res->start_time);
						res->fault		= error;					
					}				
					bkp_session_insert_du_state_change_complete(pdu_state_change_complete);
					cwmp_root_cause_dustatechangeComplete (cwmp,pdu_state_change_complete);
				}
				list_del (&(pchange_du_state->list));				
				cwmp_free_change_du_state_request(pchange_du_state);
				pthread_mutex_unlock (&mutex_change_du_state);
				continue;
			}
            if((timeout >= 0)&&(timeout <= time_of_grace))
            {
                pthread_mutex_lock (&(cwmp->mutex_session_send));
				pdu_state_change_complete = calloc (1,sizeof(struct du_state_change_complete));
				if (pdu_state_change_complete != NULL)
				{
					error = FAULT_CPE_DOWNLOAD_FAILURE; //TO UPDATE
					INIT_LIST_HEAD(&(pdu_state_change_complete->list_opresult));
					pdu_state_change_complete->command_key = strdup(pchange_du_state->command_key);
					pdu_state_change_complete->timeout = pchange_du_state->timeout;
					list_for_each_entry_safe(p, q, &pchange_du_state->list_operation, list) {
						res = calloc(1, sizeof(struct opresult)); //IF RES IS NULL
						list_add_tail(&(res->list), &(pdu_state_change_complete->list_opresult));					
						external_init();						
						switch (p->type)
				        {
				            case DU_INSTALL:
								error = cwmp_launch_change_du_state_download(p,&res);
								if(error != FAULT_CPE_NO_FAULT)
				                {
									res->uuid = strdup(p->uuid);
									res->du_ref = strdup("");
									res->version = strdup("");
									res->current_state = strdup("Failed");
									res->resolved = 0;									
									operation_endTime = mix_get_time(); //TO CHECK
									res->complete_time = strdup(operation_endTime);
									res->fault = error;
									break;
								}
								external_apply("du_download", "install", 0);
								external_handle_action(cwmp_handle_dustate_changeFault);
								external_fetch_du_change_stateFaultResp(&fault_code, &package_version, &package_name);
								if(fault_code != NULL)
								{
									if(fault_code[0]=='9')
									{
										for(i=1;i<__FAULT_CPE_MAX;i++)
										{
											if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
											{
												error = i;
												break;
											}
										}
									}
								}
								free(fault_code);
								if( error == FAULT_CPE_NO_FAULT)
								{
									dm_ctx_init(&dmctx);
									du_instance = strdup(add_softwaremodules_deploymentunit(p->uuid, p->url, p->username, p->password, package_name, package_version));
									dmuci_commit();
									dm_ctx_clean(&dmctx);
									sprintf(du_ref,DMROOT"SoftwareModules.%s.", du_instance);
									res->uuid = strdup(p->uuid);
									res->du_ref = strdup(du_ref);
									res->current_state = strdup("Installed");
									res->resolved = 1;
									res->version = strdup(package_version);
									operation_endTime = mix_get_time();
									res->complete_time = strdup(operation_endTime);
									res->fault = error;									
								}
								else
								{									
									res->uuid = strdup(p->uuid);
									res->current_state = strdup("Failed");
									res->resolved = 0;									
									operation_endTime = mix_get_time(); //TO CHECK
									res->complete_time = strdup(operation_endTime);
									res->fault = error;								
								}
								external_exit();
								break;
							case DU_UNINSTALL:
								dm_ctx_init(&dmctx);
								cur_name = strdup(get_softwaremodules_name(p->uuid));
								if (!cur_name || cur_name[0] == '\0') {
									error = FAULT_CPE_UNKNOWN_DEPLOYMENT_UNIT;
									res->fault = error;
									dm_ctx_clean(&dmctx);
									break;												
								}
								if((p->version)[0] == '\0') {
									error = cwmp_launch_uninstall_du_state(cur_name, &res);																
								}
								else {
									cur_version = strdup(get_softwaremodules_version(p->uuid));
									if(strcmp(cur_version, p->version) == 0)
										error = cwmp_launch_uninstall_du_state(cur_name, &res);
									else {
										error = FAULT_CPE_UNKNOWN_DEPLOYMENT_UNIT;
										dm_ctx_clean(&dmctx);
										break;
									}
								}
								if( error == FAULT_CPE_NO_FAULT)
								{
									dm_ctx_init(&dmctx);
									du_instance = get_softwaremodules_instance(p->uuid);
									sprintf(du_ref,DMROOT"SoftwareModules.%s.", du_instance);
									res->uuid = strdup(p->uuid);
									res->du_ref = strdup(du_ref);
									FREE(du_instance);
									res->version = strdup(p->version);
									res->current_state = strdup("Uninstalled");
									res->resolved = 1;
									operation_endTime = mix_get_time();
									res->complete_time = strdup(operation_endTime);
									res->fault = error;
									dm_ctx_clean(&dmctx);								
								}
								else
								{
									res->uuid = strdup(p->uuid);
									du_instance = get_softwaremodules_instance(p->uuid);
									sprintf(du_ref,DMROOT"SoftwareModules.%s.", du_instance);
									res->du_ref = strdup(du_ref);
									FREE(du_instance);
									res->version = strdup(p->version);
									res->current_state = strdup("Installed");
									res->resolved = 1;
									operation_endTime = mix_get_time(); //TO CHECK
									res->complete_time = strdup(operation_endTime);
									res->fault = error;
									dm_ctx_clean(&dmctx);							
								}
								external_exit();
								break;
							case DU_UPDATE:
								if ((p->url)[0] != '\0' && (p->uuid)[0] != '\0') {
									dm_ctx_init(&dmctx);
									uuid = update_softwaremodules_url(p->uuid, p->url);
									if(uuid == 0) {
										error = FAULT_CPE_UNKNOWN_DEPLOYMENT_UNIT;
										dm_ctx_clean(&dmctx);
										break;
									}
									cur_instance = strdup(get_softwaremodules_instance(p->uuid));
									dmuci_commit();
									dm_ctx_clean(&dmctx);					 										
								}
								else if ((p->url)[0] != '\0' && (p->uuid)[0] == '\0') {
									dm_ctx_init(&dmctx);
									cur_uuid = get_softwaremodules_uuid(p->url);
									if (!cur_uuid || cur_uuid[0] == '\0')
									{
										error = FAULT_CPE_UNKNOWN_DEPLOYMENT_UNIT;
										dm_ctx_clean(&dmctx);
										break;
									}
									cur_instance = strdup(get_softwaremodules_instance(p->uuid));
									
								}
								else if ((p->url)[0] == '\0' && (p->uuid)[0] != '\0') {
									dm_ctx_init(&dmctx);
									cur_url = strdup(get_softwaremodules_url(p->uuid));
									if (cur_url == NULL || cur_url[0] == '\0')
									{
										error = FAULT_CPE_UNKNOWN_DEPLOYMENT_UNIT;
										dm_ctx_clean(&dmctx);
										break;
									}
									cur_user = strdup(get_softwaremodules_username(p->uuid));
									cur_pass = strdup(get_softwaremodules_pass(p->uuid));
									dm_ctx_clean(&dmctx);
								}								
								error = cwmp_launch_update_du_state_download((cur_url && cur_url[0]!='\0' )?cur_url:p->url, p->username, p->password, &res);
								if(error != FAULT_CPE_NO_FAULT)
				                {
									if (p->uuid == NULL || (p->uuid)[0] == '\0') {
										res->uuid = strdup(cur_uuid);
										FREE(cur_uuid);
									}
									else
										res->uuid = strdup(p->uuid);
									sprintf(du_ref,DMROOT"SoftwareModules.%s.", du_instance);
									res->du_ref = strdup(du_ref);
									FREE(du_instance);
									res->current_state = strdup("Installed");
									res->complete_time = strdup("0001-01-01T00:00:00+00:00");//TO CHECK
									res->fault = error;
									// FREE(cur_user); FREE(cur_pass); 
									break;
								}						
								if(error != FAULT_CPE_NO_FAULT)
				                {
									if (p->uuid == NULL || (p->uuid)[0] == '\0') {
										res->uuid = strdup(cur_uuid);
										FREE(cur_uuid);
									}
									else
										res->uuid = strdup(p->uuid);
									sprintf(du_ref,DMROOT"SoftwareModules.%s.", du_instance);
									res->du_ref = strdup(du_ref);
									FREE(du_instance);
									res->current_state = strdup("Failed");
									operation_endTime = mix_get_time(); //TO CHECK
									res->complete_time = strdup(operation_endTime);
									res->fault = error;
									// FREE(cur_user); FREE(cur_pass); 
									break;
									break;
								}
								external_apply("du_download", "update", 0);
								external_handle_action(cwmp_handle_dustate_changeFault);
								external_fetch_du_change_stateFaultResp(&fault_code, &package_version, &package_name);
								if(fault_code != NULL)
								{
									if(fault_code[0]=='9')
									{
										for(i=1;i<__FAULT_CPE_MAX;i++)
										{
											if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
											{
												error = i;
												break;
											}
										}
									}
								}
								free(fault_code);
								sprintf(du_ref,DMROOT"SoftwareModules.%s.", du_instance);
								if (p->uuid == NULL || (p->uuid)[0] == '\0') {
									res->uuid = strdup(cur_url);
									FREE(cur_url);
								}
								else
									res->uuid = strdup(p->uuid);
								res->du_ref = strdup(du_ref);
								FREE(du_instance);
								res->current_state = strdup("Installed");
								res->resolved = 1; //TO CHECK
								operation_endTime = mix_get_time();
								res->complete_time = strdup(operation_endTime);
								res->fault = error;									
								external_exit();
								break;
						}												
					}
					bkp_session_delete_change_du_state(pchange_du_state);
					bkp_session_save();
					bkp_session_insert_du_state_change_complete(pdu_state_change_complete);
					bkp_session_save();
					cwmp_root_cause_dustatechangeComplete (cwmp,pdu_state_change_complete);
					//bkp_session_delete_du_state_change_complete(pdu_state_change_complete); //TO CHECK
					//bkp_session_save(); //TO CHECK
				}
			}
            pthread_mutex_lock (&mutex_change_du_state);
		    pthread_cond_timedwait(&threshold_change_du_state, &mutex_change_du_state, &download_timeout);

		    pthread_mutex_unlock (&mutex_change_du_state);
			pthread_mutex_unlock (&(cwmp->mutex_session_send));
                pthread_cond_signal (&(cwmp->threshold_session_send));
                pthread_mutex_lock (&mutex_change_du_state);
                list_del (&(pchange_du_state->list));
                cwmp_free_change_du_state_request(pchange_du_state);
                pthread_mutex_unlock (&mutex_change_du_state);
                continue;
		} else {
			pthread_mutex_lock (&mutex_change_du_state);
			pthread_cond_wait(&threshold_change_du_state, &mutex_change_du_state);
			pthread_mutex_unlock (&mutex_change_du_state);
		}		
    }
    return NULL;
}

int cwmp_launch_uninstall_du_state(char *package_name, struct opresult **pchange_du_state_complete) {
	
	int							i, error = FAULT_CPE_NO_FAULT;
    char						*uninstall_startTime;
   char						*fault_code;
	
	uninstall_startTime = mix_get_time();
	external_change_du_state_uninstall(package_name);
	external_handle_action(cwmp_handle_uninstallFault);
	external_fetch_uninstallFaultResp(&fault_code);
	if(fault_code != NULL)
	{
		if(fault_code[0]=='9')
		{
			for(i=1;i<__FAULT_CPE_MAX;i++)
			{
				if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
				{
					error = i;
					break;
				}
			}
		}
		free(fault_code);
	}
	(*pchange_du_state_complete)->start_time = strdup(uninstall_startTime);
	return error;
}

int cwmp_launch_update_du_state_download(char *url, char *user, char *pass, struct opresult **pchange_du_state_complete) {
	
	int							i, error = FAULT_CPE_NO_FAULT;
    char						*update_startTime;
    char						*fault_code;
	char						*package_version;
	char						*package_name;

	update_startTime = mix_get_time();
	external_change_du_state_download(url, user, pass);
	external_handle_action(cwmp_handle_dustate_changeFault);
	external_fetch_du_change_stateFaultResp(&fault_code, &package_version, &package_name);
	if(fault_code != NULL)
	{
		if(fault_code[0]=='9')
		{
			for(i=1;i<__FAULT_CPE_MAX;i++)
			{
				if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
				{
					error = i;
					break;
				}
			}
		}
		free(fault_code);
	}
	(*pchange_du_state_complete)->start_time = strdup(update_startTime);
	return error;
}


int cwmp_launch_change_du_state_download(struct operations *poperations, struct opresult **pchange_du_state_complete)
{
    int							i, error = FAULT_CPE_NO_FAULT;
    char						*download_startTime;
    struct opresult				*p;
    char						*fault_code;
	char						*package_version;
	char						*package_name;

    download_startTime = mix_get_time();

	external_change_du_state_download(poperations->url, poperations->username, poperations->password);
	external_handle_action(cwmp_handle_dustate_changeFault);
	external_fetch_du_change_stateFaultResp(&fault_code, &package_version, &package_name);
	if(fault_code != NULL)
	{
		if(fault_code[0]=='9')
		{
			for(i=1;i<__FAULT_CPE_MAX;i++)
			{
				if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
				{
					error = i;
					break;
				}
			}
		}
		free(fault_code);
	}
	p->start_time = strdup(download_startTime);
	if(error != FAULT_CPE_NO_FAULT)
	{
		p->fault 		= error;
	}

	(*pchange_du_state_complete)->start_time = strdup(download_startTime);

    return error;
}

void *thread_cwmp_rpc_cpe_upload (void *v)
{
    struct cwmp                     			*cwmp = (struct cwmp *)v;
    struct upload          					*pupload;
    struct timespec                 			upload_timeout = {0, 0};
    time_t                          			current_time, stime;
    int											i,error = FAULT_CPE_NO_FAULT;
    struct transfer_complete					*ptransfer_complete;
    long int									time_of_grace = 3600,timeout;
    char										*fault_code;

    for(;;)
    {
        if (list_upload.next!=&(list_upload)) {
            pupload = list_entry(list_upload.next,struct upload, list);
            stime = pupload->scheduled_time;
            current_time    = time(NULL);
            if(pupload->scheduled_time != 0)
                timeout = current_time - pupload->scheduled_time;
            else
                timeout = 0;
            if((timeout >= 0)&&(timeout > time_of_grace))
            {
                pthread_mutex_lock (&mutex_upload);
                bkp_session_delete_upload(pupload);
                ptransfer_complete = calloc (1,sizeof(struct transfer_complete));
                if(ptransfer_complete != NULL)
                {
                    error = FAULT_CPE_DOWNLOAD_FAILURE;

                    ptransfer_complete->command_key		= strdup(pupload->command_key);
                    ptransfer_complete->start_time 		= strdup(mix_get_time());
                    ptransfer_complete->complete_time	= strdup(ptransfer_complete->start_time);
                    ptransfer_complete->fault_code		= error;
                    ptransfer_complete->type			= TYPE_UPLOAD;
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                }
                list_del (&(pupload->list));
                if(pupload->scheduled_time != 0)
                    count_download_queue--;
                cwmp_free_upload_request(pupload);
                pthread_mutex_unlock (&mutex_download);
                continue;
            }
            if((timeout >= 0)&&(timeout <= time_of_grace))
            {
                pthread_mutex_lock (&(cwmp->mutex_session_send));
                external_init();
                CWMP_LOG(INFO,"Launch upload file %s",pupload->url);
                error = cwmp_launch_upload(pupload,&ptransfer_complete);
                if(error != FAULT_CPE_NO_FAULT)
                {
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    bkp_session_save();
                    cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                    bkp_session_delete_transfer_complete(ptransfer_complete);
                }
                else
                {
                    bkp_session_insert_transfer_complete(ptransfer_complete);
                    bkp_session_save();
                    //external_apply("download", pdownload->file_type); !!!
                    external_handle_action(cwmp_handle_uploadFault); //IBH TO ADD
                    external_fetch_uploadFaultResp(&fault_code); //IBH TO ADD
                    if(fault_code != NULL)
                    {
                        if(fault_code[0]=='9')
                        {
                            for(i=1;i<__FAULT_CPE_MAX;i++)
                            {
                                if(strcmp(FAULT_CPE_ARRAY[i].CODE,fault_code) == 0)
                                {
                                    error = i;
                                    break;
                                }
                            }
                        }
                        free(fault_code);
                        if((error == FAULT_CPE_NO_FAULT) &&
                            (pupload->file_type[0] == '1' || pupload->file_type[0] == '3')) //IBH TO ADD
                        {
                            exit(EXIT_SUCCESS);
                        }
                        bkp_session_delete_transfer_complete(ptransfer_complete);
                        ptransfer_complete->fault_code = error;
                        bkp_session_insert_transfer_complete(ptransfer_complete);
                        bkp_session_save();
                        cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
                    }
                }
                external_exit();
                pthread_mutex_unlock (&(cwmp->mutex_session_send));
                pthread_cond_signal (&(cwmp->threshold_session_send));
                pthread_mutex_lock (&mutex_upload);
                list_del (&(pupload->list));
                if(pupload->scheduled_time != 0)
                    count_download_queue--;
                cwmp_free_upload_request(pupload);
                pthread_mutex_unlock (&mutex_upload);
                continue;
            }
        pthread_mutex_lock (&mutex_upload);
        upload_timeout.tv_sec = stime;
        pthread_cond_timedwait(&threshold_upload, &mutex_upload, &upload_timeout);
        pthread_mutex_unlock (&mutex_upload);
        } else {
            pthread_mutex_lock (&mutex_upload);
            pthread_cond_wait(&threshold_upload, &mutex_upload);
            pthread_mutex_unlock (&mutex_upload);
        }
    }
    return NULL;
}

int cwmp_free_download_request(struct download *download)
{
	if(download != NULL)
	{
		if(download->command_key != NULL)
		{
			free(download->command_key);
		}
		if(download->file_type != NULL)
		{
			free(download->file_type);
		}
		if(download->url != NULL)
		{
			free(download->url);
		}
		if(download->username != NULL)
		{
			free(download->username);
		}
		if(download->password != NULL)
		{
			free(download->password);
		}
		free(download);
	}
	return CWMP_OK;
}

int cwmp_free_schedule_download_request(struct schedule_download *schedule_download)
{
	int i;	
	if(schedule_download != NULL)
	{		
		if(schedule_download->command_key != NULL)
		{
			free(schedule_download->command_key);
		}
		if(schedule_download->file_type != NULL)
		{
			free(schedule_download->file_type);
		}
		
		if(schedule_download->url != NULL)
		{
			free(schedule_download->url);
		}
		
		if(schedule_download->username != NULL)
		{
			free(schedule_download->username);
		}
		
		if(schedule_download->password != NULL)
		{
			free(schedule_download->password);
		}
		for (i = 0; i<=1; i++)
		{
			if(schedule_download->timewindowstruct[i].windowmode != NULL)
			{
				free(schedule_download->timewindowstruct[i].windowmode);				
			}
			if(schedule_download->timewindowstruct[i].usermessage != NULL)
			{
				
				free(schedule_download->timewindowstruct[i].usermessage);				
			}
		}
		free(schedule_download);
		
	}
	return CWMP_OK;
}

int cwmp_free_apply_schedule_download_request(struct apply_schedule_download *apply_schedule_download)
{
	int i;	
	if(apply_schedule_download != NULL)
	{		
		if(apply_schedule_download->command_key != NULL)
		{
			free(apply_schedule_download->command_key);
		}
		if(apply_schedule_download->file_type != NULL)
		{
			free(apply_schedule_download->file_type);
		}
		if(apply_schedule_download->start_time != NULL)
		{
			free(apply_schedule_download->start_time);
		}
		
		free(apply_schedule_download);
		
	}
	return CWMP_OK;
}

int cwmp_free_change_du_state_request(struct change_du_state *change_du_state)
{
	struct list_head *ilist, *q;
	struct operations *operation;
	if(change_du_state != NULL)
	{
		if(change_du_state->command_key != NULL)
		{
			free(change_du_state->command_key);
		}
		list_for_each_safe(ilist, q, &(change_du_state->list_operation)) {
			operation = list_entry(ilist, struct operations, list);	
			if(operation->url != NULL)
			{
				free(operation->url);
			}
			if(operation->uuid != NULL)
			{
				free(operation->uuid);
			}
			if(operation->username != NULL)
			{
				free(operation->username);
			}
			if(operation->password != NULL)
			{
				free(operation->password);
			}
			if(operation->version != NULL)
			{
				free(operation->version);
			}
			if(operation->executionenvref != NULL)
			{
				free(operation->executionenvref);
			}
			list_del(&(operation->list));
        	free (operation);
		}
		free(change_du_state);
	}
	return CWMP_OK;
}

int cwmp_free_upload_request(struct upload *upload)
{
	if(upload != NULL)
	{
		if(upload->command_key != NULL)
		{
			FREE(upload->command_key);
		}
		if(upload->file_type != NULL)
		{
			FREE(upload->file_type);
		}
		if(upload->url != NULL)
		{
			FREE(upload->url);
		}
		if(upload->username != NULL)
		{
			FREE(upload->username);
		}
		if(upload->password != NULL)
		{
			FREE(upload->password);
		}
		if(upload->f_instance != NULL)
		{
			FREE(upload->f_instance);
		}
		FREE(upload);
	}
	return CWMP_OK;
}

int cwmp_scheduledDownload_remove_all()
{
	struct download	*download;

	pthread_mutex_lock (&mutex_download);
	while (list_download.next!=&(list_download))
	{
		download = list_entry(list_download.next,struct download, list);
		list_del (&(download->list));
		bkp_session_delete_download(download);
		if(download->scheduled_time != 0)
			count_download_queue--;
		cwmp_free_download_request(download);
	}
	pthread_mutex_unlock (&mutex_download);

	return CWMP_OK;
}

int cwmp_scheduledUpload_remove_all()
{
	struct upload	*upload;

	pthread_mutex_lock (&mutex_upload);
	while (list_upload.next!=&(list_upload))
	{
		upload = list_entry(list_upload.next,struct upload, list);
		list_del (&(upload->list));
		bkp_session_delete_upload(upload);
		if(upload->scheduled_time != 0)
			count_download_queue--;
		cwmp_free_upload_request(upload);
	}
	pthread_mutex_unlock (&mutex_upload);

	return CWMP_OK;
}

int cwmp_scheduled_Download_remove_all()
{
	struct schedule_download	*schedule_download;

	pthread_mutex_lock (&mutex_schedule_download);
	while (list_schedule_download.next!=&(list_schedule_download))
	{
		schedule_download = list_entry(list_schedule_download.next,struct schedule_download, list);
		list_del (&(schedule_download->list));
		bkp_session_delete_schedule_download(schedule_download);
		if(schedule_download->timewindowstruct[0].windowstart != 0)
			count_download_queue--;
		cwmp_free_schedule_download_request(schedule_download);
	}
	pthread_mutex_unlock (&mutex_schedule_download);

	return CWMP_OK;
}

int cwmp_apply_scheduled_Download_remove_all()
{
	struct apply_schedule_download	*apply_schedule_download;

	pthread_mutex_lock (&mutex_apply_schedule_download);
	while (list_apply_schedule_download.next!=&(list_apply_schedule_download))
	{
		apply_schedule_download = list_entry(list_apply_schedule_download.next,struct apply_schedule_download, list);
		list_del (&(apply_schedule_download->list));
		bkp_session_delete_apply_schedule_download(apply_schedule_download);
		/*if(apply_schedule_download->timetimeintervals[0].windowstart != 0)
			count_download_queue--;*/ //TOCK
		cwmp_free_apply_schedule_download_request(apply_schedule_download);
	}
	pthread_mutex_unlock (&mutex_apply_schedule_download);

	return CWMP_OK;
}

int cwmp_add_apply_schedule_download(struct schedule_download *schedule_download, char *start_time)
{
	int i = 0;
	int							error = FAULT_CPE_NO_FAULT;
	struct apply_schedule_download 			*apply_schedule_download,*iapply_schedule_download;
	
	struct transfer_complete 	*ptransfer_complete;
	struct list_head    		*ilist;

	
	apply_schedule_download = calloc (1,sizeof(struct apply_schedule_download));
	if (apply_schedule_download == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}
	if(error == FAULT_CPE_NO_FAULT)
	{
		pthread_mutex_lock (&mutex_apply_schedule_download);
		apply_schedule_download->command_key = strdup(schedule_download->command_key);
		apply_schedule_download->file_type = strdup(schedule_download->file_type);
		apply_schedule_download->start_time = strdup(start_time);
		for (i = 0; i < 2; i++)
		{	
			apply_schedule_download->timeintervals[i].windowstart = schedule_download->timewindowstruct[i].windowstart;
			apply_schedule_download->timeintervals[i].windowend = schedule_download->timewindowstruct[i].windowend;
			apply_schedule_download->timeintervals[i].maxretries = schedule_download->timewindowstruct[i].maxretries;
		}
		list_add_tail (&(apply_schedule_download->list), &(list_apply_schedule_download));
			
		bkp_session_insert_apply_schedule_download(apply_schedule_download);
		bkp_session_save();		
		pthread_mutex_unlock (&mutex_apply_schedule_download);
		pthread_cond_signal(&threshold_apply_schedule_download);
	}
	return 0;
fault:
    cwmp_free_apply_schedule_download_request(apply_schedule_download);
	/*if (cwmp_create_fault_message(session, rpc, error))
		goto error;*/ //TOCK
	return 0;

error:
	return -1;
}

int cwmp_handle_rpc_cpe_change_du_state(struct session *session, struct rpc *rpc)
{
	mxml_node_t 				*tmp1, *n, *t, *b = session->body_in;
	pthread_t           			change_du_state_thread;
	char 						*c, *tmp, *file_type = NULL;
	int							error = FAULT_CPE_NO_FAULT;
	struct change_du_state 			*change_du_state,*ichange_du_state;
	struct transfer_complete 	*ptransfer_complete;
	struct list_head    		*ilist, *ilist1;
	time_t             			scheduled_time;
	time_t 						download_delay;
	bool                		cond_signal = false;
	int 				i = 0;
	struct operations *elem;
	struct installop *p;
	struct uninstallop *q;
	struct updateop *r;
	if (asprintf(&c, "%s:%s", ns.cwmp, "ChangeDUState") == -1)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}

	n = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if (!n) return -1;
	b = n;

	change_du_state = calloc (1,sizeof(struct change_du_state));
	if (change_du_state == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}
	INIT_LIST_HEAD(&(change_du_state->list_operation));
	change_du_state->timeout = time(NULL);
	while (b != NULL) {
		t = b;
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "CommandKey")) {
			change_du_state->command_key = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_ELEMENT &&
			0 == strcmp(b->parent->value.element.name, "Operations")) {
			if (!strcmp(b->value.element.name, "InstallOpStruct")) {
				elem = (operations*)calloc(1, sizeof(operations));
				elem->type = DU_INSTALL;
				list_add_tail(&(elem->list), &(change_du_state->list_operation));
			}
			if (!strcmp(b->value.element.name, "UninstallOpStruct")) {
				elem = (operations*)calloc(1,sizeof(operations));
				elem->type = DU_UNINSTALL ;
				list_add_tail(&(elem->list), &(change_du_state->list_operation));									
			}
			if (!strcmp(b->value.element.name, "UpdateOpStruct")) {
				elem = (operations*)calloc(1,sizeof(operations));
				elem->type = DU_UPDATE;
				list_add_tail(&(elem->list), &(change_du_state->list_operation));		
				
			}
			t = mxmlWalkNext(t, b, MXML_DESCEND);
			while (t) {
				tmp1 = t;	
				if (t && t->type == MXML_ELEMENT &&
				!strcmp(t->parent->value.element.name, "InstallOpStruct")) {
					tmp1 = mxmlWalkNext(tmp1, t, MXML_DESCEND);
					while (tmp1) {
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "URL")) {
							elem->url = strdup(tmp1->value.text.string);
						}						
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "UUID")) {
							elem->uuid = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "Username")) {
							elem->username = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "Password")) {
							elem->password = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "ExecutionEnvRef")) {
							elem->executionenvref = strdup(tmp1->value.text.string);
						}
						tmp1 = mxmlWalkNext(tmp1, t, MXML_DESCEND);
								
					}													
				}
				else if (t && t->type == MXML_ELEMENT &&
				!strcmp(t->parent->value.element.name, "UpdateOpStruct")) {
					tmp1 = mxmlWalkNext(tmp1, t, MXML_DESCEND);
					while (tmp1) {
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "UUID")) {
							elem->uuid = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "Version")) {
							elem->version = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "URL")) {
							elem->url = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "Username")) {
							elem->username = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "Password")) {
							elem->password = strdup(tmp1->value.text.string);
						}
						tmp1 = mxmlWalkNext(tmp1, t, MXML_DESCEND);			
					}								
				}
				else if (t && t->type == MXML_ELEMENT &&
				!strcmp(t->parent->value.element.name, "UninstallOpStruct")) {
					tmp1 = mxmlWalkNext(tmp1, t, MXML_DESCEND);
					while (tmp1) {
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "UUID")) {
							elem->uuid = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "Version")) {
							elem->version = strdup(tmp1->value.text.string);
						}
						if (tmp1 && tmp1->type == MXML_TEXT &&
						tmp1->value.text.string &&
						tmp1->parent->type == MXML_ELEMENT &&
						!strcmp(tmp1->parent->value.element.name, "ExecutionEnvRef")) {
							elem->executionenvref = strdup(tmp1->value.text.string);
						}
						tmp1 = mxmlWalkNext(tmp1, t, MXML_DESCEND);			
					}
				}
				t = mxmlWalkNext(t, b, MXML_DESCEND);		
			}
		
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}	
	t = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!t) goto fault;

	t = mxmlNewElement(t, "cwmp:ChangeDUStateResponse");
	if (!t) goto fault;

	
	if(error == FAULT_CPE_NO_FAULT)
	{
		pthread_mutex_lock (&mutex_change_du_state);
		list_add_tail (&(change_du_state->list), &(list_change_du_state));
		bkp_session_insert_change_du_state(change_du_state);	
		bkp_session_save();
		pthread_mutex_unlock (&mutex_change_du_state);
		pthread_cond_signal(&threshold_change_du_state);
	}	
	return 0;

fault:
	cwmp_free_change_du_state_request(change_du_state);    
	if (cwmp_create_fault_message(session, rpc, error))
		goto error;
	return 0;

error:
	return -1;
}

int cwmp_handle_rpc_cpe_download(struct session *session, struct rpc *rpc)
{
	mxml_node_t 				*n, *t, *b = session->body_in;
	pthread_t           		download_thread;
	char 						*c, *tmp, *file_type = NULL;
	int							error = FAULT_CPE_NO_FAULT;
	struct download 			*download,*idownload;
	struct transfer_complete 	*ptransfer_complete;
	struct list_head    		*ilist;
	time_t             			scheduled_time;
	time_t 						download_delay;
	bool                		cond_signal = false;

	if (asprintf(&c, "%s:%s", ns.cwmp, "Download") == -1)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}

	n = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if (!n) return -1;
	b = n;

	download = calloc (1,sizeof(struct download));
	if (download == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}

	while (b != NULL) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "CommandKey")) {
			download->command_key = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "FileType")) {
			if(download->file_type == NULL)
			{
				download->file_type = strdup(b->value.text.string);
				file_type = strdup(b->value.text.string);
			}
			else
			{
				tmp = file_type;
				if (asprintf(&file_type,"%s %s",tmp, b->value.text.string) == -1)
				{
					error = FAULT_CPE_INTERNAL_ERROR;
					goto fault;
				}
				FREE(tmp);
			}
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "URL")) {
			download->url = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Username")) {
			download->username = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Password")) {
			download->password = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "FileSize")) {
			download->file_size = atoi(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "DelaySeconds")) {
			download_delay = atol(b->value.text.string);
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}

	if(strcmp(file_type,"1 Firmware Upgrade Image") &&
		strcmp(file_type,"2 Web Content") &&
		strcmp(file_type,"3 Vendor Configuration File"))
	{
		error = FAULT_CPE_INVALID_ARGUMENTS;
	}
	else if(count_download_queue>=MAX_DOWNLOAD_QUEUE)
	{
		error = FAULT_CPE_RESOURCES_EXCEEDED;
	}
	else if((download->url == NULL || ((download->url != NULL) && (strcmp(download->url,"")==0))))
	{
		error = FAULT_CPE_REQUEST_DENIED;
	}
	else if(strstr(download->url,"@") != NULL)
	{
		error = FAULT_CPE_INVALID_ARGUMENTS;
	}
	else if(strncmp(download->url,DOWNLOAD_PROTOCOL_HTTP,strlen(DOWNLOAD_PROTOCOL_HTTP))!=0 &&
			strncmp(download->url,DOWNLOAD_PROTOCOL_HTTPS,strlen(DOWNLOAD_PROTOCOL_HTTPS))!=0 &&
			strncmp(download->url,DOWNLOAD_PROTOCOL_FTP,strlen(DOWNLOAD_PROTOCOL_FTP))!=0)
	{
		error = FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL;
	}

	FREE(file_type);
	if(error != FAULT_CPE_NO_FAULT)
		goto fault;

	t = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!t) goto fault;

	t = mxmlNewElement(t, "cwmp:DownloadResponse");
	if (!t) goto fault;

	b = mxmlNewElement(t, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "1");
	if (!b) goto fault;

	b = b->parent->parent;
	b = mxmlNewElement(t, "StartTime");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "0001-01-01T00:00:00+00:00");
	if (!b) goto fault;

	b = b->parent->parent;
	b = mxmlNewElement(t, "CompleteTime");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "0001-01-01T00:00:00+00:00");
	if (!b) goto fault;

	if(error == FAULT_CPE_NO_FAULT)
	{
		pthread_mutex_lock (&mutex_download);
		if(download_delay != 0)
			scheduled_time = time(NULL) + download_delay;

		list_for_each(ilist,&(list_download))
		{
			idownload = list_entry(ilist,struct download, list);
			if (idownload->scheduled_time >= scheduled_time)
			{
				break;
			}
		}
		list_add (&(download->list), ilist->prev);
		if(download_delay != 0)
		{
			count_download_queue++;
			download->scheduled_time	= scheduled_time;
		}
		bkp_session_insert_download(download);
		bkp_session_save();
		if(download_delay != 0)
		{
			CWMP_LOG(INFO,"Download will start in %us",download_delay);
		}
		else
		{
			CWMP_LOG(INFO,"Download will start at the end of session");
		}

		pthread_mutex_unlock (&mutex_download);
		pthread_cond_signal(&threshold_download);
	}

	return 0;

fault:
    cwmp_free_download_request(download);
	if (cwmp_create_fault_message(session, rpc, error))
		goto error;
	return 0;

error:
	return -1;
}

int cwmp_handle_rpc_cpe_schedule_download(struct session *session, struct rpc *rpc)
{
	mxml_node_t 				*ibc, *n, *t, *b = session->body_in;
	pthread_t           		schedule_download_thread;
	char 						*c, *tmp, *file_type = NULL;
	char 						*windowmode0 = NULL, *windowmode1 = NULL;
	int							i = 0, j = 0;
	int							error = FAULT_CPE_NO_FAULT;
	struct schedule_download 	*schedule_download,*ischedule_download;
	struct transfer_complete 	*ptransfer_complete;
	struct list_head    		*ilist;
	time_t             			scheduled_time;
	time_t 						schedule_download_delay[4] = {0, 0, 0, 0};
	bool                		cond_signal = false;
	

	if (asprintf(&c, "%s:%s", ns.cwmp, "ScheduleDownload") == -1)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}

	n = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if (!n) return -1;
	b = n;


	schedule_download = calloc (1,sizeof(struct schedule_download));
	if (schedule_download == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}

	while (b != NULL) {
		t = b;
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "CommandKey")) {
			schedule_download->command_key = strdup(b->value.text.string);			
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "FileType")) {
			if(schedule_download->file_type == NULL)
			{
				schedule_download->file_type = strdup(b->value.text.string);
				file_type = strdup(b->value.text.string);				
			}
			else
			{
				tmp = file_type;
				if (asprintf(&file_type,"%s %s",tmp, b->value.text.string) == -1)
				{
					error = FAULT_CPE_INTERNAL_ERROR;
					goto fault;
				}
				FREE(tmp);
			}
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "URL")) {
			schedule_download->url = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Username")) {
			schedule_download->username = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Password")) {
			schedule_download->password = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "FileSize")) {
			schedule_download->file_size = atoi(b->value.text.string);
		}
		
		if (b && b->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "TimeWindowList")) {
			if (!t) return -1; //TO CHECK*/
			t = mxmlWalkNext(t, b, MXML_DESCEND);
			while (t) {
				if (t && t->type == MXML_TEXT &&
					t->value.text.string &&
					t->parent->type == MXML_ELEMENT &&
					!strcmp(t->parent->value.element.name, "WindowStart")) {
					schedule_download_delay[j] = atol(t->value.text.string);
					j++;						
				}					
				if (t && t->type == MXML_TEXT &&
					t->value.text.string &&
					t->parent->type == MXML_ELEMENT &&
					!strcmp(t->parent->value.element.name, "WindowEnd")) {
					schedule_download_delay[j] = atol(t->value.text.string);
					j++;
				}
				if (t && t->type == MXML_TEXT &&
					t->value.text.string &&
					t->parent->type == MXML_ELEMENT &&
					!strcmp(t->parent->value.element.name, "WindowMode")) {
					
					if(schedule_download->timewindowstruct[i].windowmode == NULL)
					{
						schedule_download->timewindowstruct[i].windowmode = strdup(t->value.text.string);
						if (i == 0)
							windowmode0 = strdup(t->value.text.string);
						else
							windowmode1 = strdup(t->value.text.string);
					}
					else if (i == 0)
					{
						
						tmp = windowmode0;
						if (asprintf(&windowmode0,"%s %s",tmp, t->value.text.string) == -1)
						{
							error = FAULT_CPE_INTERNAL_ERROR;
							goto fault;
						}
						FREE(tmp);
					}
					else if (i == 1)
					{
						tmp = windowmode1;
						if (asprintf(&windowmode1,"%s %s",tmp, t->value.text.string) == -1)
						{
							error = FAULT_CPE_INTERNAL_ERROR;
							goto fault;
						}
						FREE(tmp);
					}
				}
				
				if (t && t->type == MXML_TEXT &&
					t->value.text.string &&
					t->parent->type == MXML_ELEMENT &&
					!strcmp(t->parent->value.element.name, "UserMessage")) {
					schedule_download->timewindowstruct[i].usermessage = strdup(t->value.text.string);
				}	
				if (t && t->type == MXML_TEXT &&
					t->value.text.string &&
					t->parent->type == MXML_ELEMENT &&
					!strcmp(t->parent->value.element.name, "MaxRetries")) {
					schedule_download->timewindowstruct[i].maxretries = atoi(t->value.text.string);
				}				
				t = mxmlWalkNext(t, b, MXML_DESCEND);
			}
			i++;
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}
	if(strcmp(file_type,"1 Firmware Upgrade Image") &&
		strcmp(file_type,"2 Web Content") &&
		strcmp(file_type,"3 Vendor Configuration File") &&
		strcmp(file_type,"4 Tone File") &&
		strcmp(file_type,"5 Ringer File"))
	{
		error = FAULT_CPE_INVALID_ARGUMENTS;
	}
	else if((
		strcmp(windowmode0,"1 At Any Time") &&
		strcmp(windowmode0,"2 Immediately") &&
		strcmp(windowmode0,"3 When Idle")) || 
		(
		strcmp(windowmode1,"1 At Any Time") &&
		strcmp(windowmode1,"2 Immediately") &&
		strcmp(windowmode1,"3 When Idle")))
	{
		error = FAULT_CPE_REQUEST_DENIED;
	}
	else if(count_download_queue>=MAX_DOWNLOAD_QUEUE)
	{
		error = FAULT_CPE_RESOURCES_EXCEEDED;
	}
	else if((schedule_download->url == NULL || ((schedule_download->url != NULL) && (strcmp(schedule_download->url,"")==0))))
	{
		error = FAULT_CPE_REQUEST_DENIED;
	}
	else if(strstr(schedule_download->url,"@") != NULL )
	{
		error = FAULT_CPE_INVALID_ARGUMENTS;
	}
	else if(strncmp(schedule_download->url,DOWNLOAD_PROTOCOL_HTTP,strlen(DOWNLOAD_PROTOCOL_HTTP))!=0 &&
			strncmp(schedule_download->url,DOWNLOAD_PROTOCOL_FTP,strlen(DOWNLOAD_PROTOCOL_FTP))!=0)
	{
		error = FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL;
	}
	else {
		for (j = 0; j<3; j++)
		{
			if (schedule_download_delay[j] > schedule_download_delay[j+1])
			{				
				error = FAULT_CPE_INVALID_ARGUMENTS;
				break;
			}
		}
	}

	FREE(file_type);
	if(error != FAULT_CPE_NO_FAULT)
		goto fault;

	t = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!t) goto fault;

	t = mxmlNewElement(t, "cwmp:ScheduleDownloadResponse"); // ScheduleDownloadResponse has no argument
	if (!t) goto fault;
	
	if(error == FAULT_CPE_NO_FAULT)
	{
		pthread_mutex_lock (&mutex_schedule_download);		
		list_add_tail (&(schedule_download->list), &(list_schedule_download));
		if(schedule_download_delay[0] != 0)
		{
			count_download_queue++;			
		}
		while (i > 0) {
			i--;
			schedule_download->timewindowstruct[i].windowstart = time(NULL) + schedule_download_delay[i*2];
			schedule_download->timewindowstruct[i].windowend = time(NULL) + schedule_download_delay[i*2+1];
			
		}
		bkp_session_insert_schedule_download(schedule_download);
		bkp_session_save();
		if(schedule_download_delay[0] != 0)
		{
			CWMP_LOG(INFO,"Schedule download will start in %us",schedule_download_delay[0]);
		}
		else
		{
			CWMP_LOG(INFO,"Schedule Download will start at the end of session");
		}
		pthread_mutex_unlock (&mutex_schedule_download);
		pthread_cond_signal(&threshold_schedule_download);
	}
	
	return 0;

fault:
	cwmp_free_schedule_download_request(schedule_download);
	if (cwmp_create_fault_message(session, rpc, error))
		goto error;
	return 0;

error:
	return -1;
}

int cwmp_handle_rpc_cpe_upload(struct session *session, struct rpc *rpc)
{
	mxml_node_t 				*n, *t, *b = session->body_in;
	pthread_t           		upload_thread;
	char 						*c, *tmp, *file_type = NULL;
	int							error = FAULT_CPE_NO_FAULT;
	struct upload 			*upload,*iupload;
	struct transfer_complete 	*ptransfer_complete;
	struct list_head    		*ilist;
	time_t             			scheduled_time;
	time_t 						upload_delay;
	bool                		cond_signal = false;

	if (asprintf(&c, "%s:%s", ns.cwmp, "Upload") == -1)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}

	n = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if (!n) return -1;
	b = n;

	upload = calloc (1,sizeof(struct upload));
	if (upload == NULL)
	{
		error = FAULT_CPE_INTERNAL_ERROR;
		goto fault;
	}
	upload->f_instance = strdup("");
	while (b != NULL) {
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "CommandKey")) {
			upload->command_key = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "FileType")) {
			if(upload->file_type == NULL)
			{
				upload->file_type = strdup(b->value.text.string);
				file_type = strdup(b->value.text.string);
			}
			else
			{
				tmp = file_type;
				if (asprintf(&file_type,"%s %s",tmp, b->value.text.string) == -1)
				{
					error = FAULT_CPE_INTERNAL_ERROR;
					goto fault;
				}
				if (isdigit(b->value.text.string[0])) {
					upload->f_instance = strdup(b->value.text.string);
				}
				FREE(tmp);
			}
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "URL")) {
			upload->url = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Username")) {
			upload->username = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Password")) {
			upload->password = strdup(b->value.text.string);
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "DelaySeconds")) {
			upload_delay = atol(b->value.text.string);
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}
	if(strncmp(file_type, "3 Vendor Configuration File", sizeof"3 Vendor Configuration File" -1) != 0 &&
		strncmp(file_type, "4 Vendor Log File", sizeof"4 Vendor Log File" -1) != 0)
	{
		error = FAULT_CPE_REQUEST_DENIED;
	}
	else if(count_download_queue>=MAX_DOWNLOAD_QUEUE)
	{
		error = FAULT_CPE_RESOURCES_EXCEEDED;
	}
	else if((upload->url == NULL || ((upload->url != NULL) && (strcmp(upload->url,"")==0))))
	{
		error = FAULT_CPE_REQUEST_DENIED;
	}
	else if(strstr(upload->url,"@") != NULL)
	{
		error = FAULT_CPE_INVALID_ARGUMENTS;
	}
	else if(strncmp(upload->url,DOWNLOAD_PROTOCOL_HTTP,strlen(DOWNLOAD_PROTOCOL_HTTP))!=0 &&
			strncmp(upload->url,DOWNLOAD_PROTOCOL_FTP,strlen(DOWNLOAD_PROTOCOL_FTP))!=0)
	{
		error = FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL;
	}

	FREE(file_type);
	if(error != FAULT_CPE_NO_FAULT)
		goto fault;

	t = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!t) goto fault;

	t = mxmlNewElement(t, "cwmp:UploadResponse");
	if (!t) goto fault;

	b = mxmlNewElement(t, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "1");
	if (!b) goto fault;

	b = b->parent->parent;
	b = mxmlNewElement(t, "StartTime");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "0001-01-01T00:00:00+00:00");
	if (!b) goto fault;

	b = b->parent->parent;
	b = mxmlNewElement(t, "CompleteTime");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "0001-01-01T00:00:00+00:00");
	if (!b) goto fault;

	if(error == FAULT_CPE_NO_FAULT)
	{
		pthread_mutex_lock (&mutex_upload);
		if(upload_delay != 0)
			scheduled_time = time(NULL) + upload_delay;

		list_for_each(ilist,&(list_upload))
		{
			iupload = list_entry(ilist,struct upload, list);
			if (iupload->scheduled_time >= scheduled_time)
			{
				break;
			}
		}
		list_add (&(upload->list), ilist->prev);
		if(upload_delay != 0)
		{
			count_download_queue++;
			upload->scheduled_time	= scheduled_time;
		}
		bkp_session_insert_upload(upload);
		bkp_session_save();
		if(upload_delay != 0)
		{
			CWMP_LOG(INFO,"Upload will start in %us",upload_delay);
		}
		else
		{
			CWMP_LOG(INFO,"Upload will start at the end of session");
		}

		pthread_mutex_unlock (&mutex_upload);
		pthread_cond_signal(&threshold_upload);
	}

	return 0;

fault:
	cwmp_free_upload_request(upload);
	if (cwmp_create_fault_message(session, rpc, error))
		goto error;
	return 0;

error:
	return -1;
}
/*
 * [FAULT]: Fault
 */

int cwmp_handle_rpc_cpe_fault(struct session *session, struct rpc *rpc)
{
	mxml_node_t *b, *t, *u, *body;
	struct param_fault *param_fault;
	int idx;

	body = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
		    NULL, NULL, MXML_DESCEND);

	b = mxmlNewElement(body, "soap_env:Fault");
	if (!b) return -1;

	t = mxmlNewElement(b, "faultcode");
	if (!t) return -1;

	u = mxmlNewText(t, 0,
			(FAULT_CPE_ARRAY[session->fault_code].TYPE == FAULT_CPE_TYPE_CLIENT) ? "Client" : "Server");
	if (!u) return -1;

	t = mxmlNewElement(b, "faultstring");
	if (!t) return -1;

	u = mxmlNewText(t, 0, "CWMP fault");
	if (!u) return -1;

	b = mxmlNewElement(b, "detail");
	if (!b) return -1;

	b = mxmlNewElement(b, "cwmp:Fault");
	if (!b) return -1;

	t = mxmlNewElement(b, "FaultCode");
	if (!t) return -1;

	u = mxmlNewText(t, 0, FAULT_CPE_ARRAY[session->fault_code].CODE);
	if (!u) return -1;

	t = mxmlNewElement(b, "FaultString");
	if (!b) return -1;

	u = mxmlNewText(t, 0, FAULT_CPE_ARRAY[session->fault_code].DESCRIPTION);
	if (!u) return -1;

	if (rpc->type == RPC_CPE_SET_PARAMETER_VALUES) {
		while (rpc->list_set_value_fault->next != rpc->list_set_value_fault) {
			param_fault = list_entry(rpc->list_set_value_fault->next, struct param_fault, list);

			if (param_fault->fault)
			{
				idx = cwmp_get_fault_code(param_fault->fault);

				t = mxmlNewElement(b, "SetParameterValuesFault");
				if (!t) return -1;

				u = mxmlNewElement(t, "ParameterName");
				if (!u) return -1;

				u = mxmlNewText(u, 0, param_fault->name);
				if (!u) return -1;

				u = mxmlNewElement(t, "FaultCode");
				if (!u) return -1;

				u = mxmlNewText(u, 0, FAULT_CPE_ARRAY[idx].CODE);
				if (!u) return -1;

				u = mxmlNewElement(t, "FaultString");
				if (!u) return -1;

				u = mxmlNewText(u, 0, FAULT_CPE_ARRAY[idx].DESCRIPTION);
				if (!u) return -1;
			}
			del_list_fault_param(param_fault);
		}
	}

	return 0;
}

int cwmp_get_fault_code (int fault_code)
{
	int i;

	for (i=1; i<__FAULT_CPE_MAX; i++)
	{
		if (FAULT_CPE_ARRAY[i].ICODE == fault_code)
			break;
	}

	if(i == __FAULT_CPE_MAX)
		i = FAULT_CPE_INTERNAL_ERROR;

	return i;
}

int cwmp_create_fault_message(struct session *session, struct rpc *rpc_cpe, int fault_code)
{
	CWMP_LOG (INFO,"Fault detected");
	session->fault_code = fault_code;

	MXML_DELETE(session->tree_out);

	if (xml_prepare_msg_out(session))
		return -1;

	CWMP_LOG (INFO,"Preparing the Fault message");
	if (rpc_cpe_methods[RPC_CPE_FAULT].handler(session, rpc_cpe))
		return -1;

	rpc_cpe->type = RPC_CPE_FAULT;

	return 0;
}
