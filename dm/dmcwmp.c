/*
 *	
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author MOHAMED Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *	  Author Feten Besbes <feten.besbes@pivasoftware.com>
 *
 */

#include <stdarg.h>
#include <uci.h>
#include "dmuci.h"
#include "dmcwmp.h"
#include "dmmem.h"
#include "root.h"
#include "times.h"
#include "upnp.h"
#include "landevice.h"
#include "wandevice.h"
#include "deviceinfo.h"
#include "managementserver.h"
#include "x_inteno_se_igmp.h"
#include "x_inteno_se_wifi.h"
#include "x_inteno_se_ice.h"
#include "x_inteno_se_power_mgmt.h"
#include "x_inteno_se_ipacccfg.h"
#include "lan_interfaces.h"
#include "x_inteno_se_logincfg.h"
#include "voice_services.h"
#include "layer_3_forwarding.h"
#include "layer_2_bridging.h"
#include "ippingdiagnostics.h"
#include "x_inteno_syslog.h"
#include "dmentry.h"
#include "dmcommon.h"
#include "wifi.h"
#include "ethernet.h"
#include "wan.h"
#include "bridging.h"
#include "hosts.h"
#include "dhcp.h"
#include "ip.h"
#include "ppp.h"
#include "softwaremodules.h"
#include "routing.h"
#include "nat.h"
#include "xmpp.h"

static char *get_parameter_notification(struct dmctx *ctx, char *param);
static int remove_parameter_notification(char *param);
static int set_parameter_notification(struct dmctx *ctx, char *param,char *value);
static int get_value_obj(DMOBJECT_ARGS);
static int get_value_param(DMPARAM_ARGS);
static int mobj_get_value_in_param(DMOBJECT_ARGS);
static int mparam_get_value_in_param(DMPARAM_ARGS);
static int mparam_get_name(DMPARAM_ARGS);
static int mobj_get_name(DMOBJECT_ARGS);
static int mparam_get_name_in_param(DMPARAM_ARGS);
static int mobj_get_name_in_param(DMOBJECT_ARGS);
static int mparam_get_name_in_obj(DMPARAM_ARGS);
static int mobj_get_name_in_obj(DMOBJECT_ARGS);
static int inform_check_obj(DMOBJECT_ARGS);
static int inform_check_param(DMPARAM_ARGS);
static int mparam_add_object(DMPARAM_ARGS);
static int mobj_add_object(DMOBJECT_ARGS);
static int delete_object_obj(DMOBJECT_ARGS);
static int delete_object_param(DMPARAM_ARGS);
static int mobj_set_value(DMOBJECT_ARGS);
static int mparam_set_value(DMPARAM_ARGS);
static int mobj_get_notification_in_param(DMOBJECT_ARGS);
static int mobj_get_notification(DMOBJECT_ARGS);
static int mparam_get_notification(DMPARAM_ARGS);
static int mparam_get_notification_in_param(DMPARAM_ARGS);
static int mparam_set_notification_in_obj(DMPARAM_ARGS);
static int mobj_set_notification_in_param(DMOBJECT_ARGS);
static int mparam_set_notification_in_param(DMPARAM_ARGS);
static int mobj_set_notification_in_obj(DMOBJECT_ARGS);
static int mparam_upnp_get_instances(DMPARAM_ARGS);
static int mobj_upnp_get_instances(DMOBJECT_ARGS);
static int mparam_upnp_structured_get_value_in_param(DMPARAM_ARGS);
static int mparam_upnp_get_supportedparams(DMPARAM_ARGS);
static int mobj_upnp_get_supportedparams(DMOBJECT_ARGS);
static int mparam_upnp_get_attributes(DMPARAM_ARGS);
static int mobj_upnp_get_attributes(DMOBJECT_ARGS);
static int enabled_notify_check_obj(DMOBJECT_ARGS);
static int enabled_notify_check_param(DMPARAM_ARGS);
static int get_linker_check_obj(DMOBJECT_ARGS);
static int get_linker_check_param(DMPARAM_ARGS);
static int get_linker_value_check_obj(DMOBJECT_ARGS);
static int get_linker_value_check_param(DMPARAM_ARGS);


LIST_HEAD( list_enabled_notify);
LIST_HEAD( list_enabled_lw_notify);
LIST_HEAD(list_execute_end_session);
int end_session_flag = 0;
int ip_version = 4;
char dm_delim = DMDELIM_CWMP;
char DMROOT[64] = DMROOT_CWMP;


struct notification notifications[] = {
	[0] = {"0", "disabled"},
	[1] = {"1", "passive"},
	[2] = {"2", "active"},
	[3] = {"3", "passive_lw"},
	[4] = {"4", "passive_passive_lw"},
	[5] = {"5", "active_lw"},
	[6] = {"6", "passive_active_lw"}
};

char *DMT_TYPE[] = {
[DMT_STRING] = "xsd:string",
[DMT_UNINT] = "xsd:unsignedInt",
[DMT_INT] = "xsd:int",
[DMT_LONG] = "xsd:long",
[DMT_BOOL] = "xsd:boolean",
[DMT_TIME] = "xsd:dateTime",
};

char *UPNP_DMT_TYPE[] = {
[DMT_STRING] = "string",
[DMT_UNINT] = "unsignedInt",
[DMT_INT] = "int",
[DMT_LONG] = "long",
[DMT_BOOL] = "boolean",
[DMT_TIME] = "dateTime",
};

struct dm_permession_s DMREAD = {"0", NULL};
struct dm_permession_s DMWRITE = {"1", NULL};
struct dm_forced_inform_s DMFINFRM = {1, NULL};
struct dm_notif_s DMNONE = { "0", NULL };
struct dm_notif_s DMPASSIVE = { "1", NULL };
struct dm_notif_s DMACTIVE = { "2", NULL };

int plugin_obj_match(DMOBJECT_ARGS)
{
	if (node->matched)
		return 0;
	if (!dmctx->inparam_isparam && strstr(node->current_object, dmctx->in_param) == node->current_object) {
		node->matched++;
		dmctx->findparam = 1;
		return 0;
	}
	if (strstr(dmctx->in_param, node->current_object) == dmctx->in_param) {
		return 0;
	}
	return FAULT_9005;
}

int plugin_leaf_match(DMOBJECT_ARGS)
{
	char *str;
	if (node->matched)
		return 0;
	if (!dmctx->inparam_isparam)
		return FAULT_9005;
	str = dmctx->in_param + strlen(node->current_object);
	if (!strchr(str, dm_delim))
		return 0;
	return FAULT_9005;
}

int plugin_obj_forcedinform_match(DMOBJECT_ARGS)
{
	unsigned char fi;
	if (forced_inform) {
		if (forced_inform->get_forced_inform)
			fi = forced_inform->get_forced_inform(node->current_object, dmctx, data, instance);
		else
			fi = forced_inform->val;
		if (fi)
			return 0;
	}
	return FAULT_9005;
}

int plugin_leaf_onlyobj_match(MOBJ_ARGS)
{
	return FAULT_9005;
}

int plugin_obj_nextlevel_match(DMOBJECT_ARGS)
{
	if (node->matched > 1)
		return FAULT_9005;
	if (node->matched) {
		node->matched++;
		return 0;
	}
	if (!dmctx->inparam_isparam && strstr(node->current_object, dmctx->in_param) == node->current_object) {
		node->matched++;
		dmctx->findparam = 1;
		return 0;
	}
	if (strstr(dmctx->in_param, node->current_object) == dmctx->in_param) {
		return 0;
	}
	return FAULT_9005;
}

int plugin_leaf_nextlevel_match(DMOBJECT_ARGS)
{
	char *str;
	if (node->matched > 1)
		return FAULT_9005;
	if (node->matched)
		return 0;
	if (!dmctx->inparam_isparam)
		return FAULT_9005;
	str = dmctx->in_param + strlen(node->current_object);
	if (!strchr(str, dm_delim))
		return 0;
	return FAULT_9005;
}

