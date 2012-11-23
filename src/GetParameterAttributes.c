/*
    GetParameterAttributes.c

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
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterAttributes (struct session *session);
int cwmp_rpc_cpe_getParameterAttributes (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterAttributes_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterAttributes_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterAttributes_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
extern struct FAULT_CPE FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
extern struct list_head external_list_parameter;

struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterAttributes (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__GetParameterAttributes));
    rpc_cpe->method                                 = cwmp_rpc_cpe_getParameterAttributes;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__GetParameterAttributesResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_getParameterAttributes_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_getParameterAttributes_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_getParameterAttributes_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:GetParameterAttributes";
    soap_methods->envelope_response                 = "cwmp:GetParameterAttributesResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__GetParameterAttributesResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__GetParameterAttributesResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__GetParameterAttributes;
    return rpc_cpe;
}

int cwmp_rpc_cpe_getParameterAttributes (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_getParameterAttributes_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__GetParameterAttributes           *p_soap_cwmp1__GetParameterAttributes;
    struct _cwmp1__GetParameterAttributesResponse   *p_soap_cwmp1__GetParameterAttributesResponse;
    struct cwmp1__ParameterAttributeStruct          **ptr_ParameterAttributeStruct;
    struct cwmp1ParameterAttributeList              *ParameterList;
    struct cwmp1AccessList 							*AccessList;
    struct list_head 								*list;
    struct external_parameter						*external_parameter;
    char                                            **name;
    int                                             i,n,size,error;
    int                                             size_response = 0;

    p_soap_cwmp1__GetParameterAttributes         = (struct _cwmp1__GetParameterAttributes *)this->method_data;
    p_soap_cwmp1__GetParameterAttributesResponse = (struct _cwmp1__GetParameterAttributesResponse *)this->method_response_data;
    name = p_soap_cwmp1__GetParameterAttributes->ParameterNames->__ptrstring;
    size = p_soap_cwmp1__GetParameterAttributes->ParameterNames->__size;


    for (i=0;i<size;i++,name++)
	{
		CWMP_LOG(INFO,"param[%d] = \"%s\"",i,*name);
		if (external_get_action_write("notification",*name, NULL))
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
		size_response++;
	}

	ParameterList                = calloc(1,sizeof(struct cwmp1ParameterAttributeList));
	ptr_ParameterAttributeStruct = calloc(size_response, sizeof(struct cwmp1__ParameterAttributeStruct *));
	if(ParameterList == NULL ||
	   ptr_ParameterAttributeStruct == NULL)
	{
		return CWMP_MEM_ERR;
	}

	ParameterList->__size                                       = size_response;
	ParameterList->__ptrParameterAttributeStruct                = ptr_ParameterAttributeStruct;
	p_soap_cwmp1__GetParameterAttributesResponse->ParameterList = ParameterList;


	while (external_list_parameter.next!=&external_list_parameter) {
		external_parameter = list_entry(external_list_parameter.next, struct external_parameter, list);
		AccessList = calloc(1,sizeof(struct cwmp1AccessList));
		AccessList->__ptrstring = calloc(1,sizeof(char *));
		*(AccessList->__ptrstring) = strdup("Subscriber");
		AccessList->__size = 1;
		*ptr_ParameterAttributeStruct = calloc(1,sizeof(struct cwmp1__ParameterAttributeStruct));
		(*ptr_ParameterAttributeStruct)->Name = external_parameter->name;
		(*ptr_ParameterAttributeStruct)->Notification = atoi(external_parameter->data);
		(*ptr_ParameterAttributeStruct)->AccessList = AccessList;
		ptr_ParameterAttributeStruct++;

		list_del(&external_parameter->list);
		FREE(external_parameter->fault_code);
		FREE(external_parameter->type);
		FREE(external_parameter->data);
		FREE(external_parameter);
	}

    return CWMP_OK;
}

int cwmp_rpc_cpe_getParameterAttributes_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send GetParameterAttributes response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}
int cwmp_rpc_cpe_getParameterAttributes_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__GetParameterAttributesResponse   *p_soap_cwmp1__GetParameterAttributesResponse;
    struct cwmp1__ParameterAttributeStruct          **ptrParameterAttributeStruct,*pParameterAttributeStruct;
    char                                            **ptrAccessList;
    int                                             i,size_ParameterAttributeStruct;

    p_soap_cwmp1__GetParameterAttributesResponse = (struct _cwmp1__GetParameterAttributesResponse *)this->method_response_data;

    if(p_soap_cwmp1__GetParameterAttributesResponse->ParameterList != NULL)
    {
        size_ParameterAttributeStruct   = p_soap_cwmp1__GetParameterAttributesResponse->ParameterList->__size;
        ptrParameterAttributeStruct     = p_soap_cwmp1__GetParameterAttributesResponse->ParameterList->__ptrParameterAttributeStruct;

        for(i=0;i<size_ParameterAttributeStruct;i++)
        {
            pParameterAttributeStruct = *ptrParameterAttributeStruct;
            if(pParameterAttributeStruct != NULL)
            {
                if(pParameterAttributeStruct->Name != NULL)
                {
                    free(pParameterAttributeStruct->Name);
                }
                if(pParameterAttributeStruct->AccessList != NULL)
                {
                	free(*(pParameterAttributeStruct->AccessList->__ptrstring));
                	free(pParameterAttributeStruct->AccessList->__ptrstring);
                    free(pParameterAttributeStruct->AccessList);
                }

                ptrParameterAttributeStruct++;
                free(pParameterAttributeStruct);
            }
        }
        free(p_soap_cwmp1__GetParameterAttributesResponse->ParameterList);
    }
    return CWMP_OK;
}
