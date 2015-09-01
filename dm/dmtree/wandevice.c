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
#define DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP "" //TODO ASK KMD

enum WAN_TYPE_CONNECTION {
	WAN_IP_CONNECTION,
	WANPPPConnection
};

enum enum_wan_dsl {
	WAN_DSL_NODSL,
	WAN_DSL_ADSL,
	WAN_DSL_VDSL
};

struct wan_device 
{
	char *instance;
	char *fdev;
	char *stype;
	char *cdev;
};

struct wan_device wan_devices[WAN_DEVICE] = {
	{"1", "eth0", "ethernet_interface", "layer2_interface_ethernet"},
	{"2", "atm", "atm_bridge", "layer2_interface_adsl"},
	{"3", "ptm", "vdsl_interface", "layer2_interface_vdsl"}
};

char *default_wan;

struct wanargs
{
	int instance;
	char *fdev;
};

struct wanargs cur_wanargs = {0};
struct wancdevargs
{
	struct uci_section *wandevsection;
	int index;
	char *fwan;
	char *iwan;
};

struct wancdevargs cur_wancdevargs = {0};

struct wancprotoargs
{
	struct uci_section *wancprotosection;
	struct uci_ptr *ptr;
	
};

struct wancprotoargs cur_wancprotoargs = {0};

inline int init_wanargs(struct dmctx *ctx, int wan_instance, char *fdev)
{
	struct wanargs *args = &cur_wanargs;
	ctx->args = (void *)args;
	args->instance = wan_instance;
	args->fdev = fdev;
	return 0;
}

inline int init_wancprotoargs(struct dmctx *ctx, struct uci_section *s)
{
	struct wancprotoargs *args = &cur_wancprotoargs;
	ctx->args = (void *)args;
	args->wancprotosection = s;
	return 0;
}

inline int init_wancdevargs(struct dmctx *ctx, struct uci_section *s, int index, char *fwan, char *iwan)
{
	struct wancdevargs *args = &cur_wancdevargs;
	ctx->args = (void *)args;
	args->wandevsection = s;
	args->index = index;
	args->fwan = fwan;
	args->iwan = iwan;
	return 0;
}

//TO CHECK
int network_get_ipaddr(char **value, char *iface)
{
	json_object *res;
	
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", iface}}, 0, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "ipv4-address", 0, "address", value, NULL);
	return 0;
}

/****** ADD-DEL OBJECT *******************/
int add_wan_wanconnectiondevice(struct dmctx *ctx, char **instancepara)
{
	int iwan;
	char *value;
	char instance[8] = {0};
	char ifname[16] = {0};
	char buf[16] = {0};
	struct uci_section *s = NULL;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	TRACE("%d \n", wandargs->instance);
	if (wandargs->instance == WAN_INST_ATM) {
		sprintf(instance, "%s", max_instance("layer2_interface_adsl", "atm_bridge", "baseifname", "waninstance", "atm"));
		iwan = atoi(instance);
		sprintf(buf,"atm%s",instance);
		sprintf(ifname,"%s.1",buf);
		sprintf(instance,"%d",iwan + 1);
		dmuci_add_section("layer2_interface_adsl", "atm_bridge", &s, &value);
		dmuci_set_value_by_section(s, "baseifname", buf);
		dmuci_set_value_by_section(s, "bridge", "0");
		dmuci_set_value_by_section(s, "encapseoa", "llcsnap_eth");
		dmuci_set_value_by_section(s, "ifname", ifname);
		dmuci_set_value_by_section(s, "link_type", "EoA");
		dmuci_set_value_by_section(s, "unit", buf+3);
		dmuci_set_value_by_section(s, "vci", "35");
		dmuci_set_value_by_section(s, "vpi", "8");
		dmuci_set_value_by_section(s, "waninstance", instance);
		//delay_service restart "layer2_interface_adsl" "1"
		*instancepara = dmstrdup(instance);
		//delay_service restart "layer2_interface" "1"
		//freecwmp_output "" "" "" "" "" "" "1" "$iwan"
		return 0;
	}
	else if (strcmp(wandargs->fdev, "ptm") == 0) {
		sprintf(instance, "%s", max_instance("layer2_interface_vdsl", "vdsl_interface", "baseifname", "waninstance", "ptm"));
		iwan = atoi(instance);
		sprintf(buf,"ptm%s",instance);
		sprintf(ifname,"%s.1",buf);
		sprintf(instance,"%d",iwan + 1);
		dmuci_add_section("layer2_interface_vdsl", "vdsl_interface", &s, &value);
		dmuci_set_value_by_section(s, "baseifname", buf);
		dmuci_set_value_by_section(s, "bridge", "0");
		dmuci_set_value_by_section(s, "ifname", ifname);
		dmuci_set_value_by_section(s, "unit", buf+3);
		//delay_service restart "layer2_interface_vdsl" "1"
		dmuci_set_value_by_section(s, "waninstance", instance);
		*instancepara = dmstrdup(instance);
		//delay_service restart "layer2_interface" "1"
		//freecwmp_output "" "" "" "" "" "" "1" "$iwan"
		return 0;
	}
	return FAULT_9005;
}

