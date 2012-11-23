/*
    inform.c

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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "soapH.h"
#include "cwmp.h"
#include <uci.h>
#include "backupSession.h"
#include "external.h"

struct rpc_acs *cwmp_add_session_rpc_acs (struct session *session);
struct rpc_acs *cwmp_add_session_rpc_acs_head (struct session *session);

int cwmp_rpc_acs_inform_data_init (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_inform_remote_call (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_inform_response (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_rpc_acs_inform_end (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int cwmp_session_rpc_acs_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *rpc_acs);
int cwmp_rpc_acs_inform_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *this);
int event_remove_all_event_container(struct session *session, int rem_from);

void soap_serialize_cwmp1__Inform (struct soap *soap, const struct cwmp1__Inform *a);
int soap_put_cwmp1__Inform (struct soap *soap, const struct cwmp1__Inform *a, const char *tag, const char *type);
struct cwmp1__InformResponse *soap_get_cwmp1__InformResponse (struct soap *soap, struct cwmp1__InformResponse *p, const char *tag, const char *type);

extern struct list_head external_list_parameter;

struct rpc_acs *cwmp_add_session_rpc_acs_inform (struct session *session)
{
    struct rpc_acs                  *rpc_acs;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_acs = cwmp_add_session_rpc_acs_head (session); /* Only the inform rpc should be added in the head of the rpc acs list*/
    if (rpc_acs == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_acs->soap_methods);
    rpc_acs->method_data                            = (void *) calloc (1,sizeof(struct cwmp1__Inform));
    rpc_acs->method_data_init                       = cwmp_rpc_acs_inform_data_init;
    rpc_acs->method_remote_call                     = cwmp_rpc_acs_inform_remote_call;
    rpc_acs->method_response_data                   = (void *) calloc (1,sizeof(struct cwmp1__InformResponse));
    rpc_acs->method_response                        = cwmp_rpc_acs_inform_response;
    rpc_acs->method_end                             = cwmp_rpc_acs_inform_end;
    rpc_acs->destructor                             = cwmp_rpc_acs_inform_destructor;
    rpc_acs->type                                   = RPC_ACS_INFORM_IDX;
    soap_methods->envelope                          = "cwmp:Inform";
    soap_methods->envelope_response                 = "cwmp:InformResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize_cwmp1__Inform;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put_cwmp1__Inform;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get_cwmp1__InformResponse;
    return rpc_acs;
}

int cwmp_rpc_acs_inform_data_init (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    struct cwmp1__Inform                *p_soap_cwmp1__Inform;
    struct cwmp1__DeviceIdStruct        *DeviceIdStruct;
    struct cwmp1__EventStruct           **pEventStruct;
    struct cwmp1__ParameterValueStruct  *ParameterValueStructForce, **ptr_ParameterValueStruct, *p,**pp;
    struct event_container              *event_container;
    struct paramater_container          *paramater_container;
    struct external_parameter			*external_parameter;
    int                                 i, j, size = 0, force_size = 0 , error;
    struct list_head                    *ilist,*jlist;


    if (session==NULL || this==NULL)
    {
        return CWMP_GEN_ERR;
    }

    DeviceIdStruct = calloc(1,sizeof(struct cwmp1__DeviceIdStruct));
    if (DeviceIdStruct == NULL)
    {
        return CWMP_MEM_ERR;
    }
    if (external_get_action_write("value","InternetGatewayDevice.DeviceInfo.ManufacturerOUI", NULL)) return CWMP_GEN_ERR;
    if (external_get_action_write("value","InternetGatewayDevice.DeviceInfo.ProductClass", NULL)) return CWMP_GEN_ERR;
    if (external_get_action_write("value","InternetGatewayDevice.DeviceInfo.SerialNumber", NULL)) return CWMP_GEN_ERR;
    DeviceIdStruct->Manufacturer = strdup("Inteno");
    if (external_get_action_execute()) return CWMP_GEN_ERR;
    while (external_list_parameter.next!=&external_list_parameter) {
        external_parameter = list_entry(external_list_parameter.next, struct external_parameter, list);
        if(strcmp("InternetGatewayDevice.DeviceInfo.ManufacturerOUI", external_parameter->name)==0)
        	DeviceIdStruct->OUI = external_parameter->data;
        if(strcmp("InternetGatewayDevice.DeviceInfo.ProductClass", external_parameter->name)==0)
        	DeviceIdStruct->ProductClass = external_parameter->data;
        if(strcmp("InternetGatewayDevice.DeviceInfo.SerialNumber", external_parameter->name)==0)
        	DeviceIdStruct->SerialNumber = external_parameter->data;
        list_del(&external_parameter->list);
        FREE(external_parameter->name);
		FREE(external_parameter->type);
        FREE(external_parameter->fault_code);
        FREE(external_parameter);
    }

    p_soap_cwmp1__Inform                = (struct cwmp1__Inform *) this->method_data;
    p_soap_cwmp1__Inform->DeviceId      = DeviceIdStruct;
    p_soap_cwmp1__Inform->CurrentTime   = time((time_t*)NULL);
    p_soap_cwmp1__Inform->RetryCount    = cwmp->retry_count_session;
    p_soap_cwmp1__Inform->MaxEnvelopesI = 1;


    p_soap_cwmp1__Inform->Event         = calloc(1,sizeof(struct cwmp1EventList));
    pEventStruct                        = calloc(session->event_size,sizeof(struct cwmp1__EventStruct *));
    if (p_soap_cwmp1__Inform->Event == NULL ||
        pEventStruct==NULL)
    {
        return CWMP_MEM_ERR;
    }

    p_soap_cwmp1__Inform->Event->__size           = session->event_size;
    p_soap_cwmp1__Inform->Event->__ptrEventStruct = pEventStruct;

    if (external_simple("inform")) return CWMP_GEN_ERR;

    list_for_each(ilist,&(session->head_event_container))
    {
        event_container = list_entry(ilist, struct event_container, list);
        *pEventStruct   = &(event_container->event);
        if (ilist->next!=&(session->head_event_container))
        	pEventStruct ++;

        list_for_each(jlist,&(event_container->head_paramater_container))
        {
            paramater_container = list_entry(jlist, struct paramater_container, list);
            if (paramater_container->paramater.Value)
            {
            	external_add_list_paramameter(paramater_container->paramater.Name, paramater_container->paramater.Value, paramater_container->paramater.Type, NULL);
            }
            else if (external_get_action_write("value",paramater_container->paramater.Name, NULL)) return CWMP_GEN_ERR;
        }
    }

    if (external_get_action_execute()) return CWMP_GEN_ERR;

    list_for_each(ilist,&external_list_parameter) {
    	size++;
    }

    p_soap_cwmp1__Inform->ParameterList = calloc(1,sizeof(struct cwmp1ParameterValueList));
    ptr_ParameterValueStruct            = calloc(size,sizeof(struct cwmp1__ParameterValueStruct *));

    if (p_soap_cwmp1__Inform->ParameterList == NULL ||
		ptr_ParameterValueStruct == NULL)
	{
		return CWMP_MEM_ERR;
	}

    pp  = ptr_ParameterValueStruct;
    size = 0;
    while (external_list_parameter.next!=&external_list_parameter) {
    	external_parameter = list_entry(external_list_parameter.next, struct external_parameter, list);
    	error=-1;
    	for (j=0;j<size;j++)
		{
			p = *(pp+j);
			if ((error = strcmp(external_parameter->name,p->Name))==0)
			{
				break;
			}
		}
		if (error!=0)
		{
			*ptr_ParameterValueStruct = calloc(1,sizeof(struct cwmp1__ParameterValueStruct));
			(*ptr_ParameterValueStruct)->Name = external_parameter->name;
			(*ptr_ParameterValueStruct)->Value = external_parameter->data;
			(*ptr_ParameterValueStruct)->Type = external_parameter->type;
			size++;
		}
		else
		{
			FREE(external_parameter->name);
			FREE(external_parameter->data);
			FREE(external_parameter->type);
		}

		list_del(&external_parameter->list);
		FREE(external_parameter->fault_code);
		FREE(external_parameter);
		if (external_list_parameter.next!=&external_list_parameter)
		{
			ptr_ParameterValueStruct++;
		}
    }


    p_soap_cwmp1__Inform->ParameterList->__size = size;

    return CWMP_OK;
}

