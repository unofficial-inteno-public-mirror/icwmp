/*
 *	Copyright (C) 2013-2015 Inteno Broadband Technology AB
 *	Author: Anis Ellouze <anis.ellouze@pivasoftware.com>	
 */

#include "dmcwmp.h"
#include "dmuci.h"
#include "dmmem.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "layer_2_bridging.h"
#include <libubox/blobmsg_json.h>
#include <json/json.h>

struct wan_interface 
{
		int instance;
		char *name;
		char *package;
		char *section;		
};

struct args_layer2
{
	struct uci_section *layer2section;
	char *interface_type;
	char *oface;
	char *key;
};

struct wan_interface wan_interface_tab[3] = {
{1,"ethernet", "layer2_interface_ethernet", "ethernet_interface"},
{2,"adsl", "layer2_interface_adsl", "atm_bridge"},
{3,"vdsl", "layer2_interface_vdsl", "vdsl_interface"}
};

struct args_layer2 cur_args = {0};

inline void init_args_layer2(struct dmctx *ctx, struct uci_section *s, char *interface_type, char *key, char *oface) {
	struct args_layer2 *args = &cur_args;
	ctx->args = (void *)args;
	args->layer2section = s;
	args->interface_type = interface_type;
	args->key = key;
	args->oface = oface;
	return ;
}

void remove_substring(char *s,const char *str_remove) {
	while( s=strstr(s,str_remove) )
	memmove(s,s+strlen(str_remove),1+strlen(s+strlen(str_remove)));
}

int get_marking_bridge_reference(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "bridgekey", value);
	return 0;
}

int set_marking_bridge_reference(char *refparam, struct dmctx *ctx, int action, char *value) {
	char *old_bridge_key, *baseifname, *br_instance, *ifname,*vb=NULL;
	struct uci_section *s = NULL;
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "bridgekey", &old_bridge_key);
	dmuci_set_value_by_section(args->layer2section, "bridgekey", value);
	dmuci_get_value_by_section_string(args->layer2section, "baseifname", &baseifname);

	if(!strcmp(strndup(baseifname,3), "cfg")) {
		uci_foreach_sections("network", "interface", s) {
				dmuci_get_value_by_section_string(s, "bridge_instance", &br_instance);
				if(!strcmp(br_instance, old_bridge_key)) {
					dmuci_set_value("wireless", "baseifname", "network", s->e.name);
			}
		}
	}
	if(!strcmp(strndup(baseifname,3),"eth")) {
		if(strcmp(baseifname+3,"0")) {
			uci_foreach_sections("network", "interface", s) {
				dmuci_get_value_by_section_string(args->layer2section, "bridge_instance", &br_instance);
				if(!strcmp(br_instance, old_bridge_key)) {
					dmuci_get_value_by_section_string(args->layer2section, "ifname", &ifname);
					remove_substring(ifname, baseifname);
					dmuci_set_value_by_section(s, "ifname",  ifname);	
				}
			}	
		}
		else {
			return;
		}
	}
	return 0;
}

int get_marking_interface_key(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "interfacekey", value);
	return 0;
}

int set_marking_interface_key(char *refparam, struct dmctx *ctx, int action, char *value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_set_value_by_section(args->layer2section, "interfacekey", value);
	return 0;
}

int get_bridge_vlan_enable(char *refparam,struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "enable", value);
	return 0;
}

int set_bridge_vlan_enable(char *refparam, struct dmctx *ctx, int action, char *value) {
	char *vb=NULL;
	bool b;
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_set_value_by_section(args->layer2section, "enable", value);
	return 0;
}

int get_bridge_vlan_name(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "name", value);
	return 0;
}

int set_bridge_vlan_name(char *refparam, struct dmctx *ctx, int action, char *value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_set_value_by_section(args->layer2section, "name", value);
	return 0;
}

int get_bridge_vlan_vid(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "vid", value);
	return 0;
}