int delete_wan_wanconnectiondevice_all(struct dmctx *ctx)
{
	int found = 0;
	struct uci_section *s = NULL; 
	struct uci_section *ss = NULL;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	TRACE("%d \n", wandargs->instance);
	
	uci_foreach_option_cont(wan_devices[wandargs->instance - 1].cdev, wan_devices[wandargs->instance - 1].stype, "baseifname", wandargs->fdev, s) {	
		if (found != 0)
			dmuci_delete_by_section(ss, NULL, NULL);
		ss = s;
		found++;
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	return 0;
}

int delete_wan_wanconnectiondevice(struct dmctx *ctx)
{
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	
	dmuci_delete_by_section(wandcdevargs->wandevsection, NULL, NULL);
	dmuci_commit();
	return 0;
}
int add_wan_wanipconnection(struct dmctx *ctx, char **instancepara)
{
	struct uci_section *s;
	char *value;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	int found = 0;
	char sname[16] = {0};
	char ifname[8] = {0};
	char instance[8] = {0};
	//TO CHECK NORMALLY THIS IS NOT NECESSAR
	/*uci_foreach_option_eq(wan_devices[wandcdevargs->index].cdev, wan_devices[wandcdevargs->index].stype, "baseifname", wandcdevargs->fwan, s) {
		found++;
		break;
	}
	if(found == 0) {
		return FAULT_9005;
	} *///TO CHECK NORMALLY THIS IS NOT NECESSAR $idev""_$iwan""_$iproto""_$((++iconp)
	sprintf(instance, "%d", atoi(max_instance("network", "interface", "ifname", "conpinstance", wandcdevargs->fwan)) + 1);
	sprintf(sname,"wan_%s_%s_%s_%s", wan_devices[wandcdevargs->index].instance, wandcdevargs->iwan, WAN_IP_CONNECTION, instance); //TODO ADD FUNCTION TO RENAME A SECTION"wan_""$idev""_$iwan""_$iproto""_$((++iconp))"
	sprintf(ifname, "%s.1", wandcdevargs->fwan);
	dmuci_add_section("network", "interface", &s, &value);
	dmuci_rename_section_by_section(s, sname);
	dmuci_set_value_by_section(s, "ifname", ifname);
	dmuci_set_value_by_section(s, "proto", "dhcp");
	dmuci_set_value_by_section(s, "conpinstance", instance);
	*instancepara = dmstrdup(instance);
	return 0;
}

int delete_wan_wanipconnectiondevice_all(struct dmctx *ctx)
{
	int found = 0;
	char *ifname, *iproto;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;

	dmuci_get_value_by_section_string(wandcdevargs->wandevsection, "ifname", &ifname);
	uci_foreach_option_eq("network", "interface", "ifname", ifname, s) {
		dmuci_get_value_by_section_string(s, "proto", &iproto);
		if (strcmp(iproto, "dhcp") == 0) { //CHECK IF WE CAN OPTIMISE AND IF iproto can be static
			if (found != 0)
				dmuci_delete_by_section(ss, NULL, NULL);
			ss = s;
			found++;
		}		
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	return 0;
}

int delete_wan_wanipconnectiondevice(struct dmctx *ctx)
{
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	
	dmuci_delete_by_section(wandcdevargs->wandevsection, NULL, NULL);
	return 0;
}

int add_wan_wanpppconnection(struct dmctx *ctx, char **instancepara)
{
	struct uci_section *s;
	char *value;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	int found = 0;
	char sname[16] = {0};
	char ifname[8] = {0};
	char instance[8] = {0};

	sprintf(instance, "%d", atoi(max_instance("network", "interface", "ifname", "conpinstance", wandcdevargs->fwan)) + 1) ;
	sprintf(sname,"wan_%s_%s_%s_%s", wan_devices[wandcdevargs->index].instance, wandcdevargs->iwan, WANPPPConnection, instance); //TODO ADD FUNCTION TO RENAME A SECTION"wan_""$idev""_$iwan""_$iproto""_$((++iconp))"
	sprintf(ifname, "%s.1", wandcdevargs->fwan);
	dmuci_add_section("network", "interface", &s, &value);
	dmuci_rename_section_by_section(s, sname);
	dmuci_set_value_by_section(s, "ifname", ifname);
	dmuci_set_value_by_section(s, "proto", "pppoe");
	dmuci_set_value_by_section(s, "conpinstance", instance);
	*instancepara = dmstrdup(instance);
	return 0;
}
int delete_wan_wanpppconnectiondevice_all(struct dmctx *ctx)
{
	int found = 0;
	char *ifname, *iproto;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;

	dmuci_get_value_by_section_string(wandcdevargs->wandevsection, "ifname", &ifname);
	uci_foreach_option_eq("network", "interface", "ifname", ifname, s) {
		dmuci_get_value_by_section_string(s, "proto", &iproto);
		if (strcmp(iproto, "pppoe") == 0) { //CHECK IF WE CAN OPTIMISE AND IF iproto can be pppoa
			if (found != 0)
				dmuci_delete_by_section(ss, NULL, NULL);
			ss = s;
			found++;
		}		
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	return 0;
}
/********************/

/************************************************************************** 
**** ****  function related to get_wandevice_wandevice_parameters  **** ****
***************************************************************************/

int get_wan_device_wan_dsl_traffic()
{
	json_object *res;
	int dsl = WAN_DSL_NODSL;
	char *str;

	dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
	if (!res) 
		return dsl;
	json_select(res, "dslstats", -1, "traffic", &str, NULL);
	if (str) {
		if (strstr(str, "ATM")) {
			dsl = WAN_DSL_ADSL;
		}
		else if (strstr(str, "PTM")) {
			dsl = WAN_DSL_VDSL;
		}
	}
	return dsl;
}

int get_wan_device_wan_access_type(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	switch(wandargs->instance) {
		case WAN_INST_ETH:
			*value = "Ethernet";
			break;
		case WAN_INST_ATM:
		case WAN_INST_PTM:
			*value = "DSL";
			break;
		default:
			*value = "";
			break;
	}
	return 0;
}

int get_wan_device_wan_dsl_interface_config_status(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	
	char *status;
	int dsl;
	json_object *res;
	if (wandargs->instance == WAN_INST_ETH)
		*value = "NoSignal Not a dsl interface";
	else {
		json_object *res = NULL;
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			*value = "";
			return 0;
		}

		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "dslstats", -1, "status", &status, NULL);
		TRACE("get_wan_device_wan_dsl_interface_config_status after select %s \n", status);
		if (strcmp(status, "Showtime") == 0)
			*value = "Up";
		else if (strcmp(status, "Training") == 0)
			*value = "Initializing";
		else if (strcmp(status, "Channel Analysis") == 0)
			*value = "EstablishingLink";
		else if (strcmp(status, "Disabled") == 0)
			*value = "Disabled";
		else 
			*value = "NoSignal";
	}	
end:
	return 0;
}

int get_wan_device_wan_dsl_interface_config_modulation_type(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *mode;
	int dsl;
	json_object *res = NULL;

	if (wandargs->instance == WAN_INST_ETH)
		*value = "Not a dsl interface";
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			*value = "";
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "dslstats", -1, "mode", &mode, NULL);
		TRACE("get_wan_device_wan_dsl_interface_config_modulation_type %s\n", mode);
		if (strcmp(mode, "G.Dmt") == 0)
			*value = "ADSL_G.dmt";
		else if (strcmp(mode, "G.lite") == 0)
			*value = "ADSL_G.lite";
		else if (strcmp(mode, "T1.413") == 0)
			*value = "ADSL_ANSI_T1.413";
		else if (strcmp(mode, "ADSL2") == 0)
			*value = "ADSL_G.dmt.bis";
		else if (strcmp(mode, "AnnexL") == 0)
			*value = "ADSL_re-adsl";
		else if (strcmp(mode, "ADSL2+") == 0)
			*value = "ADSL_2plus";
		else
			*value = mode;
	}
	return 0;
}

