#ifndef __LAYER3_FORWORDING_H
#define __LAYER3_FORWORDING_H

#define ROUTE_FILE "/proc/net/route"
struct routefwdargs
{
	char *permission;
	struct uci_section *routefwdsection;
	char *proute;
};
inline int init_args_rentry(struct dmctx *ctx, struct uci_section *s, char *permission, char *proute);
char *ip_to_hex(char *address);
char *hex_to_ip(char *address);
int proc_get_route_var_by_conf(struct uci_section *s, int name);
int get_layer3_enable(char *refparam, struct dmctx *ctx, char **value);
int set_layer3_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_layer3_status(char *refparam, struct dmctx *ctx, char **value);
int get_layer3_type(char *refparam, struct dmctx *ctx, char **value);
int get_layer3_destip(char *refparam, struct dmctx *ctx, char **value);
int set_layer3_destip(char *refparam, struct dmctx *ctx, int action, char *value);
int get_layer3_destmask(char *refparam, struct dmctx *ctx, char **value);
int set_layer3_destmask(char *refparam, struct dmctx *ctx, int action, char *value);
int get_layer3_src_address(char *refparam, struct dmctx *ctx, char **value);
int get_layer3_src_mask(char *refparam, struct dmctx *ctx, char **value);
int get_layer3_gatewayip(char *refparam, struct dmctx *ctx, char **value);
int set_layer3_gatewayip(char *refparam, struct dmctx *ctx, int action, char *value);
char *get_layer3_interface(struct dmctx *ctx);
int get_parameter_by_linker(char *refparam, struct dmctx *ctx, char **value);
char *get_linker_by_parameter();
int get_layer3_interface_linker_parameter(char *refparam, struct dmctx *ctx, char **value);
int set_layer3_interface_linker_parameter(char *refparam, struct dmctx *ctx, int action, char *value);
int get_layer3_metric(char *refparam, struct dmctx *ctx, char **value);
int set_layer3_metric(char *refparam, struct dmctx *ctx, int action, char *value);
int get_layer3_mtu(char *refparam, struct dmctx *ctx, char **value);
int set_layer3_mtu(char *refparam, struct dmctx *ctx, int action, char *value);
inline int get_object_layer3(struct dmctx *ctx, char *iroute, char *permission);
int set_layer3_def_conn_serv(char *refparam, struct dmctx *ctx, int action, char *value);
int get_layer3_nbr_entry(char *refparam, struct dmctx *ctx, char **value);
int entry_method_root_layer3_forwarding(struct dmctx *ctx);
#endif