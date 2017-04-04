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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "deviceinfo.h"

struct dev_vcf cur_dev_vcf = {0};

inline int init_args_vcf(struct dmctx *ctx, struct uci_section *s)
{
	struct dev_vcf *args = &cur_dev_vcf;
	ctx->args = (void *)args;
	args->vcf_sec = s;
	return 0;
}

char *get_deviceid_manufacturer()
{
	char *v;
	dmuci_get_option_value_string("cwmp","cpe","manufacturer", &v);
	return v;
}

char *get_deviceid_manufactureroui()
{
	char *v;
	char str[16];
	char *mac = NULL;	
	json_object *res;

	
	dmubus_call("router.system", "info", UBUS_ARGS{{}}, 0, &res);
	if(!(res)) goto not_found;
	json_select(res, "system", 0, "basemac", &mac, NULL);
	if(mac)
	{
		size_t ln = strlen(mac);
		if (ln<17) goto not_found;
		sscanf (mac,"%2c:%2c:%2c",str,str+2,str+4);
		str[6] = '\0';
		v = dmstrdup(str); // MEM WILL BE FREED IN DMMEMCLEAN
		return v;
	}
not_found:
	v = "";
	return v;
}

char *get_deviceid_productclass()
{
	char *v, *tmp, *val;
	char delimiter[] = "_";
	
	db_get_value_string("hw", "board", "iopVersion", &v);
	tmp = dmstrdup(v);// MEM WILL BE FREED IN DMMEMCLEAN
	val = cut_fx(tmp, delimiter, 1);
	return val;
}


char *get_deviceid_serialnumber()
{
	char *v;
	db_get_value_string("hw", "board", "serialNumber", &v);
	return v;
}

char *get_softwareversion()
{
	char *v, *tmp, *val;
	char delimiter[] = "_";
	
	db_get_value_string("hw", "board", "iopVersion", &v);
	tmp = dmstrdup(v);// MEM WILL BE FREED IN DMMEMCLEAN
	val = cut_fx(tmp, delimiter, 2);
	return val;
}

int get_device_manufacturer(char *refparam, struct dmctx *ctx, char **value)
{
	*value = get_deviceid_manufacturer();
	return 0;
}

int get_device_manufactureroui(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("cwmp", "cpe", "override_oui", value);
	if (*value[0] == '\0')
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
	char *pch, *spch;
	char buf[64];
	*value = "0";

	fp = fopen(UPTIME, "r");
	if (fp != NULL) {		
		fgets(buf, 64, fp);
		pch = strtok_r(buf, ".", &spch);
		if (pch)
			*value = dmstrdup(pch); // MEM WILL BE FREED IN DMMEMCLEAN
		fclose(fp);
	}
	return 0;
}

int get_device_devicelog(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "";
	int i = 0, nbrlines = 4;
	char buff[512], *msg = NULL;
	int len = klogctl(3 , buff, sizeof(buff) - 1); /* read ring buffer */
	if (len <= 0)
		return 0;
	buff[len] = '\0';
	char *p = buff;
	while (*p) { //TODO to optimize, we can avoid this if the '<' and '>' does not cause problem in the tests.
		if (*p == '<') {
			*p = '(';
			if (p == buff || *(p-1) == '\n') {
				if(msg == NULL) msg = p;
				i++;
				if (i == nbrlines) {
					*(p-1) = '\0';
					break;
				}
			}
		}
		else if (*p == '>')
			*p = ')';
		p++;
	}
	if(msg == NULL)
		*value = "";
	else
		*value = dmstrdup(msg);// MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_device_specversion(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "1.0";
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
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value("cwmp", "cpe", "provisioning_code", value);
			return 0;
	}
	return 0;
}


int get_base_mac_addr(char *refparam, struct dmctx *ctx, char **value)
{	
	json_object *res;
	
	dmubus_call("router.system", "info", UBUS_ARGS{{}}, 0, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "system", 0, "basemac", value, NULL);
	return 0;
}

int get_device_memory_bank(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("router.system", "memory_bank", UBUS_ARGS{{}}, 0, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "code", 0, NULL, value, NULL);
	return 0;
}

int set_device_memory_bank(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmubus_call_set("router.system", "memory_bank", UBUS_ARGS{{"bank", value, Integer}}, 1);
			return 0;
	}
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
	bool b;
	char *stat;
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				stat = "on";
			else
				stat = "off";
			dmuci_set_value("catv", "catv", "enable", stat);
			return 0;
	}
	return 0;
}