int get_wan_device_dsl_datapath(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *val = "";
	int dsl;
	if (wandargs->instance == WAN_INST_ETH)
		*value = "None";
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			*value = "";
			return 0;
		}

		val = "`adsl info --state|grep \"Upstream rate\"| awk -F'['$'\t'' ,]' '$1!=\"Max:\" {print $2}'`"; //TODO IS THER AN EQUIVALENT UBUS CMD
		if (strcmp(val, "FAST")) {
			*value = "Fast";
		}
		else if (strcmp(val, "INTR")) {
			*value = "Interleaved";
		}
		else {
			*value = "None";
		}
	}
	return 0;
}

int get_wan_device_dsl_downstreamcurrrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *rate_down;
	int dsl;
	json_object *res = NULL;
	json_object *sub_obj= NULL;
	*value = "0";

	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);	
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "rate_down", &rate_down, NULL);
		else 
			return 0;
		TRACE("get_wan_device_dsl_downstreamcurrrate rate down %s", rate_down);
		if (rate_down && rate_down[0] != '\0') {
			*value = rate_down;
		}
	}
	return 0;
}

int get_wan_device_dsl_downstreammaxrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *max_down;
	int dsl;
	json_object *res = NULL;
	json_object *sub_obj = NULL;
	*value = "0";
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);	
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "max_rate_down", &max_down, NULL);
		else 
			return 0;
		if (max_down && max_down[0] != '\0') {
			*value = max_down;
		}
	}
	return 0; 	
}

