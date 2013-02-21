/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	Contributed by Inteno Broadband Technology AB
 *
 *	Copyright (C) 2013 Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	Copyright (C) 2013 Ahmed Zribi <ahmed.zribi@pivasoftware.com>
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

LIST_HEAD(list_download);
static pthread_mutex_t		mutex_download = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		threshold_download;
static bool					thread_download_is_working = false;
int							count_download_queue = 0;
int							count_schedule_inform_queue = 0;

LIST_HEAD(list_schedule_inform);
static pthread_mutex_t      mutex_schedule_inform = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t       threshold_schedule_inform;
static bool                 thread_is_working=false;

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
		NULL };

struct FAULT_CPE FAULT_CPE_ARRAY [] = {
    [FAULT_CPE_METHOD_NOT_SUPPORTED]                = {"9000",FAULT_CPE_TYPE_SERVER, "Method not supported"},
    [FAULT_CPE_REQUEST_DENIED]                      = {"9001",FAULT_CPE_TYPE_SERVER, "Request denied (no reason specified)"},
    [FAULT_CPE_INTERNAL_ERROR]                      = {"9002",FAULT_CPE_TYPE_SERVER, "Internal error"},
    [FAULT_CPE_INVALID_ARGUMENTS]                   = {"9003",FAULT_CPE_TYPE_CLIENT, "Invalid arguments"},
    [FAULT_CPE_RESOURCES_EXCEEDED]                  = {"9004",FAULT_CPE_TYPE_SERVER, "Resources exceeded"},
    [FAULT_CPE_INVALID_PARAMETER_NAME]              = {"9005",FAULT_CPE_TYPE_CLIENT, "Invalid parameter name"},
    [FAULT_CPE_INVALID_PARAMETER_TYPE]              = {"9006",FAULT_CPE_TYPE_CLIENT, "Invalid parameter type"},
    [FAULT_CPE_INVALID_PARAMETER_VALUE]             = {"9007",FAULT_CPE_TYPE_CLIENT, "Invalid parameter value"},
    [FAULT_CPE_NON_WRITABLE_PARAMETER]              = {"9008",FAULT_CPE_TYPE_CLIENT, "Attempt to set a non-writable parameter"},
    [FAULT_CPE_NOTIFICATION_REJECTED]               = {"9009",FAULT_CPE_TYPE_SERVER, "Notification request rejected"},
    [FAULT_CPE_DOWNLOAD_FAILURE]                    = {"9010",FAULT_CPE_TYPE_SERVER, "Download failure"},
    [FAULT_CPE_UPLOAD_FAILURE]                      = {"9011",FAULT_CPE_TYPE_SERVER, "Upload failure"},
    [FAULT_CPE_FILE_TRANSFER_AUTHENTICATION_FAILURE]= {"9012",FAULT_CPE_TYPE_SERVER, "File transfer server authentication failure"},
    [FAULT_CPE_FILE_TRANSFER_UNSUPPORTED_PROTOCOL]  = {"9013",FAULT_CPE_TYPE_SERVER, "Unsupported protocol for file transfer"},
    [FAULT_CPE_DOWNLOAD_FAIL_MULTICAST_GROUP]       = {"9014",FAULT_CPE_TYPE_SERVER, "Download failure: unable to join multicast group"},
    [FAULT_CPE_DOWNLOAD_FAIL_CONTACT_SERVER]        = {"9015",FAULT_CPE_TYPE_SERVER, "Download failure: unable to contact file server"},
    [FAULT_CPE_DOWNLOAD_FAIL_ACCESS_FILE]           = {"9016",FAULT_CPE_TYPE_SERVER, "Download failure: unable to access file"},
    [FAULT_CPE_DOWNLOAD_FAIL_COMPLETE_DOWNLOAD]     = {"9017",FAULT_CPE_TYPE_SERVER, "Download failure: unable to complete download"},
    [FAULT_CPE_DOWNLOAD_FAIL_FILE_CORRUPTED]        = {"9018",FAULT_CPE_TYPE_SERVER, "Download failure: file corrupted"},
    [FAULT_CPE_DOWNLOAD_FAIL_FILE_AUTHENTICATION]   = {"9019",FAULT_CPE_TYPE_SERVER, "Download failure: file authentication failure"}
};

