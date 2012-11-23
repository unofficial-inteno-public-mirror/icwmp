/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
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

static struct uloop_process uproc;

LIST_HEAD(external_list_parameter);
char *external_MethodStatus = NULL;
char *external_MethodFault = NULL;
char *external_ObjectInstance = NULL;

void external_add_list_paramameter(char *param_name, char *param_data, char *param_type, char *fault_code)
{
	struct external_parameter *external_parameter;
	struct list_head *ilist; int i =0;
	external_parameter = calloc(1, sizeof(struct external_parameter));
	list_add_tail(&external_parameter->list,&external_list_parameter);
	if (param_name) external_parameter->name = strdup(param_name);
	if (param_data) external_parameter->data = strdup(param_data);
	if (param_type) external_parameter->type = strdup(param_type);
	if (fault_code) external_parameter->fault_code = strdup(fault_code);
}

void external_free_list_parameter()
{
	struct external_parameter *external_parameter;
	while (external_list_parameter.next!=&external_list_parameter) {
		external_parameter = list_entry(external_list_parameter.next, struct external_parameter, list);
		list_del(&external_parameter->list);
		FREE(external_parameter->name);
		FREE(external_parameter->data);
		FREE(external_parameter->type);
		FREE(external_parameter->fault_code);
		FREE(external_parameter);
	}
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

void external_setParamAttrRespFault (char *fault)
{
	FREE(external_MethodFault);
	external_MethodFault = fault ? strdup(fault) : NULL;
}

void external_fetch_setParamAttrRespFault (char **fault)
{
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
	if (fault) external_MethodStatus = strdup(fault);
}

void external_fetch_delObjectResp (char **status, char **fault)
{
	*status = external_MethodStatus;
	*fault = external_MethodFault;
	external_MethodStatus = NULL;
	external_MethodFault = NULL;
}

int external_get_action(char *action, char *name, char *arg /* arg is added for GetParameterNames NextLevel argument*/)
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
	struct external_parameter *external_parameter;
	external_get_action(action, name, NULL);
	if (external_list_parameter.next!=&external_list_parameter) {
		external_parameter = list_entry(external_list_parameter.next, struct external_parameter, list);
		if (external_parameter->data)
			*value = external_parameter->data;
		list_del(&external_parameter->list);
		FREE(external_parameter->name);
		FREE(external_parameter->data);
		FREE(external_parameter->type);
		FREE(external_parameter->fault_code);
		FREE(external_parameter);
	}
	external_free_list_parameter();
	return 0;
}

int external_get_action_write(char *action, char *name, char *arg)
{
	CWMP_LOG(INFO,"adding to get %s script '%s'", action, name);

	FILE *fp;

	if (access(fc_script_get_actions, R_OK | W_OK | X_OK) != -1) {
		fp = fopen(fc_script_get_actions, "a");
		if (!fp) return -1;
	} else {
		fp = fopen(fc_script_get_actions, "w");
		if (!fp) return -1;

		fprintf(fp, "#!/bin/sh\n");

		if (chmod(fc_script_get_actions,
			strtol("0700", 0, 8)) < 0) {
			return -1;
		}
	}

#ifdef DUMMY_MODE
	fprintf(fp, "/bin/sh `pwd`/%s get %s %s %s\n", fc_script, action, name, arg?arg:"");
#else
	fprintf(fp, "/bin/sh %s get %s %s %s\n", fc_script, action, name, arg?arg:"");
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
		argv[i++] = fc_script_get_actions;
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


	// TODO: add some kind of checks

	remove(fc_script_get_actions);

	return 0;
}


int external_set_action_write(char *action, char *name, char *value)
{

	CWMP_LOG(INFO,"adding to set %s script '%s'\n", action, name);

	FILE *fp;

	if (access(fc_script_set_actions, R_OK | W_OK | X_OK) != -1) {
		fp = fopen(fc_script_set_actions, "a");
		if (!fp) return -1;
	} else {
		fp = fopen(fc_script_set_actions, "w");
		if (!fp) return -1;

		fprintf(fp, "#!/bin/sh\n");

		if (chmod(fc_script_set_actions,
			strtol("0700", 0, 8)) < 0) {
			return -1;
		}
	}

#ifdef DUMMY_MODE
	fprintf(fp, "/bin/sh `pwd`/%s set %s %s %s\n", fc_script, action, name, value);
#else
	fprintf(fp, "/bin/sh %s set %s %s %s\n", fc_script, action, name, value);
#endif

	fclose(fp);

	return 0;
}

int external_set_action_execute(char *action)
{
	CWMP_LOG(INFO,"executing set script\n");

	FILE *fp;

	if (access(fc_script_set_actions, R_OK | W_OK | F_OK) == -1)
		return -1;

	fp = fopen(fc_script_set_actions, "a");
	if (!fp) return -1;

#ifdef DUMMY_MODE
	fprintf(fp, "/bin/sh `pwd`/%s apply %s\n", fc_script, action);
#else
	fprintf(fp, "/bin/sh %s apply %s\n", fc_script, action);
#endif


	if ((uproc.pid = fork()) == -1) {
		return -1;
	}

	if (uproc.pid == 0) {
		/* child */

		const char *argv[3];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script_set_actions;
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

	// TODO: add some kind of checks

	if (remove(fc_script_set_actions) != 0)
		return -1;

	return 0;
}

int external_object_action(char *action, char *name)
{
	CWMP_LOG(INFO,"executing get %s '%s'", action, name);

	if ((uproc.pid = fork()) == -1)
		goto error;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[6];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
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

		const char *argv[4];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
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

	// TODO: add some kind of checks

	return 0;
}

int external_download(char *url, char *size)
{
	CWMP_LOG(INFO,"executing download url '%s'", url);

	if ((uproc.pid = fork()) == -1)
		return -1;

	if (uproc.pid == 0) {
		/* child */

		const char *argv[8];
		int i = 0;
		argv[i++] = "/bin/sh";
		argv[i++] = fc_script;
		argv[i++] = "download";
		argv[i++] = "--url";
		argv[i++] = url;
		argv[i++] = "--size";
		argv[i++] = size;
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

	if (WIFEXITED(status) && !WEXITSTATUS(status))
		return 0;
	else
		return 1;

	return 0;
}

