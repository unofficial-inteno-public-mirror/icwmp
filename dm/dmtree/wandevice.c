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

inline int entry_wandevice_sub_instance(struct dmctx *ctx, int i, char *cwritable, bool notif_permission);
inline int entry_wandevice_wanconnectiondevice_instance(struct dmctx *ctx, int i, char *iwan, char *fwan, char *cwritable, bool notif_permission, bool ipn_perm, bool pppn_perm);
inline int entry_wandevice_wanprotocolconnection_instance(struct dmctx *ctx, char *idev, char *iwan, char *iconp, int proto, bool notif_permission, bool forced_inform_eip, int forced_notify);

enum WAN_TYPE_CONNECTION {
	WAN_IP_CONNECTION,
	WANPPPConnection
};

enum enum_wan_dsl {
	WAN_DSL_NODSL,
	WAN_DSL_ADSL,
	WAN_DSL_VDSL
};

enum enum_wandevice_idx {
	WAN_IDX_ETH,
	WAN_IDX_ATM,
	WAN_IDX_PTM
};

enum enum_wan_proto {
	WAN_PROTO_NIL,
	WAN_PROTO_PPP,
	WAN_PROTO_IP
};

struct wan_device 
{
	char *instance;
	char *fdev;
	char *stype;
	char *cdev;
};

struct wan_device wan_devices[WAN_DEVICE] = {
	[WAN_IDX_ETH] = {"1", NULL, "ethernet_interface", "layer2_interface_ethernet"},
	[WAN_IDX_ATM] = {"2", "atm", "atm_bridge", "layer2_interface_adsl"},
	[WAN_IDX_PTM] = {"3", "ptm", "vdsl_interface", "layer2_interface_vdsl"}
};

static char *default_wan;
static char *default_wan_ifname;
static int default_wan_proto;
static char *eth_wan = NULL;

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
	char *wan_ifname;
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

inline int init_wancdevargs(struct dmctx *ctx, struct uci_section *s, int index, char *fwan, char *iwan, char *wan_ifname)
{
	struct wancdevargs *args = &cur_wancdevargs;
	ctx->args = (void *)args;
	args->wandevsection = s;
	args->index = index;
	args->fwan = fwan;
	args->iwan = iwan;
	args->wan_ifname = wan_ifname;
	return 0;
}

void remove_vid_from_ifname(char *vid, char *ifname, char *new_ifname)
{
	char *sv, *pch, *p = new_ifname, *spch;
	new_ifname[0] = '\0';
	bool append;

	ifname = dmstrdup(ifname);
	pch = strtok_r(ifname, " ", &spch);
	while (pch != NULL) {
		append = false;
		char *sv = strchr(pch, '.');
		if (sv) {
			sv++;
			if (strcmp(sv, vid) != 0) {
				append = true;
			}
		}
		else {
			append = true;
		}
		if(append) {
			if (p == new_ifname) {
				dmstrappendstr(p, pch);
			}
			else {
				dmstrappendchr(p, ' ');
				dmstrappendstr(p, pch);
			}
		}
		pch = strtok_r(NULL, " ", &spch);
	}
	dmstrappendend(p);
	dmfree(ifname);
}

inline int add_wvlan(char *baseifname, char *ifname, char *vid, char *prioprity) {
	struct uci_section *ss = NULL, *vlan_interface_s;
	char *add_value;

	dmuci_add_section("layer2_interface_vlan", "vlan_interface", &vlan_interface_s, &add_value);
	dmuci_set_value_by_section(vlan_interface_s, "baseifname", baseifname);
	dmuci_set_value_by_section(vlan_interface_s, "ifname", ifname);
	dmuci_set_value_by_section(vlan_interface_s, "vlan8021q", vid);
	dmuci_set_value_by_section(vlan_interface_s, "vlan8021p", prioprity);
	return 0;
}

int get_cfg_layer2idx(char *pack, char *section_type, char *option, int shift)
{
	char *si, *value;
	int idx = 0, max = -1;
	struct uci_section *s = NULL;

	uci_foreach_sections(pack, section_type, s) {
		dmuci_get_value_by_section_string(s, option, &value);
		si = value + shift;
		idx = atoi(si);
		if (idx > max)
			max = idx;
	}
	return (max + 1);
}

int check_multiwan_interface(struct uci_section *interface_section, char *fwan)
{
	char *ifname, *type, *device = NULL, *dup, *pch, *spch;
	json_object *res;
	int cn = 0;
	
	dmuci_get_value_by_section_string(interface_section, "type", &type);
	dmuci_get_value_by_section_string(interface_section, "ifname", &ifname);
	dup = dmstrdup(ifname);
	pch = strtok_r(dup, " ", &spch);
	while (pch != NULL) {
		if (strstr(pch, "atm"))
			cn++;
		if (strstr(pch, "ptm"))
			cn++;
		if (strstr(pch, eth_wan))
			cn++;
		pch = strtok_r(NULL, " ", &spch);
	}
	dmfree(dup);
	if (type[0] == '\0' || cn < 2)
		return 0;

	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(interface_section)}}, 1, &res);
	if (res) {
		json_select(res, "device", -1, NULL, &device, NULL);
	}
	if (!res || device[0] == '\0') {
		cn = 0;
		dup = dmstrdup(ifname);
		pch = strtok_r(dup, " ", &spch);
		while (pch != NULL) {
			if (strstr(pch, "atm")) {
				cn++;
				break;
			}
			if (strstr(pch, "ptm")) {
				cn++;
				break;
			}
			if (strstr(pch, eth_wan)) {
				cn++;
				break;
			}
			pch = strtok_r(NULL, " ", &spch);
		}
		dmfree(dup);
		if (cn && strstr(pch, fwan))
			return 0;
		return -1;
	}
	else if (strstr(device, fwan)) {
		return 0;
	}	
	return -1;
}
int wan_remove_dev_interface(struct uci_section *interface_setion, char *dev)
{
	char *ifname, new_ifname[64], *p, *pch, *spch;
	new_ifname[0] = '\0';
	p = new_ifname;
	dmuci_get_value_by_section_string(interface_setion, "ifname", &ifname);
	ifname = dmstrdup(ifname);
	for (pch = strtok_r(ifname, " ", &spch); pch; pch = strtok_r(NULL, " ", &spch)) {
		if (!strstr(pch, dev)) {
			if (new_ifname[0] != '\0') {
				dmstrappendchr(p, ' ');
			}
			dmstrappendstr(p, pch);
		}
	}
	dmstrappendend(p);
	dmfree(ifname);
	if (new_ifname[0] == '\0') {
		dmuci_delete_by_section(interface_setion, NULL, NULL);
	}
	else {
		dmuci_set_value_by_section(interface_setion, "ifname", new_ifname);
	}
	return 0;
}
/****** ADD-DEL OBJECT *******************/
char *get_last_instance_proto(char *package, char *section, char *opt_inst, char *opt_check, char *value_check, char *opt_check1, int value_check1)
{
	struct uci_section *s;
	char *instance = NULL;
	char *value = NULL;
	int proto = -1;
	
	uci_foreach_option_cont(package, section, opt_check, value_check, s) {
		dmuci_get_value_by_section_string(s, opt_check1, &value);
		if (strstr(value, "ppp"))
			proto = WAN_PROTO_PPP;
		else if (strcmp(value, "dhcp") == 0 || strcmp(value, "static") == 0)
			proto = WAN_PROTO_IP;
		else
			proto = WAN_PROTO_NIL;
		if (proto == value_check1) {
			instance = update_instance(s, instance, opt_inst);
		}		
	}
	return instance;
}

