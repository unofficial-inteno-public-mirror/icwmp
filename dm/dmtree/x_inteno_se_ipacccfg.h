#ifndef __SE_IPACCCFG_H
#define __SE_IPACCCFG_H

struct ipaccargs
{
	struct uci_section *ipaccsection;
};

struct pforwardrgs
{
	struct uci_section *forwardsection;
};

inline int init_args_ipacc(struct dmctx *ctx, struct uci_section *s);
inline int init_args_pforward(struct dmctx *ctx, struct uci_section *s);
int get_x_bcm_com_ip_acc_list_cfgobj_enable(char *refparam, struct dmctx *ctx, char **value);
int set_x_bcm_com_ip_acc_list_cfgobj_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_cfgobj_address_netmask(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_cfgobj_address_netmask(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_bcm_com_ip_acc_list_cfgobj_acc_port(char *refparam, struct dmctx *ctx, char **value);
int set_x_bcm_com_ip_acc_list_cfgobj_acc_port(char *refparam, struct dmctx *ctx, int action, char *value);
inline int get_object_ip_acc_list_cfgobj(struct dmctx *ctx, char *irule);
int get_port_forwarding_name(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_name(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_enable(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_loopback(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_loopback(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_protocol(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_protocol(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_external_zone(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_external_zone(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_internal_zone(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_internal_zone(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_external_port(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_external_port(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_internal_port(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_internal_port(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_source_port(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_source_port(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_internal_ipaddress(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_internal_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_external_ipaddress(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_external_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_source_ipaddress(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_source_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value);
int get_port_forwarding_src_mac(char *refparam, struct dmctx *ctx, char **value);
int set_port_forwarding_src_mac(char *refparam, struct dmctx *ctx, int action, char *value);
inline int get_object_port_forwarding(struct dmctx *ctx, char *iforward);
int entry_method_root_X_INTENO_SE_IpAccCfg(struct dmctx *ctx);
#endif