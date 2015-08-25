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
	char str[TAILLE_MAX];
	f = fopen(BASE_MAC_ADDR, "r");
	if (f != NULL)
	{
		fgets(str, TAILLE_MAX, f);
		size_t ln = strlen(str);
		if (ln<9) goto not_found;
		str[2] = str[3];
		str[3] = str[4];
		str[4] = str[6];
		str[5] = str[7];
		str[6] = '\0';
		v = dmstrdup(str);
		fclose(f);
		return v;
	}
not_found:
	v = dmstrdup("");
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
	char wpakey[TAILLE_MAX];
	fp = fopen("UPTIME", "r");
	if (fp != NULL) {		
		fgets(wpakey, TAILLE_MAX, fp);		
		*value = dmstrdup(cut_fx(wpakey, ".", 1));
		fclose(fp);
	}
	return 0;
}

int get_device_devicelog (char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("TOCODE");
	return 0;
}

int get_device_specversion(char *refparam, struct dmctx *ctx, char **value)
{
	TRACE();
	dmuci_get_option_value_string("cwmp", "cpe", "specversion", value);
	TRACE();
	if ((*value)[0] == '\0') {
		dmfree(*value);
		*value = dmstrdup("1.0");
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
			if (string_to_bool(value, &b))
				return FAULT_9007;
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
	DM_ASSERT(res, *value = dmstrdup(""));
	json_select(res, "system", 0, "basemac", value, NULL);		
	return 0;
}

int get_catv_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *catv;
	dmuci_get_option_value_string("catv", "catv", "enable", &catv);
	if (strcmp(catv, "on") == 0) {
		dmfree(catv);
		*value = dmstrdup("1");
	} 
	else if (strcmp(catv, "off") == 0) {
		dmfree(catv);
		*value = dmstrdup("0");
	}
	else 
		*value = dmstrdup("");
	return 0;	
}

int set_device_catvenabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *stat = "";
	switch (action) {
		VALUECHECK:			
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if (value[0] != '\0') {
				if (value[0] == '1')
					stat = dmstrdup("on");
				else 
					stat = dmstrdup("off");
			}
			dmuci_set_value("catv", "catv", "enable", stat);
			//delay_service restart "catv" "1"
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
