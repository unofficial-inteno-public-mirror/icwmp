#ifndef __SE_OWSD_H
#define __SE_OWSD_H

struct owsd_listenargs
{
	struct uci_section *owsd_listensection;
};

int entry_method_root_X_INTENO_SE_OWSD(struct dmctx *ctx);
#endif
