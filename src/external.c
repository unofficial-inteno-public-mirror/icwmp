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

void external_downloadResp (char *fault_code)
{
	FREE(external_MethodFault);
	external_MethodFault = fault_code ? strdup(fault_code) : NULL;
}

void external_fetch_downloadResp (char **fault)
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

int external_get_action(char *action, char *name, char *arg)
{

	CWMP_LOG(INFO,"executing get %s '%s'", action, name);

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--ubus";
		argv[i++] = "get";
		argv[i++] = action;
		argv[i++] = name;
		if(arg) argv[i++] = arg;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

	return 0;

error:
	return -1;

}

int external_get_action_data(char *action, char *name, char **value)
{
	struct parameter_container *parameter_container;
	external_get_action(action, name, NULL);
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
	fprintf(fp, "/bin/sh `pwd`/%s --ubus get %s %s %s\n", fc_script, action, name, arg?arg:"");
#else
	fprintf(fp, "/bin/sh %s --ubus get %s %s %s\n", fc_script, action, name, arg?arg:"");
#endif

	fclose(fp);

	return 0;
}

int external_get_action_execute()
{
	CWMP_LOG(INFO,"executing get script");

	if ((uproc.pid = fork()) == -1) {
		return -1;
	}

	if (uproc.pid == 0) {
		/* child */

		const char *argv[3];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script_actions;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

	remove(fc_script_actions);

	return 0;
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
		fprintf(fp, "/bin/sh `pwd`/%s --ubus set %s %s %s %s\n", fc_script, action, name, value, change ? change : "");
#else
		fprintf(fp, "/bin/sh %s --ubus set %s %s %s %s\n", fc_script, action, name, value, change ? change : "");
#endif


	fclose(fp);

	return 0;
}

int external_set_action_execute(char *action)
{
	CWMP_LOG(INFO,"executing set script");

	FILE *fp;

	if (access(fc_script_actions, R_OK | W_OK | F_OK) == -1)
		return -1;

	fp = fopen(fc_script_actions, "a");
	if (!fp) return -1;

#ifdef DUMMY_MODE
	fprintf(fp, "/bin/sh `pwd`/%s --ubus apply %s\n", fc_script, action);
#else
	fprintf(fp, "/bin/sh %s --ubus apply %s\n", fc_script, action);
#endif

	fclose(fp);

	if ((uproc.pid = fork()) == -1) {
		return -1;
	}

	if (uproc.pid == 0) {
		/* child */

		const char *argv[3];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script_actions;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

	if (remove(fc_script_actions) != 0)
		return -1;

	return 0;
}

int external_object_action(char *action, char *name)
{
	CWMP_LOG(INFO,"executing object %s '%s'", action, name);

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--ubus";
		argv[i++] = action;
		argv[i++] = "object";
		argv[i++] = name;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		goto error;

	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG, "waiting for child to exit");
	}

	return 0;

error:
	return -1;
}

int external_simple(char *arg)
{

	CWMP_LOG(INFO,"executing %s request", arg);

	if ((uproc.pid = fork()) == -1)
		return -1;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[6];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--ubus";
		argv[i++] = arg;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != uproc.pid) {
		DD(DEBUG,"waiting for child to exit");
	}

	return 0;
}

int external_download(char *url, char *size, char *type, char *user, char *pass, time_t scheduled_time)
{
	char			delay[256];
	CWMP_LOG(INFO,"executing download url '%s'", url);

	if ((uproc.pid = fork()) == -1)
		return -1;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[20];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "download";
		argv[i++] = "--ubus";
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
		sprintf(delay,"%ld",scheduled_time);
		argv[i++] = "--delay";
		argv[i++] = delay;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != uproc.pid) {
		DD(INFO,"waiting for child to exit");
	}

	return 0;
}

int external_apply_download(char *type)
{
	CWMP_LOG(INFO,"applying downloaded file");

	if ((uproc.pid = fork()) == -1)
		return -1;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "--ubus";
		argv[i++] = "apply";
		argv[i++] = "download";
		argv[i++] = "--type";
		argv[i++] = type;
		argv[i++] = NULL;

		execvp(argv[0], (char **) argv);
		exit(ESRCH);

	} else if (uproc.pid < 0)
		return -1;

	/* parent */
	int status;
	while (wait(&status) != uproc.pid) {
		DD(INFO,"waiting for child to exit");
	}

	return 0;
}

