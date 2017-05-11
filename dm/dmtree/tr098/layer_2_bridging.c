/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2015 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 *
 */
#include <libubox/blobmsg_json.h>
#include <json-c/json.h>

#include "dmcwmp.h"
#include "dmuci.h"
#include "dmmem.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "layer_2_bridging.h"

const char *vlan_ifname[3] = {"eth","atm", "ptm"};
char *wan_baseifname = NULL;
struct wan_interface wan_interface_tab[3] = {
{"1", "ethernet", "layer2_interface_ethernet", "ethernet_interface"},
{"2", "adsl", "layer2_interface_adsl", "atm_bridge"},
{"3", "vdsl", "layer2_interface_vdsl", "vdsl_interface"}
};

/*************************************************************
* LAYER2-DM OBJ & PARAM
/************************************************************/

/*** Layer2Bridging. ***/
DMOBJ tLayer2BridgingObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"AvailableInterface", &DMREAD, NULL, NULL, NULL, browselayer2_availableinterfaceInst, NULL, NULL, NULL, tavailableinterfaceParam, NULL},
{"Marking", &DMWRITE, add_layer2bridging_marking, delete_layer2bridging_marking, NULL, browselayer2_markingInst, NULL, NULL, NULL, tlayer2_markingParam, NULL},
{"Bridge", &DMWRITE, add_layer2bridging_bridge, delete_layer2bridging_bridge, NULL, browselayer2_bridgeInst, NULL, NULL, tlayer2_bridgeObj, tlayer2_bridgeParam, NULL},
{0}
};

/*** Layer2Bridging.AvailableInterface.{i}. ***/
DMLEAF tavailableinterfaceParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_avai_int_alias, set_avai_int_alias, NULL, NULL},
{"AvailableInterfaceKey", &DMREAD, DMT_UNINT, get_available_interface_key, NULL, NULL, &DMNONE},
{"InterfaceReference", &DMREAD, DMT_STRING, get_interface_reference, NULL, NULL, &DMNONE},
{"InterfaceType", &DMREAD, DMT_STRING, get_interfaces_type, NULL, NULL, &DMNONE},
{0}
};

/*** Layer2Bridging.Marking.{i}. ***/
DMLEAF tlayer2_markingParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_marking_alias, set_marking_alias, NULL, NULL},
{"MarkingBridgeReference", &DMWRITE, DMT_INT, get_marking_bridge_reference, set_marking_bridge_key, NULL, NULL},
{"MarkingInterface", &DMWRITE, DMT_STRING, get_marking_interface_key, set_marking_interface_key, NULL, NULL},
{0}
};

/*** Layer2Bridging.Bridge.{i}. ***/
DMOBJ tlayer2_bridgeObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"VLAN", &DMWRITE, add_layer2bridging_bridge_vlan, delete_layer2bridging_bridge_vlan, NULL, browsebridge_vlanInst, NULL, NULL, NULL, tbridge_vlanParam, NULL},
{0}
};

DMLEAF tlayer2_bridgeParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_bridge_alias, set_bridge_alias, NULL, NULL},
{"BridgeEnable", &DMWRITE, DMT_BOOL, get_bridge_status, set_bridge_status, NULL, NULL},
{"BridgeKey", &DMREAD, DMT_UNINT, get_bridge_key, NULL, NULL, NULL},
{"BridgeName", &DMWRITE, DMT_STRING, get_bridge_name, set_bridge_name, NULL, NULL},
{"VLANID", &DMWRITE, DMT_UNINT, get_bridge_vlanid, set_bridge_vlanid, NULL, NULL},
{"X_INTENO_COM_AssociatedInterfaces", &DMWRITE, DMT_STRING, get_associated_interfaces, set_associated_interfaces, NULL, NULL},
{0}
};

/*** Layer2Bridging.Bridge.{i}.VLAN.{i}. ***/
DMLEAF tbridge_vlanParam[] = {
{"Alias", &DMWRITE, DMT_STRING, get_brvlan_alias, set_brvlan_alias, NULL, NULL},
{"VLANEnable", &DMWRITE, DMT_BOOL, get_bridge_vlan_enable, set_bridge_vlan_enable, NULL, NULL},
{"VLANName", &DMWRITE, DMT_STRING, get_bridge_vlan_name, set_bridge_vlan_name, NULL, NULL},
{"VLANID", &DMWRITE, DMT_UNINT, get_bridge_vlan_vid, set_bridge_vlan_vid, NULL, NULL},
{0}
};


int browselayer2_availableinterfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	int i = 0;
	char *oface, *phy_interface, *ch_ptr, *saveptr, *waninstance = NULL, *phy_interface_dup = NULL;
	char *base_ifname, *available_inst = NULL;
	struct uci_section *wifi_s , *wan_s, *ai_s;
	char *instance_last = NULL;
	struct args_layer2 curr_args = {0};

	for (i=0; i<3; i++) {
		uci_foreach_sections(wan_interface_tab[i].package, wan_interface_tab[i].section, wan_s) {
			waninstance = update_instance(wan_s, waninstance, "waninstance");
			dmasprintf(&oface, "%s%cWANDevice%c%s%cWANConnectionDevice%c%s%c", DMROOT, dm_delim, dm_delim, wan_interface_tab[i].instance, dm_delim, dm_delim, waninstance, dm_delim); // MEM WILL BE FREED IN DMMEMCLEAN
			dmuci_get_value_by_section_string(wan_s, "baseifname", &base_ifname);
			ai_s = update_availableinterface_list(dmctx, base_ifname, &available_inst, &instance_last);
			init_args_layer2(&curr_args, ai_s, NULL, instance_last, NULL, "WANInterface", oface);
			if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_args, available_inst) == DM_STOP)
				goto end;
		}
	}
	db_get_value_string("hw", "board", "ethernetLanPorts", &phy_interface);
	phy_interface_dup = dmstrdup(phy_interface);
	i = 0;
	for (ch_ptr = strtok_r(phy_interface_dup, " ", &saveptr); ch_ptr; ch_ptr = strtok_r(NULL, " ", &saveptr))
	{
		dmasprintf(&oface, "%s%cLANInterfaces%cLANEthernetInterfaceConfig%c%d%c", DMROOT, dm_delim, dm_delim, dm_delim, ++i, dm_delim); // MEM WILL BE FREED IN DMMEMCLEAN
		ai_s = update_availableinterface_list(dmctx, ch_ptr, &available_inst, &instance_last);
		init_args_layer2(&curr_args, ai_s, NULL, instance_last, NULL, "LANInterface", oface);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_args, available_inst) == DM_STOP)
			goto end;
	}
	i = 0;
	uci_foreach_sections("wireless", "wifi-iface", wifi_s) {
		dmasprintf(&oface, "%s%cLANInterfaces%cWLANConfiguration%c%d%c", DMROOT, dm_delim, dm_delim, dm_delim, ++i, dm_delim); // MEM WILL BE FREED IN DMMEMCLEAN
		ai_s = update_availableinterface_list(dmctx, section_name(wifi_s), &available_inst, &instance_last);
		init_args_layer2(&curr_args, ai_s, NULL, instance_last, NULL, "LANInterface", oface);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_args, available_inst) == DM_STOP)
			goto end;
	}
