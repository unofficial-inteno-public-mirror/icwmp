/*
    AddObject.c

    cwmp service client in C

--------------------------------------------------------------------------------
cwmp service client
Copyright (C) 20011-2012, Inteno, Inc. All Rights Reserved.

Any distribution, dissemination, modification, conversion, integral or partial
reproduction, can not be made without the prior written permission of Inteno.
--------------------------------------------------------------------------------
Author contact information:

--------------------------------------------------------------------------------
*/

#include "soapH.h"
#include "cwmp.h"
#include "external.h"

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_addObject (struct session *session);
int cwmp_rpc_cpe_addObject (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_addObject_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_addObject_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_addObject_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);

extern struct FAULT_CPE FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];

struct rpc_cpe *cwmp_add_session_rpc_cpe_addObject (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__AddObject));
    rpc_cpe->method                                 = cwmp_rpc_cpe_addObject;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__AddObjectResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_addObject_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_addObject_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_addObject_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:AddObject";
    soap_methods->envelope_response                 = "cwmp:AddObjectResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__AddObjectResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__AddObjectResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__AddObject;
    return rpc_cpe;
}

int cwmp_rpc_cpe_addObject (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_addObject_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__AddObject            *p_soap_cwmp1__AddObject;
    struct _cwmp1__AddObjectResponse    *p_soap_cwmp1__AddObjectResponse;
    char                                *instance=NULL, *status=NULL, *fault=NULL, buf[128];
    int                                 i,error;

    p_soap_cwmp1__AddObject         = (struct _cwmp1__AddObject*)this->method_data;
    p_soap_cwmp1__AddObjectResponse = (struct _cwmp1__AddObjectResponse*)this->method_response_data;

    CWMP_LOG(INFO,"ObjectName = \"%s\"", p_soap_cwmp1__AddObject->ObjectName);

	if (external_object_action("add", p_soap_cwmp1__AddObject->ObjectName))
	{
		if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
		{
			return CWMP_GEN_ERR;
		}
		return CWMP_FAULT_CPE;
	}
	external_fetch_addObjectResp(&instance, &status, &fault);
	if (fault && fault[0]=='9')
	{
		error = FAULT_CPE_INTERNAL_ERROR_IDX;
		for (i=1; i<FAULT_CPE_ARRAY_SIZE; i++)
		{
			if (strcmp(fault, FAULT_CPE_ARRAY[i].CODE)==0)
			{
				error = i;
				break;
			}
		}
		free(fault);
		if (cwmp_add_session_rpc_cpe_Fault(session,error)==NULL)
		{
			return CWMP_GEN_ERR;
		}
		return CWMP_FAULT_CPE;

	}
	if (instance==NULL || status==NULL)
	{
		if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
		{
			return CWMP_GEN_ERR;
		}
		return CWMP_FAULT_CPE;
	}
	p_soap_cwmp1__AddObjectResponse->InstanceNumber = atoi(instance);
	p_soap_cwmp1__AddObjectResponse->Status         = atoi(status);

	sprintf(buf,"%s=%s",UCI_ACS_PARAMETERKEY_PATH,p_soap_cwmp1__AddObject->ParameterKey);
	uci_set_value (buf);
	uci_commit_value();

    return CWMP_OK;
}

int cwmp_rpc_cpe_addObject_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send AddObject response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_addObject_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}
