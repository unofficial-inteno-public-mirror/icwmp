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
#include "xmpp.h"

int browsexmpp_connectionInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

char *get_xmpp_server_enable(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "enable", &v);
		return v;
	}
	v = "";	
	return v;
}

char *get_xmpp_username(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "username", &v);
		return v;
	}
	v = "";
	return v;
}

char *get_xmpp_password(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "password", &v);
		return v;
	}
	v = "";
	return v;
}

char *get_xmpp_domain(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "domain", &v);
		return v;
	}
	v = "";	
	return v;
}

char *get_xmpp_resource(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "resource", &v);
		return v;
	}
	v = "";
	return v;
}

char *get_xmpp_keepalive_interval(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "interval", &v);
		return v;
	}
	v = "";	
	return v;
}

char *get_xmpp_connect_attempts(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "attempt", &v);
		return v;
	}
	v = "";	
	return v;
}

char *get_xmpp_connect_initial_retry_interval(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "initial_retry_interval", &v);
		return v;
	}
	v = "";	
	return v;
}

char *get_xmpp_connect_retry_interval_multiplier(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "retry_interval_multiplier", &v);
		return v;
	}
	v = "";	
	return v;
}

char *get_xmpp_connect_retry_max_interval(char *instance)
{
	struct uci_section *s;
	char *v;
	uci_foreach_option_eq("cwmp", "xmpp_connection", "connection_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "retry_max_interval", &v);
		return v;
	}
	v = "";	
	return v;
}

int add_xmpp_connection(char *refparam, struct dmctx *ctx, void *data, char **instancepara)
{
	struct uci_section *s;
	char *value, *name;
	
	dmuci_add_section("cwmp", "xmpp_connection", &s, &value);
	*instancepara = get_last_instance("cwmp", "xmpp_connection", "connection_instance");
	return 0;
}

int delete_xmpp_connection(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	int found = 0;
	struct uci_section *s, *ss = NULL;
	struct connectionargs *connargs;
	struct uci_section *connsection = (struct uci_section *)data;
	
	switch (del_action) {
		case DEL_INST:
			dmuci_delete_by_section(connsection, NULL, NULL);
			return 0;
		case DEL_ALL:
			uci_foreach_sections("cwmp", "xmpp_connection", s) {
					if (found != 0) {
					dmuci_delete_by_section(ss, NULL, NULL);
				}
				ss = s;
				found++;
			}
			if (ss != NULL) {
				dmuci_delete_by_section(ss, NULL, NULL);
			}
			return 0;
	}
	return 0;
}

int get_xmpp_connection_nbr_entry(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *s;
	int cnt = 0;

	uci_foreach_sections("cwmp", "xmpp_connection", s) {
		cnt++;
	}
	dmasprintf(value, "%d", cnt); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_connection_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "enable", value);
	return 0;
}

int set_connection_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "enable", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_password(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "password", value);
	return 0;
}

int set_xmpp_connection_password(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "password", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_domain(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "password", value);
	return 0;
}

int set_xmpp_connection_domain(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "password", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_resource(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "resource", value);
	return 0;
}

int set_xmpp_connection_resource(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "resource", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_server_connect_algorithm(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "serveralgorithm", value);
	return 0;
}

int set_xmpp_connection_server_connect_algorithm(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "serveralgorithm", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_keepalive_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "interval", value);
	return 0;
}

int set_xmpp_connection_keepalive_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "interval", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_server_attempts(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "attempt", value);
	return 0;
}

int set_xmpp_connection_server_attempts(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "attempt", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_retry_initial_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "initial_retry_interval", value);
	return 0;
}

int set_xmpp_connection_retry_initial_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "initial_retry_interval", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_retry_interval_multiplier(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "retry_interval_multiplier", value);
	return 0;
}

int set_xmpp_connection_retry_interval_multiplier(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "retry_interval_multiplier", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_retry_max_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "retry_max_interval", value);
	return 0;
}

