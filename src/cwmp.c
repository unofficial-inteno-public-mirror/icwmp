/*
    cwmp.c

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
#include "cwmp.h"
#include "backupSession.h"


#define CWMP_DAEMON_MULTITHREAD  1/* TODO KMD need for debug*/

struct cwmp         	cwmp_main;
static pthread_mutex_t	thread_sync_mutex		= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	thread_sync_cond		= PTHREAD_COND_INITIALIZER;
static bool				thread_sync_wait		= TRUE;

struct rpc_acs *cwmp_add_session_rpc_acs_inform (struct session *session);
struct rpc_cpe  *cwmp_soap_receive_rpc_cpe (struct cwmp *cwmp,struct session *session);
struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);
struct paramater_container *cwmp_add_parameter_container (struct cwmp *cwmp, struct event_container *event_container, char *name, char *value, char *type);
void backup_session_move_inform_to_inform_send ();
void backup_session_move_inform_to_inform_queue ();
void cwmp_save_event_container (struct cwmp *cwmp,struct event_container *event_container);
void *cwmp_schedule_session (void *v);
void *thread_event_periodic (void *v);
void *thread_connection_request_listener (void *v);
void *thread_cwmp_rpc_cpe_scheduleInform (void *v);
void *thread_cwmp_rpc_cpe_download (void *v);

struct rpc_acs *cwmp_add_session_rpc_acs (struct session *session)
{
    struct rpc_acs     *rpc_acs;

    rpc_acs = calloc (1,sizeof(struct rpc_acs));
    if (rpc_acs==NULL)
    {
        return NULL;
    }
    list_add_tail (&(rpc_acs->list), &(session->head_rpc_acs));
    return rpc_acs;
}

struct rpc_cpe *cwmp_add_session_rpc_cpe (struct session *session)
{
    struct rpc_cpe     *rpc_cpe;

    rpc_cpe = calloc (1,sizeof(struct rpc_cpe));
    if (rpc_cpe==NULL)
    {
        return NULL;
    }
    list_add_tail (&(rpc_cpe->list), &(session->head_rpc_cpe));
    return rpc_cpe;
}

struct rpc_acs *cwmp_add_session_rpc_acs_head (struct session *session)
{
    struct rpc_acs     *rpc_acs;

    rpc_acs = calloc (1,sizeof(struct rpc_acs));
    if (rpc_acs==NULL)
    {
        return NULL;
    }
    list_add (&(rpc_acs->list), &(session->head_rpc_acs));
    return rpc_acs;
}

int cwmp_session_rpc_cpe_destructor (struct cwmp *cwmp, struct session *session, struct rpc_cpe *rpc_cpe)
{
    if (rpc_cpe == NULL)
    {
        return CWMP_OK;
    }

    if (rpc_cpe->method_data!=NULL)
    {
        free (rpc_cpe->method_data);
    }
    if (rpc_cpe->method_response_data!=NULL)
    {
        free (rpc_cpe->method_response_data);
    }
    list_del(&(rpc_cpe->list));
    free (rpc_cpe);
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

int cwmp_session_rpc_acs_destructor (struct cwmp *cwmp, struct session *session, struct rpc_acs *rpc_acs)
{
    if (rpc_acs == NULL)
    {
        return CWMP_OK;
    }

    if (rpc_acs->method_data!=NULL)
    {
        free (rpc_acs->method_data);
    }
    if (rpc_acs->method_response_data!=NULL)
    {
        free (rpc_acs->method_response_data);
    }
    list_del(&(rpc_acs->list));
    free (rpc_acs);
    return CWMP_OK;
}

void thread_sync()
{
	pthread_mutex_lock(&thread_sync_mutex);
	thread_sync_wait = FALSE;
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
	thread_sync_wait = TRUE;
	pthread_mutex_unlock(&thread_sync_mutex);
}

void *cwmp_schedule_session (void *v)
{
    struct list_head                    *ilist;
    struct session                      *session;
    int                                 t,error = CWMP_OK;
    static struct timespec              time_to_wait = {0, 0};
    bool                                retry = FALSE;
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
            retry = FALSE;
        }
        session = list_entry(ilist, struct session, list);
        cwmp_prepare_session_to_session_send (cwmp,session);
        if (error = cwmp_move_session_to_session_send (cwmp, session))
        {
            CWMP_LOG(EMERG,"FATAL error in the mutex process in the session shceduler!");
            exit(EXIT_FAILURE);
        }
        error = cwmp_schedule_rpc (cwmp,session);
        run_session_end_func(cwmp,&(session->head_session_end_func));
        if (session->error == CWMP_RETRY_SESSION)
        {
            error = cwmp_move_session_to_session_queue (cwmp, session);
            CWMP_LOG(INFO,"Retry session, retry count = %d, retry in %ds",cwmp->retry_count_session,cwmp_get_retry_interval(cwmp));
            retry = TRUE;
            pthread_mutex_unlock (&(cwmp->mutex_session_send));
            continue;
        }
        cwmp_session_destructor (cwmp, session);
        cwmp->session_send          = NULL;
        cwmp->retry_count_session   = 0;
        pthread_mutex_unlock (&(cwmp->mutex_session_send));
# if !CWMP_DAEMON_MULTITHREAD /* TODO KMD need for debug*/
        break;
#endif
    }
    return CWMP_OK;
}

