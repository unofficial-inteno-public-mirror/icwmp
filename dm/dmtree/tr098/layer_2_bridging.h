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

#ifndef __Layer_2_bridging_H
#define __Layer_2_bridging_H

extern DMOBJ tLayer2BridgingObj[];
extern DMOBJ tlayer2_bridgeObj[];
extern DMLEAF tbridge_vlanParam[];
extern DMLEAF tlayer2_bridgeParam[];
extern DMLEAF tlayer2_markingParam[];
extern DMLEAF tavailableinterfaceParam[];

struct wan_interface 
{
	char *instance;
	char *name;
	char *package;
	char *section;		
};

struct args_layer2
{
	struct uci_section *layer2section;
	struct uci_section *layer2sectionlev2;
	char *bridge_instance;
	char *availableinterface_instance;
	char *interface_type;
	char *oface;
};

int browselayer2_availableinterfaceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browselayer2_bridgeInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browselayer2_markingInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browsebridge_vlanInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int set_marking_bridge_key_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value);
int set_marking_interface_key_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value);
int set_bridge_vlan_enable_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, bool b);
int set_bridge_vlanid_sub(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value);
void remove_config_interfaces(char *baseifname, char *bridge_key, struct uci_section *bridge_s, char *mbi);
int synchrinize_layer2_bridgeInst(struct dmctx *dmctx);
int synchronize_availableinterfaceInst(struct dmctx *dmctx);
inline void init_args_layer2(struct args_layer2 *args, struct uci_section *s, struct uci_section *ss,
							char *availableinterface_instance, char *bridge_instance,
							char *interface_type, char *oface);
inline void init_args_layer2_vlan(struct args_layer2 *args, struct uci_section *ss);
struct uci_section *update_availableinterface_list(struct dmctx *ctx, char *iface, char **instance, char **instance_last);
void update_markinginterface_list(struct uci_section *interface_section, char *bridge_key, char *ifname);
int add_layer2bridging_bridge(char *refparam, struct dmctx *ctx, void *data, char **instance);
int delete_layer2bridging_bridge(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action);
int add_layer2bridging_marking(char *refparam, struct dmctx *ctx, void *data, char **instance);
int delete_layer2bridging_marking(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action);
int add_layer2bridging_bridge_vlan(char *refparam, struct dmctx *ctx, void *data, char **instance);
int delete_layer2bridging_bridge_vlan(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action);
int get_avai_int_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_avai_int_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_marking_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_marking_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_bridge_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_bridge_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_bridge_vlan_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_bridge_vlan_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_bridge_vlan_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_bridge_vlan_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_bridge_vlan_vid(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_bridge_vlan_vid(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_bridge_status(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_bridge_status(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_bridge_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int get_bridge_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_bridge_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_bridge_vlanid(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_bridge_vlanid(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_associated_interfaces(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_associated_interfaces(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_available_interface_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int get_interface_reference(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int get_interfaces_type(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
char *layer2_get_last_section_instance(char *package, char *section, char *opt_inst);
int update_bridge_vlan_config(char *vid, char *bridge_key);
int update_bridge_all_vlan_config_bybridge(struct dmctx *ctx, struct args_layer2 *curr_args);
void update_add_vlan_interfaces(char *bridge_key, char *vid);
void update_add_vlan_to_bridge_interface(char *bridge_key, struct uci_section *dmmap_s);
int get_marking_bridge_reference(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
void get_baseifname_from_ifname(char *ifname, char *baseifname);
int set_marking_bridge_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int set_marking_interface_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int set_marking_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_brvlan_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_brvlan_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_marking_interface_key(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
#endif
