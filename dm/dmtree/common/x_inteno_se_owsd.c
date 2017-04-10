/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *
 */


#include <uci.h>
#include <ctype.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "x_inteno_se_owsd.h"

struct owsd_listenargs cur_owsd_listenargs = {0};

DMLEAF XIntenoSeOwsdParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, NOTIFICATION, linker*/
{"Socket", &DMWRITE, DMT_STRING, get_x_inteno_owsd_global_sock, set_x_inteno_owsd_global_sock, NULL, NULL},
{"Redirect", &DMWRITE, DMT_STRING, get_x_inteno_owsd_global_redirect, set_x_inteno_owsd_global_redirect, NULL, NULL},
{0}
};

DMOBJ XIntenoSeOwsdObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, nextobj, leaf*/
{"X_INTENO_SE_ListenObj", &DMREAD, add_owsd_listen, delete_owsd_listen_instance, NULL, browseXIntenoOwsdListenObj, NULL, NULL, NULL, X_INTENO_SE_ListenObjParams, NULL},
{0}
};

DMLEAF X_INTENO_SE_ListenObjParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, NOTIFICATION, linker*/
{"Alias", &DMWRITE, DMT_STRING, get_x_inteno_owsd_listenobj_alias, set_x_inteno_owsd_listenobj_alias, NULL, NULL},
{"Port", &DMWRITE, DMT_UNINT, get_x_inteno_owsd_listenobj_port, set_x_inteno_owsd_listenobj_alias, NULL, NULL},
{"Interface", &DMWRITE, DMT_STRING, get_x_inteno_owsd_listenobj_interface, set_x_inteno_owsd_listenobj_interface, NULL, NULL},
{"Ipv6", &DMWRITE, DMT_BOOL, get_x_inteno_owsd_listenobj_ipv6_enable, set_x_inteno_owsd_listenobj_ipv6_enable, NULL, NULL},
{"Whitelist_interface", &DMWRITE, DMT_BOOL, get_x_inteno_owsd_listenobj_whitelist_interface, set_x_inteno_owsd_listenobj_whitelist_interface, NULL, NULL},
{"Whitelist_dhcp", &DMWRITE, DMT_BOOL, get_x_inteno_owsd_listenobj_whitelist_dhcp, set_x_inteno_owsd_listenobj_whitelist_dhcp, NULL, NULL},
{"Origin", &DMWRITE, DMT_STRING, get_x_inteno_owsd_listenobj_origin, set_x_inteno_owsd_listenobj_origin, NULL, NULL},
{0}
};

inline int browseXIntenoOwsdListenObj(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *iowsd_listen = NULL, *iowsd_listen_last = NULL;
	struct uci_section *s = NULL;

	uci_foreach_sections("owsd", "owsd-listen", s) {
		init_args_owsd_listen(dmctx, s);
		iowsd_listen =  handle_update_instance(1, dmctx, &iowsd_listen_last, update_instance_alias, 3, s, "olisteninstance", "olistenalias");
		handle_update_instance(1, dmctx, &iowsd_listen_last, update_instance_alias, 3, s, "olisteninstance", "olistenalias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, NULL, iowsd_listen) == DM_STOP)
			break;
	}
	DM_CLEAN_ARGS(cur_owsd_listenargs);
	return 0;

}

inline int init_args_owsd_listen(struct dmctx *ctx, struct uci_section *s)
{
	struct owsd_listenargs *args = &cur_owsd_listenargs;
	args->owsd_listensection = s;
	return 0;
}

/************************************************************************************* 
**** function related to owsd_origin ****
**************************************************************************************/
int get_x_inteno_owsd_global_sock(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("owsd", "global", "sock", value);
	return 0;
}

int set_x_inteno_owsd_global_sock(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("owsd", "global", "sock", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_owsd_global_redirect(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("owsd", "global", "redirect", value);
	return 0;
}

int set_x_inteno_owsd_global_redirect(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("owsd", "global", "redirect", value);
			return 0;
	}
	return 0;
}

/*************************************************************************************
**** function related to owsd_listenobj ****
**************************************************************************************/

int get_x_inteno_owsd_listenobj_port(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_owsd_listenargs.owsd_listensection, "port", value);
	if ((*value)[0] == '\0') {
		*value = "";
	}		
	return 0;
}

int set_x_inteno_owsd_listenobj_port(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "port", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_owsd_listenobj_interface(char *refparam, struct dmctx *ctx, char **value)
{
	char *iface, *linker;

	dmuci_get_value_by_section_string(cur_owsd_listenargs.owsd_listensection, "interface", &iface);
#ifdef DATAMODEL_TR098
	if (iface[0] != '\0') {
		dmastrcat(&linker, "linker_interface:", iface);
		adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cWANDevice%c", dm_delim, dm_delim), linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
		if (*value == NULL) {
			adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cLANDevice%c", dm_delim, dm_delim), linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
			if (*value == NULL)
				*value = "";
		}
		dmfree(linker);
	}
#endif
#ifdef DATAMODEL_TR181
	if (iface[0] != '\0') {
		adm_entry_get_linker_param(ctx, dm_print_path(DMROOT"%cIP%cInterface%c", dm_delim, dm_delim, dm_delim), iface, value); // MEM WILL BE FREED IN DMMEMCLEAN
		if (*value == NULL)
			*value = "";
	}
#endif
	return 0;
}

int set_x_inteno_owsd_listenobj_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int check;
	char *linker, *iface;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(ctx, value, &linker);
			if (linker) {
#ifdef DATAMODEL_TR098
				iface = linker + sizeof("linker_interface:") - 1;
#endif
				dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "interface", iface);
				dmfree(linker);
			}
		return 0;
	}
	return 0;
}

