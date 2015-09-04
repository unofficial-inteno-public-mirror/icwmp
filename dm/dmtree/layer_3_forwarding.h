#ifndef __LAYER3_FORWORDING_H
#define __LAYER3_FORWORDING_H

#define ROUTE_FILE "/proc/net/route"
#define MAX_PROC_ROUTE 256

struct proc_route {
	char *iface;
	char *flags;
	char *refcnt;
	char *use;
	char *metric;
	char *mtu;
	char *window;
	char *irtt;
	char destination[16];
	char gateway[16];
	char mask[16];
};

struct routefwdargs
{
	char *permission;
	struct uci_section *routefwdsection;
	struct proc_route *proute;
	int type;
};

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
