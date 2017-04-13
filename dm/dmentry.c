/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author MOHAMED Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *	  Author Feten Besbes <feten.besbes@pivasoftware.com>
 *
 */

#include "dmcwmp.h"
#include "dmubus.h"
#include "dmuci.h"
#include "dmentry.h"

LIST_HEAD(head_package_change);
unsigned char dmcli_timetrack = 0;
unsigned char dmcli_evaluatetest = 0;

static void print_dm_help(void)
{
	printf("Usage:\n");
	printf(" get_value [param1] [param2] .... [param n]\n");
	printf(" set_value <parameter key> <param1> <val1> [param2] [val2] .... [param n] [val n]\n");
	printf(" get_name <param> <Next Level>\n");
	printf(" get_notification [param1] [param2] .... [param n]\n");
	printf(" set_notification <param1> <notif1> <change1>  [param2] [notif2] [change2] .... [param n] [notif n] [change n]\n");
	printf(" add_obj <param> <parameter key>\n");
	printf(" del_obj <param> <parameter key>\n");
	printf(" download <url> <file type> [file size] [username] [password]\n");
	printf(" reboot\n");
	printf(" factory_reset\n");
	printf(" inform\n");
	printf(" inform_device_id\n");
	printf(" apply_service\n");
	printf(" update_value_change\n");
	printf(" check_value_change\n");
	printf(" external_command <command> [arg 1] [arg 2] ... [arg n]\n");
	printf(" exit\n");
}

static int dm_ctx_init_custom(struct dmctx *ctx, unsigned int dm_type, unsigned int amd_version, unsigned int instance_mode, int custom)
{
	if (custom == CTX_INIT_ALL) {
		memset(&dmubus_ctx, 0, sizeof(struct dmubus_ctx));
		INIT_LIST_HEAD(&dmubus_ctx.obj_head);
		uci_ctx = uci_alloc_context();
		uci_varstate_ctx = uci_alloc_context();
	}
	INIT_LIST_HEAD(&ctx->list_parameter);
	INIT_LIST_HEAD(&ctx->set_list_tmp);
	INIT_LIST_HEAD(&ctx->list_fault_param);
	ctx->amd_version = amd_version;
	ctx->instance_mode = instance_mode;
	ctx->dm_type = dm_type;
	if (dm_type == DM_UPNP) {
		strcpy(DMROOT, DMROOT_UPNP);
		dm_delim = DMDELIM_UPNP;
	}
	return 0;
}

static int dm_ctx_clean_custom(struct dmctx *ctx, int custom)
{
	free_all_list_parameter(ctx);
	free_all_set_list_tmp(ctx);
	free_all_list_fault_param(ctx);
	DMFREE(ctx->addobj_instance);
	if (custom == CTX_INIT_ALL) {
		if (uci_ctx) uci_free_context(uci_ctx);
		uci_ctx = NULL;
		if (uci_varstate_ctx) uci_free_context(uci_varstate_ctx);
		uci_varstate_ctx = NULL;
		dmubus_ctx_free(&dmubus_ctx);
		dmcleanmem();
	}
	return 0;
}

int dm_ctx_init(struct dmctx *ctx, unsigned int dm_type, unsigned int amd_version, unsigned int instance_mode)
{
	dm_ctx_init_custom(ctx, dm_type, amd_version, instance_mode, CTX_INIT_ALL);
	return 0;
}

int dm_ctx_clean(struct dmctx *ctx)
{
	dm_ctx_clean_custom(ctx, CTX_INIT_ALL);
	return 0;
}

int dm_ctx_init_sub(struct dmctx *ctx, unsigned int dm_type, unsigned int amd_version, unsigned int instance_mode)
{
	dm_ctx_init_custom(ctx, dm_type, amd_version, instance_mode, CTX_INIT_SUB);
	return 0;
}

int dm_ctx_clean_sub(struct dmctx *ctx)
{
	dm_ctx_clean_custom(ctx, CTX_INIT_SUB);
	return 0;
}

void dmentry_instance_lookup_inparam(struct dmctx *ctx)
{
	char *pch, *spch, *in_param;
	in_param = dmstrdup(ctx->in_param);
	int i = 0;
	char pat[2] = {0};
	*pat = dm_delim;
	for (pch = strtok_r(in_param, pat, &spch); pch != NULL; pch = strtok_r(NULL, pat, &spch)) {
		if (pch[0]== '[') {
			ctx->alias_register |= (1 << i);
			i++;
		} else if (isdigit(pch[0])) {
			i++;
		}
	}
	dmfree(in_param);
	ctx->nbrof_instance = i;
}

