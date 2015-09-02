/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 */

#include <ctype.h>
#include "dmcwmp.h"
#include "voiceservice.h"

struct codec_args codec_args = {0};
struct sip_args sip_args = {0};
struct brcm_args brcm_args = {0};
struct allow_sip_codec allowed_sip_codecs[] = {
	{"1", "ulaw"},
	{"2", "alaw"},
	{"3", "g729"},
	{"4", "g723"},
	{"5", "g726"}
};

struct cap_sip_codec capabilities_sip_codecs[] = {
	{"g723", "G.723.1", "6451", "30-300", "30"},
	{"gsm", "GSM-FR", "13312", "20-300", "20"},
	{"ulaw", "G.711MuLaw","65536", "10-150", "20"},
	{"alaw", "G.711ALaw", "65536", "10-150", "20"},
	{"g726aal2","g726aal2 ", "32768", "10-300", "20"},
	{"adpcm", "adpcm", "32768", "10-300", "20"},
	{"slin", "slin", "0", "10-70", "20"},
	{"lpc10", "lpc10", "2457", "20-20", "20"},
	{"g729", "G.729a", "8192", "10-230", "20"},
	{"speex", "speex", "49152", "10-60", "20"},
	{"ilbc", "iLBC", "8192", "30-30", "30"},
	{"g726", "G.726", "32768", "10-300", "20"},
	{"g722", "G.722", "65536", "0-0", "0"},
	{"siren7", "G.722.1", "32768", "0-0", "0"},
	{"siren14", "siren14 ", "0", "0-0", "0"},
	{"slin16", "slin16", "0", "0-0", "0"},
	{"g719", "g719", "0", "0-0", "0"},
	{"speex16", "speex16", "0", "0-0", "0"},
	{"testlaw", "testlaw", "0", "0-0", "0"}
};

struct region capabilities_regions[] = {
	{"AUS", "AU"},
	{"BEL", "BE"},
	{"BRA", "BR"},
	{"CHL", "CL"},
	{"CHN", "CN"},
	{"CZE", "CZ"},
	{"DNK", "DK"},
	{"FIN", "FI"},
	{"FRA", "FR"},
	{"DEU", "DE"},
	{"HUN", "HU"},
	{"IND", "IN"},
	{"ITA", "IT"},
	{"JPN", "JP"},
	{"NLD", "NL"},
	{"NZL", "NZ"},
	{"USA", "US"},
	{"ESP", "ES"},
	{"SWE", "SE"},
	{"CHE", "CH"},
	{"NOR", "NO"},
	{"TWN", "TW"},
	{"GBR", "GB"},
	{"ARE", "AE"},
	{"ETS", "ET"},
	{"T57", "T5"}
};

inline int init_codec_args(struct dmctx *ctx, char *cdc, char *id)
{
	struct codec_args *args = &codec_args;
	ctx->args = (void *)args;
	args->cdc = dmstrdup(cdc);
	args->id = dmstrdup(id);
	return 0;
}

int get_max_profile_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("8");
	return 0;
}

int get_max_line_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("6");
	return 0;
}

int get_max_session_per_line(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("1");
	return 0;
}

int get_max_session_count(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("6");
	return 0;
}

int get_signal_protocols(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("SIP");
	return 0;
}

int get_regions(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("AU, BE, BR, CL, CN, CZ, DK, FI, FR, DE, HU, IN, IT, JP, NL, NZ, US, ES, SE, CH, NO, TW, GB, AE, ET, T5");
	return 0;
}

int get_true_value(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("1");
	return 0;
}

int get_false_value(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("0");
	return 0;
}

int get_sip_role (char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("BackToBackUserAgents");
	return 0;
}

int get_sip_extension(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, SUBSCRIBE, NOTIFY, INFO, PUBLISH");
	return 0;
}

int get_sip_transport(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("UDP, TCP, TLS");
	return 0;
}

int get_sip_tls_auth_protocols(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("MD5");
	return 0;
}

int get_sip_tls_enc_protocols(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("RC4, RC2, DES, 3DES");
	return 0;
}

int get_sip_tls_key_protocols(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("RSA, DSS");
	return 0;
}


