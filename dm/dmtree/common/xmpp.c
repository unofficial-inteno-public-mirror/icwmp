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

struct connectionargs cur_connectionargs = {0};
inline int entry_xmpp_connection_instance(struct dmctx *ctx, char *iconnection);

inline int init_args_connection_entry(struct dmctx *ctx, struct uci_section *s)
{
	struct connectionargs *args = &cur_connectionargs;
	ctx->args = (void *)args;
	args->connsection = s;
	return 0;
}

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

int add_xmpp_connection(struct dmctx *ctx, char **instancepara)
{
	struct uci_section *s;
	char *value, *name;
	
	dmuci_add_section("cwmp", "xmpp_connection", &s, &value);
	*instancepara = get_last_instance("cwmp", "xmpp_connection", "connection_instance");
	return 0;
}

int delete_xmpp_connection_all()
{
	int found = 0;
	struct uci_section *s, *ss = NULL;
	
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

int delete_xmpp_connection(struct dmctx *ctx)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	
	dmuci_delete_by_section(connargs->connsection, NULL, NULL);
	return 0;
}

int get_xmpp_connection_nbr_entry(char *refparam, struct dmctx *ctx, char **value)
{
	struct uci_section *s;
	int cnt = 0;

	uci_foreach_sections("cwmp", "xmpp_connection", s) {
		cnt++;
	}
	dmasprintf(value, "%d", cnt); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

inline int entry_xmpp_connection(struct dmctx *ctx)
{
	char *iconnection = NULL, *iconnection_last = NULL;;
	struct uci_section *s = NULL;

	uci_foreach_sections("cwmp", "xmpp_connection", s) {
		init_args_connection_entry(ctx, s);
		iconnection = handle_update_instance(1, ctx, &iconnection_last, update_instance_alias, 3, s, "connection_instance", "connection_instance_alias");
		SUBENTRY(entry_xmpp_connection_instance, ctx, iconnection);
	}
}

int get_connection_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "enable", value);
	return 0;
}

int set_connection_enable(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "enable", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_password(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "password", value);
	return 0;
}

int set_xmpp_connection_password(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "password", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_domain(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "password", value);
	return 0;
}

int set_xmpp_connection_domain(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "password", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_resource(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "resource", value);
	return 0;
}

int set_xmpp_connection_resource(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "resource", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_server_connect_algorithm(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "serveralgorithm", value);
	return 0;
}

int set_xmpp_connection_server_connect_algorithm(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "serveralgorithm", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_keepalive_interval(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "interval", value);
	return 0;
}

int set_xmpp_connection_keepalive_interval(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "interval", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_server_attempts(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "attempt", value);
	return 0;
}

int set_xmpp_connection_server_attempts(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "attempt", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_retry_initial_interval(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "initial_retry_interval", value);
	return 0;
}

int set_xmpp_connection_retry_initial_interval(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "initial_retry_interval", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_retry_interval_multiplier(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "retry_interval_multiplier", value);
	return 0;
}

int set_xmpp_connection_retry_interval_multiplier(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "retry_interval_multiplier", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_retry_max_interval(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "retry_max_interval", value);
	return 0;
}

int set_xmpp_connection_retry_max_interval(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "retry_max_interval", value);
			return 0;
	}
	return 0;
}

int get_xmpp_connection_server_usetls(char *refparam, struct dmctx *ctx, char **value)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;

	dmuci_get_value_by_section_string(connargs->connsection, "usetls", value);
	return 0;
}

int set_xmpp_connection_server_usetls(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(connargs->connsection, "usetls", value);
			return 0;
	}
	return 0;
}

inline int entry_xmpp_connection_instance(struct dmctx *ctx, char *iconnection)
{
	struct connectionargs *connargs = (struct connectionargs *)ctx->args;
	char linker[32] = "xmppc:";
	char *conn_instance;
	dmuci_get_value_by_section_string(connargs->connsection, "connection_instance", &conn_instance);
	strcat(linker, conn_instance);
	
	IF_MATCH(ctx, DMROOT"XMPP.Connection.%s.", iconnection) {
		DMOBJECT(DMROOT"XMPP.Connection.%s.", ctx, "0", 1, NULL, delete_xmpp_connection, linker, iconnection);
		DMPARAM("Enable", ctx, "1", get_connection_enable, set_connection_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		//DMPARAM("Username", ctx, "1", get_xmpp_connection_username, set_xmpp_connection_username, NULL, 0, 1, UNDEF, NULL); //TO CHECK
		DMPARAM("Password", ctx, "1", get_xmpp_connection_password, set_xmpp_connection_password, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Domain", ctx, "1", get_xmpp_connection_domain, set_xmpp_connection_domain, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Resource", ctx, "1", get_xmpp_connection_resource, set_xmpp_connection_resource, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ServerConnectAlgorithm", ctx, "1", get_xmpp_connection_server_connect_algorithm, set_xmpp_connection_server_connect_algorithm, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("KeepAliveInterval", ctx, "1", get_xmpp_connection_keepalive_interval, set_xmpp_connection_keepalive_interval, "xsd:long", 0, 1, UNDEF, NULL);
		DMPARAM("ServerConnectAttempts", ctx, "1", get_xmpp_connection_server_attempts, set_xmpp_connection_server_attempts, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("ServerRetryInitialInterval", ctx, "1", get_xmpp_connection_retry_initial_interval, set_xmpp_connection_retry_initial_interval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("ServerRetryIntervalMultiplier", ctx, "1", get_xmpp_connection_retry_interval_multiplier, set_xmpp_connection_retry_interval_multiplier, "xsd:unsignedInt", 0, 1, UNDEF, NULL);		
		DMPARAM("ServerRetryMaxInterval", ctx, "1", get_xmpp_connection_retry_max_interval, set_xmpp_connection_retry_max_interval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("UseTLS", ctx, "1", get_xmpp_connection_server_usetls, set_xmpp_connection_server_usetls, "xsd:boolean", 0, 1, UNDEF, NULL); //TO CHECK
		//SUBENTRY(entry_xmpp_connection_server, ctx, iconnection);
		return 0;
	}
	return FAULT_9005;
}

int entry_method_root_xmpp(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"XMPP.") {
		DMOBJECT(DMROOT"XMPP.", ctx, "0", 1, NULL, NULL, NULL);
		DMPARAM("ConnectionNumberOfEntries", ctx, "0", get_xmpp_connection_nbr_entry, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"XMPP.Connection.", ctx, "0", 1, add_xmpp_connection, delete_xmpp_connection_all, NULL);
		SUBENTRY(entry_xmpp_connection, ctx);
		return 0;
	}
	return FAULT_9005;
}
