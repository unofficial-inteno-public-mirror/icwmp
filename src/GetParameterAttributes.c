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
#include "list.h"
#include "dm.h"
#include "dm_rpc.h"

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getParameterAttributes (struct session *session);
int cwmp_rpc_cpe_getParameterAttributes (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterAttributes_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterAttributes_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getParameterAttributes_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
int cwmp_dm_getParameterAttributes(struct cwmp *cwmp, char *path, struct list_head *list, int *n);
int free_list_getParameterAttributes(struct list_head *list);

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
    char                                            **name;
    int                                             i,n,size,error;
    int                                             size_response = 0;
    LIST_HEAD(list_ParameterAttributesStruct);

    p_soap_cwmp1__GetParameterAttributes         = (struct _cwmp1__GetParameterAttributes *)this->method_data;
    p_soap_cwmp1__GetParameterAttributesResponse = (struct _cwmp1__GetParameterAttributesResponse *)this->method_response_data;
    name = p_soap_cwmp1__GetParameterAttributes->ParameterNames->__ptrstring;
    size = p_soap_cwmp1__GetParameterAttributes->ParameterNames->__size;

    for(i=0;i<size;i++,name++)
    {
        error = cwmp_dm_getParameterAttributes(cwmp, *name, &list_ParameterAttributesStruct,&n);
        if (error != FAULT_CPE_NO_FAULT_IDX)
        {
            free_list_getParameterAttributes(&list_ParameterAttributesStruct);
            if (cwmp_add_session_rpc_cpe_Fault(session,error)==NULL)
            {
                return CWMP_GEN_ERR;
            }
            return CWMP_FAULT_CPE;
        }
        size_response += n;
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

    while(!list_empty(&list_ParameterAttributesStruct))
    {
        struct handler_ParameterAttributeStruct *handler_ParameterAttributeStruct;
        handler_ParameterAttributeStruct    = list_entry (list_ParameterAttributesStruct.next, struct handler_ParameterAttributeStruct, list);
        *ptr_ParameterAttributeStruct       = handler_ParameterAttributeStruct->ParameterAttributeStruct;
        ptr_ParameterAttributeStruct++;
        list_del(list_ParameterAttributesStruct.next);
        free(handler_ParameterAttributeStruct);
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
    char                                            **ptrAccessList,*pAccessList;
    int                                             i,j,size_ParameterAttributeStruct,size_AccessList;

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
                    size_AccessList = pParameterAttributeStruct->AccessList->__size;
                    ptrAccessList   = pParameterAttributeStruct->AccessList->__ptrstring;
                    for(j=0;j<size_AccessList;j++)
                    {
                        pAccessList = *ptrAccessList;
                        ptrAccessList++;
                        if(pAccessList != NULL)
                        {
                            free(pAccessList);
                        }
                    }
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

int free_list_getParameterAttributes(struct list_head *list)
{
    struct handler_ParameterAttributeStruct     *handler_ParameterAttributeStruct;
    struct cwmp1__ParameterAttributeStruct      *ParameterAttributeStruct;
    int                                         i;
    char                                        **pAccessList;

    while(list->next!=list)
    {
        handler_ParameterAttributeStruct = list_entry (list->next, struct handler_ParameterAttributeStruct, list);
        ParameterAttributeStruct         = handler_ParameterAttributeStruct->ParameterAttributeStruct;
        if (ParameterAttributeStruct!=NULL)
        {
            if (ParameterAttributeStruct->Name!=NULL)
            {
                free(ParameterAttributeStruct->Name);
            }
            if (ParameterAttributeStruct->AccessList!=NULL)
            {
                if (ParameterAttributeStruct->AccessList->__ptrstring!=NULL)
                {
                    pAccessList = ParameterAttributeStruct->AccessList->__ptrstring;
                    for(i=0;i<ParameterAttributeStruct->AccessList->__size;i++,pAccessList++)
                    {
                        if(*pAccessList!=NULL)
                        {
                            free(*pAccessList);
                        }
                    }
                    free(ParameterAttributeStruct->AccessList->__ptrstring);
                }
                free(ParameterAttributeStruct->AccessList);
            }
            free(ParameterAttributeStruct);
        }
        list_del(list->next);
        free (handler_ParameterAttributeStruct);
    }
    return CWMP_OK;
}