end:
	dmfree(phy_interface_dup);
	return 0;
}

int browselayer2_bridgeInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *bridge_instance = NULL, *bridge_instance_last = NULL;
	struct uci_section *bridge_s;
	struct args_layer2 curr_args = {0};

	dmuci_get_option_value_string("layer2_interface_ethernet", "Wan", "baseifname", &wan_baseifname);
	uci_foreach_option_eq("network", "interface", "type", "bridge", bridge_s) {
		bridge_instance =  handle_update_instance(1, dmctx, &bridge_instance_last, update_instance_alias, 3, bridge_s, "bridge_instance", "bridge_alias");
		init_args_layer2(&curr_args, bridge_s, NULL, NULL, bridge_instance_last, NULL, NULL);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_args, bridge_instance) == DM_STOP)
			break;
	}
	return 0;
}

int browselayer2_markingInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	char *marking_instance = NULL, *marking_instance_last = NULL;
	struct uci_section *marking_s = NULL;
	struct args_layer2 curr_args = {0};

	synchronize_availableinterfaceInst(dmctx);
	synchrinize_layer2_bridgeInst(dmctx);
	uci_foreach_sections("dmmap", "marking-bridge", marking_s) {
		marking_instance =  handle_update_instance(1, dmctx, &marking_instance_last, update_instance_alias, 3, marking_s, "marking_instance", "marking_alias");
		init_args_layer2(&curr_args, marking_s, NULL, NULL, NULL, NULL, NULL);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_args, marking_instance) == DM_STOP)
			break;
	}
	return 0;
}

int browsebridge_vlanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *ss = NULL;
	char *vlan_instance = NULL, *vlan_instance_last = NULL;
	struct args_layer2 *curr_args = (struct args_layer2 *)prev_data;

	update_bridge_all_vlan_config_bybridge(dmctx, curr_args);
	uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", curr_args->bridge_instance, ss) {
		vlan_instance =  handle_update_instance(2, dmctx, &vlan_instance_last, update_instance_alias, 3, ss, "vlan_instance", "vlan_alias");
		init_args_layer2_vlan(curr_args, ss);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)curr_args, vlan_instance) == DM_STOP)
			break;
	}
	return 0;
}

int synchronize_availableinterfaceInst(struct dmctx *dmctx)
{
	int i = 0;
	char *oface, *phy_interface, *ch_ptr, *saveptr, *waninstance = NULL, *phy_interface_dup = NULL;
	char *base_ifname, *available_inst = NULL;
	struct uci_section *wifi_s , *wan_s, *ai_s;
	char *instance_last = NULL;

	for (i=0; i<3; i++) {
		uci_foreach_sections(wan_interface_tab[i].package, wan_interface_tab[i].section, wan_s) {
			waninstance = update_instance(wan_s, waninstance, "waninstance");
			dmasprintf(&oface, "%s%cWANDevice%c%s%cWANConnectionDevice%c%s%c", DMROOT, dm_delim, dm_delim, wan_interface_tab[i].instance, dm_delim, dm_delim, waninstance, dm_delim); // MEM WILL BE FREED IN DMMEMCLEAN
			dmuci_get_value_by_section_string(wan_s, "baseifname", &base_ifname);
			update_availableinterface_list(dmctx, base_ifname, &available_inst, &instance_last);
		}
	}
	db_get_value_string("hw", "board", "ethernetLanPorts", &phy_interface);
	phy_interface_dup = dmstrdup(phy_interface);
	i = 0;
	for (ch_ptr = strtok_r(phy_interface_dup, " ", &saveptr); ch_ptr != NULL; ch_ptr = strtok_r(NULL, " ", &saveptr))
	{
		dmasprintf(&oface, "%s%cLANInterfaces%cLANEthernetInterfaceConfig%c%d%c", DMROOT, dm_delim, dm_delim, dm_delim, ++i, dm_delim); // MEM WILL BE FREED IN DMMEMCLEAN
		update_availableinterface_list(dmctx, ch_ptr, &available_inst, &instance_last);
	}
	i = 0;
	uci_foreach_sections("wireless", "wifi-iface", wifi_s) {
		dmasprintf(&oface, "%s%cLANInterfaces%cWLANConfiguration%c%d%c", DMROOT, dm_delim, dm_delim, dm_delim, ++i, dm_delim); // MEM WILL BE FREED IN DMMEMCLEAN
		update_availableinterface_list(dmctx, section_name(wifi_s), &available_inst, &instance_last);
	}
	dmfree(phy_interface_dup);
	return 0;
}

