#ifndef __LAYER3_FORWORDING_H
#define __LAYER3_FORWORDING_H

#define ROUTE_FILE "/proc/net/route"
#define MAX_PROC_ROUTE 256

struct proc_route {
	char *iface;
	char *flags;
	char *refcnt;
	char *use;
	char *metric;
	char *mtu;
	char *window;
	char *irtt;
	char destination[16];
	char gateway[16];
	char mask[16];
};

struct routefwdargs
{
	char *permission;
	struct uci_section *routefwdsection;
	struct proc_route *proute;
	int type;
};

int entry_method_root_layer3_forwarding(struct dmctx *ctx);
#endif
