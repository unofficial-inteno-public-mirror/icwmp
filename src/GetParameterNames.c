/*
    GetParameterNames.c

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

#include "soapH.h"
#include "cwmp.h"
#include "external.h"

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterNames (struct session *session);
int cwmp_rpc_cpe_getParameterNames (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterNames_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterNames_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterNames_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);

extern const struct CPE_METHOD_CONSTRUCTORS CPE_METHOD_CONSTRUCTORS_ARRAY [COUNT_RPC_CPE];
extern struct FAULT_CPE FAULT_CPE_ARRAY [FAULT_CPE_ARRAY_SIZE];
extern struct list_head external_list_parameter;

struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterNames (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__GetParameterNames));
    rpc_cpe->method                                 = cwmp_rpc_cpe_getParameterNames;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__GetParameterNamesResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_getParameterNames_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_getParameterNames_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_getParameterNames_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:GetParameterNames";
    soap_methods->envelope_response                 = "cwmp:GetParameterNamesResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__GetParameterNamesResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__GetParameterNamesResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__GetParameterNames;
    return rpc_cpe;
}

int cwmp_rpc_cpe_getParameterNames (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__GetParameterNames            *p_soap_cwmp1__GetParameterNames;
    struct _cwmp1__GetParameterNamesResponse    *p_soap_cwmp1__GetParameterNamesResponse;
    struct cwmp1ParameterInfoList               *ParameterList;
    struct cwmp1__ParameterInfoStruct           **ptrParameterInfoStruct;
    struct cwmp1__ParameterInfoStruct           *ParameterInfoStruct;
    struct external_parameter 					*external_parameter;
    struct list_head 							*list;
    char                                        *ParameterPath;
    char                                        NextLevel[2];
    int                                         i,size=0,error;

    p_soap_cwmp1__GetParameterNames         = (struct _cwmp1__GetParameterNames *)this->method_data;
    p_soap_cwmp1__GetParameterNamesResponse = (struct _cwmp1__GetParameterNamesResponse *)this->method_response_data;
    ParameterPath                           = *(p_soap_cwmp1__GetParameterNames->ParameterPath);
    sprintf(NextLevel,"%u",p_soap_cwmp1__GetParameterNames->NextLevel);

    CWMP_LOG(INFO,"ParameterPath = \"%s\"", ParameterPath);
    if (external_get_action("name", ParameterPath, NextLevel))
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
    		error = 0;
    		for (i=1; i<FAULT_CPE_ARRAY_SIZE; i++)
    		{
    			if (strcmp(external_parameter->fault_code, FAULT_CPE_ARRAY[i].CODE)==0)
    			{
    				error = i;
    				break;
    			}
    		}
    		if(!error) error = FAULT_CPE_INTERNAL_ERROR_IDX;
    		external_free_list_parameter();
			if (cwmp_add_session_rpc_cpe_Fault(session,error)==NULL)
			{
				return CWMP_GEN_ERR;
			}
			return CWMP_FAULT_CPE;

    	}
    	size++;
	}

    ParameterList               = calloc(1,sizeof(struct cwmp1ParameterInfoList));
    ptrParameterInfoStruct      = calloc(size,sizeof(struct cwmp1__ParameterInfoStruct *));
    if (ParameterList == NULL ||
        ptrParameterInfoStruct == NULL)
    {
        return CWMP_MEM_ERR;
    }
    ParameterList->__size                                   = size;
    ParameterList->__ptrParameterInfoStruct                 = ptrParameterInfoStruct;
    p_soap_cwmp1__GetParameterNamesResponse->ParameterList  = ParameterList;
    while (external_list_parameter.next!=&external_list_parameter) {
   		external_parameter = list_entry(external_list_parameter.next, struct external_parameter, list);
        *ptrParameterInfoStruct         = calloc(1,sizeof(struct cwmp1__ParameterInfoStruct));
        (*ptrParameterInfoStruct)->Name = external_parameter->name;
        (*ptrParameterInfoStruct)->Writable = atoi(external_parameter->data);
        ptrParameterInfoStruct++;

        list_del(&external_parameter->list);
		FREE(external_parameter->data);
		FREE(external_parameter->type);
		FREE(external_parameter->fault_code);
		FREE(external_parameter);
    }
    return CWMP_OK;
}

int cwmp_rpc_cpe_getParameterNames_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}
int cwmp_rpc_cpe_getParameterNames_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send GetParameterNames response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}
int cwmp_rpc_cpe_getParameterNames_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__GetParameterNames            *p_soap_cwmp1__GetParameterNames;
    struct _cwmp1__GetParameterNamesResponse    *p_soap_cwmp1__GetParameterNamesResponse;
    struct cwmp1ParameterInfoList               *ParameterList;
    struct cwmp1__ParameterInfoStruct           **ptrParameterInfoStruct,*ParameterInfoStruct;
    int                                         i;

    p_soap_cwmp1__GetParameterNamesResponse = (struct _cwmp1__GetParameterNamesResponse *)this->method_response_data;
    if (p_soap_cwmp1__GetParameterNamesResponse!=NULL)
    {
        if (p_soap_cwmp1__GetParameterNamesResponse->ParameterList != NULL)
        {
            if(p_soap_cwmp1__GetParameterNamesResponse->ParameterList->__ptrParameterInfoStruct != NULL)
            {
                ptrParameterInfoStruct = p_soap_cwmp1__GetParameterNamesResponse->ParameterList->__ptrParameterInfoStruct;
                for (i=0;i<p_soap_cwmp1__GetParameterNamesResponse->ParameterList->__size;i++,ptrParameterInfoStruct++)
                {
                    ParameterInfoStruct = *ptrParameterInfoStruct;
                    if (ParameterInfoStruct!=NULL)
                    {
                        if (ParameterInfoStruct->Name!=NULL)
                        {
                            free(ParameterInfoStruct->Name);
                        }
                        free(ParameterInfoStruct);
                    }
                }
                free (p_soap_cwmp1__GetParameterNamesResponse->ParameterList->__ptrParameterInfoStruct);
            }
            free (p_soap_cwmp1__GetParameterNamesResponse->ParameterList);
        }
    }
    return CWMP_OK;
}