int get_entry_id(char *refparam, struct dmctx *ctx, char **value)
{
	struct codec_args *codecs = (struct codec_args *)(ctx->args);
	*value = dmstrdup(codecs->id);
	return 0;
}

int get_capabilities_sip_codec(char *refparam, struct dmctx *ctx, char **value)
{
	int i;
	struct codec_args *cdcargs = (struct codec_args *)(ctx->args);
	bool sep = false;
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(strcmp(capabilities_sip_codecs[i].c1, cdcargs->cdc) == 0) {
			*value = dmstrdup(capabilities_sip_codecs[i].c2);
			break;
		}
	}
	return 0;
}

int get_capabilities_sip_bitrate(char *refparam, struct dmctx *ctx, char **value)
{
	int i;
	struct codec_args *cdcargs = (struct codec_args *)(ctx->args);
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(strcmp(capabilities_sip_codecs[i].c1, cdcargs->cdc) == 0) {
			*value = dmstrdup(capabilities_sip_codecs[i].c3);
			break;
		}
	}
	return 0;
}

int get_capabilities_sip_pperiod(char *refparam, struct dmctx *ctx, char **value)
{
	int i;
	struct codec_args *cdcargs = (struct codec_args *)(ctx->args);
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(strcmp(capabilities_sip_codecs[i].c1, cdcargs->cdc) == 0) {
			*value = dmstrdup(capabilities_sip_codecs[i].c3);
			break;
		}
	}
	return 0;
}

inline int get_voice_service_capabilities_codecs_generic(struct dmctx *ctx, char *idev, char *id)
{
	TRACE();
	DMOBJECT(DMROOT"Service.VoiceService.%s.Capabilities.Codecs.%s.", ctx, "0", 0, NULL, NULL, NULL, idev, id);
	DMPARAM("EntryID", ctx, "0", get_entry_id, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("Codec", ctx, "0", get_capabilities_sip_codec, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("BitRate", ctx, "0", get_capabilities_sip_bitrate, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("PacketizationPeriod", ctx, "0", get_capabilities_sip_pperiod, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("SilenceSuppression", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
	TRACE();
}

/*Voiceprofile*/
inline int init_sip_args(struct dmctx *ctx, struct uci_section *section)
{
	struct sip_args *args = &sip_args;
	ctx->args = (void *)args;
	args->sip_section = section;
	return 0;
}

int get_voice_service_max_line()
{
	json_object *res;
	json_object *brcm;
	int num = 0;
	dmubus_call("asterisk", "status", UBUS_ARGS{}, 0, &res);
	if(res)
		json_select(res, "brcm", -1, NULL, NULL, &brcm);
	if(brcm) {
		TRACE("==>\n value in statistics is %p \n<==\n", brcm);
		json_object_object_foreach(brcm, key, val) {
			type = json_object_get_type(val);
			if (type == json_type_object)
				num++;
		}
	}
		TRACE("==>\nbrcm number%d\n<==\n", num);
	return num;
}

/*Enable*/
int get_voice_profile_enable(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "enabled", value);
	if(strcmp(*value, "0") == 0)
		*value = dmstrdup("Disabled");
	else
		*value = dmstrdup("Enabled");
	return 0;
}

int set_voice_profile_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if(strcmp(value, "Enabled") == 0)
				dmuci_set_value_by_section(sipargs->sip_section, "enabled", "1");
			else
				dmuci_set_value_by_section(sipargs->sip_section, "enabled", "0");
			//TODO:
			//delay_service restart "voice_client" "5" TODO by IBH
			//delay_service restart "asterisk" "6" TODO by IBH);
			return 0;
	}
}

/*Reset*/
int set_voice_profile_reset(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if(strcmp(value, "1") == 0) {
				return 0;
				//TODO: delay_service restart "asterisk" "6" TODO by IBH
			}
		return 0;
	}
}

/*Name*/
int get_voice_profile_name(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "name", value);
	return 0;
}

/*SignaleProtocol*/
int get_voice_profile_signalprotocol(char *refparam, struct dmctx *ctx, char **value)
{
	*value = dmstrdup("SIP");
	return 0;
}

