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
#include "list.h"
#include "cwmp.h"
#include "dm.h"
#include "dm_rpc.h"
#include <uci.h>
#include "backupSession.h"

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
int free_list_getParameterValues(struct list_head *list);
struct cwmp1__InformResponse *soap_get_cwmp1__InformResponse (struct soap *soap, struct cwmp1__InformResponse *p, const char *tag, const char *type);

extern struct list_head     forced_inform_parameter_list;
static int                  forced_inform_parameter_size=0;
static LIST_HEAD(forced_inform_parameter_indice_list);
extern char *TYPE_VALUES_ARRAY [COUNT_TYPE_VALUES];

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
    struct forced_inform_parameter      *fip;
    int                                 i, j, size, force_size = 0 , error;
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
    dm_get_deviceId_manufacturer(cwmp,&(DeviceIdStruct->Manufacturer));
    dm_get_deviceId_oui(cwmp,&(DeviceIdStruct->OUI));
    dm_get_deviceId_productClass(cwmp,&(DeviceIdStruct->ProductClass));
    dm_get_deviceId_serialNumber(cwmp,&(DeviceIdStruct->SerialNumber));
    p_soap_cwmp1__Inform                = (struct cwmp1__Inform *) this->method_data;
    p_soap_cwmp1__Inform->DeviceId      = DeviceIdStruct;
    p_soap_cwmp1__Inform->CurrentTime   = time((time_t*)NULL);
    p_soap_cwmp1__Inform->RetryCount    = cwmp->retry_count_session;
    p_soap_cwmp1__Inform->MaxEnvelopesI = 1;
     __list_for_each(ilist, &(forced_inform_parameter_list))
    {
        fip = list_entry (ilist, struct forced_inform_parameter,list);
        if (strstr(fip->name,"{i}")==NULL)
        {
            force_size++;
        }
        else
        {
            int n = 0;
            inform_dm_getParameterPaths_by_correspondence(cwmp,fip->name,&forced_inform_parameter_indice_list,&n);
            force_size += n;
        }
    }
    forced_inform_parameter_size        = force_size;
    size                                = force_size + session->parameter_size;
    p_soap_cwmp1__Inform->ParameterList = calloc(1,sizeof(struct cwmp1ParameterValueList));
    ParameterValueStructForce           = calloc(force_size,sizeof(struct cwmp1__ParameterValueStruct));
    ptr_ParameterValueStruct            = calloc(size,sizeof(struct cwmp1__ParameterValueStruct *));
    p_soap_cwmp1__Inform->Event         = calloc(1,sizeof(struct cwmp1EventList));
    pEventStruct                        = calloc(session->event_size,sizeof(struct cwmp1__EventStruct *));
    if (p_soap_cwmp1__Inform->ParameterList == NULL ||
        ParameterValueStructForce == NULL           ||
        ptr_ParameterValueStruct == NULL            ||
        p_soap_cwmp1__Inform->Event == NULL         ||
        pEventStruct==NULL)
    {
        return CWMP_MEM_ERR;
    }
    p_soap_cwmp1__Inform->ParameterList->__ptrParameterValueStruct  = ptr_ParameterValueStruct;
    p_soap_cwmp1__Inform->Event->__size                             = session->event_size;
    p_soap_cwmp1__Inform->Event->__ptrEventStruct                   = pEventStruct;
    i = 0;
    __list_for_each(ilist, &(forced_inform_parameter_list))
    {
        fip = list_entry (ilist, struct forced_inform_parameter,list);
        if (strstr(fip->name,"{i}")==NULL)
        {
            if (i!=0)
            {
                ParameterValueStructForce++;
                ptr_ParameterValueStruct++;
            }
            i++;
            ParameterValueStructForce->Name = strdup (fip->name);
            ParameterValueStructForce->Type = TYPE_VALUES_ARRAY [fip->node->value_type];
            if (error = get_node_paramater_value(fip->node,NULL,0,&(ParameterValueStructForce->Value)))
            {
                char t[1];
                t[0] = 0;
                ParameterValueStructForce->Value = strdup(t);
            }
            *ptr_ParameterValueStruct = ParameterValueStructForce;
        }
    }
    __list_for_each(ilist, &(forced_inform_parameter_indice_list))
    {
        struct handler_ParameterValueStruct         *handler_ParameterValueStruct;
        if (i!=0)
        {
            ParameterValueStructForce++;
            ptr_ParameterValueStruct++;
        }
        i++;
        handler_ParameterValueStruct        = list_entry(ilist,struct handler_ParameterValueStruct,list);
        ParameterValueStructForce->Name     = handler_ParameterValueStruct->ParameterValueStruct->Name;
        ParameterValueStructForce->Value    = handler_ParameterValueStruct->ParameterValueStruct->Value;
        ParameterValueStructForce->Type     = handler_ParameterValueStruct->ParameterValueStruct->Type;
        *ptr_ParameterValueStruct = ParameterValueStructForce;
        ilist = ilist->prev;
        list_del(&(handler_ParameterValueStruct->list));
        free (handler_ParameterValueStruct->ParameterValueStruct);
        free (handler_ParameterValueStruct);
    }
    pp  = p_soap_cwmp1__Inform->ParameterList->__ptrParameterValueStruct;
    size    = 0;
    error   = 1;
    __list_for_each(ilist,&(session->head_event_container))
    {
        event_container = list_entry(ilist, struct event_container, list);
        *pEventStruct   = &(event_container->event);
        pEventStruct ++;

        __list_for_each(jlist,&(event_container->head_paramater_container))
        {
            paramater_container = list_entry(jlist, struct paramater_container, list);
            for (j=0;j<(size+force_size);j++)
            {
                p = *(pp+j);
                if ((error = strcmp(paramater_container->paramater.Name,p->Name))==0)
                {
                    break;
                }
            }
            if (error==0)
            {
                continue;
            }
            if (i!=0)
            {
                ptr_ParameterValueStruct++;
            }
            i++;
            inform_dm_get_value_by_path (cwmp, paramater_container->paramater.Name, &(paramater_container->paramater.Value), &(paramater_container->paramater.Type));
            *ptr_ParameterValueStruct   = &(paramater_container->paramater);
            size++;
        }
    }
    p_soap_cwmp1__Inform->ParameterList->__size = size + force_size;

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
    int                                 force_size;

    force_size              = forced_inform_parameter_size;
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
            struct cwmp1__ParameterValueStruct  *ParameterValueStructForce,*p;
            int                                 i = 0;
            if (p_soap_cwmp1__Inform->ParameterList->__ptrParameterValueStruct!= NULL)
            {
                if (force_size>0)
                {
                    ParameterValueStructForce   = *(p_soap_cwmp1__Inform->ParameterList->__ptrParameterValueStruct);
                    p                           = ParameterValueStructForce;
                    while (p!=NULL && i<force_size)
                    {
                        if (p->Name!=NULL)
                        {
                            free (p->Name);
                        }
                        if (p->Value!=NULL)
                        {
                            free (p->Value);
                        }
                        p++;
                        i++;
                    }
                    if (ParameterValueStructForce!=NULL)
                    {
                        free (ParameterValueStructForce);
                    }
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

int dm_get_deviceId_manufacturer(struct cwmp *cwmp, char **value)
{
    *value = strdup("Inteno");
    return CWMP_OK;
}

int inform_dm_get_value_by_path (struct cwmp *cwmp, char *path, char **value, char **type)
{
    int n,error;
    LIST_HEAD(list_ParameterValueStruct);

    *value  = NULL;
    *type  = NULL;
    error   = cwmp_dm_getParameterValues(cwmp, path, &list_ParameterValueStruct,&n);
    if (error == FAULT_CPE_NO_FAULT_IDX)
    {
        struct handler_ParameterValueStruct *handler_ParameterValueStruct;
        if (list_ParameterValueStruct.next!=&list_ParameterValueStruct)
        {
            handler_ParameterValueStruct    = list_entry(list_ParameterValueStruct.next,struct handler_ParameterValueStruct,list);
            *value = strdup(handler_ParameterValueStruct->ParameterValueStruct->Value);
            *type  = handler_ParameterValueStruct->ParameterValueStruct->Type;
        }
    }
    if (*value==NULL)
    {
        char t[1];
        t[0]    = 0;
        *value  = strdup(t);
    }
    if (*type==NULL)
    {
        *type  = TYPE_VALUES_ARRAY[TYPE_VALUE_string_IDX];
    }
    free_list_getParameterValues(&list_ParameterValueStruct);
    return CWMP_OK;
}

int dm_get_deviceId_oui(struct cwmp *cwmp, char **value)
{
    int  error;
    char *type;
    error = inform_dm_get_value_by_path (cwmp, "InternetGatewayDevice.DeviceInfo.ManufacturerOUI", value, &type);
    return error;
}

int dm_get_deviceId_productClass(struct cwmp *cwmp, char **value)
{
    int error;
    char *type;
    error = inform_dm_get_value_by_path (cwmp, "InternetGatewayDevice.DeviceInfo.ProductClass", value, &type);
    return error;
}

int dm_get_deviceId_serialNumber(struct cwmp *cwmp, char **value)
{
    int error;
    char *type;
    error = inform_dm_get_value_by_path (cwmp, "InternetGatewayDevice.DeviceInfo.SerialNumber", value, &type);
    return error;
}

/*
 * End Get Device ID values
 */

int inform_dm_getParameterPaths_by_correspondence(struct cwmp *cwmp, char *path, struct list_head *list, int *n)
{

    char                *prefix_path=NULL;
    char                **argv;
    struct sub_path     *sub_path;
    int                 i,sub_path_size;

    argv        = calloc (1,sizeof(char *));
    sub_path    = calloc (DM_MAX_INDICE,sizeof(struct sub_path));
    argv[0]     = path;
    cwmp_dm_get_sub_indice_path(1,argv,&prefix_path,sub_path,&sub_path_size);
    cwmp_dm_getParameterPaths_by_correspondence(cwmp,prefix_path,sub_path,sub_path_size,list,n,TRUE,FALSE,&i);
    for (i=0;i<sub_path_size;i++)
    {
        if (sub_path[i].dm_indice.indice!=NULL)
        {
            free(sub_path[i].dm_indice.indice);
        }
    }
    free(sub_path);
    if (prefix_path!=NULL)
    {
        free(prefix_path);
    }
    free(argv);
}