int set_bridge_vlan_vid(char *refparam, struct dmctx *ctx, int action, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "vid", value);
	return 0;
}

int get_bridge_status(char *refparam, struct dmctx *ctx, char **value) {
	#if 0
	json_object *res;
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmubus_call("network.interface", "status", UBUS_ARGS{{"interface", section_name(args->layer2section)}}, 1, &res);
	json_select(res, "up", 0, NULL, value, NULL);
	#endif
	*value = dmstrdup("up");
	return 0;
}

int set_bridge_status(char *refparam, struct dmctx *ctx, int action, char *value) {
	return 0;
}

int get_bridge_key(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section,  "bridge_instance", value);
	return 0;
}

void set_bridge_key(char *refparam, struct dmctx *ctx, int action, char *value) {
	return ;
}

int get_bridge_name(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	*value=dmstrdup(section_name(args->layer2section));
	return 0;
}

int set_bridge_name(char *refparam, struct dmctx *ctx, int action, char *value) {
	return 0;
}

int get_bridge_vlanid(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	*value = dmstrdup("");
	return 0;
}

int set_bridge_vlanid(char *refparam, struct dmctx *ctx, int action, char *value) {
	char **bridge_key;
	char *instance=NULL;
	char *vlan_bridge;
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	
	if (instance == "") {
		dmuci_set_value("dmmap", vlan_bridge, "bridgekey",value);
		dmuci_set_value("dmmap", vlan_bridge,"dname",value);
	}else {
		return 0;
	}
}

void get_associated_interfaces(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_get_value_by_section_string(args->layer2section, "ifname", value);
}

void set_associated_interfaces(char *refparam, struct dmctx *ctx, int action, char *value) {	
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	dmuci_set_value_by_section(args->layer2section, "ifname", value);
}

int get_available_interface_key(char *refparam, struct dmctx *ctx, char **value) {
	dmuci_get_option_value_string("dmmap", "available-bridge", "key", value);
	return 0;
}

void set_available_interface_key() {
	return;
}

int get_interface_reference(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	*value = dmstrdup(args->oface);
	return 0;
}

void set_interface_reference(char *refparam, struct dmctx *ctx, int action, char *value) {
	return;
}

int get_interface_type(char *refparam, struct dmctx *ctx, char **value) {
	struct args_layer2 *args = (struct args_layer2 *)ctx->args;
	*value = dmstrdup(args->interface_type);
	return 0;
}

void set_interface_type(char *refparam, struct dmctx *ctx, int action, char *value) {
	return;
}

int update_availableinterface_list(char *iface, char **value) {	
	struct uci_section *s = NULL;
	char *key;
	int key_int; 
	int new_key=0; 
	char *new_key_str;

	uci_foreach_option_eq("dmmap", "available-bridge", "baseifname", iface, s) {		 
		if (s != NULL) {
			dmuci_get_value_by_section_string(s, "instance", value);
			return 0;
		}
	} 
	uci_foreach_sections("dmmap", "available-bridge",s) {
		if (s != NULL) {
			dmuci_get_value_by_section_string(s, "instance", value);
			key=strdup(*value);
			if(key=="")
				new_key=1;
			else {
				int key_int=atoi(key);
				if(key_int>new_key)
					new_key=key_int;
			}
		}
	}
	dmasprintf(value,"%d",new_key+1);
	return 0;
}

void update_instances(char *package, char *stype, char *inst_opt, char *last_inst, struct uci_section *s, char **value) {
	dmuci_get_value_by_section_string(s, inst_opt, value);
	char buf[8] = {0};
	if (*value[0] == '\0') {
		if (last_inst == NULL)
			sprintf(buf, "%d", 1);
		else
			sprintf(buf, "%d", atoi(last_inst)+1);
		dmfree(*value);
		*value = dmuci_set_value_by_section(s, inst_opt, buf);
	}
	return ;
}