int add_wan_wanconnectiondevice(struct dmctx *ctx, char **instancepara)
{
	int iwan, idx;
	char *value;
	char *instance;
	char ifname[16] = {0};
	char buf[16] = {0};
	struct uci_section *s = NULL;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;

	if (wandargs->instance == WAN_INST_ATM) {
		idx = get_cfg_layer2idx("layer2_interface_adsl", "atm_bridge", "baseifname", sizeof("atm")-1);
		sprintf(buf, "atm%d",idx);
		sprintf(ifname,"%s.1",buf);
		instance = get_last_instance_lev2("layer2_interface_adsl", "atm_bridge", "waninstance", "baseifname", "atm");
		dmuci_add_section("layer2_interface_adsl", "atm_bridge", &s, &value);
		dmuci_set_value_by_section(s, "baseifname", buf);
		dmuci_set_value_by_section(s, "bridge", "0");
		dmuci_set_value_by_section(s, "encapseoa", "llcsnap_eth");
		dmuci_set_value_by_section(s, "ifname", ifname);
		dmuci_set_value_by_section(s, "link_type", "EoA");
		dmuci_set_value_by_section(s, "unit", buf+3);
		dmuci_set_value_by_section(s, "vci", "35");
		dmuci_set_value_by_section(s, "vpi", "8");
		*instancepara = update_instance(s, instance, "waninstance");
		return 0;
	}
	else if (wandargs->instance == WAN_INST_PTM) {
		idx = get_cfg_layer2idx("layer2_interface_vdsl", "vdsl_interface", "baseifname", sizeof("ptm")-1);
		sprintf(buf,"ptm%d", idx);
		sprintf(ifname,"%s.1",buf);
		instance = get_last_instance_lev2("layer2_interface_vdsl", "vdsl_interface", "waninstance", "baseifname", "ptm");
		dmuci_add_section("layer2_interface_vdsl", "vdsl_interface", &s, &value);
		dmuci_set_value_by_section(s, "baseifname", buf);
		dmuci_set_value_by_section(s, "bridge", "0");
		dmuci_set_value_by_section(s, "ifname", ifname);
		dmuci_set_value_by_section(s, "unit", buf+3);
		*instancepara = update_instance(s, instance, "waninstance");
		return 0;
	}
	return FAULT_9005;
}

