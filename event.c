/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *
 */

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/engine.h>
#include "cwmp.h"
#include "xml.h"
#include "backupSession.h"
#include "log.h"
#include "jshn.h"
#include "external.h"
#include "dmcwmp.h"
#include "deviceinfo.h"

LIST_HEAD(list_value_change);
LIST_HEAD(list_lw_value_change);
pthread_mutex_t mutex_value_change = PTHREAD_MUTEX_INITIALIZER;

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
    struct dm_parameter                 *dm_parameter;
    char                                section[256];
    mxml_node_t							*b;

    if (EVENT_CONST[event_container->code].RETRY & EVENT_RETRY_AFTER_REBOOT)
    {
        b = bkp_session_insert_event(event_container->code, event_container->command_key, event_container->id, "queue");

        list_for_each(ilist,&(event_container->head_dm_parameter))
        {
            dm_parameter = list_entry(ilist, struct dm_parameter, list);
            bkp_session_insert_parameter(b, dm_parameter->name);
        }
        bkp_session_save();
    }

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
    INIT_LIST_HEAD (&(event_container->head_dm_parameter));
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

void add_dm_parameter_tolist(struct list_head *head, char *param_name, char *param_data, char *param_type)
{
	struct dm_parameter *dm_parameter;
	struct list_head *ilist;
	int cmp;
	list_for_each (ilist, head) {
		dm_parameter = list_entry(ilist, struct dm_parameter, list);
		cmp = strcmp(dm_parameter->name, param_name);
		if (cmp == 0) {
			return;
		} else if (cmp > 0) {
			break;
		}
	}
	dm_parameter = calloc(1, sizeof(struct dm_parameter));
	_list_add(&dm_parameter->list, ilist->prev, ilist);
	if (param_name) dm_parameter->name = strdup(param_name);
	if (param_data) dm_parameter->data = strdup(param_data);
	if (param_type) dm_parameter->type = param_type ? param_type : "xsd:string";
}

void delete_dm_parameter_fromlist(struct dm_parameter *dm_parameter)
{
	list_del(&dm_parameter->list);
	free(dm_parameter->name);
	free(dm_parameter->data);
	free(dm_parameter);
}

void free_dm_parameter_all_fromlist(struct list_head *list)
{
	struct dm_parameter *dm_parameter;
	while (list->next!=list) {
		dm_parameter = list_entry(list->next, struct dm_parameter, list);
		delete_dm_parameter_fromlist(dm_parameter);
	}
}

inline void add_list_value_change(char *param_name, char *param_data, char *param_type)
{
	pthread_mutex_lock(&(mutex_value_change));
	add_dm_parameter_tolist(&list_value_change, param_name, param_data, param_type);
	pthread_mutex_unlock(&(mutex_value_change));
}
inline void add_lw_list_value_change(char *param_name, char *param_data, char *param_type)
{
	add_dm_parameter_tolist(&list_lw_value_change, param_name, param_data, param_type);
	
}

void udplw_server_param(struct addrinfo **res)
{
	struct addrinfo hints = {0};
	struct cwmp   *cwmp = &cwmp_main;
	struct config   *conf;
	char *port;
	conf = &(cwmp->conf);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	asprintf(&port, "%d", conf->lw_notification_port);
	getaddrinfo(conf->lw_notification_hostname,port,&hints,res);
	//FREE(port);
}

static void message_compute_signature(char *msg_out, char *signature)
{
	int i;
	int result_len = 20;
	unsigned char *result;
	struct cwmp   *cwmp = &cwmp_main;
	struct config   *conf;
	conf = &(cwmp->conf);
	result = HMAC(EVP_sha1(), conf->acs_passwd, strlen(conf->acs_passwd),
			msg_out, strlen(msg_out), NULL, NULL);
	for (i = 0; i < result_len; i++) {
		sprintf(&(signature[i * 2]), "%02X", result[i]);
	}
	signature[i * 2 ] = '\0';
	FREE(result);
}