inline int get_marking_bridge(struct dmctx *ctx, char *marking_br, char *marking_br_instance) {
	DMOBJECT(DMROOT"Layer2Bridging.Marking.%s.", ctx, "1", NULL, NULL, NULL, NULL, marking_br_instance);
	DMPARAM("MarkingBridgeReference", ctx, "1", get_marking_bridge_reference, set_marking_bridge_reference, "xsd:boolean", "0", 0, 0, NULL);
	DMPARAM("MarkingInterface", ctx, "1", get_marking_interface_key, set_marking_interface_key, "xsd:boolean", "0", 0, 0, NULL);
	return 0;
}

inline int get_bridge_vlan(struct dmctx *ctx,char *bridge_instance, char *vlan_instance , char *section_name) {
	DMOBJECT(DMROOT"InternetGatewayDevice.Layer2Bridging.Bridge.%s.VLAN.%s.", ctx, "0", NULL, NULL, NULL, NULL, bridge_instance, vlan_instance);
	DMPARAM("VLANEnable", ctx, "1", get_bridge_vlan_enable, set_bridge_vlan_enable, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("VLANName", ctx, "1", get_bridge_vlan_name, set_bridge_vlan_name, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("VLANID", ctx, "1", get_bridge_vlan_vid, set_bridge_vlan_vid, "xsd:boolean", "0", 0, 0,NULL);
	return 0;
}

inline int get_bridge_interface(struct dmctx *ctx, char *bridge_instance, char *interface) {
	struct uci_section *s = NULL;
	char *bridgekey, *vlan_instance;

	DMOBJECT(DMROOT"Layer2Bridging.Bridge.%s.", ctx, "1", NULL, NULL, NULL, NULL, bridge_instance);
	DMPARAM("BridgeEnable", ctx, "1", get_bridge_status, set_bridge_status, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("BridgeKey", ctx, "0", get_bridge_key, set_bridge_key, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("BridgeName", ctx, "1", get_bridge_name, set_bridge_name, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("VLANID", ctx, "1", get_bridge_vlanid, set_bridge_vlanid, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("X_INTENO_COM_AssociatedInterfaces", ctx, "1", get_associated_interfaces, set_associated_interfaces, "xsd:boolean", "0", 0, 0,NULL);
	DMOBJECT(DMROOT"Layer2Bridging.Bridge.%s.VLAN.", ctx, "0", NULL, NULL, NULL, NULL, bridge_instance);

	uci_foreach_sections("dmmap", "vlan_bridge", s) {
			dmuci_get_value_by_section_string(s, "bridgekey", &bridgekey);
			if(!strcmp(bridgekey, bridge_instance)){
				dmuci_get_value_by_section_string(s, "instance", &vlan_instance);
				init_args_layer2(ctx, s, strdup(""), strdup(""), strdup(""));
				SUBENTRY(get_bridge_vlan, ctx, bridge_instance, vlan_instance+2 , s->e.name);
			}
	}
	return 0;
}

inline int get_available_interface(struct dmctx *ctx, int available_int_inst, char *oface, char *interface_type) {
	DMOBJECT(DMROOT"Layer2Bridging.AvailableInterface.%d.", ctx, "0", NULL, NULL, NULL, NULL, available_int_inst);
	DMPARAM("AvailableInterfaceKey", ctx, "0", get_available_interface_key, set_available_interface_key, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("InterfaceReference", ctx, "0", get_interface_reference, set_interface_reference, "xsd:boolean", "0", 0, 0,NULL);
	DMPARAM("InterfaceType", ctx, "0", get_interface_type, set_interface_type, "xsd:boolean", "0", 0, 0,NULL);
	return 0;
}

int entry_method_root_Layer2Bridging(struct dmctx *ctx) {
	int i=0, available_int_inst, instance, er;
	char *oface, *phy_interface, *ch_ptr, *bridge_type, *cur_idev=NULL, *bridge_instance;
	char *base_ifname, *marking_instance, *available_inst=NULL;
	struct uci_section *s = NULL;

	
	uci_ctx = uci_alloc_context();
	IF_MATCH(ctx, DMROOT"Layer2Bridging.") {
		DMOBJECT(DMROOT"Layer2Bridging.", ctx, "0", NULL, NULL, NULL, NULL);
		DMOBJECT(DMROOT"Layer2Bridging.AvailableInterface.", ctx, "0", NULL, NULL, NULL, NULL);
		for (i=0; i<3; i++) {
			uci_foreach_sections(wan_interface_tab[i].package, wan_interface_tab[i].section, s) {
				if (s != NULL) {
					dmuci_get_value_by_section_string(s,"baseifname", &base_ifname);
					instance=atoi(base_ifname+3);
					dmasprintf(&oface,"InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.",wan_interface_tab[i].instance, ++instance);
					update_availableinterface_list(base_ifname, &available_inst);
					available_int_inst=atoi(available_inst);
					init_args_layer2(ctx, s, dmstrdup("WANInterface"), available_inst, oface);
					SUBENTRY(get_available_interface, ctx, available_int_inst, oface, dmstrdup("WANInterface")); 
				}
				else {
					break;
				}
			}
		}
		db_get_value_string("hw", "board", "ethernetLanPorts", &phy_interface);
		ch_ptr = strtok (phy_interface," ");
		while (ch_ptr != NULL)
		{
			dmasprintf(&oface,"InternetGatewayDevice.LANInterfaces.LANEthernetInterfaceConfig.%s.",ch_ptr+3);
			update_availableinterface_list(ch_ptr, &available_inst);
			available_int_inst=atoi(available_inst);
			init_args_layer2(ctx, s, dmstrdup("LANInterface"), available_inst, oface);
			SUBENTRY(get_available_interface, ctx, available_int_inst, oface, strdup("LANInterface"));
			ch_ptr = strtok (NULL, " ");
		}
		int wi_instance=0;
		uci_foreach_sections("wireless", "wifi-iface", s) {
			dmasprintf(&oface,"InternetGatewayDevice.LANInterfaces.WLANConfiguration.%d.",++wi_instance);
			update_availableinterface_list(s->e.name, &available_inst);// section_name
			available_int_inst=atoi(available_inst);
			init_args_layer2(ctx, s, dmstrdup("LANInterface"), available_inst, oface);
			SUBENTRY(get_available_interface, ctx, available_int_inst, oface, strdup("LANInterface"));
		}
		DMOBJECT(DMROOT"Layer2Bridging.Bridge.", ctx, "1", NULL, NULL, NULL, NULL);
		uci_foreach_sections("network", "interface", s) {
			dmuci_get_value_by_section_string(s, "type", &bridge_type);
				
			if(!strcmp(bridge_type, "bridge"))
			{
				update_instances("network", "interface", "bridge_instance", cur_idev, s, &bridge_instance);
				init_args_layer2(ctx, s, strdup(""), strdup(""), strdup(""));
				SUBENTRY(get_bridge_interface, ctx, bridge_instance, s->e.name);
				cur_idev = dmstrdup(bridge_instance);	
			}
		}
		DMOBJECT(DMROOT"Layer2Bridging.Marking.", ctx, "1", NULL, NULL, NULL, NULL);
		uci_foreach_sections("dmmap", "marking-bridge", s) {
			dmuci_get_value_by_section_string(s, "instance", &marking_instance);
			init_args_layer2(ctx, s, strdup(""), strdup(""), strdup(""));
			SUBENTRY(get_marking_bridge, ctx, s->e.name, marking_instance); 
		}
		dmfree(cur_idev);
		dmfree(bridge_instance);
		dmfree(oface);
		return 0;
	}
	return FAULT_9005;
}
