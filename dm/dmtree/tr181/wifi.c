/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2016 Inteno Broadband Technology AB
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 *
 */

#include <uci.h>
#include <stdio.h>
#include <ctype.h>
#include "dmuci.h"
#include "dmubus.h"
#include "dmcwmp.h"
#include "dmcommon.h"
#include "wifi.h"
#include "x_inteno_se_wifi.h"

struct wifi_radio_args cur_wifi_radio_args = {0};

inline int entry_wifi_radio(struct dmctx *ctx);

inline int init_wifi_radio(struct dmctx *ctx, struct uci_section *s, json_object *res, char *wiface)
{
	struct wifi_radio_args *args = &cur_wifi_radio_args;
	ctx->args = (void *)args;
	args->wifi_radio_sec = s;
	args->res = res;
	args->wiface = wiface;
	return 0;
}

inline int init_wifi_ssid(struct dmctx *ctx, struct uci_section *s)
{
	struct wifi_radio_args *args = &cur_wifi_radio_args;
	ctx->args = (void *)args;
	args->wifi_ssid_sec = s;
	return 0;
}
#if 0
int get_wifi_enable(char *refparam, struct dmctx *ctx, char **value)
{
	int i;
	char *val;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	dmuci_get_value_by_section_string(wlanargs->lwlansection, "disabled", &val);

	if (val[0] == '0')
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_wifi_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				dmuci_set_value_by_section(wlanargs->lwlansection, "disabled", "0");
			else
				dmuci_set_value_by_section(wlanargs->lwlansection, "disabled", "1");
			return 0;
	}
	return 0;
}

int get_wifi_status (char *refparam, struct dmctx *ctx, char **value)
{
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	dmuci_get_value_by_section_string(wlanargs->lwlansection, "disabled", value);
	if ((*value)[0] == '\0' || (*value)[0] == '0')
		*value = "Up";
	else
		*value = "Disabled";
	return 0;
}
#endif
int get_wifi_max_bit_rate (char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "hwmode", value);
	return 0;
}

int set_wifi_max_bit_rate(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "hwmode", value);
			return 0;
	}
	return 0;
}
int get_radio_frequency(char *refparam, struct dmctx *ctx, char **value)
{
	char *freq;
	json_object *res;
	char *wlan_name = section_name(cur_wifi_radio_args.wifi_radio_sec);

	dmubus_call("router", "wl", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "frequency", 0, NULL, &freq, NULL);
	dmastrcat(value, freq, "GHz");  // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_radio_operating_channel_bandwidth(char *refparam, struct dmctx *ctx, char **value)
{
	char *bandwith;

	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "bandwidth", value);
	if (value[0] == '\0')
	{
		DM_ASSERT(cur_wifi_radio_args.res, *value ="");
		json_select(cur_wifi_radio_args.res, "bandwidth", 0, NULL, &bandwith, NULL);
		dmastrcat(value, bandwith, "MHz"); // MEM WILL BE FREED IN DMMEMCLEAN
	}
	return 0;
}

int set_radio_operating_channel_bandwidth(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *pch, *spch, *dup;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dup = dmstrdup(value);
			pch = strtok_r(dup, "Mm", &spch);
			dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "bandwidth", pch);
			dmfree(dup);
			return 0;
	}
	return 0;
}

int get_radio_maxassoc(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "maxassoc", value);
	return 0;
}

int set_radio_maxassoc(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "maxassoc", value);
			return 0;
	}
	return 0;
}

int get_radio_dfsenable(char *refparam, struct dmctx *ctx, char **value)
{
	char *val;

	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "band", &val);
	if (val[0] == 'a') {
		dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "dfsc", value);
		if ((*value)[0] == '\0')
			*value = "0";
	}
	return 0;
}

int set_radio_dfsenable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *val;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "band", &val);
			if (val[0] == 'a') {
				string_to_bool(value, &b);
				if (b)
					dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "dfsc", "1");
				else
					dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "dfsc", "0");
			}
			return 0;
	}
	return 0;
}


int get_radio_supported_standard(char *refparam, struct dmctx *ctx, char **value)
{
	char *freq;

	DM_ASSERT(cur_wifi_radio_args.res, *value = "b, g, n, gst, lrs");
	json_select(cur_wifi_radio_args.res, "frequency", 0, NULL, &freq, NULL);
	if (strcmp(freq, "5") == 0)
		*value = "a, n, ac";
	else
		*value = "b, g, n, gst, lrs";
	return 0;
}

