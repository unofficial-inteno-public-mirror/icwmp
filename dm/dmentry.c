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
#include "root.h"

LIST_HEAD(head_package_change);
unsigned char dmcli_timetrack = 0;
unsigned char dmcli_evaluatetest = 0;

static void print_dm_help(void)
{
	printf("Usage:\n");
	printf(" get_value [param]\n");
	printf(" set_value <parameter key> <param1> <val1> [<param2> <val2>] .... [<param N> <val N>]\n");
	printf(" get_name <param> <Next Level>\n");
	printf(" get_notification [param]\n");
	printf(" set_notification <param1> <notif1> <change1> [<param2> <notif2> <change2>] ....[<param N> <notif N> <change N>]\n");
	printf(" add_obj <param> <parameter key>\n");
	printf(" del_obj <param> <parameter key>\n");
	printf(" inform\n");
	printf(" upnp_get_values [param]\n");
	printf(" upnp_get_selected_values [param]\n");
	printf(" upnp_get_instances <param> <depth>\n");
	printf(" upnp_get_supported_parameters <param> <depth>\n");
	printf(" upnp_set_values <param1> <val1> [<param2> <val2>] .... [<param N> <val N>]\n");
	printf(" upnp_get_attributes <param>\n");
	printf(" upnp_set_attributes <param 1> <EventOnChange 1> <AlarmOnChange 1> .... [<param N> <EventOnChange N> <AlarmOnChange N>\n");
	printf(" upnp_add_instance <param> [<sub param 1> <val1>] [<sub param 2> <val2>] .... [<sub param N> <val N>]\n");
	printf(" upnp_delete_instance <param>\n");
	printf(" upnp_get_acldata <param>\n");
	printf(" upnp_init_state_variables\n");
	printf(" upnp_get_supported_parameters_update\n");
	printf(" upnp_get_supported_datamodel_update\n");
	printf(" upnp_get_configuration_update\n");
	printf(" upnp_get_current_configuration_version\n");
	printf(" upnp_get_attribute_values_update\n");
	printf(" upnp_load_enabled_parametrs_track\n");
	printf(" upnp_get_enabled_parametrs_alarm\n");
	printf(" upnp_get_enabled_parametrs_event\n");
	printf(" upnp_get_enabled_parametrs_version\n");
	printf(" upnp_check_changed_parametrs_alarm\n");
	printf(" upnp_check_changed_parametrs_event\n");
	printf(" upnp_check_changed_parametrs_version\n");
	printf(" external_command <command> [arg 1] [arg 2] ... [arg N]\n");
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
		ctx->dm_entryobj = tEntryObjUPNP;
		ctx->user_mask = upnp_in_user_mask;
	}
	else {
		strcpy(DMROOT, DMROOT_CWMP);
		dm_delim = DMDELIM_CWMP;
		ctx->dm_entryobj = tEntryObj;
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
	bool alarm = false, event = false;
	int err, err2;
	
	if (!inparam) inparam = "";
	ctx->in_param = inparam;
	dmentry_instance_lookup_inparam(ctx);
	ctx->stop = false;
	switch(cmd) {
		case CMD_GET_VALUE:
			if (ctx->dm_type == DM_CWMP && ctx->in_param[0] == dm_delim && strlen(ctx->in_param) == 1)
				fault = FAULT_9005;
			else
				fault = dm_entry_get_value(ctx);
			break;
		case CMD_GET_NAME:
			if (ctx->dm_type == DM_CWMP && ctx->in_param[0] == dm_delim && strlen(ctx->in_param) == 1)
				fault = FAULT_9005;
			else if (arg1 && string_to_bool(arg1, &ctx->nextlevel) == 0){
				fault = dm_entry_get_name(ctx);
			} else {
				fault = FAULT_9003;
			}
			break;
		case CMD_GET_NOTIFICATION:
			if (ctx->dm_type == DM_CWMP && ctx->in_param[0] == dm_delim && strlen(ctx->in_param) == 1)
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
		case CMD_UPNP_GET_SUPPORTED_PARAMETERS:
			ctx->depth = atoi(arg1);
			fault = dm_entry_upnp_get_supported_parameters(ctx);
			break;
		case CMD_UPNP_GET_VALUES:
			fault = dm_entry_upnp_get_values(ctx);
			break;
		case CMD_UPNP_GET_SELECTED_VALUES:
			fault = dm_entry_upnp_get_selected_values(ctx);
			break;
		case CMD_UPNP_GET_INSTANCES:
			ctx->depth = atoi(arg1);
			fault = dm_entry_upnp_get_instances(ctx);
			break;
		case CMD_UPNP_SET_VALUES:
			ctx->in_value = arg1 ? arg1 : "";
			ctx->setaction = VALUECHECK;
			fault = dm_entry_upnp_set_values(ctx);
			break;
		case CMD_UPNP_SET_ATTRIBUTES:
			if (arg1)
				err = string_to_bool(arg1, &event);
			if (arg2)
				err2 = string_to_bool(arg2, &alarm);
			if (!err && !err2) {
				ctx->dmparam_flags |= (event) ? DM_PARAM_EVENT_ON_CHANGE : 0;
				ctx->dmparam_flags |= (alarm) ? DM_PARAM_ALARAM_ON_CHANGE : 0;
				ctx->setaction = VALUECHECK;
				fault = dm_entry_upnp_set_attributes(ctx);
			} else {
				fault = FAULT_9003;
			}
			break;
		case CMD_UPNP_GET_ATTRIBUTES:
			fault = dm_entry_upnp_get_attributes(ctx);
			break;
		case CMD_UPNP_DEL_INSTANCE:
			fault = dm_entry_upnp_delete_instance(ctx);
			if (!fault) {
				dmuci_change_packages(&head_package_change);
			}
			break;
		case CMD_UPNP_ADD_INSTANCE:
			fault = dm_entry_upnp_add_instance(ctx);
			if (!fault) {
				dmuci_change_packages(&head_package_change);
			}
			break;
		case CMD_UPNP_GET_ACLDATA:
			fault = dm_entry_upnp_get_acl_data(ctx);
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
		case CMD_UPNP_SET_VALUES:
			ctx->setaction = VALUESET;
			list_for_each_entry_safe(n, p, &ctx->set_list_tmp, list) {
				ctx->in_param = n->name;
				ctx->in_value = n->value ? n->value : "";
				ctx->stop = false;
				fault = dm_entry_upnp_set_values(ctx);
				if (fault) break;
			}
			if (fault) {
				//Should not happen
				dmuci_revert();
			} else {
				dmuci_change_packages(&head_package_change);
				dmuci_commit();
			}
			break;
		case CMD_UPNP_SET_ATTRIBUTES:
			ctx->setaction = VALUESET;
			list_for_each_entry_safe(n, p, &ctx->set_list_tmp, list) {
				ctx->in_param = n->name;
				ctx->dmparam_flags = n->flags;
				ctx->stop = false;
				fault = dm_entry_upnp_set_attributes(ctx);
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

	dm_entry_get_linker_value(&dmctx);
	*value = dmctx.linker;

	dm_ctx_clean_sub(&dmctx);
	return 0;
}

/****************************************
 * upnp load tracked on change parameters
 ****************************************/

int dm_entry_upnp_load_tracked_parameters(struct dmctx *dmctx)
{

	dmctx->in_param = "";

	free_all_list_upnp_param_track(&list_upnp_enabled_onevent);
	free_all_list_upnp_param_track(&list_upnp_enabled_onalarm);
	free_all_list_upnp_param_track(&list_upnp_enabled_version);
	dm_entry_upnp_tracked_parameters(dmctx);

	return 0;
}

/*********************************************
 * upnp check on change params (event & alarm)
 *********************************************/

void dm_upnp_update_enabled_track_value(struct dm_upnp_enabled_track *p, char *new_value)
{
	free(p->value); // Should be free and not dmfree
	p->value = strdup(new_value);
}

void dm_upnp_update_enabled_track_key(struct dm_upnp_enabled_track *p, char *key)
{
	free(p->key); // Should be free and not dmfree
	p->key = strdup(key);
}

int dm_entry_upnp_check_onchange_param(struct dmctx *pctx, struct list_head *enabled_head, struct list_head *changed_head)
{
	struct dmctx dmctx = {0};
	struct dm_upnp_enabled_track *p;
	struct dm_parameter *dm_parameter;
	int fault;
	int ischange = 0;
	char *all_instances;

	list_for_each_entry(p, enabled_head, list) {
		dm_ctx_init_sub(&dmctx, pctx->dm_type, pctx->amd_version, pctx->instance_mode);
		dmctx.user_mask = DM_SUPERADMIN_MASK;
		if (p->isobj) {
			all_instances = dm_entry_get_all_instance_numbers(&dmctx, p->name);
			if (all_instances && strcmp(all_instances, p->value) != 0) {
				dm_upnp_update_enabled_track_value(p, all_instances);
				add_list_upnp_param_track(&dmctx, changed_head, p->name, "1", all_instances, 1);
				ischange = 1;
			}
		}
		else {
			fault = dm_entry_param_method(&dmctx, CMD_UPNP_GET_VALUES, p->name, NULL, NULL);
			if (!fault && dmctx.list_parameter.next != &dmctx.list_parameter) {
				dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);
				if (strcmp(dm_parameter->data, p->value) != 0) {
					dm_upnp_update_enabled_track_value(p, dm_parameter->data);
					add_list_upnp_param_track(&dmctx, changed_head, p->name, "1", dm_parameter->data, 0);
					ischange = 1;
				}
			}
			free_all_list_parameter(&dmctx);
		}
		dm_ctx_clean_sub(&dmctx);
		memset(&dmctx, 0, sizeof(struct dmctx));
	}
	return ischange;
}

int dm_entry_upnp_check_alarmonchange_param(struct dmctx *dmctx)
{
	int r;
	r = dm_entry_upnp_check_onchange_param(dmctx, &list_upnp_enabled_onalarm, &list_upnp_changed_onalarm);
	return r;
}

int dm_entry_upnp_check_eventonchange_param(struct dmctx *dmctx)
{
	int r;
	r = dm_entry_upnp_check_onchange_param(dmctx, &list_upnp_enabled_onevent, &list_upnp_changed_onevent);
	return r;
}

/*************************************
 * upnp check on change version params
 *************************************/

int dm_entry_upnp_update_version_configuration(struct dmctx *dmctx)
{
	char *v, *tmp, buf[32];
	struct uci_section *s;
	int version;

	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "current_configuration_version", &v);
	version = atoi(v);
	version++;

	dmuci_get_section_type(UPNP_CFG, "@dm[0]", &tmp);
	if (!tmp || tmp[0] == '\0') {
		dmuci_add_section(UPNP_CFG, "dm", &s, &tmp);
	}
	sprintf(buf, "%d", version);
	dmuci_set_value(UPNP_CFG, "@dm[0]", "current_configuration_version", buf);
	sprintf(buf, "%ld", time(NULL));
	dmuci_set_value(UPNP_CFG, "@dm[0]", "current_configuration_epochtime", buf);

	return version;
}

