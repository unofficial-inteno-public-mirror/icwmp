/*
    Reboot.c

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
#include <sys/reboot.h>
#include <unistd.h>

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_reboot (struct session *session);
int cwmp_rpc_cpe_reboot (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_reboot_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_reboot_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_reboot_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);
void cwmp_save_event_container (struct cwmp *cwmp,struct event_container *event_container);

struct rpc_cpe *cwmp_add_session_rpc_cpe_reboot (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__Reboot));
    rpc_cpe->method                                 = cwmp_rpc_cpe_reboot;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__RebootResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_reboot_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_reboot_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_reboot_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:Reboot";
    soap_methods->envelope_response                 = "cwmp:RebootResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__RebootResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__RebootResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__Reboot;
    return rpc_cpe;
}

int cwmp_reboot(struct cwmp *cwmp,void *v)
{
	CWMP_LOG(INFO,"RUN reboot function");
	/** flush file system buffers **/
	sync();
    return reboot(RB_AUTOBOOT);
}

int cwmp_rpc_cpe_reboot (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct event_container  *event_container;
    struct _cwmp1__Reboot   *p;

    pthread_mutex_lock (&(cwmp->mutex_session_queue));
    p = (struct _cwmp1__Reboot *) this->method_data;
    event_container = cwmp_add_event_container (cwmp, EVENT_IDX_M_Reboot, p->CommandKey);
    if (event_container == NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MEM_ERR;
    }
    cwmp_save_event_container (cwmp,event_container);
    pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    add_session_end_func(session,cwmp_reboot,NULL,TRUE);
    return CWMP_OK;
}

int cwmp_rpc_cpe_reboot_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_reboot_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send Reboot response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_reboot_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

