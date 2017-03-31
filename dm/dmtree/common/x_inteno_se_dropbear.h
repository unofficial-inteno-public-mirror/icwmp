#ifndef __SE_DROPBEAR_H
#define __SE_DROPBEAR_H

struct dropbear_args
{
	struct uci_section *dropbear_section;
};

extern DMLEAF X_INTENO_SE_DropbearParams[];
inline int browseXIntenoDropbear(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int init_args_dropbear(struct dmctx *ctx, struct uci_section *s);
int get_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, int action, char *value);
int add_dropbear_instance(struct dmctx *ctx, char **instancepara);
int delete_dropbear_instance(struct dmctx *ctx, unsigned char del_action);

#endif
