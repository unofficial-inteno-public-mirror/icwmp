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
#include "x_inteno_se_dropbear.h"

inline int entry_xinteno_dropbear_instance(struct dmctx *ctx, char *idropbear);
inline int entry_xinteno_dropbear(struct dmctx *ctx);
struct dropbear_args cur_dropbear_args = {0};

inline int init_args_dropbear(struct dmctx *ctx, struct uci_section *s)
{
	struct dropbear_args *args = &cur_dropbear_args;
	ctx->args = (void *)args;
	args->dropbear_section = s;
	return 0;
}

/************************************************************************************* 
**** function ****
**************************************************************************************/


int get_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "PasswordAuth", value);
	if ((*value)[0] == '\0' || ((*value)[0] == 'o' && (*value)[1] == 'n') || (*value)[0] == '1') {
		*value = "1";
	}
	else
		*value = "0";
	return 0;
}

int set_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "PasswordAuth", "1");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "PasswordAuth", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "RootPasswordAuth", value);
	if ((*value)[0] == '\0' || ((*value)[0] == 'o' && (*value)[1] == 'n') || (*value)[0] == '1') {
		*value = "1";
	}
	else
		*value = "0";
	return 0;
}

int set_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "RootPasswordAuth", "1");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "RootPasswordAuth", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "Port", value);
	if ((*value)[0] == '\0') {
		*value = "22";
	}
	return 0;
}

int set_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "Port", "22");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "Port", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "RootLogin", value);
	if ((*value)[0] == '\0' || ((*value)[0] == 'o' && (*value)[1] == 'n') || (*value)[0] == '1' ) {
		*value = "1";
	}
	else
		*value = "0";
	return 0;
}

int set_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "RootLogin", "1");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "RootLogin", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "verbose", value);
	if ((*value)[0] == '\0' || (*value)[0] == '0' ) {
		*value = "0";
	}
	else
		*value = "1";
	return 0;
}

int set_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "verbose", "1");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "verbose", "0");
			return 0;
	}
	return 0;
}


int get_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "GatewayPorts", value);
	if ((*value)[0] == '\0' || (*value)[0] == '0' ) {
		*value = "0";
	}
	else
		*value = "1";
	return 0;
}

int set_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "GatewayPorts", "1");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "GatewayPorts", "");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "Interface", value);
	return 0;
}

int set_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "Interface", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "rsakeyfile", value);
	return 0;
}

int set_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "rsakeyfile", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "dsskeyfile", value);
	return 0;
}

int set_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "dsskeyfile", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "SSHKeepAlive", value);
	if ((*value)[0] == '\0') {
		*value = "300";
	}
	return 0;
}

int set_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcmp(value, "300") == 0)
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "SSHKeepAlive", "");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "SSHKeepAlive", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "IdleTimeout", value);
	if ((*value)[0] == '\0') {
		*value = "300";
	}
	return 0;
}

int set_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '0')
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "IdleTimeout", "");
			else
				dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "IdleTimeout", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "BannerFile", value);
	return 0;
}

int set_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, int action, char *value)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "BannerFile", value);
			return 0;
	}
	return 0;
}

////////////////////////SET AND GET ALIAS/////////////////////////////////

int get_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dropbear_args.dropbear_section, "dropbearalias", value);
	return 0;
}

int set_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_dropbear_args.dropbear_section, "dropbearalias", value);
			return 0;
	}
	return 0;
}

/***** ADD DEL OBJ *******/
int add_dropbear_instance(struct dmctx *ctx, char **instancepara)
{
	char *value;
	char *instance;
	struct uci_section *dropbear_sec = NULL;

	instance = get_last_instance("dropbear", "dropbear", "dropbearinstance");
	dmuci_add_section("dropbear", "dropbear", &dropbear_sec, &value);
	dmuci_set_value_by_section(dropbear_sec, "verbose", "0");
	dmuci_set_value_by_section(dropbear_sec, "Port", "22");
	dmuci_set_value_by_section(dropbear_sec, "RootLogin", "1");
	dmuci_set_value_by_section(dropbear_sec, "GatewayPorts", "0");
	dmuci_set_value_by_section(dropbear_sec, "SSHKeepAlive", "300");
	dmuci_set_value_by_section(dropbear_sec, "IdleTimeout", "0");
	*instancepara = update_instance(dropbear_sec, instance, "dropbearinstance");
	return 0;
}