int set_xmpp_connection_retry_max_interval(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "retry_max_interval", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_server_usetls(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *connsection = (struct uci_section *)data;

	dmuci_get_value_by_section_string(connsection, "usetls", value);
	return 0;
}

int set_xmpp_connection_server_usetls(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action) 
{
	struct uci_section *connsection = (struct uci_section *)data;
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connsection, "usetls", value);
			return 0;
	}
	return 0;
}

/**************************************************************************
* LINKER
***************************************************************************/
int  get_xmpp_connection_linker(char *refparam, struct dmctx *dmctx, void *data, char *instance, char **linker) {
	char *linker;
	char *conn_instance;
	struct uci_section *connsection = (struct uci_section *)data;

	if (connsection)
	{
		dmuci_get_value_by_section_string(connsection, "connection_instance", &conn_instance);
		dmasprintf(linker,"xmppc:%s", conn_instance);
		return 0;
	}
	else
		*linker = "";
		return 0;
}

/*************************************************************
 * ENTRY METHOD
/*************************************************************/
DMOBJ tXMPPObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"Connection", &DMWRITE, add_xmpp_connection, delete_xmpp_connection, NULL, browsexmpp_connectionInst, NULL, NULL, NULL, tConnectionParams, get_xmpp_connection_linker},
{0}
};

DMLEAF tXMPPParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"ConnectionNumberOfEntries", &DMREAD, DMT_UNINT, get_xmpp_connection_nbr_entry, NULL, NULL, NULL},
{0}
};

DMLEAF tConnectionParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"Enable", &DMWRITE, DMT_BOOL, get_connection_enable, set_connection_enable, NULL, NULL},
//{"Username", &DMWRITE, DMT_STRING, get_xmpp_connection_username, set_xmpp_connection_username, NULL, NULL},
{"Password", &DMWRITE, DMT_STRING, get_xmpp_connection_password, set_xmpp_connection_password, NULL, NULL},
{"Domain", &DMWRITE, DMT_STRING, get_xmpp_connection_domain, set_xmpp_connection_domain, NULL, NULL},
{"Resource", &DMWRITE, DMT_STRING, get_xmpp_connection_resource, set_xmpp_connection_resource, NULL, NULL},
{"ServerConnectAlgorithm", &DMWRITE, DMT_STRING, get_xmpp_connection_server_connect_algorithm, set_xmpp_connection_server_connect_algorithm, NULL, NULL},
{"KeepAliveInterval", &DMWRITE, DMT_LONG, get_xmpp_connection_keepalive_interval, set_xmpp_connection_keepalive_interval, NULL, NULL},
{"ServerConnectAttempts", &DMWRITE, DMT_UNINT, get_xmpp_connection_server_attempts, set_xmpp_connection_server_attempts, NULL, NULL},
{"ServerRetryInitialInterval", &DMWRITE, DMT_UNINT, get_xmpp_connection_retry_initial_interval, set_xmpp_connection_retry_initial_interval, NULL, NULL},
{"ServerRetryIntervalMultiplier", &DMWRITE, DMT_UNINT, get_xmpp_connection_retry_interval_multiplier, set_xmpp_connection_retry_interval_multiplier, NULL, NULL},
{"ServerRetryMaxInterval", &DMWRITE, DMT_UNINT, get_xmpp_connection_retry_max_interval, set_xmpp_connection_retry_max_interval, NULL, NULL},
{"UseTLS", &DMWRITE, DMT_BOOL, get_xmpp_connection_server_usetls, set_xmpp_connection_server_usetls, NULL, NULL},
{0}
};

int browsexmpp_connectionInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *iconnection = NULL, *iconnection_last = NULL;;
	struct uci_section *s = NULL;

	uci_foreach_sections("cwmp", "xmpp_connection", s) {
		iconnection = handle_update_instance(1, dmctx, &iconnection_last, update_instance_alias, 3, s, "connection_instance", "connection_instance_alias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)s, iconnection) == DM_STOP)
			break;
	}
	return 0;
}
