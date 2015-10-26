/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <unistd.h>
#include <libubus.h>
#include <sys/file.h>

#include "cwmp.h"
#include "ubus.h"
#include "external.h"
#include "xml.h"
#include "log.h"
#include "wepkey.h"
#include "dmentry.h"

static struct ubus_context *ctx = NULL;
static struct blob_buf b;

static const char *arr_session_status[] = {
    [SESSION_WAITING] = "waiting",
    [SESSION_RUNNING] = "running",
    [SESSION_FAILURE] = "failure",
    [SESSION_SUCCESS] = "success",
};

static const struct blobmsg_policy notify_policy[] = {};

static int
cwmp_handle_notify(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	bool send_signal = false;
	CWMP_LOG(INFO, "triggered ubus notification");

	blob_buf_init(&b, 0);

	pthread_mutex_lock(&(cwmp_main.mutex_handle_notify));
	if (!cwmp_main.count_handle_notify)
		send_signal = true;
	cwmp_main.count_handle_notify++;
	pthread_mutex_unlock(&(cwmp_main.mutex_handle_notify));
	if (send_signal)
		pthread_cond_signal(&(cwmp_main.threshold_handle_notify));

	blobmsg_add_u32(&b, "status", 1);
	ubus_send_reply(ctx, req, b.head);
	blob_buf_free(&b);

	return 0;
}


enum command {
	COMMAND_NAME,
	__COMMAND_MAX
};

static const struct blobmsg_policy command_policy[] = {
	[COMMAND_NAME] = { .name = "command", .type = BLOBMSG_TYPE_STRING },
};

static int
cwmp_handle_command(struct ubus_context *ctx, struct ubus_object *obj,
			 struct ubus_request_data *req, const char *method,
			 struct blob_attr *msg)
{
	struct blob_attr *tb[__COMMAND_MAX];

	blobmsg_parse(command_policy, ARRAYSIZEOF(command_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[COMMAND_NAME])
		return UBUS_STATUS_INVALID_ARGUMENT;

	blob_buf_init(&b, 0);

	char *cmd = blobmsg_data(tb[COMMAND_NAME]);
	char *info;

	if (!strcmp("reload_end_session", cmd)) {
		CWMP_LOG(INFO, "triggered ubus reload_end_session");
		cwmp_set_end_session(END_SESSION_RELOAD);
		blobmsg_add_u32(&b, "status", 0);
		if (asprintf(&info, "freecwmpd config will reload at the end of the session") == -1)
			return -1;
	} else if (!strcmp("reload", cmd)) {
		CWMP_LOG(INFO, "triggered ubus reload");
		if (cwmp_main.session_status.last_status == SESSION_RUNNING) {
			cwmp_set_end_session(END_SESSION_RELOAD);
			blobmsg_add_u32(&b, "status", 0);
			blobmsg_add_string(&b, "info", "Session running, reload at the end of the session");
		}
		else {
			pthread_mutex_lock (&(cwmp_main.mutex_session_queue));
			dm_global_init();
			cwmp_apply_acs_changes();
			pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));
			blobmsg_add_u32(&b, "status", 0);
			if (asprintf(&info, "freecwmp config reloaded") == -1)
				return -1;
		}
	} else if (!strcmp("reboot_end_session", cmd)) {
		CWMP_LOG(INFO, "triggered ubus reboot_end_session");
		cwmp_set_end_session(END_SESSION_REBOOT);
		blobmsg_add_u32(&b, "status", 0);
		if (asprintf(&info, "freecwmp will reboot at the end of the session") == -1)
			return -1;
	} else if (!strcmp("action_end_session", cmd)) {
		CWMP_LOG(INFO, "triggered ubus action_end_session");
		cwmp_set_end_session(END_SESSION_EXTERNAL_ACTION);
		blobmsg_add_u32(&b, "status", 0);
		if (asprintf(&info, "freecwmp will execute the scheduled action commands at the end of the session") == -1)
			return -1;
	} else if (!strcmp("exit", cmd)) {
		pthread_t exit_thread;
		int error;
		CWMP_LOG(INFO, "triggered ubus exit");
		int rc = flock(cwmp_main.pid_file, LOCK_UN | LOCK_NB);
		close(cwmp_main.pid_file);
		if(rc) {
			char *piderr = "PID file unlock failed!";
			fprintf(stderr, "%s\n", piderr);
			CWMP_LOG(ERROR, piderr);
		}
		blobmsg_add_u32(&b, "status", 0);
		if (asprintf(&info, "cwmpd daemon stopped") == -1)
			return -1;
		blobmsg_add_string(&b, "info", info);
		free(info);

		ubus_send_reply(ctx, req, b.head);

		blob_buf_free(&b);
		close(cwmp_main.cr_socket_desc);
		CWMP_LOG(INFO,"Close connection request server socket");
		error = pthread_create(&exit_thread, NULL, &thread_exit_program, NULL);
		if (error<0)
		{
			CWMP_LOG(ERROR,"Error when creating the exit thread!");
			return -1;
		}
		return 0;

	} else {
		blobmsg_add_u32(&b, "status", -1);
		if (asprintf(&info, "%s command is not supported", cmd) == -1)
			return -1;
	}

	blobmsg_add_string(&b, "info", info);
	free(info);

	ubus_send_reply(ctx, req, b.head);

	blob_buf_free(&b);

	return 0;
}

