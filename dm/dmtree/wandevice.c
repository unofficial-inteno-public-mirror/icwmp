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
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "wandevice.h"
#include "landevice.h"

#define WAN_DEVICE 3
#define WAN_INST_ETH 1
#define WAN_INST_ATM 2
#define WAN_INST_PTM 3
#define DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP "" //ASK KMD

struct wan_device 
{
	char *instance;
	char *fdev;
	char *cdev;	
};

struct wan_device wan_devices[WAN_DEVICE] = {{"1", "eth0", "ethernet"}, {"2", "atm", "adsl"}, {"3", "ptm", "vdsl"}};

char *default_wan;

struct wanargs
{
	int instance;
	char *fdev;
};

struct wanargs cur_wanargs = {0};
struct wancdevargs
{
	char *fwan;
};

struct wancdevargs cur_wancdevargs = {0};

struct wancprotoargs
{
	struct uci_section *wancprotosection;
	struct uci_ptr *ptr;
	
};

struct wancprotoargs cur_wancprotoargs = {0};

inline int init_wanargs(struct dmctx *ctx, char *idev, char *fdev)
{
	struct wanargs *args = &cur_wanargs;
	ctx->args = (void *)args;
	args->instance = atoi(idev);
	args->fdev = dmstrdup(fdev);
	return 0;
}

inline int init_wancprotoargs(struct dmctx *ctx, struct uci_section *s)
{
	struct wancprotoargs *args = &cur_wancprotoargs;
	ctx->args = (void *)args;
	args->wancprotosection = s;
	return 0;
}

inline int init_wancdevargs(struct dmctx *ctx, char *fwan)
{
	struct wancdevargs *args = &cur_wancdevargs;
	ctx->args = (void *)args;
	args->fwan = dmstrdup(fwan);
	return 0;
}

//TO CHECK
int network_get_ipaddr(char **value, char *iface)
{
	json_object *res;
	
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", iface}}, 0, &res);
	DM_ASSERT(res, *value = dmstrdup(""));
	json_select(res, "ipv4-address", 0, "address", value, NULL);
	return 0;
}

/************************************************************************** 
**** ****  function related to get_wandevice_wandevice_parameters  **** ****
***************************************************************************/

char *get_wan_device_wan_dsl_traffic()
{
	json_object *res;
	char *dsl = NULL;
	dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
	if (!res) 
		return dmstrdup("");
	json_select(res, "dslstats", -1, "traffic", &dsl, NULL);
	if (dsl) {
		if (strstr(dsl, "ATM")) {
			dsl = dmstrdup("adsl");
			goto end;
		}
		else if (strstr(dsl, "PTM")) {
			dsl = dmstrdup("vdsl");
			goto end;
		}
	}
	return dmstrdup("");
end:
	return dsl;
}

int get_wan_device_wan_access_type(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	switch(wandargs->instance) {
		case WAN_INST_ETH:
			*value = dmstrdup("Ethernet");
			break;
		case WAN_INST_ATM:
			*value = dmstrdup("DSL");
			break;
		case WAN_INST_PTM:
			*value = dmstrdup("DSL");
			break;
	}
	return 0;
}

int get_wan_device_wan_dsl_interface_config_status(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	
	char *status, *dsl;
	json_object *res;
	if (wandargs->instance == WAN_INST_ETH)
		*value = dmstrdup("NoSignal Not a dsl interface");
	else {
		json_object *res = NULL;
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup(""));
		json_select(res, "dslstats", -1, "status", &status, NULL);
		TRACE("get_wan_device_wan_dsl_interface_config_status after select %s \n", status);
		if (strcmp(status, "Showtime") == 0)
			*value = dmstrdup("Up");
		else if (strcmp(status, "Training") == 0)
			*value = dmstrdup("Initializing");
		else if (strcmp(status, "Channel Analysis") == 0)
			*value = dmstrdup("EstablishingLink");
		else if (strcmp(status, "Disabled") == 0)
			*value = dmstrdup("Disabled");
		else 
			*value = dmstrdup("NoSignal");
		dsl = get_wan_device_wan_dsl_traffic();	
		if (wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) {
			dmfree(status);
			dmfree(dsl);
			goto end;
		}
		else if (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0) {
			dmfree(status);
			dmfree(dsl);
			goto end;
		}
		else {
			dmfree(status);
			dmfree(dsl);
			dmfree(*value); //TO CHECK
			*value = dmstrdup("");
			goto end;
		}
	}	
end:
	return 0;
}

int get_wan_device_wan_dsl_interface_config_modulation_type(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *mode, *dsl;
	json_object *res = NULL;
	if (wandargs->instance == WAN_INST_ETH)
		*value = dmstrdup("Not a dsl interface");
	else {		
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup(""));
		json_select(res, "dslstats", -1, "mode", &mode, NULL);
		TRACE("get_wan_device_wan_dsl_interface_config_modulation_type %s\n", mode);
		if (strcmp(mode, "G.Dmt") == 0)
			*value = dmstrdup("ADSL_G.dmt");
		else if (strcmp(mode, "G.lite") == 0)
			*value = dmstrdup("ADSL_G.lite");
		else if (strcmp(mode, "T1.413") == 0)
			*value = dmstrdup("ADSL_ANSI_T1.413");
		else if (strcmp(mode, "ADSL2") == 0)
			*value = dmstrdup("ADSL_G.dmt.bis");
		else if (strcmp(mode, "AnnexL") == 0)
			*value = dmstrdup("ADSL_re-adsl");
		else if (strcmp(mode, "ADSL2+") == 0)
			*value = dmstrdup("ADSL_2plus");
		else
			*value = dmstrdup(mode);
		dsl = get_wan_device_wan_dsl_traffic();
		if (wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) {
			dmfree(mode);
			dmfree(dsl);
			goto end;
		}
		else if (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0) {
			dmfree(mode);
			dmfree(dsl);
			goto end;
		}
		else {
			dmfree(mode);
			dmfree(dsl);
			dmfree(*value); //TO CHECK
			*value = dmstrdup("");
			goto end;
		}
	}	
