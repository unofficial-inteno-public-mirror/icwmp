#ifndef __DMENTRY_H__
#define __DMENTRY_H__

#include "dmcwmp.h"
extern struct list_head head_package_change;
extern unsigned char dmcli_timetrack;
extern unsigned char dmcli_evaluatetest;

enum ctx_init_enum {
	CTX_INIT_ALL,
	CTX_INIT_SUB
};

int dm_ctx_init(struct dmctx *ctx, unsigned int dm_type, unsigned int amd_version, unsigned int instance_mode);
int dm_ctx_init_sub(struct dmctx *ctx, unsigned int dm_type, unsigned int amd_version, unsigned int instance_mode);
int dm_entry_param_method(struct dmctx *ctx, int cmd, char *inparam, char *arg1, char *arg2);
int dm_entry_apply(struct dmctx *ctx, int cmd, char *arg1, char *arg2);
int dm_entry_load_enabled_notify(unsigned int dm_type, unsigned int amd_version, int instance_mode);
int adm_entry_get_linker_param(struct dmctx *ctx, char *param, char *linker, char **value);
int adm_entry_get_linker_value(struct dmctx *ctx, char *param, char **value);
int dm_entry_restart_services(void);
int dm_entry_upnp_restart_services(void);
void dm_upnp_apply_config(void);
int dm_entry_upnp_check_alarmonchange_param(struct dmctx *dmctx);
int dm_entry_upnp_check_eventonchange_param(struct dmctx *dmctx);
int dm_entry_upnp_check_versiononchange_param(struct dmctx *pctx);
int dm_entry_upnp_load_tracked_parameters(struct dmctx *dmctx);
int dm_entry_upnp_get_supported_parameters_update(struct dmctx *dmctx, char **value);
int dm_entry_upnp_get_supported_datamodel_update(struct dmctx *dmctx, char **value);
int dm_entry_upnp_get_attribute_values_update(struct dmctx *dmctx, char **value);
int dm_entry_upnp_get_configuration_update(struct dmctx *dmctx, char **value);
int dm_entry_upnp_get_current_configuration_version(struct dmctx *dmctx, char **value);

int dm_ctx_clean(struct dmctx *ctx);
int dm_ctx_clean_sub(struct dmctx *ctx);
void dm_execute_cli_shell(int argc, char** argv, unsigned int dmtype, unsigned int amd_version, unsigned int instance_mode);
void dm_execute_cli_command(char *file, unsigned int dmtype, unsigned int amd_version, unsigned int instance_mode);
void wepkey_cli(int argc, char** argv);
#endif