/*MaxSessions*/
int get_voice_profile_maxsession(char *refparam, struct dmctx *ctx, char **value)
{
	//TODO: wait ubus command
	/*
	local number_line=`/usr/sbin/asterisk -rx "brcm show status"|grep "Subchannel:"|sort -u|wc -l`
	local sub_channel=`/usr/sbin/asterisk -rx "brcm show status"|grep -c "Default context     : sip$num"`
	let val=$number_line*$sub_channel
	*/
	return 0;
}

/*NumberOfLines*/
int get_voice_profile_numberlines(char *refparam, struct dmctx *ctx, char **value)
{
	//TODO: wait ubus command
	///usr/sbin/asterisk -rx "brcm show status"|grep -c "Default context     : sip$num"
	return 0;
}

/*SIP*/
int get_voice_profile_sip_proxyserver(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "sip_proxy", value);
	return 0;
}

int set_voice_profile_sip_proxyserver(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("voice_client", "SIP", "sip_proxy", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*ProxuServerPort*/
int set_voice_profile_sip_proxyserverport(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			//TODO: not implemented in old scripts
			return 0;
	}
}

/*ProxyServerTransport*/
int get_voice_profile_sip_proxyservertransport(char *refparam, struct dmctx *ctx, char **value)
{
	//TODO: wait ubus command
	///usr/sbin/asterisk -rx "sip show settings"|grep "Outbound transport"|awk -F' ' '{print $3}'
	return 0;
}

int set_voice_profile_sip_proxyserverport(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			//TODO: not implemented in old scripts
			return 0;
	}
}

/*RegisterServer*/
int get_voice_profile_sip_registerserver(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "host", value);
	return 0;
}

int set_voice_profile_sip_registerserver(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "host", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*RegistrarServerPort*/
int get_voice_profile_sip_registerserverport(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "port", value);
	return 0;
}

int set_voice_profile_sip_registerserverport(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "port", value);
			return 0;
	}
}

/*RegistrarServerTransport*/
int get_voice_profile_sip_registerservertransport(char *refparam, struct dmctx *ctx, char **value)
{
	//TODO: wait ubus command
	///usr/sbin/asterisk -rx "sip show settings"|grep "Outbound transport"|awk -F' ' '{print $3}''
	return 0;
}

int set_voice_profile_sip_registerservertransport(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			//TODO: not implemented in old scripts
			return 0;
	}
}

/*UserAgentDomain*/
int get_voice_profile_sip_useragentdomain(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "domain", value);
	return 0;
}

int set_voice_profile_sip_useragentdomain(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "domain", value);
			return 0;
	}
}

/*UserAgentPort*/
int get_voice_profile_sip_useragentport(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "bindport", value);
	return 0;
}

int set_voice_profile_sip_useragentport(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("voice_client", "SIP", "bindport", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*UserAgentTransport*/
int get_voice_profile_sip_useragenttransport(char *refparam, struct dmctx *ctx, char **value)
{
	//TODO: wait ubus command
	///usr/sbin/asterisk -rx "sip show settings"|grep "Outbound transport"|awk -F' ' '{print $3}'
	return 0;
}

int set_voice_profile_sip_useragenttransport(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			//TODO: not implemented in old scripts
			return 0;
	}
}

/*OutboundProxy*/
int get_voice_profile_sip_outboundproxy(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "outboundproxy", value);
	return 0;
}

int set_voice_profile_sip_outboundproxy(char *refparam, struct dmctx *ctx, char **value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "outboundproxy", value);
			return 0;
	}
}

/*OutboundProxyPort*/
int get_voice_profile_sip_outboundproxy(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "outboundport", value);
	return 0;
}

int set_voice_profile_sip_outboundproxy(char *refparam, struct dmctx *ctx, char **value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);	
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "outboundport", value);
			return 0;
	}
}

/*Organization*/
int set_voice_profile_sip_organization(char *refparam, struct dmctx *ctx, char **value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			return 0;
	}
}

/*RegistrationPeriod*/
int get_voice_profile_sip_registrationperiod(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "defaultexpiry", value);
	return 0;
}