int dm_browse_leaf(struct dmctx *dmctx, DMNODE *parent_node, DMLEAF *leaf, void *data, char *instance)
{
	int err = 0;
	if (!leaf)
		return 0;

	for (; leaf->parameter; leaf++) {
		err = dmctx->method_param(dmctx, parent_node, leaf->parameter, leaf->permission, leaf->type, leaf->getvalue, leaf->setvalue, leaf->forced_inform, leaf->notification, data, instance);
		if (dmctx->stop)
			return err;
	}
	return err;
}

int dm_browse(struct dmctx *dmctx, DMNODE *parent_node, DMOBJ *entryobj, void *data, char *instance)
{
	int err = 0;
	if (!entryobj)
		return 0;
	char *parent_obj = parent_node->current_object;
	for (; entryobj->obj; entryobj++) {
		DMNODE node = {0};
		node.obj = entryobj;
		node.parent = parent_node;
		node.instance_level = parent_node->instance_level;
		node.matched = parent_node->matched;
		dmasprintf(&(node.current_object), "%s%s%c", parent_obj, entryobj->obj, dm_delim);
		if (dmctx->checkobj) {
			err = dmctx->checkobj(dmctx, &node, entryobj->permission, entryobj->addobj, entryobj->delobj, entryobj->forced_inform, entryobj->notification, entryobj->get_linker, data, instance);
			if (err)
				continue;
		}
		err = dmctx->method_obj(dmctx, &node, entryobj->permission, entryobj->addobj, entryobj->delobj, entryobj->forced_inform, entryobj->notification, entryobj->get_linker, data, instance);
		if (dmctx->stop)
			return err;
		if (entryobj->checkobj && ((entryobj->checkobj)(dmctx, data) == false) ){
			continue;
		}
		if (entryobj->browseinstobj) {
			if (dmctx->instance_wildchar) {
				dm_link_inst_obj(dmctx, &node, data, dmctx->instance_wildchar);
				continue;
			}
			else {
				entryobj->browseinstobj(dmctx, &node, data, instance);
				err = dmctx->faultcode;
				if (dmctx->stop)
					return err;
				continue;
			}
		}
		if (entryobj->leaf) {
			if (dmctx->checkleaf) {
				err = dmctx->checkleaf(dmctx, &node, entryobj->permission, entryobj->addobj, entryobj->delobj, entryobj->forced_inform, entryobj->notification, entryobj->get_linker, data, instance);
				if (!err) {
					err = dm_browse_leaf(dmctx, &node, entryobj->leaf, data, instance);
					if (dmctx->stop)
						return err;
				}
			} else {
				err = dm_browse_leaf(dmctx, &node, entryobj->leaf, data, instance);
				if (dmctx->stop)
					return err;
			}
		}
		if (entryobj->nextobj) {
			err = dm_browse(dmctx, &node, entryobj->nextobj, data, instance);
			if (dmctx->stop)
				return err;
		}
	}
	return err;
}

int dm_link_inst_obj(struct dmctx *dmctx, DMNODE *parent_node, void *data, char *instance)
{
	int err = 0;
	char *parent_obj;
	DMOBJ *prevobj = parent_node->obj;
	DMOBJ *nextobj = prevobj->nextobj;
	DMLEAF *nextleaf = prevobj->leaf;

	DMNODE node = {0};
	node.obj = prevobj;
	node.parent = parent_node;
	node.instance_level = parent_node->instance_level + 1;
	node.is_instanceobj = 1;
	node.matched = parent_node->matched;

	parent_obj = parent_node->current_object;
	if (instance == NULL)
		return -1;
	dmasprintf(&node.current_object, "%s%s%c", parent_obj, instance, dm_delim);
	if (dmctx->checkobj) {
		err = dmctx->checkobj(dmctx, &node, prevobj->permission, prevobj->addobj, prevobj->delobj, prevobj->forced_inform, prevobj->notification, prevobj->get_linker, data, instance);
		if (err)
			return err;
	}
	err = dmctx->method_obj(dmctx, &node, prevobj->permission, prevobj->addobj, prevobj->delobj, prevobj->forced_inform, prevobj->notification, prevobj->get_linker, data, instance);
	if (dmctx->stop)
		return err;
	if (nextleaf) {
		if (dmctx->checkleaf) {
			err = dmctx->checkleaf(dmctx, &node, prevobj->permission, prevobj->addobj, prevobj->delobj, prevobj->forced_inform, prevobj->notification, prevobj->get_linker, data, instance);
			if (!err) {
				err = dm_browse_leaf(dmctx, &node, nextleaf, data, instance);
				if (dmctx->stop)
					return err;
			}
		} else {
			err = dm_browse_leaf(dmctx, &node, nextleaf, data, instance);
			if (dmctx->stop)
				return err;
		}
	}
	if (nextobj) {
		err = dm_browse(dmctx, &node, nextobj, data, instance);
		if (dmctx->stop)
			return err;
	}
	return err;
}

int rootcmp(char *inparam, char *rootobj)
{
	int cmp = -1;
	char buf[32];
	sprintf(buf, "%s%c", rootobj, dm_delim);
	cmp = strcmp(inparam, buf);
	return cmp;
}

//END//
/***************************
 * update instance & alias
 ***************************/
char *handle_update_instance(int instance_ranck, struct dmctx *ctx, char **last_inst, char * (*up_instance)(int action, char **last_inst, void *argv[]), int argc, ...)
{
	va_list arg;
	char *instance, *inst_mode;
	char *alias;
	int i = 0;
	int pos = instance_ranck - 1;
	unsigned int alias_resister = 0, max, action;
	void *argv[argc];

	va_start(arg, argc);
	for (i = 0; i < argc; i++) {
		argv[i] = va_arg(arg, void*);
	}
	va_end(arg);
	if (ctx->amd_version >= AMD_4) {
		if(pos < ctx->nbrof_instance) {
			action = (ctx->alias_register & (1 << pos)) ? INSTANCE_UPDATE_ALIAS : INSTANCE_UPDATE_NUMBER;
		} else {
			action = (ctx->instance_mode == INSTANCE_MODE_ALIAS) ? INSTANCE_UPDATE_ALIAS : INSTANCE_UPDATE_NUMBER;
		}
	} else {
		action = INSTANCE_UPDATE_NUMBER;
	}
	instance = up_instance(action, last_inst, argv);
	if(*last_inst)
		ctx->inst_buf[pos] = dmstrdup(*last_inst);

	return instance;
}
char *update_instance(struct uci_section *s, char *last_inst, char *inst_opt)
{
	char *instance;
	void *argv[3];

	argv[0] = s;
	argv[1] = inst_opt;
	argv[2] = "";

	instance = update_instance_alias(0, &last_inst, argv);
	return instance;
}

char *update_instance_alias(int action, char **last_inst, void *argv[])
{
	char *instance;
	char *alias;
	char buf[64] = {0};

	struct uci_section *s = (struct uci_section *) argv[0];
	char *inst_opt = (char *) argv[1];
	char *alias_opt = (char *) argv[2];

	dmuci_get_value_by_section_string(s, inst_opt, &instance);
	if (instance[0] == '\0') {
		if (*last_inst == NULL)
			sprintf(buf, "%d", 1);
		else
			sprintf(buf, "%d", atoi(*last_inst) + 1);
		instance = dmuci_set_value_by_section(s, inst_opt, buf);
	}
	*last_inst = instance;
	if (action == INSTANCE_MODE_ALIAS) {
		dmuci_get_value_by_section_string(s, alias_opt, &alias);
		if (alias[0] == '\0') {
			sprintf(buf, "cpe-%s", instance);
			alias = dmuci_set_value_by_section(s, alias_opt, buf);
		}
		sprintf(buf, "[%s]", alias);
		instance = dmstrdup(buf);
	}
	return instance;
}

char *update_instance_without_section(int action, char **last_inst, void *argv[])
{
	char *instance;
	char *alias;
	char buf[64] = {0};

	int instnbr = (int) argv[0];

	if (action == INSTANCE_MODE_ALIAS) {
		sprintf(buf, "[cpe-%d]", instnbr);
		instance = dmstrdup(buf);
	} else {
		sprintf(buf, "%d", instnbr);
		instance = dmstrdup(buf);
	}
	return instance;
}