int dm_entry_upnp_check_versiononchange_param(struct dmctx *pctx)
{
	struct dmctx dmctx = {0};
	struct dm_upnp_enabled_track *p;
	struct dm_parameter *dm_parameter;
	int version, fault;
	int ischange;
	char *all_instances;

	list_for_each_entry(p, &list_upnp_enabled_version, list) {
		ischange = 0;
		dm_ctx_init_sub(&dmctx, pctx->dm_type, pctx->amd_version, pctx->instance_mode);
		dmctx.user_mask = DM_SUPERADMIN_MASK;
		if (p->isobj) {
			all_instances = dm_entry_get_all_instance_numbers(&dmctx, p->name);
			if (strcmp(all_instances, p->value) != 0) {
				dm_upnp_update_enabled_track_value(p, all_instances);
				add_list_upnp_param_track(&dmctx, &list_upnp_changed_version, p->name, "1", all_instances, 1);
				ischange = 1;
			}
		}
		else {
			fault = dm_entry_param_method(&dmctx, CMD_UPNP_GET_VALUES, p->name, NULL, NULL);
			if (!fault && dmctx.list_parameter.next != &dmctx.list_parameter) {
				dm_parameter = list_entry(dmctx.list_parameter.next, struct dm_parameter, list);
				if (strcmp(dm_parameter->data, p->value) != 0) {
					dm_upnp_update_enabled_track_value(p, dm_parameter->data);
					add_list_upnp_param_track(&dmctx, &list_upnp_changed_version, p->name, p->key, dm_parameter->data, 0);
					ischange = 1;
				}
			}
			free_all_list_parameter(&dmctx);
		}
		if (ischange)
		{
			char buf[32];
			char *tmp;
			struct uci_section *s = NULL;
			version = dm_entry_upnp_update_version_configuration(&dmctx);
			sprintf(buf, "%d", version);
			if (p->key) {
				dmuci_set_value(UPNP_CFG, p->key, "version", buf);
			}
			else {
				dmuci_add_section(UPNP_CFG, "parameter_version", &s, &tmp);
				if (s != NULL) {
					dmuci_set_value_by_section(s, "version", buf);
					dmuci_set_value_by_section(s, "parameter", p->name);
					dm_upnp_update_enabled_track_key(p, section_name(s));
				}
			}
			dmuci_commit();
		}
		dm_ctx_clean_sub(&dmctx);
		memset(&dmctx, 0, sizeof(struct dmctx));
	}
	return ischange;
}

