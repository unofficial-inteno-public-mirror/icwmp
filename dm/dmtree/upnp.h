#ifndef __UPNP_H
#define __UPNP_H

int get_upnp_enable(char *refparam, struct dmctx *ctx, char **value);
int set_upnp_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_upnp_status(char *refparam, struct dmctx *ctx, char **value);
int entry_method_root_upnp(struct dmctx *ctx);

#endif