char *get_last_instance(char *package, char *section, char *opt_inst)
{
	struct uci_section *s;
	char *inst = NULL;
	uci_foreach_sections(package, section, s) {
		inst = update_instance(s, inst, opt_inst);
	}
	return inst;
}

char *get_last_instance_lev2(char *package, char *section, char *opt_inst, char *opt_check, char *value_check)
{
	struct uci_section *s;
	char *instance = NULL;

	uci_foreach_option_cont(package, section, opt_check, value_check, s) {
		instance = update_instance(s, instance, opt_inst);
	}
	return instance;
}

int get_empty(char *refparam, struct dmctx *args, char **value)
{
	*value = "";
	return 0;
}

void add_list_paramameter(struct dmctx *ctx, char *param_name, char *param_data, char *param_type, unsigned int flags)
{
	struct dm_parameter *dm_parameter;
	struct list_head *ilist;
	list_for_each(ilist, &ctx->list_parameter)
	{
		dm_parameter = list_entry(ilist, struct dm_parameter, list);
		int cmp = strcmp(dm_parameter->name, param_name);
		if (cmp == 0) {
			return;
		} else if (cmp > 0) {
			break;
		}
	}
	dm_parameter = dmcalloc(1, sizeof(struct dm_parameter));
	_list_add(&dm_parameter->list, ilist->prev, ilist);
	dm_parameter->name = param_name;
	dm_parameter->data = param_data ? param_data : ""; //allocate memory in function
	dm_parameter->type = param_type;
	dm_parameter->flags = flags;
}

void del_list_parameter(struct dm_parameter *dm_parameter)
{
	list_del(&dm_parameter->list);
	dmfree(dm_parameter->name);
	dmfree(dm_parameter);
}

void free_all_list_parameter(struct dmctx *ctx)
{
	struct dm_parameter *dm_parameter;
	while (ctx->list_parameter.next != &ctx->list_parameter) {
		dm_parameter = list_entry(ctx->list_parameter.next, struct dm_parameter, list);
		del_list_parameter(dm_parameter);
	}
}

void add_set_list_tmp(struct dmctx *ctx, char *param, char *value)
{
	struct set_tmp *set_tmp;
	set_tmp = dmcalloc(1, sizeof(struct set_tmp));
	list_add_tail(&set_tmp->list, &ctx->set_list_tmp);
	set_tmp->name = dmstrdup(param);
	set_tmp->value = dmstrdup(value);
}

void del_set_list_tmp(struct set_tmp *set_tmp)
{
	list_del(&set_tmp->list);
	dmfree(set_tmp->name);
	dmfree(set_tmp->value);
	dmfree(set_tmp);
}

void free_all_set_list_tmp(struct dmctx *ctx)
{
	struct set_tmp *set_tmp;
	while (ctx->set_list_tmp.next != &ctx->set_list_tmp) {
		set_tmp = list_entry(ctx->set_list_tmp.next, struct set_tmp, list);
		del_set_list_tmp(set_tmp);
	}
}

void add_list_fault_param(struct dmctx *ctx, char *param, int fault)
{
	struct param_fault *param_fault;
	if (param == NULL) param = "";

	param_fault = dmcalloc(1, sizeof(struct param_fault));
	list_add_tail(&param_fault->list, &ctx->list_fault_param);
	param_fault->name = dmstrdup(param);
	param_fault->fault = fault;
}

void del_list_fault_param(struct param_fault *param_fault)
{
	list_del(&param_fault->list);
	dmfree(param_fault->name);
	dmfree(param_fault);
}

void free_all_list_fault_param(struct dmctx *ctx)
{
	struct param_fault *param_fault;
	while (ctx->list_fault_param.next != &ctx->list_fault_param) {
		param_fault = list_entry(ctx->list_fault_param.next, struct param_fault, list);
		del_list_fault_param(param_fault);
	}
}

void add_list_enabled_notify(struct dmctx *dmctx, char *param, struct dm_notif_s *notification, char *value)
{
	struct dm_enabled_notify *dm_enabled_notify;

	dm_enabled_notify = calloc(1, sizeof(struct param_fault)); // Should be calloc and not dmcalloc
	list_add_tail(&dm_enabled_notify->list, &list_enabled_notify);
	dm_enabled_notify->name = strdup(param); // Should be strdup and not dmstrdup
	dm_enabled_notify->value = value ? strdup(value) : strdup(""); // Should be strdup and not dmstrdup
	dm_enabled_notify->notification = notification->val ? strdup(notification->val): strdup(notification->get_notif(param, dmctx, NULL, NULL)); // Should be strdup and not dmstrdup
}

void add_list_enabled_lwnotify(struct dmctx *dmctx, char *param, struct dm_notif_s *notification, char *value)
{
	struct dm_enabled_notify *dm_enabled_notify;

	dm_enabled_notify = calloc(1, sizeof(struct param_fault)); // Should be calloc and not dmcalloc
	list_add_tail(&dm_enabled_notify->list, &list_enabled_lw_notify);
	dm_enabled_notify->name = strdup(param); // Should be strdup and not dmstrdup
	dm_enabled_notify->value = value ? strdup(value) : strdup(""); // Should be strdup and not dmstrdup
	dm_enabled_notify->notification = notification->val ? strdup(notification->val): strdup(notification->get_notif(param, dmctx, NULL, NULL)); // Should be strdup and not dmstrdup
}
void del_list_enabled_notify(struct dm_enabled_notify *dm_enabled_notify)
{
	list_del(&dm_enabled_notify->list); // Should be free and not dmfree
	free(dm_enabled_notify->name);
	free(dm_enabled_notify->value);
	free(dm_enabled_notify->notification);
	free(dm_enabled_notify);
}

void free_all_list_enabled_notify()
{
	struct dm_enabled_notify *dm_enabled_notify;
	while (list_enabled_notify.next != &list_enabled_notify) {
		dm_enabled_notify = list_entry(list_enabled_notify.next, struct dm_enabled_notify, list);
		del_list_enabled_notify(dm_enabled_notify);
	}
}

void free_all_list_enabled_lwnotify()
{
	struct dm_enabled_notify *dm_enabled_notify;
	while (list_enabled_lw_notify.next != &list_enabled_lw_notify) {
		dm_enabled_notify = list_entry(list_enabled_lw_notify.next, struct dm_enabled_notify, list);
		del_list_enabled_notify(dm_enabled_notify);
	}
}

void dm_update_enabled_notify(struct dm_enabled_notify *p, char *new_value)
{
	free(p->value); // Should be free and not dmfree
	p->value = strdup(new_value);
}

void dm_update_enabled_notify_byname(char *name, char *new_value)
{
	struct dm_enabled_notify *p;

	list_for_each_entry(p, &list_enabled_notify, list) {
		if (strcmp(p->name, name) == 0)
			dm_update_enabled_notify(p, new_value);
	}
}

static int upnp_get_parameter_onchange(struct dmctx *ctx, char *param, char *onchange)
{
	struct uci_list *list_onchange;
	char *pch;
	struct uci_element *e;

	dmuci_get_option_value_list("cwmp", "@notifications[0]", onchange, &list_onchange);
	if (list_onchange) {
		uci_foreach_element(list_onchange, e) {
			pch = e->name;
			if (strcmp(pch, param) == 0) {
				return 1;
			}
		}
	}
	return 0;
}

