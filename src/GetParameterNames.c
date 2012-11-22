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
#include "list.h"
#include "dm.h"
#include "dm_rpc.h"

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterNames (struct session *session);
int cwmp_rpc_cpe_getParameterNames (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterNames_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterNames_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterNames_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_dm_getParameterNames(struct cwmp *cwmp, char *path, struct list_head *list, int *n, enum xsd__boolean NextLevel);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
static int free_list_getParameterNames(struct list_head *list);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);

extern const struct CPE_METHOD_CONSTRUCTORS CPE_METHOD_CONSTRUCTORS_ARRAY [COUNT_RPC_CPE];

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
    char                                        *ParameterPath;
    enum xsd__boolean                           NextLevel;
    int                                         i,size,error;
    LIST_HEAD(list_ParameterInfoStruct);

    p_soap_cwmp1__GetParameterNames         = (struct _cwmp1__GetParameterNames *)this->method_data;
    p_soap_cwmp1__GetParameterNamesResponse = (struct _cwmp1__GetParameterNamesResponse *)this->method_response_data;
    ParameterPath                           = *(p_soap_cwmp1__GetParameterNames->ParameterPath);
    NextLevel                               = p_soap_cwmp1__GetParameterNames->NextLevel;

    CWMP_LOG(INFO,"ParameterPath = \"%s\"", ParameterPath);
    error = cwmp_dm_getParameterNames(cwmp, ParameterPath, &list_ParameterInfoStruct, &size,NextLevel);
    if (error != FAULT_CPE_NO_FAULT_IDX)
    {
        free_list_getParameterNames(&list_ParameterInfoStruct);
        if (cwmp_add_session_rpc_cpe_Fault(session,error)==NULL)
        {
            return CWMP_GEN_ERR;
        }
        return CWMP_FAULT_CPE;
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
    while(list_ParameterInfoStruct.next!=&list_ParameterInfoStruct)
    {
        struct handler_ParameterInfoStruct *handler_ParameterInfoStruct;
        handler_ParameterInfoStruct     = list_entry(list_ParameterInfoStruct.next,struct handler_ParameterInfoStruct,list);
        *ptrParameterInfoStruct         = handler_ParameterInfoStruct->ParameterInfoStruct;
        ptrParameterInfoStruct++;
        list_del(list_ParameterInfoStruct.next);
        free (handler_ParameterInfoStruct);
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

static int free_list_getParameterNames(struct list_head *list)
{
    struct handler_ParameterInfoStruct      *handler_ParameterInfoStruct;
    struct cwmp1__ParameterInfoStruct       *ParameterInfoStruct;
    while(list->next!=list)
    {
        handler_ParameterInfoStruct     = list_entry(list->next,struct handler_ParameterInfoStruct,list);
        ParameterInfoStruct             = handler_ParameterInfoStruct->ParameterInfoStruct;
        if (ParameterInfoStruct!=NULL)
        {
            if (ParameterInfoStruct->Name!=NULL)
            {
                free(ParameterInfoStruct->Name);
            }
            free(ParameterInfoStruct);
        }
        list_del(list->next);
        free (handler_ParameterInfoStruct);
    }
    return CWMP_OK;
}
