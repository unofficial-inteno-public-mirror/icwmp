/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 */

#ifndef __LAN_DEVICE_H
#define __LAN_DEVICE_H
#include <libubox/blobmsg_json.h>
#include <json/json.h>
#define NVRAM_FILE "/proc/nvram/WpaKey"
inline int init_ldargs_dhcp(struct dmctx *ctx, struct uci_section *s);
inline int init_ldargs_wlan(struct dmctx *ctx, struct uci_section *s, int wlctl_num, char *wunit, int pki);
inline int init_ldargs_eth_cfg(struct dmctx *ctx, char *eth);
inline int init_ldargs_ip(struct dmctx *ctx, struct uci_section *s);
inline int init_ldargs_lan(struct dmctx *ctx, struct uci_section *s);
int ip_to_int(char *address);
int reset_wlan(struct uci_section *s);
int get_lan_dns(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dns(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_server_configurable(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_server_configurable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_server_enable(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_server_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_interval_address_start(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_address_start(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_interval_address_end(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_address_end(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_reserved_addresses(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_subnetmask(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_subnetmask(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_iprouters(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_iprouters(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_leasetime(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_leasetime(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_dhcp_domainname(char *refparam, struct dmctx *ctx, char **value);
int set_lan_dhcp_domainname(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_host_nbr_entries(char *refparam, struct dmctx *ctx, char **value);
int filter_lan_device_interface(struct uci_section *s, void *v);
int filter_lan_ip_interface(struct uci_section *ss, void *v);
int filter_is_dhcp_interface(struct uci_section *s, char *str); ////SEE IF USED
/**landevice_lanhostconfigmanagement_ipinterface**/
char *json_parse_string(json_object *jobj, char *key_val);
int get_interface_enable_ubus (char *refparam, struct dmctx *ctx, char **value);
int set_interface_enable_ubus(char *refparam, struct dmctx *ctx, int action, char *value);
int get_interface_firewall_enabled(char *refparam, struct dmctx *ctx, char **value);
struct uci_section *create_firewall_zone_config(char *fwl, char *iface, char *input, char *forward, char *output);
int set_interface_firewall_enabled(char *refparam, struct dmctx *ctx, int action, char *value);
int get_interface_ipaddress (char *refparam, struct dmctx *ctx, char **value);
int set_interface_ipaddress(char *refparam, struct dmctx *ctx, int action, char *value);
int get_interface_subnetmask (char *refparam, struct dmctx *ctx, char **value);
int set_interface_subnetmask(char *refparam, struct dmctx *ctx, int action, char *value);
int get_interface_addressingtype (char *refparam, struct dmctx *ctx, char **value);
int set_interface_addressingtype(char *refparam, struct dmctx *ctx, int action, char *value);
int get_dhcpstaticaddress_enable (char *refparam, struct dmctx *ctx, char **value);
int set_dhcpstaticaddress_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_dhcpstaticaddress_chaddr(char *refparam, struct dmctx *ctx, char **value);
int set_dhcpstaticaddress_chaddr(char *refparam, struct dmctx *ctx, int action, char *value);
int get_dhcpstaticaddress_yiaddr(char *refparam, struct dmctx *ctx, char **value);
int set_dhcpstaticaddress_yiaddr(char *refparam, struct dmctx *ctx, int action, char *value);
int entry_method_root_LANDevice(struct dmctx *ctx);
int get_wlan_enable (char *refparam, struct dmctx *ctx, char **value);
int set_wlan_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_status (char *refparam, struct dmctx *ctx, char **value);
int get_wlan_bssid (char *refparam, struct dmctx *ctx, char **value);
int get_wlan_max_bit_rate (char *refparam, struct dmctx *ctx, char **value);
int set_wlan_max_bit_rate(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_channel(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_channel(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_auto_channel_enable(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_auto_channel_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_ssid(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_ssid(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_beacon_type(char *refparam, struct dmctx *ctx, char **value); 
int set_wlan_beacon_type(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_mac_control_enable(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_mac_control_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_standard(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_standard(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_wep_key_index(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_wep_key_index(char *refparam, struct dmctx *ctx, int action, char *value);
int set_wlan_key_passphrase(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_wep_encryption_level(char *refparam, struct dmctx *ctx, char **value);
int get_wlan_basic_encryption_modes(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_basic_encryption_modes(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_basic_authentication_mode(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_basic_authentication_mode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_wpa_encryption_modes(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_wpa_encryption_modes(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_wpa_authentication_mode(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_wpa_authentication_mode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_ieee_11i_encryption_modes(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_ieee_11i_encryption_modes(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_ieee_11i_authentication_mode(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_ieee_11i_authentication_mode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_radio_enabled(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_radio_enabled(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_device_operation_mode(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_device_operation_mode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_authentication_service_mode(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_authentication_service_mode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_total_associations(char *refparam, struct dmctx *ctx, char **value);
int get_wlan_devstatus_statistics_tx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_wlan_devstatus_statistics_rx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_wlan_devstatus_statistics_tx_packets(char *refparam, struct dmctx *ctx, char **value);
int get_wlan_devstatus_statistics_rx_packets(char *refparam, struct dmctx *ctx, char **value);
int get_wlan_ssid_advertisement_enable(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_ssid_advertisement_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_wps_enable(char *refparam, struct dmctx *ctx, char **value);
int set_wlan_wps_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_se_channelmode(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_se_channelmode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_se_supported_standard(char *refparam, struct dmctx *ctx, char **value);
int get_x_inteno_se_operating_channel_bandwidth(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_se_operating_channel_bandwidth(char *refparam, struct dmctx *ctx, int action, char *value);
int get_x_inteno_se_maxssid(char *refparam, struct dmctx *ctx, char **value);
int set_x_inteno_se_maxssid(char *refparam, struct dmctx *ctx, int action, char *value);
int set_wlan_wep_key(char *refparam, struct dmctx *ctx, int action, char *value);
inline int get_landevice_lanhostconfigmanagement_ipinterface (struct dmctx *ctx, char *idev, char *ilan);
inline int get_landevice_lanhostconfigmanagement_dhcpstaticaddress(struct dmctx *ctx, char *idev, char *idhcp);
inline int get_landevice_wlanconfiguration_generic(struct dmctx *ctx, char *idev,char *iwlan);
/**landevice_wlanconfiguration_presharedkey**/
inline int get_landevice_wlanconfiguration_presharedkey(struct dmctx *ctx, int pki, char *idev, char *iwlan);
int set_wlan_pre_shared_key(char *refparam, struct dmctx *ctx, int action, char *value);
int get_wlan_psk_assoc_MACAddress(char *refparam, struct dmctx *ctx, char **value);
//*get_landevice_ethernet_interface_config **/
int get_lan_ethernet_interfaces(char *ndev, char *iface[], int taille, int *length);
int get_lan_eth_iface_cfg_enable(char *refparam, struct dmctx *ctx, char **value);
int set_lan_eth_iface_cfg_enable(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_eth_iface_cfg_status(char *refparam, struct dmctx *ctx, char **value);
int get_lan_eth_iface_cfg_maxbitrate(char *refparam, struct dmctx *ctx, char **value);
int set_lan_eth_iface_cfg_maxbitrate(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_eth_iface_cfg_duplexmode(char *refparam, struct dmctx *ctx, char **value);
int set_lan_eth_iface_cfg_duplexmode(char *refparam, struct dmctx *ctx, int action, char *value);
int get_lan_eth_iface_cfg_stats_tx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_lan_eth_iface_cfg_stats_rx_bytes(char *refparam, struct dmctx *ctx, char **value);
int get_lan_eth_iface_cfg_stats_tx_packets(char *refparam, struct dmctx *ctx, char **value);
int get_lan_eth_iface_cfg_stats_rx_packets(char *refparam, struct dmctx *ctx, char **value);
inline int get_landevice_ethernet_interface_config( struct dmctx *ctx, char *idev, char *ieth);
#endif