/* *************************
 * UPNP init state variables
 * ************************/
int upnp_state_variables_init(struct dmctx *dmctx)
{
	char *v, *tmp;
	struct uci_section *s;
	char buf[32];
	int n;

	dmuci_get_section_type(UPNP_CFG, "@dm[0]", &tmp);
	if (!tmp || tmp[0] == '\0') {
		dmuci_add_section(UPNP_CFG, "dm", &s, &tmp);
	}
	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "supported_datamodel_version", &v);
	n = atoi(v);
	if (n != UPNP_SUPPORTED_DATAMODEL_VERSION) {
		sprintf(buf, "%d", UPNP_SUPPORTED_DATAMODEL_VERSION);
		dmuci_set_value(UPNP_CFG, "@dm[0]", "supported_datamodel_version", buf);
		sprintf(buf, "%ld", time(NULL));
		dmuci_set_value(UPNP_CFG, "@dm[0]", "supported_datamodel_epochtime", buf);
	}
	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "supported_parameters_version", &v);
	n = atoi(v);
	if (n != UPNP_SUPPORTED_PARAMETERS_VERSION) {
		sprintf(buf, "%d", UPNP_SUPPORTED_PARAMETERS_VERSION);
		dmuci_set_value(UPNP_CFG, "@dm[0]", "supported_parameters_version", buf);
		sprintf(buf, "%ld", time(NULL));
		dmuci_set_value(UPNP_CFG, "@dm[0]", "supported_parameters_epochtime", buf);
	}
	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "current_configuration_version", &v);
	if (*v == '\0') {
		dmuci_set_value(UPNP_CFG, "@dm[0]", "current_configuration_version", "0");
		sprintf(buf, "%ld", time(NULL));
		dmuci_set_value(UPNP_CFG, "@dm[0]", "current_configuration_epochtime", buf);
	}
	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "attribute_values_version", &v);
	if (*v == '\0') {
		dmuci_set_value(UPNP_CFG, "@dm[0]", "attribute_values_version", "0");
		sprintf(buf, "%ld", time(NULL));
		dmuci_set_value(UPNP_CFG, "@dm[0]", "attribute_values_epochtime", buf);
	}

	dmuci_commit();
	return 0;
}

/* ************************************
 * UPNP get supported parameters update
 * ***********************************/

int dm_entry_upnp_get_supported_parameters_update(struct dmctx *dmctx, char **value)
{
	static char csv[128] = "";
	char *v;
	time_t time_value;

	*value = csv;

	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "supported_parameters_epochtime", &v);
	if (v[0] != '0' && v[0] != '\0') {
		time_value = atoi(v);
		char s_now[sizeof "AAAA-MM-JJTHH:MM:SS.000Z"];
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S.000Z", localtime(&time_value));
		sprintf(csv, "%d,%s", UPNP_SUPPORTED_PARAMETERS_VERSION, s_now);
	}

	return 0;
}

/* ************************************
 * UPNP get supported_datamodel update
 * ***********************************/

int dm_entry_upnp_get_supported_datamodel_update(struct dmctx *dmctx, char **value)
{
	static char csv[128] = "";
	char *v;
	time_t time_value;

	*value = csv;

	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "supported_datamodel_epochtime", &v);
	if (v[0] != '0' && v[0] != '\0') {
		time_value = atoi(v);
		char s_now[sizeof "AAAA-MM-JJTHH:MM:SS.000Z"];
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S.000Z", localtime(&time_value));
		sprintf(csv, "%d,%s", UPNP_SUPPORTED_DATAMODEL_VERSION, s_now);
	}

	return 0;
}

/* ********************************
 * UPNP get attribute values update
 * ********************************/

int dm_entry_upnp_get_attribute_values_update(struct dmctx *dmctx, char **value)
{
	static char csv[128] = "";
	char *v, *s;
	time_t time_value;

	*value = csv;

	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "attribute_values_epochtime", &v);
	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "attribute_values_version", &s);
	if (v[0] != '0' && v[0] != '\0' && s[0] != '\0') {
		time_value = atoi(v);
		char s_now[sizeof "AAAA-MM-JJTHH:MM:SS.000Z"];
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S.000Z", localtime(&time_value));
		sprintf(csv, "%s,%s", s, s_now);
	}

	return 0;
}

/* ********************************
 * UPNP get configuration update
 * ********************************/

int dm_entry_upnp_get_configuration_update(struct dmctx *dmctx, char **value)
{
	static char csv[128] = "";
	char *v, *s;
	time_t time_value;

	*value = csv;

	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "current_configuration_epochtime", &v);
	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "current_configuration_version", &s);
	if (v[0] != '\0' && s[0] != '\0') {
		time_value = atoi(v);
		char s_now[sizeof "AAAA-MM-JJTHH:MM:SS.000Z"];
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S.000Z", localtime(&time_value));
		sprintf(csv, "%s,%s", s, s_now);
	}

	return 0;
}

/* **************************************
 * UPNP get current configuration version
 * *************************************/

int dm_entry_upnp_get_current_configuration_version(struct dmctx *dmctx, char **value)
{
	dmuci_get_option_value_string(UPNP_CFG, "@dm[0]", "current_configuration_version", value);
	return 0;
}

/************************/

int dm_entry_restart_services(void)
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