int delete_wan_wanconnectiondevice_all(struct dmctx *ctx)
{
	struct uci_section *s = NULL; 
	struct uci_section *ss = NULL;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	
	uci_foreach_option_cont(wan_devices[wandargs->instance - 1].cdev, wan_devices[wandargs->instance - 1].stype, "baseifname", wandargs->fdev, s) {
		if (ss)
			dmuci_delete_by_section(ss, NULL, NULL);
		ss = s;
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	
	ss = NULL;
	uci_foreach_option_cont("network", "interface", "ifname", wandargs->fdev, s) {
		if (ss)
			wan_remove_dev_interface(ss, wandargs->fdev);
		ss = s;
	}
	if (ss != NULL)
		wan_remove_dev_interface(ss, wandargs->fdev);
	return 0;
}

int delete_wan_wanconnectiondevice(struct dmctx *ctx)
{
	struct uci_section *s = NULL; 
	struct uci_section *ss = NULL;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	
	dmuci_delete_by_section(wandcdevargs->wandevsection, NULL, NULL);
	uci_foreach_option_cont("network", "interface", "ifname", wandcdevargs->fwan, s) {
		if (ss)
			wan_remove_dev_interface(ss, wandcdevargs->fwan);
		ss = s;
	}
	if (ss != NULL)
		wan_remove_dev_interface(ss, wandcdevargs->fwan);
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
	char *instance;
	instance = get_last_instance_proto("network", "interface", "conipinstance", "ifname", wandcdevargs->fwan, "proto", WAN_PROTO_IP);
	sprintf(sname,"wan_%s_%s_%d_%s", wan_devices[wandcdevargs->index].instance, wandcdevargs->iwan, WAN_IP_CONNECTION, instance);
	sprintf(ifname, "%s.1", wandcdevargs->fwan);
	dmuci_set_value("network", sname, NULL, "interface");
	dmuci_set_value("network", sname, "ifname", ifname);
	dmuci_set_value("network", sname, "proto", "dhcp");
	dmasprintf(instancepara, "%d", instance ? atoi(instance) + 1 : 1); //MEM WILL BE FREED IN DMMEMCLEAN
	dmuci_set_value("network", sname, "conipinstance", *instancepara);
	return 0;
}

int delete_wan_wanipconnectiondevice_all(struct dmctx *ctx)
{
	char *ifname, *iproto;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;

	dmuci_get_value_by_section_string(wandcdevargs->wandevsection, "ifname", &ifname);
	uci_foreach_option_eq("network", "interface", "ifname", ifname, s) {
		dmuci_get_value_by_section_string(s, "proto", &iproto);
		if (strcmp(iproto, "dhcp") == 0 || strcmp(iproto, "static") == 0) {
			if (ss)
				dmuci_delete_by_section(ss, NULL, NULL);
			ss = s;
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
	char *instance;

	instance = get_last_instance_proto("network", "interface", "conpppinstance", "ifname", wandcdevargs->fwan, "proto", WAN_PROTO_PPP);
	sprintf(sname,"wan_%s_%s_%d_%s", wan_devices[wandcdevargs->index].instance, wandcdevargs->iwan, WANPPPConnection, instance);
	sprintf(ifname, "%s.1", wandcdevargs->fwan);
	dmuci_set_value("network", sname, NULL, "interface");
	dmuci_set_value("network", sname, "ifname", ifname);
	dmuci_set_value("network", sname, "proto", "pppoe");
	dmasprintf(instancepara, "%d", instance ? atoi(instance) + 1 : 1);
	dmuci_set_value("network", sname, "conpppinstance", *instancepara); //MEM WILL BE FREED IN DMMEMCLEAN
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
		if (strstr(iproto, "ppp")) { //CHECK IF WE CAN OPTIMISE AND IF iproto can be pppoa
			if (ss)
				dmuci_delete_by_section(ss, NULL, NULL);
			ss = s;
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

	*value = "";
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
	*value = "";

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
	char buf[512], *val = "", *pch, *spch, *pch2, *spch2, *dup;
	int dsl, pp, r;
	*value = "None";

	if (wandargs->instance != WAN_INST_ETH) {
		dsl = get_wan_device_wan_dsl_traffic();
		if (!(wandargs->instance == WAN_INST_ATM && dsl == WAN_DSL_ADSL) &&
			!(wandargs->instance == WAN_INST_PTM && dsl == WAN_DSL_VDSL) ) {
			return 0;
		}

		buf[0] = '\0';
		pp = dmcmd("adsl", 2, "info", "--state"); //TODO wait ubus command
		if (pp) {
			r = dmcmd_read(pp, buf, 512);
			close(pp);
		}
		for (pch = strtok_r(buf, "\n\r", &spch); pch; pch = strtok_r(NULL, "\n\r", &spch)) {
			if (!strstr(pch, "Upstream rate"))
				continue;
			if (strstr(pch, "Max:"))
				continue;
			dup = dmstrdup(pch);
			pch2 = strtok_r(dup, " \t", &spch2);
			pch2 = strtok_r(NULL, " \t", &spch2);
			if (pch2 != NULL) {
				if (strcasecmp(pch2, "FAST") == 0) {
					*value = "Fast";
				}
				else if (strcasecmp(pch2, "INTR") == 0) {
					*value = "Interleaved";
				}
				else {
					*value = "None";
				}
			}
			dmfree(dup);
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
			dmasprintf(&attn_down_x100, "%d", (atoi(attn_down_x100) / 10));// MEM WILL BE FREED IN DMMEMCLEAN
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
			dmasprintf(&snr_down_x100, "%d", (atoi(snr_down_x100) / 10));// MEM WILL BE FREED IN DMMEMCLEAN
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
			dmasprintf(&attn_up_x100, "%d", (atoi(attn_up_x100) / 10)); // MEM WILL BE FREED IN DMMEMCLEAN
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
			dmasprintf(&snr_up_x100, "%d", (atoi(snr_up_x100) / 10));// MEM WILL BE FREED IN DMMEMCLEAN
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
	char *val = "0";
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	
	if (wandargs->instance == WAN_INST_ATM) {
		dmuci_get_option_value_string("layer2_interface", "capabilities", "AnnexM", &val);
		if (val[0] != '\0') {
			if (strcasecmp(val, "enabled") == 0) {
				*value = "1";
				return 0;
			}
		}
	}
	return 0;
}

int set_annexm_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;

	struct wanargs *wandargs = (struct wanargs *)ctx->args;
		
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (wandargs->instance != WAN_INST_ATM) {
				return 0;
			}
			string_to_bool(value, &b);
			if(b) {
				dmuci_set_value("layer2_interface", "capabilities", "AnnexM", "Enabled");					
			}
			else
				dmuci_set_value("layer2_interface", "capabilities", "AnnexM", "");				
			return 0;
	}
	return 0;
}

//TO CHECK IF NO VALUE RETURNE BY UBUS CMD
int get_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *val;
	bool b;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = "0");
	json_select(res, "up", -1, NULL, &val, NULL);
	if (val) {
		string_to_bool(val, &b);
		if (b)
			*value = "1";
		else
			*value = "0";
	}
	return 0;
}

int set_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{		
	struct uci_section *s;
	json_object *res;
	char *enable, *type, *device, json_name[32];
	bool b;
	bool enable_b;
	struct wanargs *wandargs = (struct wanargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET: //ENHANCEMENT look for function to start and stop the ethernet driver
			string_to_bool(value, &b);
			dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
			if (res) {
				json_select(res, "up", 0, NULL, &enable, NULL);
				string_to_bool(enable, &enable_b);
				if (b == enable_b)
					return 0;
			}
			if(b) {
				uci_foreach_option_cont("network", "interface", "ifname", wandargs->fdev, s) {
					sprintf(json_name, "network.interface.%s", section_name(s));
					dmubus_call_set(json_name, "up", UBUS_ARGS{}, 0);
				}
			}
			else {
				uci_foreach_option_cont("network", "interface", "ifname", wandargs->fdev, s) {
					dmuci_get_value_by_section_string(s, "type", &type);
					if (strcmp(type, "anywan") != 0 && strcmp(type, "multiwan") != 0) {
						sprintf(json_name, "network.interface.%s", section_name(s));
						dmubus_call_set(json_name, "down", UBUS_ARGS{}, 0);
						goto end;
					}
					else {
						dmubus_call("network.interface", "status", UBUS_ARGS{{"name", section_name(s)}}, 1, &res);
						if (res) {
							json_select(res, "device", -1, NULL, &device, NULL);
							if (strstr(device, wandargs->fdev)) {
								sprintf(json_name, "network.interface.%s", section_name(s));
								dmubus_call_set(json_name, "down", UBUS_ARGS{}, 0);
								goto end;
							}
						}
					}
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
	bool b;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
	DM_ASSERT(res, *value = "Disabled");
	json_select(res, "up", 0, NULL, value, NULL);
	if (*value) {
		string_to_bool(*value, &b);
		if (!b)
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
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", wandargs->fdev}}, 1, &res);
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
	if (!(*value) || (*value)[0] == '\0') {
		*value = "0";
	}
	return 0;
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
	
	uci_foreach_option_cont("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) { 
		dmuci_get_value_by_section_string(s, "vpi", &vpi);
		dmuci_get_value_by_section_string(s, "vci", &vci);
		dmasprintf(value, "PVC: %s/%s", vpi, vci); // MEM WILL BE FREED IN DMMEMCLEAN
		break;
	}
	return 0;
}

int set_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *vpi = NULL, *vci = NULL, *spch, *val;
	struct uci_section *s;
	struct wancdevargs *wandcdevargs = (struct wancdevargs *)ctx->args;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_cont("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) {
				if (strstr(value, "PVC: "))
					value += 5;
				else
					return 0;
				val = dmstrdup(value);
				vpi = strtok_r(val, "/", &spch);
				if (vpi) {
					vci = strtok_r(NULL, "/", &spch);
				}
				if (vpi && vci) {
					dmuci_set_value_by_section(s, "vpi", vpi);
					dmuci_set_value_by_section(s, "vci", vci);
				}
				dmfree(val);
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
	
	uci_foreach_option_cont("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) {
		dmuci_get_value_by_section_string(s,"link_type", &type);
		if (strcmp(type, "EoA") == 0 ) {
			type = "encapseoa";
		} else if (strcmp(type, "PPPoA") == 0) {
			type = "encapspppoa";
		} else if (strcmp(type, "IPoA") == 0) {
			type = "encapsipoa";
		}
		dmuci_get_value_by_section_string(s, type, &encapsulation);
		if (strstr(encapsulation, "vcmux")) {
			*value = "VCMUX";
		}
		else if (strstr(encapsulation, "llc")) {
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
			uci_foreach_option_cont("layer2_interface_adsl", "atm_bridge", "ifname", wandcdevargs->fwan, s) {
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

/************************************************************************** 
**** ****  function related to get_wandevice_wanprotoclconnection_parameters **** ****
***************************************************************************/

int get_interface_enable_wanproto(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *intf = section_name(wandcprotoargs->wancprotosection);
	return get_interface_enable_ubus(intf, refparam, ctx, value);
}

int set_interface_enable_wanproto(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *intf = section_name(wandcprotoargs->wancprotosection);
	return set_interface_enable_ubus(intf, refparam, ctx, action, value);
}

int get_interface_firewall_enabled_wanproto(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *intf = section_name(wandcprotoargs->wancprotosection);
	return get_interface_firewall_enabled(intf, refparam, ctx, value);
}

int set_interface_firewall_enabled_wanproto(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *intf = section_name(wandcprotoargs->wancprotosection);
	return set_interface_firewall_enabled(intf, refparam, ctx, action, value);
}
//THE same as get_wan_device_ppp_status WHY DO WE CREATE A SEPARATED FUNCTION
int get_wan_device_mng_status(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res = NULL;
	char *pending = NULL;
	char *intf;
	char *status = NULL;
	char *uptime = NULL;
	bool b = false;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);

	intf = section_name(wandcprotoargs->wancprotosection);
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = "Disconnected");
	if (json_select(res, "up", 0, NULL, &status, NULL) != -1)
	{
		if (strcasecmp(status, "true") == 0) {
			json_select(res, "uptime", 0, NULL, &uptime, NULL);
			json_select(res, "pending", 0, NULL, &pending, NULL);
			string_to_bool(pending, &b);
		}
	}
	if (uptime && atoi(uptime) > 0)
		*value = "Connected";
	else if (pending && b) {
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
	
	intf = section_name(wandcprotoargs->wancprotosection);
	network_get_ipaddr(value, intf);
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
	char *type = "";
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
		if (masq[0] == '1' && masq[1] == '\0') {
			dmuci_get_value_by_section_string(s, "network", &network);
			if (strstr(network, intf)) {
				*value = "1";
				return 0;
			}
		}
	}
	*value = "0";
	return 0;
}

int set_wan_ip_link_connection_nat_enabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
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
			string_to_bool(value, &b);
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



int get_wan_igmp_rule_idx(char *iface, struct uci_section **rule, struct uci_section **zone, char **enable)
{
	char *input, *proto, *target, *zname;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	*enable = "1";
	*rule = NULL;
	
	uci_foreach_option_cont("firewall", "zone", "network", iface, *zone) {
		dmuci_get_value_by_section_string(*zone, "input", &input);
		if (strcmp(input, "DROP") == 0)
			*enable = "0";
		dmuci_get_value_by_section_string(*zone, "name", &zname);
		uci_foreach_option_cont("firewall", "rule", "src", zname, *rule) {
			dmuci_get_value_by_section_string(*rule, "proto", &proto);
			if (strcmp(proto, "igmp") == 0) {
				dmuci_get_value_by_section_string(*rule, "enabled", enable);
				if ((*enable)[0] != '\0') {
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
	bool b;
	int found = 0;
	char *val, *intf, *enable, *zname, buf[32];
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
			string_to_bool(value, &b);
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
				dmuci_add_section("firewall", "rule", &rule, &val);
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
	bool b;
	char *intf;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	
	intf = section_name(wandcprotoargs->wancprotosection);
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				value = "1";
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
	bool bstatus = false, bpend = false;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);

	intf = section_name(wandcprotoargs->wancprotosection);
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = "");
	if (json_select(res, "up", 0, NULL, &status, NULL) != -1)
	{
		string_to_bool(status, &bstatus);
		if (bstatus) {
			json_select(res, "uptime", 0, NULL, &uptime, NULL);
			json_select(res, "pending", 0, NULL, &pending, NULL);
			string_to_bool(pending, &bpend);
		}
	}
	if (uptime && atoi(uptime) > 0)
		*value = "Connected";
	else if (pending && bpend)
		*value = "Pending Disconnect";
	else
		*value = "Disconnected";
	return 0;
}

int get_wan_device_ppp_interface_ip(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *val;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);

	intf = section_name(wandcprotoargs->wancprotosection);
	network_get_ipaddr(value, intf);
	return 0;
}

int get_wan_device_mng_interface_mac(char *refparam, struct dmctx *ctx, char **value)
{
	char *intf, *device;
	json_object *res;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	*value = "";
	intf = section_name(wandcprotoargs->wancprotosection);
	
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", intf}}, 1, &res);
	DM_ASSERT(res, *value = "");
	if (json_select(res, "device", 0, NULL, &device, NULL) != -1) {
		dmubus_call("network.device", "status", UBUS_ARGS{{"name", device}}, 1, &res);
		if (res) {
			json_select(res, "macaddr", 0, NULL, value, NULL);
			return 0;
		}
	}
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

int get_wan_ip_link_connection_name(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *args = (struct wancprotoargs *)ctx->args;
	*value = dmstrdup(section_name(args->wancprotosection));
	return 0;
}

int set_wan_ip_link_connection_connection_name(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct wancprotoargs *args;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			args = (struct wancprotoargs *)ctx->args;
			dmuci_rename_section_by_section(args->wancprotosection, value);
			return 0;
	}
	return 0;
}

int get_layer2_interface(char *wan_name, char **ifname)
{
	char *wtype, *wifname, *device, *dup, *pch, *spch;
	int vid;
	json_object *res;

	*ifname = "";
	dmuci_get_option_value_string("network", wan_name, "ifname", &wifname);
	if(wifname[0] == '\0')
		return 0;

	dmuci_get_option_value_string("network", wan_name, "type", &wtype);
	if(wtype[0] == '\0' || strcmp(wtype, "anywan") == 0)
	{
		dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", wan_name}}, 1, &res);
		if (res) {
			json_select(res, "device", -1, NULL, &device, NULL);
			if(device[0] != '\0') {
				if (atoi(device + 5) > 1) {
					*ifname = device;
					return 0;
				}
			}
		}
	}
	dup = dmstrdup(wifname); // MEM will be freed in the DMMCLEAN
	for (pch = strtok_r(dup, " ", &spch); pch != NULL; pch = strtok_r(NULL, " ", &spch)) {
		if (strstr(pch, "atm") ||
			strstr(pch, "ptm") ||
			strstr(pch, eth_wan)) {
			if (atoi(pch + 5) > 1) {
				*ifname = pch;
				return 0;
			}
		}
	}
	return  0;
}

int get_wan_ip_link_connection_vid(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *wan_name = section_name(wandcprotoargs->wancprotosection);
	struct uci_section *ss = NULL;
	char *ifname;

	*value = "0";
	get_layer2_interface(wan_name, &ifname);
	if (ifname[0] == '\0') {
		uci_foreach_sections("dmmap", wan_name, ss)
		{
			dmuci_get_value_by_section_string(ss, "vid", value);
		}
		return 0;
	}
	else
		*value = ifname + 5;
	return 0;
}

int set_wan_ip_link_connection_vid(char *refparam, struct dmctx *ctx, int action, char *value)
{
	struct uci_section *ss = NULL, *w_vlan, *s_last = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *add_value, *ifname, *vid, *prio;
	char *wan_name = section_name(wandcprotoargs->wancprotosection);
	bool found = false;
	char *p, *q, *wifname, *baseifname="";
	char r_new_wifname[128];
	char a_new_wifname[128];
	char v_ifname[16];

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			get_layer2_interface(wan_name, &ifname);
			if (ifname[0] != '\0')
			{
				uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", ifname, ss) {
					dmuci_get_value_by_section_string(ss, "vlan8021q", &vid);
					if (strcmp(vid, value) == 0)
						return 0;
					dmuci_get_option_value_string("network", wan_name, "ifname", &wifname);
					if (wifname[0] == '\0')
						return 0;
					remove_vid_from_ifname(vid, wifname, r_new_wifname);
					dmuci_get_value_by_section_string(ss, "baseifname", &baseifname);
					p = a_new_wifname;
					q = v_ifname;
					dmuci_get_value_by_section_string(ss, "vlan8021q", &vid);
					dmstrappendstr(q, baseifname);
					dmstrappendchr(q, '.');
					dmstrappendstr(q, value);
					dmstrappendend(q);
					dmstrappendstr(p, v_ifname);
					dmstrappendchr(p, ' ');
					dmstrappendstr(p, r_new_wifname);
					dmstrappendend(p);
					dmuci_set_value_by_section(ss, "ifname", v_ifname);
					dmuci_set_value_by_section(ss, "vlan8021q", value);
					dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "ifname", a_new_wifname);
				}
				return 0;
			}
			uci_foreach_sections("dmmap", wan_name, ss)
			{
				dmuci_set_value_by_section(ss, "vid", value);
				dmuci_get_value_by_section_string(ss, "baseifname", &baseifname);
				dmuci_get_value_by_section_string(ss, "priority", &prio);
				if (strstr(baseifname, "atm0") ||  strstr(baseifname, "ptm0") || strstr(baseifname, eth_wan)) {
					dmuci_get_option_value_string("network", wan_name, "ifname", &wifname);
					p = a_new_wifname;
					q = v_ifname;
					dmstrappendstr(q, baseifname);
					dmstrappendchr(q, '.');
					dmstrappendstr	(q, value);
					dmstrappendend(q);
					dmstrappendstr(p, v_ifname);
					dmstrappendchr(p, ' ');
					dmstrappendstr(p, wifname);
					dmstrappendend(p);
					add_wvlan(baseifname, v_ifname, value, (prio) ? prio : "0");
					dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "ifname", a_new_wifname);
					s_last = ss;
					goto remove_section;
				}
				return 0;
			}
			dmuci_add_section("dmmap", wan_name, &w_vlan, &add_value);
			dmuci_set_value_by_section(w_vlan, "vid", value);
			return 0;

	remove_section:
		dmuci_delete_by_section(s_last, NULL, NULL);
		return 0;
	}
	return 0;
}
int get_wan_ip_link_connection_vpriority(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *wan_name = section_name(wandcprotoargs->wancprotosection);
	struct uci_section *ss = NULL;
	char *ifname;
	*value = "0";

	get_layer2_interface(wan_name, &ifname);
	if (ifname[0] == '\0') {
		uci_foreach_sections("dmmap", wan_name, ss)
		{
			dmuci_get_value_by_section_string(ss, "priority", value);
		}
		return 0;
	}
	uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", ifname, ss)
	{
		dmuci_get_value_by_section_string(ss, "vlan8021p", value);
	}
	return 0;
}

int set_wan_ip_link_connection_vpriority(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *ifname;
	struct uci_section *ss = NULL;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *wan_name = section_name(wandcprotoargs->wancprotosection);

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			get_layer2_interface(wan_name, &ifname);
			if (ifname[0] == '\0') {
				uci_foreach_sections("dmmap", wan_name, ss)
				{
					dmuci_set_value_by_section(ss, "priority", value);
				}
				return 0;
			}
			uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", ifname, ss)
			{
				dmuci_set_value_by_section(ss, "vlan8021p", value);
			}
			return 0;
	}
	return 0;
}

int get_wan_ip_link_connection_layer2_interface(char *refparam, struct dmctx *ctx, char **value)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *wan_name = section_name(wandcprotoargs->wancprotosection);
	struct uci_section *ss = NULL;
	char *ifname, *p;

	*value = "";
	get_layer2_interface(wan_name, &ifname);
	if (ifname[0] == '\0') {
		uci_foreach_sections("dmmap", wan_name, ss)
		{
			dmuci_get_value_by_section_string(ss, "baseifname", value);
		}
		return 0;
	}
	*value = dmstrdup(ifname); // MEM will be freed in the DMMCLEAN
	p = strchr(*value, '.');
	if (p)
		*p = '\0';
	return 0;
}

int set_wan_ip_link_connection_layer2_interface(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *ifname, *add_value;
	struct uci_section *ss = NULL, *w_vlan, *s_last;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *wan_name = section_name(wandcprotoargs->wancprotosection);
	char *p, *q, *wifname, *baseifname = NULL, *vid = NULL, *prio = NULL;
	char r_new_wifname[128];
	char a_new_wifname[128];
	char ifname_buf[16];

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			get_layer2_interface(wan_name, &ifname);
			if (ifname[0] != '\0')
			{
				uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", ifname, ss) {
					dmuci_get_value_by_section_string(ss, "baseifname", &baseifname);
					if (strcmp(baseifname, value) == 0)
						return 0;
					dmuci_get_option_value_string("network", wan_name, "ifname", &wifname);
					if (wifname[0] == '\0')
						return 0;
					dmuci_get_value_by_section_string(ss, "vlan8021q", &vid);
					remove_vid_from_ifname(vid, wifname, r_new_wifname);
					p = a_new_wifname;
					q = ifname_buf;
					dmstrappendstr(q, value);
					dmstrappendchr(q, '.');
					dmstrappendstr(q, vid);
					dmstrappendend(q);
					dmstrappendstr(p, ifname_buf);
					if (r_new_wifname[0] != '\0')
						dmstrappendchr(p, ' ');
					dmstrappendstr(p, r_new_wifname);
					dmstrappendend(p);
					dmuci_set_value_by_section(ss, "baseifname", value);
					dmuci_set_value_by_section(ss, "ifname", ifname_buf);
					dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "ifname", a_new_wifname);
				}
				return 0;
			}
			uci_foreach_sections("dmmap", wan_name, ss)
			{
				dmuci_set_value_by_section(ss, "baseifname", value);
				dmuci_get_value_by_section_string(ss, "vid", &vid);
				dmuci_get_value_by_section_string(ss, "priority", &prio);
				if (atoi(vid) > 1) {
					dmuci_get_option_value_string("network", wan_name, "ifname", &wifname);
					p = a_new_wifname;
					q = ifname_buf;
					dmstrappendstr(q, value);
					dmstrappendchr(q, '.');
					dmstrappendstr(q, vid);
					dmstrappendend(q);
					dmstrappendstr(p, ifname_buf);
					dmstrappendchr(p, ' ');
					dmstrappendstr(p, wifname);
					dmstrappendend(p);
					add_wvlan(value, ifname_buf, vid, (prio) ? prio : "0");
					dmuci_set_value_by_section(wandcprotoargs->wancprotosection, "ifname", a_new_wifname);
					s_last = ss;
					goto remove_section;
				}
				return 0;
			}
			dmuci_add_section("dmmap", wan_name, &w_vlan, &add_value);
			dmuci_set_value_by_section(w_vlan, "baseifname", value);
			return 0;

	remove_section:
		dmuci_delete_by_section(s_last, NULL, NULL);
		return 0;
	}
	return 0;
}