int synchrinize_layer2_bridgeInst(struct dmctx *dmctx)
{
	char *ifname = "", *bridge_instance = "";
	struct uci_section *bridge_s = NULL;
	uci_foreach_option_eq("network", "interface", "type", "bridge", bridge_s) {
		handle_update_instance(1, dmctx, &bridge_instance, update_instance_alias, 3, bridge_s, "bridge_instance", "bridge_alias");
		dmuci_get_value_by_section_string(bridge_s, "ifname", &ifname);
		update_markinginterface_list(bridge_s, bridge_instance, ifname);
	}
	return 0;
}

inline void init_args_layer2(struct args_layer2 *args, struct uci_section *s, struct uci_section *ss,
							char *availableinterface_instance, char *bridge_instance,
							char *interface_type, char *oface)
{
	args->layer2section = s;
	args->layer2sectionlev2 = ss;
	args->interface_type = interface_type;
	args->oface = oface;
	args->availableinterface_instance = availableinterface_instance;
	args->bridge_instance = bridge_instance;
}

inline void init_args_layer2_vlan(struct args_layer2 *args, struct uci_section *ss)
{
	args->layer2sectionlev2 = ss;
}

char *layer2_get_last_section_instance(char *package, char *section, char *opt_inst)
{
	struct uci_section *s, *last = NULL;
	char *inst = NULL;

	uci_foreach_sections(package, section, s)
	{
		last = s;
	}
	if (last) {
		dmuci_get_value_by_section_string(last, opt_inst, &inst);
		inst = update_instance(last, inst, opt_inst);
	}
	return inst;
}

int update_bridge_vlan_config(char *vid, char *bridge_key)
{
	struct uci_section *s, *ss;
	char *add_value, *instance, *p;
	char *name;

	uci_foreach_option_eq("dmmap", "vlan_bridge", "vid", vid, s)
	{
		return 0;
	}
	instance = get_last_instance_lev2("dmmap", "vlan_bridge", "vlan_instance", "bridgekey", bridge_key);
	dmuci_add_section("dmmap", "vlan_bridge", &ss, &add_value);
	instance = update_instance(ss, instance, "vlan_instance");
	dmasprintf(&name, "vlan_%s.%s", bridge_key, instance);
	dmuci_set_value_by_section(ss, "bridgekey", bridge_key);
	dmuci_set_value_by_section(ss, "name", name);
	dmuci_set_value_by_section(ss, "vid", vid);
	dmfree(name);
	return 0;
}

int update_bridge_all_vlan_config_bybridge(struct dmctx *ctx, struct args_layer2 *curr_args)
{
	char *ifname, *ifname_tmp, *pch, *spch, *vid;
	struct uci_section *s, *ss;

	dmuci_get_value_by_section_string(curr_args->layer2section, "ifname", &ifname);
	ifname_tmp = dmstrdup(ifname);
	for (pch = strtok_r(ifname_tmp, " ", &spch); pch != NULL; pch = strtok_r(NULL, " ", &spch)) {
		if (strncmp(pch, wan_baseifname, 4) == 0
			|| strncmp(pch, "ptm", 3) == 0
			|| strncmp(pch, "atm", 3) == 0) {
			uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", pch, s) {
				vid = strchr(pch, '.') + 1;
				update_bridge_vlan_config(vid, curr_args->bridge_instance);
				break;
			}
		}
	}
	dmfree(ifname_tmp);
	return 0;
}

void update_add_vlan_interfaces(char *bridge_key, char *vid)
{
	char *baseifname, *add_value;
	char baseifname_dup[16];
	char *p;
	bool found;
	struct uci_section *s, *vlan_interface_s, *vi_sec;

	uci_foreach_option_eq("dmmap", "marking-bridge", "bridgekey", bridge_key, s) {
		dmuci_get_value_by_section_string(s, "baseifname", &baseifname);
		p = baseifname_dup;
		dmstrappendstr(p, baseifname);
		dmstrappendchr(p, '.');
		dmstrappendstr(p, vid);
		dmstrappendend(p);
		found = false;
		uci_foreach_option_eq("layer2_interface_vlan", "vlan_interface", "ifname", baseifname_dup, vi_sec) {
			found = true;
			break;
		}
		if(found)
			continue;

		if(strncmp(baseifname, wan_baseifname, 4) == 0 ||
			strncmp(baseifname, "ptm", 3) == 0 ||
			strncmp(baseifname, "atm", 3) == 0) {
			dmuci_add_section("layer2_interface_vlan", "vlan_interface", &vlan_interface_s, &add_value);
			dmuci_set_value_by_section(vlan_interface_s, "baseifname", baseifname);
			dmuci_set_value_by_section(vlan_interface_s, "bridge", bridge_key);
			dmuci_set_value_by_section(vlan_interface_s, "ifname", baseifname_dup);
			dmuci_set_value_by_section(vlan_interface_s, "name", baseifname_dup);
			dmuci_set_value_by_section(vlan_interface_s, "vlan8021q", vid);
		}
	}
}

void update_add_vlan_to_bridge_interface(char *bridge_key, struct uci_section *dmmap_s)
{
	char *vid, *ifname, *baseifname;
	struct uci_section *interface_s, *marking_bridge_s;
	char baseifname_dup[16];
	char *p;
	char ifname_dup[128];
	char *ptr;

	dmuci_get_value_by_section_string(dmmap_s, "vid", &vid);
	if(vid[0] == '\0')
		return ;

	uci_foreach_option_eq("network", "interface", "bridge_instance", bridge_key, interface_s)
	{
		dmuci_get_value_by_section_string(interface_s, "ifname", &ifname);
		ifname_dup[0] = '\0';
		ptr = ifname_dup;
		dmstrappendstr(ptr, ifname);
		dmstrappendend(ptr);
		uci_foreach_option_eq("dmmap", "marking-bridge", "bridgekey", bridge_key, marking_bridge_s)
		{
			dmuci_get_value_by_section_string(marking_bridge_s, "baseifname", &baseifname);
			if (strncmp(baseifname, wan_baseifname, 4) == 0
				|| strncmp(baseifname, "ptm", 3) == 0
				|| strncmp(baseifname, "atm", 3) == 0) {
				p = baseifname_dup;
				dmstrappendstr(p, baseifname);
				dmstrappendchr(p, '.');
				dmstrappendstr(p, vid);
				dmstrappendend(p);
				if (is_strword_in_optionvalue(ifname_dup, baseifname_dup)) continue;
				if (ifname_dup[0] != '\0')
					dmstrappendchr(ptr, ' ');
				dmstrappendstr(ptr, baseifname_dup);
				dmstrappendend(ptr);
			}
		}
		dmuci_set_value_by_section(interface_s, "ifname", ifname_dup);
	}
}

