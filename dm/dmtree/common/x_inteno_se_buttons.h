#ifndef __SE_BUTTONS_H
#define __SE_BUTTONS_H

struct button_args
{
	struct uci_section *button_section;
};

extern DMLEAF X_INTENO_SE_ButtonParams[];
inline int browseXIntenoButton(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
inline int init_args_button(struct dmctx *ctx, struct uci_section *s);
int get_x_inteno_button_name(char *refparam, struct dmctx *ctx, char **value);
int get_x_inteno_button_hotplug(char *refparam, struct dmctx *ctx, char **value);
int get_x_inteno_button_hotplug_long(char *refparam, struct dmctx *ctx, char **value);
int get_x_inteno_button_minpress(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_button_minpress(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_button_longpress(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_button_longpress(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_button_enable(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_button_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_button_alias(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_button_alias(char *refparam, struct dmctx *ctx, int action, char *value);

#endif