inline int ubus_get_wan_stats(json_object *res, char **value, char *stat_mod)
{
	char *proto;

	dmuci_get_option_value_string("network", section_name(cur_wancprotoargs.wancprotosection), "proto", &proto);
	if (strcmp(proto, "dhcp") == 0 || strcmp(proto, "pppoe") == 0)
	{
		dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_wancdevargs.wan_ifname}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "statistics", 0, stat_mod, value, NULL);
		return 0;
	}
	*value = "";
	return 0;
}

int get_wan_link_connection_eth_bytes_received(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "rx_bytes");
	return 0;
}

int get_wan_link_connection_eth_bytes_sent(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "tx_bytes");
	return 0;
}

int get_wan_link_connection_eth_pack_received(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "rx_packets");
	return 0;
}

int get_wan_link_connection_eth_pack_sent(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	ubus_get_wan_stats(res, value, "tx_packets");
	return 0;
}

/////////////SUB ENTRIES///////////////
inline int entry_wandevice_sub(struct dmctx *ctx)
{
	int i;
	bool notif_permission;
	char *cwritable;
	char *defwanproto;

	dmuci_get_option_value_string("network", default_wan, "ifname", &default_wan_ifname);
	dmuci_get_option_value_string("network", default_wan, "proto", &defwanproto);
	dmuci_get_option_value_string("layer2_interface_ethernet", "Wan", "baseifname", &eth_wan);
	wan_devices[WAN_IDX_ETH].fdev = eth_wan;

	if (strstr(defwanproto, "ppp"))
		default_wan_proto = WAN_PROTO_PPP;
	else if (strcmp(defwanproto, "dhcp") == 0 || strcmp(defwanproto, "static") == 0)
		default_wan_proto = WAN_PROTO_IP;
	else
		default_wan_proto = WAN_PROTO_NIL;

	for (i=0; i < WAN_DEVICE; i++) {
		init_wanargs(ctx, i+1, wan_devices[i].fdev);

		if (strstr(default_wan_ifname, wan_devices[i].fdev))
			notif_permission = false;
		else
			notif_permission = true;

		if (i == WAN_IDX_ETH)
			cwritable = "0";
		else
			cwritable = "1";

		SUBENTRY(entry_wandevice_sub_instance, ctx, i, cwritable, notif_permission);
	}
	return 0;
}