int get_marking_bridge_reference(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	dmuci_get_value_by_section_string(args->layer2section, "bridgekey", value);
	return 0;
}

void get_baseifname_from_ifname(char *ifname, char *baseifname)
{
	while (*ifname != '.' && *ifname != '\0') {
		*baseifname++ = *ifname++;
	}
	*baseifname = '\0';
}

void update_markinginterface_list(struct uci_section *interface_section, char *bridge_key, char *ifname)
{
	char *dupifname, *ifname_element, *bridgekey, *instance;
	char *add_value, *interfacekey="", *spch;
	struct uci_section *marking_section, *new_marking_section, *wireless_section;
	struct uci_section *ciface;
	bool found;
	char baseifname[8];

	dupifname = dmstrdup(ifname);
	ifname_element = strtok_r(dupifname, " ", &spch);
	while (ifname_element != NULL) {
		get_baseifname_from_ifname(ifname_element, baseifname);
		found = false;
		uci_foreach_option_eq("dmmap", "marking-bridge", "baseifname", baseifname, marking_section)
		{
			dmuci_get_value_by_section_string(marking_section, "bridgekey", &bridgekey);
			if (strcmp(bridgekey, bridge_key) == 0) {
				found = true;
				break;
			}
		}
		if (found){
			goto nextifname;
		}
		uci_foreach_option_eq("dmmap", "available-bridge", "baseifname", baseifname, ciface)
		{
			dmuci_get_value_by_section_string(ciface, "key", &interfacekey);
			break;
		}
		if (!ciface) goto nextifname;
		instance = layer2_get_last_section_instance("dmmap", "marking-bridge", "marking_instance");
		dmuci_add_section("dmmap", "marking-bridge", &new_marking_section, &add_value);
		dmuci_set_value_by_section(new_marking_section, "baseifname", baseifname);
		dmuci_set_value_by_section(new_marking_section, "bridgekey", bridge_key);
		dmuci_set_value_by_section(new_marking_section, "interfacekey", interfacekey);
		update_instance(new_marking_section, instance, "marking_instance");

nextifname:
		ifname_element = strtok_r(NULL, " ", &spch);
	}
	dmfree(dupifname);
	new_marking_section = NULL;
	uci_foreach_option_eq("wireless", "wifi-iface", "network",  section_name(interface_section), wireless_section) {
		found = false;
		uci_foreach_option_eq("dmmap", "marking-bridge", "baseifname", section_name(wireless_section), marking_section)
		{
			dmuci_get_value_by_section_string(marking_section, "bridgekey", &bridgekey);
			if (strcmp(bridgekey, bridge_key) == 0) {
				found = true;
				break;
			}
		}
		if (found) continue;
		uci_foreach_option_eq("dmmap", "available-bridge", "baseifname", section_name(wireless_section), ciface)
		{
			dmuci_get_value_by_section_string(ciface, "key", &interfacekey);
			break;
		}
		if (!ciface) continue;
		instance = layer2_get_last_section_instance("dmmap", "marking-bridge", "marking_instance");
		dmuci_add_section("dmmap", "marking-bridge", &new_marking_section, &add_value);
		dmuci_set_value_by_section(new_marking_section, "baseifname", section_name(wireless_section));
		dmuci_set_value_by_section(new_marking_section, "bridgekey", bridge_key);
		dmuci_set_value_by_section(new_marking_section, "interfacekey", interfacekey);
		update_instance(new_marking_section, instance, "marking_instance");
	}
}

//set_marking_bridgekey
int set_marking_bridge_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				return 0;
			set_marking_bridge_key_sub(refparam, ctx, data, instance, value);
			return 0;
	}
	return 0;
}