int dm_entry_param_method(struct dmctx *ctx, int cmd, char *inparam, char *arg1, char *arg2)
{
	int fault = 0;
	bool setnotif = true;
	int err;
	
	if (!inparam) inparam = "";
	ctx->in_param = inparam;
	dmentry_instance_lookup_inparam(ctx);
	if (ctx->in_param[0] == '\0' || rootcmp(ctx->in_param, DMROOT) == 0) {
		ctx->tree = true;
	} else {
		ctx->tree = false;
	}
	ctx->stop = false;
	switch(cmd) {
		case CMD_GET_VALUE:
			if (ctx->in_param[0] == dm_delim && strlen(ctx->in_param) == 1)
				fault = FAULT_9005;
			else
				fault = dm_entry_get_value(ctx);
			break;
		case CMD_GET_NAME:
			if (ctx->in_param[0] == dm_delim && strlen(ctx->in_param) == 1)
				fault = FAULT_9005;
			else if (arg1 && string_to_bool(arg1, &ctx->nextlevel) == 0){
				fault = dm_entry_get_name(ctx);
			} else {
				fault = FAULT_9003;
			}
			break;
		case CMD_GET_NOTIFICATION:
			if (ctx->in_param[0] == dm_delim && strlen(ctx->in_param) == 1)
				fault = FAULT_9005;
			else
				fault = dm_entry_get_notification(ctx);
			break;
		case CMD_SET_VALUE:
			ctx->in_value = arg1 ? arg1 : "";
			ctx->setaction = VALUECHECK;
			fault = dm_entry_set_value(ctx);
			if (fault)
				add_list_fault_param(ctx, ctx->in_param, fault);
			break;
		case CMD_SET_NOTIFICATION:
			if (arg2)
				err = string_to_bool(arg2, &setnotif);
			if (!err && arg1 &&
				(strcmp(arg1, "0") == 0 ||
				strcmp(arg1, "1") == 0  ||
				strcmp(arg1, "2") == 0 ||
				strcmp(arg1, "3") == 0 ||
				strcmp(arg1, "4") == 0 ||
				strcmp(arg1, "5") == 0 ||
				strcmp(arg1, "6") == 0)) {
				ctx->in_notification = arg1;
				ctx->setaction = VALUECHECK;
				ctx->notification_change = setnotif;
				fault = dm_entry_set_notification(ctx);
			} else {
				fault = FAULT_9003;
			}
			break;
		case CMD_INFORM:
			dm_entry_inform(ctx);
			break;
		case CMD_ADD_OBJECT:
			fault = dm_entry_add_object(ctx);
			if (!fault) {
				dmuci_set_value("cwmp", "acs", "ParameterKey", arg1 ? arg1 : "");
				dmuci_change_packages(&head_package_change);
			}
			break;
		case CMD_DEL_OBJECT:
			fault = dm_entry_delete_object(ctx);
			if (!fault) {
				dmuci_set_value("cwmp", "acs", "ParameterKey", arg1 ? arg1 : "");
				dmuci_change_packages(&head_package_change);
			}
			break;
	}
	dmuci_commit();
	return fault;
}

int dm_entry_apply(struct dmctx *ctx, int cmd, char *arg1, char *arg2)
{
	int fault = 0;
	struct set_tmp *n, *p;
	
	switch(cmd) {
		case CMD_SET_VALUE:
			ctx->setaction = VALUESET;
			ctx->tree = false;
			list_for_each_entry_safe(n, p, &ctx->set_list_tmp, list) {
				ctx->in_param = n->name;
				ctx->in_value = n->value ? n->value : "";
				ctx->stop = false;
				fault = dm_entry_set_value(ctx);
				if (fault) break;
			}
			if (fault) {
				//Should not happen
				dmuci_revert();
				add_list_fault_param(ctx, ctx->in_param, fault);
			} else {
				dmuci_set_value("cwmp", "acs", "ParameterKey", arg1 ? arg1 : "");
				dmuci_change_packages(&head_package_change);
				dmuci_commit();
			}
			free_all_set_list_tmp(ctx);
			break;
		case CMD_SET_NOTIFICATION:
			ctx->setaction = VALUESET;
			ctx->tree = false;
			list_for_each_entry_safe(n, p, &ctx->set_list_tmp, list) {
				ctx->in_param = n->name;
				ctx->in_notification = n->value ? n->value : "0";
				ctx->stop = false;
				fault = dm_entry_set_notification(ctx);
				if (fault) break;
			}
			if (fault) {
				//Should not happen
				dmuci_revert();
			} else {
				dmuci_commit();
			}
			free_all_set_list_tmp(ctx);
			break;
	}
	return fault;
}

