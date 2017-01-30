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
inline int entry_xinteno_owsd_listenobj_instance(struct dmctx *ctx, char *iowsd_listen);
inline int entry_xinteno_owsd_listenobj(struct dmctx *ctx);

inline int init_args_owsd_listen(struct dmctx *ctx, struct uci_section *s)
{
	struct owsd_listenargs *args = &cur_owsd_listenargs;
	ctx->args = (void *)args;
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
		adm_entry_get_linker_param(DMROOT"WANDevice.", linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
		if (*value == NULL) {
			adm_entry_get_linker_param(DMROOT"LANDevice.", linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
			if (*value == NULL)
				*value = "";
		}
		dmfree(linker);
	}
#endif
#ifdef DATAMODEL_TR181
	if (iface[0] != '\0') {
		adm_entry_get_linker_param(DMROOT"IP.Interface.", iface, value); // MEM WILL BE FREED IN DMMEMCLEAN
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
			adm_entry_get_linker_value(value, &linker);
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

int delete_owsd_listen_all(struct dmctx *ctx)
{
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	int found = 0;

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


int delete_owsd_listen_instance(struct dmctx *ctx)
{
	dmuci_delete_by_section(cur_owsd_listenargs.owsd_listensection, NULL, NULL);
	return 0;
}
/////////////SUB ENTRIES///////////////
inline int entry_xinteno_owsd_listenobj(struct dmctx *ctx)
{
	char *iowsd_listen = NULL, *iowsd_listen_last = NULL;
	struct uci_section *s = NULL;

	uci_foreach_sections("owsd", "owsd-listen", s) {
		init_args_owsd_listen(ctx, s);
		iowsd_listen =  handle_update_instance(1, ctx, &iowsd_listen_last, update_instance_alias, 3, s, "olisteninstance", "olistenalias");
		SUBENTRY(entry_xinteno_owsd_listenobj_instance, ctx, iowsd_listen);
	}
	return 0;
}

//////////////////////////////////////

int entry_method_root_X_INTENO_SE_OWSD(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_Owsd.") {
		DMOBJECT(DMROOT"X_INTENO_SE_Owsd.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("Socket", ctx, "1", get_x_inteno_owsd_global_sock, set_x_inteno_owsd_global_sock, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Redirect", ctx, "1", get_x_inteno_owsd_global_redirect, set_x_inteno_owsd_global_redirect, NULL, 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"X_INTENO_SE_Owsd.X_INTENO_SE_ListenObj.", ctx, "1", 1, add_owsd_listen, delete_owsd_listen_all, NULL);
		SUBENTRY(entry_xinteno_owsd_listenobj, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_xinteno_owsd_listenobj_instance(struct dmctx *ctx, char *iowsd_listen)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_Owsd.X_INTENO_SE_ListenObj.%s.", iowsd_listen) {
		DMOBJECT(DMROOT"X_INTENO_SE_Owsd.X_INTENO_SE_ListenObj.%s.", ctx, "1", 1, NULL, delete_owsd_listen_instance, NULL, iowsd_listen);
		DMPARAM("Alias", ctx, "1", get_x_inteno_owsd_listenobj_alias, set_x_inteno_owsd_listenobj_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Port", ctx, "1", get_x_inteno_owsd_listenobj_port, set_x_inteno_owsd_listenobj_port, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("Interface", ctx, "1", get_x_inteno_owsd_listenobj_interface, set_x_inteno_owsd_listenobj_interface, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Ipv6", ctx, "1", get_x_inteno_owsd_listenobj_ipv6_enable, set_x_inteno_owsd_listenobj_ipv6_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Whitelist_interface", ctx, "1", get_x_inteno_owsd_listenobj_whitelist_interface, set_x_inteno_owsd_listenobj_whitelist_interface, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Whitelist_dhcp", ctx, "1", get_x_inteno_owsd_listenobj_whitelist_dhcp, set_x_inteno_owsd_listenobj_whitelist_dhcp, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Origin", ctx, "1", get_x_inteno_owsd_listenobj_origin, set_x_inteno_owsd_listenobj_origin, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

