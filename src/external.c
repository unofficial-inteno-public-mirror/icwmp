/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	Contributed by Inteno Broadband Technology AB
 *
 *	Copyright (C) 2013 Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	Copyright (C) 2013 Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <errno.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <libubox/uloop.h>

#include "external.h"
#include "cwmp.h"
#include "log.h"

static struct uloop_process uproc;

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

static void external_action_jshn_parse(int fp, int external_handler(char *msg))
{
    char buf[1], *value = NULL, *c = NULL;
    int i=0, len;

    while(read(fp, buf, sizeof(buf))>0) {
        if (buf[0]!='\n') {
			if (value)
				asprintf(&c,"%s%c",value,buf[0]);
			else
				asprintf(&c,"%c",buf[0]);

			free(value);
			value = c;
        } else {
        	if (!value) continue;
        	external_handler(value);
            FREE(value);
        }
    }
}

int external_get_action(char *action, char *name, char *arg, int external_handler(char *msg))
{
	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	CWMP_LOG(INFO,"executing get %s '%s'", action, name);

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--json";
		argv[i++] = "get";
		argv[i++] = action;
		argv[i++] = name;
		if(arg) argv[i++] = arg;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

	external_action_jshn_parse(pfds[0], external_handler);

    close(pfds[0]);
	return 0;

error:
	close(pfds[0]);
	return -1;
}

int external_get_action_data(char *action, char *name, char **value, int external_handler(char *msg))
{
	struct parameter_container *parameter_container;
	external_get_action(action, name, NULL, external_handler);
	if (external_list_parameter.next!=&external_list_parameter) {
		parameter_container = list_entry(external_list_parameter.next, struct parameter_container, list);
		if (parameter_container->data)
			*value = strdup(parameter_container->data);
		parameter_container_delete(parameter_container);
	}
	external_free_list_parameter();
	return 0;
}

int external_get_action_write(char *action, char *name, char *arg)
{
	CWMP_LOG(INFO,"adding to get %s script '%s'", action, name);

	FILE *fp;

	if (access(fc_script_actions, R_OK | W_OK | X_OK) != -1) {
		fp = fopen(fc_script_actions, "a");
		if (!fp) return -1;
	} else {
		fp = fopen(fc_script_actions, "w");
		if (!fp) return -1;

		fprintf(fp, "#!/bin/sh\n");

		if (chmod(fc_script_actions,
			strtol("0700", 0, 8)) < 0) {
			return -1;
		}
	}

#ifdef DUMMY_MODE
	fprintf(fp, "/bin/sh `pwd`/%s --json get %s %s %s\n", fc_script, action, name, arg?arg:"");
#else
	fprintf(fp, "/bin/sh %s --json get %s %s %s\n", fc_script, action, name, arg?arg:"");
#endif

	fclose(fp);

	return 0;
}