int get_x_inteno_owsd_listenobj_ipv6_enable(char *refparam, struct dmctx *ctx, char **value)
{

	dmuci_get_value_by_section_string(cur_owsd_listenargs.owsd_listensection, "ipv6", value);
	if ((*value)[0] != '\0' && (*value)[0] == 'o' && (*value)[1] == 'n' ) {
		*value = "1";
	}
	else
		*value = "0";
	return 0;
}

int set_x_inteno_owsd_listenobj_ipv6_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "ipv6", "on");
			else
				dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "ipv6", "off");
			return 0;
	}
	return 0;
}

int get_x_inteno_owsd_listenobj_whitelist_interface(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_owsd_listenargs.owsd_listensection, "whitelist_interface_as_origin", value);
	if ((*value)[0] == '\0' ) {
		*value = "0";
	}
	return 0;
}

int set_x_inteno_owsd_listenobj_whitelist_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "whitelist_interface_as_origin", "1");
			else
				dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "whitelist_interface_as_origin", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_owsd_listenobj_whitelist_dhcp(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_owsd_listenargs.owsd_listensection, "whitelist_dhcp_domains", value);
	if ((*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int set_x_inteno_owsd_listenobj_whitelist_dhcp(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "whitelist_dhcp_domains", "1");
			else
				dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "whitelist_dhcp_domains", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_owsd_listenobj_origin(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_list *val;

	dmuci_get_value_by_section_list(cur_owsd_listenargs.owsd_listensection, "origin", &val);
	if (val)
		*value = dmuci_list_to_string(val, " ");
	else
		*value = "";
	return 0;
}

int set_x_inteno_owsd_listenobj_origin(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch, *spch;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_delete_by_section(cur_owsd_listenargs.owsd_listensection, "origin", NULL);
			value = dmstrdup(value);
			pch = strtok_r(value, " ", &spch);
			while (pch != NULL) {
				dmuci_add_list_value_by_section(cur_owsd_listenargs.owsd_listensection, "origin", pch);
				pch = strtok_r(NULL, " ", &spch);
			}
			dmfree(value);
			return 0;
	}
	return 0;
}
////////////////////////SET AND GET ALIAS/////////////////////////////////

int get_x_inteno_owsd_listenobj_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_owsd_listenargs.owsd_listensection, "olistenalias", value);
	return 0;
}

int set_x_inteno_owsd_listenobj_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_owsd_listenargs.owsd_listensection, "olistenalias", value);
			return 0;
	}
	return 0;
}

/***** ADD DEL OBJ *******/
int add_owsd_listen(struct dmctx *ctx, char **instancepara)
{
	char *value;
	char *instance;
	struct uci_section *listen_sec = NULL;

	instance = get_last_instance("owsd", "owsd-listen", "olisteninstance");

	dmuci_add_section("owsd", "owsd-listen", &listen_sec, &value);
	dmuci_set_value_by_section(listen_sec, "ipv6", "on");
	dmuci_set_value_by_section(listen_sec, "whitelist_interface_as_origin", "1");
	dmuci_add_list_value_by_section(listen_sec, "origin", "*");
	*instancepara = update_instance(listen_sec, instance, "olisteninstance");
	return 0;
}

int delete_owsd_listen_instance(struct dmctx *ctx, unsigned char del_action)
{
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	int found = 0;
	switch (del_action) {
		case DEL_INST:
			dmuci_delete_by_section(cur_owsd_listenargs.owsd_listensection, NULL, NULL);
			break;
		case DEL_ALL:
			uci_foreach_sections("owsd", "owsd-listen", s) {
				if (found != 0)
					dmuci_delete_by_section(ss, NULL, NULL);
				ss = s;
				found++;
			}
			if (ss != NULL)
				dmuci_delete_by_section(ss, NULL, NULL);
			return 0;
	}
	return 0;
}