static char *get_parameter_notification(struct dmctx *ctx, char *param)
{
	int i, maxlen = 0, len;
	struct uci_list *list_notif;
	char *pch, *new_param;
	char *notification = "0";
	struct uci_element *e;

	update_param_instance_alias(ctx, param, &new_param);
	for (i = (ARRAY_SIZE(notifications) - 1); i >= 0; i--) {
		dmuci_get_option_value_list("cwmp", "@notifications[0]", notifications[i].type, &list_notif);
		if (list_notif) {
			uci_foreach_element(list_notif, e) {
				pch = e->name;
				if (strcmp(pch, new_param) == 0) {
					notification = notifications[i].value;
					return notification;
				}
				len = strlen(pch);
				if (pch[len-1] == dm_delim) {
					if (strstr(new_param, pch)) {
						if (len > maxlen )
						{
							notification = notifications[i].value;
							maxlen = len;
						}
					}
				}
			}
		}
	}
	dmfree(new_param);
	return notification;
}

static int remove_parameter_notification(char *param)
{
	int i;
	struct uci_list *list_notif;
	struct uci_element *e, *tmp;
	char *pch;
	for (i = (ARRAY_SIZE(notifications) - 1); i >= 0; i--) {
		if (param[strlen(param)-1] == dm_delim) {
			dmuci_get_option_value_list("cwmp", "@notifications[0]", notifications[i].type, &list_notif);
			if (list_notif) {
				uci_foreach_element_safe(list_notif, e, tmp) {
					pch = tmp->name;
					if (strstr(pch, param)) {
						dmuci_del_list_value("cwmp", "@notifications[0]", notifications[i].type, pch);
					}
				}
			}
		} else {
			dmuci_del_list_value("cwmp", "@notifications[0]", notifications[i].type, param);
		}
	}
	return 0;
}

int update_param_instance_alias(struct dmctx *ctx, char *param, char **new_param)
{
	char *pch, *spch, *p;
	char buf[512];
	int i = 0, j = 0;
	char pat[2] = {0};

	char *dup = dmstrdup(param);
	*pat = dm_delim;
	p = buf;
	for (pch = strtok_r(dup, pat, &spch); pch != NULL; pch = strtok_r(NULL, pat, &spch)) {
		if (isdigit(pch[0])) {
			dmstrappendchr(p, dm_delim);
			dmstrappendstr(p, pch);
			i++;
		} else if (pch[0]== '[') {
			dmstrappendchr(p, dm_delim);
			dmstrappendstr(p, ctx->inst_buf[i]);
			i++;
		} else {
			if (j > 0) {
				dmstrappendchr(p, dm_delim);
				dmstrappendstr(p, pch);
			}
			if (j == 0) {
				dmstrappendstr(p, pch);
				j++;
			}
		}
	}
	if (param[strlen(param) - 1] == dm_delim)
		dmstrappendchr(p, dm_delim);
	dmstrappendend(p);
	*new_param = dmstrdup(buf);
	dmfree(dup);
	return 0;
}

static int set_parameter_notification(struct dmctx *ctx, char *param, char *value)
{
	char *tmp = NULL, *buf = NULL, *pch, *new_param;
	char *notification = NULL;
	struct uci_section *s;
	dmuci_get_section_type("cwmp", "@notifications[0]", &tmp);
	update_param_instance_alias(ctx, param, &new_param);
	if (!tmp || tmp[0] == '\0') {
		dmuci_add_section("cwmp", "notifications", &s, &buf);
	} else {
		remove_parameter_notification(new_param);
	}

	notification = get_parameter_notification(ctx, new_param);
	if (strcmp(notification, value) == 0) {
		goto end;
	}
	if (strcmp(value, "1") == 0) {
		dmuci_add_list_value("cwmp", "@notifications[0]", "passive", new_param);
	} else if (strcmp(value, "2") == 0) {
		dmuci_add_list_value("cwmp", "@notifications[0]", "active", new_param);
	} else if (strcmp(value, "3") == 0) {
		dmuci_add_list_value("cwmp", "@notifications[0]", "passive_lw", new_param);
	} else if (strcmp(value, "4") == 0) {
		dmuci_add_list_value("cwmp", "@notifications[0]", "passive_passive_lw", new_param);
	} else if (strcmp(value, "5") == 0) {
		dmuci_add_list_value("cwmp", "@notifications[0]", "active_lw", new_param);
	} else if (strcmp(value, "6") == 0) {
		dmuci_add_list_value("cwmp", "@notifications[0]", "passive_active_lw", new_param);
	} else if (strcmp(value, "0") == 0) {
		struct uci_list *list_notif;
		struct uci_element *e;
		int i, len;
		for (i = (ARRAY_SIZE(notifications) - 1); i >= 1; i--) {
			dmuci_get_option_value_list("cwmp", "@notifications[0]", notifications[i].type, &list_notif);
			if (list_notif) {
				uci_foreach_element(list_notif, e) {
					pch = e->name;
					len = strlen(pch);
					if (pch[len-1] == dm_delim && strstr(new_param, pch)) {
						dmuci_add_list_value("cwmp", "@notifications[0]", "disabled", new_param);
						goto end;
					}
				}
			}
		}

	} else {
		return -1;
	}
end:
	dmfree(new_param);
	return 0;
}

int string_to_bool(char *v, bool *b)
{
	if (v[0] == '1' && v[1] == '\0') {
		*b = true;
		return 0;
	}
	if (v[0] == '0' && v[1] == '\0') {
		*b = false;
		return 0;
	}
	if (strcasecmp(v, "true") == 0) {
		*b = true;
		return 0;
	}
	if (strcasecmp(v, "false") == 0) {
		*b = false;
		return 0;
	}
	*b = false;
	return -1;
}

/* **********
 * get value 
 * **********/
int dm_entry_get_value(struct dmctx *dmctx)
{
	int i;
	int err = 0;
	unsigned char findparam_check = 0;
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = {.current_object = ""};

	if (dmctx->in_param[0] == '\0' || rootcmp(dmctx->in_param, root->obj) == 0) {
		dmctx->inparam_isparam = 0;
		dmctx->method_obj = get_value_obj;
		dmctx->method_param = get_value_param;
		dmctx->checkobj = NULL;
		dmctx->checkleaf = NULL;
		dmctx->findparam = 1;
		dmctx->stop = 0;
		findparam_check = 1;
	} else if (dmctx->in_param[strlen(dmctx->in_param) - 1] == dm_delim) {
		dmctx->inparam_isparam = 0;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_obj_match;
		dmctx->checkleaf = plugin_leaf_match;
		dmctx->method_obj = get_value_obj;
		dmctx->method_param = get_value_param;
		findparam_check = 1;
	} else {
		dmctx->inparam_isparam = 1;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_obj_match;
		dmctx->checkleaf = plugin_leaf_match;
		dmctx->method_obj = mobj_get_value_in_param;
		dmctx->method_param = mparam_get_value_in_param;
	}
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (findparam_check && dmctx->findparam)
		return 0;
	else
		return err;
}

static int get_value_obj(DMOBJECT_ARGS)
{
	return 0;
}

static int get_value_param(DMPARAM_ARGS)
{
	char *full_param;
	char *value = "";

	dmastrcat(&full_param, node->current_object, lastname);
	(get_cmd)(full_param, dmctx, &value);
	add_list_paramameter(dmctx, full_param, value, DMT_TYPE[type], 0);
	return 0;
}

static int mobj_get_value_in_param(DMOBJECT_ARGS)
{
	return 0;
}
static int mparam_get_value_in_param(DMPARAM_ARGS)
{
	char *full_param;
	char *value = "";

	dmastrcat(&full_param, node->current_object, lastname);
	if (strcmp(dmctx->in_param, full_param) != 0) {
		dmfree(full_param);
		return FAULT_9005;
	}

	(get_cmd)(full_param, dmctx, &value);
	add_list_paramameter(dmctx, full_param, value, DMT_TYPE[type], 0);
	dmctx->stop = true;
	return 0;
}

/* **********
 * get name 
 * **********/

