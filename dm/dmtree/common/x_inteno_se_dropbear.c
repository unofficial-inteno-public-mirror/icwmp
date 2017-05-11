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

DMLEAF X_INTENO_SE_DropbearParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, NOTIFICATION, linker*/
{"Alias", &DMWRITE, DMT_STRING, get_x_inteno_dropbear_alias, set_x_inteno_dropbear_alias, NULL, NULL},
{"PasswordAuth", &DMWRITE, DMT_BOOL, get_x_inteno_dropbear_password_auth, set_x_inteno_dropbear_password_auth, NULL, NULL},
{"RootPasswordAuth", &DMWRITE, DMT_BOOL, get_x_inteno_dropbear_root_password_auth, set_x_inteno_dropbear_root_password_auth, NULL, NULL},
{"Port", &DMWRITE, DMT_UNINT, get_x_inteno_dropbear_port, set_x_inteno_dropbear_port, NULL, NULL},
{"RootLogin", &DMWRITE, DMT_BOOL, get_x_inteno_dropbear_root_login, set_x_inteno_dropbear_root_login, NULL, NULL},
{"GatewayPorts", &DMWRITE, DMT_BOOL, get_x_inteno_dropbear_gateway_ports, set_x_inteno_dropbear_gateway_ports, NULL, NULL},
{"Interface", &DMWRITE, DMT_STRING, get_x_inteno_dropbear_interface, set_x_inteno_dropbear_interface, NULL, NULL},
{"rsakeyfile", &DMWRITE, DMT_STRING, get_x_inteno_dropbear_rsakeyfile, set_x_inteno_dropbear_rsakeyfile, NULL, NULL},
{"dsskeyfile", &DMWRITE, DMT_STRING, get_x_inteno_dropbear_dsskeyfile, set_x_inteno_dropbear_dsskeyfile, NULL, NULL},
{"SSHKeepAlive", &DMWRITE, DMT_UNINT, get_x_inteno_dropbear_ssh_keepalive, set_x_inteno_dropbear_ssh_keepalive, NULL, NULL},
{"IdleTimeout", &DMWRITE, DMT_UNINT, get_x_inteno_dropbear_idle_timeout, set_x_inteno_dropbear_idle_timeout, NULL, NULL},
{"verbose", &DMWRITE, DMT_BOOL, get_x_inteno_dropbear_gateway_ports, set_x_inteno_dropbear_gateway_ports, NULL, NULL},
{"BannerFile", &DMWRITE, DMT_STRING, get_x_inteno_dropbear_interface, set_x_inteno_dropbear_interface, NULL, NULL},
{0}
};

int browseXIntenoDropbear(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *idropbear = NULL, *idropbear_last = NULL;
	struct uci_section *s = NULL;

	uci_foreach_sections("dropbear", "dropbear", s) {
		idropbear =  handle_update_instance(1, dmctx, &idropbear_last, update_instance_alias, 3, s, "dropbearinstance", "dropbearalias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)s, idropbear) == DM_STOP)
			break;
	}
	return 0;
}

/************************************************************************************* 
**** function ****
**************************************************************************************/


int get_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "PasswordAuth", value);
	if ((*value)[0] == '\0' || ((*value)[0] == 'o' && (*value)[1] == 'n') || (*value)[0] == '1') {
		*value = "1";
	}
	else
		*value = "0";
	return 0;
}

int set_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
				dmuci_set_value_by_section((struct uci_section *)data, "PasswordAuth", "1");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "PasswordAuth", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "RootPasswordAuth", value);
	if ((*value)[0] == '\0' || ((*value)[0] == 'o' && (*value)[1] == 'n') || (*value)[0] == '1') {
		*value = "1";
	}
	else
		*value = "0";
	return 0;
}

int set_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
				dmuci_set_value_by_section((struct uci_section *)data, "RootPasswordAuth", "1");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "RootPasswordAuth", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "Port", value);
	if ((*value)[0] == '\0') {
		*value = "22";
	}
	return 0;
}

int set_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				dmuci_set_value_by_section((struct uci_section *)data, "Port", "22");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "Port", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "RootLogin", value);
	if ((*value)[0] == '\0' || ((*value)[0] == 'o' && (*value)[1] == 'n') || (*value)[0] == '1' ) {
		*value = "1";
	}
	else
		*value = "0";
	return 0;
}

int set_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
				dmuci_set_value_by_section((struct uci_section *)data, "RootLogin", "1");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "RootLogin", "0");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "verbose", value);
	if ((*value)[0] == '\0' || (*value)[0] == '0' ) {
		*value = "0";
	}
	else
		*value = "1";
	return 0;
}

int set_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
				dmuci_set_value_by_section((struct uci_section *)data, "verbose", "1");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "verbose", "0");
			return 0;
	}
	return 0;
}


int get_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "GatewayPorts", value);
	if ((*value)[0] == '\0' || (*value)[0] == '0' ) {
		*value = "0";
	}
	else
		*value = "1";
	return 0;
}

int set_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
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
				dmuci_set_value_by_section((struct uci_section *)data, "GatewayPorts", "1");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "GatewayPorts", "");
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "Interface", value);
	return 0;
}

int set_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section((struct uci_section *)data, "Interface", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "rsakeyfile", value);
	return 0;
}

int set_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section((struct uci_section *)data, "rsakeyfile", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "dsskeyfile", value);
	return 0;
}

int set_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section((struct uci_section *)data, "dsskeyfile", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "SSHKeepAlive", value);
	if ((*value)[0] == '\0') {
		*value = "300";
	}
	return 0;
}

int set_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcmp(value, "300") == 0)
				dmuci_set_value_by_section((struct uci_section *)data, "SSHKeepAlive", "");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "SSHKeepAlive", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "IdleTimeout", value);
	if ((*value)[0] == '\0') {
		*value = "300";
	}
	return 0;
}

int set_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '0')
				dmuci_set_value_by_section((struct uci_section *)data, "IdleTimeout", "");
			else
				dmuci_set_value_by_section((struct uci_section *)data, "IdleTimeout", value);
			return 0;
	}
	return 0;
}

int get_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "BannerFile", value);
	return 0;
}

int set_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section((struct uci_section *)data, "BannerFile", value);
			return 0;
	}
	return 0;
}

////////////////////////SET AND GET ALIAS/////////////////////////////////

int get_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "dropbearalias", value);
	return 0;
}

int set_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section((struct uci_section *)data, "dropbearalias", value);
			return 0;
	}
	return 0;
}

/***** ADD DEL OBJ *******/
int add_dropbear_instance(char *refparam, struct dmctx *ctx, void *data, char **instancepara)
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

int delete_dropbear_instance(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	int found = 0;
	switch (del_action) {
		case DEL_INST:
			dmuci_delete_by_section((struct uci_section *)data, NULL, NULL);
			break;
		case DEL_ALL:
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
	return 0;
}