end:
	return 0;
}

int get_wan_device_dsl_datapath(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *val = "";
	char *dsl;
	if (wandargs->instance == WAN_INST_ETH)
		*value = dmstrdup("None");
	else {
		val = dmstrdup("`adsl info --state|grep \"Upstream rate\"| awk -F'['$'\t'' ,]' '$1!=\"Max:\" {print $2}'`"); //TODO IS THER AN EQUIVALENT UBUS CMD
		if (strcmp(val, "FAST")) {
			dmfree(val);
			dmfree(*value);
			*value = dmstrdup("Fast");
		}
		else if (strcmp(val, "INTR")) {
			dmfree(val);
			dmfree(*value); 
			*value = dmstrdup("Interleaved");
		}
		else {
			dmfree(val);
			dmfree(*value); 
			*value = dmstrdup("None");
		}
		dsl = get_wan_device_wan_dsl_traffic();
		if (wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) {
			dmfree(dsl);
			goto end;
		}
		else if (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0) {
			dmfree(dsl);
			goto end;
		}
		else {
			dmfree(dsl);
			dmfree(*value); //TO CHECK
			*value = dmstrdup("None");
			goto end;
		}
	}
end:
	return 0;
}

int get_wan_device_dsl_downstreamcurrrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *rate_down, *dsl;
	json_object *res = NULL;
	json_object *sub_obj= NULL;
	*value = dmstrdup("0");
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup(""));
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);	
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "rate_down", &rate_down, NULL);
		else 
			return 0;
		TRACE("get_wan_device_dsl_downstreamcurrrate rate down %s", rate_down);
		dsl = get_wan_device_wan_dsl_traffic();
		if (((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
			|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0)) && rate_down) {
			if(rate_down[0] != '\0') {
				dmfree(*value);
				*value = rate_down;
			}
			dmfree(dsl);
		}
		else {
			if (rate_down)
				dmfree(rate_down);
			dmfree(dsl);
		}
	}
	return 0;
}

int get_wan_device_dsl_downstreammaxrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *max_down, *dsl;
	json_object *res = NULL;
	json_object *sub_obj = NULL;
	*value = dmstrdup("0");
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup(""));
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);	
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "max_rate_down", &max_down, NULL);
		else 
			return 0;
		dsl = get_wan_device_wan_dsl_traffic();
		if (((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
			|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0)) && max_down) {
			if (max_down != '\0') {
				dmfree(*value);
				*value = max_down;
			}
			dmfree(dsl);
		}
		else {
			if (max_down)
				dmfree(max_down);
			dmfree(dsl);
		}
	}
	return 0; 	
}

int get_wan_device_dsl_downstreamattenuation(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *attn_down_x100, *dsl;
	json_object *res = NULL;
	*value = dmstrdup("0");
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup("0"));
		json_select(res, "dslstats", -1, "attn_down_x100", &attn_down_x100, NULL);
		if (attn_down_x100) {
			dmasprintf(&attn_down_x100, "%d", (atoi(attn_down_x100) % 10));
			//sprintf(&attn_down_x100, "%d", atoi(attn_down_x100) %10);
			dsl = get_wan_device_wan_dsl_traffic();
			if ((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
				|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0)) {
				dmfree(*value);
				*value = attn_down_x100;
				dmfree(dsl);
			}
			//dmfree(attn_down_x100);
		}
	}
	return 0; 	
}

int get_wan_device_dsl_downstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *snr_down_x100, *dsl;
	json_object *res;
	*value = dmstrdup("0");
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup("0"));
		json_select(res, "dslstats", -1, "snr_down_x100", &snr_down_x100, NULL);
		if (snr_down_x100) {
			dmasprintf(&snr_down_x100, "%d", (atoi(snr_down_x100) % 10));
		//sprintf(&snr_down_x100, "%d", atoi(snr_down_x100) %10);
			dsl = get_wan_device_wan_dsl_traffic();
			if (((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
			|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0))) {
				dmfree(*value);
				*value = snr_down_x100;
				dmfree(dsl);
			}
			//dmfree(snr_down_x100);
		}		
	}
	return 0;
}

int get_wan_device_dsl_upstreamcurrrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *rate_up, *dsl;
	json_object *res = NULL;
	json_object *sub_obj = NULL;
	*value = dmstrdup("0");
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup(""));
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);	
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "rate_up", &rate_up, NULL);
		else 
			return 0;
		dsl = get_wan_device_wan_dsl_traffic();
		if (((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
			|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0)) && rate_up) {
			if(rate_up[0] != '\0') {
				dmfree(*value);
				*value = rate_up;
			}
			dmfree(dsl);
		}
		else {
			if (rate_up)
				dmfree(rate_up);
			dmfree(dsl);
		}
	}
	return 0;
}

int get_wan_device_dsl_upstreammaxrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *max_up, *dsl;
	json_object *res = NULL; 
	json_object *sub_obj = NULL;
	*value = dmstrdup("0");
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup(""));
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "max_rate_up", &max_up, NULL);
		else 
			return 0;
		dsl = get_wan_device_wan_dsl_traffic();
		if (((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
			|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0)) && max_up) {
			if (max_up != '\0') {
				dmfree(*value);
				*value = max_up;
			}
			dmfree(dsl);
		}
		else {
			if (max_up)
				dmfree(max_up);
			dmfree(dsl);
		}
	}
	return 0;
}