int set_voice_profile_sip_registrationperiod(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("voice_client", "SIP", "defaultexpiry", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*InviteExpires*/
int set_voice_profile_sip_inviteexpires(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			return 0;
	}
}

/*reinviteExpires - RegisterExpires*/
int get_voice_profile_sip_expires(char *refparam, struct dmctx *ctx, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "registertimeout", value);
	return 0;
}

int set_voice_profile_sip_expires(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("voice_client", "SIP", "registertimeout", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*RegistersMinExpires*/
int set_voice_profile_sip_registersminexpires(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			return 0;
	}
}

/*InboundAuth*/
int set_voice_profile_sip_inbound(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			return 0;
	}
}

/*X_call*/
int get_voice_profile_sip_xcallline(char *refparam, struct dmctx *ctx, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "call_lines", value);
	return 0;
}

int set_voice_profile_sip_xcallline(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "call_lines", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*DTMFMethod*/
int get_voice_profile_sip_dtmfmethod(char *refparam, struct dmctx *ctx,  char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "dtmfmode", value);
	if(strcmp(*value, "inband") == 0)
		*value = strdup("InBand");
	else if(strcmp(*value, "rfc2833") == 0)
		*value = strdup("RFC2833");
	else if(strcmp(*value, "info") == 0)
		*value = strdup("SIPInfo");
	return 0;
}

/*Region*/
int get_voice_profile_region(char *refparam, struct dmctx *ctx,  char **value)
{
	dmuci_get_option_value_string("voice_client", "BRCM", "country", value);
	for (i = 0; i < ARRAY_SIZE(capabilities_regions); i++) {
		if(strcmp(*value, capabilities_regions[i]->country) == 0){
			*value = dmstrdup(capabilities_regions[i]->country);
			return 0;
		}
	}
	return 0;
}

/*ServiceProviderInfo*/
int get_voice_profile_provider_name(char *refparam, struct dmctx *ctx,  char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "provider_name", value);
	if(strcmp(*value, "") == 0)
		dmuci_get_value_by_section_string(sipargs->sip_section, "domain", value);
	return 0;
}

int set_voice_profile_provider_name(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "provider_name", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*FaxT38*/
int get_voice_profile_faxt38_enable(char *refparam, struct dmctx *ctx,  char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "is_fax", value);
	return 0;
}

int set_voice_profile_faxt38_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "is_fax", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*LocalPortMin*/
int get_voice_profile_rtp_localportmin(char *refparam, struct dmctx *ctx,  char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "rtpstart", value);
	if(*value[0] == '\0')
		*value = dmstrdup("5000");
	return 0;
}

