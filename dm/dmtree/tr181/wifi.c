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

struct wifi_radio_args cur_wifi_radio_args = {0};
struct wifi_ssid_args cur_wifi_ssid_args = {0};

inline int entry_wifi_radio(struct dmctx *ctx);

inline int init_wifi_radio(struct dmctx *ctx, struct uci_section *s)
{
	struct wifi_radio_args *args = &cur_wifi_radio_args;
	ctx->args = (void *)args;
	args->wifi_radio_sec = s;
	return 0;
}

inline int init_wifi_ssid(struct dmctx *ctx, struct uci_section *s, char *wiface, char *linker)
{
	struct wifi_ssid_args *args = &cur_wifi_ssid_args;
	args->wifi_ssid_sec = s;
	args->ifname = wiface;
	args->linker = linker;
	return 0;
}

int get_wifi_enable(char *refparam, struct dmctx *ctx, char **value)
{
	int i;
	char *val;
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "disabled", &val);
	if (val[0] == '0')
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_wifi_enable(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "disabled", "0");
			else
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "disabled", "1");
			return 0;
	}
	return 0;
}

int get_wifi_status (char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "disabled", value);
	if ((*value)[0] == '\0' || (*value)[0] == '0')
		*value = "Up";
	else
		*value = "Down";
	return 0;
}

int get_wlan_ssid(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "ssid", value);
	return 0;
}

int set_wlan_ssid(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "ssid", value);
			return 0;
	}
	return 0;
}

int get_wlan_bssid(char *refparam, struct dmctx *ctx, char **value)
{
	char *wlan_name;
	json_object *res;

	wlan_name = section_name(cur_wifi_radio_args.wifi_radio_sec);
	dmubus_call("router", "wl", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "bssid", 0, NULL, value, NULL);
	return 0;
}

int get_radio_enable(char *refparam, struct dmctx *ctx, char **value)
{
	int i;
	char *val;
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "disabled", &val);

	if (val[0] == '1')
		*value = "0";
	else
		*value = "1";
	return 0;
}

int set_radio_enable(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "disabled", "0");
			else
				dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "disabled", "1");
			return 0;
	}
	return 0;
}

int get_radio_status (char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "disabled", value);
	if ((*value)[0] == '1')
		*value = "Down";
	else
		*value = "Up";
	return 0;
}
int get_radio_max_bit_rate (char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "hwmode", value);
	return 0;
}

int set_radio_max_bit_rate(char *refparam, struct dmctx *ctx, int action, char *value)
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
	if(strcmp(freq, "2") == 0 ) {
		dmastrcat(value, freq, ".4GHz");  // MEM WILL BE FREED IN DMMEMCLEAN
		return 0;
	}
	dmastrcat(value, freq, "GHz");  // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_radio_operating_channel_bandwidth(char *refparam, struct dmctx *ctx, char **value)
{
	char *bandwith;
	json_object *res;
	char *wlan_name;
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "bandwidth", value);
	if (value[0] == '\0')
	{
		wlan_name = section_name(cur_wifi_radio_args.wifi_radio_sec);
		dmubus_call("router", "wl", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "bandwidth", 0, NULL, &bandwith, NULL);
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
	char *freq, *wlan_name;
	json_object *res;
	wlan_name = section_name(cur_wifi_radio_args.wifi_radio_sec);
	dmubus_call("router", "wl", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
	DM_ASSERT(res, *value = "b, g, n");
	json_select(res, "frequency", 0, NULL, &freq, NULL);
	if (strcmp(freq, "5") == 0)
		*value = "a, n, ac";
	else
		*value = "b, g, n";
	return 0;
}

int get_radio_operating_standard(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "hwmode", value);
	if (strcmp(*value, "11b") == 0)
		*value = "b";
	else if (strcmp(*value, "11bg") == 0)
		*value = "b,g";
	else if (strcmp(*value, "11g") == 0 || strcmp(*value, "11gst") == 0 || strcmp(*value, "11lrs") == 0)
		*value = "g";
	else if (strcmp(*value, "11n") == 0 || strcmp(*value, "auto") == 0)
		*value = "n";
	else if (strcmp(*value, "11ac") == 0)
		*value = "ac";
	return 0;
}

