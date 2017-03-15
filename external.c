/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <errno.h>
#include <malloc.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libubox/uloop.h>

#include <json-c/json.h>

#include "external.h"
#include "cwmp.h"
#include "log.h"

#include <stdarg.h>
static int pid;
static json_object *json_obj_in;
static int pfds_in[2], pfds_out[2];
static FILE *fpipe;
char *external_MethodFault = NULL;
char *external_MethodName = NULL;
char *external_MethodVersion = NULL;

void external_downloadFaultResp (char *fault_code)
{
	FREE(external_MethodFault);
	external_MethodFault = fault_code ? strdup(fault_code) : NULL;
}

void external_fetch_downloadFaultResp (char **fault)
{
	*fault = external_MethodFault;
	external_MethodFault = NULL;
}

void external_uploadFaultResp (char *fault_code)
{
	FREE(external_MethodFault);
	external_MethodFault = fault_code ? strdup(fault_code) : NULL;
}

void external_fetch_uploadFaultResp (char **fault)
{
	*fault = external_MethodFault;
	external_MethodFault = NULL;
}

void external_uninstallFaultResp (char *fault_code)
{
	FREE(external_MethodFault);
	external_MethodFault = fault_code ? strdup(fault_code) : NULL;
}

void external_fetch_uninstallFaultResp (char **fault)
{
	*fault = external_MethodFault;
	external_MethodFault = NULL;
}

void external_du_change_stateFaultResp (char *fault_code, char *version, char *name)
{
	FREE(external_MethodFault);
	external_MethodFault = fault_code ? strdup(fault_code) : NULL;
	FREE(external_MethodVersion);
	external_MethodVersion = version ? strdup(version) : NULL;
	FREE(external_MethodName);
	external_MethodName = name ? strdup(name) : NULL;	
}

void external_fetch_du_change_stateFaultResp (char **fault, char **version, char **name)
{
	*fault = external_MethodFault;
	external_MethodFault = NULL;
	*version = external_MethodVersion;
	external_MethodVersion = NULL;
	*name = external_MethodName;
	external_MethodName = NULL;
}
static void external_read_pipe_input(int (*external_handler)(char *msg))
{
    char buf[1], *value = NULL, *c = NULL;
    int i=0, len;
	struct pollfd fd = {
		.fd	= pfds_in[0],
		.events	= POLLIN
	};
    while(1) {
    	poll(&fd, 1, 500000);
    	if (!(fd.revents & POLLIN)) break;
    	if (read(pfds_in[0], buf, sizeof(buf))<=0) break;
        if (buf[0]!='\n') {
			if (value)
				asprintf(&c,"%s%c",value,buf[0]);
			else
				asprintf(&c,"%c",buf[0]);

			FREE(value);
			value = c;
        } else {
        	if (!value) continue;
        	if (strcmp(value, "icwmp>")==0) {
        	    FREE(value);
        	    break;
        	}
        	if(external_handler) external_handler(value);
            FREE(value);
        }
    }
}

static void external_write_pipe_output(const char *msg)
{
    char *value = NULL;
    int i=0, len;

    asprintf(&value, "%s\n", msg);
    if (write(pfds_out[1], value, strlen(value)) == -1) {
    	CWMP_LOG(ERROR,"Error occured when trying to write to the pipe");
	}
    free(value);
}

static void json_obj_out_add(json_object *json_obj_out, char *name, char *val)
{
	json_object *json_obj_tmp;

	json_obj_tmp = json_object_new_string(val);
	json_object_object_add(json_obj_out, name, json_obj_tmp);
	}

void external_init()
{
	if (pipe(pfds_in) < 0)
			return;

	if (pipe(pfds_out) < 0)
		return;

	if ((pid = fork()) == -1)
		goto error;

	if (pid == 0) {
		/* child */

		close(pfds_out[1]);
		close(pfds_in[0]);

		dup2(pfds_out[0], STDIN_FILENO);
		dup2(pfds_in[1], STDOUT_FILENO);

		const char *argv[5];
		int i = 0;
		argv[i++] = "/bin/sh";
	 	argv[i++] = fc_script;
	 	argv[i++] = "json_continuous_input";
		argv[i++] = NULL;
		execvp(argv[0], (char **) argv);

		close(pfds_out[0]);
		close(pfds_in[1]);

		exit(ESRCH);
	}

	close(pfds_in[1]);
    close(pfds_out[0]);

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    	DD(ERROR, "icwmp script intialization: signal ignoring error");

	external_read_pipe_input(NULL);

	DD(INFO, "icwmp script is listening");
	return;

error:
	CWMP_LOG(ERROR,"icwmp script intialization failed");
	exit(EXIT_FAILURE);
}

void external_exit()
{
    int status;

	json_object *json_obj_out;

	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "exit");

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	while (wait(&status) != pid) {
		DD(DEBUG, "waiting for child to exit");
	}

	close(pfds_in[0]);
    close(pfds_out[1]);
}

int external_handle_action(int (*external_handler)(char *msg))
{
	json_object *json_obj_out;

	json_obj_out = json_object_new_object();
	json_obj_out_add(json_obj_out, "command", "end");
	external_write_pipe_output(json_object_to_json_string(json_obj_out));
	json_object_put(json_obj_out);
	external_read_pipe_input(external_handler);
	return 0;
}

int external_simple(char *command, char *arg, int c)
{
	DD(INFO,"executing %s request", command);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", command);
	if (arg) json_obj_out_add(json_obj_out, "arg", arg);

	if (c)  json_obj_out_add(json_obj_out, "ipv6", "1");
	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_download(char *url, char *size, char *type, char *user, char *pass, time_t c)
{
	DD(INFO,"executing download url '%s'", url);

	json_object *json_obj_out;
	char *id = NULL;

	if (c) asprintf(&id, "%ld", c);
	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "download");
	json_obj_out_add(json_obj_out, "url", url);
	json_obj_out_add(json_obj_out, "size", size);
	json_obj_out_add(json_obj_out, "type", type);
	if(user) json_obj_out_add(json_obj_out, "user", user);
	if(pass) json_obj_out_add(json_obj_out, "pass", pass);
	if(id) json_obj_out_add(json_obj_out, "ids", id);
	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_change_du_state_download(char *url, char *user, char *pass)
{
	DD(INFO,"executing DU download url '%s'", url);
	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "du_download");
	json_obj_out_add(json_obj_out, "url", url);
	if(user) json_obj_out_add(json_obj_out, "user", user);
	if(pass) json_obj_out_add(json_obj_out, "pass", pass);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_upload(char *url, char *type, char *user, char *pass, char *name)
{
	DD(INFO,"executing download url '%s'", url);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "upload");
	json_obj_out_add(json_obj_out, "url", url);
	json_obj_out_add(json_obj_out, "type", type);
	json_obj_out_add(json_obj_out, "name", name);
	if(user) json_obj_out_add(json_obj_out, "user", user);
	if(pass) json_obj_out_add(json_obj_out, "pass", pass);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_change_du_state_uninstall(char *package_name)
{
	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "du_uninstall");
	json_obj_out_add(json_obj_out, "package_name", package_name);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_apply(char *action, char *arg, time_t c)
{
	DD(INFO,"executing apply %s", action);

	json_object *json_obj_out;
	char *id = NULL;

	if (c) asprintf(&id, "%ld", c);

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "apply");
	json_obj_out_add(json_obj_out, "action", action);
	if (arg) json_obj_out_add(json_obj_out, "arg", arg);

	if(id) json_obj_out_add(json_obj_out, "ids", id);
	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

