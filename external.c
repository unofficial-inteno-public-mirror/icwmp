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

#include <json/json.h>

#include "external.h"
#include "cwmp.h"
#include "log.h"

static int pid;
static json_object *json_obj_in;
static int pfds_in[2], pfds_out[2];
static FILE *fpipe;

LIST_HEAD(external_list_parameter);
LIST_HEAD(external_list_value_change);
pthread_mutex_t external_mutex_value_change = PTHREAD_MUTEX_INITIALIZER;
char *external_MethodStatus = NULL;
char *external_MethodFault = NULL;
char *external_ObjectInstance = NULL;

inline void external_add_list_parameter(char *param_name, char *param_data, char *param_type, char *fault_code)
{
	parameter_container_add(&external_list_parameter, param_name, param_data, param_type, fault_code);
}

inline void external_add_list_value_change(char *param_name, char *param_data, char *param_type)
{
	pthread_mutex_lock(&(external_mutex_value_change));
	parameter_container_add(&external_list_value_change, param_name, param_data, param_type, NULL);
	pthread_mutex_unlock(&(external_mutex_value_change));
}

inline void external_free_list_parameter()
{
	parameter_container_delete_all(&external_list_parameter);
}

inline void external_free_list_value_change()
{
	parameter_container_delete_all(&external_list_value_change);
}

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

void external_setParamValRespStatus (char *status)
{
	FREE(external_MethodStatus);
	external_MethodStatus = status ? strdup(status) : NULL;
}

void external_fetch_setParamValRespStatus (char **status)
{
	*status = external_MethodStatus;
	external_MethodStatus = NULL;
}

void external_setParamAttrResp (char *status, char *fault)
{
	FREE(external_MethodStatus);
	external_MethodStatus = status ? strdup(status) : NULL;
	FREE(external_MethodFault);
	external_MethodFault = fault ? strdup(fault) : NULL;
}

void external_fetch_setParamAttrResp (char **status, char **fault)
{
	*status = external_MethodStatus;
	external_MethodStatus = NULL;
	*fault = external_MethodFault;
	external_MethodFault = NULL;
}

void external_addObjectResp (char *instance, char *status, char *fault)
{
	FREE(external_MethodStatus);
	FREE(external_ObjectInstance);
	FREE(external_MethodFault);
	external_MethodStatus = status ? strdup(status) : NULL;
	external_ObjectInstance = instance ? strdup(instance) : NULL;
	external_MethodFault = fault ? strdup(fault) : NULL;
}

void external_fetch_addObjectResp (char **instance, char **status, char **fault)
{
	*instance = external_ObjectInstance;
	*status = external_MethodStatus;
	*fault = external_MethodFault;
	external_ObjectInstance = NULL;
	external_MethodStatus = NULL;
	external_MethodFault = NULL;
}

void external_delObjectResp (char *status, char *fault)
{
	FREE(external_MethodStatus);
	FREE(external_MethodFault);
	if (status) external_MethodStatus = strdup(status);
	if (fault) external_MethodFault = strdup(fault);
}

void external_fetch_delObjectResp (char **status, char **fault)
{
	*status = external_MethodStatus;
	*fault = external_MethodFault;
	external_MethodStatus = NULL;
	external_MethodFault = NULL;
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
        	if (strcmp(value, "EOF")==0) break;
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

	if ((pid = vfork()) == -1)
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
	 	argv[i++] = "--json";
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
    	DD(ERROR, "freecwmp script intialization: signal ignoring error");

	external_read_pipe_input(NULL);

	DD(INFO, "freecwmp script is listening");
	return;

error:
	CWMP_LOG(ERROR,"freecwmp script intialization failed");
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


int external_get_action(char *action, char *name, char *next_level)
{
	DD(INFO,"executing get %s '%s'", action, name);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "get");
	json_obj_out_add(json_obj_out, "action", action);
	json_obj_out_add(json_obj_out, "parameter", name);
	if (next_level) json_obj_out_add(json_obj_out, "next_level", next_level);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_set_action(char *action, char *name, char *value, char *change)
{
	DD(INFO,"executing set %s '%s'", action, name);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "set");
	json_obj_out_add(json_obj_out, "action", action);
	json_obj_out_add(json_obj_out, "parameter", name);
	json_obj_out_add(json_obj_out, "value", value);
	if (change) json_obj_out_add(json_obj_out, "change", change);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_object_action(char *command, char *name)
{
	DD(INFO,"executing %s object '%s'", command, name);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", command);
	json_obj_out_add(json_obj_out, "action", "object");
	json_obj_out_add(json_obj_out, "parameter", name);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_simple(char *command, char *arg)
{
	DD(INFO,"executing %s request", command);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", command);
	if (arg) json_obj_out_add(json_obj_out, "arg", arg);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_download(char *url, char *size, char *type, char *user, char *pass)
{
	DD(INFO,"executing download url '%s'", url);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "download");
	json_obj_out_add(json_obj_out, "url", url);
	json_obj_out_add(json_obj_out, "size", size);
	json_obj_out_add(json_obj_out, "type", type);
	if(user) json_obj_out_add(json_obj_out, "user", user);
	if(pass) json_obj_out_add(json_obj_out, "pass", pass);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

int external_apply(char *action, char *type)
{
	DD(INFO,"executing apply %s", action);

	json_object *json_obj_out;

	/* send data to the script */
	json_obj_out = json_object_new_object();

	json_obj_out_add(json_obj_out, "command", "apply");
	json_obj_out_add(json_obj_out, "action", action);
	if (type) json_obj_out_add(json_obj_out, "type", type);

	external_write_pipe_output(json_object_to_json_string(json_obj_out));

	json_object_put(json_obj_out);

	return 0;
}

