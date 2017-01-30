#ifndef __SE_BUTTONS_H
#define __SE_BUTTONS_H

struct button_args
{
	struct uci_section *button_section;
};

int entry_method_root_X_INTENO_SE_BUTTONS(struct dmctx *ctx);
#endif
