#ifndef __SE_ICE_H
#define __SE_ICE_H

int get_ice_cloud_enable(char *refparam, struct dmctx *ctx, char **value);
int set_ice_cloud_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_ice_cloud_server(char *refparam, struct dmctx *ctx, char **value);
int set_ice_cloud_server(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_X_INTENO_SE_Ice(struct dmctx *ctx);

#endif