int get_wan_device_dsl_downstreamattenuation(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *attn_down_x100;
	int dsl;
	json_object *res = NULL;
	*value = "0";
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, "attn_down_x100", &attn_down_x100, NULL);
		if (attn_down_x100) {
			dmasprintf(&attn_down_x100, "%d", (atoi(attn_down_x100) % 10));// MEM WILL BE FREED IN DMMEMCLEAN
			*value = attn_down_x100;
		}
	}
	return 0;
}

int get_wan_device_dsl_downstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *snr_down_x100;
	int dsl;
	json_object *res;
	*value = "0";
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, "snr_down_x100", &snr_down_x100, NULL);
		if (snr_down_x100) {
			dmasprintf(&snr_down_x100, "%d", (atoi(snr_down_x100) % 10));// MEM WILL BE FREED IN DMMEMCLEAN
			*value = snr_down_x100;
		}
	}
	return 0;
}

int get_wan_device_dsl_upstreamcurrrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *rate_up;
	int dsl;
	json_object *res = NULL;
	json_object *sub_obj = NULL;
	*value = "0";
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);	
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "rate_up", &rate_up, NULL);
		else 
			return 0;
		*value = rate_up;
	}
	return 0;
}

int get_wan_device_dsl_upstreammaxrate(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *max_up;
	int dsl;
	json_object *res = NULL; 
	json_object *sub_obj = NULL;
	*value = "0";
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, NULL, NULL, &sub_obj);
		if (sub_obj)
			json_select(sub_obj, "bearers", 0, "max_rate_up", &max_up, NULL);
		else 
			return 0;
		*value = max_up;
	}
	return 0;
}

int get_wan_device_dsl_upstreamattenuation(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *attn_up_x100;
	int dsl;
	json_object *res = NULL;
	*value = "0";
	if (wandargs->instance == WAN_INST_ETH)
		return 0;
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, "attn_up_x100", &attn_up_x100, NULL);
		if (attn_up_x100) {
			dmasprintf(&attn_up_x100, "%d", (atoi(attn_up_x100) % 10)); // MEM WILL BE FREED IN DMMEMCLEAN
			*value = attn_up_x100;
		}
	}
	return 0;
}

int get_wan_device_dsl_upstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	char *snr_up_x100;
	int dsl;
	json_object *res;
	if (wandargs->instance == WAN_INST_ETH) {
		*value = "0";
		return 0;
	}
	else {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			*value = "0";
			return 0;
		}
		dmubus_call("router", "dslstats", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "0");
		json_select(res, "dslstats", -1, "snr_up_x100", &snr_up_x100, NULL);
		if (snr_up_x100) {
			dmasprintf(&snr_up_x100, "%d", (atoi(snr_up_x100) % 10));// MEM WILL BE FREED IN DMMEMCLEAN
			*value = snr_up_x100;
		}
		else {
			*value = "0";
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
	if (*value != NULL) {
		if (strcasecmp(*value, "enabled") == 0) {
			*value = "1";
			return 0;
		}
	}
	*value = "0";
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
					value = "Enabled";
				else
					value = "";
			}
			dmuci_set_value("layer2_interface", "capabilities", "AnnexM", value);
			return 0;
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
	DM_ASSERT(res, *value = "0");
	json_select(res, "up", -1, NULL, value, NULL);
	TRACE("get_wan_eth_intf_enable \n");
	if (*value) {
		if (strcmp(*value, "true") != 0 || (*value)[0] != '1') 
			*value = "0";
	}
	return 0;
}

int set_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{		
	struct uci_section *s;
	json_object *res;
	char *enable, *type, *device;
	static bool b;
	bool enable_b;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
			if (!res)
				return 0;
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
			return 0;
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
	DM_ASSERT(res, *value = "Disabled");
	json_select(res, "up", 0, NULL, value, NULL);
	if (*value) {
		if (strcmp(*value, "true") != 0 || (*value)[0] != '1') 
			*value = "Disabled";
		else
			*value = "Up";
	}	
	return 0;
}

int get_wan_eth_intf_mac(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res); //TOCHECK
	DM_ASSERT(res, *value = "00:00:00:00:00:00");
	json_select(res, "macaddr", 0, NULL, value, NULL);
	if (!(*value) || (*value)[0] == '\0') {
		*value = "00:00:00:00:00:00";
	}
	return 0;
}

