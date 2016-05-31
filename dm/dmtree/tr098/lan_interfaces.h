#ifndef __LAN_INTERFACES_H
#define __LAN_INTERFACES_H

struct linterfargs
{
	char *linterf;
	char *eths[16];
	int eths_size;
	struct uci_section *port_sec;
};

struct wifaceargs
{
	struct uci_section *wiface_sec;
};

int entry_method_root_InternetGatewayDevice_LANInterfaces(struct dmctx *ctx);

#endif