char *calculate_lwnotification_cnonce()
{
	int i;
	char *cnonce = malloc( 33 * sizeof(char));
	srand((unsigned int) time(NULL));
	for (i = 0; i < 4; i++) {
		sprintf(&(cnonce[i * 8]), "%08x", rand());
	}
	cnonce[i * 8 ] = '\0';
	return cnonce;
}

static void send_udp_message(struct addrinfo *servaddr, char *msg)
{
	int fd;

	fd = socket(servaddr->ai_family, SOCK_DGRAM, 0);
	printf ("servaddr->ai_family %d \n", servaddr->ai_family);
	printf ("fd: %d \n", fd);
	
	if ( fd >= 0) {
		printf("msg %s \n", msg);
		sendto(fd, msg, strlen(msg), 0, servaddr->ai_addr, servaddr->ai_addrlen);
		close(fd);
	}
}

void del_list_lw_notify(struct dm_parameter *dm_parameter)
{
	
	list_del(&dm_parameter->list);
	free(dm_parameter->name);
	free(dm_parameter);
}

void free_all_list_lw_notify()
{
	struct dm_parameter *dm_parameter;
	while (list_lw_value_change.next != &list_lw_value_change) {
		dm_parameter = list_entry(list_lw_value_change.next, struct dm_parameter, list);
		printf("before del_list_lw_notify \n");
		del_list_lw_notify(dm_parameter);
		printf("after del_list_lw_notify \n");
	}
}

void cwmp_lwnotification()
{
	char *msg, *msg_out;
	char signature[41];
	struct addrinfo *servaddr;
	struct cwmp   *cwmp = &cwmp_main;
	struct config   *conf;
	conf = &(cwmp->conf);

	printf("before udplw_server_param \n");
	udplw_server_param(&servaddr);
	printf("after udplw_server_param \n");
	xml_prepare_lwnotification_message(&msg_out);
	printf("after xml_prepare_lwnotification_message \n");
	message_compute_signature(msg_out, signature);
	printf("after message_compute_signature \n");
	asprintf(&msg, "%s \n %s: %s \n %s: %s \n %s: %d\n %s: %s\n\n%s",
			"POST /HTTPS/1.1",
			"HOST",	conf->lw_notification_hostname,
			"Content-Type", "test/xml; charset=utf-8",
			"Content-Lenght", strlen(msg_out),
			"Signature",signature,
			msg_out);

	send_udp_message(servaddr, msg);
	printf("after send_udp_message \n");
	free_all_list_lw_notify(); 
	printf("free_all_list_enabled_lwnotify \n");

	//freeaddrinfo(servaddr); //To check
	printf("freeaddrinfo \n");
	FREE(msg);
	FREE(msg_out);
}

