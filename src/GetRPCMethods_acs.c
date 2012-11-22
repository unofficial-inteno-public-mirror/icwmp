/*
    GetRPCMethods_acs.c

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

int cwmp_rpc_acs_getRPCMethods_data_init (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_getRPCMethods_remote_call (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_getRPCMethods_response (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_getRPCMethods_end (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_getRPCMethods_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);

void soap_serialize__cwmp1__GetRPCMethods(struct soap *soap, const struct _cwmp1__GetRPCMethods *a);
int soap_put__cwmp1__GetRPCMethods(struct soap *soap, const struct _cwmp1__GetRPCMethods *a, const char *tag, const char *type);
struct _cwmp1__GetRPCMethodsResponse * soap_get__cwmp1__GetRPCMethodsResponse(struct soap *soap, struct _cwmp1__GetRPCMethodsResponse *p, const char *tag, const char *type);

struct rpc_acs *cwmp_add_session_rpc_acs (struct session *session);

struct rpc_acs *cwmp_add_session_rpc_acs_getRPCMethods (struct session *session)
{
    struct rpc_acs                  *rpc_acs;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_acs = cwmp_add_session_rpc_acs (session);
    if (rpc_acs == NULL)
    {
        return NULL;
    }
    soap_methods                    = &(rpc_acs->soap_methods);
    rpc_acs->method_data            = (void *) calloc (1,sizeof(struct _cwmp1__GetRPCMethods));
    rpc_acs->method_data_init       = cwmp_rpc_acs_getRPCMethods_data_init;
    rpc_acs->method_remote_call     = cwmp_rpc_acs_getRPCMethods_remote_call;
    rpc_acs->method_response_data   = (void *) calloc (1,sizeof(struct _cwmp1__GetRPCMethodsResponse));
    rpc_acs->method_response        = cwmp_rpc_acs_getRPCMethods_response;
    rpc_acs->method_end             = cwmp_rpc_acs_getRPCMethods_end;
    rpc_acs->destructor             = cwmp_rpc_acs_getRPCMethods_destructor;
    rpc_acs->type                   = RPC_ACS_GETRPCMETHODS_IDX;
    soap_methods->envelope                          = "cwmp:GetRPCMethods";
    soap_methods->envelope_response                 = "cwmp:GetRPCMethodsResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__GetRPCMethods;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__GetRPCMethods;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__GetRPCMethodsResponse;

    return rpc_acs;
}

int cwmp_rpc_acs_getRPCMethods_data_init (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    return CWMP_OK;
}

int cwmp_rpc_acs_getRPCMethods_remote_call (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    CWMP_LOG (INFO,"Trying to call the GetRPCMethods remote method");
    cwmp_soap_call_rpc_acs (cwmp, session, this);
    return CWMP_OK;
}

int cwmp_rpc_acs_getRPCMethods_response (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    return CWMP_OK;
}

int cwmp_rpc_acs_getRPCMethods_end (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    return CWMP_OK;
}

int cwmp_rpc_acs_getRPCMethods_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    cwmp_session_rpc_acs_destructor (cwmp, session, this);
    return CWMP_OK;
}
