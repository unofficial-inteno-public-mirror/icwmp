/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *		Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *
 */

#include <uci.h>
#include <stdio.h>
#include <ctype.h>
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "x_inteno_se_wifi.h"

int browsesewifiradioInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

int get_wifi_frequency(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *freq;
	json_object *res;
	struct uci_section *sewifisection = (struct uci_section *)data;
	char *wlan_name = section_name(sewifisection);
	
	dmubus_call("router.wireless", "status", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "frequency", 0, NULL, &freq, NULL);
	dmastrcat(value, freq, "GHz");  // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_wifi_maxassoc(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *sewifisection = (struct uci_section *)data;
	
	dmuci_get_value_by_section_string(sewifisection, "maxassoc", value);
	return 0;
}

int set_wifi_maxassoc(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct uci_section *sewifisection = (struct uci_section *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sewifisection, "maxassoc", value);
			return 0;
	}
	return 0;
}

int get_wifi_dfsenable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *val;
	struct uci_section *sewifisection = (struct uci_section *)data;
	char *wlan_name = section_name(sewifisection);
	*value = "";
	
	dmuci_get_value_by_section_string(sewifisection, "band", &val);
	if (val[0] == 'a') {
		dmuci_get_value_by_section_string(sewifisection, "dfsc", value);
		if ((*value)[0] == '\0')
			*value = "0";
	}
	return 0;
}

int set_wifi_dfsenable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{	
	bool b;
	char *val;
	struct uci_section *sewifisection = (struct uci_section *)data;
	char *wlan_name = section_name(sewifisection);
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(sewifisection, "band", &val);
			if (val[0] == 'a') {
				string_to_bool(value, &b);
				if (b)
					dmuci_set_value_by_section(sewifisection, "dfsc", "1");
				else
					dmuci_set_value_by_section(sewifisection, "dfsc", "0");
			}
			return 0;
	}
	return 0;
}

////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_radio_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string((struct uci_section *)data, "radioalias", value);
	return 0;
}

int set_radio_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section((struct uci_section *)data, "radioalias", value);
			return 0;
	}
	return 0;
}
/////////////SUB ENTRIES///////////////
DMLEAF tsewifiradioParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_radio_alias, set_radio_alias, NULL, NULL},
{"Frequency", &DMREAD, DMT_STRING, get_wifi_frequency, NULL, NULL, NULL},
{"MaxAssociations", &DMWRITE, DMT_STRING, get_wifi_maxassoc, set_wifi_maxassoc, NULL, NULL},
{"DFSEnable", &DMWRITE, DMT_BOOL, get_wifi_dfsenable, set_wifi_dfsenable, NULL, NULL},
{0}
};

DMOBJ tsewifiObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"Radio", &DMREAD, NULL, NULL, NULL, browsesewifiradioInst, NULL, NULL, NULL, tsewifiradioParam, NULL},
{0}
};

int browsesewifiradioInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *wnum = NULL, *wnum_last = NULL;
	struct uci_section *s = NULL;
	uci_foreach_sections("wireless", "wifi-device", s) {
		wnum =  handle_update_instance(1, dmctx, &wnum_last, update_instance_alias, 3, s, "radioinstance", "radioalias");
		//SUBENTRY(entry_sewifi_radio_instance, ctx, wnum);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)s, wnum) == DM_STOP)
			break;
	}
	return 0;
}
