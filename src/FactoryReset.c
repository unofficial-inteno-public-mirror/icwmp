/*
    FactoryReset.c

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

#include <stdio.h>
#include <string.h>
#include "soapH.h"
#include "cwmp.h"

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_factoryReset (struct session *session);
int cwmp_rpc_cpe_factoryReset (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_factoryReset_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_factoryReset_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_factoryReset_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
int cwmp_reset_factory(struct cwmp *cwmp,void *v);

struct rpc_cpe *cwmp_add_session_rpc_cpe_factoryReset (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__FactoryReset));
    rpc_cpe->method                                 = cwmp_rpc_cpe_factoryReset;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__FactoryResetResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_factoryReset_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_factoryReset_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_factoryReset_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:FactoryReset";
    soap_methods->envelope_response                 = "cwmp:FactoryResetResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__FactoryReset;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__FactoryReset;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__FactoryReset;
    return rpc_cpe;
}

int cwmp_rpc_cpe_factoryReset (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    add_session_end_func(session,cwmp_reset_factory,NULL,TRUE);
    return CWMP_OK;
}

int cwmp_rpc_cpe_factoryReset_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_factoryReset_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_factoryReset_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}