int get_wan_eth_intf_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = "0");
	json_select(res, "statistics", 0, "tx_bytes", value, NULL);
	if (!(*value) || (*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int get_wan_eth_intf_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = "0");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	if (!(*value) || (*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int get_wan_eth_intf_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = "0");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	if (!(*value) || (*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int get_wan_eth_intf_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;
	
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = "0");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	TRACE("get_wan_eth_intf_stats_rx_packets %s", value);
	if (!(*value) || (*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
}

int get_wandevice_wandevice_parameters(struct dmctx *ctx, char *dev, char *fdev)
{
	char *notif_permission;
	char cwritable[] = "1";
	
	dmuci_get_option_value_string("network", default_wan, "ifname", &notif_permission); //TODO check this with script notif_permission is bool??
	if (strcmp(dev, wan_devices[0].instance) == 0)
		cwritable[0] = '0';
	if (notif_permission[0] != '\0')
		notif_permission = "0"; //notif_permission is bool
	bool notif_b; 
	string_to_bool(notif_permission, &notif_b);
	TRACE("get_wandevice_wandevice_parameters %s\n", dev);
	DMOBJECT(DMROOT"WANDevice.%s.", ctx, "0", notif_b, NULL, NULL, NULL, dev); //TODO CHECK NOTIF PERMISSION AND PERMISSION
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.", ctx, cwritable, notif_b, add_wan_wanconnectiondevice, delete_wan_wanconnectiondevice_all, NULL, dev);
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
  return 0;
}

//TO CHECK
int check_multiwan_interface(struct uci_section *s)
{
	char *type, *value;
	dmuci_get_value_by_section_string(s, "ifname", &value);
	dmuci_get_value_by_section_string(s, "type", &type);
	if (type[0] == '\0')
		return 0;
	value = dmstrdup(value);
	char *pch = strtok(value," ");
	while (pch != NULL) {
		if(strstr(pch, "atm") || strstr(pch, "ptm") || strstr(pch, "eth0"))
		{
			dmfree(value);
			return 1;
		}
		pch = strtok(NULL, " ");
	}
	dmfree(value);
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
			init_wanargs(ctx, i+1, wan_devices[i].fdev);
			SUBENTRY(get_wandevice_wandevice_parameters, ctx, wan_devices[i].instance, wan_devices[i].fdev); //param have to be setted as args
			pack = wan_devices[i].cdev;
			stype = wan_devices[i].stype;
			cur_iwan = NULL;
			uci_foreach_sections(pack, stype, s) {
				TRACE("found section %s \n", section_name(s));
				dmuci_get_value_by_section_string(s, "baseifname", &fwan);
				TRACE("entry_method_root_WANDevice call update_instance \n");
//					char *update_instance(char *package, char *stype, char *inst_opt, char *last_inst, struct uci_section *s, struct uci_ptr *uci_ptr)
				iwan = update_instance(s, cur_iwan, "waninstance");
				TRACE("entry_method_root_WANDevice call update_instance %s\n", iwan);
				TRACE("entry_method_root_WANDevice call update_instance end\n");
				//wanconnectiondevice
				init_wancdevargs(ctx, s, i, fwan, iwan);
				SUBENTRY(get_wandevice_wanconnectiondevice_parameters, ctx, wan_devices[i].instance, iwan);//"$idev" "$iwan" "$fwan"	 ONLY fdev will be used as arg for get and set function
				//BREAK POINT1
				cur_iconp = NULL;
				uci_foreach_option_cont("network", "interface", "ifname", fwan, ss) {
					//fconp is ss name
					TRACE("BREAK POINT1 \n");
					if (check_multiwan_interface(ss) != 0) //TODO
						continue;
					dmuci_get_value_by_section_string(ss, "proto", &proto);
					init_wancprotoargs(ctx, ss);
					iconp = update_instance(ss, cur_iconp, "conpinstance");
					SUBENTRY(get_wandevice_wanprotoclconnection_parameters, ctx, wan_devices[i].instance, iwan, iconp, proto);// "$idev" "$iwan" "$iconp" "$fconp" "$proto" //ONLY fconp will be usedad parameter of get and set method
					TRACE("BREAK SUBENTRY \n");
					dmfree(cur_iconp);
					cur_iconp = dmstrdup(iconp);
				}
				dmfree(cur_iconp);
				dmfree(cur_iwan);
				cur_iwan = dmstrdup(iwan);
			}
			dmfree(cur_iwan);
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
	*value = "1";
	return 0;
}

int get_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	char *vpi, *vci;
	struct uci_section *s;
	
	uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) { 
		dmuci_get_value_by_section_string(s, "vpi", &vpi);
		dmuci_get_value_by_section_string(s, "vci", &vci);
		dmasprintf(value, "PVC: %s/%s", vpi, vci); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
}

int set_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *vpi = NULL, *vci = NULL;
	struct uci_section *s;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) {
				//PVC: VPI/VCI
				if (strstr(value, "PVC: "))
					value += 5;
				else
					return 0;
				value = dmstrdup(value);
				vpi = strtok(value, "/");
				if (vpi) {
					vci = strtok(NULL, "/");
				}
				if (vpi && vci) {
					dmuci_set_value_by_section(s, "vpi", vpi);
					dmuci_set_value_by_section(s, "vci", vci);
				}
				dmfree(value);
				break;
			}
			return 0;
	}	
	return 0;
} 

