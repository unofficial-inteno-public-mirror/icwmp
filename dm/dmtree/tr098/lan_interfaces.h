#ifndef __LAN_INTERFACES_H
#define __LAN_INTERFACES_H

struct linterfargs
{
	char *linterf;
	struct uci_section *port_sec;
};

struct ethrnet_ifaces_s
{
	char *eths[16];
	int eths_size;
};

bool check_laninterfaces(struct dmctx *dmctx, void *data);
extern DMLEAF tlaninterface_lanParam[];
extern DMLEAF tlaninterface_wlanParam[];
extern DMLEAF tLANInterfacesParam[];
extern DMOBJ tLANInterfacesObj[];
#endif
