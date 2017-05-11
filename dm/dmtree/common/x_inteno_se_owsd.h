#ifndef __SE_OWSD_H
#define __SE_OWSD_H

int browseXIntenoOwsdListenObj(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int get_x_inteno_owsd_global_sock(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_global_sock(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_global_redirect(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_global_redirect(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_listenobj_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_listenobj_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_listenobj_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_listenobj_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_listenobj_ipv6_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_listenobj_ipv6_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_listenobj_whitelist_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_listenobj_whitelist_interface(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_listenobj_whitelist_dhcp(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_listenobj_whitelist_dhcp(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_listenobj_origin(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_listenobj_origin(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int get_x_inteno_owsd_listenobj_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value);
int set_x_inteno_owsd_listenobj_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action);
int add_owsd_listen(char *refparam, struct dmctx *ctx, void *data, char **instancepara);
int delete_owsd_listen_instance(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action);

extern DMLEAF XIntenoSeOwsdParams[];
extern DMOBJ XIntenoSeOwsdObj[];
extern DMLEAF X_INTENO_SE_ListenObjParams[];

#endif