int get_wan_dsl_link_config_atm_encapsulation(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	struct uci_section *s;
	char *type, *encapsulation, *encaptype;
	*value = "";

	uci_foreach_option_eq("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) {
		dmuci_get_value_by_section_string(s,"link_type", &type);
		if (strcmp(type, "EoA") == 0 ) {
			type = "encapseoa";
		} else if (strcmp(type, "PPPoA") == 0) {
			type = "encapspppoa";
		} else if (strcmp(type, "IPoA") == 0) {
			type = "encapsipoa";
		}
		dmuci_get_value_by_section_string(s, type, &encapsulation);
		if (strcmp(encapsulation, "vcmux") == 0) {
			*value = "VCMUX";
		}
		else if (strcmp(encapsulation, "llc") == 0) {
			*value = "LLC";
		} else {
			*value = "";
		}
		break;
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
				dmuci_get_value_by_section_string(s, "link_type", &type);
				int enc;
				if (strstr(value, "VCMUX")) {
					enc = 1;
				}
				else if (strstr(value, "LLC")) {
					enc = 0;
				}
				else
					return 0;
				if (strstr(type, "EoA")) {
					encaptype = "encapseoa";
					encapsulation = enc ? "vcmux_eth" : "llcsnap_eth";
				}
				else if (strstr(type, "PPPoA")) {
					encaptype = "encapspppoa";
					encapsulation = enc ? "vcmux_pppoa" : "llcencaps_ppp";
				}
				else if (strstr(type, "IPoA")) {
					encaptype = "encapsipoa";
					encapsulation = enc ? "vcmux_ipoa" : "llcsnap_rtip";
				}
				else
					return 0;
				break;
			}
			dmuci_set_value_by_section(s, encaptype, encapsulation);
			return 0;
	}
	return 0;
}