int get_wan_device_dsl_upstreamattenuation(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *attn_up_x100, *dsl;
	json_object *res = NULL;
	*value = dmstrdup("0");
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup(""));
		json_select(res, "dslstats", -1, "attn_up_x100", &attn_up_x100, NULL);
		if (attn_up_x100) {
			dmasprintf(&attn_up_x100, "%d", (atoi(attn_up_x100) % 10));
			//sprintf(&attn_up_x100, "%d", atoi(attn_up_x100) %10);
			dsl = get_wan_device_wan_dsl_traffic();
			if ((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
				|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0)) {
				dmfree(*value);
				*value = attn_up_x100;
				dmfree(dsl);
			}
			//dmfree(attn_up_x100);
		}
	}
	return 0; 	
}

int get_wan_device_dsl_upstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *snr_up_x100, *dsl;
	json_object *res;
	if (wandargs->instance == WAN_INST_ETH) {
		*value = dmstrdup("0");
		return 0;
	}	
	else {
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = dmstrdup("0"));
		json_select(res, "dslstats", -1, "snr_up_x100", &snr_up_x100, NULL);
		if (snr_up_x100) {
			dmasprintf(&snr_up_x100, "%d", (atoi(snr_up_x100) % 10));
		//sprintf(&snr_up_x100, "%d", atoi(snr_up_x100) %10);
			dsl = get_wan_device_wan_dsl_traffic();
			if (((wandargs->instance == WAN_INST_ATM && strcmp(dsl, "adsl") == 0) 
			|| (wandargs->instance == WAN_INST_PTM && strcmp(dsl, "vdsl") == 0))) {
				//dmfree(*value);
				*value = snr_up_x100;
				dmfree(dsl);
			}
			//dmfree(snr_up_x100);
		}
		else {
			*value = dmstrdup("0");
		}
	}
	return 0;
}

int get_annexm_status(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	if (wandargs->instance == WAN_INST_ATM) {
		dmuci_get_option_value_string("layer2_interface", "capabilities", "AnnexM", value);	
	}
	if (*value) {
		TRACE("get_annexm_status in if\n");
		if (strcasecmp(*value, "enabled") == 0) {
			//dmfree(value);
			*value = dmstrdup("1");
			return 0;
		} 
		else {
			//dmfree(*value);
			goto end;
		}	
	}
end:
	*value = dmstrdup("0");	
	return 0;
}

int set_annexm_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
		
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (wandargs->instance == WAN_INST_ATM) {
				if (b)
					value = dmstrdup("Enabled");
				else
					value[0] = '\0';
			}
			dmuci_set_value("layer2_interface", "capabilities", "AnnexM", value);
			//delay_service restart "layer2_interface" "1" //TODO			
	}	
	return 0;
}

/*get_wan_eth_intf_enable() {
	local fdev="$1"
	local val
	json_load "$(devstatus $fdev)"
	json_get_var val up
	[ "$val" != "true" -a "$val" != "1" ] && val="false"
	echo $val
}*/

//TO CHECK IF NO VALUE RETURNE BY UBUS CMD
int get_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	//dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", wandargs->fdev}}, 1, &res);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup("false"));
	json_select(res, "up", -1, NULL, value, NULL);
	TRACE("get_wan_eth_intf_enable \n");
	if (*value) {
		if (strcmp(*value, "true") != 0 || (*value)[0] != '1') 
			*value = dmstrdup("false");
	}	
	return 0;
}

int set_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{		
	struct uci_section *s;
	json_object *res;
	char *enable, *type, *device;
	static bool b, enable_b;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
			if (!res)
				return 0; //TO CHECK
			json_select(res, "up", 0, NULL, &enable, NULL);
			string_to_bool(value, &enable_b);
			if (b == enable_b)
				return 0;
			if(b) {
				uci_foreach_option_eq("network", "interface", "ifname", wandargs->fdev, s) {
					if(s != NULL) {
					//ubus call network.interface.$intf up '{}' &	$intf is section name //TODO
					}
					else 
						goto end;
				}
			}
			else {
				uci_foreach_option_eq("network", "interface", "ifname", wandargs->fdev, s) {
					if(s != NULL) {
						dmuci_get_value_by_section_string(s, "type", &type);
						if (strcmp(type, "anywan") != 0 && strcmp(type, "multiwan") != 0) {
							//ubus call network.interface.$intf down '{}' & //TODO
							goto end;
						}
						else {
							dmubus_call("network.device", "status", UBUS_ARGS{{"name", section_name(s)}}, 1, &res);
							if (res) {
								json_select(res, "device", -1, NULL, &device, NULL);
								if (strstr(device, wandargs->fdev)) {
									//ubus call network.interface.$intf down '{}' & //TODO
									goto end;
								}
							}							
						}
					}
					else 
						goto end;
				}
			}
	}
end:
	return 0;	
}

int get_wan_eth_intf_status(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	//dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", wandargs->fdev}}, 1, &res);
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup("Disabled"));
	json_select(res, "up", 0, NULL, value, NULL);
	if (*value) {
		if (strcmp(*value, "true") != 0 || (*value)[0] != '1') 
			*value = dmstrdup("Disabled");
		else
			*value = dmstrdup("Up");
	}	
	return 0;
}

int get_wan_eth_intf_mac(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res); //TOCHECK
	DM_ASSERT(res, *value = dmstrdup("00:00:00:00:00:00"));	
	json_select(res, "macaddr", 0, NULL, value, NULL);
	if (*value) {
		if ((*value)[0] == '\0') 
			*value = dmstrdup("00:00:00:00:00:00");
	} else 
		*value = dmstrdup(""); //TO CHECK
	return 0;
}

int get_wan_eth_intf_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup("0"));	
	json_select(res, "statistics", 0, "tx_bytes", value, NULL);
	if (*value) {
		if ((*value)[0] == '\0')
			*value = dmstrdup("0");
	}
	return 0;
}

int get_wan_eth_intf_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup("0"));
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	if (*value) {
		if ((*value)[0] == '\0')
			*value = dmstrdup("0");
	}
	return 0;
}