int dm_entry_get_name(struct dmctx *ctx)
{
	DMOBJ *root = ctx->dm_entryobj;
	DMNODE node = {.current_object = ""};
	unsigned char findparam_check = 0;
	int err;
	if (ctx->nextlevel == 0	&& (ctx->in_param[0] == '\0' || rootcmp(ctx->in_param, root->obj) == 0)) {
		ctx->inparam_isparam = 0;
		ctx->findparam = 1;
		ctx->stop = 0;
		ctx->checkobj = NULL;
		ctx->checkleaf = NULL;
		ctx->method_obj = mobj_get_name;
		ctx->method_param = mparam_get_name;
	} else if (ctx->nextlevel && (ctx->in_param[0] == '\0')) {
		ctx->inparam_isparam = 0;
		ctx->findparam = 1;
		ctx->stop = 0;
		ctx->checkobj = plugin_obj_nextlevel_match;
		ctx->checkleaf = plugin_leaf_nextlevel_match;
		ctx->method_obj = mobj_get_name;
		ctx->method_param = mparam_get_name;
		ctx->in_param = root->obj;
		node.matched = 1;
		findparam_check = 1;
	} else if (*(ctx->in_param + strlen(ctx->in_param) - 1) == dm_delim) {
		ctx->inparam_isparam = 0;
		ctx->findparam = 0;
		ctx->stop = 0;
		ctx->method_obj = mobj_get_name_in_obj;
		ctx->method_param = mparam_get_name_in_obj;
		ctx->checkobj = (ctx->nextlevel) ? plugin_obj_nextlevel_match : plugin_obj_match;
		ctx->checkleaf = (ctx->nextlevel) ? plugin_leaf_nextlevel_match : plugin_leaf_match;
		findparam_check = 1;
	} else {
		ctx->inparam_isparam = 1;
		ctx->findparam = 0;
		ctx->stop = 0;
		ctx->checkobj = plugin_obj_match;
		ctx->checkleaf = plugin_leaf_match;
		ctx->method_obj = mobj_get_name_in_param;
		ctx->method_param = mparam_get_name_in_param;
	}
	err = dm_browse(ctx, &node, root, NULL, NULL);
	if (findparam_check && ctx->findparam)
		return 0;
	else
		return err;
}

static int mparam_get_name(DMPARAM_ARGS)
{
	char *refparam;
	char *perm = permission->val;
	dmastrcat(&refparam, node->current_object, lastname);
	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	add_list_paramameter(dmctx, refparam, perm, NULL, 0);
	return 0;
}

static int mobj_get_name(DMOBJECT_ARGS)
{
	char *refparam;
	char *perm = permission->val;
	refparam = node->current_object;
	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	add_list_paramameter(dmctx, refparam, perm, NULL, 0);
	return 0;
}

static int mparam_get_name_in_param(DMPARAM_ARGS)
{
	char *refparam;
	char *perm = permission->val;
	dmastrcat(&refparam, node->current_object, lastname);
	if (strcmp(refparam, dmctx->in_param) != 0) {
		dmfree(refparam);
		return FAULT_9005;
	}
	dmctx->stop = 1;
	if (dmctx->nextlevel == 1) {
		dmfree(refparam);
		return FAULT_9003;
	}
	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	add_list_paramameter(dmctx, refparam, perm, NULL, 0);
	return 0;
}

static int mobj_get_name_in_param(DMOBJECT_ARGS)
{
	return 0;
}

static int mparam_get_name_in_obj(DMPARAM_ARGS)
{
	char *refparam;
	char *perm = permission->val;

	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	dmastrcat(&refparam, node->current_object, lastname);
	add_list_paramameter(dmctx, refparam, perm, NULL, 0);
	return 0;
}

static int mobj_get_name_in_obj(DMOBJECT_ARGS)
{
	char *refparam;
	char *perm = permission->val;

	if (!node->matched) {
		return FAULT_9005;
	}

	if (dmctx->nextlevel && strcmp(node->current_object, dmctx->in_param) == 0)
		return 0;

	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	refparam = node->current_object;
	add_list_paramameter(dmctx, refparam, perm, NULL, 0);
	return 0;
}

/* ********************
 * get notification
 * ********************/
int dm_entry_get_notification(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };
	unsigned char findparam_check = 0;
	int err;

	if (dmctx->in_param[0] == '\0'
		|| rootcmp(dmctx->in_param, root->obj) == 0) {
		dmctx->inparam_isparam = 0;
		dmctx->findparam = 1;
		dmctx->stop = 0;
		dmctx->checkobj = NULL;
		dmctx->checkleaf = NULL;
		dmctx->method_obj = mobj_get_notification;
		dmctx->method_param = mparam_get_notification;
		findparam_check = 1;
	} else if (*(dmctx->in_param + strlen(dmctx->in_param) - 1) == dm_delim) {
		dmctx->inparam_isparam = 0;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_obj_match;
		dmctx->checkleaf = plugin_leaf_match;
		dmctx->method_obj = mobj_get_notification;
		dmctx->method_param = mparam_get_notification;
		findparam_check = 1;
	} else {
		dmctx->inparam_isparam = 1;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_obj_match;
		dmctx->checkleaf = plugin_leaf_match;
		dmctx->method_obj = mobj_get_notification_in_param;
		dmctx->method_param = mparam_get_notification_in_param;
	}
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (findparam_check && dmctx->findparam)
		return 0;
	else
		return err;
}

static int mparam_get_notification(DMPARAM_ARGS)
{
	char *value;
	char *refparam;

	dmastrcat(&refparam, node->current_object, lastname);

	if (notification == NULL) {
		value = get_parameter_notification(dmctx, refparam);
	} else {
		value = notification->val;
		if (notification->get_notif)
			value = notification->get_notif(refparam, dmctx, data, instance);
	}
	add_list_paramameter(dmctx, refparam, value, NULL, 0);
	return 0;
}

static int mobj_get_notification(DMOBJECT_ARGS)
{
	return 0;
}

static int mparam_get_notification_in_param(DMPARAM_ARGS)
{
	char *value = NULL;
	char *refparam;

	dmastrcat(&refparam, node->current_object, lastname);
	if (strcmp(refparam, dmctx->in_param) != 0) {
		dmfree(refparam);
		return FAULT_9005;
	}
	if (notification == NULL) {
		value = get_parameter_notification(dmctx, refparam);
	} else {
		value = notification->val;
		if (notification->get_notif)
			value = notification->get_notif(refparam, dmctx, data, instance);
	}
	add_list_paramameter(dmctx, refparam, value, NULL, 0);
	dmctx->stop = 1;
	return 0;
}

static int mobj_get_notification_in_param(DMOBJECT_ARGS)
{
	return 0;
}

/***************
 * inform
 ***************/
int dm_entry_inform(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = {.current_object = ""};
	int err;

	dmctx->inparam_isparam = 0;
	dmctx->stop = 0;
	dmctx->checkobj = plugin_obj_forcedinform_match;
	dmctx->checkleaf = NULL;
	dmctx->method_obj = &inform_check_obj;
	dmctx->method_param = &inform_check_param;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->stop)
		return err;
	else
		return FAULT_9005;
}

static int inform_check_obj(DMOBJECT_ARGS)
{
	return 0;
}

static int inform_check_param(DMPARAM_ARGS)
{
	char *value = "";
	char *full_param;
	unsigned char fi;

	if (!forced_inform)
		return FAULT_9005;

	if (forced_inform->get_forced_inform)
		fi = forced_inform->get_forced_inform(node->current_object, dmctx, data,
				instance);
	else
		fi = forced_inform->val;

	if (!fi)
		return FAULT_9005;

	dmastrcat(&full_param, node->current_object, lastname);
	(get_cmd)(full_param, dmctx, &value);
	add_list_paramameter(dmctx, full_param, value, DMT_TYPE[type], 0);
	return 0;
}

/* **************
 * add object 
 * **************/
int dm_entry_add_object(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };
	int err;

	if (dmctx->in_param == NULL || dmctx->in_param[0] == '\0'
			|| (*(dmctx->in_param + strlen(dmctx->in_param) - 1) != dm_delim))
		return FAULT_9005;

	dmctx->inparam_isparam = 0;
	dmctx->stop = 0;
	dmctx->checkobj = plugin_obj_match;
	dmctx->checkleaf = plugin_leaf_onlyobj_match;
	dmctx->method_obj = mobj_add_object;
	dmctx->method_param = mparam_add_object;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->stop)
		return err;
	else
		return FAULT_9005;
}

