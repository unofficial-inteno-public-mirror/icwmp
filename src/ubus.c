/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	Contributed by Inteno Broadband Technology AB
 *
 *	Copyright (C) 2013 Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	Copyright (C) 2013 Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <unistd.h>
#include <libubus.h>
#include <sys/file.h>

#include "cwmp.h"
#include "ubus.h"
#include "external.h"
#include "log.h"

static struct ubus_context *ctx = NULL;
static struct blob_buf b;

enum notify {
	NOTIFY_PARAM,
	NOTIFY_VALUE,
	NOTIFY_ATTRIB,
	NOTIFY_TYPE,
	__NOTIFY_MAX
};

static const struct blobmsg_policy notify_policy[] = {
	[NOTIFY_PARAM] = { .name = "parameter", .type = BLOBMSG_TYPE_STRING },
	[NOTIFY_VALUE] = { .name = "value", .type = BLOBMSG_TYPE_STRING },
	[NOTIFY_ATTRIB] = { .name = "attribute", .type = BLOBMSG_TYPE_STRING },
	[NOTIFY_TYPE] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
};

static int
cwmp_handle_notify(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
	struct blob_attr *tb[__NOTIFY_MAX];

	blobmsg_parse(notify_policy, ARRAYSIZEOF(notify_policy), tb,
		      blob_data(msg), blob_len(msg));

	if (!tb[NOTIFY_PARAM])
		return UBUS_STATUS_INVALID_ARGUMENT;

	if (!tb[NOTIFY_ATTRIB])
			return UBUS_STATUS_INVALID_ARGUMENT;

	CWMP_LOG(INFO, "triggered ubus notification parameter %s",
						     blobmsg_data(tb[NOTIFY_PARAM]));
	cwmp_add_notification(blobmsg_data(tb[NOTIFY_PARAM]),
			tb[NOTIFY_VALUE]? blobmsg_data(tb[NOTIFY_VALUE]) : NULL,
			blobmsg_data(tb[NOTIFY_ATTRIB]),
			tb[NOTIFY_TYPE]? blobmsg_data(tb[NOTIFY_TYPE]) : NULL);

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
		cwmp_apply_acs_changes();
		blobmsg_add_u32(&b, "status", 0);
		if (asprintf(&info, "freecwmp config reloaded") == -1)
			return -1;
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

static const struct ubus_method freecwmp_methods[] = {
	UBUS_METHOD("notify", cwmp_handle_notify, notify_policy),
	UBUS_METHOD("command", cwmp_handle_command, command_policy),
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