int get_wandevice_wanconnectiondevice_parameters(struct dmctx *ctx, char *idev, char *iwan)
{
	bool b_notif;
	bool ipn_perm = 1;
	bool pppn_perm = 1;
	char *proto, *notif_permission;
	char cwritable[] = "1";
	
	dmuci_get_option_value_string("network", default_wan, "ifname", &notif_permission); //TODO bool!!!??
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
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.", ctx, cwritable, string_to_bool(notif_permission, &b_notif), NULL, delete_wan_wanconnectiondevice, NULL, idev, iwan);//noti permission is bool ADD notif_permission,
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.", ctx, "1", ipn_perm, add_wan_wanipconnection, delete_wan_wanipconnectiondevice_all, NULL, idev, iwan); //ADD notif_permission:ipn_perm, 
	DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.", ctx, "1", pppn_perm, add_wan_wanpppconnection, delete_wan_wanpppconnectiondevice_all, NULL, idev, iwan); //ADD notif_permission:pppn_perm, 
	if (strcmp(idev, wan_devices[1].instance) == 0) {
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANDSLLinkConfig.", ctx, "0", 1, NULL, NULL, NULL, idev, iwan); //ADD notif_permission:, true
		DMPARAM("Enable", ctx, "0", get_wan_dsl_link_config_enable, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("DestinationAddress", ctx, "1", get_wan_dsl_link_config_destination_address, set_wan_dsl_link_config_destination_address, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("ATMEncapsulation", ctx, "1", get_wan_dsl_link_config_atm_encapsulation, set_wan_dsl_link_config_atm_encapsulation, NULL, 0, 0, UNDEF, NULL);
	}
	return 0;
}

/************************************************************************** 
**** ****  function related to get_wandevice_wanprotoclconnection_parameters **** ****
***************************************************************************/

int get_wandevice_wanprotoclconnection_parameters(struct dmctx *ctx, char *idev, char *iwan, char *iconp, char *proto)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	//ifname is section name
	bool notif_b = true;
	bool forced_inform_eip = 0;
	char *forced_notify= "";
	char *linker;
	char *lan_name = section_name(wandcprotoargs->wancprotosection);
	dmastrcat(&linker, "linker_interface:", lan_name);
	if (strcmp(lan_name, default_wan) == 0) {
		forced_inform_eip = 1;
		forced_notify = "2"; //TODO fix that and should be int and not char
		notif_b = false;
	}
	
	if (strcmp(proto, "dhcp") == 0 || strcmp(proto, "static") == 0) {
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.%s.", ctx, "1", notif_b, NULL, delete_wan_wanconnectiondevice, section_name(wandcprotoargs->wancprotosection), idev, iwan, iconp);//TO CHECK "linker_interface:$nlan"
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
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.%s.", ctx, "1", 1, NULL, delete_wan_wanconnectiondevice, linker, idev, iwan, iconp);//TO CHECK "linker_interface:$nlan"
		DMPARAM("Enable", ctx, "1", get_interface_enable_ubus, set_interface_enable_ubus, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ConnectionStatus", ctx, "0", get_wan_device_ppp_status, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("ExternalIPAddress", ctx, "0", get_wan_device_ppp_interface_ip, NULL, NULL, notif_b, forced_inform_eip, UNDEF, NULL);	//TO ADD "$forced_notify"
		DMPARAM("MACAddress", ctx, "0", get_wan_device_mng_interface_mac, NULL, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("Username", ctx, "1", get_wan_device_ppp_username, set_wan_device_username, NULL, 0, 0, UNDEF, NULL);
		DMPARAM("Password", ctx, "1", get_empty, set_wan_device_password, NULL, 0, 0, UNDEF, NULL);
	}
	dmfree(linker);
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

	intf = section_name(wandcprotoargs->wancprotosection);
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = "");
	if (json_select(res, "up", 0, NULL, &status, NULL) != -1)
	{
		if (strcmp(status, "true") == 0) {
			json_select(res, "uptime", 0, NULL, &uptime, NULL);
			json_select(res, "pending", 0, NULL, &pending, NULL);
		}
	}
	if (uptime && atoi(uptime) > 0)
		*value = "Connected";
	else if (pending && (strcmp(pending, "true") == 0)) {
		*value = "Pending Disconnect";
	}
	else
		*value = "Disconnected";
	return 0;
}

int get_wan_device_mng_interface_ip(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	if (DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP[0] != '\0')
		*value = DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP;
	else {
		intf = section_name(wandcprotoargs->wancprotosection);
		network_get_ipaddr(value, intf); //TODO FUNCTION NOT FOUND
	}	
	return 0;
}

int get_wan_ip_link_connection_connection_type(char *refparam, struct dmctx *ctx, char **value)
{
	char *type;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *)ctx->args;
	
	dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "type", &type);
	if (strcmp(type, "bridge") == 0 || strcmp(type, "alias") == 0)
		*value = "IP_Bridged";
	else 
		*value = "IP_Routed";
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
					type = "bridge";
				}
			} 
			else if (strcmp(value, "IP_Routed") == 0) {
				if (strcmp(type, "bridge") != 0 || strcmp(type, "alias") != 0)
					return 0;
				else {
					type = "";
				}
			}
			else 
				return 0;
			dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "type", type);
			return 0;
	}
	return 0;
} 

int get_wan_ip_link_connection_addressing_type(char *refparam, struct dmctx *ctx, char **value)
{
	char *proto;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "proto", &proto);
	if (strcmp(proto, "dhcp") == 0)
		*value = "DHCP";
	else if (strcmp(proto, "static") == 0)
		*value = "Static";
	else 
		*value = proto;
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
				proto = "dhcp";
			else if(strcmp(value, "Static") == 0)
				proto = "static";
			else 
				return 0;
			dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "proto", proto);
			return 0;
	}
	return 0;
}

int get_wan_ip_link_connection_nat_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *masq, *network;
	struct uci_section *s = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = section_name(wandcprotoargs->wancprotosection);
	uci_foreach_sections("firewall", "zone", s) {
		dmuci_get_value_by_section_string(s, "masq", &masq);
		if (masq[0] != '\0') {
			if (masq[0] == '1' && masq[1] == '\0') {
				dmuci_get_value_by_section_string(s, "network", &network);
				if (strstr(network, intf)) {
					*value = "1";
					return 0;
				}
			}
		}
	}
	*value = "0";
	return 0;
}

int set_wan_ip_link_connection_nat_enabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	static bool b;
	char *intf;
	int found = 0;
	struct uci_section *s = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = section_name(wandcprotoargs->wancprotosection);
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if(!b)
				value = "";
			else
				value = "1";
			uci_foreach_option_cont("firewall", "zone", "network", intf, s) {
				found++;
				dmuci_set_value_by_section(s, "masq", value);
				if (value[0] != '\0')
					break;
			}
			if (found == 0 && value[0] != '\0') {
				s = create_firewall_zone_config("nat", intf, "ACCEPT", "ACCEPT", "ACCEPT");
				dmuci_set_value_by_section(s, "masq", value);
			}
			return 0;
	}	
	return 0;
}

