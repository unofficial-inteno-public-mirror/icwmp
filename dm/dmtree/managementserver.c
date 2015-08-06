/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */
#include <ctype.h>
#include <uci.h>
#include <stdio.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "managementserver.h"
#include "dmcommon.h"

int get_management_server_url(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "url", value);
	return 0;	
}

int set_management_server()
{
	return 0;
}

int set_management_server_url(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *stat = "";
	switch (action) {
		VALUECHECK:			
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("cwmp", "acs", "dhcp_discovery", "disable");
			set_management_server(); //TODO BY IBH
			return 0;
	}
	return 0;
}

int get_management_server_username(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "userid", value);
	return 0;	
}

int set_management_server_username(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server(); //TODO BY IBH
	return 0;	
}

int set_management_server_passwd(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server(); //TODO BY IBH
	return 0;	
}

int get_management_server_key(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "ParameterKey", value);
	return 0;	
}

int set_management_server_key(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server(); //TODO BY IBH
	return 0;	
}

int get_management_server_periodic_inform_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "periodic_inform_enable", value);
	return 0;	
}

int set_management_server_periodic_inform_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server();//TODO BY IBH
	return 0;	
}

int get_management_server_periodic_inform_interval(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "periodic_inform_interval", value);
	return 0;
}

int set_management_server_periodic_inform_interval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server();//TODO BY IBH
	return 0;
}

/*get_management_server_periodic_inform_time() {
	local val=""
	val=`$UCI_GET cwmp.acs.periodic_inform_time`
	if [ "$val" != "0" -a "$val" != "" ];then
		val=`date -d @$val +"%Y-%m-%dT%H:%M:%S.000Z"`
	else
		val="0001-01-01T00:00:00Z"
	fi
	echo "$val"
}
*/
int get_management_server_periodic_inform_time(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "periodic_inform_time", value);
	if ((*value)[0] != '0' && (*value)[0] != '\0') {
		*value = dmstrdup("TOCODE");
	}
	else {
		dmfree(*value);
		*value = dmstrdup("0001-01-01T00:00:00Z");
	}		
	return 0;	
}

int set_management_server_periodic_inform_time(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server(); //TODO BY IBH
	return 0;	
}

//TODO
int get_management_server_connection_request_url(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("TOCODE");
	return 0;
}

int get_management_server_connection_request_username(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "cpe", "userid", value);
	return 0;
}

int set_management_server_connection_request_username(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server();//TODO BY IBH
	return 0;
}

int set_management_server_connection_request_passwd(char *refparam, struct dmctx *ctx, int action, char *value)
{
	set_management_server();//TODO BY IBH
	return 0;
}
/*
get_cache_InternetGatewayDevice_ManagementServer() {
	get_object_cache_generic "InternetGatewayDevice.ManagementServer." "0" "0"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.URL" "1" "\$UCI_GET cwmp.acs.url" "set_management_server_url \$val" "" "cwmp.acs.url"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.Username" "1" "\$UCI_GET cwmp.acs.userid" "set_management_server cwmp.acs.userid \$val" "" "cwmp.acs.userid"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.Password" "1" "" "set_management_server cwmp.acs.passwd \$val" "" "" "" "" "" "" "1"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.ParameterKey" "1" "\$UCI_GET cwmp.acs.ParameterKey" "set_management_server cwmp.acs.ParameterKey \$val" "" "cwmp.acs.ParameterKey" "" "0" "1"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.PeriodicInformEnable" "1" "\$UCI_GET cwmp.acs.periodic_inform_enable" "set_management_server cwmp.acs.periodic_inform_enable \$val" "" "cwmp.acs.periodic_inform_enable" "xsd:boolean"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.PeriodicInformInterval" "1" "\$UCI_GET cwmp.acs.periodic_inform_interval" "set_management_server cwmp.acs.periodic_inform_interval \$val" "" "cwmp.acs.periodic_inform_interval" "xsd:unsignedInt"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.PeriodicInformTime" "1" "get_management_server_periodic_inform_time" "set_management_server_periodic_inform_time \$val" "" "cwmp.acs.periodic_inform_time" "xsd:dateTime"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.ConnectionRequestURL" "0" "get_management_server_connection_request_url" "" "1" "" "" "0" "1" "2"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.ConnectionRequestUsername" "1" "\$UCI_GET cwmp.cpe.userid" "set_management_server cwmp.cpe.userid \$val" "" "cwmp.cpe.userid"
	get_param_cache_generic "InternetGatewayDevice.ManagementServer.ConnectionRequestPassword" "1" "" "set_management_server cwmp.cpe.passwd \$val" "" "" "" "" "" "" "1"
}

*/


int entry_method_root_ManagementServer(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"ManagementServer.") {
		DMOBJECT(DMROOT"ManagementServer.", ctx, "0", 0, NULL, NULL, NULL);
		DMPARAM("URL", ctx, "1", get_management_server_url, set_management_server_url, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("Username", ctx, "1", get_management_server_username, set_management_server_username, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("Password", ctx, "1", get_empty, set_management_server_passwd, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ParameterKey", ctx, "1", get_management_server_key, set_management_server_key, "", 1, 0, UNDEF, NULL);
		DMPARAM("PeriodicInformEnable", ctx, "1", get_management_server_periodic_inform_enable, set_management_server_periodic_inform_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("PeriodicInformInterval", ctx, "1", get_management_server_periodic_inform_interval, set_management_server_periodic_inform_interval, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("PeriodicInformTime", ctx, "1", get_management_server_periodic_inform_time, set_management_server_periodic_inform_time, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ConnectionRequestURL", ctx, "0", get_management_server_connection_request_url, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("ConnectionRequestUsername", ctx, "1", get_management_server_connection_request_username, set_management_server_connection_request_username, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ConnectionRequestPassword", ctx, "1", get_empty, set_management_server_connection_request_passwd, "xsd:boolean", 0, 0, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