static inline time_t get_session_status_next_time() {
    time_t ntime = 0;
    if(list_schedule_inform.next!=&(list_schedule_inform)) {
        struct schedule_inform *schedule_inform;
        schedule_inform = list_entry(list_schedule_inform.next,struct schedule_inform, list);
        ntime = schedule_inform->scheduled_time;
    }
    if (!ntime || (cwmp_main.session_status.next_retry && ntime > cwmp_main.session_status.next_retry)) {
        ntime = cwmp_main.session_status.next_retry;
    }
    if (!ntime || (cwmp_main.session_status.next_periodic && ntime > cwmp_main.session_status.next_periodic)) {
        ntime = cwmp_main.session_status.next_periodic;
    }
    return ntime;
}

static const struct blobmsg_policy status_policy[] = {
};

static int
cwmp_handle_status(struct ubus_context *ctx, struct ubus_object *obj,
             struct ubus_request_data *req, const char *method,
             struct blob_attr *msg)
{
    void *c;
    time_t ntime = 0;

    blob_buf_init(&b, 0);

    c = blobmsg_open_table(&b, "cwmp");
    blobmsg_add_string(&b, "status", "up");
    blobmsg_add_string(&b, "start_time", mix_get_time_of(cwmp_main.start_time));
    blobmsg_add_string(&b, "acs_url", cwmp_main.conf.acsurl);
    blobmsg_close_table(&b, c);

    c = blobmsg_open_table(&b, "last_session");
    blobmsg_add_string(&b, "status", cwmp_main.session_status.last_start_time ? arr_session_status[cwmp_main.session_status.last_status] : "N/A");
    blobmsg_add_string(&b, "start_time", cwmp_main.session_status.last_start_time ? mix_get_time_of(cwmp_main.session_status.last_start_time) : "N/A");
    blobmsg_add_string(&b, "end_time", cwmp_main.session_status.last_end_time ? mix_get_time_of(cwmp_main.session_status.last_end_time) : "N/A");
    blobmsg_close_table(&b, c);

    c = blobmsg_open_table(&b, "next_session");
    blobmsg_add_string(&b, "status", arr_session_status[SESSION_WAITING]);
    ntime = get_session_status_next_time();
    blobmsg_add_string(&b, "start_time", ntime ? mix_get_time_of(ntime) : "N/A");
    blobmsg_add_string(&b, "end_time", "N/A");
    blobmsg_close_table(&b, c);