//TODO get_interface_firewall_enabled PRESENT IN LANDEVICE.C TO REMOVE IN DMCOMMON FILE
//TODO set_interface_firewall_enabled PRESENT IN LANDEVICE.C TO REMOVE IN DMCOMMON FILE


int get_wan_igmp_rule_idx(char *iface, struct uci_section **rule, struct uci_section **zone, char **enable)
{
	char *input, *proto, *target;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	
	uci_foreach_option_cont("firewall", "zone", "network", iface, *zone) {
		dmuci_get_value_by_section_string(*zone, "input", &input);
		if (strcmp(input, "DROP") == 0)
			*enable = "0";
		uci_foreach_option_cont("firewall", "rule", "src", section_name(*zone), *rule) {
			dmuci_get_value_by_section_string(*rule, "proto", &proto);
			if (strcmp(proto, "igmp")) {
				dmuci_get_value_by_section_string(*rule, "enabled", enable);
				if (*enable != '\0') {
					if ((*enable)[0] == '0')
						break;
				}
				dmuci_get_value_by_section_string(*rule, "target", &target);
				if (target[0] != '\0') {
					if (strcmp(target, "DROP") == 0)
						*enable = "0";
					else
						*enable = "1";
				}
				break;
			}
		}
		if (*rule != NULL)
				break;
	}
	return 0;
}

int get_wan_ip_link_connection_igmp_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *enable = "0";
	struct uci_section *rule = NULL;
	struct uci_section *zone = NULL;
	
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = section_name(wandcprotoargs->wancprotosection);
	get_wan_igmp_rule_idx(intf, &rule, &zone, &enable);
	*value = enable;
	return 0;
}

int set_wan_ip_link_connection_igmp_enabled(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	static bool b;
	int found = 0;
	char *intf, *enable, *zname, buf[32];
	struct uci_section *rule = NULL;
	struct uci_section *zone = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = section_name(wandcprotoargs->wancprotosection);
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b)
				value = "ACCEPT";
			else
				value = "DROP";
			get_wan_igmp_rule_idx(intf, &rule, &zone, &enable);
			if(zone == NULL) {
				create_firewall_zone_config("igmp", intf, "ACCEPT", "ACCEPT", "ACCEPT");
				sprintf(buf, "igmp_%s", intf);
				zname = buf;
			} else {
				dmuci_get_value_by_section_string(zone, "name", &zname);
			}
			if(rule == NULL) {
				dmuci_add_section("firewall", "rule", &rule, NULL);
				dmuci_set_value_by_section(rule, "src", zname);
				dmuci_set_value_by_section(rule, "proto", "igmp");
			}
			dmuci_set_value_by_section(rule, "target", value);
			dmuci_set_value_by_section(rule, "enabled", "1");
			return 0;
	}
	return 0;
}

int get_wan_ip_link_connection_dns_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	dmuci_get_value_by_section_string(wandcprotoargs->wancprotosection, "peerdns", value);
	if ((*value)[0] == '\0') {
		*value = "1";
	}
	return 0;
}

int set_wan_ip_link_connection_dns_enabled(char *refparam, struct dmctx *ctx, int action, char *value) 
{
	static bool b;
	char *intf;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = section_name(wandcprotoargs->wancprotosection);
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if(b)
				value = "";
			else
				value = "0";
			dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "peerdns", value);
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

	intf = section_name(wandcprotoargs->wancprotosection);
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = "");
	if (json_select(res, "up", 0, NULL, &status, NULL) != -1)
	{
		if (status[0] == '1' && status[1] == '\0') {
			json_select(res, "uptime", 0, NULL, &uptime, NULL);
			json_select(res, "pending", 0, NULL, &pending, NULL);
		}
	}
	if (uptime && atoi(uptime) > 0)
		*value = "Connected";
	else if (pending && strcmp(pending, "true") == 0)
		*value = "Pending Disconnect";
	else
		*value = "Disconnected";
	return 0;
}

int get_wan_device_ppp_interface_ip(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *val;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);

	if (DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP[0] != '\0')
		*value = DEFAULT_WAN_DEVICE_MNG_INTERFACE_IP;
	else {
		intf = section_name(wandcprotoargs->wancprotosection);
		network_get_ipaddr(value, intf); //TODO FUNCTION NOT FOUND
	}
	return 0;
}

int get_wan_device_mng_interface_mac(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *device;
	json_object *res;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	intf = section_name(wandcprotoargs->wancprotosection);
	
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = "");
	if (json_select(res, "device", 0, NULL, &device, NULL) != -1) {
		TRACE("device %s \n", device);
		dmubus_call("network.device", "status", UBUS_ARGS{{"name", device}}, 1, &res);
		if (res) {
			json_select(res, "macaddr", 0, NULL, value, NULL);
			return 0;
		}
	}
	*value = "";
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
			return 0;
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
			return 0;
	}
	return 0;
}
