#ifndef __SE_WIFI_H
#define __SE_WIFI_H

struct sewifiargs
{
	struct uci_section *sewifisection;
};

int entry_method_root_SE_Wifi(struct dmctx *ctx);

#endif