int dm_entry_upnp_restart_services(void)
{
	struct package_change *pc;

	list_for_each_entry(pc, &head_package_change, list) {
		dmubus_call_set("uci", "commit", UBUS_ARGS{{"config", pc->package}}, 1);
	}
	free_all_list_package_change(&head_package_change);

	return 0;
}

void dm_upnp_apply_config(void)
{
	apply_end_session();
	dm_entry_upnp_restart_services();
}

int cli_output_dm_upnp_variable_state(struct dmctx *dmctx, int cmd, char *variable)
{
	switch (cmd) {
		case CMD_UPNP_GET_CONFIGURATION_UPDATE:
			fprintf (stdout, "{ \"ConfigurationUpdate\": \"%s\"}\n", variable);
			break;
		case CMD_UPNP_GET_CURRENT_CONFIGURATION_VERSION:
			fprintf (stdout, "{ \"CurrentConfigurationVersion\": \"%s\"}\n", variable);
			break;
		case CMD_UPNP_GET_SUPPORTED_DATA_MODEL_UPDATE:
			fprintf (stdout, "{ \"SupportedDataModelsUpdate\": \"%s\"}\n", variable);
			break;
		case CMD_UPNP_GET_SUPPORTED_PARAMETERS_UPDATE:
			fprintf (stdout, "{ \"SupportedParametersUpdate\": \"%s\"}\n", variable);
			break;
		case CMD_UPNP_GET_ATTRIBUTE_VALUES_UPDATE:
			fprintf (stdout, "{ \"AttributeValuesUpdate\": \"%s\"}\n", variable);
			break;
	}
	return 0;
}
int cli_output_dm_result(struct dmctx *dmctx, int fault, int cmd, int out)
{
	struct dm_parameter *n;
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

	switch (cmd) {
	case CMD_ADD_OBJECT:
		if (dmctx->addobj_instance) {
			fprintf (stdout, "{ \"status\": \"1\", \"instance\": \"%s\" }\n", dmctx->addobj_instance);
		} else {
			fprintf (stdout, "{ \"fault\": \"%d\" }\n", FAULT_9002);
		}
		break;

	case CMD_UPNP_ADD_INSTANCE:
		if (dmctx->addobj_instance) {
			fprintf (stdout, "{ \"status\": \"ChangesApplied\", \"instance_path\": \"%s%s%c\" }\n", dmctx->in_param, dmctx->addobj_instance, dm_delim);
		} else {
			fprintf (stdout, "{ \"fault\": \"%d\" }\n", FAULT_UPNP_701);
		}
		break;

	case CMD_DEL_OBJECT:
	case CMD_SET_VALUE:
		fprintf (stdout, "{ \"status\": \"1\" }\n");
		break;

	case CMD_SET_NOTIFICATION:
		fprintf (stdout, "{ \"status\": \"0\" }\n");
		break;

	case CMD_UPNP_SET_ATTRIBUTES:
	case CMD_UPNP_SET_VALUES:
	case CMD_UPNP_DEL_INSTANCE:
		fprintf (stdout, "{ \"status\": \"ChangesApplied\" }\n");
		break;

	case CMD_GET_NAME:
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"writable\": \"%s\" }\n", n->name, n->data);
		}
		break;
	case CMD_GET_NOTIFICATION:
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"notification\": \"%s\" }\n", n->name, n->data);
		}
		break;
	case CMD_GET_VALUE:
	case CMD_INFORM:
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"type\": \"%s\" }\n", n->name, n->data, n->type);
		}
		break;
	case CMD_UPNP_GET_VALUES:
	case CMD_UPNP_GET_SELECTED_VALUES:
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\" }\n", n->name, n->data);
		}
		break;
	case CMD_UPNP_GET_ATTRIBUTES:
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			char alrm[32] = "", evnt[32] = "", btype[16], bversion[32] = "", *stype = NULL;
			if (n->flags & DM_PARAM_ALARAM_ON_CHANGE)
				strcpy(alrm, ", \"alarmOnChange\": \"1\"");
			if (n->flags & DM_PARAM_EVENT_ON_CHANGE)
				strcpy(evnt, ", \"eventOnChange\": \"1\"");
			if (n->version)
				sprintf(bversion, ", \"version\": \"%s\"", n->version);
			switch (n->flags & NODE_DATA_ATTRIBUTE_TYPEMASK) {
			case NODE_DATA_ATTRIBUTE_TYPEINT:
				stype = "int";
				break;
			case NODE_DATA_ATTRIBUTE_TYPESTRING:
				stype = "string";
				break;
			case NODE_DATA_ATTRIBUTE_TYPELONG:
				stype = "long";
				break;
			case NODE_DATA_ATTRIBUTE_TYPEBOOL:
				stype = "boolean";
				break;
			case NODE_DATA_ATTRIBUTE_TYPEDATETIME:
				stype = "dateTime";
				break;
			case NODE_DATA_ATTRIBUTE_TYPEPTR:
				if (n->flags & NODE_DATA_ATTRIBUTE_INSTANCE)
					stype = "Instance";
				else if (n->flags & NODE_DATA_ATTRIBUTE_MULTIINSTANCE)
					stype = "MultiInstance";
				break;
			}
			if (stype)
				sprintf(btype, ",  \"type\": \"%s\"", stype);
			fprintf (stdout, "{ \"parameter\": \"%s\", \"access\": \"%s\"%s%s%s%s}\n", n->name, n->data, btype, evnt, alrm, bversion);
		}
		break;
	case CMD_UPNP_GET_INSTANCES:
	case CMD_UPNP_GET_SUPPORTED_PARAMETERS:
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			fprintf (stdout, "{ \"parameter\": \"%s\"}\n", n->name);
		}
		break;
	case CMD_UPNP_GET_ACLDATA:
		list_for_each_entry(n, &dmctx->list_parameter, list) {
			char blist[64] = "";
			char bread[64] = "";
			char bwrite[64] = "";
			char bfac[32] = "";
			if (n->flags & DM_PUBLIC_LIST) {
				strcat(blist, "Public ");
			}
			if (n->flags & DM_BASIC_LIST) {
				strcat(blist, "Basic ");
			}
			if (n->flags & DM_XXXADMIN_LIST) {
				strcat(blist, "xxxAdmin ");
			}
			if (*blist)
				blist[strlen(blist) - 1] = '\0';
			if (n->flags & DM_PUBLIC_READ) {
				strcat(bread, "Public ");
			}
			if (n->flags & DM_BASIC_READ) {
				strcat(bread, "Basic ");
			}
			if (n->flags & DM_XXXADMIN_READ) {
				strcat(bread, "xxxAdmin ");
			}
			if (*bread)
				bread[strlen(bread) - 1] = '\0';
			if (n->flags & DM_PUBLIC_WRITE) {
				strcat(bwrite, "Public ");
			}
			if (n->flags & DM_BASIC_WRITE) {
				strcat(bwrite, "Basic ");
			}
			if (n->flags & DM_XXXADMIN_WRITE) {
				strcat(bwrite, "xxxAdmin ");
			}
			if (*bwrite)
				bwrite[strlen(bwrite) - 1] = '\0';
			if (n->flags & DM_FACTORIZED)
				sprintf(bfac, ", \"factorized\": \"1\"");
			fprintf (stdout, "{ \"ACLDataPath\": \"%s\", \"List\": \"%s\", \"Read\": \"%s\", \"Write\": \"%s\"%s }\n", n->name, blist, bread, bwrite, bfac);
		}
		break;
	case CMD_UPNP_GET_ENABLED_PARAMETRS_ALARM:
		{
			struct dm_upnp_enabled_track *p;
			list_for_each_entry(p, &list_upnp_enabled_onalarm, list) {
				fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"key\": \"%s\"}\n", p->name, p->value, p->key ? p->key : "");
			}
		}
		break;
	case CMD_UPNP_GET_ENABLED_PARAMETRS_EVENT:
		{
			struct dm_upnp_enabled_track *p;
			list_for_each_entry(p, &list_upnp_enabled_onevent, list) {
				fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"key\": \"%s\"}\n", p->name, p->value, p->key ? p->key : "");
			}
		}
		break;
	case CMD_UPNP_GET_ENABLED_PARAMETRS_VERSION:
		{
			struct dm_upnp_enabled_track *p;
			list_for_each_entry(p, &list_upnp_enabled_version, list) {
				fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"key\": \"%s\"}\n", p->name, p->value, p->key ? p->key : "");
			}
		}
		break;
	case CMD_UPNP_CHECK_CHANGED_PARAMETRS_ALARM:
		{
			struct dm_upnp_enabled_track *p;
			list_for_each_entry(p, &list_upnp_changed_onalarm, list) {
				fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"key\": \"%s\"}\n", p->name, p->value, p->key ? p->key : "");
			}
		}
		break;
	case CMD_UPNP_CHECK_CHANGED_PARAMETRS_EVENT:
		{
			struct dm_upnp_enabled_track *p;
			list_for_each_entry(p, &list_upnp_changed_onevent, list) {
				fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"key\": \"%s\"}\n", p->name, p->value, p->key ? p->key : "");
			}
		}
		break;
	case CMD_UPNP_CHECK_CHANGED_PARAMETRS_VERSION:
		{
			struct dm_upnp_enabled_track *p;
			list_for_each_entry(p, &list_upnp_changed_version, list) {
				fprintf (stdout, "{ \"parameter\": \"%s\", \"value\": \"%s\", \"key\": \"%s\"}\n", p->name, p->value, p->key ? p->key : "");
			}
		}
		break;
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
	int output = 1, dmrpc;
	char *param, *next_level, *parameter_key, *value, *cmd;
	int fault = 0, status = -1;
	bool set_fault = false;
	long ms; // Milliseconds
	time_t s;  // Seconds
	struct timespec tstart, tend;
	unsigned char apply_services = 0;

	if (dmcli_timetrack)
		clock_gettime(CLOCK_REALTIME, &tstart);

	dm_ctx_init(&cli_dmctx, dmtype, amd_version, instance_mode);

	if (argc < 4) goto invalid_arguments;

	output = atoi(argv[2]);
	cmd = argv[3];

	/* GET NAME */
	if (strcmp(cmd, "get_name") == 0) {
		if (argc < 6) goto invalid_arguments;
		dmrpc = CMD_GET_NAME;
		param = argv[4];
		next_level =argv[5];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NAME, param, next_level, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NAME, output);
	}
	/* GET VALUE */
	else if (strcmp(cmd, "get_value") == 0) {
		if (argc < 5) goto invalid_arguments;
		dmrpc = CMD_GET_VALUE;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_VALUE, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_VALUE, output);
	}
	/* GET NOTIFICATION */
	else if (strcmp(cmd, "get_notification") == 0) {
		if (argc < 5) goto invalid_arguments;
		dmrpc = CMD_GET_NOTIFICATION;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NOTIFICATION, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NOTIFICATION, output);
	}
	/* SET VALUE */
	else if (strcmp(cmd, "set_value") == 0) {
		if (argc < 7 || (argc % 2) == 0) goto invalid_arguments;
		dmrpc = CMD_SET_VALUE;
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
		dmrpc = CMD_SET_NOTIFICATION;
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
		dmrpc = CMD_ADD_OBJECT;
		param =argv[5];
		parameter_key =argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_ADD_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_ADD_OBJECT, output);
	}
	/* DEL OBJECT */
	else if (strcmp(cmd, "delete_object") == 0) {
		dmrpc = CMD_DEL_OBJECT;
		if (argc < 6) goto invalid_arguments;
		param =argv[5];
		parameter_key =argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_DEL_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_DEL_OBJECT, output);
	}
	/* INFORM */
	else if (strcmp(cmd, "inform") == 0) {
		dmrpc = CMD_INFORM;
		fault = dm_entry_param_method(&cli_dmctx, CMD_INFORM, "", NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_INFORM, output);
	}
	/* UPNP GET VALUES */
	else if (strcmp(cmd, "upnp_get_values") == 0) {
		if (argc < 5) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_VALUES;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_VALUES, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_VALUES, output);
	}
	/* UPNP GET SELECTED VALUES */
	else if (strcmp(cmd, "upnp_get_selected_values") == 0) {
		if (argc < 5) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_SELECTED_VALUES;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_SELECTED_VALUES, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_SELECTED_VALUES, output);
	}
	/* UPNP GET INSTANCES */
	else if (strcmp(cmd, "upnp_get_instances") == 0) {
		if (argc < 6) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_INSTANCES;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_INSTANCES, param, argv[5], NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_INSTANCES, output);
	}
	/* UPNP GET SUPPORTED PARAMETERS */
	else if (strcmp(cmd, "upnp_get_supported_parameters") == 0) {
		if (argc < 6) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_SUPPORTED_PARAMETERS;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_SUPPORTED_PARAMETERS, param, argv[5], NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_SUPPORTED_PARAMETERS, output);
	}
	/* UPNP SET VALUE */
	else if (strcmp(cmd, "upnp_set_values") == 0) {
		if (argc < 6 || (argc % 2) == 1) goto invalid_arguments;
		dmrpc = CMD_UPNP_SET_VALUES;
		int i;
		for (i = 4; i < argc; i+=2) {
			param = argv[i];
			value = argv[i+1];
			fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_SET_VALUES, param, value, NULL);
			if (fault) break;
		}
		if (!fault) {
			apply_services = 1;
			fault = dm_entry_apply(&cli_dmctx, CMD_UPNP_SET_VALUES, NULL, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_SET_VALUES, output);
	}
	/* UPNP DEL INSTANCE */
	else if (strcmp(cmd, "upnp_delete_instance") == 0) {
		if (argc < 5) goto invalid_arguments;
		dmrpc = CMD_UPNP_DEL_INSTANCE;
		param =argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_DEL_INSTANCE, param, NULL, NULL);
		if (!fault)
			apply_services = 1;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_DEL_INSTANCE, output);
	}
	/* UPNP ADD INSTANCE */
	else if (strcmp(cmd, "upnp_add_instance") == 0) {
		char buf[256];
		dmrpc = CMD_UPNP_ADD_INSTANCE;
		int i;
		if (argc < 5 || (argc % 2) == 0) goto invalid_arguments;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_ADD_INSTANCE, param, NULL, NULL);
		if (!fault && cli_dmctx.addobj_instance) {
			struct dmctx set_dmctx = {0};
			apply_services = 1;
			if (argc >= 6) {
				dm_ctx_init_sub(&set_dmctx, dmtype, amd_version, instance_mode);
				for (i = 5; i < argc; i+=2) {
					sprintf(buf, "%s%s%c%s", param, cli_dmctx.addobj_instance, dm_delim, argv[i]); // concatenate obj path + instance + sub param
					value = argv[i+1];
					dm_entry_param_method(&set_dmctx, CMD_UPNP_SET_VALUES, buf, value, NULL);
				}
				dm_entry_apply(&set_dmctx, CMD_UPNP_SET_VALUES, NULL, NULL);
				dm_ctx_clean_sub(&set_dmctx);
			}
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_ADD_INSTANCE, output);
	}
	/* UPNP GET ATTRIBUTES */
	else if (strcmp(cmd, "upnp_get_attributes") == 0) {
		if (argc < 5) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_ATTRIBUTES;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_ATTRIBUTES, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ATTRIBUTES, output);
	}
	/* UPNP SET ATTRIBUTES */
	else if (strcmp(cmd, "upnp_set_attributes") == 0) {
		if (argc < 7 || (argc % 3) != 1) goto invalid_arguments;
		dmrpc = CMD_UPNP_SET_ATTRIBUTES;
		int i;
		for (i = 4; i < argc; i+=3) {
			param = argv[i];
			char *evnt = argv[i+1];
			char *alrm = argv[i+2];
			fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_SET_ATTRIBUTES, param, evnt, alrm);
			if (fault) break;
		}
		if(!fault) {
			fault = dm_entry_apply(&cli_dmctx, CMD_UPNP_SET_ATTRIBUTES, NULL, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_SET_ATTRIBUTES, output);
	}
	/* UPNP GET ACLDATA */
	else if (strcmp(cmd, "upnp_get_acldata") == 0) {
		if (argc < 5) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_ACLDATA;
		param = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_ACLDATA, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ACLDATA, output);
	}
	else if (strcmp(cmd, "upnp_init_state_variables") == 0) {
		dmrpc = CMD_UPNP_INIT_STATE_VARIABLES;
		upnp_state_variables_init(&cli_dmctx);
	}
	else if (strcmp(cmd, "upnp_get_supported_parameters_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_SUPPORTED_PARAMETERS_UPDATE;
		dm_entry_upnp_get_supported_parameters_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_SUPPORTED_PARAMETERS_UPDATE, var);
	}
	else if (strcmp(cmd, "upnp_get_supported_datamodel_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_SUPPORTED_DATA_MODEL_UPDATE;
		dm_entry_upnp_get_supported_datamodel_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_SUPPORTED_DATA_MODEL_UPDATE, var);
	}
	else if (strcmp(cmd, "upnp_get_configuration_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_CONFIGURATION_UPDATE;
		dm_entry_upnp_get_configuration_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_CONFIGURATION_UPDATE, var);
	}
	else if (strcmp(cmd, "upnp_get_current_configuration_version") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_CURRENT_CONFIGURATION_VERSION;
		dm_entry_upnp_get_current_configuration_version(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_CURRENT_CONFIGURATION_VERSION, var);
	}
	else if (strcmp(cmd, "upnp_get_attribute_values_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ATTRIBUTE_VALUES_UPDATE;
		dm_entry_upnp_get_attribute_values_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_ATTRIBUTE_VALUES_UPDATE, var);
	}
	else if (strcmp(cmd, "upnp_load_enabled_parametrs_track") == 0) {
		char *var;
		dmrpc = CMD_UPNP_LOAD_ENABLED_PARAMETRS_TRACK;
		dm_entry_upnp_load_tracked_parameters(&cli_dmctx);
	}
	else if (strcmp(cmd, "upnp_get_enabled_parametrs_alarm") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ENABLED_PARAMETRS_ALARM;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ENABLED_PARAMETRS_ALARM, output);
	}
	else if (strcmp(cmd, "upnp_get_enabled_parametrs_event") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ENABLED_PARAMETRS_EVENT;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ENABLED_PARAMETRS_EVENT, output);
	}
	else if (strcmp(cmd, "upnp_get_enabled_parametrs_version") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ENABLED_PARAMETRS_VERSION;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ENABLED_PARAMETRS_VERSION, output);
	}
	else if (strcmp(cmd, "upnp_check_changed_parametrs_alarm") == 0) {
		char *var;
		dmrpc = CMD_UPNP_CHECK_CHANGED_PARAMETRS_ALARM;
		dm_entry_upnp_check_alarmonchange_param(&cli_dmctx);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_CHECK_CHANGED_PARAMETRS_ALARM, output);
	}
	else if (strcmp(cmd, "upnp_check_changed_parametrs_event") == 0) {
		char *var;
		dmrpc = CMD_UPNP_CHECK_CHANGED_PARAMETRS_EVENT;
		dm_entry_upnp_check_eventonchange_param(&cli_dmctx);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_CHECK_CHANGED_PARAMETRS_EVENT, output);
	}
	else if (strcmp(cmd, "upnp_check_changed_parametrs_version") == 0) {
		char *var;
		dmrpc = CMD_UPNP_CHECK_CHANGED_PARAMETRS_VERSION;
		dm_entry_upnp_check_versiononchange_param(&cli_dmctx);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_CHECK_CHANGED_PARAMETRS_VERSION, output);
	}
	else {
		goto invalid_arguments;
	}

	dm_ctx_clean(&cli_dmctx);
	if (apply_services) {
		dm_upnp_apply_config();
	}

	if (!fault) {
		int ualarm, uversion, uevent;
		switch (dmrpc) {
		case CMD_UPNP_SET_VALUES:
		case CMD_UPNP_DEL_INSTANCE:
		case CMD_UPNP_ADD_INSTANCE:
			DM_ENTRY_UPNP_CHECK_CHANGES(ualarm, uevent, uversion);
			break;
		case CMD_UPNP_SET_ATTRIBUTES:
			DM_ENTRY_UPNP_LOAD_TRACKED_PARAMETERS();
			break;
		}
	}

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
	int i, dmrpc;
	char *param;
	char *value;
	char *parameter_key;
	char *notifset;
	unsigned char apply_services = 0;

	if (argc < 3) {
		fprintf(stderr, "Wrong arguments!\n");
		return -1;
	}

	dm_ctx_init(&cli_dmctx, dmtype, amd_version, instance_mode);
	if (strcmp(argv[2], "get_value") == 0) {
		char *param = "";
		dmrpc = CMD_GET_VALUE;
		if (argc >= 4)
			param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_VALUE, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_VALUE, 1);
	}
	else if (strcmp(argv[2], "get_name") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		dmrpc = CMD_GET_NAME;
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NAME, argv[3], argv[4], NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NAME, 1);
	}
	else if (strcmp(argv[2], "get_notification") == 0) {
		char *param = "";
		dmrpc = CMD_GET_NOTIFICATION;
		if (argc >= 4)
			param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_GET_NOTIFICATION, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_GET_NOTIFICATION, 1);
	}
	else if (strcmp(argv[2], "set_value") == 0) {
		if (argc < 6 || (argc % 2) != 0)
			goto invalid_arguments;

		dmrpc = CMD_SET_VALUE;
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
		dmrpc = CMD_SET_NOTIFICATION;
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
		dmrpc = CMD_INFORM;
		fault = dm_entry_param_method(&cli_dmctx, CMD_INFORM, "", NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_INFORM, 1);
	}
	else if (strcmp(argv[2], "add_obj") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		dmrpc = CMD_ADD_OBJECT;
		param = argv[3];
		parameter_key = argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_ADD_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_ADD_OBJECT, 1);
	}
	else if (strcmp(argv[2], "del_obj") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		dmrpc = CMD_DEL_OBJECT;
		param =argv[3];
		parameter_key =argv[4];
		fault = dm_entry_param_method(&cli_dmctx, CMD_DEL_OBJECT, param, parameter_key, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_DEL_OBJECT, 1);
	}
	else if (strcmp(argv[2], "external_command") == 0) {
		if (argc < 4)
			goto invalid_arguments;
		dmrpc = CMD_EXTERNAL_COMMAND;
		argv[argc] = NULL;
		dmentry_external_cmd(&argv[3]);
	}
	else if (strcmp(argv[2], "upnp_get_values") == 0) {
		char *param = "";
		dmrpc = CMD_UPNP_GET_VALUES;
		if (argc >= 4)
			param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_VALUES, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_VALUES, 1);
	}
	else if (strcmp(argv[2], "upnp_get_selected_values") == 0) {
		char *param = "";
		dmrpc = CMD_UPNP_GET_SELECTED_VALUES;
		if (argc >= 4)
			param = argv[3];
		param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_SELECTED_VALUES, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_SELECTED_VALUES, 1);
	}
	else if (strcmp(argv[2], "upnp_get_supported_parameters") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_SUPPORTED_PARAMETERS;
		param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_SUPPORTED_PARAMETERS, param, argv[4], NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_SUPPORTED_PARAMETERS, 1);
	}
	else if (strcmp(argv[2], "upnp_get_instances") == 0) {
		if (argc < 5)
			goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_INSTANCES;
		param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_INSTANCES, param, argv[4], NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_INSTANCES, 1);
	}
	else if (strcmp(argv[2], "upnp_set_values") == 0) {
		if (argc < 5 || (argc % 2) == 0)
			goto invalid_arguments;

		dmrpc = CMD_UPNP_SET_VALUES;
		for (i = 3; i < argc; i+=2) {
			param = argv[i];
			value = argv[i+1];
			fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_SET_VALUES, param, value, NULL);
			if (fault) break;
		}
		if (!fault) {
			apply_services = 1;
			fault = dm_entry_apply(&cli_dmctx, CMD_UPNP_SET_VALUES, parameter_key, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_SET_VALUES, 1);
	}
	else if (strcmp(argv[2], "upnp_get_attributes") == 0) {
		if (argc < 4) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_ATTRIBUTES;
		param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_ATTRIBUTES, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ATTRIBUTES, 1);
	}
	else if (strcmp(argv[2], "upnp_set_attributes") == 0) {
		if (argc < 6 || (argc % 3) != 0) goto invalid_arguments;
		dmrpc = CMD_UPNP_SET_ATTRIBUTES;
		int i;
		for (i = 3; i < argc; i+=3) {
			param = argv[i];
			char *evnt = argv[i+1];
			char *alrm = argv[i+2];
			fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_SET_ATTRIBUTES, param, evnt, alrm);
			if (fault) break;
		}
		if(!fault) {
			fault = dm_entry_apply(&cli_dmctx, CMD_UPNP_SET_ATTRIBUTES, NULL, NULL);
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_SET_ATTRIBUTES, 1);
	}
	else if (strcmp(argv[2], "upnp_add_instance") == 0) {
		char buf[256];
		int i;
		if (argc < 4 || (argc % 2) != 0)
			goto invalid_arguments;
		dmrpc = CMD_UPNP_ADD_INSTANCE;
		param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_ADD_INSTANCE, param, NULL, NULL);
		if (!fault && cli_dmctx.addobj_instance) {
			struct dmctx set_dmctx = {0};
			apply_services = 1;
			if (argc >= 5) {
				dm_ctx_init_sub(&set_dmctx, dmtype, amd_version, instance_mode);
				for (i = 4; i < argc; i+=2) {
					sprintf(buf, "%s%s%c%s", param, cli_dmctx.addobj_instance, dm_delim, argv[i]); // concatenate obj path + instance + sub param
					value = argv[i+1];
					dm_entry_param_method(&set_dmctx, CMD_UPNP_SET_VALUES, buf, value, NULL);
				}
				dm_entry_apply(&set_dmctx, CMD_UPNP_SET_VALUES, NULL, NULL);
				dm_ctx_clean_sub(&set_dmctx);
			}
		}
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_ADD_INSTANCE, 1);
	}
	else if (strcmp(argv[2], "upnp_delete_instance") == 0) {
		if (argc < 4)
			goto invalid_arguments;
		dmrpc = CMD_UPNP_DEL_INSTANCE;
		param =argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_DEL_INSTANCE, param, NULL, NULL);
		if (!fault)
			apply_services = 1;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_DEL_INSTANCE, 1);
	}
	else if (strcmp(argv[2], "upnp_get_acldata") == 0) {
		if (argc < 4) goto invalid_arguments;
		dmrpc = CMD_UPNP_GET_ACLDATA;
		param = argv[3];
		fault = dm_entry_param_method(&cli_dmctx, CMD_UPNP_GET_ACLDATA, param, NULL, NULL);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ACLDATA, 1);
	}
	else if (strcmp(argv[2], "upnp_init_state_variables") == 0) {
		dmrpc = CMD_UPNP_INIT_STATE_VARIABLES;
		upnp_state_variables_init(&cli_dmctx);
	}
	else if (strcmp(argv[2], "upnp_get_supported_parameters_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_SUPPORTED_PARAMETERS_UPDATE;
		dm_entry_upnp_get_supported_parameters_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_SUPPORTED_PARAMETERS_UPDATE, var);
	}
	else if (strcmp(argv[2], "upnp_get_supported_datamodel_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_SUPPORTED_DATA_MODEL_UPDATE;
		dm_entry_upnp_get_supported_datamodel_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_SUPPORTED_DATA_MODEL_UPDATE, var);
	}
	else if (strcmp(argv[2], "upnp_get_configuration_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_CONFIGURATION_UPDATE;
		dm_entry_upnp_get_configuration_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_CONFIGURATION_UPDATE, var);
	}
	else if (strcmp(argv[2], "upnp_get_current_configuration_version") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_CURRENT_CONFIGURATION_VERSION;
		dm_entry_upnp_get_current_configuration_version(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_CURRENT_CONFIGURATION_VERSION, var);
	}
	else if (strcmp(argv[2], "upnp_get_attribute_values_update") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ATTRIBUTE_VALUES_UPDATE;
		dm_entry_upnp_get_attribute_values_update(&cli_dmctx, &var);
		cli_output_dm_upnp_variable_state(&cli_dmctx, CMD_UPNP_GET_ATTRIBUTE_VALUES_UPDATE, var);
	}
	else if (strcmp(argv[2], "upnp_load_enabled_parametrs_track") == 0) {
		char *var;
		dmrpc = CMD_UPNP_LOAD_ENABLED_PARAMETRS_TRACK;
		dm_entry_upnp_load_tracked_parameters(&cli_dmctx);
	}
	else if (strcmp(argv[2], "upnp_get_enabled_parametrs_alarm") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ENABLED_PARAMETRS_ALARM;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ENABLED_PARAMETRS_ALARM, 1);
	}
	else if (strcmp(argv[2], "upnp_get_enabled_parametrs_event") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ENABLED_PARAMETRS_EVENT;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ENABLED_PARAMETRS_EVENT, 1);
	}
	else if (strcmp(argv[2], "upnp_get_enabled_parametrs_version") == 0) {
		char *var;
		dmrpc = CMD_UPNP_GET_ENABLED_PARAMETRS_VERSION;
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_GET_ENABLED_PARAMETRS_VERSION, 1);
	}
	else if (strcmp(argv[2], "upnp_check_changed_parametrs_alarm") == 0) {
		char *var;
		dmrpc = CMD_UPNP_CHECK_CHANGED_PARAMETRS_ALARM;
		dm_entry_upnp_check_alarmonchange_param(&cli_dmctx);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_CHECK_CHANGED_PARAMETRS_ALARM, 1);
	}
	else if (strcmp(argv[2], "upnp_check_changed_parametrs_event") == 0) {
		char *var;
		dmrpc = CMD_UPNP_CHECK_CHANGED_PARAMETRS_EVENT;
		dm_entry_upnp_check_eventonchange_param(&cli_dmctx);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_CHECK_CHANGED_PARAMETRS_EVENT, 1);
	}
	else if (strcmp(argv[2], "upnp_check_changed_parametrs_version") == 0) {
		char *var;
		dmrpc = CMD_UPNP_CHECK_CHANGED_PARAMETRS_VERSION;
		dm_entry_upnp_check_versiononchange_param(&cli_dmctx);
		cli_output_dm_result(&cli_dmctx, fault, CMD_UPNP_CHECK_CHANGED_PARAMETRS_VERSION, 1);
	}
	else {
		goto invalid_arguments;
	}

	dm_ctx_clean(&cli_dmctx);
	if (apply_services) {
		dm_upnp_apply_config();
	}

	if (!fault) {
		int ualarm, uversion, uevent;
		switch (dmrpc) {
		case CMD_UPNP_SET_VALUES:
		case CMD_UPNP_DEL_INSTANCE:
		case CMD_UPNP_ADD_INSTANCE:
			DM_ENTRY_UPNP_CHECK_CHANGES(ualarm, uevent, uversion);
			break;
		case CMD_UPNP_SET_ATTRIBUTES:
			DM_ENTRY_UPNP_LOAD_TRACKED_PARAMETERS();
			break;
		}
	}

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