    c = blobmsg_open_table(&b, "statistics");
    blobmsg_add_u32(&b, "success_sessions", cwmp_main.session_status.success_session);
    blobmsg_add_u32(&b, "failure_sessions", cwmp_main.session_status.failure_session);
    blobmsg_add_u32(&b, "total_sessions", cwmp_main.session_status.success_session + cwmp_main.session_status.failure_session);
    blobmsg_close_table(&b, c);


    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);

    return 0;
}

enum enum_inform {
	INFORM_GET_RPC_METHODS,
	__INFORM_MAX
};

static const struct blobmsg_policy inform_policy[] = {
	[INFORM_GET_RPC_METHODS] = { .name = "GetRPCMethods", .type = BLOBMSG_TYPE_BOOL },
};

static int
cwmp_handle_inform(struct ubus_context *ctx, struct ubus_object *obj,
             struct ubus_request_data *req, const char *method,
             struct blob_attr *msg)
{
    struct blob_attr *tb[__INFORM_MAX];
    bool grm = false;
    struct event_container  *event_container;
    struct session          *session;

    blob_buf_init(&b, 0);

	blobmsg_parse(inform_policy, ARRAYSIZEOF(inform_policy), tb,
			  blob_data(msg), blob_len(msg));

	if (tb[INFORM_GET_RPC_METHODS]) {
		grm = blobmsg_data(tb[INFORM_GET_RPC_METHODS]);
	}
	if (grm) {
		pthread_mutex_lock (&(cwmp_main.mutex_session_queue));
		event_container = cwmp_add_event_container (&cwmp_main, EVENT_IDX_2PERIODIC, "");
		if (event_container == NULL)
		{
			pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));
			return 0;
		}
		cwmp_save_event_container (&cwmp_main,event_container);
		session = list_entry (cwmp_main.head_event_container, struct session,head_event_container);
		if(cwmp_add_session_rpc_acs(session, RPC_ACS_GET_RPC_METHODS) == NULL)
		{
			pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));
			return 0;
		}
		pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));
		pthread_cond_signal(&(cwmp_main.threshold_session_send));
		blobmsg_add_u32(&b, "status", 1);
		blobmsg_add_string(&b, "info", "Session with GetRPCMethods will start");
	}
	else {
		if (cwmp_main.session_status.last_status == SESSION_RUNNING) {
			blobmsg_add_u32(&b, "status", -1);
			blobmsg_add_string(&b, "info", "Session already running");
		}
		else {
			pthread_mutex_lock (&(cwmp_main.mutex_session_queue));
			cwmp_add_event_container (&cwmp_main, EVENT_IDX_6CONNECTION_REQUEST, "");
			pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));
			pthread_cond_signal(&(cwmp_main.threshold_session_send));
			blobmsg_add_u32(&b, "status", 1);
			blobmsg_add_string(&b, "info", "Session started");
		}
	}
    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);

    return 0;
}

static const struct ubus_method freecwmp_methods[] = {
	UBUS_METHOD("notify", cwmp_handle_notify, notify_policy),
	UBUS_METHOD("command", cwmp_handle_command, command_policy),
	UBUS_METHOD("status", cwmp_handle_status, status_policy),
	UBUS_METHOD("inform", cwmp_handle_inform, inform_policy),
};

static struct ubus_object_type main_object_type =
	UBUS_OBJECT_TYPE("freecwmpd", freecwmp_methods);

static struct ubus_object main_object = {
	.name = "tr069",
	.type = &main_object_type,
	.methods = freecwmp_methods,
	.n_methods = ARRAYSIZEOF(freecwmp_methods),
};

int
ubus_init(struct cwmp *cwmp)
{
	uloop_init();

	if (netlink_init()) {
		CWMP_LOG(ERROR,"netlink initialization failed");
	}

	ctx = ubus_connect(cwmp->conf.ubus_socket);
	if (!ctx) return -1;

	ubus_add_uloop(ctx);

	if (ubus_add_object(ctx, &main_object)) return -1;
		uloop_run();
	return 0;
}

void
ubus_exit(void)
{
	if (ctx) ubus_free(ctx);
}
