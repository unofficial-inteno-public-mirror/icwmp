/*
    SetParameterAttributes.c

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
struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterAttributes (struct session *session);
int cwmp_rpc_cpe_setParameterAttributes (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterAttributes_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterAttributes_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterAttributes_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);

extern struct FAULT_CPE FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
extern const struct CPE_METHOD_CONSTRUCTORS CPE_METHOD_CONSTRUCTORS_ARRAY [COUNT_RPC_CPE];


struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterAttributes (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterAttributes));
    rpc_cpe->method                                 = cwmp_rpc_cpe_setParameterAttributes;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterAttributesResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_setParameterAttributes_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_setParameterAttributes_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_setParameterAttributes_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:SetParameterAttributes";
    soap_methods->envelope_response                 = "cwmp:SetParameterAttributesResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__SetParameterAttributesResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__SetParameterAttributesResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__SetParameterAttributes;
    return rpc_cpe;
}

int cwmp_rpc_cpe_setParameterAttributes (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{

    struct _cwmp1__SetParameterAttributes           *p_soap_cwmp1__SetParameterAttributes;
    int                                             i,size,error;
    struct cwmp1__SetParameterAttributesStruct      *ParameterAttributesStruct,**ptrParameterAttributeStruct;
    char *fault = NULL;

    p_soap_cwmp1__SetParameterAttributes        = (struct _cwmp1__SetParameterAttributes *)this->method_data;
    size                                        = p_soap_cwmp1__SetParameterAttributes->ParameterList->__size;
    ptrParameterAttributeStruct                 = p_soap_cwmp1__SetParameterAttributes->ParameterList->__ptrSetParameterAttributesStruct;

    for (i=0;i<size;i++,ptrParameterAttributeStruct++)
	{
    	ParameterAttributesStruct = *ptrParameterAttributeStruct;
    	CWMP_LOG(INFO,"param[%d] = \"%s\"",i,*(ParameterAttributesStruct->Name));
		if (external_set_action_write("notification",*(ParameterAttributesStruct->Name), NULL))
		{
			if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
			{
				return CWMP_GEN_ERR;
			}
			return CWMP_FAULT_CPE;
		}
	}
    if (external_set_action_execute("notification"))
	{
		if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
		{
			return CWMP_GEN_ERR;
		}
		return CWMP_FAULT_CPE;
	}
    external_fetch_setParamAttrRespFault(&fault);
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
    return CWMP_OK;
}

int cwmp_rpc_cpe_setParameterAttributes_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_setParameterAttributes_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send SetParameterAttributes response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_setParameterAttributes_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}