int set_marking_bridge_key_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value)
{
	char *old_bridge_key, *baseifname, *ifname, *vid, *enable, *bridgekey, *p;
	char new_ifname[128];
	struct uci_section *s, *ss;
	struct args_layer2 *args = (struct args_layer2 *)data;
	char iface[16];
	bool found;

	dmuci_get_value_by_section_string(args->layer2section, "bridgekey", &old_bridge_key);
	dmuci_set_value_by_section(args->layer2section, "bridgekey", value);
	dmuci_get_value_by_section_string(args->layer2section, "baseifname", &baseifname);

	if (strncmp(baseifname, "cfg", 3) == 0) {
		uci_foreach_option_eq("network", "interface", "bridge_instance", value, s)
		{
			dmuci_set_value("wireless", baseifname, "network", section_name(s));
			break;
		}
		if (!s) return 0;
	}
	else if (strncmp(baseifname, wan_baseifname, 4) == 0
			|| strncmp(baseifname, "ptm", 3) == 0
			|| strncmp(baseifname, "atm", 3) == 0) {
		found = false;
		uci_foreach_option_eq("dmmap", "marking-bridge", "baseifname", baseifname, s)
		{
			dmuci_get_value_by_section_string(s, "bridgekey", &bridgekey);
			if (strcmp(bridgekey, old_bridge_key) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", old_bridge_key, s)
			{
				dmuci_get_value_by_section_string(s, "vid", &vid);
				if (vid[0] == '\0') continue;
				uci_foreach_option_eq("network", "interface", "bridge_instance", old_bridge_key, ss)
				{
					break;
				}
				if (!ss) return 0;
				dmuci_get_value_by_section_string(ss, "ifname", &ifname);
				p = iface;
				dmstrappendstr(p, baseifname);
				dmstrappendchr(p, '.');
				dmstrappendstr(p, vid);
				dmstrappendend(p);
				remove_interface_from_ifname(iface, ifname, new_ifname);
				dmuci_set_value_by_section(ss, "ifname", new_ifname);
			}
		}
		uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", value, s)
		{
			dmuci_get_value_by_section_string(s, "vid", &vid);
			if (vid[0] == '\0') continue;
			dmuci_get_value_by_section_string(s, "enable", &enable);
			if (enable[0] == '\0' || enable[0] == '0') continue;
			update_remove_vlan_from_bridge_interface(value, s);
			update_add_vlan_interfaces(value, vid);
			update_add_vlan_to_bridge_interface(value, s);
		}
	}
	else if (strncmp(baseifname, "eth", 3) == 0) {
		uci_foreach_option_eq("network", "interface", "bridge_instance", old_bridge_key, s)
		{
			dmuci_get_value_by_section_string(s, "ifname", &ifname);
			if (strlen(ifname) > 5) {
				remove_interface_from_ifname(baseifname, ifname, new_ifname);
			}
			else {
			remove_interface_from_ifname(baseifname, ifname, new_ifname);
			}
			dmuci_set_value_by_section(s, "ifname",  new_ifname);
		}
		uci_foreach_option_eq("network", "interface", "bridge_instance", value, ss)
		{
			dmuci_get_value_by_section_string(ss, "ifname", &ifname);
			p = new_ifname;
			if (ifname[0] != '\0') {
				dmstrappendstr(p, ifname);
				dmstrappendchr(p, ' ');
			}
			dmstrappendstr(p, baseifname);
			dmstrappendend(p);
			dmuci_set_value_by_section(ss, "ifname",  new_ifname);
		}

	}
	return 0;
}

int get_marking_interface_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	dmuci_get_value_by_section_string(args->layer2section, "interfacekey", value);
	return 0;
}

//set_marking_interfacekey
int set_marking_interface_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if (value[0] == '\0')
				return 0;
		set_marking_interface_key_sub(refparam, ctx, data, instance, value);
			return 0;
	}
	return 0;
}

int set_marking_interface_key_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value)
{
	char *vid, *ifname, *baseifname, *bkey, *p, *enable;
	char new_ifname[128];
	char *obifname, *instce;
	struct uci_section *s = NULL, *ss = NULL, *ab, *mb, *sbridge = NULL;
	struct args_layer2 *args = (struct args_layer2 *)data;
	mb = args->layer2section;

	uci_foreach_option_eq("dmmap", "available-bridge", "key", value, ab) {
		break;
	}
	if (!ab) return 0;

	dmuci_get_value_by_section_string(mb, "bridgekey", &bkey);
	if (bkey[0] != '\0') {
		uci_foreach_option_eq("network", "interface", "bridge_instance", bkey, sbridge) {
			break;
		}
		if (sbridge) {
			dmuci_get_value_by_section_string(mb, "baseifname", &obifname);
			dmuci_get_value_by_section_string(mb, "marking_instance", &instce);
			if (obifname[0] != '\0') {
				remove_config_interfaces(obifname, bkey, sbridge, instce);
			}
		}
	}

	dmuci_get_value_by_section_string(ab, "baseifname", &baseifname);
	if (strncmp(baseifname, "cfg", 3) == 0) {
		uci_foreach_option_eq("dmmap", "marking-bridge", "baseifname", baseifname, s)
		{
			break;
		}
		if (s) return 0;
		dmuci_set_value_by_section(mb, "interfacekey",  value);
		dmuci_set_value_by_section(mb, "baseifname",  baseifname);
		if (bkey[0] == '\0' || !sbridge) return 0;
		dmuci_set_value("wireless", baseifname, "network",  section_name(sbridge));
	}
	else if (strncmp(baseifname, wan_baseifname, 4) == 0
			|| strncmp(baseifname, "ptm", 3) == 0
			|| strncmp(baseifname, "atm", 3) == 0) {
		dmuci_set_value_by_section(mb, "interfacekey",  value);
		dmuci_set_value_by_section(mb, "baseifname",  baseifname);
		if (bkey[0] == '\0') return 0;
		uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", bkey, s)
		{
			dmuci_get_value_by_section_string(s, "vid", &vid);
			if (vid[0] == '\0') continue;
			dmuci_get_value_by_section_string(s, "enable", &enable);
			if (enable[0] == '\0' || enable[0] == '0') continue;
			update_remove_vlan_from_bridge_interface(bkey, s);
			update_add_vlan_interfaces(bkey, vid);
			update_add_vlan_to_bridge_interface(bkey, s);
		}
	}
	else if (strncmp(baseifname, "eth", 3) == 0) {
		uci_foreach_option_eq("dmmap", "marking-bridge", "baseifname", baseifname, s)
		{
			break;
		}
		if (s) return 0;
		dmuci_set_value_by_section(mb, "interfacekey",  value);
		dmuci_set_value_by_section(mb, "baseifname",  baseifname);
		if (bkey[0] == '\0' || !sbridge) return 0;
		dmuci_get_value_by_section_string(sbridge, "ifname", &ifname);
		p = new_ifname;
		if (ifname[0] != '\0') {
			dmstrappendstr(p, ifname);
			dmstrappendchr(p, ' ');
		}
		dmstrappendstr(p, baseifname);
		dmstrappendend(p);
		dmuci_set_value_by_section(sbridge, "ifname",  new_ifname);
	}
	return 0;
}

int get_bridge_vlan_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	dmuci_get_value_by_section_string(args->layer2sectionlev2, "enable", value);
	return 0;
}

