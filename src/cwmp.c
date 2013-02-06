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
#include <signal.h>
#include <sys/types.h>
#include "cwmp.h"
#include "backupSession.h"
#include "xml.h"
#include "log.h"
#include "external.h"

struct cwmp         	cwmp_main = {0};
static pthread_mutex_t	thread_sync_mutex		= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	thread_sync_cond		= PTHREAD_COND_INITIALIZER;
static bool				thread_sync_wait		= true;

struct rpc *cwmp_add_session_rpc_acs (struct session *session, int type)
{
    struct rpc     *rpc_acs;

    rpc_acs = calloc (1,sizeof(struct rpc));
    if (rpc_acs==NULL)
    {
        return NULL;
    }
    rpc_acs->type = type;
    list_add_tail (&(rpc_acs->list), &(session->head_rpc_acs));
    return rpc_acs;
}

struct rpc *cwmp_add_session_rpc_cpe (struct session *session, int type)
{
    struct rpc     *rpc_cpe;

    rpc_cpe = calloc (1,sizeof(struct rpc));
    if (rpc_cpe==NULL)
    {
        return NULL;
    }
    rpc_cpe->type = type;
    list_add_tail (&(rpc_cpe->list), &(session->head_rpc_cpe));
    return rpc_cpe;
}

struct rpc *cwmp_add_session_rpc_acs_head (struct session *session, int type)
{
    struct rpc     *rpc_acs;

    rpc_acs = calloc (1,sizeof(struct rpc));
    if (rpc_acs==NULL)
    {
        return NULL;
    }
    rpc_acs->type = type;
    list_add (&(rpc_acs->list), &(session->head_rpc_acs));
    return rpc_acs;
}

int cwmp_session_rpc_destructor (struct rpc *rpc)
{
    list_del(&(rpc->list));
    free (rpc);
    return CWMP_OK;
}

int cwmp_get_retry_interval (struct cwmp *cwmp)
{
    switch (cwmp->retry_count_session)
    {
        case 0: return MAX_INT32;
        case 1: return 6;
        case 2: return 11;
        case 3: return 21;
        case 4: return 41;
        case 5: return 81;
        case 6: return 161;
        case 7: return 321;
        case 8: return 641;
        case 9: return 1281;
        default: return 2561;
    }
}

void thread_sync()
{
	pthread_mutex_lock(&thread_sync_mutex);
	thread_sync_wait = false;
	pthread_cond_signal(&thread_sync_cond);
	pthread_mutex_unlock(&thread_sync_mutex);
}

void thread_wait_sync()
{
	pthread_mutex_lock(&thread_sync_mutex);
	if(thread_sync_wait)
	{
		pthread_cond_wait(&thread_sync_cond,&thread_sync_mutex);
	}
	thread_sync_wait = true;
	pthread_mutex_unlock(&thread_sync_mutex);
}

static void cwmp_prepare_value_change (struct cwmp *cwmp, struct session *session)
{
	struct event_container *event_container;
	if (external_list_value_change.next == &(external_list_value_change))
		return;
	pthread_mutex_lock(&(cwmp->mutex_session_queue));
	event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
	if (!event_container) goto end;
	pthread_mutex_lock(&(external_mutex_value_change));
	list_splice_init(&(external_list_value_change), &(event_container->head_parameter_container));
	pthread_mutex_unlock(&(external_mutex_value_change));
	cwmp_save_event_container (cwmp,event_container);

end:
	pthread_mutex_unlock(&(cwmp->mutex_session_queue));
}