int dm_entry_load_enabled_notify(unsigned int dm_type, unsigned int amd_version, int instance_mode)
{
	struct dmctx dmctx = {0};

	dm_ctx_init(&dmctx, dm_type, amd_version, instance_mode);
	dmctx.in_param = "";
	dmctx.tree = true;

	free_all_list_enabled_lwnotify();
	free_all_list_enabled_notify();
	dm_entry_enabled_notify(&dmctx);

	dm_ctx_clean(&dmctx);
	return 0;
}

int adm_entry_get_linker_param(struct dmctx *ctx, char *param, char *linker, char **value)
{
	struct dmctx dmctx = {0};

	dm_ctx_init_sub(&dmctx, ctx->dm_type, ctx->amd_version, ctx->instance_mode);
	dmctx.in_param = param ? param : "";
	dmctx.linker = linker;

	if (dmctx.in_param[0] == '\0') {
		dmctx.tree = true;
	} else {
		dmctx.tree = false;
	}
	dm_entry_get_linker(&dmctx);
	*value = dmctx.linker_param;
	dm_ctx_clean_sub(&dmctx);
	return 0;
}

int adm_entry_get_linker_value(struct dmctx *ctx, char *param, char **value)
{
	struct dmctx dmctx = {0};
	*value = NULL;

	if (!param || param[0] == '\0') {
		return 0;
	}

	dm_ctx_init_sub(&dmctx, ctx->dm_type, ctx->amd_version, ctx->instance_mode);
	dmctx.in_param = param;
	dmctx.tree = false;

	dm_entry_get_linker_value(&dmctx);
	*value = dmctx.linker;

	dm_ctx_clean_sub(&dmctx);
	return 0;
}

int dm_entry_restart_services()
{
	struct package_change *pc;

	list_for_each_entry(pc, &head_package_change, list) {
		if(strcmp(pc->package, "cwmp") == 0)
			continue;
		dmubus_call_set("uci", "commit", UBUS_ARGS{{"config", pc->package}}, 1);
	}
	free_all_list_package_change(&head_package_change);

	return 0;
}

int cli_output_dm_result(struct dmctx *dmctx, int fault, int cmd, int out)
{
	if (!out) return 0;

	if (dmctx->list_fault_param.next != &dmctx->list_fault_param) {
		struct param_fault *p;
		list_for_each_entry(p, &dmctx->list_fault_param, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"fault\": \"%d\" }\n", p->name, p->fault);
		}
		goto end;
	}
	if (fault) {
		fprintf (stdout, "{ \"fault\": \"%d\" }\n", fault);
		goto end;
	}

	if (cmd == CMD_ADD_OBJECT) {
		if (dmctx->addobj_instance) {
			fprintf (stdout, "{ \"status\": \"1\", \"instance\": \"%s\" }\n", dmctx->addobj_instance);
			goto end;
		} else {
			fprintf (stdout, "{ \"fault\": \"%d\" }\n", FAULT_9002);
			goto end;
		}
	}

	if (cmd == CMD_DEL_OBJECT || cmd == CMD_SET_VALUE) {
		fprintf (stdout, "{ \"status\": \"1\" }\n");
		goto end;
	}

	if (cmd == CMD_SET_NOTIFICATION) {
		fprintf (stdout, "{ \"status\": \"0\" }\n");
		goto end;
	}

	struct dm_parameter *n;
	if (cmd == CMD_GET_NAME) {
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"writable\": \"%s\" }\n", n->name, n->data);
		}
	}
	else if (cmd == CMD_GET_NOTIFICATION) {
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"notification\": \"%s\" }\n", n->name, n->data);
		}
	}
	else if (cmd == CMD_GET_VALUE || cmd == CMD_INFORM) {
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"type\": \"%s\" }\n", n->name, n->data, n->type);
		}
	}
end:
	return 0;
}