int set_radio_operating_standard(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *freq, *wlan_name;
	json_object *res;
	switch (action) {
			case VALUECHECK:
				return 0;
			case VALUESET:
				wlan_name = section_name(cur_wifi_radio_args.wifi_radio_sec);
				dmubus_call("router", "wl", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
				json_select(res, "frequency", 0, NULL, &freq, NULL);
				if (strcmp(freq, "5") == 0) {
					 if (strcmp(value, "n") == 0)
						value = "11n"; 
					 else if (strcmp(value, "ac") == 0)
						value = "11ac";
					dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "hwmode", value);
				} else {
					if (strcmp(value, "b") == 0)
						value = "11b";
					else if (strcmp(value, "b,g") == 0 || strcmp(value, "g,b") == 0)
						value = "11bg";
					else if (strcmp(value, "g") == 0)
						value = "11g";
					 else if (strcmp(value, "n") == 0)
						value = "11n";
					dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "hwmode", value);
				}
				return 0;
		}
		return 0;
}

int get_radio_channel(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	char *wlan_name;
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "channel", value);
	if (strcmp(*value, "auto") == 0 || (*value)[0] == '\0') {
		wlan_name = section_name(cur_wifi_radio_args.wifi_radio_sec);
		dmubus_call("router", "wl", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
		DM_ASSERT(res, *value = "");
		json_select(res, "channel", 0, NULL, value, NULL);
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
	json_object *res;
	char *wlan_name;
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
				wlan_name = section_name(cur_wifi_radio_args.wifi_radio_sec);
				dmubus_call("router", "wl", UBUS_ARGS{{"vif", wlan_name}}, 1, &res);
				if(res == NULL)
					return 0;
				else
					json_select(res, "channel", 0, NULL, &value, NULL);
			}
			dmuci_set_value_by_section(cur_wifi_radio_args.wifi_radio_sec, "channel", value);
			return 0;
	}
	return 0;
}
//////////////////////GET STAT/////////////////////////////////////
int get_radio_statistics_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;

	dmubus_call("network.device", "status", UBUS_ARGS{{"name", section_name(cur_wifi_radio_args.wifi_radio_sec)}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", -1, "tx_bytes", value, NULL);
	return 0;
}

int get_radio_statistics_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", section_name(cur_wifi_radio_args.wifi_radio_sec)}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	return 0;
}

int get_radio_statistics_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", section_name(cur_wifi_radio_args.wifi_radio_sec)}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	return 0;
}

int get_radio_statistics_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", section_name(cur_wifi_radio_args.wifi_radio_sec)}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	return 0;
}
//////////////////////GET STAT/////////////////////////////////////
int get_ssid_statistics_tx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_wifi_ssid_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", -1, "tx_bytes", value, NULL);
	return 0;
}

int get_ssid_statistics_rx_bytes(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_wifi_ssid_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_bytes", value, NULL);
	return 0;
}

int get_ssid_statistics_tx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_wifi_ssid_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "tx_packets", value, NULL);
	return 0;
}

int get_ssid_statistics_rx_packets(char *refparam, struct dmctx *ctx, char **value)
{
	json_object *res;
	dmubus_call("network.device", "status", UBUS_ARGS{{"name", cur_wifi_ssid_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "statistics", 0, "rx_packets", value, NULL);
	return 0;
}

int get_wlan_ssid_advertisement_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *hidden;
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "hidden", &hidden);
	if (hidden[0] == '1' && hidden[1] == '\0')
		*value = "0";
	else
		*value = "1";
	return 0;
}

