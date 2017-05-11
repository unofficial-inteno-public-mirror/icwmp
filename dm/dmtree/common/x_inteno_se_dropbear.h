#ifndef __SE_DROPBEAR_H
#define __SE_DROPBEAR_H

extern DMLEAF X_INTENO_SE_DropbearParams[];
int browseXIntenoDropbear(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int get_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_root_password_auth(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_root_login(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_verbose(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_gateway_ports(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_rsakeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_dsskeyfile(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_ssh_keepalive(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_idle_timeout(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_banner_file(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_dropbear_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int add_dropbear_instance(char *refparam, struct dmctx *ctx, void *data, char **instancepara);
int delete_dropbear_instance(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action);

#endif