static char *parse_arg_r(char *pch, char **last)
{
	if (pch == NULL) {
		pch = *last;
	}

	if (pch == NULL) {
		return NULL;
	}

	for(; *pch != '\0'; pch++)
	{
		if(*pch == ' ' || *pch == '\t')
			continue;
		if (*pch == '"')
		{
			char *s = strchr(++pch, '"');
			if(s) {
				*s = '\0';
				*last = s + 1;
				return pch;
			}
			else {
				*last = NULL;
				return NULL;
			}
		}
		else
		{
			char *s = strchr(pch, ' ');
			if(s) {
				*s = '\0';
				 *last = s + 1;
			}
			else {
				s = strchr(pch, '\t');
				if(s) {
					*s = '\0';
					 *last = s + 1;
				}
				else {
					*last = NULL;
				}
			}

			return pch;
		}
	}
	*last = NULL;
	return NULL;
}

static int dmentry_external_cmd(char **argv)
{
	int pid;

	if ((pid = fork()) == -1)
		return -1;

	if (pid == 0) {
		/* child */
		execvp(argv[0], argv);
		exit(ESRCH);

	} else if (pid < 0)
		return -1;

	int status;
	while (wait(&status) != pid);

	return 0;
}

void dm_execute_cli_shell(int argc, char** argv, unsigned int dmtype, unsigned int amd_version, unsigned int instance_mode)
{
	struct dmctx cli_dmctx = {0};
	int output = 1;
	char *param, *next_level, *parameter_key, *value, *cmd;
	int fault = 0, status = -1;
	bool set_fault = false;
	long ms; // Milliseconds
	time_t s;  // Seconds
	struct timespec tstart, tend;

	if (dmcli_timetrack)
		clock_gettime(CLOCK_REALTIME, &tstart);

	dm_ctx_init(&cli_dmctx, dmtype, amd_version, instance_mode);

	if (argc < 4) goto invalid_arguments;

	output = atoi(argv[2]);
	cmd = argv[3];

	/* GET NAME */
	if (strcmp(cmd, "get_name") == 0) {
		if (argc < 6) goto invalid_arguments;
		param = argv[4];
		next_level =argv[5];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NAME, param, next_level, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NAME, output);
	}
	/* GET VALUE */
	else if (strcmp(cmd, "get_value") == 0) {
		if (argc < 5) goto invalid_arguments;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_VALUE, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_VALUE, output);
	}
	/* GET NOTIFICATION */
	else if (strcmp(cmd, "get_notification") == 0) {
		if (argc < 5) goto invalid_arguments;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NOTIFICATION, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NOTIFICATION, output);
	}
	/* SET VALUE */
	else if (strcmp(cmd, "set_value") == 0) {
		if (argc < 7 || (argc % 2) == 0) goto invalid_arguments;
		int i;
		for (i = 5; i < argc; i+=2) {
			param = argv[i];
			value = argv[i+1];
			fault = dm_entry_param_method(&cli_dmctx, CMD_SET_VALUE, param, value, NULL);
			if (fault) set_fault = true;
		}
		parameter_key = argv[4];
		if (!set_fault) {
			fault = dm_entry_apply(&cli_dmctx, CMD_SET_VALUE, parameter_key, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_SET_VALUE, output);
	}
	/* SET NOTIFICATION */
	else if (strcmp(cmd, "set_notification") == 0) {
		if (argc < 6 || (argc % 2) != 0) goto invalid_arguments;
		int i;
		for (i = 4; i < argc; i+=2) {
			param = argv[i];
			value = argv[i+1];
			fault = dm_entry_param_method(&cli_dmctx, CMD_SET_NOTIFICATION, param, value, "1");
			if (fault) set_fault = true;
		}
		if(!set_fault) {
			fault = dm_entry_apply(&cli_dmctx, CMD_SET_NOTIFICATION, NULL, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_SET_NOTIFICATION, output);
	}
	/* ADD OBJECT */
	else if (strcmp(cmd, "add_object") == 0) {
		if (argc < 6) goto invalid_arguments;
		param =argv[5];
		parameter_key =argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_ADD_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_ADD_OBJECT, output);
	}
	/* DEL OBJECT */
	else if (strcmp(cmd, "delete_object") == 0) {
		if (argc < 6) goto invalid_arguments;
		param =argv[5];
		parameter_key =argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_DEL_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_DEL_OBJECT, output);
	}
	/* INFORM */
	else if (strcmp(cmd, "inform") == 0) {
		fault = dm_entry_param_method(&cli_dmctx, CMD_INFORM, "", NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_INFORM, output);
	}
	else {
		goto invalid_arguments;
	}
	dm_ctx_clean(&cli_dmctx);

	if (dmcli_timetrack) {
		clock_gettime(CLOCK_REALTIME, &tend);
		s = tend.tv_sec - tstart.tv_sec;
		ms = (tend.tv_nsec - tstart.tv_nsec) / 1.0e6; // Convert nanoseconds to milliseconds
		if (ms < 0) {
			ms = 1000 + ms;
			s--;
		}
		fprintf(stdout, "-----------------------------\n");
		fprintf(stdout, "End: %ld s : %ld ms\n", (long)s, ms);
		fprintf(stdout, "-----------------------------\n");
		fflush(stdout);
	}
	return;

