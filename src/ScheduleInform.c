/*
    ScheduleInform.c

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
#include <signal.h>
#include "soapH.h"
#include "cwmp.h"
#include "list.h"

LIST_HEAD(list_schedule_inform);
static pthread_mutex_t      mutex_schedule_inform;
static pthread_cond_t       threshold_schedule_inform;
static bool                 thread_is_working=FALSE;

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session);
int cwmp_rpc_cpe_scheduleInform (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_scheduleInform_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_scheduleInform_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_rpc_cpe_scheduleInform_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this);
int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe);
struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);
void cwmp_save_event_container (struct cwmp *cwmp,struct event_container *event_container);
void backup_session_insert_schedule_time(time_t schedule_time,char *commandKey);
void backup_session_delete_schedule_time(time_t schedule_time,char *commandKey);
void backup_session_delete_scheduled_inform();

struct rpc_cpe *cwmp_add_session_rpc_cpe_scheduleInform (struct session *session)
{
    struct rpc_cpe                  *rpc_cpe;
    struct soap_cwmp1_methods__rpc  *soap_methods;

    rpc_cpe = cwmp_add_session_rpc_cpe (session);
    if (rpc_cpe == NULL)
    {
        return NULL;
    }
    soap_methods                                    = &(rpc_cpe->soap_methods);
    rpc_cpe->method_data                            = (void *) calloc (1,sizeof(struct _cwmp1__ScheduleInform));
    rpc_cpe->method                                 = cwmp_rpc_cpe_scheduleInform;
    rpc_cpe->method_response_data                   = (void *) calloc (1,sizeof(struct _cwmp1__ScheduleInformResponse));
    rpc_cpe->method_response_data_init              = cwmp_rpc_cpe_scheduleInform_response_data_init;
    rpc_cpe->method_response                        = cwmp_rpc_cpe_scheduleInform_response;
    rpc_cpe->method_end                             = cwmp_rpc_cpe_scheduleInform_end;
    rpc_cpe->destructor                             = cwmp_session_rpc_cpe_destructor;
    soap_methods->envelope                          = "cwmp:ScheduleInform";
    soap_methods->envelope_response                 = "cwmp:ScheduleInformResponse";
    soap_methods->soap_serialize_cwmp1__send_data   = (void *) soap_serialize__cwmp1__ScheduleInformResponse;
    soap_methods->soap_put_cwmp1__send_data         = (void *) soap_put__cwmp1__ScheduleInformResponse;
    soap_methods->soap_get_cwmp1__rpc_received_data = (void *) soap_get__cwmp1__ScheduleInform;
    return rpc_cpe;
}

void *thread_cwmp_rpc_cpe_scheduleInform (void *v)
{
    struct cwmp                     *cwmp = (struct cwmp *)v;
    struct event_container          *event_container;
    struct schedule_inform          *schedule_inform;
    struct timespec                 si_timeout = {0, 0};
    time_t                          current_time;
    bool                            add_event_same_time = FALSE;

    thread_is_working = TRUE;
    while (list_schedule_inform.next!=&(list_schedule_inform))
    {
        schedule_inform = list_entry(list_schedule_inform.next,struct schedule_inform, list);
        current_time    = time(NULL);
        if (current_time >= schedule_inform->scheduled_time)
        {
            if (add_event_same_time)
            {
                pthread_mutex_lock (&mutex_schedule_inform);
                list_del (&(schedule_inform->list));
                if (schedule_inform->commandKey!=NULL)
                {
                    backup_session_delete_schedule_time(schedule_inform->scheduled_time,schedule_inform->commandKey);
                    free (schedule_inform->commandKey);
                }
                free(schedule_inform);
                pthread_mutex_unlock (&mutex_schedule_inform);
                continue;
            }
            pthread_mutex_lock (&(cwmp->mutex_session_queue));
            CWMP_LOG(INFO,"Schedule Inform thread: add ScheduleInform event in the queue");
            event_container = cwmp_add_event_container (cwmp, EVENT_IDX_3SCHEDULED, "");
            if (event_container != NULL)
            {
                cwmp_save_event_container (cwmp,event_container);
            }
            event_container = cwmp_add_event_container (cwmp, EVENT_IDX_M_ScheduleInform, schedule_inform->commandKey);
            if (event_container != NULL)
            {
                cwmp_save_event_container (cwmp,event_container);
            }
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            pthread_cond_signal(&(cwmp->threshold_session_send));
            pthread_mutex_lock (&mutex_schedule_inform);
            list_del (&(schedule_inform->list));
            if (schedule_inform->commandKey!=NULL)
            {
                backup_session_delete_schedule_time(schedule_inform->scheduled_time,schedule_inform->commandKey);
                free (schedule_inform->commandKey);
            }
            free(schedule_inform);
            pthread_mutex_unlock (&mutex_schedule_inform);
            add_event_same_time = TRUE;
            continue;
        }
        add_event_same_time = FALSE;
        pthread_mutex_lock (&mutex_schedule_inform);
        si_timeout.tv_sec = schedule_inform->scheduled_time;
        pthread_cond_timedwait(&threshold_schedule_inform, &mutex_schedule_inform, &si_timeout);
        pthread_mutex_unlock (&mutex_schedule_inform);
    }
    thread_is_working = FALSE;
    return CWMP_OK;
}

int cwmp_rpc_cpe_scheduleInform (struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    struct event_container          *event_container;
    struct _cwmp1__ScheduleInform   *p;
    struct schedule_inform          *schedule_inform;
    time_t                          scheduled_time;
    struct list_head                *ilist;
    bool                            cond_signal=FALSE;
    pthread_t                       scheduleInform_thread;
    int                             error;

    pthread_mutex_lock (&mutex_schedule_inform);
    p = (struct _cwmp1__ScheduleInform *) this->method_data;
    scheduled_time = time(NULL) + p->DelaySeconds;
    __list_for_each(ilist,&(list_schedule_inform))
    {
        schedule_inform = list_entry(ilist,struct schedule_inform, list);
        if (schedule_inform->scheduled_time == scheduled_time)
        {
            pthread_mutex_unlock (&mutex_schedule_inform);
            return CWMP_OK;
        }
        if (schedule_inform->scheduled_time > scheduled_time)
        {
            cond_signal = TRUE;
            break;
        }
    }
    CWMP_LOG(INFO,"Schedule inform event will start in %us",p->DelaySeconds);
    schedule_inform = calloc (1,sizeof(struct schedule_inform));
    if (schedule_inform==NULL)
    {
        pthread_mutex_unlock (&mutex_schedule_inform);
        return CWMP_OK;
    }
    schedule_inform->commandKey     = strdup(p->CommandKey);
    schedule_inform->scheduled_time = scheduled_time;
    list_add (&(schedule_inform->list), ilist->prev);
    backup_session_insert_schedule_time(schedule_inform->scheduled_time,schedule_inform->commandKey);
    if (cond_signal)
    {
        pthread_cond_signal(&threshold_schedule_inform);
    }
    pthread_mutex_unlock (&mutex_schedule_inform);

    if (!thread_is_working)
    {
        thread_is_working = TRUE;
        error = pthread_create(&scheduleInform_thread, NULL, &thread_cwmp_rpc_cpe_scheduleInform, (void *)cwmp);
        if (error<0)
        {
            CWMP_LOG(ERROR,"Error error when creating the schedule event thread!");
            thread_is_working = FALSE;
            return CWMP_OK;
        }

    }

    return CWMP_OK;
}

int cwmp_rpc_cpe_scheduleInform_response_data_init(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_rpc_cpe_scheduleInform_response(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    int error;
    CWMP_LOG (INFO,"Trying to send ScheduleInform response to the ACS");
    error = cwmp_soap_send_rpc_cpe_response (cwmp, session, this);
    return error;
}

int cwmp_rpc_cpe_scheduleInform_end(struct cwmp *cwmp, struct session *session, struct rpc_cpe *this)
{
    return CWMP_OK;
}

int cwmp_scheduleInform_remove_all()
{
	struct schedule_inform          *schedule_inform;

	pthread_mutex_lock (&mutex_schedule_inform);
	while (list_schedule_inform.next!=&(list_schedule_inform))
    {
        schedule_inform = list_entry(list_schedule_inform.next,struct schedule_inform, list);

		list_del (&(schedule_inform->list));
		if (schedule_inform->commandKey!=NULL)
		{
			backup_session_delete_schedule_time(schedule_inform->scheduled_time,schedule_inform->commandKey);
			free (schedule_inform->commandKey);
		}
		free(schedule_inform);
    }
	backup_session_delete_scheduled_inform();
	pthread_mutex_unlock (&mutex_schedule_inform);

	return CWMP_OK;
}