int set_wlan_ssid_advertisement_enable(char *refparam, struct dmctx *ctx, int action, char *value)
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
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "hidden", "");
			else
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "hidden", "1");
			return 0;

	}
	return 0;
}

int get_wmm_enabled(char *refparam, struct dmctx *ctx, char **value)
{
	bool b;
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "device", value);
	dmuci_get_option_value_string("wireless", *value, "wmm", value);
	string_to_bool(*value, &b);
		if (b)
			*value = "Enabled";
		else
			*value = "Disabled";

	return 0;
}

int set_wmm_enabled(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	char *device;
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
		case VALUESET:
			string_to_bool(value, &b);
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "device", &device);
			if (b) {
				dmuci_set_value("wireless", device, "wmm", "1");
				dmuci_set_value("wireless", device, "wmm_noack", "1");
				dmuci_set_value("wireless", device, "wmm_apsd", "1");
			}
			else {
				dmuci_set_value("wireless", device, "wmm", "0");
				dmuci_set_value("wireless", device, "wmm_noack", "");
				dmuci_set_value("wireless", device, "wmm_apsd", "");
			}
			return 0;
	}
	return 0;
}

int get_access_point_total_associations(char *refparam, struct dmctx *ctx, char **value)
{
	int i = 0;
	json_object *res;
	char *wunit, buf[8];
	dmubus_call("router", "sta", UBUS_ARGS{{"vif", cur_wifi_ssid_args.ifname}}, 1, &res);
	DM_ASSERT(res, *value = "0");
	json_object_object_foreach(res, key, val) {
		if (strstr(key, "sta-"))
			i++;
	}
	dmasprintf(value, "%d", i); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_access_point_maxassoc(char *refparam, struct dmctx *ctx, char **value)
{
	char *device;
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "device", &device);
	dmuci_get_option_value_string("wireless", device, "maxassoc", value);
	return 0;
}

int set_access_point_maxassoc(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *device;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "device", &device);
			dmuci_set_value("wireless", device, "maxassoc", value);
			return 0;
	}
	return 0;
}

int get_access_point_control_enable(char *refparam, struct dmctx *ctx, char **value)
{
	char *macfilter;
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "macfilter", &macfilter);
	if (macfilter[0] != '0')
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_access_point_control_enable(char *refparam, struct dmctx *ctx, int action, char *value)
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
				value = "2";
			else
				value = "0";
			dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "macfilter", value);
			return 0;
	}
	return 0;
}

int get_access_point_security_supported_modes(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "None, WEP-64, WEP-128, WPA-Personal, WPA2-Personal, WPA-WPA2-Personal, WPA-Enterprise, WPA2-Enterprise, WPA-WPA2-Enterprise";
	return 0;
}

int get_access_point_security_modes(char *refparam, struct dmctx *ctx, char **value)
{
	char *encryption, *cipher;

	*value = "";
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "cipher", &cipher);
	if (strcmp(encryption, "none") == 0)
		*value = "None";
	else if (strcmp(encryption, "wep-open") == 0 || strcmp(encryption, "wep-shared") == 0)
		*value = "WEP-64";
	else if (strcmp(encryption, "psk") == 0)
		*value = "WPA-Personal";
	else if (strcmp(encryption, "wpa") == 0)
		*value = "WPA-Enterprise";
	else if (strcmp(encryption, "psk2") == 0 && strcmp(cipher, "ccmp") == 0)
		*value = "WPA2-Personal";
	else if (strcmp(encryption, "wpa2") == 0)
		*value = "WPA2-Enterprise";
	else if (strcmp(encryption, "mixed-psk") == 0 && strcmp(cipher, "tkip+ccmp") == 0)
		*value = "WPA-WPA2-Personal";
	else if (strcmp(encryption, "mixed-wpa") == 0)
		*value = "WPA-WPA2-Enterprise";
	return 0;
}