inline int entry_wandevice_wanconnectiondevice(struct dmctx *ctx, int i, char *cwritable)
{
	struct uci_section *s = NULL;
	char *fwan;
	char *wan_ifname;
	char *iwan = NULL;
	char *pack, *stype;
	bool ipn_perm = true;
	bool pppn_perm = true;
	bool notif_permission = true;

	pack = wan_devices[i].cdev;
	stype = wan_devices[i].stype;
	uci_foreach_sections(pack, stype, s) {
		dmuci_get_value_by_section_string(s, "baseifname", &fwan);
		dmuci_get_value_by_section_string(s, "ifname", &wan_ifname);
		if (strstr(default_wan_ifname, fwan)) {
			notif_permission = false;
			if (default_wan_proto == WAN_PROTO_IP) ipn_perm = false;
			else if (default_wan_proto == WAN_PROTO_PPP) pppn_perm = false;
		}
		iwan = update_instance(s, iwan, "waninstance");
		init_wancdevargs(ctx, s, i, fwan, iwan, wan_ifname);
		SUBENTRY(entry_wandevice_wanconnectiondevice_instance, ctx, i, iwan, fwan, cwritable, notif_permission, ipn_perm, pppn_perm);
	}
	return 0;
}

inline int entry_wandevice_wanprotocolconnection(struct dmctx *ctx, char *idev, char *iwan, char *fwan)
{
	struct uci_section *ss = NULL;
	char *pack, *stype, *p;
	char *iconp = NULL, *iconp_ip = NULL, *iconp_ppp = NULL, *iconp_nil = NULL;
	int proto;
	bool notif_permission = true;
	bool forced_inform_eip = false;
	int forced_notify = UNDEF;
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	char *lan_name;


	uci_foreach_option_cont("network", "interface", "ifname", fwan, ss) {
		dmuci_get_value_by_section_string(ss, "proto", &p);
		lan_name = section_name(ss);
		if (strstr(p, "ppp"))
			proto = WAN_PROTO_PPP;
		else if (strcmp(p, "dhcp") == 0 || strcmp(p, "static") == 0)
			proto = WAN_PROTO_IP;
		else
			proto = WAN_PROTO_NIL;
		if (strcmp(lan_name, default_wan) == 0) {
			forced_inform_eip = true;
			forced_notify = 2;
			notif_permission = false;
		}
		if (check_multiwan_interface(ss, fwan) != 0)
			continue;
		init_wancprotoargs(ctx, ss);
		if (proto == WAN_PROTO_IP) {
			iconp_ip = update_instance(ss, iconp_ip, "conipinstance");
			iconp = iconp_ip;
		}
		else if (proto == WAN_PROTO_PPP) {
			iconp_ppp = update_instance(ss, iconp_ppp, "conpppinstance");
			iconp = iconp_ppp;
		}
		SUBENTRY(entry_wandevice_wanprotocolconnection_instance, ctx, idev, iwan, iconp, proto,
				notif_permission, forced_inform_eip, forced_notify);
	}
	return 0;
}

