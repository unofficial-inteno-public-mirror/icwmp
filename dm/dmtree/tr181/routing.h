#ifndef __ROUTING_H
#define __ROUTING_H

#define ROUTING_FILE "/proc/net/route"
#define MAX_PROC_ROUTING 256

struct proc_routing {
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

struct routingfwdargs
{
	char *permission;
	struct uci_section *routefwdsection;
	struct proc_routing *proute;
	int type;
};

extern DMLEAF tRouterInstParam[];
extern DMLEAF tIPv4ForwardingParam[];
extern DMOBJ tRoutingObj[];
extern DMLEAF tRoutingParam[];
extern DMOBJ tRouterObj[];

int browseRouterInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseIPv4ForwardingInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
#endif