void *cwmp_schedule_session (void *v)
{
    struct list_head                    *ilist;
    struct session                      *session;
    int                                 t,error = CWMP_OK;
    static struct timespec              time_to_wait = {0, 0};
    bool                                retry = false;
    struct cwmp                         *cwmp = (struct cwmp *) v;

    thread_sync();
    while (1)
    {
    	pthread_mutex_lock (&(cwmp->mutex_session_send));
    	ilist = (&(cwmp->head_session_queue))->next;
        while ((ilist == &(cwmp->head_session_queue)) || retry)
        {
            t = cwmp_get_retry_interval(cwmp);
            time_to_wait.tv_sec = time(NULL) + t;
            CWMP_LOG(INFO,"Waiting the next session");
            pthread_cond_timedwait(&(cwmp->threshold_session_send), &(cwmp->mutex_session_send), &time_to_wait);
            ilist = (&(cwmp->head_session_queue))->next;
            retry = false;
        }

        session = list_entry(ilist, struct session, list);

        cwmp_prepare_value_change(cwmp, session);

        if (error = cwmp_move_session_to_session_send (cwmp, session))
        {
            CWMP_LOG(EMERG,"FATAL error in the mutex process in the session scheduler!");
            exit(EXIT_FAILURE);
        }
        error = cwmp_schedule_rpc (cwmp,session);
        run_session_end_func(session);
        if (session->error == CWMP_RETRY_SESSION)
        {
            error = cwmp_move_session_to_session_queue (cwmp, session);
            CWMP_LOG(INFO,"Retry session, retry count = %d, retry in %ds",cwmp->retry_count_session,cwmp_get_retry_interval(cwmp));
            retry = true;
            pthread_mutex_unlock (&(cwmp->mutex_session_send));
            continue;
        }
        cwmp_session_destructor (cwmp, session);
        cwmp->session_send          = NULL;
        cwmp->retry_count_session   = 0;
        pthread_mutex_unlock (&(cwmp->mutex_session_send));
    }
    return CWMP_OK;
}

int cwmp_rpc_cpe_handle_message (struct session *session, struct rpc *rpc_cpe)
{
	if (xml_prepare_msg_out(session))
		return -1;
	if (rpc_cpe_methods[rpc_cpe->type].handler(session, rpc_cpe))
		return -1;
	if (xml_set_cwmp_id(session))
		return -1;
	return 0;
}

int cwmp_schedule_rpc (struct cwmp *cwmp, struct session *session)
{
    struct list_head	*ilist;
    struct rpc			*rpc_acs, *rpc_cpe;
    int					error=0,e;

    if (http_client_init(cwmp)) {
		CWMP_LOG(INFO, "initializing http client failed\n");
		goto retry;
	}

    while (1)
    {
        list_for_each(ilist, &(session->head_rpc_acs))
        {
            rpc_acs = list_entry (ilist, struct rpc, list);
            if (!rpc_acs->type)
            	goto retry;
            if (rpc_acs_methods[rpc_acs->type].prepare_message(cwmp, session, rpc_acs))
            	goto retry;
            CWMP_LOG (INFO,"Send the RPC message to the ACS");
            if (xml_send_message(cwmp, session, rpc_acs))
            	goto retry;
            if (rpc_acs_methods[rpc_acs->type].parse_response)
            	if (rpc_acs_methods[rpc_acs->type].parse_response(cwmp, session, rpc_acs))
            		goto retry;

            ilist = ilist->prev;
            if (rpc_acs_methods[rpc_acs->type].extra_clean != NULL)
            	rpc_acs_methods[rpc_acs->type].extra_clean(session,rpc_acs);
            cwmp_session_rpc_destructor(rpc_acs);
            MXML_DELETE(session->tree_in);
            MXML_DELETE(session->tree_out);
            if (session->hold_request)
                 break;
        }
		if (xml_send_message(cwmp, session, NULL))
			goto retry;
		if (!session->tree_in)
			goto success;

		if (xml_handle_message(session))
			goto retry;

		while (session->head_rpc_cpe.next != &(session->head_rpc_cpe))
		{
			rpc_cpe = list_entry (session->head_rpc_cpe.next, struct rpc, list);
			if (!rpc_cpe->type)
				goto retry;
			if (cwmp_rpc_cpe_handle_message(session, rpc_cpe))
				goto retry;
			MXML_DELETE(session->tree_in);
			CWMP_LOG (INFO,"Send the RPC response message to the ACS");
			if (xml_send_message(cwmp, session, rpc_cpe))
				goto retry;
			MXML_DELETE(session->tree_out);
			if (!session->tree_in)
				break;
			cwmp_session_rpc_destructor(rpc_cpe);
			if (xml_handle_message(session))
				goto retry;
		}
        if (session->head_rpc_acs.next==&(session->head_rpc_acs))
            break;
        MXML_DELETE(session->tree_in);
        MXML_DELETE(session->tree_out);
    }

success:
	session->error = CWMP_OK;
	goto end;
retry:
	session->error = CWMP_RETRY_SESSION;

end:
	MXML_DELETE(session->tree_in);
	MXML_DELETE(session->tree_out);
	http_client_exit();
	xml_exit();
    return session->error;
}