const struct rpc_cpe_method rpc_cpe_methods[] = {
	[RPC_CPE_GET_RPC_METHODS] 				= {"GetRPCMethods", cwmp_handle_rpc_cpe_get_rpc_methods},
	[RPC_CPE_SET_PARAMETER_VALUES] 			= {"SetParameterValues", cwmp_handle_rpc_cpe_set_parameter_values},
	[RPC_CPE_GET_PARAMETER_VALUES] 			= {"GetParameterValues", cwmp_handle_rpc_cpe_get_parameter_values},
	[RPC_CPE_GET_PARAMETER_NAMES] 			= {"GetParameterNames", cwmp_handle_rpc_cpe_get_parameter_names},
	[RPC_CPE_SET_PARAMETER_ATTRIBUTES] 		= {"SetParameterAttributes", cwmp_handle_rpc_cpe_set_parameter_attributes},
	[RPC_CPE_GET_PARAMETER_ATTRIBUTES] 		= {"GetParameterAttributes", cwmp_handle_rpc_cpe_get_parameter_attributes},
	[RPC_CPE_ADD_OBJECT] 					= {"AddObject", cwmp_handle_rpc_cpe_add_object},
	[RPC_CPE_DELETE_OBJECT] 				= {"DeleteObject", cwmp_handle_rpc_cpe_delete_object},
	[RPC_CPE_REBOOT] 						= {"Reboot", cwmp_handle_rpc_cpe_reboot},
	[RPC_CPE_DOWNLOAD] 						= {"Download", cwmp_handle_rpc_cpe_download},
	[RPC_CPE_FACTORY_RESET] 				= {"FactoryReset", cwmp_handle_rpc_cpe_factory_reset},
	[RPC_CPE_SCHEDULE_INFORM] 				= {"ScheduleInform", cwmp_handle_rpc_cpe_schedule_inform},
	[RPC_CPE_FAULT] 						= {"Fault", cwmp_handle_rpc_cpe_fault}
};

const struct rpc_acs_method rpc_acs_methods[] = {
	[RPC_ACS_INFORM] 			= {"Inform", cwmp_rpc_acs_prepare_message_inform, cwmp_rpc_acs_parse_response_inform, cwmp_rpc_acs_destroy_data_inform},
	[RPC_ACS_GET_RPC_METHODS] 	= {"GetRPCMethods", cwmp_rpc_acs_prepare_get_rpc_methods, NULL, NULL},
	[RPC_ACS_TRANSFER_COMPLETE] = {"TransferComplete", cwmp_rpc_acs_prepare_transfer_complete,	NULL, cwmp_rpc_acs_destroy_data_transfer_complete}
};

const struct DEVICE_ID_STRUCT DEVICE_ID_CONST [] = {
		{ "OUI",          "InternetGatewayDevice.DeviceInfo.ManufacturerOUI" },
		{ "ProductClass", "InternetGatewayDevice.DeviceInfo.ProductClass" },
		{ "SerialNumber", "InternetGatewayDevice.DeviceInfo.SerialNumber" },
		{ "Manufacturer", "InternetGatewayDevice.DeviceInfo.Manufacturer" }
};

