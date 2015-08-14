#ifndef __TIMES_H
#define __TIMES_H

int get_time_enable(char *refparam, struct dmctx *ctx, char **value);
int set_time_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_time_ntpserver(char *refparam, struct dmctx *ctx, char **value);
int set_time_ntpserver(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_Time(struct dmctx *ctx);

#endif