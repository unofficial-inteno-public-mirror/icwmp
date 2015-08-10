#ifndef __SE_WIFI_H
#define __SE_WIFI_H

struct sewifiargs
{
	struct uci_section *sewifisection;
};

inline int init_se_wifi(struct dmctx *ctx, struct uci_section *s);
int get_wifi_frequency(char *refparam, struct dmctx *ctx, char **value);
int get_wifi_maxassoc(char *refparam, struct dmctx *ctx, char **value);
int set_wifi_maxassoc(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_SE_Wifi(struct dmctx *ctx);

#endif