int get_wan_eth_intf_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup("0"));
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	if (*value) {
		if ((*value)[0] == '\0')
			*value = dmstrdup("0");
	}
	return 0;
}

int get_wan_eth_intf_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup("0"));
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	TRACE("get_wan_eth_intf_stats_rx_packets %s", value);
	if (*value) {
		if ((*value)[0] == '\0')
			*value = dmstrdup("0");
	}
	return 0;
}

int get_wandevice_wandevice_parameters(struct dmctx *ctx, char *dev, char *fdev)
{
	char *notif_permission;
	char *cwritable = dmstrdup("1");
	
	dmuci_get_option_value_string("network", default_wan, "ifname", &notif_permission);
	if (strcmp(dev, wan_devices[0].instance) == 0)
		cwritable[0] = '0';
	if (notif_permission[0] != '\0')
		notif_permission = dmstrdup("0"); //notif_permission is bool
	bool notif_b; 
	string_to_bool(notif_permission, &notif_b);
	TRACE("get_wandevice_wandevice_parameters %s\n", dev);
	DMOBJECT(DMROOT"WANDevice.%s.", ctx, "0", notif_b, NULL, NULL, NULL, dev); //TODO CHECK NOTIF PERMISSION AND PERMISSION
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.", ctx, cwritable, notif_b, NULL, NULL, NULL, dev);
	DMOBJECT(DMROOT"WANDevice.%s.WANCommonInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL, dev);
	DMPARAM("WANAccessType", ctx, "0", get_wan_device_wan_access_type, NULL, NULL, 0, 0, UNDEF, NULL);
	DMOBJECT(DMROOT"WANDevice.%s.WANDSLInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL, dev);
	DMPARAM("Status", ctx, "0", get_wan_device_wan_dsl_interface_config_status, NULL, NULL, 0, 0, UNDEF, NULL);
	DMPARAM("ModulationType", ctx, "0", get_wan_device_wan_dsl_interface_config_modulation_type, NULL, NULL, 0, 0, UNDEF, NULL);
	DMPARAM("DataPath", ctx, "0", get_wan_device_dsl_datapath, NULL, NULL, 0, 0, UNDEF, NULL);
	DMPARAM("DownstreamCurrRate", ctx, "0", get_wan_device_dsl_downstreamcurrrate, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("DownstreamMaxRate", ctx, "0", get_wan_device_dsl_downstreammaxrate, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("DownstreamAttenuation", ctx, "0", get_wan_device_dsl_downstreamattenuation, NULL, "xsd:int", 0, 0, UNDEF, NULL);
	DMPARAM("DownstreamNoiseMargin", ctx, "0", get_wan_device_dsl_downstreamnoisemargin, NULL, "xsd:int", 0, 0, UNDEF, NULL);
	DMPARAM("UpstreamCurrRate", ctx, "0", get_wan_device_dsl_upstreamcurrrate, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("UpstreamMaxRate", ctx, "0", get_wan_device_dsl_upstreammaxrate, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("UpstreamAttenuation", ctx, "0", get_wan_device_dsl_upstreamattenuation, NULL, "xsd:int", 0, 0, UNDEF, NULL);
	DMPARAM("UpstreamNoiseMargin", ctx, "0", get_wan_device_dsl_upstreamnoisemargin, NULL, "xsd:int", 0, 0, UNDEF, NULL);
	DMPARAM("X_INTENO_SE_AnnexMEnable", ctx, "1", get_annexm_status, set_annexm_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
	TRACE("AFTER X_INTENO_SE_AnnexMEnable %s \n", cwritable);
	if( cwritable[0] == '0' ) {
		DMOBJECT(DMROOT"WANDevice.%s.WANEthernetInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL, dev); //TODO CHECK NOTIF PERMISSION AND PERMISSION
		DMPARAM("Enable", ctx, "1", get_wan_eth_intf_enable, set_wan_eth_intf_enable, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_wan_eth_intf_status, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("MACAddress", ctx, "0", get_wan_eth_intf_mac, NULL, NULL, 0, 0, UNDEF, NULL);
		DMOBJECT(DMROOT"WANDevice.%s.WANEthernetInterfaceConfig.Stats.", ctx, "0", 1, NULL, NULL, NULL, dev); //TODO CHECK NOTIF PERMISSION AND PERMISSION
		DMPARAM("BytesSent", ctx, "0", get_wan_eth_intf_stats_tx_bytes, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("BytesReceived", ctx, "0", get_wan_eth_intf_stats_rx_bytes, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("PacketsSent", ctx, "0", get_wan_eth_intf_stats_tx_packets, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("PacketsReceived", ctx, "0", get_wan_eth_intf_stats_rx_packets, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	}
	dmfree(cwritable);
	dmfree(notif_permission);
  return 0;
}

int check_multiwan_interface(struct uci_section *s)
{
	char *type, *value;
	dmuci_get_value_by_section_string(s, "ifname", &value);
	dmuci_get_value_by_section_string(s, "type", &type);
	if (type[0] == '\0')
		return 0;
	char *pch = strtok(value," ");
	while (pch != NULL) {
		if(strcmp(pch, "atm") == 0 || strcmp(pch, "ptm") == 0 || strcmp(pch, "eth0") == 0)
		{
			return 1;
		}
		pch = strtok(NULL, " ");
	}
	return 0;	
}

int entry_method_root_WANDevice(struct dmctx *ctx)
{
	int i;
	dmuci_get_option_value_string("cwmp", "cpe", "default_wan_interface", &default_wan);
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	char *fwan;
	char *iwan = NULL;
	char *cur_iwan = NULL;
	char *pack, *stype, *proto;
	char *iconp, *cur_iconp = NULL;
	struct uci_ptr ptr = {0};
	struct uci_ptr ptr_dev = {0};
	IF_MATCH(ctx, DMROOT"WANDevice.") {
		DMOBJECT(DMROOT"WANDevice.", ctx, "0", 1, NULL, NULL, NULL);
		for (i=0; i < WAN_DEVICE; i++) {
			init_wanargs(ctx, wan_devices[i].instance, wan_devices[i].fdev);
			SUBENTRY(get_wandevice_wandevice_parameters, ctx, wan_devices[i].instance, wan_devices[i].fdev); //param have to be setted as args
			dmastrcat(&pack, "layer2_interface_", wan_devices[i].cdev);
			if ((wan_devices[i].instance)[0] == '1')
				stype = dmstrdup("ethernet_interface");
			else if ((wan_devices[i].instance)[0] == '2')
				stype = dmstrdup("atm_bridge");
			else if ((wan_devices[i].instance)[0] == '3')
				stype = dmstrdup("vdsl_interface");
			uci_foreach_sections(pack, stype, s) {
				if (s != NULL) {
					TRACE("found section %s \n", section_name(s));
					dmuci_get_value_by_section_string(s, "baseifname", &fwan);
					TRACE("entry_method_root_WANDevice call update_instance \n");
//					char *update_instance(char *package, char *stype, char *inst_opt, char *last_inst, struct uci_section *s, struct uci_ptr *uci_ptr)
					iwan = update_instance(s, cur_iwan, "waninstance");
					TRACE("entry_method_root_WANDevice call update_instance %s\n", iwan);
					TRACE("entry_method_root_WANDevice call update_instance end\n");
					//wanconnectiondevice
					init_wancdevargs(ctx, fwan);
					SUBENTRY(get_wandevice_wanconnectiondevice_parameters, ctx, wan_devices[i].instance, iwan);//"$idev" "$iwan" "$fwan"	 ONLY fdev will be used as arg for get and set function				
					//BREAK POINT1
					uci_foreach_option_cont("network", "interface", "ifname", fwan, ss) {
						if (ss != NULL) {
							//fconp is ss name
							TRACE("BREAK POINT1 \n");
							if (check_multiwan_interface(ss) != 0) //TODO
								continue;
							dmuci_get_value_by_section_string(ss, "proto", &proto);
							init_wancprotoargs(ctx, ss);
							iconp = update_instance(ss, cur_iconp, "conpinstance");
							SUBENTRY(get_wandevice_wanprotoclconnection_parameters, ctx, wan_devices[i].instance, iwan, iconp, proto);// "$idev" "$iwan" "$iconp" "$fconp" "$proto" //ONLY fconp will be usedad parameter of get and set method
							TRACE("BREAK SUBENTRY \n");
							if (cur_iconp != NULL)
								dmfree(cur_iconp);
							cur_iconp = dmstrdup(iconp);
							dmfree(iconp);
						}
						else 
							break;
					}
					if (cur_iwan)
						dmfree(cur_iwan);
					cur_iwan = dmstrdup(iwan);
					dmfree(iwan);
				}
				else 
					break;
			}
			if (cur_iwan)
				dmfree(cur_iwan);
			dmfree(stype);
			dmfree(pack);
		}
		return 0;
	}
	return FAULT_9005;
}

/************************************************************************** 
**** ****  function related to get_wandevice_wanconnectiondevice_parameters **** ****
***************************************************************************/

int get_wan_dsl_link_config_enable(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("1");
	return 0;
}

int get_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	char *vpi, *vci;
	struct uci_section *s;
	
	uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) { 
		if (s == NULL) {
			return 0;
		} //TO CHECK IF DOSEN4T CAUSE A CRASH
		else {
			dmuci_get_value_by_section_string(s, "vpi", &vpi);
			dmuci_get_value_by_section_string(s, "vci", &vci);
			dmasprintf(value, "PVC: %s/%s", vpi, vci);
			dmfree(vpi);
			dmfree(vci);
			break;
		}
	}
	return 0;
}

int set_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *vpi, *vci;
	char *delimiter = "/";
	struct uci_section *s;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) { 
				if (s == NULL) {
					return 0;
				}
				if (strstr(value, "PVC:")) {
					value += 4;
				}
				char *pvc = dmstrdup(value);
				char *tmp = cut_fx(value, delimiter, 1);
				vpi = dmstrdup(tmp);
				compress_spaces(vpi);
				dmfree(tmp);
				tmp = cut_fx(value, delimiter, 2);
				vci = dmstrdup(tmp);
				dmfree(tmp);
				compress_spaces(vci);
				dmfree(pvc);
				dmuci_set_value_by_section(s, "vpi", vpi);
				dmuci_set_value_by_section(s, "vci", vci);
				//delay_service restart "layer2_interface_adsl" "1" //TODO
				//delay_service restart "layer2_interface" "1" //TODO
				break; //TO CHECK
			}
	}	
	return 0;
} 

int get_wan_dsl_link_config_atm_encapsulation(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	struct uci_ptr ptr = {0};
	struct uci_section *s;
	char *type, *encapsulation, *encaptype;
	uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) {
		if (s == NULL)
			*value = dmstrdup("");
		else {
			dmuci_get_value_by_section_string(s,"link_type", &type);
			if (strcmp(type, "EoA") == 0 ) {
				dmfree(type);
				type = dmstrdup("eoa");
			} else if (strcmp(type, "PPPoA") == 0) {
				dmfree(type);
				type = dmstrdup("pppoa");
			} else if (strcmp(type, "IPoA") == 0) {
				dmfree(type);
				type = dmstrdup("ipoa");
			}
			dmastrcat(&encaptype, "encaps", type);
			dmfree(type);
			dmuci_get_value_by_section_string(s, encaptype, &encapsulation);
			dmfree(encaptype);
			if (strcmp(encapsulation, "vcmux") == 0) {
				dmfree(encapsulation);
				*value = dmstrdup("VCMUX");
			}
			else if (strcmp(encapsulation, "llc") == 0) {
				dmfree(encapsulation);
				*value = dmstrdup("LLC");	
			} else { //TO CHECK
				dmfree(encapsulation);
				*value = dmstrdup("");
			}
		}
	}
	return 0;
}

int set_wan_dsl_link_config_atm_encapsulation(char *refparam, struct dmctx *ctx, int action, char *value)
{
	int i;
	struct uci_section *s;
	char *type, *encapsulation, *encaptype, *pch;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) {
			if (s == NULL)
				return 0;
			dmuci_get_value_by_section_string(s, "link_type", &type);
			if (strstr(type, "EoA")) {
				encaptype = dmstrdup("encapseoa");
				encapsulation = dmstrdup("vcmux_eth:llcsnap_eth");
			}
			else if (strstr(type, "PPPoA")) {
				encaptype = dmstrdup("encapspppoa");
				encapsulation = dmstrdup("vcmux_pppoa:llcencaps_ppp");
			}
			else if (strstr(type, "IPoA")) {
				encaptype = dmstrdup("encapsipoa");
				encapsulation = dmstrdup("vcmux_ipoa:llcsnap_rtip");
			}
			if (strstr(value, "VCMUX")) {
				pch=strrchr(encapsulation,':');
				i= pch-encapsulation;
				encapsulation[i] = '\0';
			} // pch=strchr(str,':') + 1;
			else if (strstr(value, "LLC")) {
				pch=strchr(encapsulation,':');
				i= pch-encapsulation+1;
				encapsulation += i;
			}
			else
				return 0;
			break;
		}
		dmuci_set_value_by_section(s, encaptype, encapsulation);
		//delay_service restart "layer2_interface_adsl" "1" //TODO
		//delay_service restart "layer2_interface" "1" //TODO
	}	
	return 0;
}

int get_wandevice_wanconnectiondevice_parameters(struct dmctx *ctx, char *idev, char *iwan)
{
	bool b_notif;
	bool ipn_perm = 1;
	bool pppn_perm = 1;
	char *proto, *notif_permission;
	char *cwritable = dmstrdup("1");
	
	dmuci_get_option_value_string("network", default_wan, "ifname", &notif_permission);
	if (strcmp(idev, wan_devices[0].instance) == 0)
		cwritable[0] = '0';
	if (notif_permission[0] != '\0') {
		notif_permission[0] = '0';
		dmuci_get_option_value_string("network", default_wan, "proto", &proto);
		if (strcmp(proto, "dhcp") == 0 || strcmp(proto, "static") == 0)
			ipn_perm = 0; 
		if (strcmp(proto, "pppoa") == 0 || strcmp(proto, "pppoe") == 0)
			pppn_perm = 0;
	}
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.", ctx, cwritable, string_to_bool(notif_permission, &b_notif), NULL, NULL, NULL, idev, iwan);//noti permission is bool ADD notif_permission,
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.", ctx, "1", ipn_perm, NULL, NULL, NULL, idev, iwan); //ADD notif_permission:ipn_perm, 
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.", ctx, "1", pppn_perm, NULL, NULL, NULL, idev, iwan); //ADD notif_permission:pppn_perm, 
	if (strcmp(idev, wan_devices[1].instance) == 0) {
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANDSLLinkConfig.", ctx, "0", 1, NULL, NULL, NULL, idev, iwan); //ADD notif_permission:, true
		DMPARAM("Enable", ctx, "0", get_wan_dsl_link_config_enable, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("DestinationAddress", ctx, "1", get_wan_dsl_link_config_destination_address, set_wan_dsl_link_config_destination_address, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("ATMEncapsulation", ctx, "1", get_wan_dsl_link_config_atm_encapsulation, set_wan_dsl_link_config_atm_encapsulation, NULL, 0, 0, UNDEF, NULL);
	}
	dmfree(cwritable);
	return 0;
}

/************************************************************************** 
**** ****  function related to get_wandevice_wanprotoclconnection_parameters **** ****
***************************************************************************/

int get_wandevice_wanprotoclconnection_parameters(struct dmctx *ctx, char *idev, char *iwan, char *iconp, char *proto)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	//ifname is section name
	bool notif_b, perm_b;
	bool forced_inform_eip = 0;
	char *forced_notify= "";
	char *notif_permission="";
	char *linker;
	char *lan_name = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	dmastrcat(&linker, "linker_interface:", lan_name);
	if (strcmp(lan_name, default_wan) == 0) {
		forced_inform_eip = 1;
		forced_notify = "2";
		notif_permission = "0";
	}
	string_to_bool(notif_permission, &notif_b);
	
	if (strcmp(proto, "dhcp") == 0 || strcmp(proto, "static") == 0) {
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.%s.", ctx, "1", notif_b, NULL, NULL, section_name(wandcprotoargs->wancprotosection), idev, iwan, iconp);//TO CHECK "linker_interface:$nlan"
		DMPARAM("Enable", ctx, "1", get_interface_enable_ubus, set_interface_enable_ubus, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ConnectionStatus", ctx, "0", get_wan_device_mng_status, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("ExternalIPAddress", ctx, "0", get_wan_device_mng_interface_ip, NULL, NULL, notif_b, forced_inform_eip, UNDEF, NULL);	//TO ADD "$forced_notify"
		DMPARAM("MACAddress", ctx, "0", get_wan_device_mng_interface_mac, NULL, NULL, 0, 0, UNDEF, NULL);//TOCHECK
		DMPARAM("ConnectionType", ctx, "1", get_wan_ip_link_connection_connection_type, set_wan_ip_link_connection_connection_type, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("AddressingType", ctx, "1", get_wan_ip_link_connection_addressing_type, set_wan_ip_link_connection_addressing_type, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("NATEnabled", ctx, "1", get_wan_ip_link_connection_nat_enabled, set_wan_ip_link_connection_nat_enabled, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("X_BROADCOM_COM_FirewallEnabled", ctx, "1", get_interface_firewall_enabled, set_interface_firewall_enabled, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("X_BROADCOM_COM_IGMPEnabled", ctx, "1", get_wan_ip_link_connection_igmp_enabled, set_wan_ip_link_connection_igmp_enabled, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("DNSEnabled", ctx, "1", get_wan_ip_link_connection_dns_enabled, set_wan_ip_link_connection_dns_enabled, "xsd:boolean", 0, 0, UNDEF, NULL);
		//DMPARAM("DNSOverrideAllowed", ctx, "", , , "xsd:boolean", 0, 0, UNDEF, NULL);	
	} 
	else if (strcmp(proto, "pppoa") == 0 || strcmp(proto, "pppoe") == 0) {
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.%s.", ctx, "1", 1, NULL, NULL, linker, idev, iwan, iconp);//TO CHECK "linker_interface:$nlan"
		DMPARAM("Enable", ctx, "1", get_interface_enable_ubus, set_interface_enable_ubus, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ConnectionStatus", ctx, "0", get_wan_device_ppp_status, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("ExternalIPAddress", ctx, "0", get_wan_device_ppp_interface_ip, NULL, NULL, notif_b, forced_inform_eip, UNDEF, NULL);	//TO ADD "$forced_notify"
		DMPARAM("MACAddress", ctx, "0", get_wan_device_mng_interface_mac, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("Username", ctx, "1", get_wan_device_ppp_username, set_wan_device_username, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("Password", ctx, "1", get_empty, set_wan_device_password, NULL, 0, 0, UNDEF, NULL);
	}
	dmfree(lan_name);
	//dmfree(linker);
	return 0;
}

//THE same as get_wan_device_ppp_status WHY DO WE CREATE A SEPARATED FUNCTION
int get_wan_device_mng_status(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res = NULL;
	char *pending = NULL;
	char *intf;
	char *status = NULL;
	char *uptime = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);

	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup(""));
		if (json_select(res, "up", 0, NULL, &status, NULL) != -1)
		{
			if (strcmp(status, "true") == 0) {
				json_select(res, "uptime", 0, NULL, &uptime, NULL);
				json_select(res, "pending", 0, NULL, &pending, NULL);
			}
			else
				uptime = dmstrdup("0");
			dmfree(status);
		}		
		if (atoi(uptime) > 0)
			*value = dmstrdup("Connected");
		else if (pending) {
			if (strcmp(pending, "true") == 0)
				*value = dmstrdup("Pending Disconnect");
		}
		else 
			*value = dmstrdup("Disconnected");
	dmfree(uptime);
	dmfree(pending);
	dmfree(intf);
	return 0;
}

int get_wan_device_mng_interface_ip(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	if (DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP[0] != '\0')
		*value = dmstrdup(DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP);
	else {
		intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
		network_get_ipaddr(value, intf); //TODO FUNCTION NOT FOUND
		dmfree(intf);
	}	
	return 0;
}

int get_wan_ip_link_connection_connection_type(char *refparam, struct dmctx *ctx, char **value)
{
	char *type;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "type", &type);
	if (strcmp(type, "bridge") == 0 || strcmp(type, "alias") == 0)
		*value = dmstrdup("IP_Bridged");
	else 
		*value = dmstrdup("type");
	dmfree(type);
	return 0;
}

int set_wan_ip_link_connection_connection_type(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *type;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "type", &type);
			if (strcmp(value, "IP_Bridged") == 0) {
				if (strcmp(type, "bridge") == 0 || strcmp(type, "alias") == 0)
					return 0;
				else {
					if (type)
						dmfree(type);
					type = dmstrdup("bridge");
				}
			} 
			else if (strcmp(value, "IP_Routed") == 0) {
				if (strcmp(type, "bridge") != 0 || strcmp(type, "alias") != 0)
					return 0;
				else {
					if (type)
						dmfree(type);
					type = dmstrdup("");
				}
			}
			else 
				return 0;
	}
	dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "type", type);
	//delay_service reload "network" "1"
	return 0;
} 

int get_wan_ip_link_connection_addressing_type(char *refparam, struct dmctx *ctx, char **value)
{
	char *proto;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "proto", &proto);
	if (strcmp(proto, "dhcp") == 0)
		*value = dmstrdup("DHCP");
	else if (strcmp(proto, "static") == 0)
		*value = dmstrdup("Static");
	else 
		*value = dmstrdup(proto);
	dmfree(proto);
	return 0;
}

int set_wan_ip_link_connection_addressing_type(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *proto;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if(strcmp(value, "DHCP") == 0)
				proto = dmstrdup("dhcp");
			else if(strcmp(value, "Static") == 0)
				proto = dmstrdup("static");
			else 
				return 0;
			dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "proto", proto);
			//delay_service reload "network" "1"
			return 0;
	}
	return 0;
}

int get_wan_ip_link_connection_nat_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *masq, *network;
	struct uci_section *s = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	uci_foreach_sections("firewall", "zone", s) {
		dmuci_get_value_by_section_string(s, "masq", &masq);
		if (masq[0] != '\0') {
			if (masq[0] == '1' && masq[1] == '\0') {
				dmuci_get_value_by_section_string(s, "network", &network);
				if (strstr(network, intf)) {
					*value = dmstrdup("1");
					dmfree(masq);
					dmfree(network);
					return 0;
				}
			}
		}
		dmfree(masq);
	}
	*value = dmstrdup("0");
	return 0;
}

int set_wan_ip_link_connection_nat_enabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	char *intf;
	int found = 0;
	struct uci_section *s = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if(!b)
				value = "";
			uci_foreach_option_cont("firewall", "zone", "network", intf, s) {
				if (s != NULL) {
					found++;
					dmuci_set_value_by_section(s, "masq", value);
					if (value[0] != '\0')
						break;
				}				
			}
			if (found == 0 && value[0] != '\0') {
				s = create_firewall_zone_config("nat", intf, "ACCEPT", "ACCEPT", "ACCEPT");
				dmuci_set_value_by_section(s, "masq", value);
			}
			//delay_service reload "firewall" "1"
			return 0;
	}	
	return 0;
}

//get_interface_firewall_enabled PRESENT IN LANDEVICE.C TO REMOVE IN DMCOMMON FILE
//set_interface_firewall_enabled PRESENT IN LANDEVICE.C TO REMOVE IN DMCOMMON FILE


int get_wan_igmp_rule_idx(char *iface, struct uci_section **rule, struct uci_section **zone, char **enable)
{
	char *input, *izname, *proto, *target;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	
	uci_foreach_option_cont("firewall", "zone", "network", iface, *zone) {
		if (*zone != NULL) {
			dmuci_get_value_by_section_string(*zone, "name", &izname); //izname char
			dmuci_get_value_by_section_string(*zone, "input", &input);
			if (strcmp(input, "DROP") == 0)
				*enable = dmstrdup("0");
			uci_foreach_option_cont("firewall", "rule", "src", section_name(*zone), *rule) {
				dmuci_get_value_by_section_string(*rule, "proto", &proto);
				if (strcmp(proto, "igmp")) {
					dmuci_get_value_by_section_string(*rule, "enabled", enable);
					if (*enable != '\0') {
						if (*enable[0] == '0')
							break;
					}
					dmuci_get_value_by_section_string(*rule, "target", &target);
					if (target != '\0') {
						if (strcmp(target, "DROP") == 0)
							*enable = dmstrdup("0");
						else 
							*enable = dmstrdup("1");
					}
					break;
				}
			}
			if (*rule != NULL)
					break;
		}
	}
}

int get_wan_ip_link_connection_igmp_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *enable;
	struct uci_section *rule = NULL;
	struct uci_section *zone = NULL;
	
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	get_wan_igmp_rule_idx(intf, &rule, &zone, &enable);
	*value = dmstrdup(enable);
	return 0;
}

int set_wan_ip_link_connection_igmp_enabled(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	static bool b;
	int found = 0;
	char *intf, *enable, *zname;
	struct uci_section *rule = NULL;
	struct uci_section *zone = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b)
				value = dmstrdup("ACCEPT");
			else
				value = dmstrdup("DROP");
			get_wan_igmp_rule_idx(intf, &rule, &zone, &enable);
			if(zone == NULL) {
				create_firewall_zone_config("igmp", intf, "ACCEPT", "ACCEPT", "ACCEPT");
				dmastrcat(&zname, "igmp_", intf);//zname=igmp_$iface" NOT USED
			}
			if(rule == NULL) {
				dmuci_add_section("firewall", "rule", &rule, NULL);
				dmuci_set_value_by_section(rule, "src", zname);
				dmuci_set_value_by_section(rule, "proto", "igmp");
			}
			dmuci_set_value_by_section(rule, "target", value);
			dmuci_set_value_by_section(rule, "enabled", "1");
			//delay_service reload "firewall" "1"
	}
	return 0;
}