int set_bridge_vlan_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	int error = string_to_bool(value, &b);
	switch (action) {
		case VALUECHECK:
			if (error)
				return FAULT_9007;
			return 0;
		case VALUESET:
			set_bridge_vlan_enable_sub(refparam, ctx, data, instance, b);
			return 0;
	}
	return 0;
}

int set_bridge_vlan_enable_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, bool b)
{
	char *value, *vid, *bkey, *cval;
	struct uci_section *vb;
	bool bcval;
	struct args_layer2 *args = (struct args_layer2 *)data;

	vb = args->layer2sectionlev2;
	dmuci_get_value_by_section_string(vb, "vid", &vid);
	dmuci_get_value_by_section_string(vb, "bridgekey", &bkey);
	dmuci_get_value_by_section_string(vb, "enable", &cval);
	string_to_bool(cval, &bcval);
	if (b && !bcval) {
		dmuci_set_value_by_section(vb, "enable", "1");
		if (vid[0] == '\0') return 0;
		update_add_vlan_interfaces(bkey, vid);
		update_add_vlan_to_bridge_interface(bkey, vb);
	}
	else if (!b && bcval) {
		dmuci_set_value_by_section(vb, "enable", "0");
		if (vid[0] == '\0') return 0;
		update_remove_vlan_from_bridge_interface(bkey, vb);
	}
	return 0;
}

int get_bridge_vlan_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	dmuci_get_value_by_section_string(args->layer2sectionlev2, "name", value);
	return 0;
}

int set_bridge_vlan_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct args_layer2 *args;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			args = (struct args_layer2 *)data;
			dmuci_set_value_by_section(args->layer2sectionlev2, "name", value);
			return 0;
	}
	return 0;
}

int get_bridge_vlan_vid(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	dmuci_get_value_by_section_string(args->layer2sectionlev2, "vid", value);
	return 0;
}

int set_bridge_vlan_vid(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct uci_section *vb;
	struct args_layer2 *args;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			args = (struct args_layer2 *)data;
			vb = args->layer2sectionlev2;
			set_bridge_vlan_vid_sub(vb, value);
			return 0;
	}
	return 0;
}

int set_bridge_vlan_vid_sub(struct uci_section *vb, char *value)
{
	char *enable, *bkey, *cval;
	dmuci_get_value_by_section_string(vb, "vid", &cval);
	if (strcmp(cval, value) == 0) return 0;
	dmuci_get_value_by_section_string(vb, "enable", &enable);
	if (enable[0] == '1') {
		dmuci_get_value_by_section_string(vb, "bridgekey", &bkey);
		update_remove_vlan_from_bridge_interface(bkey, vb);
		dmuci_set_value_by_section(vb, "vid", value);
		update_add_vlan_interfaces(bkey, value);
		update_add_vlan_to_bridge_interface(bkey, vb);
	}
	else {
		dmuci_set_value_by_section(vb, "vid", value);
	}

	return 0;
}

int get_bridge_status(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	json_object *res;
	struct args_layer2 *args = (struct args_layer2 *)data;
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(args->layer2section)}}, 1, &res);
	DM_ASSERT(res, *value = "0");
	json_select(res, "up", 0, NULL, value, NULL);
	return 0;
}

int set_bridge_status(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	struct args_layer2 *args = (struct args_layer2 *)data;

	int error = string_to_bool(value, &b);
	switch (action) {
		case VALUECHECK:
			if (error)
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b) {
				dmubus_call_set("network.interface", "up", UBUS_ARGS{{"interface", section_name(args->layer2section)}}, 1);
			}
			else {
				dmubus_call_set("network.interface", "down", UBUS_ARGS{{"interface", section_name(args->layer2section)}}, 1);
			}
			return 0;
	}
	return 0;
}

int get_bridge_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	*value = args->bridge_instance;
	return 0;
}

int get_bridge_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	*value = dmstrdup(section_name(args->layer2section));
	return 0;
}

int set_bridge_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct args_layer2 *args;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			args = (struct args_layer2 *)data;
			dmuci_rename_section_by_section(args->layer2section, value);
			return 0;
	}
	return 0;
}

int get_bridge_vlanid(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *bridge_instance;
	struct uci_section *s = NULL;
	struct args_layer2 *args = (struct args_layer2 *)data;
	*value = "";

	uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", args->bridge_instance, s)
	{
		dmuci_get_value_by_section_string(s, "vid", value);
		break;
	}
	return 0;
}

int set_bridge_vlanid(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	struct uci_section *vb;

	switch(action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			args = (struct args_layer2 *)data;
			vb = args->layer2section;
			set_bridge_vlanid_sub(refparam, ctx, data, instance, value);
			return 0;
	}
	return 0;
}

int set_bridge_vlanid_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value)
{
	char *add_value, *name, *instce;
	struct uci_section *s = NULL, *vb;;
	struct args_layer2 *args = (struct args_layer2 *)data;

	uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", args->bridge_instance, s)
	{
		break;
	}
	if (s == NULL) {
		instce = get_last_instance_lev2("dmmap", "vlan_bridge", "vlan_instance", "bridgekey", args->bridge_instance);
		dmuci_add_section("dmmap", "vlan_bridge", &vb, &add_value);
		instce = update_instance(vb, instce, "vlan_instance");
		dmasprintf(&name, "vlan_%s.%s", args->bridge_instance, instce);
		dmuci_set_value_by_section(vb, "bridgekey", args->bridge_instance);
		dmuci_set_value_by_section(vb, "name", name);
		dmuci_set_value_by_section(vb, "enable", "0");
		dmuci_set_value_by_section(vb, "vid", value);
		dmfree(name);
	}
	else {
		set_bridge_vlan_vid_sub(s, value);
	}
	return 0;
}

int get_associated_interfaces(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	dmuci_get_value_by_section_string(args->layer2section, "ifname", value);
	return 0;
}

