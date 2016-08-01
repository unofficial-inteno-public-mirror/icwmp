#ifndef __SE_IPACCCFG_H
#define __SE_IPACCCFG_H

struct ipaccargs
{
	struct uci_section *ipaccsection;
};

struct pforwardrgs
{
	struct uci_section *forwardsection;
};

extern DMOBJ tSe_IpAccObj[];
extern DMLEAF tSe_IpAccCfgParam[];
extern DMLEAF tSe_PortForwardingParam[];
#endif