int set_access_point_security_modes(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *option, *gnw;
	char strk64[4][11];
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			//dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(value, "None") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "none");
			}
			else if (strcmp(value, "WEP-64") == 0 || strcmp(value, "WEP-128") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "wep-open");
				wepkey64("Inteno", strk64);
				int i = 0;
				while (i < 4) {
					dmasprintf(&option, "key%d", i + 1);
					dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, option, strk64[i]);
					dmfree(option);
					i++;
				}
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "key", "1");
			}
			else if (strcmp(value, "WPA-Personal") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				gnw = get_nvram_wpakey();
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "psk");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "key", gnw);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "cipher", "tkip");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "gtk_rekey", "3600");
				dmfree(gnw);
			}
			else if (strcmp(value, "WPA-Enterprise") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "wpa");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_server", "");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_port", "1812");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_secret", "");
			}
			else if (strcmp(value, "WPA2-Personal") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				gnw = get_nvram_wpakey();
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "psk2");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "key", gnw);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "cipher", "ccmp");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "gtk_rekey", "3600");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "wps_pbc", "1");
				dmfree(gnw);
			}
			else if (strcmp(value, "WPA2-Enterprise") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "wpa2");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_server", "");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_port", "1812");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_secret", "");
			}
			else if (strcmp(value, "WPA-WPA2-Personal") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				gnw = get_nvram_wpakey();
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "mixed-psk");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "key", gnw);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "cipher", "tkip+ccmp");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "gtk_rekey", "3600");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "wps_pbc", "1");
				dmfree(gnw);
			}
			else if (strcmp(value, "WPA-WPA2-Enterprise") == 0) {
				reset_wlan(cur_wifi_ssid_args.wifi_ssid_sec);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", "mixed-wpa");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_server", "");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_port", "1812");
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_secret", "");
			}
			return 0;
	}
	return 0;
}

int set_access_point_security_wepkey(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *key_index, *encryption;
	char buf[8];
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(encryption, "wep-open") == 0 || strcmp(encryption, "wep-shared") == 0 ) {
				dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "key", &key_index);
				sprintf(buf,"key%s", key_index);
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, buf, value);
			}
			return 0;
	}
	return 0;
}

int set_access_point_security_shared_key(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *encryption;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(encryption, "psk") == 0 || strcmp(encryption, "psk2") == 0 || strcmp(encryption, "mixed-psk") == 0 ) {
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "key", value);
			}
			return 0;
	}
	return 0;
}

int set_access_point_security_passphrase(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *encryption;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(encryption, "psk") == 0 || strcmp(encryption, "psk2") == 0 || strcmp(encryption, "mixed-psk") == 0 ) {
				set_access_point_security_shared_key(refparam, ctx, action, value);
			}
			return 0;
	}
	return 0;
}

int get_access_point_security_rekey_interval(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "gtk_rekey", value);
	return 0;
}

int set_access_point_security_rekey_interval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *key_index, *encryption;
	char buf[8];
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(encryption, "wep-open") == 0 || strcmp(encryption, "wep-shared") == 0 || strcmp(encryption, "none") == 0)
				return 0;
			else {
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "gtk_rekey", value);
			}
			return 0;
	}
	return 0;
}

int get_access_point_security_radius_ip_address(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "radius_server", value);
	return 0;
}

int set_access_point_security_radius_ip_address(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *key_index, *encryption;
	char buf[8];
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(encryption, "wpa") == 0 || strcmp(encryption, "wpa2") == 0 || strcmp(encryption, "mixed-wpa") == 0)
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_server", value);
			return 0;
	}
	return 0;
}

int get_access_point_security_radius_server_port(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "radius_port", value);
	return 0;
}

int set_access_point_security_radius_server_port(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *key_index, *encryption;
	char buf[8];
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(encryption, "wpa") == 0 || strcmp(encryption, "wpa2") == 0 || strcmp(encryption, "mixed-wpa") == 0)
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_port", value);
			return 0;
	}
	return 0;
}

