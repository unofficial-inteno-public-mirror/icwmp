/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	Powered by Inteno Broadband Technology AB
 *
 *	Copyright (C) 2013 Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	Copyright (C) 2013 Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *
 */

#include <pthread.h>
#include "cwmp.h"
#include "xml.h"
#include "backupSession.h"
#include "log.h"

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
    struct list_head                    *ilist;
    struct parameter_container          *parameter_container;
    char                                section[256];
    mxml_node_t							*b;

    if (EVENT_CONST[event_container->code].RETRY != 0)
    {
        b = bkp_session_insert_event(event_container->code, event_container->command_key, event_container->id, "queue");

        list_for_each(ilist,&(event_container->head_parameter_container))
        {
            parameter_container = list_entry(ilist, struct parameter_container, list);
            bkp_session_insert_parameter(b, parameter_container->name);
        }
    }
    bkp_session_save();
    return;
}

struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_code, char *command_key)
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
        if (event_container->code==event_code &&
            EVENT_CONST[event_code].TYPE==EVENT_TYPE_SINGLE)
        {
            return event_container;
        }
        if(event_container->code > event_code)
        {
            break;
        }
    }
    event_container = calloc (1,sizeof(struct event_container));
    if (event_container==NULL)
    {
        return NULL;
    }
    INIT_LIST_HEAD (&(event_container->head_parameter_container));
    list_add (&(event_container->list), ilist->prev);
    event_container->code = event_code;
    event_container->command_key = strdup(command_key);
    if((id<0) || (id>=MAX_INT_ID) )
    {
        id=0;
    }
    id++;
    event_container->id         = id;
    return event_container;
}

void parameter_container_add(struct list_head *list, char *param_name, char *param_data, char *param_type, char *fault_code)
{
	struct parameter_container *parameter_container;
	parameter_container = calloc(1, sizeof(struct parameter_container));
	list_add_tail(&parameter_container->list,list);
	if (param_name) parameter_container->name = strdup(param_name);
	if (param_data) parameter_container->data = strdup(param_data);
	if (param_type) parameter_container->type = strdup(param_type);
	if (fault_code) parameter_container->fault_code = strdup(fault_code);
}

void parameter_container_delete(struct parameter_container *parameter_container)
{
	list_del(&parameter_container->list);
	free(parameter_container->name);
	free(parameter_container->data);
	free(parameter_container->type);
	free(parameter_container->fault_code);
	free(parameter_container);
}

void parameter_container_delete_all(struct list_head *list)
{
	struct parameter_container *parameter_container;
	while (list->next!=list) {
		parameter_container = list_entry(list->next, struct parameter_container, list);
		parameter_container_delete(parameter_container);
	}
}

void cwmp_add_notification (char *name, char *value, char *type)
{
	char *notification = NULL;
	struct event_container   *event_container;
	struct cwmp   *cwmp = &cwmp_main;

	external_get_action_data("notification", name, &notification);
	if (!notification || notification[0]=='0')
	{
		free(notification);
		return;
	}
	external_add_list_value_change(name, value, type);
	pthread_mutex_lock (&(cwmp->mutex_session_queue));
	if (notification[0]=='2')
	{
		event_container = cwmp_add_event_container(cwmp, EVENT_IDX_4VALUE_CHANGE, "");
		if (event_container == NULL)
		{
			pthread_mutex_unlock (&(cwmp->mutex_session_queue));
			free(notification);
			return;
		}
		cwmp_save_event_container(cwmp,event_container);
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
    struct event_container              *event_container;
    struct parameter_container          *parameter_container;

    while (session->head_event_container.next!=&(session->head_event_container))
    {
        event_container = list_entry(session->head_event_container.next, struct event_container, list);
        bkp_session_delete_event(event_container->id, rem_from?"send":"queue");
        free (event_container->command_key);
        parameter_container_delete_all(&(event_container->head_parameter_container));
        list_del(&(event_container->list));
        free (event_container);
    }
    bkp_session_save();
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
        free(acsurl);
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
        pthread_mutex_lock (&(cwmp->mutex_session_queue));
        event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        parameter_container_add(&(event_container->head_parameter_container),
        		"InternetGatewayDevice.ManagementServer.URL", NULL, NULL, NULL);
        cwmp_save_event_container (cwmp,event_container);
        save_acs_bkp_config (cwmp);
        cwmp_scheduleInform_remove_all();
        cwmp_scheduledDownload_remove_all();
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    }

    return CWMP_OK;
}

int cwmp_root_cause_TransferComplete (struct cwmp *cwmp, struct transfer_complete *p)
{
    struct event_container                      *event_container;
    struct session                              *session;
    struct rpc									*rpc_acs;

    pthread_mutex_lock (&(cwmp->mutex_session_queue));
    event_container = cwmp_add_event_container (cwmp, EVENT_IDX_7TRANSFER_COMPLETE, "");
    if (event_container == NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MEM_ERR;
    }
    event_container = cwmp_add_event_container (cwmp, EVENT_IDX_M_Download, p->command_key?p->command_key:"");
    if (event_container == NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MEM_ERR;
    }
    session = list_entry (cwmp->head_event_container, struct session,head_event_container);
    if((rpc_acs = cwmp_add_session_rpc_acs(session, RPC_ACS_TRANSFER_COMPLETE)) == NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MEM_ERR;
    }
    rpc_acs->extra_data = (void *)p;
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
        if(cwmp_add_session_rpc_acs(session, RPC_ACS_GET_RPC_METHODS) == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
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
    static bool		periodic_enable = false;
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