int cwmp_move_session_to_session_send (struct cwmp *cwmp, struct session *session)
{
    pthread_mutex_lock (&(cwmp->mutex_session_queue));
    if (cwmp->session_send != NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_MUTEX_ERR;
    }
    list_del (&(session->list));
    cwmp->session_send          = session;
    cwmp->head_event_container  = NULL;
    bkp_session_move_inform_to_inform_send ();
    bkp_session_save();
    pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    return CWMP_OK;
}

int cwmp_move_session_to_session_queue (struct cwmp *cwmp, struct session *session)
{
    struct list_head            *ilist,*jlist;
    struct rpc              	*rpc_acs,*queue_rpc_acs;
    struct event_container      *event_container_old, *event_container_new;
    struct session              *session_queue;
    bool                        dup;

    pthread_mutex_lock (&(cwmp->mutex_session_queue));
    cwmp->retry_count_session ++;
    cwmp->session_send  = NULL;
    if (cwmp->head_session_queue.next == &(cwmp->head_session_queue))
    {
        list_add_tail (&(session->list), &(cwmp->head_session_queue));
        session->hold_request       = 0;
        session->digest_auth        = 0;
        cwmp->head_event_container  = &(session->head_event_container);
        if (session->head_rpc_acs.next != &(session->head_rpc_acs))
        {
            rpc_acs = list_entry(session->head_rpc_acs.next, struct rpc, list);
            if (rpc_acs->type != RPC_ACS_INFORM)
            {
                if ((rpc_acs = cwmp_add_session_rpc_acs_head(session, RPC_ACS_INFORM)) == NULL)
                {
                    pthread_mutex_unlock (&(cwmp->mutex_session_queue));
                    return CWMP_MEM_ERR;
                }
            }
        }
        else
        {
        	if ((rpc_acs = cwmp_add_session_rpc_acs_head(session, RPC_ACS_INFORM)) == NULL)
			{
				pthread_mutex_unlock (&(cwmp->mutex_session_queue));
				return CWMP_MEM_ERR;
			}
        }
        bkp_session_move_inform_to_inform_queue ();
        bkp_session_save();
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_OK;
    }
    list_for_each(ilist, &(session->head_event_container))
    {
        event_container_old = list_entry (ilist, struct event_container, list);
        event_container_new = cwmp_add_event_container (cwmp, event_container_old->code, event_container_old->command_key);
        if (event_container_new == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        list_splice_init(&(event_container_old->head_parameter_container),
        		&(event_container_new->head_parameter_container));
        cwmp_save_event_container (cwmp,event_container_new);
    }
    session_queue = list_entry(cwmp->head_event_container,struct session, head_event_container);
    list_for_each(ilist, &(session->head_rpc_acs))
    {
        rpc_acs = list_entry(ilist, struct rpc, list);
        dup     = false;
        list_for_each(jlist, &(session_queue->head_rpc_acs))
        {
            queue_rpc_acs = list_entry(jlist, struct rpc, list);
            if (queue_rpc_acs->type == rpc_acs->type &&
                (rpc_acs->type == RPC_ACS_INFORM ||
                 rpc_acs->type == RPC_ACS_GET_RPC_METHODS))
            {
                dup = true;
                break;
            }
        }
        if (dup)
        {
            continue;
        }
        ilist = ilist->prev;
        list_del(&(rpc_acs->list));
        list_add_tail (&(rpc_acs->list), &(session_queue->head_rpc_acs));
    }
    cwmp_session_destructor (cwmp, session);
    pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    return CWMP_OK;
}

void cwmp_set_end_session (unsigned int end_session_flag)
{
	cwmp_main.session_send->end_session |= end_session_flag;
}

int cwmp_session_destructor (struct cwmp *cwmp, struct session *session)
{
	struct rpc          *rpc;
    struct session_end_func *session_end_func;

    while (session->head_rpc_acs.next != &(session->head_rpc_acs))
    {
    	rpc = list_entry(session->head_rpc_acs.next, struct rpc, list);
		if (rpc_acs_methods[rpc->type].extra_clean != NULL)
			rpc_acs_methods[rpc->type].extra_clean(session,rpc);
    	cwmp_session_rpc_destructor(rpc);
    }
    while (session->head_rpc_cpe.next != &(session->head_rpc_cpe))
    {
    	rpc = list_entry(session->head_rpc_cpe.next, struct rpc, list);
    	cwmp_session_rpc_destructor(rpc);
    }
    if (session->list.next != NULL && session->list.prev != NULL)
    {
        list_del (&(session->list));
    }
    free (session);

    return CWMP_OK;
}



struct session *cwmp_add_queue_session (struct cwmp *cwmp)
{
    struct session     *session;
    struct rpc		   *rpc_acs;

    session = calloc (1,sizeof(struct session));
    if (session==NULL)
    {
        return NULL;
    }
    list_add_tail (&(session->list), &(cwmp->head_session_queue));
    INIT_LIST_HEAD (&(session->head_event_container));
    INIT_LIST_HEAD (&(session->head_rpc_acs));
    INIT_LIST_HEAD (&(session->head_rpc_cpe));
    if ((rpc_acs = cwmp_add_session_rpc_acs_head(session, RPC_ACS_INFORM)) == NULL)
    {
    	free (session);
        return NULL;
    }

    return session;
}

int run_session_end_func (struct session *session)
{
	if (session->end_session & END_SESSION_EXTERNAL_ACTION)
		external_simple("end_session");

	if (session->end_session & END_SESSION_REBOOT)
		external_simple("reboot");

	if (session->end_session & END_SESSION_RELOAD)
		cwmp_apply_acs_changes();

	session->end_session = 0;

	return CWMP_OK;
}

int cwmp_apply_acs_changes ()
{
    int error;
    if (error = cwmp_config_reload(&cwmp_main))
    {
        return error;
    }
    if (error = cwmp_root_cause_events(&cwmp_main))
    {
        return error;
    }
    return CWMP_OK;
}

void *thread_uloop_run (void *v)
{
	thread_sync();
	ubus_init(&cwmp_main);
	return NULL;
}

int main(int argc, char **argv)
{

    struct cwmp                     *cwmp = &cwmp_main;
    int                             error;
    pthread_t                       session_scheduler_thread;
    pthread_t                       periodic_event_thread;
    pthread_t                       scheduleInform_thread;
    pthread_t                       download_thread;
    pthread_t                       ubus_thread;

    if (error = cwmp_init(argc, argv, cwmp))
    {
        return error;
    }
    CWMP_LOG(INFO,"STARTING CWMP");

    if (error = cwmp_load_saved_session(cwmp, NULL, ALL))
    {
        return error;
    }

    if (error = cwmp_root_cause_events(cwmp))
    {
        return error;
    }

    error = pthread_create(&ubus_thread, NULL, &thread_uloop_run, NULL);
    if (error<0)
	{
		CWMP_LOG(ERROR,"Error when creating the ubus thread!");
	}
    thread_wait_sync();
    error = pthread_create(&session_scheduler_thread, NULL, &cwmp_schedule_session, (void *)cwmp);
    if (error<0)
    {
        CWMP_LOG(EMERG,"FATAL error when creating the session scheduler thread!");
        exit(EXIT_FAILURE);
    }

    error = pthread_create(&periodic_event_thread, NULL, &thread_event_periodic, (void *)cwmp);
    if (error<0)
    {
        CWMP_LOG(ERROR,"Error error when creating the periodic event thread!");
    }
    error = pthread_create(&scheduleInform_thread, NULL, &thread_cwmp_rpc_cpe_scheduleInform, (void *)cwmp);
    if (error<0)
    {
        CWMP_LOG(ERROR,"Error when creating the scheduled inform thread!");
    }
    thread_wait_sync();
    error = pthread_create(&download_thread, NULL, &thread_cwmp_rpc_cpe_download, (void *)cwmp);
    if (error<0)
    {
        CWMP_LOG(ERROR,"Error when creating the download thread!");
    }

    pthread_join(ubus_thread, NULL);
    pthread_join(session_scheduler_thread, NULL);
    pthread_join(periodic_event_thread, NULL);
    pthread_join(scheduleInform_thread, NULL);
    pthread_join(download_thread, NULL);

    CWMP_LOG(INFO,"EXIT CWMP");
    return CWMP_OK;
}