int set_access_point_security_radius_secret(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *key_index, *encryption;
	char buf[8];
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "encryption", &encryption);
			if (strcmp(encryption, "wpa") == 0 || strcmp(encryption, "wpa2") == 0 || strcmp(encryption, "mixed-wpa") == 0)
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "radius_secret", value);
			return 0;
	}
	return 0;
}

int get_radio_supported_frequency_bands(char *refparam, struct dmctx *ctx, char **value)
{
	*value = "2.4GHz, 5GHz";
	return 0;
}

////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_radio_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_radio_args.wifi_radio_sec, "radioalias", value);
	return 0;
}

int set_radio_alias(char *refparam, struct dmctx *ctx, int action, char *value)
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

int get_ssid_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "ssidalias", value);
	return 0;
}

int set_ssid_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "ssidalias", value);
			return 0;
	}
	return 0;
}
int get_access_point_alias(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_value_by_section_string(cur_wifi_ssid_args.wifi_ssid_sec, "accesspointalias", value);
	return 0;
}

int set_access_point_alias(char *refparam, struct dmctx *ctx, int action, char *value)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "accesspointalias", value);
			return 0;
	}
	return 0;
}
////////////////////////////////GET & SET LOWER LAYER//////////////////////////
int get_ssid_lower_layer(char *refparam, struct dmctx *ctx, char **value)
{
	if (cur_wifi_ssid_args.linker[0] != '\0') {
		adm_entry_get_linker_param(DMROOT"WiFi.Radio.", cur_wifi_ssid_args.linker, value); // MEM WILL BE FREED IN DMMEMCLEAN
		if (*value == NULL)
			*value = "";
	}
	return 0;
}

int set_ssid_lower_layer(char *refparam, struct dmctx *ctx, int action, char *value)
{
	char *linker;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			adm_entry_get_linker_value(value, &linker);
			if (linker) {
				dmuci_set_value_by_section(cur_wifi_ssid_args.wifi_ssid_sec, "device", linker);
				dmfree(linker);
			}
			return 0;
	}
	return 0;
}

int get_ap_ssid_ref(char *refparam, struct dmctx *ctx, char **value)
{

	adm_entry_get_linker_param(DMROOT"WiFi.SSID.", cur_wifi_ssid_args.ifname, value); // MEM WILL BE FREED IN DMMEMCLEAN
	if (*value == NULL)
		*value = "";
	return 0;
}
////////////////ADD DEL OBJ//////////////////////////
int add_wifi_ssid(struct dmctx *ctx, char **new_instance)
{
	char *value;
	char ssid[16] = {0};
	char *instance;
	struct uci_section *s = NULL;

	instance = get_last_instance("wireless", "wifi-iface", "lwlaninstance");
	sprintf(ssid, "Inteno_%d", instance ? (atoi(instance)+1) : 1);
	dmuci_add_section("wireless", "wifi-iface", &s, &value);
	dmuci_set_value_by_section(s, "device", "wl0");
	dmuci_set_value_by_section(s, "encryption", "none");
	dmuci_set_value_by_section(s, "macfilter", "0");
	dmuci_set_value_by_section(s, "mode", "ap");
	dmuci_set_value_by_section(s, "ssid", ssid);
	*new_instance = update_instance(s, instance, "lwlaninstance");
	return 0;
}

int delete_wifi_ssid_all(struct dmctx *ctx)
{
	int found = 0;
	char *lan_name;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;

	uci_foreach_sections("wireless", "wifi-iface", s) {
		if (found != 0)
				dmuci_delete_by_section(ss, NULL, NULL);
		ss = s;
		found++;
	}
	if (ss != NULL)
		dmuci_delete_by_section(ss, NULL, NULL);
	return 0;
}