invalid_arguments:
	dm_ctx_clean(&cli_dmctx);
	fprintf(stdout, "Invalid arguments!\n");;
}

int dmentry_cli(int argc, char *argv[], unsigned int dmtype, unsigned int amd_version, unsigned int instance_mode)
{
	struct dmctx cli_dmctx = {0};
	int fault = 0, set_fault = 0;
	int i;
	char *param;
	char *value;
	char *parameter_key;
	char *notifset;

	if (argc < 3) {
		fprintf(stderr, "Wrong arguments!\n");
		return -1;
	}

	dm_ctx_init(&cli_dmctx, dmtype, amd_version, instance_mode);
	if (strcmp(argv[2], "get_value") == 0) {
		char *param = "";
		if (argc >= 4)
			param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_VALUE, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_VALUE, 1);
	}
	else if (strcmp(argv[2], "get_name") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NAME, argv[3], argv[4], NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NAME, 1);
	}
	else if (strcmp(argv[2], "get_notification") == 0) {
		char *param = "";
		if (argc >= 4)
			param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NOTIFICATION, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NOTIFICATION, 1);
	}
	else if (strcmp(argv[2], "set_value") == 0) {
		if (argc < 6 || (argc % 2) != 0)
			goto invalid_arguments;

		for (i = 4; i < argc; i+=2) {
			param = argv[i];
			value = argv[i+1];
			fault = dm_entry_param_method(&cli_dmctx, CMD_SET_VALUE, param, value, NULL);
			if (fault) set_fault = true;
		}
		parameter_key = argv[3];
		if (!set_fault) {
			fault = dm_entry_apply(&cli_dmctx, CMD_SET_VALUE, parameter_key, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_SET_VALUE, 1);
	}
	else if (strcmp(argv[2], "set_notification") == 0) {
		if (argc < 6 || (argc % 3) != 0)
			goto invalid_arguments;
		for (i=3; i<argc; i+=3) {
			param = argv[i];
			value = argv[i+1];
			notifset = argv[i+2];
			fault = dm_entry_param_method(&cli_dmctx, CMD_SET_NOTIFICATION, param, value, notifset);
			if (fault) set_fault = true;
		}
		if(!set_fault) {
			fault = dm_entry_apply(&cli_dmctx, CMD_SET_NOTIFICATION, NULL, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_SET_NOTIFICATION, 1);
	}
	else if (strcmp(argv[2], "inform") == 0 || strcmp(argv[2], "inform_parameter") == 0) {
		fault = dm_entry_param_method(&cli_dmctx, CMD_INFORM, "", NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_INFORM, 1);
	}
	else if (strcmp(argv[2], "add_obj") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		param = argv[3];
		parameter_key = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_ADD_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_ADD_OBJECT, 1);
	}
	else if (strcmp(argv[2], "del_obj") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		param =argv[3];
		parameter_key =argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_DEL_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_DEL_OBJECT, 1);
	}
	else if (strcmp(argv[2], "external_command") == 0) {
		if (argc < 4)
			goto invalid_arguments;
		argv[argc] = NULL;
		dmentry_external_cmd(&argv[3]);
	}
	else {
		goto invalid_arguments;
	}
	dm_ctx_clean(&cli_dmctx);
	return 0;

invalid_arguments:
	dm_ctx_clean(&cli_dmctx);
	fprintf(stdout, "Invalid arguments!\n");
	return -1;
}