int get_catv_optical_input_level(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *str;
	*value = "";
	dmubus_call("catv", "vpd", UBUS_ARGS{}, 0, &res);
	if (!res)
		return 0;
	json_select(res, "VPD", -1, NULL, value, NULL);
	return 0;
}

int get_catv_rf_output_level(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *str;
	*value = "";
	dmubus_call("catv", "rf", UBUS_ARGS{}, 0, &res);
	if (!res)
		return 0;
	json_select(res, "RF", -1, NULL, value, NULL);
	return 0;
}

int get_catv_temperature(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *str;
	*value = "";
	dmubus_call("catv", "temp", UBUS_ARGS{}, 0, &res);
	if (!res)
		return 0;
	json_select(res, "Temperature", -1, NULL, value, NULL);
	return 0;
}

int get_catv_voltage(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *str;
	*value = "";
	dmubus_call("catv", "vcc", UBUS_ARGS{}, 0, &res);
	if (!res)
		return 0;
	json_select(res, "VCC", -1, NULL, value, NULL);
	return 0;
}

int get_vcf_name(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dev_vcf.vcf_sec, "name", value);
	return 0;
}

int get_vcf_version(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dev_vcf.vcf_sec, "version", value);
	return 0;
}

int get_vcf_date(char *refparam, struct dmctx *ctx, char **value)
{
	DIR *dir;
	struct dirent *d_file;
	struct stat attr;
	char path[128];
	char date[sizeof "AAAA-MM-JJTHH:MM:SS.000Z"];
	*value = "";
	dmuci_get_value_by_section_string(cur_dev_vcf.vcf_sec, "name", value);
	if ((dir = opendir (DEFAULT_CONFIG_DIR)) != NULL) {
		while ((d_file = readdir (dir)) != NULL) {
			if(strcmp(*value, d_file->d_name) == 0) {
				sprintf(path, "DEFAULT_CONFIG_DIR%s", d_file->d_name);
				stat(path, &attr);
				strftime(date, sizeof date, "%Y-%m-%dT%H:%M:%S.000Z", localtime(&attr.st_mtime));
				*value = dmstrdup(date);
			}
		}
	closedir (dir);
	}
	return 0;
}

int get_vcf_backup_restore(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dev_vcf.vcf_sec, "backup_restore", value);
	return 0;
}

int get_vcf_desc(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dev_vcf.vcf_sec, "description", value);
	return 0;
}

int get_vcf_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_dev_vcf.vcf_sec, "vcf_alias", value);
	return 0;
}

int set_vcf_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_dev_vcf.vcf_sec, "vcf_alias", value);
			return 0;
	}
	return 0;
}

int lookup_vcf_name(char *instance, char **value)
{
	struct uci_section *s = NULL;
	uci_foreach_option_eq("dmmap", "vcf", "vcf_instance", instance, s) {
		dmuci_get_value_by_section_string(s, "name", value);
	}
	return 0;
}

int check_file_dir(char *name)
{
	DIR *dir;
	struct dirent *d_file;
	if ((dir = opendir (DEFAULT_CONFIG_DIR)) != NULL) {
		while ((d_file = readdir (dir)) != NULL) {
			if(strcmp(name, d_file->d_name) == 0) {
				closedir(dir);
				return 1;
			}
		}
	closedir(dir);
	}
	return 0;
}
/*************************************************************
 * ENTRY METHOD
/*************************************************************/

