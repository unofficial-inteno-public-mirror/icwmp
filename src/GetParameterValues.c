/*
    GetParameterValues.c

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
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterValues (struct session *session);
int cwmp_rpc_cpe_getParameterValues (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterValues_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterValues_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterValues_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);

extern const struct CPE_METHOD_CONSTRUCTORS CPE_METHOD_CONSTRUCTORS_ARRAY [COUNT_RPC_CPE];
extern struct FAULT_CPE FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
extern struct list_head external_list_parameter;

struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterValues (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__GetParameterValues));
    rpc_cpe->method                                 = cwmp_rpc_cpe_getParameterValues;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__GetParameterValuesResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_getParameterValues_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_getParameterValues_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_getParameterValues_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:GetParameterValues";
    soap_methods->envelope_response                 = "cwmp:GetParameterValuesResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__GetParameterValuesResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__GetParameterValuesResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__GetParameterValues;
    return rpc_cpe;
}

int cwmp_rpc_cpe_getParameterValues (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__GetParameterValues           *p_soap_cwmp1__GetParameterValues;
    struct _cwmp1__GetParameterValuesResponse   *p_soap_cwmp1__GetParameterValuesResponse;
    struct cwmp1ParameterValueList              *ParameterList;
    struct cwmp1__ParameterValueStruct          **ptr_ParameterValueStruct;
    struct list_head 							*list;
    struct external_parameter					*external_parameter;
    char                                        **name;
    int                                         i,n,size,error;
    int                                         size_resp=0;

    p_soap_cwmp1__GetParameterValues            = (struct _cwmp1__GetParameterValues *)this->method_data;
    p_soap_cwmp1__GetParameterValuesResponse    = (struct _cwmp1__GetParameterValuesResponse *)this->method_response_data;
    name                                        = p_soap_cwmp1__GetParameterValues->ParameterNames->__ptrstring;
    size                                        = p_soap_cwmp1__GetParameterValues->ParameterNames->__size;

    for (i=0;i<size;i++,name++)
    {
        CWMP_LOG(INFO,"param[%d] = \"%s\"",i,*name);
        if (external_get_action_write("value",*name, NULL))
        {
        	external_free_list_parameter();
            if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
            {
                return CWMP_GEN_ERR;
            }
            return CWMP_FAULT_CPE;
        }
    }

    if (external_get_action_execute())
    {
    	external_free_list_parameter();
		if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INTERNAL_ERROR_IDX)==NULL)
		{
			return CWMP_GEN_ERR;
		}
		return CWMP_FAULT_CPE;
    }

    list_for_each(list,&external_list_parameter) {
    	external_parameter = list_entry(list, struct external_parameter, list);
    	if (external_parameter->fault_code &&
    			external_parameter->fault_code[0]=='9')
    	{
    		error = FAULT_CPE_INTERNAL_ERROR_IDX;
    		for (i=1; i<FAULT_CPE_ARRAY_SIZE; i++)
    		{
    			if (strcmp(external_parameter->fault_code, FAULT_CPE_ARRAY[i].CODE)==0)
    			{
    				error = i;
    				break;
    			}
    		}
    		external_free_list_parameter();
			if (cwmp_add_session_rpc_cpe_Fault(session,error)==NULL)
			{
				return CWMP_GEN_ERR;
			}
			return CWMP_FAULT_CPE;

    	}
    	size_resp++;
	}

    ParameterList               = calloc(1,sizeof(struct cwmp1ParameterValueList));
    ptr_ParameterValueStruct    = calloc(size_resp,sizeof(struct cwmp1__ParameterValueStruct *));
    if (ParameterList == NULL ||
        ptr_ParameterValueStruct == NULL)
    {
        return CWMP_MEM_ERR;
    }
    ParameterList->__size                                   = size_resp;
    ParameterList->__ptrParameterValueStruct                = ptr_ParameterValueStruct;
    p_soap_cwmp1__GetParameterValuesResponse->ParameterList = ParameterList;

    while (external_list_parameter.next!=&external_list_parameter) {
		external_parameter = list_entry(external_list_parameter.next, struct external_parameter, list);
		*ptr_ParameterValueStruct = calloc(1,sizeof(struct cwmp1__ParameterValueStruct));
		(*ptr_ParameterValueStruct)->Name = external_parameter->name;
		(*ptr_ParameterValueStruct)->Value = external_parameter->data;
		(*ptr_ParameterValueStruct)->Type = external_parameter->type;
		ptr_ParameterValueStruct++;

		list_del(&external_parameter->list);
		FREE(external_parameter->fault_code);
		FREE(external_parameter);
	}

    return CWMP_OK;
}

int cwmp_rpc_cpe_getParameterValues_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}
int cwmp_rpc_cpe_getParameterValues_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send GetParameterValues response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}
int cwmp_rpc_cpe_getParameterValues_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__GetParameterValues           *p_soap_cwmp1__GetParameterValues;
    struct _cwmp1__GetParameterValuesResponse   *p_soap_cwmp1__GetParameterValuesResponse;
    struct cwmp1__ParameterValueStruct          **ptr_ParameterValueStruct,*ParameterValueStruct;
    int                                         i;

    p_soap_cwmp1__GetParameterValuesResponse = (struct _cwmp1__GetParameterValuesResponse *)this->method_response_data;
    if (p_soap_cwmp1__GetParameterValuesResponse!=NULL)
    {
        if (p_soap_cwmp1__GetParameterValuesResponse->ParameterList != NULL)
        {
            if(p_soap_cwmp1__GetParameterValuesResponse->ParameterList->__ptrParameterValueStruct != NULL)
            {
                ptr_ParameterValueStruct = p_soap_cwmp1__GetParameterValuesResponse->ParameterList->__ptrParameterValueStruct;
                for (i=0;i<p_soap_cwmp1__GetParameterValuesResponse->ParameterList->__size;i++,ptr_ParameterValueStruct++)
                {
                    ParameterValueStruct = *ptr_ParameterValueStruct;
                    if (ParameterValueStruct!=NULL)
                    {
						free(ParameterValueStruct->Name);
						free(ParameterValueStruct->Value);
						free(ParameterValueStruct->Type);
                        free(ParameterValueStruct);
                    }
                }
                free (p_soap_cwmp1__GetParameterValuesResponse->ParameterList->__ptrParameterValueStruct);
            }
            free (p_soap_cwmp1__GetParameterValuesResponse->ParameterList);
        }
    }
    return CWMP_OK;
}