int delete_wifi_ssid(struct dmctx *ctx)
{
	dmuci_delete_by_section(cur_wifi_ssid_args.wifi_ssid_sec, NULL, NULL);
	return 0;
}
/////////////SUB ENTRIES///////////////
inline int entry_wifi_radio(struct dmctx *ctx)
{
	char *wnum = NULL, *wnum_last = NULL;
	char buf[12];
	struct uci_section *s = NULL;

	uci_foreach_sections("wireless", "wifi-device", s) {
		init_wifi_radio(ctx, s);
		wnum =  handle_update_instance(1, ctx, &wnum_last, update_instance_alias, 3, s, "radioinstance", "radioalias");
		SUBENTRY(entry_wifi_radio_instance, ctx, wnum);
	}
	return 0;
}

inline int entry_wifi_ssid(struct dmctx *ctx)
{
	char *wnum = NULL, *ssid_last = NULL, *ifname, *acpt_last = NULL, *linker;
	struct uci_section *ss = NULL;
	json_object *res;

	uci_foreach_sections("wireless", "wifi-iface", ss) {
		dmuci_get_value_by_section_string(ss, "ifname", &ifname);
		dmuci_get_value_by_section_string(ss, "device", &linker);
		init_wifi_ssid(ctx, ss, ifname, linker);
		wnum =  handle_update_instance(1, ctx, &ssid_last, update_instance_alias, 3, ss, "ssidinstance", "ssidalias");
		SUBENTRY(entry_wifi_ssid_instance, ctx, wnum);
		wnum =  handle_update_instance(1, ctx, &acpt_last, update_instance_alias, 3, ss, "accesspointinstance", "accesspointalias");
		SUBENTRY(entry_wifi_access_point_instance, ctx, wnum);
	}
	return 0;
}
///////////////////WIFI ENTRY///////////////////