int cwmp_schedule_rpc (struct cwmp *cwmp, struct session *session)
{
    struct list_head                    *ilist;
    struct rpc_acs                      *rpc_acs;
    struct rpc_cpe                      *rpc_cpe;
    int                                 error=0,e;

    cwmp_session_soap_init(session);

    while (1)
    {
        session->error = CWMP_OK;
        list_for_each(ilist, &(session->head_rpc_acs))
        {
            rpc_acs = list_entry (ilist, struct rpc_acs, list);
            if (!(error = rpc_acs->method_data_init (cwmp, session, rpc_acs)))
            {
                rpc_acs->method_remote_call(cwmp, session, rpc_acs);
                if (rpc_acs->error==CWMP_SUCCESS_RPC && session->error!=CWMP_RETRY_SESSION)
                {
                    error = rpc_acs->method_response (cwmp, session, rpc_acs);
                }
            }

            if (e = rpc_acs->method_end (cwmp, session, rpc_acs))
            {
                error = e;
            }

            cwmp_session_soap_destroy_end (session);
            if (error || session->error==CWMP_RETRY_SESSION)
            {
                session->error = CWMP_RETRY_SESSION;
                goto end_rpc_sheculer;
            }
            ilist = ilist->prev;
            if (error = rpc_acs->destructor (cwmp, session, rpc_acs))
            {
                session->error = CWMP_RETRY_SESSION;
                goto end_rpc_sheculer;
            }

            if (session->hold_request)
            {
                break;
            }
        }

        while (1)
        {
            session->error = CWMP_OK;
            cwmp_soap_send_http_empty (cwmp,session);

            if (session->error == CWMP_RETRY_SESSION)
            {
                goto end_rpc_sheculer;
            }
            session->error = CWMP_OK;
            e = cwmp_soap_recv_http_empty (cwmp,session);
            if (session->error == CWMP_RETRY_SESSION)
            {
                goto end_rpc_sheculer;
            }
            else if (e == CWMP_UNAUTHORIZED_401)
            {
                CWMP_LOG(INFO,"Receive http 401: need authentication");
                cwmp_session_soap_destroy_end (session);
                continue;
            }
            else if (session->error == CWMP_CONTINUE_SESSION ||
                    session->error == CWMP_SUCCESS_SESSION)
            {
                break;
            }
            session->error = CWMP_RETRY_SESSION;
            goto end_rpc_sheculer;
        }
        if (session->error == CWMP_CONTINUE_SESSION)
        {
            while (1)
            {
                rpc_cpe = cwmp_soap_receive_rpc_cpe (cwmp, session);
                if (rpc_cpe == NULL)
                {
                    session->error = CWMP_RETRY_SESSION;
                    goto end_rpc_sheculer;
                }
                while (1)
                {
                    if (!(error = rpc_cpe->method(cwmp, session, rpc_cpe)))
                    {
                        if (!(error = rpc_cpe->method_response_data_init(cwmp, session, rpc_cpe)))
                        {
                            while (1)
                            {
                                cwmp_session_soap_destroy_end (session);
                                if (error = rpc_cpe->method_response(cwmp, session, rpc_cpe))
                                {
                                    session->error = CWMP_RETRY_SESSION;
                                    break;
                                }
                                session->error = CWMP_OK;
                                e = cwmp_soap_recv_http_empty (cwmp,session);
                                if (session->error == CWMP_RETRY_SESSION    ||
                                    session->error == CWMP_CONTINUE_SESSION ||
                                    session->error == CWMP_SUCCESS_SESSION)
                                {
                                    break;
                                }
                                else if (e == CWMP_UNAUTHORIZED_401)
                                {
                                    CWMP_LOG(INFO,"Receive http 401: need authentication");
                                    continue;
                                }
                                session->error = CWMP_RETRY_SESSION;
                                break;
                            }
                        }
                        else if(error != CWMP_FAULT_CPE)
                        {
                        	session->error = CWMP_RETRY_SESSION;
                        }
                    }
                    else if(error != CWMP_FAULT_CPE)
                    {
                    	session->error = CWMP_RETRY_SESSION;
                    }

                    if (rpc_cpe->method_end(cwmp, session, rpc_cpe) ||
                        rpc_cpe->destructor(cwmp, session, rpc_cpe))
                    {
                        session->error = CWMP_RETRY_SESSION;
                    }

                    if (session->error == CWMP_RETRY_SESSION)
                    {
                        goto end_rpc_sheculer;
                    }

                    ilist = session->head_rpc_cpe.next;
                    if (ilist == &(session->head_rpc_cpe))
                    {
                        break;
                    }
                    rpc_cpe = list_entry(ilist, struct rpc_cpe, list);
                }
                if (session->error != CWMP_CONTINUE_SESSION)
                {
                    break;
                }
            }
        }
        if (session->head_rpc_acs.next==&(session->head_rpc_acs))
        {
            break;
        }
    }
end_rpc_sheculer:
    cwmp_session_done(session);
    return session->error;
}

