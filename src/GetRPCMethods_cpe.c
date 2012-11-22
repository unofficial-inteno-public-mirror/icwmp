/*
    GetRPCMethods_cpe.c

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

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_getRPCMethods (struct session *session);
int cwmp_rpc_cpe_getRPCMethods (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getRPCMethods_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getRPCMethods_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_getRPCMethods_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
extern const struct CPE_METHOD_CONSTRUCTORS CPE_METHOD_CONSTRUCTORS_ARRAY [COUNT_RPC_CPE];

struct rpc_cpe *cwmp_add_session_rpc_cpe_getRPCMethods (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__GetRPCMethods));
    rpc_cpe->method                                 = cwmp_rpc_cpe_getRPCMethods;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__GetRPCMethodsResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_getRPCMethods_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_getRPCMethods_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_getRPCMethods_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:GetRPCMethods";
    soap_methods->envelope_response                 = "cwmp:GetRPCMethodsResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__GetRPCMethodsResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__GetRPCMethodsResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__GetRPCMethods;
    return rpc_cpe;
}

int cwmp_rpc_cpe_getRPCMethods (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_getRPCMethods_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int                                     i;
    struct cwmp1MethodList                  *MethodList;
    char                                    **mlstruct;
    struct _cwmp1__GetRPCMethodsResponse    *p_soap_cwmp1__GetRPCMethodsResponse;

    MethodList = calloc(1,sizeof(struct cwmp1MethodList));
    mlstruct = calloc(COUNT_RPC_CPE,sizeof(char *));
    if(MethodList == NULL ||
       mlstruct == NULL)
    {
        return CWMP_MEM_ERR;
    }
    MethodList->__ptrstring = mlstruct;
    for(i=0;i<COUNT_RPC_CPE;i++)
    {
        if(CPE_METHOD_CONSTRUCTORS_ARRAY[i].CONSTRUCTOR != NULL)
        {
            *mlstruct = CPE_METHOD_CONSTRUCTORS_ARRAY[i].METHOD;
            MethodList->__size++;
            mlstruct++;
        }
    }


    p_soap_cwmp1__GetRPCMethodsResponse             = (struct _cwmp1__GetRPCMethodsResponse *)this->method_response_data;
    p_soap_cwmp1__GetRPCMethodsResponse->MethodList = MethodList;
    return CWMP_OK;
}

int cwmp_rpc_cpe_getRPCMethods_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send GetRPCMethods response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_getRPCMethods_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__GetRPCMethodsResponse    *p_soap_cwmp1__GetRPCMethodsResponse;

    p_soap_cwmp1__GetRPCMethodsResponse = (struct _cwmp1__GetRPCMethodsResponse *)this->method_response_data;
    if (p_soap_cwmp1__GetRPCMethodsResponse!=NULL)
    {
        if (p_soap_cwmp1__GetRPCMethodsResponse->MethodList != NULL)
        {
            if(p_soap_cwmp1__GetRPCMethodsResponse->MethodList->__ptrstring != NULL)
            {
                free (p_soap_cwmp1__GetRPCMethodsResponse->MethodList->__ptrstring);
            }
            free (p_soap_cwmp1__GetRPCMethodsResponse->MethodList);
        }
    }

    return CWMP_OK;
}