int get_radio_operating_standard(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "hwmode", value);
	if (strcmp(*value, "11b") == 0)
		*value = "b";
	else if (strcmp(*value, "11bg") == 0)
		*value = "g";
	else if (strcmp(*value, "11g") == 0 || strcmp(*value, "11gst") == 0 || strcmp(*value, "11lrs") == 0)
		*value = "g-only";
	else if (strcmp(*value, "11n") == 0 || strcmp(*value, "auto") == 0)
		*value = "n";
	return 0;
}

int get_radio_channel(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "channel", value);
	if (strcmp(*value, "auto") == 0 || (*value)[0] == '\0') {
		DM_ASSERT(cur_wifi_radio_args.res, *value ="");
		json_select(cur_wifi_radio_args.res, "channel", 0, NULL, value, NULL);
	}
	return 0;
}

int set_radio_channel(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "channel", value);
			return 0;
	}
	return 0;
}

int get_radio_auto_channel_enable(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "channel", value);
	if (strcmp(*value, "auto") == 0 || (*value)[0] == '\0')
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_radio_auto_channel_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;

	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				value = "auto";
			else {
				if(cur_wifi_radio_args.res == NULL)
					return 0;
				else
					json_select(cur_wifi_radio_args.res, "channel", 0, NULL, &value, NULL);
			}
			dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "channel", value);
			return 0;
	}
	return 0;
}

int get_radio_statistics_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_wifi_radio_args.wiface}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", -1, "tx_bytes", value, NULL);
	return 0;
}

int get_radio_statistics_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name",  cur_wifi_radio_args.wiface}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	return 0;
}

int get_radio_statistics_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_wifi_radio_args.wiface}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	return 0;
}

int get_radio_statistics_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *val = NULL;
	struct ldwlanargs *wlanargs = (struct ldwlanargs *)ctx->args;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name",  cur_wifi_radio_args.wiface}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	return 0;
}

////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_radio_alias1(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "radioalias", value);
	return 0;
}

