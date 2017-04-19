#ifndef __DMENTRY_H__
#define __DMENTRY_H__

#include "dmcwmp.h"
extern struct list_head head_package_change;

enum ctx_init_enum {
	CTX_INIT_ALL,
	CTX_INIT_SUB
};

int dm_global_init(void);
int dm_ctx_init(struct dmctx *ctx);
int dm_ctx_init_sub(struct dmctx *ctx);
int dm_entry_param_method(struct dmctx *ctx, int cmd, char *inparam, char *arg1, char *arg2);
int dm_entry_apply(struct dmctx *ctx, int cmd, char *arg1, char *arg2);
int dm_entry_load_enabled_notify();
int adm_entry_get_linker_param(char *param, char *linker, char **value);
int adm_entry_get_linker_value(char *param, char **value);
int dm_entry_restart_services();
int dm_ctx_clean(struct dmctx *ctx);
int dm_ctx_clean_sub(struct dmctx *ctx);
void dm_entry_cli(int argc, char** argv);
void wepkey_cli(int argc, char** argv);
void dmentry_instance_lookup_inparam(struct dmctx *ctx);
#endif