int external_get_action_execute(int external_handler(char *msg))
{
	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	if (access(fc_script_actions, F_OK) == -1)
		goto success;

	CWMP_LOG(INFO,"executing get script");

	if ((uproc.pid = fork()) == -1) {
		goto error;
	}

	if (uproc.pid == 0) {
		/* child */

		const char *argv[3];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script_actions;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

	external_action_jshn_parse(pfds[0], external_handler);
	remove(fc_script_actions);

success:
	close(pfds[0]);
	return 0;

error:
	close(pfds[0]);
	return -1;
}


int external_set_action_write(char *action, char *name, char *value, char *change)
{

	CWMP_LOG(INFO,"adding to set %s script '%s'", action, name);

	FILE *fp;

	if (access(fc_script_actions, R_OK | W_OK | X_OK) != -1) {
		fp = fopen(fc_script_actions, "a");
		if (!fp) return -1;
	} else {
		fp = fopen(fc_script_actions, "w");
		if (!fp) return -1;

		fprintf(fp, "#!/bin/sh\n");

		if (chmod(fc_script_actions,
			strtol("0700", 0, 8)) < 0) {
			return -1;
		}
	}
#ifdef DUMMY_MODE
		fprintf(fp, "/bin/sh `pwd`/%s --json set %s %s %s %s\n", fc_script, action, name, value, change ? change : "");
#else
		fprintf(fp, "/bin/sh %s --json set %s %s %s %s\n", fc_script, action, name, value, change ? change : "");
#endif


	fclose(fp);

	return 0;
}

int external_set_action_execute(char *action, int external_handler(char *msg))
{
	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	CWMP_LOG(INFO,"executing set script");

	FILE *fp;

	if (access(fc_script_actions, R_OK | W_OK | F_OK) == -1)
		goto error;

	fp = fopen(fc_script_actions, "a");
	if (!fp) goto error;

#ifdef DUMMY_MODE
	fprintf(fp, "/bin/sh `pwd`/%s --json apply %s\n", fc_script, action);
#else
	fprintf(fp, "/bin/sh %s --json apply %s\n", fc_script, action);
#endif

	fclose(fp);

	if ((uproc.pid = fork()) == -1) {
		goto error;
	}

	if (uproc.pid == 0) {
		/* child */

		const char *argv[3];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script_actions;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

 	external_action_jshn_parse(pfds[0], external_handler);

	if (remove(fc_script_actions) != 0)
		goto error;

    close(pfds[0]);
	return 0;

error:
	close(pfds[0]);
	return -1;
}

int external_object_action(char *action, char *name, int external_handler(char *msg))
{
	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	CWMP_LOG(INFO,"executing object %s '%s'", action, name);

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--json";
		argv[i++] = action;
		argv[i++] = "object";
		argv[i++] = name;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG, "waiting for child to exit");
	}

 	external_action_jshn_parse(pfds[0], external_handler);

	close(pfds[0]);
	return 0;

error:
	close(pfds[0]);
	return -1;
}

int external_simple(char *arg, int external_handler(char *msg))
{

	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	CWMP_LOG(INFO,"executing %s request", arg);

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[6];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--json";
		argv[i++] = arg;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

 	if (external_handler)
 		external_action_jshn_parse(pfds[0], external_handler);
    close(pfds[0]);
	return 0;

error:
	close(pfds[0]);
	return -1;
}

int external_download(char *url, char *size, char *type, char *user, char *pass, int external_handler(char *msg))
{
	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	CWMP_LOG(INFO,"executing download url '%s'", url);

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[20];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "download";
		argv[i++] = "--json";
		argv[i++] = "--url";
		argv[i++] = url;
		argv[i++] = "--size";
		argv[i++] = size;
		argv[i++] = "--type";
		argv[i++] = type;
		if(user)
		{
			argv[i++] = "--user";
			argv[i++] = user;
		}
		if(pass)
		{
			argv[i++] = "--pass";
			argv[i++] = pass;
		}
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD(INFO,"waiting for child to exit");
	}

 	external_action_jshn_parse(pfds[0], external_handler);
    close(pfds[0]);
	return 0;

error:
	close(pfds[0]);
	return -1;
}

int external_apply_download(char *type, int external_handler(char *msg))
{
	int pfds[2];
	if (pipe(pfds) < 0)
		return -1;

	CWMP_LOG(INFO,"applying downloaded file");

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--json";
		argv[i++] = "apply";
		argv[i++] = "download";
		argv[i++] = "--type";
		argv[i++] = type;
		argv[i++] = NULL;

		close(pfds[0]);
		dup2(pfds[1], 1);
		close(pfds[1]);

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	/* parent */
	close(pfds[1]);

	int status;
	while (wait(&status) != uproc.pid) {
		DD(INFO,"waiting for child to exit");
	}

 	external_action_jshn_parse(pfds[0], external_handler);
    close(pfds[0]);
	return 0;

error:
	close(pfds[0]);
	return -1;
}

