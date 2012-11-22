/*
    event.c

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
#include <pthread.h>
#include "cwmp.h"
#include "backupSession.h"

struct rpc_acs *cwmp_add_session_rpc_acs_getRPCMethods (struct session *session);
struct rpc_acs *cwmp_add_session_rpc_acs_transferComplete (struct session *session);
struct session *cwmp_add_queue_session (struct cwmp *cwmp);
void backup_session_insert_rpc(char *name, char *commandKey, int status);
void backup_session_delete_rpc(char *name, char *commandKey, int status);
void backup_session_insert_event(char *name, char *commandKey, int id, int rpc_status);
void backup_session_insert_parameter(char *name, char *value, char *parent,char *parent_name, int id, char *commandKey);
int cwmp_scheduleInform_remove_all();
int cwmp_scheduledDownload_remove_all();

extern struct cwmp cwmp_main;


const struct EVENT_CONST_STRUCT EVENT_CONST [] = {
        [EVENT_IDX_0BOOTSTRAP]                      = {"0 BOOTSTRAP",                       EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_1BOOT]                           = {"1 BOOT",                            EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL},
        [EVENT_IDX_2PERIODIC]                       = {"2 PERIODIC",                        EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_3SCHEDULED]                      = {"3 SCHEDULED",                       EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_4VALUE_CHANGE]                   = {"4 VALUE CHANGE",                    EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL},
        [EVENT_IDX_5KICKED]                         = {"5 KICKED",                          EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_6CONNECTION_REQUEST]             = {"6 CONNECTION REQUEST",              EVENT_TYPE_SINGLE,  0},
        [EVENT_IDX_7TRANSFER_COMPLETE]              = {"7 TRANSFER COMPLETE",               EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_8DIAGNOSTICS_COMPLETE]           = {"8 DIAGNOSTICS COMPLETE",            EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL},
        [EVENT_IDX_9REQUEST_DOWNLOAD]               = {"9 REQUEST DOWNLOAD",                EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_10AUTONOMOUS_TRANSFER_COMPLETE]  = {"10 AUTONOMOUS TRANSFER COMPLETE",   EVENT_TYPE_SINGLE,  EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_M_Reboot]                        = {"M Reboot",                          EVENT_TYPE_MULTIPLE,EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_M_ScheduleInform]                = {"M ScheduleInform",                  EVENT_TYPE_MULTIPLE,EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_M_Download]                      = {"M Download",                        EVENT_TYPE_MULTIPLE,EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT},
        [EVENT_IDX_M_Upload]                        = {"M Upload",                          EVENT_TYPE_MULTIPLE,EVENT_RETRY_AFTER_TRANSMIT_FAIL|EVENT_RETRY_AFTER_REBOOT}
};


void cwmp_save_event_container (struct cwmp *cwmp,struct event_container *event_container)
{

    struct cwmp1__EventStruct           *pEventStruct;
    struct list_head                    *ilist;
    struct paramater_container          *paramater_container;
    struct cwmp1__ParameterValueStruct  *ptr_ParameterValueStruct;
    char                                section[256];

    pEventStruct    = &(event_container->event);
    if (EVENT_CONST[event_container->idx].RETRY != 0)
    {
        backup_session_insert_rpc("Inform",NULL,RPC_QUEUE);
        backup_session_insert_event(pEventStruct->EventCode, pEventStruct->CommandKey, event_container->id, RPC_QUEUE);

        list_for_each(ilist,&(event_container->head_paramater_container))
        {
            paramater_container         = list_entry(ilist, struct paramater_container, list);
            ptr_ParameterValueStruct    = &(paramater_container->paramater);
            backup_session_insert_parameter(ptr_ParameterValueStruct->Name,NULL,"event",pEventStruct->EventCode,event_container->id,pEventStruct->CommandKey);
        }
    }
    return;
}

struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key)
{
    static unsigned int      id;
    struct event_container   *event_container;
    struct session           *session;
    struct list_head         *ilist;

    if (cwmp->head_event_container == NULL)
    {
        session = cwmp_add_queue_session(cwmp);
        if (session == NULL)
        {
            return NULL;
        }
        cwmp->head_event_container = &(session->head_event_container);
    }
    session = list_entry (cwmp->head_event_container, struct session,head_event_container);
    list_for_each(ilist, cwmp->head_event_container)
    {
        event_container = list_entry (ilist, struct event_container, list);
        if (event_container->idx==event_idx &&
            EVENT_CONST[event_idx].TYPE==EVENT_TYPE_SINGLE)
        {
            return event_container;
        }
        if(event_container->idx > event_idx)
        {
            break;
        }
    }
    event_container = calloc (1,sizeof(struct event_container));
    if (event_container==NULL)
    {
        return NULL;
    }
    INIT_LIST_HEAD (&(event_container->head_paramater_container));
    list_add (&(event_container->list), ilist->prev);
    event_container->event.EventCode    = strdup(EVENT_CONST[event_idx].CODE);
    event_container->event.CommandKey   = strdup(command_key);
    if((id<0) || (id>=MAX_INT_ID) )
    {
        id=0;
    }
    id++;
    event_container->id         = id;
    event_container->idx        = event_idx;
    session->event_size++;
    return event_container;
}

struct paramater_container *cwmp_add_parameter_container
    (struct cwmp *cwmp,
     struct event_container *event_container,
     char *name,
     char *value,
     char *type)
{
    struct paramater_container          *paramater_container;
    struct cwmp1__ParameterValueStruct  *ptr_ParameterValueStruct;
    struct list_head                    *ilist;
    struct session                      *session;

    list_for_each(ilist,&(event_container->head_paramater_container))
    {
        paramater_container         = list_entry(ilist, struct paramater_container, list);
        ptr_ParameterValueStruct    = &(paramater_container->paramater);
        if(strcmp(ptr_ParameterValueStruct->Name,name)==0)
        {
        	if (value)
        	{
				free(ptr_ParameterValueStruct->Value);
				ptr_ParameterValueStruct->Value = strdup(value);
        	}
        	if (type)
			{
				free(ptr_ParameterValueStruct->Type);
				ptr_ParameterValueStruct->Type = strdup(type);
			}
        	return paramater_container;
        }
    }
    paramater_container = calloc (1, sizeof(struct paramater_container));
    if(paramater_container == NULL)
    {
        return NULL;
    }
    paramater_container->paramater.Name  = strdup(name);
    if (value) ptr_ParameterValueStruct->Value = strdup(value);
    if (type) ptr_ParameterValueStruct->Type = strdup(type);
    list_add_tail(&(paramater_container->list), &(event_container->head_paramater_container));
    session = list_entry (cwmp->head_event_container, struct session,head_event_container);
    session->parameter_size++;
    return paramater_container;
}

void cwmp_add_notification (char *name, char *value, char *type)
{
	struct handler_ParameterValueStruct *handler_ParameterValueStruct;
	char *notification = NULL;
	struct event_container   *event_container;
	struct cwmp   *cwmp = &cwmp_main;

	external_get_action_data("notification", name, &notification);
	if (!notification || notification[0]=='0')
	{
		free(notification);
		return;
	}

	pthread_mutex_lock (&(cwmp->mutex_session_queue));
	pthread_mutex_lock (&(cwmp->api_value_change.mutex));
	handler_ParameterValueStruct = calloc(1, sizeof(struct handler_ParameterValueStruct));
	handler_ParameterValueStruct->ParameterValueStruct = calloc (1, sizeof(struct cwmp1__ParameterValueStruct));
	handler_ParameterValueStruct->ParameterValueStruct->Name = strdup(name);
	if (value) handler_ParameterValueStruct->ParameterValueStruct->Value = strdup(value);
	if (type) handler_ParameterValueStruct->ParameterValueStruct->Type = strdup(type);
	else if (value)	handler_ParameterValueStruct->ParameterValueStruct->Type = strdup("xsd:string");
	list_add_tail(&(handler_ParameterValueStruct->list), &(cwmp->api_value_change.parameter_list));
	pthread_mutex_unlock (&(cwmp->api_value_change.mutex));
	if (notification[0]=='2')
	{
		event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
		if (event_container == NULL)
		{
			pthread_mutex_unlock (&(cwmp->mutex_session_queue));
			free(notification);
			return;
		}
		cwmp_save_event_container (cwmp,event_container);
		pthread_mutex_unlock (&(cwmp->mutex_session_queue));
		pthread_cond_signal(&(cwmp->threshold_session_send));
		free(notification);
		return;
	}
	free(notification);
	pthread_mutex_unlock (&(cwmp->mutex_session_queue));
}

int cwmp_root_cause_event_boot (struct cwmp *cwmp)
{
    struct event_container   *event_container;
    if (cwmp->env.boot == CWMP_START_BOOT)
    {
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        cwmp->env.boot = 0;
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_1BOOT, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        cwmp_save_event_container (cwmp,event_container);
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    }
    return CWMP_OK;
}
int event_remove_all_event_container(struct session *session, int rem_from)
{
    struct cwmp1__EventStruct           *eventStruct;
    struct event_container              *event_container;
    struct paramater_container          *paramater_container;
    struct cwmp1__ParameterValueStruct  *paramater;

    while (session->head_event_container.next!=&(session->head_event_container))
    {
        event_container = list_entry(session->head_event_container.next, struct event_container, list);
        eventStruct     = &(event_container->event);
        if (eventStruct->EventCode!=NULL)
        {
            free (eventStruct->EventCode);
        }
        if (eventStruct->CommandKey!=NULL)
        {
            free (eventStruct->CommandKey);
        }
        while (event_container->head_paramater_container.next!=&(event_container->head_paramater_container))
        {
            paramater_container = list_entry(event_container->head_paramater_container.next, struct paramater_container, list);
            paramater           = &(paramater_container->paramater);
            if (paramater->Name!=NULL)
            {
                free (paramater->Name);
            }
            if (paramater->Value!=NULL)
            {
                free(paramater->Value);
            }
            list_del(&(paramater_container->list));
            free(paramater_container);
        }
        list_del(&(event_container->list));
        free (event_container);
    }
    session->event_size     = 0;
    session->parameter_size = 0;
    backup_session_delete_rpc("Inform",NULL,rem_from);
    return CWMP_OK;
}

int cwmp_root_cause_event_bootstrap (struct cwmp *cwmp)
{
    char                    *acsurl = NULL;
    int                     error,cmp=0;
    struct event_container  *event_container;
    struct session          *session;

    error   = cwmp_load_saved_session(cwmp, &acsurl, ACS);

    if(acsurl == NULL)
    {
        save_acs_bkp_config (cwmp);
    }

    if (acsurl == NULL || ((acsurl != NULL)&&(cmp = strcmp(cwmp->conf.acsurl,acsurl))))
    {
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        if (cwmp->head_event_container!=NULL && cwmp->head_session_queue.next!=&(cwmp->head_session_queue))
        {
            session = list_entry(cwmp->head_event_container,struct session, head_event_container);
            event_remove_all_event_container (session,RPC_QUEUE);
        }
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_0BOOTSTRAP, "");
        if (acsurl != NULL)
        {
            free(acsurl);
        }
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        cwmp_save_event_container (cwmp,event_container);
        cwmp_scheduleInform_remove_all();
        cwmp_scheduledDownload_remove_all();
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    }

    if (cmp)
    {
        struct paramater_container *paramater_container;
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        paramater_container = cwmp_add_parameter_container (cwmp,event_container, "InternetGatewayDevice.ManagementServer.URL", NULL, NULL);
        cwmp_save_event_container (cwmp,event_container);
        save_acs_bkp_config (cwmp);
        cwmp_scheduleInform_remove_all();
        cwmp_scheduledDownload_remove_all();
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    }

    return CWMP_OK;
}

int cwmp_root_cause_TransferComplete (struct cwmp *cwmp)
{
    struct event_container                      *event_container;
    struct session                              *session;
    struct rpc_acs                              *rpc_acs;
    struct _cwmp1__TransferComplete             *p;

    pthread_mutex_lock (&(cwmp->mutex_session_queue));
    event_container = cwmp_add_event_container (cwmp, EVENT_IDX_7TRANSFER_COMPLETE, "");
    if (event_container == NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MEM_ERR;
    }
    session = list_entry (cwmp->head_event_container, struct session,head_event_container);
    if((rpc_acs = cwmp_add_session_rpc_acs_transferComplete (session)) == NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MEM_ERR;
    }
    p = (struct _cwmp1__TransferComplete  *) rpc_acs->method_data;
    event_container = cwmp_add_event_container (cwmp, EVENT_IDX_M_Download, p->CommandKey);
    if (event_container == NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MEM_ERR;
    }
    pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    return CWMP_OK;
}

int cwmp_root_cause_getRPCMethod (struct cwmp *cwmp)
{
    char                    acsurl[256];
    int                     error,cmp=0;
    struct event_container  *event_container;
    struct session          *session;

    if (cwmp->env.periodic == CWMP_START_PERIODIC)
    {
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        cwmp->env.periodic = 0;
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_2PERIODIC, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        cwmp_save_event_container (cwmp,event_container);
        session = list_entry (cwmp->head_event_container, struct session,head_event_container);
        if(cwmp_add_session_rpc_acs_getRPCMethods (session) == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    }

    return CWMP_OK;
}

int cwmp_root_cause_event_iccu_value_change (struct cwmp *cwmp)
{
    struct event_container   *event_container;
    if (cwmp->env.iccu == CWMP_START_ICCU)
    {
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        cwmp->env.iccu = 0;
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        cwmp_add_parameter_container (cwmp, event_container, "InternetGatewayDevice.ManagementServer.ConnectionRequestURL", NULL, NULL);
        cwmp_save_event_container (cwmp,event_container);
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    }
    return CWMP_OK;
}

int cwmp_root_cause_event_api_value_change(struct cwmp *cwmp, struct session *session)
{
    struct event_container  *event_container;
    struct list_head        *ilist;

    if (cwmp->api_value_change.parameter_list.next != &(cwmp->api_value_change.parameter_list))
    {
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        pthread_mutex_lock (&(cwmp->api_value_change.mutex));
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->api_value_change.mutex));
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        list_for_each(ilist,&(cwmp->api_value_change.parameter_list))
        {
            struct handler_ParameterValueStruct         *handler_ParameterValueStruct;
            handler_ParameterValueStruct    = list_entry(ilist,struct handler_ParameterValueStruct,list);
            cwmp_add_parameter_container (cwmp, event_container, handler_ParameterValueStruct->ParameterValueStruct->Name, handler_ParameterValueStruct->ParameterValueStruct->Value, handler_ParameterValueStruct->ParameterValueStruct->Type);
            ilist = ilist->prev;
            list_del(&(handler_ParameterValueStruct->list));
            free (handler_ParameterValueStruct->ParameterValueStruct->Name);
            free (handler_ParameterValueStruct->ParameterValueStruct->Type);
            free (handler_ParameterValueStruct->ParameterValueStruct->Value);
            free (handler_ParameterValueStruct->ParameterValueStruct);
            free (handler_ParameterValueStruct);
        }
        cwmp->api_value_change.parameter_size = 0;
        cwmp_save_event_container (cwmp,event_container);
        pthread_mutex_unlock (&(cwmp->api_value_change.mutex));
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    }
    return CWMP_OK;
}

void *thread_event_periodic (void *v)
{
    struct cwmp                 *cwmp = (struct cwmp *) v;
    struct event_container      *event_container;
    static int                  periodic_interval;
    static bool 				periodic_enable;
    static struct timespec      periodic_timeout = {0, 0};

    periodic_interval 	= cwmp->conf.period;
    periodic_enable		= cwmp->conf.periodic_enable;

    for(;;)
    {
        pthread_mutex_lock (&(cwmp->mutex_periodic));
        periodic_timeout.tv_sec = time(NULL) + periodic_interval;
        if (cwmp->conf.periodic_enable)
        {
        	pthread_cond_timedwait(&(cwmp->threshold_periodic), &(cwmp->mutex_periodic), &periodic_timeout);
        }
        else
        {
        	pthread_cond_wait(&(cwmp->threshold_periodic), &(cwmp->mutex_periodic));
        }
        pthread_mutex_unlock (&(cwmp->mutex_periodic));
        if (periodic_interval != cwmp->conf.period || periodic_enable != cwmp->conf.periodic_enable)
        {
        	periodic_enable		= cwmp->conf.periodic_enable;
        	periodic_interval	= cwmp->conf.period;
            continue;
        }
        CWMP_LOG(INFO,"Periodic thread: add periodic event in the queue");
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_2PERIODIC, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            continue;
        }
        cwmp_save_event_container (cwmp,event_container);
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        pthread_cond_signal(&(cwmp->threshold_session_send));
    }
    return CWMP_OK;
}

int cwmp_root_cause_event_periodic (struct cwmp *cwmp)
{
    static int      period = 0;
    static bool		periodic_enable = FALSE;
    if (period==cwmp->conf.period && periodic_enable==cwmp->conf.periodic_enable)
    {
        return CWMP_OK;
    }
    period  		= cwmp->conf.period;
    periodic_enable = cwmp->conf.periodic_enable;
    CWMP_LOG(INFO,periodic_enable?"Periodic event is enabled. Interval period = %ds":"Periodic event is disabled", period);
    pthread_cond_signal(&(cwmp->threshold_periodic));
    return CWMP_OK;
}

int cwmp_root_cause_events (struct cwmp *cwmp)
{
    int                     error;

    if (error = cwmp_root_cause_event_bootstrap(cwmp))
	{
		return error;
	}

    if (error = cwmp_root_cause_event_boot(cwmp))
    {
        return error;
    }

    if (error = cwmp_root_cause_event_iccu_value_change(cwmp))
	{
		return error;
	}

    if (error = cwmp_root_cause_getRPCMethod(cwmp))
    {
        return error;
    }

    if (error = cwmp_root_cause_event_periodic(cwmp))
    {
        return error;
    }
    return CWMP_OK;
}