static int mparam_add_object(DMPARAM_ARGS)
{
	return FAULT_9005;
}

static int mobj_add_object(DMOBJECT_ARGS)
{
	char *addinst;
	char newparam[256];
	char *refparam = node->current_object;
	char *perm = permission->val;
	char *objinst;

	if (strcmp(refparam, dmctx->in_param) != 0)
		return FAULT_9005;

	dmctx->stop = 1;
	if (node->is_instanceobj)
		return FAULT_9005;
	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	if (perm[0] == '0' || addobj == NULL)
		return FAULT_9005;

	int fault = (addobj)(dmctx, &instance);
	if (fault)
		return fault;
	dmctx->addobj_instance = instance;
	dmasprintf(&objinst, "%s%s%c", dmctx->current_obj, instance, dm_delim);
	set_parameter_notification(dmctx, objinst, "0");
	dmfree(objinst);
	return 0;
}
/* **************
 * del object 
 * **************/
int dm_entry_delete_object(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };
	int err;

	if (dmctx->in_param == NULL || dmctx->in_param[0] == '\0'
			|| (*(dmctx->in_param + strlen(dmctx->in_param) - 1) != dm_delim))
		return FAULT_9005;

	dmctx->inparam_isparam = 0;
	dmctx->stop = 0;
	dmctx->checkobj = plugin_obj_match;
	dmctx->checkleaf = plugin_leaf_onlyobj_match;
	dmctx->method_obj = delete_object_obj;
	dmctx->method_param = delete_object_param;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->stop)
		return err;
	else
		return FAULT_9005;
}

static int delete_object_obj(DMOBJECT_ARGS)
{
	char *refparam = node->current_object;
	char *perm = permission->val;
	unsigned char del_action = DEL_INST;
	if (strcmp(refparam, dmctx->in_param) != 0)
		return FAULT_9005;

	dmctx->stop = 1;

	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	if (perm[0] == '0' || delobj == NULL)
		return FAULT_9005;

	if (!node->is_instanceobj)
		del_action = DEL_ALL;
	int fault = (delobj)(dmctx, del_action);
	return fault;
}

static int delete_object_param(DMPARAM_ARGS)
{
	return FAULT_9005;
}

/* **************
 * set value  
 * **************/
int dm_entry_set_value(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };
	int err;

	if (dmctx->in_param == NULL || dmctx->in_param[0] == '\0'
			|| (*(dmctx->in_param + strlen(dmctx->in_param) - 1) == dm_delim))
		return FAULT_9005;

	dmctx->inparam_isparam = 1;
	dmctx->stop = 0;
	dmctx->checkobj = plugin_obj_match;
	dmctx->checkleaf = plugin_leaf_match;
	dmctx->method_obj = mobj_set_value;
	dmctx->method_param = mparam_set_value;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->stop)
		return err;
	else
		return FAULT_9005;
}

static int mobj_set_value(DMOBJECT_ARGS)
{
	return FAULT_9005;
}

static int mparam_set_value(DMPARAM_ARGS)
{
	int err;
	char *refparam;
	char *perm;
	char *v = "";

	dmastrcat(&refparam, node->current_object, lastname);
	if (strcmp(refparam, dmctx->in_param) != 0) {
		dmfree(refparam);
		return FAULT_9005;
	}
	dmctx->stop = 1;

	if (dmctx->setaction == VALUECHECK) {
		perm = permission->val;
		if (permission->get_permission != NULL)
			perm = permission->get_permission(refparam, dmctx, data, instance);

		if (perm[0] == '0' || !set_cmd) {
			dmfree(refparam);
			return FAULT_9008;
		}
		int fault = (set_cmd)(refparam, dmctx, VALUECHECK, dmctx->in_value);
		if (fault) {
			dmfree(refparam);
			return fault;
		}
		add_set_list_tmp(dmctx, dmctx->in_param, dmctx->in_value);
	}
	else if (dmctx->setaction == VALUESET) {
		(set_cmd)(refparam, dmctx, VALUESET, dmctx->in_value);
		(get_cmd)(refparam, dmctx, &v);
		dm_update_enabled_notify_byname(refparam, v);
	}
	dmfree(refparam);
	return 0;
}

/* ****************
 * set notification  
 * ****************/
int dm_entry_set_notification(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };
	unsigned char findparam_check = 0;
	int err;

	if (dmcommon_check_notification_value(dmctx->in_notification) < 0)
		return FAULT_9003;

	if (dmctx->in_param[0] == '\0' || rootcmp(dmctx->in_param, root->obj) == 0) {
		return FAULT_9009;
	} else if (*(dmctx->in_param + strlen(dmctx->in_param) - 1) == dm_delim) {
		dmctx->inparam_isparam = 0;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_obj_match;
		dmctx->checkleaf = plugin_leaf_match;
		dmctx->method_obj = mobj_set_notification_in_obj;
		dmctx->method_param = mparam_set_notification_in_obj;
	} else {
		dmctx->inparam_isparam = 1;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_obj_match;
		dmctx->checkleaf = plugin_leaf_match;
		dmctx->method_obj = mobj_set_notification_in_param;
		dmctx->method_param = mparam_set_notification_in_param;
	}
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->stop)
		return err;
	else
		return FAULT_9005;
}

/* SET Notification*/

static int mparam_set_notification_in_obj(DMPARAM_ARGS)
{
	return FAULT_9005;
}

static int mobj_set_notification_in_obj(DMOBJECT_ARGS)
{
	int err;
	char *refparam;
	char *perm;
	char tparam[256];

	refparam = node->current_object;
	if (strcmp(refparam, dmctx->in_param) != 0) {
		return FAULT_9005;
	}
	dmctx->stop = 1;
	if (!dmctx->notification_change) {
		return 0;
	}
	if (dmctx->setaction == VALUECHECK) {
		if (notification)
			return FAULT_9009;

		add_set_list_tmp(dmctx, dmctx->in_param, dmctx->in_notification);
	}
	else if (dmctx->setaction == VALUESET) {
		set_parameter_notification(dmctx, dmctx->in_param, dmctx->in_notification);
		cwmp_set_end_session(END_SESSION_RELOAD);
	}
	return 0;
}

static int mparam_set_notification_in_param(DMPARAM_ARGS)
{
	int err;
	char *refparam;
	char tparam[256];

	dmastrcat(&refparam, node->current_object, lastname);
	if (strcmp(refparam, dmctx->in_param) != 0) {
		dmfree(refparam);
		return FAULT_9005;
	}

	dmctx->stop = 1;
	if (!dmctx->notification_change) {
		return 0;
	}
	if (dmctx->setaction == VALUECHECK) {
		if (notification) {
			dmfree(refparam);
			return FAULT_9009;
		}
		add_set_list_tmp(dmctx, dmctx->in_param, dmctx->in_notification);
	} else if (dmctx->setaction == VALUESET) {
		set_parameter_notification(dmctx, dmctx->in_param, dmctx->in_notification);
		cwmp_set_end_session(END_SESSION_RELOAD);
	}
	dmfree(refparam);
	return 0;
}

static int mobj_set_notification_in_param(DMOBJECT_ARGS)
{
	return FAULT_9005;
}

/*********************
 * load enabled notify
 ********************/
int dm_entry_enabled_notify(struct dmctx *dmctx)
{
	int err;
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };
	//unsigned char findparam_check = 0;

	dmctx->method_obj = enabled_notify_check_obj;
	dmctx->method_param = enabled_notify_check_param;
	dmctx->checkobj = NULL ;
	dmctx->checkleaf = NULL;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	return err;

}