static int xml_recreate_namespace(mxml_node_t *tree)
{
	const char *cwmp_urn;
	char *c;
	int i;

	FREE(ns.soap_env);
	FREE(ns.soap_enc);
	FREE(ns.xsd);
	FREE(ns.xsi);
	FREE(ns.cwmp);

	c = (char *) mxmlElementGetAttrName(tree, soap_env_url);
	if (c && *(c + 5) == ':') {
		ns.soap_env = strdup((c + 6));
	} else {
		return -1;
	}

	c = (char *) mxmlElementGetAttrName(tree, soap_enc_url);
	if (c && *(c + 5) == ':') {
		ns.soap_enc = strdup((c + 6));
	} else {
		return -1;
	}

	c = (char *) mxmlElementGetAttrName(tree, xsd_url);
	if (c && *(c + 5) == ':') {
		ns.xsd = strdup((c + 6));
	} else {
		return -1;
	}

	c = (char *) mxmlElementGetAttrName(tree, xsi_url);
	if (c && *(c + 5) == ':') {
		ns.xsi = strdup((c + 6));
	} else {
		return -1;
	}

	for (i = 0; cwmp_urls[i] != NULL; i++) {
		cwmp_urn = cwmp_urls[i];
		c = (char *) mxmlElementGetAttrName(tree, cwmp_urn);
		if (c && *(c + 5) == ':') {
			ns.cwmp = strdup((c + 6));
			break;
		}
	}

	if (!ns.cwmp) return -1;

	return 0;

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
	int f, r = 0;
	mxml_node_t	*b;

	if (session->tree_out)
		msg_out = mxmlSaveAllocString(session->tree_out, whitespace_cb);
	while (1) {
		f = 0;
		if (http_send_message(cwmp, msg_out, &msg_in))
			goto error;
		if (msg_in) {
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
		if (b && b->value.text.string)
			session->hold_request = atoi(b->value.text.string);
	} else {
		if (asprintf(&c, "%s:%s", ns.cwmp, "HoldRequests") == -1)
			goto error;

		b = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
		FREE(c);
		if (b) {
			b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
			if (b && b->value.text.string)
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
#ifdef DUMMY_MODE
	FILE *fp;
	fp = fopen("./ext/soap_msg_templates/cwmp_response_message.xml", "r");
	session->tree_out = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
#else
	session->tree_out = mxmlLoadString(NULL, CWMP_RESPONSE_MESSAGE, MXML_NO_CALLBACK);
#endif
	if (!session->tree_out) return -1;

	return 0;
}

int xml_set_cwmp_id(struct session *session)
{
	char		*c;
	mxml_node_t	*b;

	/* handle cwmp:ID */
	if (asprintf(&c, "%s:%s", ns.cwmp, "ID") == -1)
		return -1;

	b = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	/* ACS did not send ID parameter, we are continuing without it */
	if (b) {

		b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
		if (!b || !b->value.text.string) return 0;
		c = strdup(b->value.text.string);

		b = mxmlFindElement(session->tree_out, session->tree_out, "cwmp:ID", NULL, NULL, MXML_DESCEND);
		if (!b) return -1;

		b = mxmlNewText(b, 0, c);
		FREE(c);

		if (!b) return -1;
	}
	return 0;
}

int xml_handle_message(struct session *session)
{
	struct rpc	*rpc_cpe;
	char		*c;
	int 		i;
	mxml_node_t	*b;

	/* get method */

	if (asprintf(&c, "%s:%s", ns.soap_env, "Body") == -1)
		goto error;

	b = mxmlFindElement(session->tree_in, session->tree_in, c, NULL, NULL, MXML_DESCEND);
	FREE(c);

	if(!b)
		goto error;

	session->body_in = b;

	while (1) {
		b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
		if (!b) goto error;
		if (b->type == MXML_ELEMENT) break;
	}

	c = b->value.element.name;
	/* convert QName to localPart, check that ns is the expected one */
	if (strchr(c, ':')) {
		char *tmp = strchr(c, ':');
		size_t ns_len = tmp - c;

		if (strlen(ns.cwmp) != ns_len)
			goto error;

		if (strncmp(ns.cwmp, c, ns_len))
			goto error;

		c = tmp + 1;
	} else {
		goto error;
	}
	CWMP_LOG (INFO,"SOAP RPC message: %s", c);
	rpc_cpe = NULL;
	for (i = 1; i < __RPC_CPE_MAX; i++) {
		if (i!= RPC_CPE_FAULT && strcmp(c, rpc_cpe_methods[i].name) == 0) {
			CWMP_LOG (INFO,"%s RPC is supported",c);
			rpc_cpe = cwmp_add_session_rpc_cpe(session, i);
			if (rpc_cpe == NULL) goto error;
			break;
		}
	}
	if (!rpc_cpe) {
		CWMP_LOG (INFO,"%s RPC is not supported",c);
		session->fault_code = FAULT_CPE_METHOD_NOT_SUPPORTED;
		rpc_cpe = cwmp_add_session_rpc_cpe(session, RPC_CPE_FAULT);
		if (rpc_cpe == NULL) goto error;
	}
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
		mxmlElementSetAttr(b1, "soap_enc:arrayType", c);
		free(c);
	}
	return 0;

error:
	return -1;
}

static int xml_prepare_parameters_inform(struct parameter_container *parameter_container, mxml_node_t *parameter_list, int *size)
{
	mxml_node_t *node, *b;
	int found;

	if (parameter_container->fault_code && parameter_container->fault_code[0]=='9')
		return -1;

	b = mxmlFindElementText(parameter_list, parameter_list, parameter_container->name, MXML_DESCEND);
	if(b && parameter_container->data != NULL)
	{
		node = b->parent->parent;
		b = mxmlFindElement(node, node, "Value", NULL, NULL, MXML_DESCEND_FIRST);
		if(!b) return 0;
		if (b->child && strcmp(parameter_container->data, b->child->value.text.string)==0)
			return 0;
		mxmlDelete(b);
		(*size)--;

		goto create_value;
	}
	else if (b && parameter_container->data == NULL)
	{
		return 0;
	}
	else if (!b && parameter_container->data == NULL)
	{
		external_get_action_write("value", parameter_container->name, NULL);
		return 0;
	}
	node = mxmlNewElement (parameter_list, "ParameterValueStruct");
	if (!node) return -1;

	b = mxmlNewElement(node, "Name");
	if (!b) return -1;

	b = mxmlNewText(b, 0, parameter_container->name);
	if (!b) return -1;

create_value:
	b = mxmlNewElement(node, "Value");
	if (!b) return -1;

#ifdef ACS_MULTI
	mxmlElementSetAttr(b, "xsi:type", parameter_container->type? parameter_container->type : "xsd:string");
#endif
	b = mxmlNewText(b, 0, parameter_container->data);
	if (!b) return -1;

	(*size)++;
	return 0;
}

int cwmp_rpc_acs_prepare_message_inform (struct cwmp *cwmp, struct session *session, struct rpc *this)
{
    struct parameter_container *parameter_container;
    struct event_container *event_container;
    mxml_node_t *tree, *b, *node, *parameter_list;
    char *c = NULL;
    int size = 0, i, error;
    struct list_head *ilist,*jlist;

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

	b = mxmlFindElement(tree, tree, "RetryCount", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;

	b = mxmlNewInteger(b, cwmp->retry_count_session);
	if (!b) goto error;

	for (i=0; i<ARRAYSIZEOF(DEVICE_ID_CONST); i++)
		if (external_get_action_write("value",DEVICE_ID_CONST[i].parameter_name, NULL)) return -1;
	if (external_get_action_execute(cwmp_handle_getParamValues)) return -1;
	while (external_list_parameter.next!=&external_list_parameter) {
		parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);
		for (i=0; i<ARRAYSIZEOF(DEVICE_ID_CONST); i++)
		{
			if(strcmp(DEVICE_ID_CONST[i].parameter_name, parameter_container->name)==0) {
				if (parameter_container->fault_code && parameter_container->fault_code[0]=='9')
					goto error;
				b = mxmlFindElement(tree, tree, DEVICE_ID_CONST[i].device_id_name, NULL, NULL, MXML_DESCEND);
				if (!b) goto error;

				b = mxmlNewText(b, 0, parameter_container->data? parameter_container->data : "");
				if (!b) goto error;
			}
		}
		parameter_container_delete(parameter_container);
	}

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
        list_for_each(jlist, &(event_container->head_parameter_container))
        {
        	parameter_container = list_entry(jlist, struct parameter_container, list);
        	if (xml_prepare_parameters_inform(parameter_container, parameter_list, &size))
        	    goto error;
        }
    }

    external_get_action_execute(cwmp_handle_getParamValues);
    external_simple("inform", cwmp_handle_getParamValues);

    while (external_list_parameter.next!=&external_list_parameter) {

    	parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);
    	if (xml_prepare_parameters_inform(parameter_container, parameter_list, &size))
    		goto error;
		parameter_container_delete(parameter_container);
    }

    if (asprintf(&c, "cwmp:ParameterValueStruct[%d]", size) == -1)
		goto error;

	mxmlElementSetAttr(parameter_list, "soap_enc:arrayType", c);

	free(c);
	session->tree_out = tree;

	return 0;

error:
	external_free_list_parameter();
	return -1;
}