int get_wan_ip_link_connection_dns_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "peerdns", value);
	if ((*value)[0] == '\0') {
		//dmfree(*value);
		*value = dmstrdup("1"); //TO CHECK WITH KMD
	}		
	return 0;
}

int set_wan_ip_link_connection_dns_enabled(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	static bool b;
	char *intf;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if(b)
				value = "";
			dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "peerdns", value);
			//delay_service reload "network" "1"
			//delay_service reload "dnsmasq" "1"
			return 0;
	}
}

int get_wan_device_ppp_status(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf; 
	char *status = NULL;
	char *uptime = NULL;
	char *pending = NULL;
	json_object *res = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);

	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup(""));
		if (json_select(res, "up", 0, NULL, &status, NULL) != -1)
		{
			if (strcmp(status, "1") == 0) {
				json_select(res, "uptime", 0, NULL, &uptime, NULL);
				json_select(res, "pending", 0, NULL, &pending, NULL);
			}
			else
				uptime = dmstrdup("0");
			dmfree(status);
		}		
		if (atoi(uptime) > 0)
			*value = dmstrdup("Connected");
		else if (pending) {
			if (strcmp(pending, "true") == 0)
				*value = dmstrdup("Pending Disconnect");
		}
		else 
			*value = dmstrdup("Disconnected");
	dmfree(uptime);
	dmfree(pending);
	dmfree(intf);
	return 0;
}

