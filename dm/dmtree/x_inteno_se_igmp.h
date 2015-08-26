#ifndef __SE_IGMP_H
#define __SE_IGMP_H

int get_igmp_dscp_mark(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_dscp_mark(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_proxy_interface(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_proxy_interface(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_default_version(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_default_version(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_query_interval(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_query_interval(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_query_response_interval(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_query_response_interval(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_last_member_queryinterval(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_last_member_queryinterval(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_robustness_value(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_robustness_value(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_multicast_enable(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_multicast_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_fastleave_enable(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_fastleave_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_joinimmediate_enable(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_joinimmediate_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_proxy_enable(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_proxy_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_maxgroup(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_maxgroup(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_maxsources(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_maxsources(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_maxmembers(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_maxmembers(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_snooping_mode(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_snooping_mode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_igmp_snooping_interface(char *refparam, struct dmctx *ctx, char **value);
int set_igmp_snooping_interface(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_X_INTENO_SE_IGMP(struct dmctx *ctx);

#endif