void dm_execute_cli_command(char *file, unsigned int dmtype, unsigned int amd_version, unsigned int instance_mode)
{
	FILE *fp;
	char *argv[64];
	char buf[2048], dbuf[2048];
	char *pch, *pchr;
	int argc, len, i=0;
	long ms; // Milliseconds
	time_t s;  // Seconds
	struct timespec tstart, tend;

	if (file) {
		fp = fopen(file, "r");
		if (fp == NULL) {
			fprintf(stderr, "ERROR: Wrong file!\n");
			fflush(stderr);
			return;
		}
	}
	else {
		fp = stdin;
	}

	printf("%s", DM_PROMPT" "); fflush(stdout);

	while (fgets (buf , 2048 , fp) != NULL) {
		if (dmcli_evaluatetest)
			argc = 1;
		else
			argc = 2;

		len = strlen(buf);
		if (len>0 && buf[len-1] == '\n') {
			buf[len-1] = '\0';
		}
		if (strcasecmp(buf, "exit") == 0) {
			if (file) {
				fprintf(stdout, "%s\n", buf);
				fflush(stdout);
			}
			return;
		}
		if (strcasecmp(buf, "help") == 0) {
			if (file) {
				fprintf(stdout, "%s\n", buf);
				fflush(stdout);
			}
			print_dm_help();
			printf(DM_PROMPT" "); fflush(stdout);
			continue;
		}

		i++;

		strcpy(dbuf, buf);
		for (pch = parse_arg_r(buf, &pchr); pch != NULL; pch = parse_arg_r(NULL, &pchr)) {
			if(argc < 3 && (pch[0] == '#' || pch[0] == '\0'))
				break;
			if (*pch == '"')
				pch++;
			len = strlen(pch);
			if (len>0 && pch[len-1] == '"')
				pch[len-1] = '\0';
			argv[argc++] = pch;
		}
		if (file) {
			if (!pch || pch[0] != '#') {
				fprintf(stdout, "%s\n", dbuf);
				fflush(stdout);
			}
			else {
				fprintf(stdout, "\n");
				fflush(stdout);
			}
		}
		if (argc>2) {
			char testref[32] = "";
			if (dmcli_evaluatetest)
				sprintf(testref, "Ref: %s - ", argv[1]);
			if (dmcli_timetrack || dmcli_evaluatetest) {
				fprintf(stdout, "-----------------------------\n");
				fprintf(stdout, "[%s%04d] %s\n", testref, i, dbuf);
				fprintf(stdout, "-----------------------------\n");
				fflush(stdout);
				clock_gettime(CLOCK_REALTIME, &tstart);
			}
			if (dmentry_cli(argc, argv, dmtype, amd_version, instance_mode) == 0) {
				if (dmcli_timetrack || dmcli_evaluatetest) {
					clock_gettime(CLOCK_REALTIME, &tend);
					s = tend.tv_sec - tstart.tv_sec;
					ms = (tend.tv_nsec - tstart.tv_nsec) / 1.0e6; // Convert nanoseconds to milliseconds
					if (ms < 0) {
						ms = 1000 + ms;
						s--;
					}
					fprintf(stdout, "-----------------------------\n");
					fprintf(stdout, "%sEnd: %ld s : %ld ms\n", testref, (long)s, ms);
					fprintf(stdout, "-----------------------------\n");
					fflush(stdout);
				}
			}
			else {
				fprintf(stdout, "Type help for help\n");
				fflush(stdout);
			}
		}
		printf(DM_PROMPT" "); fflush(stdout);
	}
	if (file) {
		fclose(fp);
		fprintf(stdout, "\n");
		fflush(stdout);
	}
}

int cli_output_wepkey64(char strk64[4][11])
{
	fprintf(stdout, "%s\n%s\n%s\n%s\n", strk64[0], strk64[1], strk64[2], strk64[3]);
	return 0;
}

int cli_output_wepkey128(char strk128[27])
{
	fprintf(stdout, "%s\n", strk128);
	return 0;
}

void wepkey_cli(int argc, char** argv)
{
	if (argc < 4) goto invalid_arguments;

	char *strength = argv[2];
	char *passphrase =  argv[3];

	if (!strength || !passphrase || passphrase[0] == '\0')
		goto invalid_arguments;

	if (strcmp(strength, "64") == 0) {
		char strk64[4][11];
		wepkey64(passphrase, strk64);
		cli_output_wepkey64(strk64);
	}
	else if (strcmp(strength, "128") == 0) {
		char strk128[27];
		wepkey128(passphrase, strk128);
		cli_output_wepkey128(strk128);
	}
	else {
		goto invalid_arguments;
	}
	return;

invalid_arguments:
	fprintf(stdout, "Invalid arguments!\n");;
}