DMLEAF tDeviceInfoParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification, linker*/
{"Manufacturer", &DMREAD, DMT_STRING, get_device_manufacturer, NULL, &DMFINFRM, NULL},
{"ManufacturerOUI", &DMREAD, DMT_STRING, get_device_manufactureroui, NULL, &DMFINFRM, NULL},
{"ModelName", &DMREAD, DMT_STRING, get_device_routermodel, NULL, &DMFINFRM, NULL},
{"ProductClass", &DMREAD, DMT_STRING, get_device_productclass, NULL, &DMFINFRM, NULL},
{"SerialNumber", &DMREAD, DMT_STRING, get_device_serialnumber, NULL,  &DMFINFRM, NULL},
{"HardwareVersion", &DMREAD, DMT_STRING, get_device_hardwareversion, NULL, &DMFINFRM, NULL},
{"SoftwareVersion", &DMREAD, DMT_STRING, get_device_softwareversion, NULL, &DMFINFRM, &DMACTIVE},
{"UpTime", &DMREAD, DMT_UNINT, get_device_info_uptime, NULL, NULL, NULL},
{"DeviceLog", &DMREAD, DMT_STRING, get_device_devicelog, NULL, NULL, NULL},
{"SpecVersion", &DMREAD, DMT_STRING, get_device_specversion, NULL,  &DMFINFRM, NULL},
{"ProvisioningCode", &DMWRITE, DMT_STRING, get_device_provisioningcode, set_device_provisioningcode, &DMFINFRM, &DMACTIVE},
{"X_INTENO_SE_BaseMacAddr", &DMREAD, DMT_STRING, get_base_mac_addr, NULL, NULL, NULL},
{"X_INTENO_SE_CATVEnabled", &DMWRITE, DMT_STRING, get_catv_enabled, set_device_catvenabled, NULL, NULL},
{"X_INTENO_SE_MemoryBank", &DMWRITE, DMT_STRING, get_device_memory_bank, set_device_memory_bank, NULL, NULL},
{0}
};

DMOBJ tDeviceInfoObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf, linker*/
{"X_INTENO_SE_CATV", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tCatTvParams, NULL},
{"VendorConfigFile", &DMREAD, NULL, NULL, NULL, browseVcfInst, NULL, NULL, NULL, tVcfParams, NULL},
{0}
};

DMLEAF tCatTvParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification*/
{"Enabled", &DMWRITE, DMT_STRING, get_catv_enabled, set_device_catvenabled, NULL, NULL},
{"OpticalInputLevel", &DMREAD, DMT_STRING, get_catv_optical_input_level, NULL, NULL, NULL},
{"RFOutputLevel", &DMREAD, DMT_STRING, get_catv_rf_output_level, NULL, NULL, NULL},
{"Temperature", &DMREAD, DMT_STRING, get_catv_temperature, NULL, NULL, NULL},
{"Voltage", &DMREAD, DMT_STRING, get_catv_voltage, NULL, NULL, NULL},
{0}
};

DMLEAF tVcfParams[] = {
/* PARAM, permission, type, getvalue, setvalue, forced_inform, notification*/
{"Alias", &DMWRITE, DMT_STRING, get_vcf_alias, set_vcf_alias, NULL, NULL},
{"Name", &DMREAD, DMT_STRING, get_vcf_name, NULL, NULL, NULL},
{"Version", &DMREAD, DMT_STRING, get_vcf_version, NULL, NULL, NULL},
{"Date", &DMREAD, DMT_TIME, get_vcf_date, NULL, NULL, NULL},
{"Description", &DMREAD, DMT_STRING, get_vcf_desc, NULL, NULL, NULL},
{"UseForBackupRestore", &DMREAD, DMT_BOOL, get_vcf_backup_restore, NULL, NULL, NULL},
{0}
};

int browseVcfInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *vcf = NULL, *vcf_last = NULL, *name;
	struct uci_section *s = NULL, *del_sec = NULL;
	DIR *dir;
	struct dirent *d_file;

	if ((dir = opendir (DEFAULT_CONFIG_DIR)) != NULL) {
		while ((d_file = readdir (dir)) != NULL) {
			if(d_file->d_name[0] == '.')
				continue;
			update_section_list("dmmap","vcf", "name", 1, d_file->d_name, NULL, NULL, "backup_restore", "1");
		}
		closedir (dir);
	}
	uci_foreach_sections("dmmap", "vcf", s) {
		dmuci_get_value_by_section_string(s, "name", &name);
		if(del_sec) {
			dmuci_delete_by_section(del_sec, NULL, NULL);
			del_sec = NULL;
		}
		if (check_file_dir(name) == 0) {
			del_sec = s;
			continue;
		}
		init_args_vcf(dmctx, s);
		vcf = handle_update_instance(1, dmctx, &vcf_last, update_instance_alias, 3, s, "vcf_instance", "vcf_alias");
		DM_LINK_INST_OBJ(dmctx, parent_node, NULL, vcf);
	}
	if(del_sec)
		dmuci_delete_by_section(del_sec, NULL, NULL);

	return 0;
}
