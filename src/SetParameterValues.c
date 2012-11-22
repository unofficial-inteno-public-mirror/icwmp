/*
    SetParameterValues.c

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

LIST_HEAD(list_dm_set_handler);

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterValues (struct session *session);
int cwmp_rpc_cpe_setParameterValues (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterValues_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterValues_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_setParameterValues_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_dm_setParameterValues(struct cwmp *cwmp, struct dm_set_handler *dm_set_handler, char *path, char *value);
int dm_run_queue_cmd_handler_at_end_session (struct cwmp *cwmp, struct dm_set_handler *dm_set_handler);
int cwmp_reboot(struct cwmp *cwmp,void *v);
int dm_cwmp_config_reload (struct cwmp *cwmp, void *v );
struct rpc_cpe *cwmp_add_session_rpc_cpe_Fault (struct session *session, int idx);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
static int check_duplicated_parameter_name (struct cwmp1__ParameterValueStruct **ptr_ParameterValueStruct, int size);

struct rpc_cpe *cwmp_add_session_rpc_cpe_setParameterValues (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterValues));
    rpc_cpe->method                                 = cwmp_rpc_cpe_setParameterValues;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__SetParameterValuesResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_setParameterValues_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_setParameterValues_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_setParameterValues_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:SetParameterValues";
    soap_methods->envelope_response                 = "cwmp:SetParameterValuesResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__SetParameterValuesResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__SetParameterValuesResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__SetParameterValues;
    return rpc_cpe;
}

int cwmp_rpc_cpe_setParameterValues (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct _cwmp1__SetParameterValues           *p_soap_cwmp1__SetParameterValues;
    struct _cwmp1__SetParameterValuesResponse   *p_soap_cwmp1__SetParameterValuesResponse;
    struct cwmp1ParameterValueList              *ParameterList;
    struct cwmp1__ParameterValueStruct          **ptr_ParameterValueStruct,*ParameterValueStruct;
    char                                        *parameter_key,buf[128];
    int                                         i,n,size,error;
    struct dm_set_handler                       *dm_set_handler;
    struct rpc_cpe                  			*rpc_cpe_fault;

    dm_set_handler = calloc(1,sizeof(struct dm_set_handler));

    if (dm_set_handler == NULL)
    {
        return FAULT_CPE_INTERNAL_ERROR_IDX;
    }

    list_add_tail(&(dm_set_handler->list),&(list_dm_set_handler));

    INIT_LIST_HEAD (&(dm_set_handler->cmd_list));
    INIT_LIST_HEAD (&(dm_set_handler->cancel_list));
    INIT_LIST_HEAD (&(dm_set_handler->service_list));

    p_soap_cwmp1__SetParameterValues            = (struct _cwmp1__SetParameterValues *)this->method_data;
    p_soap_cwmp1__SetParameterValuesResponse    = (struct _cwmp1__SetParameterValuesResponse *)this->method_response_data;
    size                                        = p_soap_cwmp1__SetParameterValues->ParameterList->__size;
    ptr_ParameterValueStruct                    = p_soap_cwmp1__SetParameterValues->ParameterList->__ptrParameterValueStruct;
    parameter_key                               = p_soap_cwmp1__SetParameterValues->ParameterKey;

    if (error = check_duplicated_parameter_name (ptr_ParameterValueStruct, size))
    {
        if (cwmp_add_session_rpc_cpe_Fault(session,FAULT_CPE_INVALID_ARGUMENTS_IDX)==NULL)
        {
            return CWMP_GEN_ERR;
        }
        return CWMP_FAULT_CPE;
    }

    for (i=0;i<size;i++,ptr_ParameterValueStruct++)
    {
        ParameterValueStruct = *ptr_ParameterValueStruct;
        CWMP_LOG(INFO,"param[%d] Name = %s, Value = %s",i,ParameterValueStruct->Name,(strlen(ParameterValueStruct->Value)<1000)?ParameterValueStruct->Value:"(Unable to display big stings)");

        error = cwmp_dm_setParameterValues(cwmp, dm_set_handler, ParameterValueStruct->Name, ParameterValueStruct->Value);

        if (error != FAULT_CPE_NO_FAULT_IDX)
        {
            if (dm_set_handler->uci)
            {
                CWMP_LOG(INFO,"RUN uci revert"); /* TODO to be removed*/
                uci_revert_value();
            }
            dm_run_queue_cancel_handler(dm_set_handler);
            dm_free_dm_set_handler_queues(dm_set_handler);
            if (dm_set_handler!=NULL)
            {
                list_del(&(dm_set_handler->list));
                free(dm_set_handler);
            }
            if ((rpc_cpe_fault = cwmp_add_session_rpc_cpe_Fault(session,error))==NULL)
            {
                return CWMP_GEN_ERR;
            }
            cwmp_rpc_cpe_Fault_parameter_cause(rpc_cpe_fault,ParameterValueStruct->Name);
            return CWMP_FAULT_CPE;
        }
    }

    dm_run_queue_cmd_handler(dm_set_handler,FALSE);
    p_soap_cwmp1__SetParameterValuesResponse->Status = _cwmp1__SetParameterValuesResponse_Status__0;

    CWMP_LOG(INFO,"RUN uci commit");
	sprintf(buf,"%s=%s",UCI_ACS_PARAMETERKEY_PATH,parameter_key);
	uci_set_value (buf);
	uci_commit_value();

    if (dm_set_handler->cwmp_reload==TRUE)
    {
        CWMP_LOG(INFO,"Add cwmp  config reload at the end of the session");
        add_session_end_func(session,dm_cwmp_config_reload,NULL,FALSE);
        p_soap_cwmp1__SetParameterValuesResponse->Status = _cwmp1__SetParameterValuesResponse_Status__1;
    }
    if (dm_set_handler->reboot_required==TRUE)
    {
        CWMP_LOG(INFO,"Add reboot at the end of the session");
        add_session_end_func(session,cwmp_reboot,NULL,TRUE);
        p_soap_cwmp1__SetParameterValuesResponse->Status = _cwmp1__SetParameterValuesResponse_Status__1;
    }
    if (dm_set_handler->cmd_list.next!=&(dm_set_handler->cmd_list) ||
        (dm_set_handler->service_list.next!=&(dm_set_handler->service_list) && dm_set_handler->reboot_required==FALSE))
    {
        add_session_end_func(session,dm_run_queue_cmd_handler_at_end_session,dm_set_handler,FALSE);
        p_soap_cwmp1__SetParameterValuesResponse->Status = _cwmp1__SetParameterValuesResponse_Status__1;
        return CWMP_OK;
    }

    dm_free_dm_set_handler_queues(dm_set_handler);

    if (dm_set_handler!=NULL)
    {
        list_del(&(dm_set_handler->list));
        free(dm_set_handler);
    }

    return CWMP_OK;
}

int cwmp_rpc_cpe_setParameterValues_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}
int cwmp_rpc_cpe_setParameterValues_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send SetParameterValues response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}
int cwmp_rpc_cpe_setParameterValues_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

static int check_duplicated_parameter_name (struct cwmp1__ParameterValueStruct **ptr_ParameterValueStruct, int size)
{

    struct cwmp1__ParameterValueStruct **p, **q, *pvs, *qvs;
    int                                 i,j;
    for (i=0,p=ptr_ParameterValueStruct;i<(size-1);i++,p++)
    {
        pvs = *p;
        for (j=i+1,q=p+1;j<size;j++,q++)
        {
            qvs = *q;
            if (strcmp(pvs->Name,qvs->Name)==0)
            {
                return CWMP_GEN_ERR;
            }
        }
    }
    return CWMP_OK;
}
