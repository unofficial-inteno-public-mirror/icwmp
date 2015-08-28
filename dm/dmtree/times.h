#ifndef __TIMES_H
#define __TIMES_H

int get_time_enable(char *refparam, struct dmctx *ctx, char **value);
int set_time_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_time_ntpserver(char *refparam, struct dmctx *ctx, char **value, int index);
int get_time_ntpserver1(char *refparam, struct dmctx *ctx, char **value);
int get_time_ntpserver2(char *refparam, struct dmctx *ctx, char **value);
int get_time_ntpserver3(char *refparam, struct dmctx *ctx, char **value);
int get_time_ntpserver4(char *refparam, struct dmctx *ctx, char **value);
int get_time_ntpserver5(char *refparam, struct dmctx *ctx, char **value);
int set_time_ntpserver(char *refparam, struct dmctx *ctx, int action, char *value, int index);
int set_time_ntpserver1(char *refparam, struct dmctx *ctx, int action, char *value);
int set_time_ntpserver2(char *refparam, struct dmctx *ctx, int action, char *value);
int set_time_ntpserver3(char *refparam, struct dmctx *ctx, int action, char *value);
int set_time_ntpserver4(char *refparam, struct dmctx *ctx, int action, char *value);
int set_time_ntpserver5(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_Time(struct dmctx *ctx);

#endif