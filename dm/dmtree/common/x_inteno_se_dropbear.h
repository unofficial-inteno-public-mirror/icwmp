#ifndef __SE_DROPBEAR_H
#define __SE_DROPBEAR_H

struct dropbear_args
{
	struct uci_section *dropbear_section;
};

int entry_method_root_X_INTENO_SE_DROPBEAR(struct dmctx *ctx);
#endif
