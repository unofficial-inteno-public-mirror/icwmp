#ifndef __SE_LOGINCFG_H
#define __SE_LOGINCFG_H

int set_x_bcm_password(char *refparam, struct dmctx *ctx, int action, char *value, char *user_type);
int set_x_bcm_admin_password(char *refparam, struct dmctx *ctx, int action, char *value);
int set_x_bcm_support_password(char *refparam, struct dmctx *ctx, int action, char *value);
int set_x_bcm_user_password(char *refparam, struct dmctx *ctx, int action, char *value);
int set_x_bcm_root_password(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_X_INTENO_SE_LOGIN_CFG(struct dmctx *ctx);

#endif