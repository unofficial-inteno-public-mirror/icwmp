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
#include "list.h"
#include "dm.h"
#include "dm_rpc.h"

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterValues (struct session *session);
int cwmp_rpc_cpe_getParameterValues (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterValues_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterValues_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterValues_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_dm_getParameterValues(struct cwmp *cwmp, char *name, struct list_head *list, int *n);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
int free_list_getParameterValues(struct list_head *list);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);

extern const struct CPE_METHOD_CONSTRUCTORS CPE_METHOD_CONSTRUCTORS_ARRAY [COUNT_RPC_CPE];

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
    char                                        **name;
    int                                         i,n,size,error;
    int                                         size_resp=0;
    LIST_HEAD(list_ParameterValueStruct);

    p_soap_cwmp1__GetParameterValues            = (struct _cwmp1__GetParameterValues *)this->method_data;
    p_soap_cwmp1__GetParameterValuesResponse    = (struct _cwmp1__GetParameterValuesResponse *)this->method_response_data;
    name                                        = p_soap_cwmp1__GetParameterValues->ParameterNames->__ptrstring;
    size                                        = p_soap_cwmp1__GetParameterValues->ParameterNames->__size;

    for (i=0;i<size;i++,name++)
    {
        CWMP_LOG(INFO,"param[%d] = \"%s\"",i,*name);
        error = cwmp_dm_getParameterValues(cwmp, *name, &list_ParameterValueStruct,&n);
        if (error != FAULT_CPE_NO_FAULT_IDX)
        {
            free_list_getParameterValues(&list_ParameterValueStruct);
            if (cwmp_add_session_rpc_cpe_Fault(session,error)==NULL)
            {
                return CWMP_GEN_ERR;
            }
            return CWMP_FAULT_CPE;
        }
        size_resp += n;
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
    while(list_ParameterValueStruct.next!=&list_ParameterValueStruct)
    {
        struct handler_ParameterValueStruct *handler_ParameterValueStruct;
        handler_ParameterValueStruct    = list_entry(list_ParameterValueStruct.next,struct handler_ParameterValueStruct,list);
        *ptr_ParameterValueStruct       = handler_ParameterValueStruct->ParameterValueStruct;
        ptr_ParameterValueStruct++;
        list_del(list_ParameterValueStruct.next);
        free (handler_ParameterValueStruct);
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
                        if (ParameterValueStruct->Name!=NULL)
                        {
                            free(ParameterValueStruct->Name);
                        }
                        if (ParameterValueStruct->Value!=NULL)
                        {
                            free(ParameterValueStruct->Value);
                        }
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

int free_list_getParameterValues(struct list_head *list)
{
    struct handler_ParameterValueStruct     *handler_ParameterValueStruct;
    struct cwmp1__ParameterValueStruct      *ParameterValueStruct;
    while(list->next!=list)
    {
        handler_ParameterValueStruct    = list_entry(list->next,struct handler_ParameterValueStruct,list);
        ParameterValueStruct            = handler_ParameterValueStruct->ParameterValueStruct;
        if (ParameterValueStruct!=NULL)
        {
            if (ParameterValueStruct->Name!=NULL)
            {
                free(ParameterValueStruct->Name);
            }
            if (ParameterValueStruct->Value!=NULL)
            {
                free(ParameterValueStruct->Value);
            }
            free(ParameterValueStruct);
        }
        list_del(list->next);
        free (handler_ParameterValueStruct);
    }
    return CWMP_OK;
}