int cwmp_rpc_acs_parse_response_inform (struct cwmp *cwmp, struct session *session, struct rpc *this)
{
	mxml_node_t *tree, *b;

	tree = session->tree_in;
	if (!tree) goto error;
	b = mxmlFindElement(tree, tree, "MaxEnvelopes", NULL, NULL, MXML_DESCEND);
	if (!b) goto error;
	b = mxmlWalkNext(b, tree, MXML_DESCEND_FIRST);
	if (!b || !b->value.text.string)
		goto error;
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
	if (!b || !b->value.text.string)
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

	if(p->fault_code != 0)
	{
		n = n->parent->parent;
		n = mxmlNewElement(n, "FaultStruct");
		if (!n) goto error;

		n = mxmlNewElement(n, "FaultCode");
		if (!n) goto error;

		n = mxmlNewText(n, 0, FAULT_CPE_ARRAY[p->fault_code].CODE);
		if (!n) goto error;

		n = n->parent->parent;
		n = mxmlNewElement(n, "FaultString");
		if (!n) goto error;

		n = mxmlNewText(n, 0, FAULT_CPE_ARRAY [p->fault_code].DESCRIPTION);
		if (!n) goto error;
	}

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
	}
	FREE(rpc->extra_data);
	return 0;
}

/*
 * [RPC CPE]: GetParameterValues
 */