int entry_method_root_Wifi(struct dmctx *ctx)
{
	IF_MATCH(ctx, DMROOT"WiFi.") {
		DMOBJECT(DMROOT"WiFi.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"WiFi.Radio.", ctx, "0", 1, NULL, NULL, NULL);
		DMOBJECT(DMROOT"WiFi.SSID.", ctx, "1", 1, add_wifi_ssid, delete_wifi_ssid_all, NULL);
		DMOBJECT(DMROOT"WiFi.AccessPoint.", ctx, "0", 1, NULL, NULL, NULL);
		SUBENTRY(entry_wifi_radio, ctx);
		SUBENTRY(entry_wifi_ssid, ctx);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_wifi_radio_instance(struct dmctx *ctx, char *wnum)
{
	IF_MATCH(ctx, DMROOT"WiFi.Radio.%s.", wnum) {
		char linker[32] = "";
		strcat(linker, section_name(cur_wifi_radio_args.wifi_radio_sec));
		DMOBJECT(DMROOT"WiFi.Radio.%s.", ctx, "0", 1, NULL, NULL, linker, wnum);
		DMPARAM("Alias", ctx, "1", get_radio_alias, set_radio_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_radio_enable, set_radio_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_radio_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("MaxBitRate", ctx, "1", get_radio_max_bit_rate, set_radio_max_bit_rate, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("OperatingFrequencyBand", ctx, "0", get_radio_frequency, NULL, NULL, 0, 1, UNDEF, NULL); //TO CHECK R/W
		DMPARAM("SupportedFrequencyBands", ctx, "0", get_radio_supported_frequency_bands, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("OperatingChannelBandwidth", ctx, "1", get_radio_operating_channel_bandwidth, set_radio_operating_channel_bandwidth, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_MaxAssociations", ctx, "1", get_radio_maxassoc, set_radio_maxassoc, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("X_INTENO_SE_DFSEnable", ctx, "1", get_radio_dfsenable, set_radio_dfsenable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("SupportedStandards", ctx, "0", get_radio_supported_standard, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("OperatingStandards", ctx, "1", get_radio_operating_standard, set_radio_operating_standard, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ChannelsInUse", ctx, "0", get_radio_channel, NULL, NULL, 0, 1, UNDEF, NULL); /// TO CHECK
		DMPARAM("Channel", ctx, "1", get_radio_channel, set_radio_channel, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
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

inline int entry_wifi_ssid_instance(struct dmctx *ctx, char *wnum)
{
	IF_MATCH(ctx, DMROOT"WiFi.SSID.%s.", wnum) {
		DMOBJECT(DMROOT"WiFi.SSID.%s.", ctx, "1", 1, NULL, delete_wifi_ssid, cur_wifi_ssid_args.ifname, wnum);
		DMPARAM("Alias", ctx, "1", get_ssid_alias, set_ssid_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1", get_wifi_enable, set_wifi_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_wifi_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SSID", ctx, "1", get_wlan_ssid, set_wlan_ssid, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Name", ctx, "1", get_wlan_ssid, set_wlan_ssid, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("LowerLayers", ctx, "1", get_ssid_lower_layer, set_ssid_lower_layer, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("BSSID", ctx, "0", get_wlan_bssid, NULL, NULL, 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"WiFi.SSID.%s.Stats.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("BytesSent", ctx, "0", get_ssid_statistics_tx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("BytesReceived", ctx, "0", get_ssid_statistics_rx_bytes, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsSent", ctx, "0", get_ssid_statistics_tx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("PacketsReceived", ctx, "0", get_ssid_statistics_rx_packets, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}

inline int entry_wifi_access_point_instance(struct dmctx *ctx, char *wnum)
{
	IF_MATCH(ctx, DMROOT"WiFi.AccessPoint.%s.", wnum) {
		DMOBJECT(DMROOT"WiFi.AccessPoint.%s.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("Alias", ctx, "1",  get_access_point_alias, set_access_point_alias, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("Enable", ctx, "1",  get_wifi_enable, set_wifi_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("Status", ctx, "0", get_wifi_status, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SSIDReference", ctx, "0", get_ap_ssid_ref, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("SSIDAdvertisementEnabled", ctx, "1", get_wlan_ssid_advertisement_enable, set_wlan_ssid_advertisement_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("WMMEnable", ctx, "1", get_wmm_enabled, set_wmm_enabled, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMPARAM("AssociatedDeviceNumberOfEntries", ctx, "0", get_access_point_total_associations, NULL, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MaxAssociatedDevices", ctx, "1", get_access_point_maxassoc, set_access_point_maxassoc, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("MACAddressControlEnabled", ctx, "1", get_access_point_control_enable, set_access_point_control_enable, "xsd:boolean", 0, 1, UNDEF, NULL);
		DMOBJECT(DMROOT"WiFi.AccessPoint.%s.Security.", ctx, "0", 1, NULL, NULL, NULL, wnum);
		DMPARAM("ModesSupported", ctx, "0", get_access_point_security_supported_modes, NULL, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("ModeEnabled", ctx, "1", get_access_point_security_modes, set_access_point_security_modes, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("WEPKey", ctx, "1", get_empty, set_access_point_security_wepkey, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("PreSharedKey", ctx, "1", get_empty, set_access_point_security_shared_key, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("KeyPassphrase", ctx, "1", get_empty, set_access_point_security_passphrase, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("RekeyingInterval", ctx, "1", get_access_point_security_rekey_interval, set_access_point_security_rekey_interval, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("RadiusServerIPAddr", ctx, "1", get_access_point_security_radius_ip_address, set_access_point_security_radius_ip_address, NULL, 0, 1, UNDEF, NULL);
		DMPARAM("RadiusServerPort", ctx, "1", get_access_point_security_radius_server_port, set_access_point_security_radius_server_port, "xsd:unsignedInt", 0, 1, UNDEF, NULL);
		DMPARAM("RadiusSecret", ctx, "1", get_empty, set_access_point_security_radius_secret, NULL, 0, 1, UNDEF, NULL);
		return 0;
	}
	return FAULT_9005;
}