/*************************************************/
int entry_method_root_WANDevice(struct dmctx *ctx)
{
	dmuci_get_option_value_string("cwmp", "cpe", "default_wan_interface", &default_wan);
	IF_MATCH(ctx, DMROOT"WANDevice.") {
		DMOBJECT(DMROOT"WANDevice.", ctx, "0", 0, NULL, NULL, NULL);
		SUBENTRY(entry_wandevice_sub, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_wandevice_sub_instance(struct dmctx *ctx, int i, char *cwritable, bool notif_permission)
{
	char *dev = wan_devices[i].instance;
	IF_MATCH(ctx, DMROOT"WANDevice.", dev) {
		DMOBJECT(DMROOT"WANDevice.%s.", ctx, "0", notif_permission, NULL, NULL, NULL, dev);
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.", ctx, cwritable, notif_permission, add_wan_wanconnectiondevice, delete_wan_wanconnectiondevice_all, NULL, dev);
		DMOBJECT(DMROOT"WANDevice.%s.WANCommonInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL, dev);
		DMPARAM("WANAccessType", ctx, "0", get_wan_device_wan_access_type, NULL, NULL, 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"WANDevice.%s.WANDSLInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL, dev);
		DMPARAM("Status", ctx, "0", get_wan_device_wan_dsl_interface_config_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ModulationType", ctx, "0", get_wan_device_wan_dsl_interface_config_modulation_type, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("DataPath", ctx, "0", get_wan_device_dsl_datapath, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("DownstreamCurrRate", ctx, "0", get_wan_device_dsl_downstreamcurrrate, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("DownstreamMaxRate", ctx, "0", get_wan_device_dsl_downstreammaxrate, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("DownstreamAttenuation", ctx, "0", get_wan_device_dsl_downstreamattenuation, NULL, "xsd:int", 0, 1, UNDEF, NULL);
		DMPARAM("DownstreamNoiseMargin", ctx, "0", get_wan_device_dsl_downstreamnoisemargin, NULL, "xsd:int", 0, 1, UNDEF, NULL);
		DMPARAM("UpstreamCurrRate", ctx, "0", get_wan_device_dsl_upstreamcurrrate, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("UpstreamMaxRate", ctx, "0", get_wan_device_dsl_upstreammaxrate, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("UpstreamAttenuation", ctx, "0", get_wan_device_dsl_upstreamattenuation, NULL, "xsd:int", 0, 1, UNDEF, NULL);
		DMPARAM("UpstreamNoiseMargin", ctx, "0", get_wan_device_dsl_upstreamnoisemargin, NULL, "xsd:int", 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_AnnexMEnable", ctx, "1", get_annexm_status, set_annexm_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		if( i == WAN_IDX_ETH ) {
			DMOBJECT(DMROOT"WANDevice.%s.WANEthernetInterfaceConfig.", ctx, "0", 1, NULL, NULL, NULL, dev);
			DMPARAM("Enable", ctx, "1", get_wan_eth_intf_enable, set_wan_eth_intf_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("Status", ctx, "0", get_wan_eth_intf_status, NULL, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("MACAddress", ctx, "0", get_wan_eth_intf_mac, NULL, NULL, 0, 1, UNDEF, NULL);
			DMOBJECT(DMROOT"WANDevice.%s.WANEthernetInterfaceConfig.Stats.", ctx, "0", 1, NULL, NULL, NULL, dev);
			DMPARAM("BytesSent", ctx, "0", get_wan_eth_intf_stats_tx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("BytesReceived", ctx, "0", get_wan_eth_intf_stats_rx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("PacketsSent", ctx, "0", get_wan_eth_intf_stats_tx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("PacketsReceived", ctx, "0", get_wan_eth_intf_stats_rx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		}
		SUBENTRY(entry_wandevice_wanconnectiondevice, ctx, i, cwritable);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_wandevice_wanconnectiondevice_instance(struct dmctx *ctx, int i, char *iwan, char *fwan, char *cwritable, bool notif_permission, bool ipn_perm, bool pppn_perm)
{
	char *idev = wan_devices[i].instance;
	IF_MATCH(ctx, DMROOT"WANDevice.%s.WANConnectionDevice.%s.", idev, iwan) {
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.", ctx, cwritable, notif_permission, NULL, delete_wan_wanconnectiondevice, NULL, idev, iwan);
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.", ctx, "1", ipn_perm, add_wan_wanipconnection, delete_wan_wanipconnectiondevice_all, NULL, idev, iwan);
		DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.", ctx, "1", pppn_perm, add_wan_wanpppconnection, delete_wan_wanpppconnectiondevice_all, NULL, idev, iwan);
		if (i == WAN_IDX_ATM) {
			DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANDSLLinkConfig.", ctx, "0", 1, NULL, NULL, NULL, idev, iwan);
			DMPARAM("Enable", ctx, "0", get_wan_dsl_link_config_enable, NULL, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("DestinationAddress", ctx, "1", get_wan_dsl_link_config_destination_address, set_wan_dsl_link_config_destination_address, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("ATMEncapsulation", ctx, "1", get_wan_dsl_link_config_atm_encapsulation, set_wan_dsl_link_config_atm_encapsulation, NULL, 0, 1, UNDEF, NULL);
		}
		SUBENTRY(entry_wandevice_wanprotocolconnection, ctx, idev, iwan, fwan);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_wandevice_wanprotocolconnection_instance(struct dmctx *ctx, char *idev, char *iwan, char *iconp, int proto,
														bool notif_permission, bool forced_inform_eip, int forced_notify)
{
	struct wancprotoargs *wandcprotoargs = (struct wancprotoargs *) (ctx->args);
	//ifname is section name
	char linker[32] = "linker_interface:";
	char *lan_name = section_name(wandcprotoargs->wancprotosection);
	strcat(linker, lan_name);
	if (proto == WAN_PROTO_IP) {
		IF_MATCH(ctx, DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.%s.", idev, iwan, iconp) {
			DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.%s.", ctx, "1", notif_permission, NULL, delete_wan_wanconnectiondevice, linker, idev, iwan, iconp);
			DMPARAM("Enable", ctx, "1", get_interface_enable_wanproto, set_interface_enable_wanproto, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("ConnectionStatus", ctx, "0", get_wan_device_mng_status, NULL, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("ExternalIPAddress", ctx, "0", get_wan_device_mng_interface_ip, NULL, NULL, forced_inform_eip, notif_permission, forced_notify, NULL);
			DMPARAM("MACAddress", ctx, "0", get_wan_device_mng_interface_mac, NULL, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("ConnectionType", ctx, "1", get_wan_ip_link_connection_connection_type, set_wan_ip_link_connection_connection_type, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("AddressingType", ctx, "1", get_wan_ip_link_connection_addressing_type, set_wan_ip_link_connection_addressing_type, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("NATEnabled", ctx, "1", get_wan_ip_link_connection_nat_enabled, set_wan_ip_link_connection_nat_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("X_BROADCOM_COM_FirewallEnabled", ctx, "1", get_interface_firewall_enabled_wanproto, set_interface_firewall_enabled_wanproto, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("X_BROADCOM_COM_IGMPEnabled", ctx, "1", get_wan_ip_link_connection_igmp_enabled, set_wan_ip_link_connection_igmp_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("DNSEnabled", ctx, "1", get_wan_ip_link_connection_dns_enabled, set_wan_ip_link_connection_dns_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("DNSOverrideAllowed", ctx, 0, get_empty, NULL, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("Name", ctx, "1", get_wan_ip_link_connection_name, set_wan_ip_link_connection_connection_name, NULL, 0, 1, UNDEF, NULL);
			DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.%s.X_INTENO_COM_VLAN.", ctx, "1", 1, NULL, NULL, NULL, idev, iwan, iconp);
			DMPARAM("VLANID", ctx, "1", get_wan_ip_link_connection_vid, set_wan_ip_link_connection_vid, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("VLANPriority", ctx, "1", get_wan_ip_link_connection_vpriority, set_wan_ip_link_connection_vpriority, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("Layer2Interface", ctx, "1", get_wan_ip_link_connection_layer2_interface, set_wan_ip_link_connection_layer2_interface, NULL, 0, 1, UNDEF, NULL);
			DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANIPConnection.%s.Stats.", ctx, "1", 1, NULL, NULL, NULL, idev, iwan, iconp);
			DMPARAM("EthernetBytesReceived", ctx, "0", get_wan_link_connection_eth_bytes_received, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("EthernetBytesSent", ctx, "0", get_wan_link_connection_eth_bytes_sent, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("EthernetPacketsReceived", ctx, "0", get_wan_link_connection_eth_pack_received, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("EthernetPacketsSent", ctx, "0", get_wan_link_connection_eth_pack_sent, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			return 0;
		}
	}
	else if (proto == WAN_PROTO_PPP) {
		IF_MATCH(ctx, DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.%s.", idev, iwan, iconp) {
			DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.%s.", ctx, "1", notif_permission, NULL, delete_wan_wanconnectiondevice, linker, idev, iwan, iconp);
			DMPARAM("Enable", ctx, "1", get_interface_enable_wanproto, set_interface_enable_wanproto, "xsd:boolean", 0, 1, UNDEF, NULL);
			DMPARAM("ConnectionStatus", ctx, "0", get_wan_device_ppp_status, NULL, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("ExternalIPAddress", ctx, "0", get_wan_device_ppp_interface_ip, NULL, NULL, forced_inform_eip, notif_permission,  forced_notify, NULL);
			DMPARAM("MACAddress", ctx, "0", get_wan_device_mng_interface_mac, NULL, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("Username", ctx, "1", get_wan_device_ppp_username, set_wan_device_username, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("Password", ctx, "1", get_empty, set_wan_device_password, NULL, 0, 1, UNDEF, NULL);
			DMPARAM("Name", ctx, "1", get_wan_ip_link_connection_name, set_wan_ip_link_connection_connection_name, NULL, 0, 1, UNDEF, NULL);
			DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.%s.X_INTENO_COM_VLAN.", ctx, "1", 1, NULL, NULL, NULL, idev, iwan, iconp);
			DMPARAM("VLANID", ctx, "1", get_wan_ip_link_connection_vid, set_wan_ip_link_connection_vid, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("VLANPriority", ctx, "1", get_wan_ip_link_connection_vpriority, set_wan_ip_link_connection_vpriority, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("Layer2Interface", ctx, "1", get_wan_ip_link_connection_layer2_interface, set_wan_ip_link_connection_layer2_interface, NULL, 0, 1, UNDEF, NULL);
			DMOBJECT(DMROOT"WANDevice.%s.WANConnectionDevice.%s.WANPPPConnection.%s.Stats.", ctx, "1", 1, NULL, NULL, NULL, idev, iwan, iconp);
			DMPARAM("EthernetBytesReceived", ctx, "0", get_wan_link_connection_eth_bytes_received, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("EthernetBytesSent", ctx, "0", get_wan_link_connection_eth_bytes_sent, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("EthernetPacketsReceived", ctx, "0", get_wan_link_connection_eth_pack_received, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			DMPARAM("EthernetPacketsSent", ctx, "0", get_wan_link_connection_eth_pack_sent,NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
			return 0;
		}
	}
	return FAULT_9005;
}