int set_voice_profile_rtp_localportmin(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("voice_client", "SIP", "rtpstart", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*LocalPortMax*/
int get_voice_profile_rtp_localportmin(char *refparam, struct dmctx *ctx,  char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "rtpend", value);
	if(*value[0] == '\0')
		*value = dmstrdup("31000");
	return 0;
}

int set_voice_profile_rtp_localportmin(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("voice_client", "SIP", "rtpend", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*RTCP.Enable*/
int get_voice_profile_rtcp_enable(char *refparam, struct dmctx *ctx,  char **value)
{
	*value = dmstrdup(get_pid("asterisk"));
	if(strcmp(*value, "-1") == 0)
		*value = dmstrdup("0");
	else
		*value = dmstrdup("1");
	return 0;
}

/*TXRepeatInterval*/
int get_voice_profile_rtcp_txrepeatinterval(char *refparam, struct dmctx *ctx,  char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "rtcpinterval", value);
	if(*value[0] == '\0')
		*value = dmstrdup("5000");
	return 0;
}

int set_voice_profile_rtcp_txrepeatinterval(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value("voice_client", "SIP", "rtcpinterval", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

/*SRTP.Enable*/
int get_voice_profile_srtp_enable(char *refparam, struct dmctx *ctx,  char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "encryption", value);
	if(strcmp(*value, "yes") == 0)
		*value = dmstrdup("1");
	else
		*value = dmstrdup("0");
	return 0;
}

int set_voice_profile_srtp_enable(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			if(strcmp(value, "1") == 0 || strcmp(value, "true") == 0)
				value = dmstrdup("yes");
			else
				value = dmstrdup("");
			dmuci_set_value_by_section(sipargs->sip_section, "encryption", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}


/*Voiceprofile.Line*/
inline int init_brcm_args(struct dmctx *ctx, struct uci_section *section)
{
	struct brcm_args *args = &brcm_args;
	ctx->args = (void *)args;
	args->brcm_section = section;
	return 0;
}


int get_voice_profile_line_directorynumber(char *refparam, struct dmctx *ctx,  char **value)
{
	struct brcm_args *brcmargs = (struct brcm_args *)(ctx->args);
	dmuci_get_value_by_section_string(brcmargs->brcm_section, "extension", value);
	return 0;
}
int set_voice_profile_line_directorynumber(char *refparam, struct dmctx *ctx, int action, char *value)
{
	bool b;
	struct brcm_args *brcmargs = (struct brcm_args *)(ctx->args);
	switch (action) {
		VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		VALUESET:
			dmuci_set_value_by_section(brcmargs->brcm_section, "extension", value);
			/*TODO:
			delay_service restart "voice_client" "5"
			delay_service restart "asterisk" "6"
			*/
			return 0;
	}
}

int get_voice_profile_line_status(char *refparam, struct dmctx *ctx,  char **value)
{
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	dmuci_get_value_by_section_string(sipargs->sip_section, "sip_registry_registered", value);
	if (strcmp(*value, "yes") == 0) {
		*value = dmstrdup("Up");
	}
	else {
		dmuci_get_value_by_section_string(sipargs->sip_section, "sip_registry_request_sent", value);
		if(strcmp(*value, "yes") == 0)
			*value = dmstrdup("Registering");
		else
			*value = dmstrdup("Disabled");
	}
	return 0;
}

int get_voice_profile_line_callstate(char *refparam, struct dmctx *ctx,  char **value)
{}

int get_voice_profile_line_x_lineprofile(char *refparam, struct dmctx *ctx,  char **value)
{}
int set_voice_profile_line_x_lineprofile(char *refparam, struct dmctx *ctx, int action, char *value)
{}

int get_voice_profile_line_x_brcmline(char *refparam, struct dmctx *ctx,  char **value)
{}
int set_voice_profile_line_x_brcmline(char *refparam, struct dmctx *ctx, int action, char *value)
{}

inline int get_services_voice_service_line_generic(struct dmctx *ctx, char *idev, char *profile_num, char *line_num)
{
	int i = 0;
	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.Line.%s", ctx, "0", 1, NULL, NULL, NULL, idev, profile_num, line_num);
	DMPARAM("Enable", ctx, 1, get_voice_profile_enable, set_voice_profile_enable, "", 0, 0, UNDEF, NULL);
	DMPARAM("DirectoryNumber", ctx, 1, get_voice_profile_line_directorynumber, set_voice_profile_line_directorynumber, "", 0, 0, UNDEF, NULL);
	DMPARAM("Status", ctx, 0, get_voice_profile_line_status, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("CallState", ctx, 0, get_voice_profile_line_callstate, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("X_002207_LineProfile", ctx, 1, get_voice_profile_line_x_lineprofile, set_voice_profile_line_x_lineprofile, "", 0, 0, UNDEF, NULL);
	DMPARAM("X_002207_BRCMLine", ctx, 1, get_voice_profile_line_x_brcmline, set_voice_profile_line_x_brcmline, "", 0, 0, UNDEF, NULL);

	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.Line.%s.CallingFeatures.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num, line_num);
	DMPARAM("CallerIDName", ctx, 1, get_voice_profile_line_enable, set_voice_profile_line_enable, "", 0, 0, UNDEF, NULL);
	DMPARAM("CallWaitingEnable", ctx, 1, get_voice_profile_line_directorynumber, set_voice_profile_line_directorynumber, "", 0, 0, UNDEF, NULL);
	
	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.Line.%s.SIP.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num, line_num);
	DMPARAM("AuthUserName", ctx, 1, get_voice_profile_line_authname, set_voice_profile_line_authname, "", 0, 0, UNDEF, NULL);
	DMPARAM("AuthPassword", ctx, 1, get_voice_profile_line_authpassword, set_voice_profile_line_authpassword, "", 0, 0, UNDEF, NULL);
	DMPARAM("URI" , ctx, 1, get_voice_profile_line_uri, set_voice_profile_line_uri, "", 0, 0, UNDEF, NULL);
	
	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.Line.%s.codec.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num, line_num);
	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.Line.%s.codec.List.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num, line_num);
	
	/*local codec cn=0
	codec_priority_update "$sip_id"
	*/
	for (i = 0; i < ARRAY_SIZE(allowed_sip_codecs); i++) {
		/*
		get_services_voice_service_line_codec_list_generic "$profile_num" "$sip_id" "$line_num" "$((++cn))" "$codec"
		*/
	}

}

inline int get_services_voice_service_generic (struct dmctx *ctx, char *idev, char *profile_num)
{
	int maxLine;
	char *line_num = NULL;
	struct uci_section *b_section = NULL;
	struct sip_args *sipargs = (struct sip_args *)(ctx->args);
	
	maxLine = get_voice_service_max_line();
	/*VoiceProfile.1.*/
	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.", ctx, "0", 1, NULL, NULL, NULL, idev, profile_num);
	DMPARAM("Enable", ctx, "1", get_voice_profile_enable, set_voice_profile_enable, "xsd:unsignedInt", 0, 0, UNDEF, linker);
	DMPARAM("Reset", ctx, "1", get_false_value, set_voice_profile_reset, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("Name", ctx, "0", get_voice_profile_name, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("SignalingProtocol", ctx, "1" ,get_voice_profile_signalprotocol, NULL, "", 0, 0, UNDEF, NULL);
	DMPARAM("MaxSessions", ctx, "0" ,get_voice_profile_maxsession, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("NumberOfLines", ctx, "0" ,get_voice_profile_numberlines, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	
	/*VoiceProfile.1.SIP*/
	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.SIP.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num);
	DMPARAM("ProxyServer", ctx, "1" ,get_voice_profile_sip_proxyserver, set_voice_profile_sip_proxyserver, "", 0, 0, UNDEF, NULL);
	DMPARAM("ProxyServerPort", ctx, "1" ,NULL, set_voice_profile_sip_proxyserverport, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("ProxyServerTransport", ctx, "1" ,get_voice_profile_sip_proxyservertransport, set_voice_profile_sip_proxyservertransport, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	
	DMPARAM("RegistrarServer", ctx, "1" ,get_voice_profile_sip_registerserver, set_voice_profile_sip_registerserver, "", 0, 0, UNDEF, NULL);
	DMPARAM("RegistrarServerPort", ctx, "1" ,get_voice_profile_sip_registerserverport, set_voice_profile_sip_registerserverport, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("RegistrarServerTransport", ctx, "1" ,get_voice_profile_sip_registerservertransport, set_voice_profile_sip_registerservertransport, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	
	DMPARAM("UserAgentDomain", ctx, "1", get_voice_profile_sip_useragentdomain, set_voice_profile_sip_useragentdomain, "", 0, 0, UNDEF, NULL);
	DMPARAM("UserAgentPort", ctx, "1", get_voice_profile_sip_useragentport, set_voice_profile_sip_useragentport, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("UserAgentTransport", ctx, "1", get_voice_profile_sip_useragenttransport, set_voice_profile_sip_useragenttransport, "", 0, 0, UNDEF, NULL);
	
	DMPARAM("OutboundProxy", ctx, "1", get_voice_profile_sip_outboundproxy, set_voice_profile_sip_outboundproxy, "", 0, 0, UNDEF, NULL);
	DMPARAM("OutboundProxyPort", ctx, "1", get_voice_profile_sip_outboundproxyport, set_voice_profile_sip_outboundproxyport, "xsd:unsignedInt", 0, 0, UNDEF, NULL);

	DMPARAM("Organization", ctx, "1", NULL, set_voice_profile_sip_organization, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("RegistrationPeriod", ctx, "1", get_voice_profile_sip_registrationperiod, set_voice_profile_sip_registrationperiod, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	
	DMPARAM("InviteExpires", ctx, "1", NULL, set_voice_profile_sip_inviteexpires, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("ReInviteExpires", ctx, "1", get_voice_profile_sip_reinviteexpires, set_voice_profile_sip_reinviteexpires, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	
	DMPARAM("RegisterExpires", ctx, "1", get_voice_profile_sip_expires, set_voice_profile_sip_expires, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("RegistersMinExpires", ctx, "1", NULL, set_voice_profile_sip_registersminexpires, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("RegisterRetryInterval", ctx, "1",get_voice_profile_sip_expires, set_voice_profile_sip_expires, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	
	DMPARAM("InboundAuth", ctx, 1, NULL, set_voice_profile_sip_inbound, "", 0, 0, UNDEF, NULL);
	DMPARAM("InboundAuthUsername", ctx, 1, NULL, set_voice_profile_sip_inbound, "", 0, 0, UNDEF, NULL);
	DMPARAM("InboundAuthPassword", ctx, 1, NULL, set_voice_profile_sip_inbound, "", 0, 0, UNDEF, NULL);
	
	DMPARAM("X_002207_CallLines", ctx, 1, get_voice_profile_sip_xcallline, set_voice_profile_sip_xcalllines, "", 0, 0, UNDEF, NULL);
	DMPARAM("DTMFMethod", ctx, 1, get_voice_profile_sip_dtmfmethod, set_voice_profile_sip_dtmfmethod, "", 0, 0, UNDEF, NULL);
	DMPARAM("Region", ctx, 1, get_voice_profile_sip_region, set_voice_profile_sip_region, "", 0, 0, UNDEF, NULL);

	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.ServiceProviderInfo.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num);
	DMPARAM("Name", ctx, 0, get_voice_profile_provider_name, set_voice_profile_provider_name, "", 0, 0, UNDEF, NULL);

	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.FaxT38.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num);
	DMPARAM("Enable", ctx, 1, get_voice_profile_faxt38_enable, set_voice_profile_faxt38_enable, "xsd:boolean", 0, 0, UNDEF, NULL);

	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.RTP.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num);
	DMPARAM("LocalPortMin", ctx, 1, get_voice_profile_rtp_localportmin, set_voice_profile_rtp_localportmin, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("LocalPortMax", ctx, 1, get_voice_profile_rtp_localportmax, set_voice_profile_rtp_localportmax, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
	DMPARAM("DSCPMark", ctx, 1, get_voice_profile_rtp_dscpmark, set_voice_profile_rtp_dscpmark, "xsd:unsignedInt", 0, 0, UNDEF, NULL);

	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.RTP.RTCP.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num);
	DMPARAM("Enable", ctx, 0, get_voice_profile_rtcp_enable, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
	DMPARAM("TxRepeatInterval", ctx, 1, get_voice_profile_rtcp_txrepeatinterval, set_voice_profile_rtcp_txrepeatinterval, "xsd:unsignedInt", 0, 0, UNDEF, NULL);

	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.RTP.SRTP.", ctx, "0", 0, NULL, NULL, NULL, idev, profile_num);
	DMPARAM("Enable", ctx, 1, get_voice_profile_srtp_enable, set_voice_profile_srtp_enable, "xsd:boolean", 0, 0, UNDEF, NULL);

	DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.%s.Line.", ctx, "0", 1, NULL, NULL, NULL, idev, profile_num);
	line_num = 0;
	uci_foreach_option_eq("voice_client", "brcm_line", "sip_account", section_name(sipargs->sip_section), b_section) {
		if (brcm_section){
			line_num = update_instance(brcm_section, cur_line_num, "lineinstance");
			if ( atoi(line_num) > max_line )
				break;
			init_brcm_args(ctx, section_name(brcm_section));
			SUBENTRY(get_services_voice_service_line_generic, ctx, idev, profile_num, line_num);
			dmfree(cur_line_num);
			cur_line_num = dmstrdup(line_num);
		}
	}
}

int entry_method_root_Service(struct dmctx *ctx)
{
	struct uci_section *sip_section = NULL;
	char *idev = NULL;
	int i = 0;
	char *profile_num = NULL;
	idev = dmstrdup("1");
	IF_MATCH(ctx, DMROOT"Service.") {
		DMOBJECT(DMROOT"Service.", ctx, "0", 0, NULL, NULL, NULL);
		DMOBJECT(DMROOT"Service.VoiceService.%s.", ctx, "0", 0, NULL, NULL, NULL, idev);
		/* Capabilities. */
		DMOBJECT(DMROOT"Service.VoiceService.%s.Capabilities.", ctx, "0", 0, NULL, NULL, NULL, idev);
		DMPARAM("MaxProfileCount", ctx, "0", get_max_profile_count, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("MaxLineCount", ctx, "0", get_max_line_count, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("MaxSessionsPerLine", ctx, "0", get_true_value, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("MaxSessionsCount", ctx, "0", get_max_session_count, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("SignalingProtocols", ctx, "0", get_signal_protocols, NULL, "", 0, 0, UNDEF, NULL);
		DMPARAM("Regions", ctx, "0", get_regions, NULL, "xsd:unsignedInt", 0, 0, UNDEF, NULL);
		DMPARAM("RTCP", ctx, "0", get_true_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("SRTP", ctx, "0", get_true_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("RTPRedundancy", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("PSTNSoftSwitchOver", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("FaxT38", ctx, "0", get_true_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("FaxPassThrough", ctx, "0", get_true_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ModemPassThrough", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ToneGeneration", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ToneDescriptionsEditable", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("PatternBasedToneGeneration", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("FileBasedToneGeneration", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ToneFileFormats", ctx, "0", get_empty, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		TRACE();
		DMPARAM("RingGeneration", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("RingDescriptionsEditable", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("PatternBasedRingGeneration", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("RingPatternEditable", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("FileBasedRingGeneration", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("RingFileFormats", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("DigitMap", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("NumberingPlan", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ButtonMap", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("VoicePortTests", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		/* Capabilities.SIP. */
		DMOBJECT(DMROOT"Service.VoiceService.%s.Capabilities.SIP.", ctx, "0", 0, NULL, NULL, NULL, idev);
		DMPARAM("Role", ctx, "0", get_sip_role, NULL, "", 0, 0, UNDEF, NULL);
		DMPARAM("Extensions", ctx, "0", get_sip_extension, NULL, "", 0, 0, UNDEF, NULL);
		DMPARAM("Transports", ctx, "0", get_sip_transport, NULL, "", 0, 0, UNDEF, NULL);
		TRACE();
		DMPARAM("URISchemes", ctx, "0", get_empty, NULL, "", 0, 0, UNDEF, NULL);
		DMPARAM("EventSubscription", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("ResponseMap", ctx, "0", get_false_value, NULL, "xsd:boolean", 0, 0, UNDEF, NULL);
		DMPARAM("TLSAuthenticationProtocols", ctx, "0", get_sip_tls_auth_protocols, NULL, "", 0, 0, UNDEF, NULL);
		DMPARAM("TLSEncryptionProtocols", ctx, "0", get_sip_tls_enc_protocols, NULL, "", 0, 0, UNDEF, NULL);
		DMPARAM("TLSKeyExchangeProtocols", ctx, "0", get_sip_tls_key_protocols, NULL, "", 0, 0, UNDEF, NULL);
		TRACE();
		/*Capabilities.Codecs.*/
		DMOBJECT(DMROOT"Service.VoiceService.%s.Capabilities.Codecs.", ctx, "0", 0, NULL, NULL, NULL, idev);
		//TRACE();
		for (i = 0; i < ARRAY_SIZE(allowed_sip_codecs); i++) {
			init_codec_args(ctx, allowed_sip_codecs[i].allowed_cdc, allowed_sip_codecs[i].id);
			SUBENTRY(get_voice_service_capabilities_codecs_generic, ctx, idev, allowed_sip_codecs[i].id);
		}
		/*Voiceprofile*/
		DMOBJECT(DMROOT"Service.VoiceService.%s.VoiceProfile.", ctx, "1", 0, NULL, NULL, NULL, idev);
		profile_num = 0;
		uci_foreach_sections("voice_client", "sip_service_provider", s_section) {
			if (s_section != NULL) {
				profile_num = update_instance(sip_section, cur_profile_num, "profileinstance");
				init_sip_args(ctx, sip_section);
				SUBENTRY(get_services_voice_service_generic, ctx, idev, profile_num);
				dmfree(cur_profile_num);
				cur_profile_num = dmstrdup(profile_num);
			}
		}
		return 0;
	}
	return FAULT_9005;
}