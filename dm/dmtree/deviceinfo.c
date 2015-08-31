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
#include "dmcommon.h"
#include "deviceinfo.h"

char *get_deviceid_manufacturer()
{
	char *v;
	dmuci_get_option_value_string("cwmp","cpe","manufacturer", &v);
	return v;
}

char *get_deviceid_manufactureroui()
{
	FILE* f = NULL;
	char *v;
	char str[16];
	f = fopen(BASE_MAC_ADDR, "r");
	if (f != NULL)
	{
		fgets(str, 16, f);
		size_t ln = strlen(str);
		if (ln<9) goto not_found;
		str[2] = str[3];
		str[3] = str[4];
		str[4] = str[6];
		str[5] = str[7];
		str[6] = '\0';
		v = dmstrdup(str); // MEM WILL BE FREED IN DMMEMCLEAN
		fclose(f);
		return v;
	}
not_found:
	v = "";
	return v;
}

char *get_deviceid_productclass()
{
	char *v;
	db_get_value_string("hw", "board", "iopVersion", &v);
	return v;
}


char *get_deviceid_serialnumber()
{
	char *v;
	db_get_value_string("hw", "board", "serialNumber", &v);
	return v;
}

char *get_softwareversion()
{
	char *v;
	db_get_value_string("hw", "board", "iopVersion", &v);
	return v;
}

int get_device_manufacturer(char *refparam, struct dmctx *ctx, char **value)
{
	*value = get_deviceid_manufacturer();
	return 0;
}

int get_device_manufactureroui(char *refparam, struct dmctx *ctx, char **value)
{
	*value = get_deviceid_manufactureroui();
	return 0;
}

int get_device_productclass(char *refparam, struct dmctx *ctx, char **value)
{
	*value = get_deviceid_productclass();
	return 0;
}

int get_device_serialnumber(char *refparam, struct dmctx *ctx, char **value)
{
	*value = get_deviceid_serialnumber();
	return 0;
}

int get_device_softwareversion(char *refparam, struct dmctx *ctx, char **value)
{
	*value = get_softwareversion();
	return 0;
}

int get_device_hardwareversion(char *refparam, struct dmctx *ctx, char **value)
{
	db_get_value_string("hw", "board", "hardwareVersion", value);
	return 0;
}

int get_device_routermodel(char *refparam, struct dmctx *ctx, char **value)
{
	db_get_value_string("hw", "board", "routerModel", value);
	return 0;
}

int get_device_info_uptime(char *refparam, struct dmctx *ctx, char **value)
{
	FILE* fp = NULL;
	char *pch;
	char buf[64];
	*value = "0";

	fp = fopen(UPTIME, "r");
	if (fp != NULL) {		
		fgets(buf, 64, fp);
		pch = strtok(buf, ".");
		if (pch)
			*value = dmstrdup(pch); // MEM WILL BE FREED IN DMMEMCLEAN
		fclose(fp);
	}
	return 0;
}

int get_device_devicelog (char *refparam, struct dmctx *ctx, char **value)
{
	*value = "";
	char buff[1024];
	int len = klogctl(3 , buff, 1024); /* read ring buffer */
	if (len <= 0)
		return 0;
	buff[len] = '\0';
	char *p = buff;
	while (*p++) { //TODO to optimize, we can avoid this if the '<' and '>' does not cause problem in the tests.
		if (*p == '<')
			*p = '(';
		else if (*p == '>')
			*p = ')';
	}
	*value = dmstrdup(buff); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_device_specversion(char *refparam, struct dmctx *ctx, char **value)
{
	TRACE();
	dmuci_get_option_value_string("cwmp", "cpe", "specversion", value);
	TRACE();
	if ((*value)[0] == '\0') {
		*value = "1.0";
	}		
	return 0;
}

int get_device_provisioningcode(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "cpe", "provisioning_code", value);
	return 0;
}

int set_device_provisioningcode(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:			
			return 0;
		VALUESET:
			dmuci_set_value("cwmp", "cpe", "provisioning_code", value);
	}			
	return 0;
}


int get_base_mac_addr(char *refparam, struct dmctx *ctx, char **value)
{	
	json_object *res;
	
	dmubus_call("router", "info", UBUS_ARGS{{}}, 0, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "system", 0, "basemac", value, NULL);		
	return 0;
}

int get_catv_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *catv;
	dmuci_get_option_value_string("catv", "catv", "enable", &catv);
	if (strcmp(catv, "on") == 0) {
		*value = "1";
	} 
	else 
		*value = "0";
	return 0;	
}

int set_device_catvenabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	char *stat = "";
	switch (action) {
		VALUECHECK:			
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (b)
				stat = "on";
			else
				stat = "off";
			dmuci_set_value("catv", "catv", "enable", stat);
	}
	return 0;
}

int entry_method_root_DeviceInfo(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"DeviceInfo.") {
		DMOBJECT(DMROOT"DeviceInfo.", ctx, "0", 0, NULL, NULL, NULL);
		DMPARAM("Manufacturer", ctx, "0", get_device_manufacturer, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("ManufacturerOUI", ctx, "0", get_device_manufactureroui, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("ModelName", ctx, "0", get_device_routermodel, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("ProductClass", ctx, "0", get_device_productclass, NULL, "xsd:boolean", 1, 0, UNDEF, NULL);
		DMPARAM("SerialNumber", ctx, "0", get_device_serialnumber, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("HardwareVersion", ctx, "0", get_device_hardwareversion, NULL, "xsd:boolean", 1, 0, UNDEF, NULL);
		DMPARAM("SoftwareVersion", ctx, "0", get_device_softwareversion, NULL, "xsd:boolean", 1, 0, UNDEF, NULL);
		DMPARAM("UpTime", ctx, "0", get_device_info_uptime, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("DeviceLog", ctx, "0", get_device_devicelog, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("SpecVersion", ctx, "0", get_device_specversion, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("ProvisioningCode", ctx, "1", get_device_provisioningcode, set_device_provisioningcode, "", 1, 0, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_BaseMacAddr", ctx, "0", get_base_mac_addr, NULL, "", 1, 0, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_CATVEnabled", ctx, "1", get_catv_enabled, set_device_catvenabled, "", 1, 0, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