int set_associated_interfaces(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct args_layer2 *args;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			args = (struct args_layer2 *)data;
			dmuci_set_value_by_section(args->layer2section, "ifname", value);
			return 0;
	}
	return 0;
}

int get_available_interface_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	*value = args->availableinterface_instance;
	return 0;
}

int get_interface_reference(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	*value = args->oface;
	return 0;
}

int get_interfaces_type(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct args_layer2 *args = (struct args_layer2 *)data;
	*value = args->interface_type;
	return 0;
}

struct uci_section *update_availableinterface_list(struct dmctx *ctx, char *iface, char **instance, char **instance_last)
{
	struct uci_section *s = NULL;
	struct uci_section *available_bridge = NULL;
	char *add_value;

	uci_foreach_option_eq("dmmap", "available-bridge", "baseifname", iface, s)
	{
		*instance =  handle_update_instance(1, ctx, instance_last, update_instance_alias, 3, s, "key", "avbralias");
		return s;
	}
	dmuci_add_section("dmmap", "available-bridge", &available_bridge, &add_value);
	dmuci_set_value_by_section(available_bridge, "baseifname", iface);
	*instance =  handle_update_instance(1, ctx, instance_last, update_instance_alias, 3, available_bridge, "key", "avbralias");
	return available_bridge;
}

/*************************************************************
 * ADD DELETE OBJECT
/*************************************************************/

int add_layer2bridging_bridge(char *refparam, struct dmctx *ctx, void *data, char **instance)
{
	char *last_instance;
	char bridge_name[16], ib[8];
	char *p = bridge_name;

	last_instance = get_last_instance_lev2("network", "interface", "bridge_instance", "type", "bridge");
	sprintf(ib, "%d", last_instance ? atoi(last_instance)+1 : 1);
	dmstrappendstr(p, "bridge_0_");
	dmstrappendstr(p, ib);
	dmstrappendend(p);
	dmuci_set_value("network", bridge_name, "", "interface");
	dmuci_set_value("network", bridge_name, "type", "bridge");
	dmuci_set_value("network", bridge_name, "proto", "dhcp");
	*instance = dmuci_set_value("network", bridge_name, "bridge_instance", ib);
	return 0;
}

int delete_layer2bridging_bridge(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	struct args_layer2 *args_bridge = (struct args_layer2 *)data;
	struct uci_section *bridge_s, *vlan_s, *prev_s = NULL;
	char *bridgekey = NULL, *bridge_instance;

	switch (del_action) {
		case DEL_INST:
			dmuci_get_value_by_section_string(args_bridge->layer2section, "bridge_instance", &bridge_instance);
			dmuci_set_value_by_section(args_bridge->layer2section, "type", "");
			dmuci_set_value_by_section(args_bridge->layer2section, "bridge_instance", "");
			uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", bridge_instance, vlan_s) {
				if (prev_s)
					dmuci_delete_by_section(prev_s, NULL, NULL);
				prev_s = vlan_s;
			}
			if (prev_s)
				dmuci_delete_by_section(prev_s, NULL, NULL);
			return 0;
		case DEL_ALL:
			uci_foreach_option_eq("network", "interface", "type", "bridge", bridge_s) {
				dmuci_set_value_by_section(bridge_s, "type", "");
				dmuci_set_value_by_section(bridge_s, "bridge_instance", "");
			}
			uci_foreach_sections("dmmap", "vlan_bridge", vlan_s) {
				if(prev_s != NULL && bridgekey[0] != '\0')
					dmuci_delete_by_section(prev_s, NULL, NULL);
				prev_s = vlan_s;
				dmuci_get_value_by_section_string(vlan_s, "bridgekey", &bridgekey);
			}
			if(prev_s != NULL && bridgekey[0] != '\0')
				dmuci_delete_by_section(prev_s, NULL, NULL);
			return 0;
	}
	return 0;
}

int add_layer2bridging_marking(char *refparam, struct dmctx *ctx, void *data, char **instance)
{
	char *last_instance;
	char *value;
	struct uci_section *marking_s;

	last_instance = get_last_instance("dmmap", "marking-bridge", "marking_instance");
	dmuci_add_section("dmmap", "marking-bridge", &marking_s, &value);
	*instance = update_instance(marking_s, last_instance, "marking_instance");
	return 0;
}

void remove_config_interfaces(char *baseifname, char *bridge_key, struct uci_section *bridge_s, char *mbi)
{
	struct uci_section *marking_s, *vlan_s;
	char *b_key, *m_instance, *vid, *ifname;
	bool found = false;
	char new_ifname[128];
	char *p, iface[16];

	if (strncmp(baseifname, "cfg", 3) == 0) {
		dmuci_delete("wireless", baseifname , "network", NULL);
	}
	else if (strncmp(baseifname, wan_baseifname, 4) == 0
			|| strncmp(baseifname, "ptm", 3) == 0
			|| strncmp(baseifname, "atm", 3) == 0)
	{
		uci_foreach_option_eq("dmmap", "marking-bridge", "baseifname", baseifname, marking_s) {
			dmuci_get_value_by_section_string(marking_s, "bridgekey", &b_key);
			dmuci_get_value_by_section_string(marking_s, "marking_instance", &m_instance);
			if(strcmp(b_key, bridge_key) == 0 && strcmp(m_instance, mbi) != 0) {
				found = true;
				break;
			}
		}
		if (!found)
		{
			uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", bridge_key, vlan_s) {
				dmuci_get_value_by_section_string(vlan_s, "vid", &vid);
				if(vid[0] == '\0')
					continue;
				dmuci_get_value_by_section_string(bridge_s, "ifname", &ifname);
				p = iface;
				dmstrappendstr(p, baseifname);
				dmstrappendchr(p, '.');
				dmstrappendstr(p, vid);
				dmstrappendend(p);
				remove_interface_from_ifname(iface, ifname, new_ifname);
				dmuci_set_value_by_section(bridge_s, "ifname", new_ifname);
			}
		}
	}
	else if (strncmp(baseifname, "eth", 3) == 0) {
		dmuci_get_value_by_section_string(bridge_s, "ifname", &ifname);
		remove_interface_from_ifname(baseifname, ifname, new_ifname);
		dmuci_set_value_by_section(bridge_s, "ifname", new_ifname);
	}
}