int get_wan_device_ppp_interface_ip(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *val;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);

	if (DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP[0] != '\0')
		*value = dmstrdup(DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP);
	else {
		intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
		network_get_ipaddr(value, intf); //TODO FUNCTION NOT FOUND
		dmfree(intf);
	}	
	return 0;
}

int get_wan_device_mng_interface_mac(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *device;
	json_object *res;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	intf = dmstrdup(section_name(wandcprotoargs->wancprotosection));
	
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = dmstrdup(""));
	if (json_select(res, "device", 0, NULL, &device, NULL) != -1) {
		//dmfree(res);
		TRACE("device %s \n", device);
		dmubus_call("network.device", "status", UBUS_ARGS{{"name", device}}, 1, &res);
		if (res) {
			json_select(res, "macaddr", 0, NULL, value, NULL);
			goto end;
		}
		else 
			dmfree(device);
	}
	*value = dmstrdup("");
	return 0;
end:
	TRACE("get_wan_device_mng_interface_mac \n");
	dmfree(intf);
	dmfree(device);
	//dmfree(res);
	return 0;
}

int get_wan_device_ppp_username(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "username", value);
	return 0;
}

int set_wan_device_username(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "username", value);
	}	
	return 0;
} 

int set_wan_device_password(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	switch (action) {
		case VALUECHECK:			
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "password", value);
	}	
	return 0;
}