int cwmp_handle_rpc_cpe_get_parameter_values(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *parameter_list, *b;
	struct parameter_container *parameter_container;
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	char *c = NULL;
	int counter = 0, fault_code = FAULT_CPE_INTERNAL_ERROR;

	b = session->body_in;
	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
			    NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:GetParameterValuesResponse");
	if (!n) goto fault;

	parameter_list = mxmlNewElement(n, "ParameterList");
	if (!parameter_list) goto fault;

#ifdef ACS_MULTI
	mxmlElementSetAttr(n, "xsi:type", "soap_enc:Array");
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
			if (external_get_action_write("value",parameter_name, NULL))
				goto fault;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
		parameter_name = NULL;
	}
	if (external_get_action_execute(cwmp_handle_getParamValues))
		goto fault;

	while (external_list_parameter.next!=&external_list_parameter) {
		parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);

		if (parameter_container->fault_code && parameter_container->fault_code[0]=='9')
		{
			fault_code = cwmp_get_fault_code(parameter_container->fault_code);
			goto fault;
		}

		n = mxmlNewElement(parameter_list, "ParameterValueStruct");
		if (!n) goto fault;

		n = mxmlNewElement(n, "Name");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, parameter_container->name);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "Value");
		if (!n) goto fault;

#ifdef ACS_MULTI
		mxmlElementSetAttr(n, "xsi:type", parameter_container->type);
#endif
		n = mxmlNewText(n, 0, parameter_container->data? parameter_container->data : "");
		if (!n) goto fault;

		counter++;

		parameter_container_delete(parameter_container);
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

	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	external_free_list_parameter();
	return 0;

error:
	external_free_list_parameter();
	return -1;
}

/*
 * [RPC CPE]: GetParameterNames
 */

int cwmp_handle_rpc_cpe_get_parameter_names(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *parameter_list, *b = session->body_in;
	struct parameter_container *parameter_container;
	char *parameter_name = NULL;
	char *NextLevel = NULL;
	char *c;
	int counter = 0, fault_code = FAULT_CPE_INTERNAL_ERROR;

	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:GetParameterNamesResponse");
	if (!n) goto fault;

	parameter_list = mxmlNewElement(n, "ParameterList");
	if (!parameter_list) goto fault;

#ifdef ACS_MULTI
	mxmlElementSetAttr(n, "xsi:type", "soap_enc:Array");
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
		if (external_get_action("name", parameter_name, NextLevel, cwmp_handle_getParamNames))
			goto fault;
	}

	while (external_list_parameter.next!=&external_list_parameter) {

		parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);

		if (parameter_container->fault_code && parameter_container->fault_code[0]=='9')
		{
			fault_code = cwmp_get_fault_code(parameter_container->fault_code);
			goto fault;
		}

		n = mxmlNewElement(parameter_list, "ParameterInfoStruct");
		if (!n) goto fault;

		n = mxmlNewElement(n, "Name");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, parameter_container->name);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "Writable");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, parameter_container->data);
		if (!n) goto fault;

		counter++;

		parameter_container_delete(parameter_container);
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

	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	external_free_list_parameter();
	return 0;

error:
	external_free_list_parameter();
	return -1;
}

/*
 * [RPC CPE]: GetParameterAttributes
 */

int cwmp_handle_rpc_cpe_get_parameter_attributes(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *parameter_list, *b;
	struct parameter_container *parameter_container;
	char *parameter_name = NULL;
	char *c=NULL;
	int counter = 0, fault_code = FAULT_CPE_INTERNAL_ERROR;

	b = session->body_in;
	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:GetParameterAttributesResponse");
	if (!n) goto fault;

	parameter_list = mxmlNewElement(n, "ParameterList");
	if (!parameter_list) goto fault;

#ifdef ACS_MULTI
	mxmlElementSetAttr(n, "xsi:type", "soap_enc:Array");
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
			if (external_get_action_write("notification",parameter_name, NULL))
				goto fault;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
		parameter_name = NULL;
	}
	if (external_get_action_execute(cwmp_handle_getParamAttributes))
		goto fault;

	while (external_list_parameter.next!=&external_list_parameter) {
		parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);

		if (parameter_container->fault_code && parameter_container->fault_code[0]=='9')
		{
			fault_code = cwmp_get_fault_code(parameter_container->fault_code);
			goto fault;
		}

		n = mxmlNewElement(parameter_list, "ParameterAttributeStruct");
		if (!n) goto fault;

		n = mxmlNewElement(n, "Name");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, parameter_container->name);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "Notification");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, parameter_container->data);
		if (!n) goto fault;

		n = n->parent->parent;
		n = mxmlNewElement(n, "AccessList");
		if (!n) goto fault;

		n = mxmlNewText(n, 0, "");
		if (!n) goto fault;

		counter++;

		parameter_container_delete(parameter_container);
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

	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	external_free_list_parameter();
	return 0;

