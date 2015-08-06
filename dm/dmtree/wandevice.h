/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#ifndef __WAN_DEVICE_H
#define __WAN_DEVICE_H
#include <libubox/blobmsg_json.h>
#include <json/json.h>
inline int init_wanargs(struct dmctx *ctx, char *idev, char *fdev);
inline int init_wancprotoargs(struct dmctx *ctx, struct uci_section *s);
inline int init_wancdevargs(struct dmctx *ctx, char *fwan);
char *get_wan_device_wan_dsl_traffic();
int check_multiwan_interface(struct uci_section *s);
int network_get_ipaddr(char **value, char *iface);
/************************************************************************** 
**** ****  function related to get_wandevice_wandevice_parameters  **** ****
***************************************************************************/
int get_wan_device_wan_access_type(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_wan_dsl_interface_config_status(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_wan_dsl_interface_config_modulation_type(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_datapath(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreamcurrrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreammaxrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreamattenuation(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreamcurrrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreammaxrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreamattenuation(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value);
int get_annexm_status(char *refparam, struct dmctx *ctx, char **value);
int set_annexm_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, char **value);
int set_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_eth_intf_status(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_mac(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value);
int get_wandevice_wandevice_parameters(struct dmctx *ctx, char *dev, char *fdev);
/*****/
int entry_method_root_WANDevice(struct dmctx *ctx);
/************************************************************************** 
**** ****  function related to get_wandevice_wanconnectiondevice_parameters **** ****
***************************************************************************/
int get_wan_dsl_link_config_enable(char *refparam, struct dmctx *ctx, char **value);
int get_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, char **value);
int set_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_dsl_link_config_atm_encapsulation(char *refparam, struct dmctx *ctx, char **value);
int set_wan_dsl_link_config_atm_encapsulation(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wandevice_wanconnectiondevice_parameters(struct dmctx *ctx, char *idev, char *iwan);
/************************************************************************** 
**** ****  function related to get_wandevice_wanprotoclconnection_parameters **** ****
***************************************************************************/
int get_wandevice_wanprotoclconnection_parameters(struct dmctx *ctx, char *idev, char *iwan, char *iconp, char *proto);
int get_wan_device_mng_status(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_mng_interface_ip(char *refparam, struct dmctx *ctx, char **value);
int get_wan_ip_link_connection_connection_type(char *refparam, struct dmctx *ctx, char **value);
int set_wan_ip_link_connection_connection_type(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_ip_link_connection_addressing_type(char *refparam, struct dmctx *ctx, char **value);
int set_wan_ip_link_connection_addressing_type(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_ip_link_connection_nat_enabled(char *refparam, struct dmctx *ctx, char **value);
int set_wan_ip_link_connection_nat_enabled(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_igmp_rule_idx(char *iface, struct uci_section **rule, struct uci_section **zone, char **enable);
int get_wan_ip_link_connection_igmp_enabled(char *refparam, struct dmctx *ctx, char **value);
int set_wan_ip_link_connection_igmp_enabled(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_ip_link_connection_dns_enabled(char *refparam, struct dmctx *ctx, char **value);
int set_wan_ip_link_connection_dns_enabled(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_device_ppp_status(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_ppp_interface_ip(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_mng_interface_mac(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_ppp_username(char *refparam, struct dmctx *ctx, char **value);
int set_wan_device_username(char *refparam, struct dmctx *ctx, int action, char *value);
int set_wan_device_password(char *refparam, struct dmctx *ctx, int action, char *value);



//OLD
int get_wan_device_wan_dsl_interface_config_status(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_wan_dsl_interface_config_modulation_type(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_datapath(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreamcurrrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreammaxrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreamattenuation(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_downstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreamcurrrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreammaxrate(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreamattenuation(char *refparam, struct dmctx *ctx, char **value);
int get_wan_device_dsl_upstreamnoisemargin(char *refparam, struct dmctx *ctx, char **value);
int get_annexm_status(char *refparam, struct dmctx *ctx, char **value);
int set_annexm_enable(char *refparam, struct dmctx *ctx, int action,  char *value);
int get_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, char **value);
int set_wan_eth_intf_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wan_eth_intf_status(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_mac(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value);
int get_wan_eth_intf_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value);
int get_wandevice_wandevice_parameters (struct dmctx *ctx, char *dev, char *fdev);
int entry_method_root_WANDevice(struct dmctx *ctx);
int get_wan_dsl_link_config_enable(char *refparam, struct dmctx *ctx, char **value);
int set_wan_dsl_link_config_destination_address(char *refparam, struct dmctx *ctx, int action,  char *value);
int get_wan_dsl_link_config_atm_encapsulation(char *refparam, struct dmctx *ctx, char **value);
int set_wan_dsl_link_config_atm_encapsulation(char *refparam, struct dmctx *ctx, int action,  char *value);
int get_wandevice_wanconnectiondevice_parameters(struct dmctx *ctx, char *idev, char *iwan);
#endif