void cwmp_add_notification(void)
{
	int fault;
	int i = 0;
	struct event_container   *event_container;
	struct cwmp   *cwmp = &cwmp_main;
	struct dm_enabled_notify *p;
	struct dm_parameter *dm_parameter;
	struct dmctx dmctx = {0};
	struct config   *conf;
	conf = &(cwmp->conf);	
	bool isactive = false;
	bool initiate = false;
	bool lw_isactive = false;

	pthread_mutex_lock(&(cwmp->mutex_session_send));
	pthread_mutex_lock(&(cwmp->mutex_handle_notify));
	cwmp->count_handle_notify = 0;
	pthread_mutex_unlock(&(cwmp->mutex_handle_notify));

	dm_ctx_init(&dmctx);
	list_for_each_entry(p, &list_enabled_notify, list) {
		dm_ctx_init_sub(&dmctx);
		initiate = true;
		fault = dm_entry_param_method(&dmctx, CMD_GET_VALUE, p->name, NULL, NULL);
		if (!fault && dmctx.list_parameter.next != &dmctx.list_parameter) {
			dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);
			if (strcmp(dm_parameter->data, p->value) != 0) {
				dm_update_enabled_notify(p, dm_parameter->data);
				if (p->notification[0] == '1' || p->notification[0] == '2' || p->notification[0] == '4' || p->notification[0] == '6' ) 
				add_list_value_change(p->name, dm_parameter->data, dm_parameter->type);
				if (p->notification[0] == '2')
					isactive = true;
			}
		}
		//dm_ctx_clean_sub(&dmctx);
	}
	//dm_ctx_clean(&dmctx);
	//dm_ctx_init(&dmctx);
	list_for_each_entry(p, &list_enabled_lw_notify, list) {
		if (!initiate || i != 0)		
			dm_ctx_init_sub(&dmctx);
		i++;
		if (!conf->lw_notification_enable)
			break;		
		fault = dm_entry_param_method(&dmctx, CMD_GET_VALUE, p->name, NULL, NULL);
		if (!fault && dmctx.list_parameter.next != &dmctx.list_parameter) {
			dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);
			if (strcmp(dm_parameter->data, p->value) != 0) {
				dm_update_enabled_notify(p, dm_parameter->data);
				if (p->notification[0] >= '3' )
					add_lw_list_value_change(p->name, dm_parameter->data, dm_parameter->type);
				if (p->notification[0] == '5' || p->notification[0] == '6')
					lw_isactive = true;
			}
		}
		dm_ctx_clean_sub(&dmctx);
	}
	dm_ctx_clean(&dmctx);
	if (lw_isactive) {
		cwmp_lwnotification();
	}
	pthread_mutex_unlock(&(cwmp->mutex_session_send));
	if (isactive)
	{
		pthread_mutex_lock(&(cwmp->mutex_session_queue));
		event_container = cwmp_add_event_container(cwmp, EVENT_IDX_4VALUE_CHANGE, "");
		if (event_container == NULL)
		{
			pthread_mutex_unlock(&(cwmp->mutex_session_queue));
			return;
		}
		cwmp_save_event_container(cwmp,event_container);
		pthread_mutex_unlock(&(cwmp->mutex_session_queue));
		pthread_cond_signal(&(cwmp->threshold_session_send));
		return;
	}
}

void cwmp_root_cause_event_ipdiagnostic(void)
{
	struct cwmp   *cwmp = &cwmp_main;
	struct event_container   *event_container;
    
	pthread_mutex_lock (&(cwmp->mutex_session_queue));        
	event_container = cwmp_add_event_container (cwmp, EVENT_IDX_8DIAGNOSTICS_COMPLETE, "");
    if (event_container == NULL)
	{
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
		return;
	}    
    cwmp_save_event_container(cwmp,event_container);
	pthread_mutex_unlock (&(cwmp->mutex_session_queue));
	pthread_cond_signal(&(cwmp->threshold_session_send));
    return;		
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
    struct dm_parameter                 *dm_parameter;

    while (session->head_event_container.next!=&(session->head_event_container))
    {
        event_container = list_entry(session->head_event_container.next, struct event_container, list);
        bkp_session_delete_event(event_container->id, rem_from?"send":"queue");
        if (event_container->code == EVENT_IDX_1BOOT && rem_from == RPC_SEND) {
        	remove("/etc/icwmpd/.icwmpd_boot");
        }
        free (event_container->command_key);
        free_dm_parameter_all_fromlist(&(event_container->head_dm_parameter));
        list_del(&(event_container->list));
        free (event_container);
    }
    bkp_session_save();
    return CWMP_OK;
}