error:
	external_free_list_parameter();
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
	mxml_node_t *b;
	struct parameter_container *parameter_container;
	char *parameter_name = NULL;
	char *parameter_value = NULL;
	char *parameter_key = NULL;
	char *status = NULL;
	char buf[128];
	int fault_code = FAULT_CPE_INTERNAL_ERROR;


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
			parameter_value = NULL;
		}

		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Value")) {
			parameter_value = b->value.text.string;
		}

		if (parameter_name && parameter_value) {
			if (external_set_action_write("value",
					parameter_name, parameter_value,NULL))
				goto fault;
			parameter_name = NULL;
			parameter_value = NULL;
		}
		b = mxmlWalkNext(b, session->body_in, MXML_DESCEND);
	}

	b = mxmlFindElement(session->body_in, session->body_in, "ParameterKey", NULL, NULL, MXML_DESCEND);
	if(!b) {
		fault_code = FAULT_CPE_REQUEST_DENIED;
		goto fault;
	}

	b = mxmlWalkNext(b, session->tree_in, MXML_DESCEND_FIRST);
	if (b && b->value.text.string)
		parameter_key = b->value.text.string;

	if (external_set_action_execute("value",cwmp_handle_setParamValues))
		goto fault;

	while (external_list_parameter.next != &external_list_parameter) {
		parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);

		if (parameter_container->fault_code && parameter_container->fault_code[0]=='9')
		{
			fault_code = FAULT_CPE_INVALID_ARGUMENTS;
			goto fault;
		}
	}

	external_fetch_setParamValRespStatus(&status);

	if(!status)
		goto fault;

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:SetParameterValuesResponse");
	if (!b) goto fault;

	b = mxmlNewElement(b, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, status);
	if (!b) goto fault;

	if (parameter_key != NULL) {
		sprintf(buf,"%s=%s", UCI_ACS_PARAMETERKEY_PATH, parameter_key);
		uci_set_value (buf);
		uci_commit_value();
	}

success:
	free(status);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	external_free_list_parameter();
	goto success;

error:
	external_free_list_parameter();
	free(status);
	return -1;
}

/*
 * [RPC CPE]: SetParameterAttributes
 */

int cwmp_handle_rpc_cpe_set_parameter_attributes(struct session *session, struct rpc *rpc)
{
	mxml_node_t *n, *b = session->body_in;
	char *c, *parameter_name = NULL, *parameter_notification = NULL, *attr_notification_update = NULL, *success=NULL, *fault=NULL;
	int fault_code = FAULT_CPE_INTERNAL_ERROR;

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
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "NotificationChange")) {
			attr_notification_update = b->value.text.string;
		}
		if (b && b->type == MXML_TEXT &&
			b->value.text.string &&
			b->parent->type == MXML_ELEMENT &&
			!strcmp(b->parent->value.element.name, "Notification")) {
			parameter_notification = b->value.text.string;
		}
		if (attr_notification_update && parameter_name && parameter_notification) {
			if (external_set_action_write("notification",
					parameter_name, parameter_notification, attr_notification_update))
				goto fault;
			attr_notification_update = NULL;
			parameter_name = NULL;
			parameter_notification = NULL;
		}
		b = mxmlWalkNext(b, n, MXML_DESCEND);
	}

	if (external_set_action_execute("notification", cwmp_handle_setParamAttributes))
		goto fault;

	external_fetch_setParamAttrResp(&success, &fault);

	if (fault && fault[0]=='9')
	{
		fault_code = cwmp_get_fault_code(fault);
		goto fault;
	}

	if(!success)
		goto fault;

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body", NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:SetParameterAttributesResponse");
	if (!b) goto fault;

end_success:
	free(success);
	free(fault);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	goto end_success;