int delete_layer2bridging_marking(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	struct uci_section *mark_s, *prev_s = NULL;
	char *bifname, *b_key, *m_instance;
	struct uci_section *bridge_s;
	struct args_layer2 *args_bridge;

	switch (del_action) {
		case DEL_INST:
			args_bridge = (struct args_layer2 *)data;
			dmuci_get_value_by_section_string(args_bridge->layer2section, "bridgekey", &b_key);
			dmuci_get_value_by_section_string(args_bridge->layer2section, "baseifname", &bifname);
			dmuci_get_value_by_section_string(args_bridge->layer2section, "marking_instance", &m_instance);
			dmuci_delete_by_section(args_bridge->layer2section, NULL, NULL);
			if(b_key[0] == '\0')
				return 0;
			uci_foreach_option_eq("network", "interface", "bridge_instance", b_key, bridge_s) {
				remove_config_interfaces(bifname, b_key, bridge_s, m_instance);
				break;
			}
			break;
		case DEL_ALL:
			uci_foreach_sections("dmmap", "marking-bridge", mark_s) {
				dmuci_get_value_by_section_string(mark_s, "bridgekey", &b_key);
				dmuci_get_value_by_section_string(mark_s, "baseifname", &bifname);
				dmuci_get_value_by_section_string(mark_s, "marking_instance", &m_instance);
				if (prev_s != NULL) {
					dmuci_delete_by_section(prev_s, NULL, NULL);
				}
				prev_s = mark_s;
				if (b_key[0] == '\0')
					continue;
				uci_foreach_option_eq("network", "interface", "bridge_instance", b_key, bridge_s) {
					remove_config_interfaces(bifname, b_key, bridge_s, m_instance);
					break;
				}
			}
			if(prev_s != NULL)
				dmuci_delete_by_section(prev_s, NULL, NULL);
			break;
	}
	return 0;
}

int add_layer2bridging_bridge_vlan(char *refparam, struct dmctx *ctx, void *data, char **instance)
{
	struct args_layer2 *args_bridge = (struct args_layer2 *)data;
	char *value, *last_instance ;
	struct uci_section *vlan_s;
	char buf[16];
	char *v_name = buf;

	last_instance = get_last_instance_lev2("dmmap", "vlan_bridge", "vlan_instance", "bridgekey", args_bridge->bridge_instance);
	dmuci_add_section("dmmap", "vlan_bridge", &vlan_s, &value);
	dmuci_set_value_by_section(vlan_s, "bridgekey", args_bridge->bridge_instance);
	dmuci_set_value_by_section(vlan_s, "enable", "0");
	*instance = update_instance(vlan_s, last_instance, "vlan_instance");
	dmstrappendstr(v_name, "vlan_");
	dmstrappendstr(v_name, args_bridge->bridge_instance);
	dmstrappendchr(v_name, '.');
	dmstrappendstr(v_name, *instance);
	dmstrappendend(v_name);
	dmuci_set_value_by_section(vlan_s, "name", v_name);
	return 0;
}

int delete_layer2bridging_bridge_vlan(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	char *vid, *ifname;
	char new_ifname[128];
	struct uci_section *vlan_s, *prev_s = NULL ;
	struct args_layer2 *args_bridge = (struct args_layer2 *)data;

	switch (del_action) {
		case DEL_INST:
			dmuci_get_value_by_section_string(args_bridge->layer2sectionlev2, "vid", &vid);
			dmuci_get_value_by_section_string(args_bridge->layer2section, "ifname", &ifname);
			remove_vid_interfaces_from_ifname(vid, ifname, new_ifname);
			dmuci_set_value_by_section(args_bridge->layer2section, "ifname", new_ifname);
			dmuci_delete_by_section(args_bridge->layer2sectionlev2, NULL, NULL);
			break;
		case DEL_ALL:
			uci_foreach_option_eq("dmmap", "vlan_bridge", "bridgekey", args_bridge->bridge_instance, vlan_s) {
				dmuci_get_value_by_section_string(vlan_s, "vid", &vid);
				dmuci_get_value_by_section_string(args_bridge->layer2section, "ifname", &ifname);
				remove_vid_interfaces_from_ifname(vid, ifname, new_ifname);
				dmuci_set_value_by_section(args_bridge->layer2section, "ifname", new_ifname);
				if (prev_s != NULL)
					dmuci_delete_by_section(prev_s, NULL, NULL);
				prev_s = vlan_s;
			}
			if (prev_s != NULL)
				dmuci_delete_by_section(prev_s, NULL, NULL);
			break;
	}
	return 0;
}

////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_avai_int_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string(((struct args_layer2 *)data)->layer2section, "avbralias", value);
	return 0;
}

int set_avai_int_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(((struct args_layer2 *)data)->layer2section, "avbralias", value);
			return 0;
	}
	return 0;
}

int get_marking_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string(((struct args_layer2 *)data)->layer2section, "marking_alias", value);
	return 0;
}

int set_marking_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(((struct args_layer2 *)data)->layer2section, "marking_alias", value);
			return 0;
	}
	return 0;
}

int get_bridge_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string(((struct args_layer2 *)data)->layer2section, "bridge_alias", value);
	return 0;
}

int set_bridge_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(((struct args_layer2 *)data)->layer2section, "bridge_alias", value);
			return 0;
	}
	return 0;
}
int get_brvlan_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string(((struct args_layer2 *)data)->layer2section, "vlan_alias", value);
	return 0;
}

int set_brvlan_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(((struct args_layer2 *)data)->layer2section, "vlan_alias", value);
			return 0;
	}
	return 0;
}