int cwmp_prepare_session_to_session_send (struct cwmp *cwmp, struct session *session)
{
    cwmp_root_cause_event_api_value_change (cwmp,session);
    return CWMP_OK;
}

int cwmp_move_session_to_session_send (struct cwmp *cwmp, struct session *session)
{
    pthread_mutex_lock (&(cwmp->mutex_session_queue));
    if (cwmp->session_send != NULL)
    {
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return cwmp->error = CWMP_MUTEX_ERR;
    }
    list_del (&(session->list));
    strcpy (session->acs_url, cwmp->conf.acsurl);
    cwmp->session_send          = session;
    cwmp->head_event_container  = NULL;
    backup_session_move_inform_to_inform_send ();
    pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    return CWMP_OK;
}

int cwmp_move_session_to_session_queue (struct cwmp *cwmp, struct session *session)
{
    struct list_head            *ilist,*jlist;
    struct rpc_acs              *rpc_acs,*queue_rpc_acs;
    struct event_container      *event_container_old, *event_container_new;
    struct paramater_container  *paramater_container_old, *paramater_container_new;
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
            rpc_acs = list_entry(session->head_rpc_acs.next, struct rpc_acs, list);
            if (rpc_acs->type != RPC_ACS_INFORM_IDX)
            {
                if (cwmp_add_session_rpc_acs_inform (session) == NULL)
                {
                    pthread_mutex_unlock (&(cwmp->mutex_session_queue));
                    return CWMP_MEM_ERR;
                }
            }
        }
        else
        {
            if (cwmp_add_session_rpc_acs_inform (session) == NULL)
            {
                pthread_mutex_unlock (&(cwmp->mutex_session_queue));
                return CWMP_MEM_ERR;
            }
        }
        backup_session_move_inform_to_inform_queue ();
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
        return CWMP_OK;
    }
    list_for_each(ilist, &(session->head_event_container))
    {
        event_container_old = list_entry (ilist, struct event_container, list);
        event_container_new = cwmp_add_event_container (cwmp, event_container_old->idx, event_container_old->event.CommandKey);
        if (event_container_new == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        list_for_each(jlist, &(event_container_old->head_paramater_container))
        {
            paramater_container_old = list_entry(jlist,struct paramater_container, list);
            paramater_container_new = cwmp_add_parameter_container (cwmp,event_container_new, paramater_container_old->paramater.Name, paramater_container_old->paramater.Value, paramater_container_old->paramater.Type);
        }
        cwmp_save_event_container (cwmp,event_container_new);
    }
    session_queue = list_entry(cwmp->head_event_container,struct session, head_event_container);
    list_for_each(ilist, &(session->head_rpc_acs))
    {
        rpc_acs = list_entry(ilist, struct rpc_acs, list);
        dup     = FALSE;
        list_for_each(jlist, &(session_queue->head_rpc_acs))
        {
            queue_rpc_acs = list_entry(jlist, struct rpc_acs, list);
            if (queue_rpc_acs->type == rpc_acs->type &&
                (rpc_acs->type == RPC_ACS_INFORM_IDX ||
                 rpc_acs->type == RPC_ACS_GETRPCMETHODS_IDX))
            {
                dup = TRUE;
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

int cwmp_session_destructor (struct cwmp *cwmp, struct session *session)
{
    struct rpc_acs          *rpc_acs;
    struct rpc_cpe          *rpc_cpe;
    struct session_end_func *session_end_func;

    while (session->head_rpc_acs.next != &(session->head_rpc_acs))
    {
        rpc_acs = list_entry(session->head_rpc_acs.next, struct rpc_acs, list);
        rpc_acs->destructor (cwmp, session, rpc_acs);
    }
    while (session->head_rpc_cpe.next != &(session->head_rpc_cpe))
    {
        rpc_cpe = list_entry(session->head_rpc_cpe.next, struct rpc_cpe, list);
        rpc_cpe->destructor (cwmp, session, rpc_cpe);
    }
    while (session->head_session_end_func.next != &(session->head_session_end_func))
    {
        session_end_func = list_entry(session->head_session_end_func.next, struct session_end_func, list);
        list_del (&(session_end_func->list));
        free (session_end_func);
    }
    if (session->list.next != NULL && session->list.prev != NULL) /* KMD Modified for freecwmp integration*/
    {
        list_del (&(session->list));
    }
    free (session);

    return CWMP_OK;
}

struct session *cwmp_add_queue_session (struct cwmp *cwmp)
{
    struct session     *session;

    session = calloc (1,sizeof(struct session));
    if (session==NULL)
    {
        return NULL;
    }
    list_add_tail (&(session->list), &(cwmp->head_session_queue));
    INIT_LIST_HEAD (&(session->head_event_container));
    INIT_LIST_HEAD (&(session->head_session_end_func));
    INIT_LIST_HEAD (&(session->head_rpc_acs));
    INIT_LIST_HEAD (&(session->head_rpc_cpe));
    if (cwmp_add_session_rpc_acs_inform (session) == NULL)
    {
        free (session);
        return NULL;
    }

    return session;
}

int add_session_end_func (struct session *session, int (*func)(struct cwmp *cwmp, void *input),void *input, bool end)
{
    struct session_end_func     *session_end_func;
    struct list_head            *ilist;

    list_for_each(ilist,&(session->head_session_end_func))
    {
        session_end_func = list_entry(ilist, struct session_end_func, list);
        if ((session_end_func->func==func) &&
            (session_end_func->input==input))
        {
            return CWMP_OK;
        }
    }

    session_end_func = calloc(1,sizeof(struct session_end_func));
    session_end_func->func  = func;
    session_end_func->input = input;

    if (end==TRUE)
    {
        list_add_tail(&(session_end_func->list),&(session->head_session_end_func));
    }
    else
    {
        list_add(&(session_end_func->list),&(session->head_session_end_func));
    }
    return CWMP_OK;
}
int run_session_end_func (struct cwmp *cwmp, struct list_head *head_func)
{
    struct list_head            *ilist;
    struct session_end_func     *session_end_func;
    int                         error = CWMP_OK;
    list_for_each(ilist,head_func)
    {
        session_end_func = list_entry(ilist, struct session_end_func, list);
        error = session_end_func->func(cwmp,session_end_func->input);
        ilist = ilist->prev;
        list_del(&(session_end_func->list));
        free (session_end_func);
    }
    return error;
}

int cwmp_apply_acs_changes (struct cwmp *cwmp)
{
    int error;
    if (error = cwmp_config_reload(cwmp))
    {
        return error;
    }
    if (error = cwmp_root_cause_events(cwmp))
    {
        return error;
    }
    return CWMP_OK;
}

void *thread_uloop_run (void *v)
{
	ubus_init(&cwmp_main);
	return NULL;
}

int main(int argc, char **argv)
{

    struct cwmp                     *cwmp = &cwmp_main;
    int                             error;
    pthread_t                       session_scheduler_thread;
    pthread_t                       periodic_event_thread;
    pthread_t                       connection_request_thread;
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
# if !CWMP_DAEMON_MULTITHREAD /* TODO KMD need for debug*/

    cwmp_schedule_session(cwmp);

#else
    error = pthread_create(&ubus_thread, NULL, &thread_uloop_run, NULL);
    if (error<0)
	{
		CWMP_LOG(ERROR,"Error when creating the ubus thread!");
	}
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
    error = pthread_create(&connection_request_thread, NULL, &thread_connection_request_listener, (void *)cwmp);
    if (error<0)
    {
        CWMP_LOG(ERROR,"Error when creating the connection request thread!");
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
    pthread_join(connection_request_thread, NULL);
    pthread_join(scheduleInform_thread, NULL);
    pthread_join(download_thread, NULL);

#endif

    CWMP_LOG(INFO,"EXIT CWMP");
    return CWMP_OK;
}
