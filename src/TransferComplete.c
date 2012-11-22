/*
    TransferComplete.c

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
#include "backupSession.h"

int cwmp_rpc_acs_transferComplete_data_init (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_transferComplete_remote_call (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_transferComplete_response (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_transferComplete_end (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_transferComplete_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_session_rpc_acs_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *rpc_acs);

void soap_serialize__cwmp1__TransferComplete(struct soap *soap, const struct _cwmp1__TransferComplete *a);
int soap_put__cwmp1__TransferComplete(struct soap *soap, const struct _cwmp1__TransferComplete *a, const char *tag, const char *type);
struct _cwmp1__TransferCompleteResponse * soap_get__cwmp1__TransferCompleteResponse(struct soap *soap, struct _cwmp1__TransferCompleteResponse *p, const char *tag, const char *type);
struct rpc_acs *cwmp_add_session_rpc_acs (struct session *session);
void backup_session_delete_rpc(char *name, char *commandKey, int status);

static struct _cwmp1__TransferComplete          *pcwmp1__TransferComplete_data;

struct _cwmp1__TransferComplete *cwmp_set_data_rpc_acs_transferComplete ()
{
    pcwmp1__TransferComplete_data = calloc (1,sizeof(struct _cwmp1__TransferComplete));
    return pcwmp1__TransferComplete_data;
}

struct rpc_acs *cwmp_add_session_rpc_acs_transferComplete (struct session *session)
{
    struct rpc_acs                                  *rpc_acs;
    struct soap_cwmp1_methods__rpc                  *soap_methods;

    rpc_acs = cwmp_add_session_rpc_acs (session);
    if (rpc_acs == NULL)
    {
        return NULL;
    }
    soap_methods                    = &(rpc_acs->soap_methods);
    rpc_acs->method_data            = (void *) pcwmp1__TransferComplete_data;
    rpc_acs->method_data_init       = cwmp_rpc_acs_transferComplete_data_init;
    rpc_acs->method_remote_call     = cwmp_rpc_acs_transferComplete_remote_call;
    rpc_acs->method_response_data   = (void *) calloc (1,sizeof(struct _cwmp1__TransferCompleteResponse));
    rpc_acs->method_response        = cwmp_rpc_acs_transferComplete_response;
    rpc_acs->method_end             = cwmp_rpc_acs_transferComplete_end;
    rpc_acs->destructor             = cwmp_rpc_acs_transferComplete_destructor;
    rpc_acs->type                   = RPC_ACS_TRANSFERCOMPLETE_IDX;
    soap_methods->envelope                          = "cwmp:TransferComplete";
    soap_methods->envelope_response                 = "cwmp:TransferCompleteResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__TransferComplete;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__TransferComplete;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__TransferCompleteResponse;
    return rpc_acs;
}

int cwmp_rpc_acs_transferComplete_data_init (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    return CWMP_OK;
}

int cwmp_rpc_acs_transferComplete_remote_call (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    CWMP_LOG (INFO,"Trying to call the TransferComplete remote method");
    cwmp_soap_call_rpc_acs (cwmp, session, this);
    return CWMP_OK;
}

int cwmp_rpc_acs_transferComplete_response (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    return CWMP_OK;
}

int cwmp_rpc_acs_transferComplete_end (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    return CWMP_OK;
}

int cwmp_rpc_acs_transferComplete_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    struct _cwmp1__TransferComplete             *p;

    p = (struct _cwmp1__TransferComplete  *) this->method_data;
    backup_session_delete_rpc("TransferComplete", p->CommandKey, RPC_NO_STATUS);
    if (p!=NULL)
    {
        if (p->CommandKey!=NULL)
        {
            free(p->CommandKey);
        }
        if (p->FaultStruct!=NULL)
        {
            if (p->FaultStruct->FaultCode!=NULL)
            {
                free(p->FaultStruct->FaultCode);
            }
            if (p->FaultStruct->FaultString!=NULL)
            {
                free(p->FaultStruct->FaultString);
            }
            free(p->FaultStruct);
        }
    }
    cwmp_session_rpc_acs_destructor (cwmp, session, this);
    return CWMP_OK;
}