static int enabled_notify_check_obj(DMOBJECT_ARGS)
{
	return FAULT_9005;
}
// TO check
static int enabled_notify_check_param(DMPARAM_ARGS)
{
	char *refparam;
	char *value = "";
	char *notif;

	dmastrcat(&refparam, dmctx->current_obj, lastname);

	if (notification == NULL) {
		notif = get_parameter_notification(dmctx, refparam);
	} else {
		notif = notification->val;
		if (notification->get_notif)
			notif = notification->get_notif(refparam, dmctx, data, instance);
	}
	if (notif[0] == '0') {
		dmfree(refparam);
		return 0;
	}
	(get_cmd)(refparam, dmctx, &value);
	if (notif[0] == '1' || notif[0] == '2'
			|| notif[0] == '4' || notif[0] == '6')
		add_list_enabled_notify(dmctx, refparam, notification, value);
	if (notif[0] >= '3') {
		add_list_enabled_lwnotify(dmctx, refparam, notification, value);
	}
	dmfree(refparam);
	return 0;
}

/******************
 * get linker param
 *****************/
int dm_entry_get_linker(struct dmctx *dmctx)
{
	int err;
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };

	dmctx->method_obj = get_linker_check_obj;
	dmctx->method_param = get_linker_check_param;
	dmctx->checkobj = plugin_obj_match;
	dmctx->checkleaf = plugin_leaf_onlyobj_match;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->stop)
		return err;
	else
		return FAULT_9005;
}

static int get_linker_check_obj(DMOBJECT_ARGS)
{
	char *link_val = "";

	if (!get_linker) {
		return  FAULT_9005;
	}
	get_linker(node->current_object, dmctx, NULL, NULL, &link_val);
	if (dmctx->linker[0] == '\0')
		return  FAULT_9005;
	if (link_val && link_val[0] != '\0' && strcmp(link_val, dmctx->linker) == 0) {
		dmctx->linker_param = dmstrdup(node->current_object);
		dmctx->stop = true;
		return 0;
	}
	return FAULT_9005;
}

static int get_linker_check_param(DMPARAM_ARGS)
{
	return FAULT_9005;
}

/******************
 * get linker value
 *****************/
int dm_entry_get_linker_value(struct dmctx *dmctx)
{
	int err ;
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };

	dmctx->method_obj = get_linker_value_check_obj;
	dmctx->method_param = get_linker_value_check_param;
	dmctx->checkobj = plugin_obj_match;
	dmctx->checkleaf = plugin_leaf_match;
	dmentry_instance_lookup_inparam(dmctx);
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->stop)
		return err;
	else
		return FAULT_9005;
}

static int get_linker_value_check_obj(DMOBJECT_ARGS)
{
	char *link_val;
	if (!get_linker)
		return FAULT_9005;

	if (strcmp(dmctx->current_obj, dmctx->in_param) == 0) {
		get_linker(dmctx->current_obj, dmctx, NULL, NULL, &link_val);
		dmctx->linker = dmstrdup(link_val);
		dmctx->stop = true;
		return 0;
	}
	return FAULT_9005;
}

static int get_linker_value_check_param(DMPARAM_ARGS)
{
	return FAULT_9005;
}
/* ******************
 * UPNP entries
 * ******************/

int upnp_map_cwmp_fault(int cwmp_fault)
{
	switch (cwmp_fault) {
	case FAULT_9005:
		return FAULT_UPNP_703;
	case FAULT_9003:
		return FAULT_UPNP_701;
	case FAULT_9007:
		return FAULT_UPNP_705;
	case FAULT_9008:
		return FAULT_UPNP_706;
	}
	if (cwmp_fault > __FAULT_UPNP_MAX)
		return FAULT_UPNP_701;
	return cwmp_fault;
}

int plugin_upnp_structured_obj_match(DMOBJECT_ARGS)
{
	if (node->matched)
		return 0;
	if (!dmctx->inparam_isparam && strstructered(node->current_object, dmctx->in_param) == STRUCTERED_SAME) {
		node->matched++;
		dmctx->findparam = 1;
		return 0;
	}
	if (strstructered(dmctx->in_param, node->current_object) == STRUCTERED_PART) {
		return 0;
	}
	return FAULT_UPNP_703;
}

int plugin_upnp_leaf_match(DMOBJECT_ARGS)
{
	char *str;
	if (node->matched)
		return 0;
	if (!dmctx->inparam_isparam)
		return FAULT_UPNP_703;
	str = dmctx->in_param + strlen(node->current_object);
	if (!strchr(str, dm_delim))
		return 0;
	return FAULT_UPNP_703;
}

static int plugin_upnp_obj_depth_match(DMOBJECT_ARGS)
{
	if (node->matched) {
		node->matched++;
	}
	else if (strcmp(node->current_object, dmctx->in_param) == 0) {
		node->matched++;
		dmctx->findparam = 1;
	}
	else if (strstr(dmctx->in_param, node->current_object) == dmctx->in_param) {
		return 0;
	}
	if (dmctx->depth == 0 || dmctx->depth >= (node->matched - 1))
		return 0;
	return FAULT_UPNP_703;
}

int plugin_upnp_leaf_depth_match(DMOBJECT_ARGS)
{
	char *str;
	if (!node->matched) {
		if (!dmctx->inparam_isparam) {
			return FAULT_UPNP_703;
		}
		else {
			str = dmctx->in_param + strlen(node->current_object);
			if (strchr(str, dm_delim))
				return FAULT_UPNP_703;
		}
	}
	if (dmctx->depth == 0)
		return 0;
	if (dmctx->depth >= node->matched)
		return 0;
	return FAULT_UPNP_703;
}

static int plugin_upnp_skip_leafs(DMOBJECT_ARGS)
{
	return FAULT_UPNP_703;
}

/* ******************
 * UPNP get instances
 * ******************/

int dm_entry_upnp_get_instances(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = {.current_object = ""};
	int err;
	char buf[4] = {0};
	buf[0] = dm_delim;

	if (*(dmctx->in_param) == '\0')
		dmctx->in_param = buf;

	if (*(dmctx->in_param + strlen(dmctx->in_param) - 1) != dm_delim)
		return FAULT_UPNP_701;

	dmctx->inparam_isparam = 0;
	dmctx->findparam = 0;
	dmctx->stop = 0;
	dmctx->checkobj = plugin_upnp_obj_depth_match;
	dmctx->checkleaf = plugin_upnp_skip_leafs;
	dmctx->method_obj = mobj_upnp_get_instances;
	dmctx->method_param = mparam_upnp_get_instances;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->findparam)
		return 0;
	else
		return (upnp_map_cwmp_fault(err));
}

static int mparam_upnp_get_instances(DMPARAM_ARGS)
{
	return 0;
}

static int mobj_upnp_get_instances(DMOBJECT_ARGS)
{
	char *refparam;
	refparam = node->current_object;
	if (!node->is_instanceobj || !node->matched)
		return FAULT_UPNP_703;
	add_list_paramameter(dmctx, refparam, NULL, NULL, 0);
	return 0;
}

/* *****************************
 * UPNP get supported parameters
 * *****************************/

int dm_entry_upnp_get_supported_parameters(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = {.current_object = ""};
	int err;

	char buf[4] = {0};
	buf[0] = dm_delim;

	if (*(dmctx->in_param) == '\0')
		dmctx->in_param = buf;

	if (*(dmctx->in_param + strlen(dmctx->in_param) - 1) != dm_delim)
		dmctx->inparam_isparam = 1;
	else
		dmctx->inparam_isparam = 0;
	dmctx->instance_wildchar = "#";
	dmctx->findparam = 0;
	dmctx->stop = 0;
	dmctx->checkobj = plugin_upnp_obj_depth_match;
	dmctx->checkleaf = plugin_upnp_leaf_depth_match;
	dmctx->method_obj = mobj_upnp_get_supportedparams;
	dmctx->method_param = mparam_upnp_get_supportedparams;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->findparam)
		return 0;
	else
		return (upnp_map_cwmp_fault(err));
}

static int mparam_upnp_get_supportedparams(DMPARAM_ARGS)
{
	char *refparam;
	dmastrcat(&refparam, node->current_object, lastname);
	if (dmctx->inparam_isparam) {
		if (strcmp(refparam, dmctx->in_param) == 0) {
			dmctx->findparam = 1;
			add_list_paramameter(dmctx, refparam, NULL, NULL, 0);
			return 0;
		}
	}
	else {
		add_list_paramameter(dmctx, refparam, NULL, NULL, 0);
		return 0;
	}
	return FAULT_UPNP_703;
}