error:
	free(success);
	free(fault);
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
	char *status = NULL, *instance = NULL, *fault = NULL;
	int fault_code = FAULT_CPE_INTERNAL_ERROR;

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
		if (external_object_action("add", object_name, cwmp_handle_addObject))
			goto fault;
	} else {
		fault_code = FAULT_CPE_INVALID_PARAMETER_NAME;
		goto fault;
	}

	external_fetch_addObjectResp(&instance, &status, &fault);

	if (fault && fault[0]=='9')
	{
		fault_code = cwmp_get_fault_code(fault);
		goto fault;
	}
	if (instance == NULL || status == NULL)
		goto fault;

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:AddObjectResponse");
	if (!b) goto fault;

	b = mxmlNewElement(b, "InstanceNumber");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, instance);
	if (!b) goto fault;

	b = b->parent->parent;
	b = mxmlNewElement(b, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, status);
	if (!b) goto fault;


	if (parameter_key != NULL) {
		sprintf(buf,"%s=%s", UCI_ACS_PARAMETERKEY_PATH, parameter_key);
		uci_set_value (buf);
		uci_commit_value();
	}

success:
	free(instance);
	free(fault);
	free(status);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	goto success;

error:
	free(instance);
	free(fault);
	free(status);
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
	char *status = NULL, *fault = NULL;
	int fault_code = FAULT_CPE_INTERNAL_ERROR;

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
		if (external_object_action("delete", object_name, cwmp_handle_delObject))
			goto fault;
	} else {
		fault_code = FAULT_CPE_INVALID_PARAMETER_NAME;
		goto fault;
	}

	external_fetch_delObjectResp(&status, &fault);

	if (fault && fault[0]=='9')
	{
		fault_code = cwmp_get_fault_code(fault);
		goto fault;
	}

	if (status == NULL)
		goto fault;

	b = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!b) goto fault;

	b = mxmlNewElement(b, "cwmp:DeleteObjectResponse");
	if (!b) goto fault;

	b = mxmlNewElement(b, "Status");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, status);
	if (!b) goto fault;


	if (parameter_key != NULL) {
		sprintf(buf,"%s=%s", UCI_ACS_PARAMETERKEY_PATH, parameter_key);
		uci_set_value (buf);
		uci_commit_value();
	}

success:
	free(fault);
	free(status);
	return 0;

fault:
	if (cwmp_create_fault_message(session, rpc, fault_code))
		goto error;
	goto success;

error:
	free(fault);
	free(status);
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
    time_t                          current_time;
    bool                            add_event_same_time = false;

    thread_is_working = true;
    while (list_schedule_inform.next!=&(list_schedule_inform))
    {
        schedule_inform = list_entry(list_schedule_inform.next,struct schedule_inform, list);
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
        add_event_same_time = false;
        pthread_mutex_lock (&mutex_schedule_inform);
        si_timeout.tv_sec = schedule_inform->scheduled_time;
        pthread_cond_timedwait(&threshold_schedule_inform, &mutex_schedule_inform, &si_timeout);
        pthread_mutex_unlock (&mutex_schedule_inform);
    }
    thread_is_working = false;
    bkp_session_save();
    return CWMP_OK;
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
        if (schedule_inform->scheduled_time == scheduled_time)
        {
            pthread_mutex_unlock (&mutex_schedule_inform);
            return CWMP_OK;
        }
        if (schedule_inform->scheduled_time > scheduled_time)
        {
            cond_signal = true;
            break;
        }
    }
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
    if (cond_signal)
    {
        pthread_cond_signal(&threshold_schedule_inform);
    }

    if (!thread_is_working)
    {
        thread_is_working = true;
        error = pthread_create(&scheduleInform_thread, NULL, &thread_cwmp_rpc_cpe_scheduleInform, (void *)&cwmp_main);
        if (error<0)
        {
            CWMP_LOG(ERROR,"Error error when creating the schedule event thread!");
            thread_is_working = false;
            goto fault;
        }

    }

	n = mxmlFindElement(session->tree_out, session->tree_out, "soap_env:Body",
				NULL, NULL, MXML_DESCEND);
	if (!n) goto fault;

	n = mxmlNewElement(n, "cwmp:ScheduleInformResponse");
	if (!n) goto fault;

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
    		pdownload->username, pdownload->password,
    		cwmp_handle_downloadFault);
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

	bkp_session_insert_transfer_complete(p);
	*ptransfer_complete = p;

    return error;
}