int set_radio_alias1(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "radioalias", value);
			return 0;
	}
	return 0;
}
/////////////SUB ENTRIES///////////////
inline int entry_wifi_radio(struct dmctx *ctx)
{
	char *wnum = NULL, *wnum_last = NULL;
	char *wiface;
	char buf[12];
	struct uci_section *s = NULL;
	int wlctl_num = 0;
	json_object *res;

	uci_foreach_sections("wireless", "wifi-device", s) {
		wlctl_num = 0;
		wiface = section_name(s);
		if (wlctl_num != 0) {
			sprintf(buf, "%s.%d", wiface, wlctl_num);
			wiface = buf;
		}
		dmubus_call("router", "wl", UBUS_ARGS{{"vif", wiface}}, 1, &res);
		init_wifi_radio(ctx, s, res, wiface);
		wlctl_num++;
		wnum =  handle_update_instance(1, ctx, &wnum_last, update_instance_alias, 3, s, "radioinstance", "radioalias");
		SUBENTRY(entry_wifi_radio_instance, ctx, wnum);
	}
	return 0;
}
#if 0
inline int entry_wifi_ssid(struct dmctx *ctx)
{
	char *wnum = NULL, *wnum_last = NULL;
	struct uci_section *s = NULL;
	//iwlan = get_last_instance_lev2("wireless", "wifi-iface", "lwlaninstance", "network", section_name(landevice_section));
		uci_foreach_sections("wireless", "wifi-device", ss) {
		uci_foreach_option_eq("wireless", "wifi-iface", "device", section_name(ss), sss) {
		init_se_wifi(ctx, s);
		wnum =  handle_update_instance(1, ctx, &wnum_last, update_instance_alias, 3, s, "ssidinstance", "ssidalias");
		SUBENTRY(entry_wifi_ssid_instance, ctx, wnum);
	}
	return 0;
}

inline int entry_wifi_access_point(struct dmctx *ctx)
{
	char *wnum = NULL, *wnum_last = NULL;
	struct uci_section *s = NULL;
	uci_foreach_sections("wireless", "wifi-device", s) {
		init_se_wifi(ctx, s);
		wnum =  handle_update_instance(1, ctx, &wnum_last, update_instance_alias, 3, s, "accespointinstance", "accespointalias");
		SUBENTRY(entry_wifi_access_point_instance, ctx, wnum);
	}
	return 0;
}
#endif
//////////////////////////////////////

int entry_method_root_Wifi(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"WiFi.") {
		DMOBJECT(DMROOT"WiFi.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"WiFi.Radio.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"WiFi.SSID.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"WiFi.AccessPoint.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_wifi_radio, ctx);
		//SUBENTRY(entry_wifi_ssid, ctx);
		//SUBENTRY(entry_wifi_access_point, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_wifi_radio_instance(struct dmctx *ctx, char *wnum)
{
	IF_MATCH(ctx, DMROOT"WiFi.Radio.%s.", wnum) {
		DMOBJECT(DMROOT"WiFi.Radio.%s.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("Alias", ctx, "1", get_radio_alias1, set_radio_alias1, NULL, 0, 1, UNDEF, NULL);
		//DMPARAM("Enable", ctx, "1", get_wifi_enable, set_wifi_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		//DMPARAM("Status", ctx, "0", get_wlan_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("MaxBitRate", ctx, "1", get_wifi_max_bit_rate, set_wifi_max_bit_rate, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Frequency", ctx, "0", get_radio_frequency, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("OperatingChannelBandwidth", ctx, "1", get_radio_operating_channel_bandwidth, set_radio_operating_channel_bandwidth, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_MaxAssociations", ctx, "1", get_radio_maxassoc, set_radio_maxassoc, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_DFSEnable", ctx, "1", get_radio_dfsenable, set_radio_dfsenable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("SupportedStandards", ctx, "0", get_radio_supported_standard, NULL, NULL, 0, 1, UNDEF, NULL);
		//DMPARAM("Standard", ctx, "1", get_wlan_standard, set_wlan_standard, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("OperatingStandards", ctx, "0", get_radio_operating_standard, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ChannelsInUse", ctx, "1", get_radio_channel, get_radio_channel, NULL, 0, 1, UNDEF, NULL); /// TO CHECK
		DMPARAM("Channel", ctx, "1", get_radio_channel, get_radio_channel, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("AutoChannelEnable", ctx, "1", get_radio_auto_channel_enable, set_radio_auto_channel_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"WiFi.Radio.%s.Stats.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("BytesSent", ctx, "0", get_radio_statistics_tx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("BytesReceived", ctx, "0", get_radio_statistics_rx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsSent", ctx, "0", get_radio_statistics_tx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsReceived", ctx, "0", get_radio_statistics_rx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);

		return 0;
	}
	return FAULT_9005;
}
#if 0
inline int entry_wifi_ssid_instance(struct dmctx *ctx, char *wnum)
{
	IF_MATCH(ctx, DMROOT"WiFi.SSID.%s.", wnum) {
		DMOBJECT(DMROOT"WiFi.SSID.%s.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("Alias", ctx, "1", get_radio_alias, set_radio_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_wlan_enable, set_wlan_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_wlan_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SSID", ctx, "1", get_wlan_ssid, set_wlan_ssid, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "1", get_wlan_ssid, set_wlan_ssid, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LowerLayers", ctx, "0", get_wifi_frequency, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("BSSID", ctx, "0", get_wlan_bssid, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("BSSID", ctx, "0", get_wlan_bssid, NULL, NULL, 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"WiFi.SSID.%s.Stats.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("BytesSent", ctx, "0", get_wlan_devstatus_statistics_tx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("BytesReceived", ctx, "0", get_wlan_devstatus_statistics_rx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsSent", ctx, "0", get_wlan_devstatus_statistics_tx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsReceived", ctx, "0", get_wlan_devstatus_statistics_rx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_wifi_access_point_instance(struct dmctx *ctx, char *wnum)
{
	IF_MATCH(ctx, DMROOT"WiFi.AccessPoint.%s.", wnum) {
		DMOBJECT(DMROOT"WiFi.AccessPoint.%s.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("Alias", ctx, "1", get_radio_alias, set_radio_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_wlan_enable, set_wlan_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_wlan_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SSIDReference", ctx, "1", get_wlan_ssid, set_wlan_ssid, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SSIDAdvertisementEnabled", ctx, "1", get_wlan_ssid_advertisement_enable, set_wlan_ssid_advertisement_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("WMMEnable", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("AssociatedDeviceNumberOfEntries", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("MaxAssociatedDevices", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("MACAddressControlEnabled", ctx, "1", get_wlan_mac_control_enable, set_wlan_mac_control_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("AllowedMACAddress", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"WiFi.AccessPoint.%s.Security", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("ModesSupported", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("ModeEnabled", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("WEPKey", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("PreSharedKey", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("KeyPassphrase", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("RekeyingInterval", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("RadiusServerIPAddr", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("RadiusServerPort", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("RadiusSecret", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
#endif