int event_remove_noretry_event_container(struct session *session)
{
	struct event_container              *event_container;
	struct dm_parameter                 *dm_parameter;
	struct list_head *ilist, *q;

	list_for_each_safe(ilist,q,&(session->head_event_container))
	{
		event_container = list_entry(ilist, struct event_container, list);
		if (EVENT_CONST[event_container->code].RETRY == 0) {
			free (event_container->command_key);
			free_dm_parameter_all_fromlist(&(event_container->head_dm_parameter));
			list_del(&(event_container->list));
			free (event_container);
		}        
	}
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
        FREE(acsurl);
        if (event_container == NULL)
        {
            pthread_mutex_unlock (&(cwmp->mutex_session_queue));
            return CWMP_MEM_ERR;
        }
        cwmp_save_event_container (cwmp,event_container);
        cwmp_scheduleInform_remove_all();
        cwmp_scheduledDownload_remove_all();
        pthread_mutex_unlock (&(cwmp->mutex_session_queue));
    } else {
        FREE(acsurl);
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
        add_dm_parameter_tolist(&(event_container->head_dm_parameter),
        		DMROOT"ManagementServer.URL", NULL, NULL);
        cwmp_save_event_container (cwmp,event_container);
        save_acs_bkp_config(cwmp);
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
    switch (p->type) {
        case TYPE_DOWNLOAD:
    event_container = cwmp_add_event_container (cwmp, EVENT_IDX_M_Download, p->command_key?p->command_key:"");
		if (event_container == NULL)
		{
			pthread_mutex_unlock (&(cwmp->mutex_session_queue));
			return CWMP_MEM_ERR;
		}		
	case TYPE_UPLOAD:
		event_container = cwmp_add_event_container (cwmp, EVENT_IDX_M_Upload, p->command_key?p->command_key:"");
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

void *thread_handle_notify(void *v)
{
    struct cwmp                 *cwmp = (struct cwmp *) v;

    for(;;)
    {
        pthread_mutex_lock(&(cwmp->mutex_handle_notify));
        pthread_cond_wait(&(cwmp->threshold_handle_notify), &(cwmp->mutex_handle_notify));
        pthread_mutex_unlock(&(cwmp->mutex_handle_notify));
		while(cwmp->count_handle_notify) {
			cwmp_add_notification();
		}

    }
    return CWMP_OK;
}

void *thread_event_periodic (void *v)
{
    struct cwmp                 *cwmp = (struct cwmp *) v;
    struct event_container      *event_container;
    static int                  periodic_interval;
    static bool 				periodic_enable;
    static time_t				periodic_time;
    static struct timespec      periodic_timeout = {0, 0};
    time_t						current_time;
    long int					delta_time;

    periodic_interval 	= cwmp->conf.period;
    periodic_enable		= cwmp->conf.periodic_enable;
    periodic_time		= cwmp->conf.time;

    for(;;)
    {
        pthread_mutex_lock (&(cwmp->mutex_periodic));
        if (cwmp->conf.periodic_enable)
        {
	        current_time = time(NULL);
		    if(periodic_time != 0)
		    {
		    	delta_time = (current_time - periodic_time) % periodic_interval;
		    	if (delta_time >= 0)
		    		periodic_timeout.tv_sec = current_time + periodic_interval - delta_time;
	    		else
	    			periodic_timeout.tv_sec = current_time - delta_time;
		    }
		    else
		    {
		    	periodic_timeout.tv_sec = current_time + periodic_interval;
		    }
		    cwmp->session_status.next_periodic = periodic_timeout.tv_sec;
        	pthread_cond_timedwait(&(cwmp->threshold_periodic), &(cwmp->mutex_periodic), &periodic_timeout);
        }
        else
        {
            cwmp->session_status.next_periodic = 0;
        	pthread_cond_wait(&(cwmp->threshold_periodic), &(cwmp->mutex_periodic));
        }
        pthread_mutex_unlock (&(cwmp->mutex_periodic));
        if (periodic_interval != cwmp->conf.period || periodic_enable != cwmp->conf.periodic_enable || periodic_time != cwmp->conf.time)
        {
        	periodic_enable		= cwmp->conf.periodic_enable;
        	periodic_interval	= cwmp->conf.period;
        	periodic_time		= cwmp->conf.time;
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
    static time_t	periodic_time = 0;
    char 			local_time[26] = {0};
    struct tm 		*t_tm;
    
    if (period==cwmp->conf.period && periodic_enable==cwmp->conf.periodic_enable && periodic_time==cwmp->conf.time)
    {
        return CWMP_OK;
    }
    pthread_mutex_lock (&(cwmp->mutex_periodic));
    period  		= cwmp->conf.period;
    periodic_enable = cwmp->conf.periodic_enable;
    periodic_time	= cwmp->conf.time;
    CWMP_LOG(INFO,periodic_enable?"Periodic event is enabled. Interval period = %ds":"Periodic event is disabled", period);
	
	t_tm = localtime(&periodic_time);
	if (t_tm == NULL)
		return CWMP_GEN_ERR;

	if(strftime(local_time, sizeof(local_time), "%FT%T%z", t_tm) == 0)
		return CWMP_GEN_ERR;
	
	local_time[25] = local_time[24];
	local_time[24] = local_time[23];
	local_time[22] = ':';
	local_time[26] = '\0';
	
    CWMP_LOG(INFO,periodic_time?"Periodic time is %s":"Periodic time is Unknown", local_time);
    pthread_mutex_unlock (&(cwmp->mutex_periodic));
    pthread_cond_signal(&(cwmp->threshold_periodic));
    return CWMP_OK;
}

void sotfware_version_value_change(struct cwmp *cwmp, struct transfer_complete *p)
{
	char *current_software_version = NULL;

	if (!p->old_software_version || p->old_software_version[0] == 0)
		return;

	current_software_version = cwmp->deviceid.softwareversion;
	if (p->old_software_version && current_software_version &&
		strcmp(p->old_software_version, current_software_version) != 0) {
		pthread_mutex_lock (&(cwmp->mutex_session_queue));
		cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
		pthread_mutex_unlock (&(cwmp->mutex_session_queue));
	}
}


void connection_request_ip_value_change(struct cwmp *cwmp)
{
	char *bip = NULL;
	struct event_container *event_container;
	int error;

	error   = cwmp_load_saved_session(cwmp, &bip, CR_IP);

	if(bip == NULL)
	{
		bkp_session_simple_insert_in_parent("connection_request", "ip", cwmp->conf.ip);
		bkp_session_save();
		return;
	}
	if (strcmp(bip, cwmp->conf.ip)!=0)
	{
		pthread_mutex_lock (&(cwmp->mutex_session_queue));
		event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
		if (event_container == NULL)
		{
			FREE(bip);
			pthread_mutex_unlock (&(cwmp->mutex_session_queue));
			return;
		}
		cwmp_save_event_container (cwmp,event_container);
		bkp_session_simple_insert_in_parent("connection_request", "ip", cwmp->conf.ip);
		bkp_session_save();
		pthread_mutex_unlock (&(cwmp->mutex_session_queue));
		pthread_cond_signal(&(cwmp->threshold_session_send));
	}
	FREE(bip);
}

void connection_request_port_value_change(struct cwmp *cwmp, int port)
{
	char *bport = NULL;
	struct event_container *event_container;
	int error;
	char bufport[32];

	sprintf(bufport, "%d", port);

	error   = cwmp_load_saved_session(cwmp, &bport, CR_PORT);

	if(bport == NULL)
	{
		bkp_session_simple_insert_in_parent("connection_request", "port", bufport);
		bkp_session_save();
		return;
	}
	if (strcmp(bport, bufport)!=0)
	{
		event_container = cwmp_add_event_container (cwmp, EVENT_IDX_4VALUE_CHANGE, "");
		if (event_container == NULL)
		{
			FREE(bport);
			return;
		}
		cwmp_save_event_container (cwmp,event_container);
		bkp_session_simple_insert_in_parent("connection_request", "port", bufport);
		bkp_session_save();
	}
	FREE(bport);
}

int cwmp_root_cause_events (struct cwmp *cwmp)
{
    int error;

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