int delete_dropbear_all(struct dmctx *ctx)
{
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	int found = 0;

	uci_foreach_sections("dropbear", "dropbear", s) {
		if (found != 0)
			dmuci_delete_by_section(ss, NULL, NULL);
		ss = s;
		found++;
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	return 0;
}


int delete_dropbear_instance(struct dmctx *ctx)
{
	dmuci_delete_by_section(cur_dropbear_args.dropbear_section, NULL, NULL);
	return 0;
}
/////////////SUB ENTRIES///////////////
inline int entry_xinteno_dropbear(struct dmctx *ctx)
{
	char *idropbear = NULL, *idropbear_last = NULL;
	struct uci_section *s = NULL;

	uci_foreach_sections("dropbear", "dropbear", s) {
		init_args_dropbear(ctx, s);
		idropbear =  handle_update_instance(1, ctx, &idropbear_last, update_instance_alias, 3, s, "dropbearinstance", "dropbearalias");
		SUBENTRY(entry_xinteno_dropbear_instance, ctx, idropbear);
	}
	return 0;
}

//////////////////////////////////////

int entry_method_root_X_INTENO_SE_DROPBEAR(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_Dropbear.") {
		DMOBJECT(DMROOT"X_INTENO_SE_Dropbear.", ctx, "1", 1, add_dropbear_instance, delete_dropbear_all, NULL);
		SUBENTRY(entry_xinteno_dropbear, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_xinteno_dropbear_instance(struct dmctx *ctx, char *idropbear)
{
	IF_MATCH(ctx, DMROOT"X_INTENO_SE_Dropbear.%s.", idropbear) {
		DMOBJECT(DMROOT"X_INTENO_SE_Dropbear.%s.", ctx, "1", 1, NULL, delete_dropbear_instance, NULL, idropbear);
		DMPARAM("Alias", ctx, "1", get_x_inteno_dropbear_alias, set_x_inteno_dropbear_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("PasswordAuth", ctx, "1", get_x_inteno_dropbear_password_auth, set_x_inteno_dropbear_password_auth, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("RootPasswordAuth", ctx, "1", get_x_inteno_dropbear_root_password_auth, set_x_inteno_dropbear_root_password_auth, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Port", ctx, "1", get_x_inteno_dropbear_port, set_x_inteno_dropbear_port, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("RootLogin", ctx, "1", get_x_inteno_dropbear_root_login, set_x_inteno_dropbear_root_login, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("GatewayPorts", ctx, "1", get_x_inteno_dropbear_gateway_ports, set_x_inteno_dropbear_gateway_ports, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Interface", ctx, "1", get_x_inteno_dropbear_interface, set_x_inteno_dropbear_interface, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("rsakeyfile", ctx, "1", get_x_inteno_dropbear_rsakeyfile, set_x_inteno_dropbear_rsakeyfile, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("dsskeyfile", ctx, "1", get_x_inteno_dropbear_dsskeyfile, set_x_inteno_dropbear_dsskeyfile, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SSHKeepAlive", ctx, "1", get_x_inteno_dropbear_ssh_keepalive, set_x_inteno_dropbear_ssh_keepalive, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("IdleTimeout", ctx, "1", get_x_inteno_dropbear_idle_timeout, set_x_inteno_dropbear_idle_timeout, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		//DMPARAM("enable", ctx, "1", get_x_inteno_dropbear_enable, set_x_inteno_dropbear_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("verbose", ctx, "1", get_x_inteno_dropbear_verbose, set_x_inteno_dropbear_verbose, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("BannerFile", ctx, "1", get_x_inteno_dropbear_banner_file, set_x_inteno_dropbear_banner_file, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

