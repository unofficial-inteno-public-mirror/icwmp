#ifndef __LAN_INTERFACES_H
#define __LAN_INTERFACES_H

inline int init_lan_interface_args(struct dmctx *ctx, char *lif);
int get_lan_ethernet_interface_number(char *refparam, struct dmctx *ctx, char **value);
int lan_wlan_configuration_number();
int get_lan_wlan_configuration_number(char *refparam, struct dmctx *ctx, char **value);
int get_eth_name(char *refparam, struct dmctx *ctx, char **value);
int entry_method_root_InternetGatewayDevice_LANInterfaces(struct dmctx *ctx);
int get_lan_interface(struct dmctx *ctx, char *li);
int get_wlan_interface(struct dmctx *ctx, char *wli);

#endif