static int mobj_upnp_get_supportedparams(DMOBJECT_ARGS)
{
	char *refparam;
	if (node->matched <= 1)
		return FAULT_UPNP_703;
	if (dmctx->depth == (node->matched - 1)) {
		if (node->obj->browseinstobj && !node->is_instanceobj)
			dmastrcat(&refparam, node->current_object, "#/");
		else
			refparam = node->current_object;
		add_list_paramameter(dmctx, refparam, NULL, NULL, 0);
		return 0;
	}
	return FAULT_UPNP_703;
}

/* ************************
 * UPNP get selected values
 * ************************/

int dm_entry_upnp_get_selected_values(struct dmctx *dmctx)
{
	int i;
	int err = 0;
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = {.current_object = ""};

	if (dmctx->in_param[0] == '\0' || rootcmp(dmctx->in_param, root->obj) == 0) {
		dmctx->inparam_isparam = 0;
		dmctx->method_obj = get_value_obj;
		dmctx->method_param = get_value_param;
		dmctx->checkobj = NULL;
		dmctx->checkleaf = NULL;
		dmctx->findparam = 1;
		dmctx->stop = 0;
	} else if (dmctx->in_param[strlen(dmctx->in_param) - 1] == dm_delim) {
		dmctx->inparam_isparam = 0;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_upnp_structured_obj_match;
		dmctx->checkleaf = plugin_upnp_leaf_match;
		dmctx->method_obj = get_value_obj;
		dmctx->method_param = get_value_param;
	} else {
		dmctx->inparam_isparam = 1;
		dmctx->findparam = 0;
		dmctx->stop = 0;
		dmctx->checkobj = plugin_upnp_structured_obj_match;
		dmctx->checkleaf = plugin_upnp_leaf_match;
		dmctx->method_obj = mobj_get_value_in_param;
		dmctx->method_param = mparam_upnp_structured_get_value_in_param;
	}
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->findparam)
		return 0;
	else
		return (upnp_map_cwmp_fault(err));
}

static int mparam_upnp_structured_get_value_in_param(DMPARAM_ARGS)
{
	char *full_param;
	char *value = "";

	dmastrcat(&full_param, node->current_object, lastname);
	if (strstructered(dmctx->in_param, full_param) == STRUCTERED_NULL) {
		dmfree(full_param);
		return FAULT_UPNP_703;
	}
	dmctx->findparam = 1;
	(get_cmd)(full_param, dmctx, &value);
	add_list_paramameter(dmctx, full_param, value, DMT_TYPE[type], 0);
	return 0;
}

/* ***************
 * UPNP get values
 * ***************/

int dm_entry_upnp_get_values(struct dmctx *dmctx)
{
	int err;
	err = dm_entry_get_value(dmctx);
	return (upnp_map_cwmp_fault(err));
}

/* ***************
 * UPNP set values
 * ***************/

int dm_entry_upnp_set_values(struct dmctx *dmctx)
{
	int err;
	err = dm_entry_set_value(dmctx);
	return (upnp_map_cwmp_fault(err));
}

/* ********************
 * UPNP delete instance
 * *******************/

int dm_entry_upnp_delete_instance(struct dmctx *dmctx)
{
	int err;
	err = dm_entry_delete_object(dmctx);
	return (upnp_map_cwmp_fault(err));
}

/* ********************
 * UPNP add instance
 * *******************/

int dm_entry_upnp_add_instance(struct dmctx *dmctx)
{
	int err;
	err = dm_entry_add_object(dmctx);
	return (upnp_map_cwmp_fault(err));
}

/* ********************
 * UPNP get attributes
 * ********************/
int dm_entry_upnp_get_attributes(struct dmctx *dmctx)
{
	DMOBJ *root = dmctx->dm_entryobj;
	DMNODE node = { .current_object = "" };
	unsigned char findparam_check = 0;
	int err;
	char buf[4] = {0};
	buf[0] = dm_delim;

	if (*(dmctx->in_param) == '\0')
		dmctx->in_param = buf;

	if (*(dmctx->in_param + strlen(dmctx->in_param) - 1) != dm_delim)
		dmctx->inparam_isparam = 1;
	else
		dmctx->inparam_isparam = 0;

	dmctx->findparam = 0;
	dmctx->stop = 0;
	dmctx->checkobj = plugin_obj_match;
	dmctx->checkleaf = plugin_leaf_match;
	dmctx->method_obj = mobj_upnp_get_attributes;
	dmctx->method_param = mparam_upnp_get_attributes;
	err = dm_browse(dmctx, &node, root, NULL, NULL);
	if (dmctx->findparam)
		return 0;
	else
		return (upnp_map_cwmp_fault(err));
}

static int mparam_upnp_get_attributes(DMPARAM_ARGS)
{
	char *refparam;
	unsigned int flags = 0;
	char *perm = permission->val;

	dmastrcat(&refparam, node->current_object, lastname);

	if (strcmp(refparam, dmctx->in_param) != 0)
			return FAULT_UPNP_703;

	dmctx->stop = 1;

	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	perm = (*perm == '1') ? "readWrite" : "readOnly";

	if (upnp_get_parameter_onchange(dmctx, refparam, "alarmchange"))
		flags |= DM_PARAM_ALARAM_ON_CHANGE;
	if (upnp_get_parameter_onchange(dmctx, refparam, "eventchange"))
		flags |= DM_PARAM_EVENT_ON_CHANGE;

	add_list_paramameter(dmctx, refparam, perm, UPNP_DMT_TYPE[type], flags);
	return 0;
}

static int mobj_upnp_get_attributes(DMOBJECT_ARGS)
{
	char *refparam;
	unsigned int flags = 0;
	char *perm = permission->val;

	refparam = node->current_object;

	if (strcmp(refparam, dmctx->in_param) != 0)
			return FAULT_UPNP_703;

	dmctx->stop = 1;

	if (permission->get_permission != NULL)
		perm = permission->get_permission(refparam, dmctx, data, instance);

	perm = (*perm == '1') ? "readWrite" : "readOnly";

	if (upnp_get_parameter_onchange(dmctx, refparam, "eventchange"))
		flags |= DM_PARAM_EVENT_ON_CHANGE;

	add_list_paramameter(dmctx, refparam, perm, NULL, flags);

	return 0;
}

/********************/

int dm_add_end_session(struct dmctx *ctx, void(*function)(struct execute_end_session *), int action, void *data)
{
	struct execute_end_session *execute_end_session;

	execute_end_session = calloc (1,sizeof(struct execute_end_session));
	if (execute_end_session == NULL)
	{
		return -1;
	}
	execute_end_session->action = action;
	execute_end_session->data = data;
	execute_end_session->function = function;
	execute_end_session->amd_version = ctx->amd_version;
	execute_end_session->instance_mode = ctx->instance_mode;
	execute_end_session->dm_type = ctx->dm_type;
	list_add_tail (&(execute_end_session->list), &(list_execute_end_session));

	return 0;
}

int cwmp_free_dm_end_session(struct execute_end_session *execute_end_session)
{
	if(execute_end_session != NULL)
	{
		if(execute_end_session->data != NULL)
		{
			FREE(execute_end_session->data);
		}
		FREE(execute_end_session);
	}
	return 0;
}

int apply_end_session()
{
	struct execute_end_session *p, *q;
	list_for_each_entry_safe(p, q, &(list_execute_end_session), list) {
		p->function(p);
		list_del(&(p->list));
		cwmp_free_dm_end_session(p);
	}
	return 0;
}

void cwmp_set_end_session (unsigned int flag)
{
	end_session_flag |= flag;
}

char *dm_print_path(char *fpath, ...)
{
	static char pathbuf[512] = "";
	va_list vl;

	va_start(vl, fpath);
	vsprintf(pathbuf, fpath, vl);
	va_end(vl);
	return pathbuf;
}