int cwmp_rpc_acs_inform_remote_call (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    CWMP_LOG(INFO, "Trying to call the Inform remote method");
    cwmp_soap_call_rpc_acs (cwmp, session, this);
    return CWMP_OK;
}

int cwmp_rpc_acs_inform_response (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    return CWMP_OK;
}

int cwmp_rpc_acs_inform_end (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{
    struct cwmp1__Inform                *p_soap_cwmp1__Inform;
    struct cwmp1__InformResponse        *p_soap_cwmp1__InformResponse;

    p_soap_cwmp1__Inform    = (struct cwmp1__Inform *)this->method_data;

    if (p_soap_cwmp1__Inform!=NULL)
    {
        if (p_soap_cwmp1__Inform->DeviceId!=NULL)
        {
            if (p_soap_cwmp1__Inform->DeviceId->Manufacturer!=NULL)
            {
                free (p_soap_cwmp1__Inform->DeviceId->Manufacturer);
            }
            if (p_soap_cwmp1__Inform->DeviceId->OUI!=NULL)
            {
                free (p_soap_cwmp1__Inform->DeviceId->OUI);
            }
            if (p_soap_cwmp1__Inform->DeviceId->ProductClass!=NULL)
            {
                free (p_soap_cwmp1__Inform->DeviceId->ProductClass);
            }
            if (p_soap_cwmp1__Inform->DeviceId->SerialNumber!=NULL)
            {
                free (p_soap_cwmp1__Inform->DeviceId->SerialNumber);
            }
            free (p_soap_cwmp1__Inform->DeviceId);
        }
        if (p_soap_cwmp1__Inform->Event!=NULL)
        {
            if (p_soap_cwmp1__Inform->Event->__ptrEventStruct!=NULL)
            {
                free (p_soap_cwmp1__Inform->Event->__ptrEventStruct);
            }
            free (p_soap_cwmp1__Inform->Event);
        }
        if (p_soap_cwmp1__Inform->ParameterList!=NULL)
        {
            struct cwmp1__ParameterValueStruct  *p;
            int                                 i = 0;
            if (p_soap_cwmp1__Inform->ParameterList->__ptrParameterValueStruct!= NULL)
            {
				p   = *(p_soap_cwmp1__Inform->ParameterList->__ptrParameterValueStruct);
				while (p!=NULL && i<p_soap_cwmp1__Inform->ParameterList->__size)
				{
					FREE(p->Name);
					FREE(p->Value);
					FREE(p->Type);
					p++;
					i++;
				}
                free(p_soap_cwmp1__Inform->ParameterList->__ptrParameterValueStruct);
            }
            free (p_soap_cwmp1__Inform->ParameterList);
        }
    }
    return CWMP_OK;
}

int cwmp_rpc_acs_inform_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *this)
{

    cwmp_session_rpc_acs_destructor (cwmp, session, this);
    event_remove_all_event_container (session,RPC_SEND);
    return CWMP_OK;
}

/*
 * Get Device ID values
 */
