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
#include <time.h>
#include "cwmp.h"
#include "ubus.h"
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "managementserver.h"
#include "xml.h"

int get_management_server_url(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_varstate_string("cwmp", "acs", "url", value);
	return 0;	
}

int set_management_server_url(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "acs", "dhcp_discovery", "disable");
			dmuci_set_value("cwmp", "acs", "url", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
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
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "acs", "userid", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;	
}

int set_management_server_passwd(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "acs", "passwd", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;	
}

int get_management_server_key(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "ParameterKey", value);
	return 0;	
}

int get_management_server_periodic_inform_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "periodic_inform_enable", value);
	return 0;	
}

int set_management_server_periodic_inform_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;

	switch (action) {
		case VALUECHECK:			
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value("cwmp", "acs", "periodic_inform_enable", "1");
			else
				dmuci_set_value("cwmp", "acs", "periodic_inform_enable", "0");
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;	
}

int get_management_server_periodic_inform_interval(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "periodic_inform_interval", value);
	return 0;
}

int set_management_server_periodic_inform_interval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "acs", "periodic_inform_interval", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int get_management_server_periodic_inform_time(char *refparam, struct dmctx *ctx, char **value)
{
	time_t time_value;
	
	dmuci_get_option_value_string("cwmp", "acs", "periodic_inform_time", value);
	if ((*value)[0] != '0' && (*value)[0] != '\0') {
		time_value = atoi(*value);
		char s_now[sizeof "AAAA-MM-JJTHH:MM:SS.000Z"];
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S.000Z", localtime(&time_value));
		*value = dmstrdup(s_now); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	else {
		*value = "0001-01-01T00:00:00Z";
	}		
	return 0;	
}

int set_management_server_periodic_inform_time(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct tm tm;
	char *p, buf[16];
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			if (!(strptime(value, "%Y-%m-%dT%H:%M:%S", &tm))) {
				return 0;
			}
			sprintf(buf, "%d", mktime(&tm));
			dmuci_set_value("cwmp", "acs", "periodic_inform_time", buf);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;	
}

int get_management_server_connection_request_url(char *refparam, struct dmctx *ctx, char **value)
{
	char *ip, *port, *iface;

	*value = "";
	dmuci_get_option_value_string("cwmp", "cpe", "default_wan_interface", &iface);
	network_get_ipaddr(&ip, iface);	
	dmuci_get_option_value_string("cwmp", "cpe", "port", &port);
	if (ip[0] != '\0' && port[0] != '\0') {
		char buf[64];
		sprintf(buf,"http://%s:%s/", ip, port);
		*value = dmstrdup(buf); //  MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
}

int get_management_server_connection_request_username(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "cpe", "userid", value);
	return 0;
}

int set_management_server_connection_request_username(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "cpe", "userid", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int set_management_server_connection_request_passwd(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "cpe", "passwd", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int get_lwn_protocol_supported(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "UDP";
	return 0;
}

int get_lwn_protocol_used(char *refparam, struct dmctx *ctx, char **value)
{
	bool b;
	char *tmp;
	
	dmuci_get_option_value_string("cwmp", "lwn", "enable", &tmp);
	string_to_bool(tmp, &b);
	if (b)
		*value = "UDP";
	else	
		*value = "";
	return 0;
}

int set_lwn_protocol_used(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (strcmp(value,"UDP") ==0) {
				dmuci_set_value("cwmp", "lwn", "enable", "1");
				cwmp_set_end_session(END_SESSION_RELOAD);
			} 
			else {
				dmuci_set_value("cwmp", "lwn", "enable", "0");
				cwmp_set_end_session(END_SESSION_RELOAD);
			}
			return 0;
	}
	return 0;
}

int get_lwn_host(char *refparam, struct dmctx *ctx, char **value)
{	
	dmuci_get_option_value_string("cwmp", "lwn", "hostname", value);
	return 0;
}

int set_lwn_host(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "lwn", "hostname", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int get_lwn_port(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "lwn", "port", value);
	return 0;
}

int set_lwn_port(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "lwn", "port", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int get_management_server_http_compression_supportted(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "GZIP,Deflate";
	return 0;
}

int get_management_server_http_compression(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "compression", value);
	return 0;
}

int set_management_server_http_compression(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			 if (0 == strcasecmp(value, "gzip") || 0 == strcasecmp(value, "deflate") || 0 == strncasecmp(value, "disable", 7)) {
				 return 0;
			 }
			return FAULT_9007;
		case VALUESET:
			dmuci_set_value("cwmp", "acs", "compression", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int get_management_server_retry_min_wait_interval(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "retry_min_wait_interval", value);
	return 0;
}

int set_management_server_retry_min_wait_interval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int a;
	switch (action) {
		case VALUECHECK:
			a = atoi(value);
			if (a <= 65535 && a >= 1) {
				 return 0;
			}
			return FAULT_9007;
		case VALUESET:
			dmuci_set_value("cwmp", "acs", "retry_min_wait_interval", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int get_management_server_retry_interval_multiplier(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "acs", "retry_interval_multiplier", value);
	return 0;
}

int set_management_server_retry_interval_multiplier(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int a;
	switch (action) {
		case VALUECHECK:
			a = atoi(value);
			if (a <= 65535 && a >= 1000) {
				 return 0;
			}
			return FAULT_9007;
		case VALUESET:
			dmuci_set_value("cwmp", "acs", "retry_interval_multiplier", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int get_alias_based_addressing(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "cpe", "amd_version", value);
	if((*value)[0] == '\0'|| atoi(*value) <= AMD_4) {
		*value = "false";
	}
	else {
		*value = "true";
	}
	return 0;
}

int get_instance_mode(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "cpe", "instance_mode", value);
	return 0;
}

int set_instance_mode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			if (0 == strcmp(value, "InstanceNumber") || 0 == strcmp(value, "InstanceAlias") ) {
				return 0;
			}
			return FAULT_9007;
		case VALUESET:
			dmuci_set_value("cwmp", "cpe", "instance_mode", value);
			cwmp_set_end_session(END_SESSION_RELOAD);
			return 0;
	}
	return 0;
}

int entry_method_root_ManagementServer(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"ManagementServer.") {
		DMOBJECT(DMROOT"ManagementServer.", ctx, "0", 0, NULL, NULL, NULL);
		DMPARAM("URL", ctx, "1", get_management_server_url, set_management_server_url, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Username", ctx, "1", get_management_server_username, set_management_server_username, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Password", ctx, "1", get_empty, set_management_server_passwd, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ParameterKey", ctx, "0", get_management_server_key, NULL, NULL, 1, 0, 0, NULL);
		DMPARAM("PeriodicInformEnable", ctx, "1", get_management_server_periodic_inform_enable, set_management_server_periodic_inform_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("PeriodicInformInterval", ctx, "1", get_management_server_periodic_inform_interval, set_management_server_periodic_inform_interval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PeriodicInformTime", ctx, "1", get_management_server_periodic_inform_time, set_management_server_periodic_inform_time, "xsd:dateTime", 0, 1, UNDEF, NULL);
		DMPARAM("ConnectionRequestURL", ctx, "0", get_management_server_connection_request_url, NULL, NULL, 1, 0, 2, NULL);
		DMPARAM("ConnectionRequestUsername", ctx, "1", get_management_server_connection_request_username, set_management_server_connection_request_username, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ConnectionRequestPassword", ctx, "1", get_empty, set_management_server_connection_request_passwd, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("HTTPCompressionSupported", ctx, "0", get_management_server_http_compression_supportted, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("HTTPCompression", ctx, "1", get_management_server_http_compression, set_management_server_http_compression, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LightweightNotificationProtocolsSupported", ctx, "0", get_lwn_protocol_supported, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LightweightNotificationProtocolsUsed", ctx, "1", get_lwn_protocol_used, set_lwn_protocol_used, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("UDPLightweightNotificationHost", ctx, "1", get_lwn_host, set_lwn_host, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("UDPLightweightNotificationPort", ctx, "1", get_lwn_port, set_lwn_port, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("CWMPRetryMinimumWaitInterval", ctx, "1", get_management_server_retry_min_wait_interval, set_management_server_retry_min_wait_interval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("CWMPRetryIntervalMultiplier", ctx, "1", get_management_server_retry_interval_multiplier, set_management_server_retry_interval_multiplier, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("AliasBasedAddressing", ctx, "0", get_alias_based_addressing, NULL, "xsd:boolean", 1, 1, UNDEF, NULL);
		DMPARAM("InstanceMode", ctx, "1", get_instance_mode, set_instance_mode, NULL, 0, 1, UNDEF, NULL);

		return 0;
	}
	return FAULT_9005;
}