void *thread_cwmp_rpc_cpe_download (void *v)
{
    struct cwmp                     			*cwmp = (struct cwmp *)v;
    struct download          					*pdownload;
    struct timespec                 			download_timeout = {0, 0};
    time_t                          			current_time;
    int											i,error = FAULT_CPE_NO_FAULT;
    struct transfer_complete					*ptransfer_complete;
    long int									time_of_grace = 3600,timeout;
    char										*fault_code;

    thread_download_is_working = true;
    while (list_download.next!=&(list_download))
    {
    	pdownload = list_entry(list_download.next,struct download, list);
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

				bkp_session_insert_transfer_complete(ptransfer_complete);
				cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
			}
			list_del (&(pdownload->list));
			if(pdownload->scheduled_time != 0)
				count_download_queue--;
			cwmp_free_download_request(pdownload);
			pthread_mutex_unlock (&mutex_download);
		}
        if((timeout >= 0)&&(timeout <= time_of_grace))
        {
        	pthread_mutex_lock (&(cwmp->mutex_session_send));
            CWMP_LOG(INFO,"Launch download file %s",pdownload->url);
            error = cwmp_launch_download(pdownload,&ptransfer_complete);
    		if(error != FAULT_CPE_NO_FAULT)
    		{
    			cwmp_root_cause_TransferComplete (cwmp,ptransfer_complete);
    			bkp_session_delete_transfer_complete(ptransfer_complete);
    		}
    		else
    		{
				bkp_session_save();
				external_apply_download(pdownload->file_type, cwmp_handle_downloadFault);
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
        download_timeout.tv_sec = pdownload->scheduled_time;
        pthread_cond_timedwait(&threshold_download, &mutex_download, &download_timeout);
        pthread_mutex_unlock (&mutex_download);
    }
    thread_download_is_working = false;
    return CWMP_OK;
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
		error = FAULT_CPE_REQUEST_DENIED;
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

	b = mxmlNewText(b, 0, "0000-00-00T00:00:00+00:00");
	if (!b) goto fault;

	b = b->parent->parent;
	b = mxmlNewElement(t, "CompleteTime");
	if (!b) goto fault;

	b = mxmlNewText(b, 0, "0000-00-00T00:00:00+00:00");
	if (!b) goto fault;

	if(error == FAULT_CPE_NO_FAULT)
	{
		pthread_mutex_lock (&mutex_download);
		if(download_delay != 0)
			scheduled_time = time(NULL) + download_delay;

		list_for_each(ilist,&(list_download))
		{
			idownload = list_entry(ilist,struct download, list);
			if (idownload->scheduled_time == scheduled_time)
			{
				pthread_mutex_unlock (&mutex_download);
				return 0;
			}
			if (idownload->scheduled_time > scheduled_time)
			{
				cond_signal = true;
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
		CWMP_LOG(INFO,"Download will start in %us",download_delay);

		pthread_mutex_unlock (&mutex_download);
		if (cond_signal)
		{
			pthread_cond_signal(&threshold_download);
		}
		if (!thread_download_is_working)
		{
			thread_download_is_working = true;
			error = pthread_create(&download_thread, NULL, &thread_cwmp_rpc_cpe_download, (void *)&cwmp_main);
			if (error<0)
			{
				CWMP_LOG(ERROR,"Error when creating the download thread!");
				thread_download_is_working = false;
				return -1;
			}
		}
	}

	return 0;

fault:
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
	struct parameter_container *parameter_container;
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
		while (external_list_parameter.next != &external_list_parameter) {
			parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);

			if (parameter_container->fault_code && parameter_container->fault_code[0]=='9')
			{
				idx = cwmp_get_fault_code(parameter_container->fault_code);

				t = mxmlNewElement(b, "SetParameterValuesFault");
				if (!t) return -1;

				u = mxmlNewElement(t, "ParameterName");
				if (!u) return -1;

				u = mxmlNewText(u, 0, parameter_container->name);
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
			parameter_container_delete(parameter_container);
		}
	}

	return 0;
}

int cwmp_get_fault_code (char *fault_code)
{
	int i;

	for (i=1; i<__FAULT_CPE_MAX; i++)
	{
		if (strcmp(FAULT_CPE_ARRAY[i].CODE, fault_code) == 0)
			break;
	}

	if(i == __FAULT_CPE_MAX)
		i = FAULT_CPE_INTERNAL_ERROR;

	return i;
}

int cwmp_create_fault_message(struct session *session, struct rpc *rpc_cpe, int fault_code)
{
	CWMP_LOG (INFO,"Fault in the received RPC");
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
