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
#include <uci.h>
#include <unistd.h>
#include "dmcwmp.h"
#include "dmuci.h"
#include "dmubus.h"
#include "dmcommon.h"
#include "voice_services.h"

#define MAX_ALLOWED_SIP_CODECS 20
int available_sip_codecs = 0;
struct allow_sip_codec allowed_sip_codecs[MAX_ALLOWED_SIP_CODECS];
char *codec_option_array[5] = {"codec0", "codec1", "codec2", "codec3", "codec4"};
struct cap_sip_codec capabilities_sip_codecs[] = {
	{SIP_CODEC_G723, "g723", "G.723.1", "6451", "30-300", "30"},
	{SIP_CODEC_GSM, "gsm", "GSM-FR", "13312", "20-300", "20"},
	{SIP_CODEC_ULAW, "ulaw", "G.711MuLaw","65536", "10-150", "20"},
	{SIP_CODEC_ALAW, "alaw", "G.711ALaw", "65536", "10-150", "20"},
	{SIP_CODEC_G726AAL2, "g726aal2","g726aal2 ", "32768", "10-300", "20"},
	{SIP_CODEC_ADPCM, "adpcm", "adpcm", "32768", "10-300", "20"},
	{SIP_CODEC_SLIN, "slin", "slin", "0", "10-70", "20"},
	{SIP_CODEC_LPC10, "lpc10", "lpc10", "2457", "20-20", "20"},
	{SIP_CODEC_G729, "g729", "G.729a", "8192", "10-230", "20"},
	{SIP_CODEC_SPEEX, "speex", "speex", "49152", "10-60", "20"},
	{SIP_CODEC_ILBC, "ilbc", "iLBC", "8192", "30-30", "30"},
	{SIP_CODEC_G726, "g726", "G.726", "32768", "10-300", "20"},
	{SIP_CODEC_G722, "g722", "G.722", "65536", "0-0", "0"},
	{SIP_CODEC_SIREN7, "siren7", "G.722.1", "32768", "0-0", "0"},
	{SIP_CODEC_SIREN14, "siren14", "siren14 ", "0", "0-0", "0"},
	{SIP_CODEC_SLIN16, "slin16", "slin16", "0", "0-0", "0"},
	{SIP_CODEC_G719, "g719", "g719", "0", "0-0", "0"},
	{SIP_CODEC_SPEEX16, "speex16", "speex16", "0", "0-0", "0"},
	{SIP_CODEC_TESTLAW, "testlaw", "testlaw", "0", "0-0", "0"}
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
struct rtp_tos list_rtp_tos[] = {
	{"CS0", "0"},
	{"CS1", "32"},
	{"AF11", "40"},
	{"AF12", "48"},
	{"AF13", "56"},
	{"CS2", "64"},
	{"AF21", "72"},
	{"AF22", "80"},
	{"AF23", "88"},
	{"CS3", "96"},
	{"AF31", "104"},
	{"AF32", "112"},
	{"AF33", "120"},
	{"CS4", "128"},
	{"AF41", "136"},
	{"AF42", "144"},
	{"AF43", "152"},
	{"CS5", "160"},
	{"EF", "184"},
	{"CS6", "192"},
	{"CS7", "224"}
};

int browseVoiceServiceInst(struct dmctx *ctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseCodecsInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseProfileInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseLineInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
int browseLineCodecListInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);

///////////////////////////////INIT ARGS//////////////////
void wait_voice_service_up(void)
{
	json_object *res;
	int i = 0;
	while (i++ < 10) {
		dmubus_call("asterisk", "status", UBUS_ARGS{}, 0, &res);
		if (res)
			return;
	}
}

static inline int init_allowed_sip_codecs()
{
	json_object *res = NULL;
	char id[8], priority[24], ptime[24];
	int i;
	available_sip_codecs = 0;
	dmubus_call("asterisk", "codecs", UBUS_ARGS{}, 0, &res);
	if(res) {
		json_object_object_foreach(res, key, val) {
			for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
				if(strcmp(capabilities_sip_codecs[i].c1, key) == 0) {
					allowed_sip_codecs[available_sip_codecs].enumid = capabilities_sip_codecs[i].enumid;
					break;
				}
			}
			sprintf(id, "%d", available_sip_codecs + 1);
			sprintf(priority, "priority_%s", key);
			sprintf(ptime, "ptime_%s", key);
			allowed_sip_codecs[available_sip_codecs].id = dmstrdup(id);
			allowed_sip_codecs[available_sip_codecs].allowed_cdc = key;
			allowed_sip_codecs[available_sip_codecs].priority_cdc = dmstrdup(priority);
			allowed_sip_codecs[available_sip_codecs].ptime_cdc = dmstrdup(ptime);
			available_sip_codecs++;
		}
	}	
	return 0;
}

int init_sip_args(struct sip_args *args, struct uci_section *section, char *profile_num)
{
	args->sip_section = section;
	args->profile_num = profile_num;
	return 0;
}

int init_codec_args(struct codec_args *args, char *cdc, char *id, int enumid,  struct uci_section *s)
{
	args->cdc = dmstrdup(cdc);
	args->id = dmstrdup(id);
	args->enumid = enumid;
	args->codec_section = s;
	return 0;
}

int fini_codec_args(struct codec_args *args)
{
	dmfree(args->cdc);
	dmfree(args->id);
	return 0;
}

int init_line_code_args(struct line_codec_args *args, int i, struct uci_section *s, struct uci_section *codec_sec)
{
	args->cdc = allowed_sip_codecs[i].allowed_cdc;
	args->id = allowed_sip_codecs[i].id;
	args->sip_section = s;
	args->priority_cdc = allowed_sip_codecs[i].priority_cdc;
	args->enumid = allowed_sip_codecs[i].enumid;
	args->ptime_cdc = allowed_sip_codecs[i].ptime_cdc;
	args->codec_sec = codec_sec;
	return 0;
}

int init_brcm_args(struct brcm_args *args, struct uci_section *section, struct uci_section *section2, char *instance)
{
	args->brcm_section = section;
	args->sip_section = section2;
	args->profile_num = instance;
	return 0;
}

/**************************ADD/DEL OBJECT *********************************/
int get_cfg_sipidx(void)
{
	char *si;
	int idx = 0, max = -1;
	struct uci_section *s = NULL;

	uci_foreach_sections("voice_client", "sip_service_provider", s) {
		si = section_name(s) + sizeof("sip") - 1;
		idx = atoi(si);
		if (idx > max)
			max = idx;
	}
	return (max + 1);
}

int add_profile_object(char *refparam, struct dmctx *ctx, void *data, char **instancepara)
{
	char sname[8];
	char account[16];
	char bufinst[4];
	int sipidx;
	char *add_value, *instance, *max_instance;	
	struct uci_section *voice_profile_section;
	
	sipidx = get_cfg_sipidx();
	sprintf(sname, "sip%d", sipidx);
	sprintf(account, "Account %d", sipidx);
	dmuci_set_value("voice_client", sname, NULL, "sip_service_provider");
	dmuci_set_value("voice_client", sname, "name", account);
	dmuci_set_value("voice_client", sname, "enabled", "0");
	dmuci_set_value("voice_client", sname, "codec0", "ulaw");
	dmuci_set_value("voice_client", sname, "codec1", "alaw");
	dmuci_set_value("voice_client", sname, "codec2", "g729");
	dmuci_set_value("voice_client", sname, "codec3", "g726");
	dmuci_set_value("voice_client", sname, "cfim_on", "*21*");
	dmuci_set_value("voice_client", sname, "cfim_off", "#21#");
	dmuci_set_value("voice_client", sname, "cfbs_on", "*61*");
	dmuci_set_value("voice_client", sname, "cfbs_off", "#61#");
	dmuci_set_value("voice_client", sname, "call_return", "*69");
	dmuci_set_value("voice_client", sname, "redial", "*66");
	dmuci_set_value("voice_client", sname, "cbbs_key", "5");
	dmuci_set_value("voice_client", sname, "cbbs_maxretry", "5");
	dmuci_set_value("voice_client", sname, "cbbs_retrytime", "300");
	dmuci_set_value("voice_client", sname, "cbbs_waittime", "30");
	*instancepara = get_last_instance("voice_client", "sip_service_provider", "profileinstance");
	return 0;
}

int delete_associated_line_instances(char *sip_id)
{
	struct uci_section *s;

	uci_foreach_option_eq("voice_client", "brcm_line", "sip_account", sip_id, s) {
		dmuci_set_value_by_section(s, "sip_account", "-");
		dmuci_set_value_by_section(s, "lineinstance", "");
		dmuci_set_value_by_section(s, "linealias", "");
	}
	return 0;
}

int delete_profile_object(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	int found = 0;
	struct uci_section *s, *ss = NULL;
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (del_action) {
		case DEL_INST:
			delete_associated_line_instances(section_name(sipargs->sip_section));
			dmuci_delete_by_section(sipargs->sip_section, NULL, NULL);
			break;
		case DEL_ALL:
			uci_foreach_sections("voice_client", "sip_service_provider", s) {
				if (found != 0) {
					delete_associated_line_instances(section_name(ss));
					dmuci_delete_by_section(ss, NULL, NULL);
				}
				ss = s;
				found++;
			}
			if (ss != NULL) {
				delete_associated_line_instances(section_name(ss));
				dmuci_delete_by_section(ss, NULL, NULL);
			}
			break;
	}
	
	return 0;
}

/*********/
int get_line_max_instance(struct uci_section **brcm_section)
{
	struct uci_section *s;
	int line_number, i=0;
	json_object *res;
	char *value;
	
	line_number = get_voice_service_max_line();
	line_number--;
	
	uci_foreach_sections("voice_client", "brcm_line", s) {
		i++;
		dmuci_get_value_by_section_string(s, "sip_account", &value);
		if (strcmp(value, "-") == 0)
			break;
		else if (i > line_number) {
			i = 0;
			break;
		}
	}
	if (i != 0)
		*brcm_section = s;
	else 
		*brcm_section = NULL;
	return i;
}

char *update_vp_line_instance(struct uci_section *brcm_s, char *sipx)
{
	struct uci_section *s = NULL;
	int last_instance = 0, i_instance;
	char *instance, buf[8];

	dmuci_get_value_by_section_string(brcm_s, "lineinstance", &instance);
	if(instance[0] != '\0')
		return instance;
	uci_foreach_option_eq("voice_client", "brcm_line", "sip_account", sipx, s) {
		dmuci_get_value_by_section_string(s, "lineinstance", &instance);
		if (instance[0] != '\0') {
			i_instance = atoi(instance);
			if ( i_instance > last_instance)
				last_instance = i_instance;
		}
	}
	sprintf(buf, "%d", last_instance + 1);
	instance = dmuci_set_value_by_section(brcm_s, "lineinstance", buf);
	return instance;
}

char *update_vp_line_instance_alias(int action, char **last_inst, void *argv[])
{
	struct uci_section *s = NULL;
	int last_instance = 0, i_instance;
	char *instance, *alias, buf[64];

	struct uci_section *brcm_s = (struct uci_section *) argv[0];
	char *sipx = (char *) argv[1];

	dmuci_get_value_by_section_string(brcm_s, "lineinstance", &instance);
	if (instance[0] == '\0') {
		uci_foreach_option_eq("voice_client", "brcm_line", "sip_account", sipx, s) {
			dmuci_get_value_by_section_string(s, "lineinstance", &instance);
			if (instance[0] != '\0') {
				i_instance = atoi(instance);
				if ( i_instance > last_instance)
					last_instance = i_instance;
			}
		}
		sprintf(buf, "%d", last_instance + 1);
		instance = dmuci_set_value_by_section(brcm_s, "lineinstance", buf);
	}
	*last_inst = instance;
	if (action == INSTANCE_MODE_ALIAS) {
		dmuci_get_value_by_section_string(brcm_s, "linealias", &alias);
		if (alias[0] == '\0') {
			sprintf(buf, "cpe-%s", instance);
			alias = dmuci_set_value_by_section(brcm_s, "linealias", buf);
		}
		sprintf(buf, "[%s]", alias);
		instance = dmstrdup(buf);
	}
	return instance;
}
/*******/

int add_line(struct uci_section *s, char *s_name)
{
	dmuci_set_value_by_section(s, "sip_account", s_name);
	return 0;
}

int add_line_object(char *refparam, struct dmctx *ctx, void *data, char **instancepara)
{	
	int i;
	char *value;
	char instance[4];
	char call_lines[16] = {0};
	struct uci_section *s = NULL;
	struct sip_args *sipargs = (struct sip_args *)data;
	int last_instance;
	i = get_line_max_instance(&s);
	if (i == 0)
		return FAULT_9004;
	add_line(s, section_name(sipargs->sip_section));
	*instancepara = update_vp_line_instance(s, section_name(sipargs->sip_section)); //TODO: To Check
	dmuci_get_value_by_section_string(sipargs->sip_section, "call_lines", &value);
	if (value[0] == '\0') {
		sprintf(call_lines, "%d", i - 1);
	}
	else {
		sprintf(call_lines, "%s %d", value, i - 1);
	}
	dmuci_set_value_by_section(sipargs->sip_section, "call_lines", call_lines);
	return 0;
}

int delete_line(struct uci_section *line_section, struct uci_section *sip_section)
{
	int len, found =0;
	char *section_name, *line_id, *value = NULL;
	char *pch, *spch, *call_lines, *p, new_call_lines[34] = {0};
	
	section_name = section_name(line_section);
	line_id = section_name + sizeof("brcm") - 1;
	dmuci_set_value_by_section(line_section, "sip_account", "-");
	dmuci_set_value_by_section(line_section, "lineinstance", "");
	dmuci_set_value_by_section(line_section, "linealias", "");
	dmuci_get_value_by_section_string(sip_section, "call_lines", &value);
	call_lines = dmstrdup(value);
	pch = strtok_r(call_lines, " ", &spch);
	p = new_call_lines;
	while (pch != NULL) {
		if (strcmp(pch, line_id) != 0) {
			if (new_call_lines[0] == '\0') {
				dmstrappendstr(p, pch);
			}
			else {
				dmstrappendchr(p, ' ');
				dmstrappendstr(p, pch);
			}
		}
		pch = strtok_r(NULL, " ", &spch);
	}
	dmstrappendend(p);
	dmuci_set_value_by_section(sip_section, "call_lines", new_call_lines);
	return 0;
}

int delete_line_object(char *refparam, struct dmctx *ctx, void *data, char *instance, unsigned char del_action)
{
	int found = 0;
	char *s_name;
	struct uci_section *s;
	struct sip_args *sipargs;
	struct brcm_args *bargs; //profile_num must be added to brcm_args
	
	switch (del_action) {
		case DEL_INST:
			bargs = (struct brcm_args *)data;
			delete_line(bargs->brcm_section, bargs->sip_section);
			break;
		case DEL_ALL:
			sipargs = (struct sip_args *)data;
			s_name = section_name(sipargs->sip_section);
			uci_foreach_option_eq("voice_client", "brcm_line", "sip_account", s_name, s) {
				delete_line(s, sipargs->sip_section);
			}
			break;
	}

	return 0;
}
/**************************Function for root entry *************************/
int get_max_profile_count(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "8";
	return 0;
}

int get_max_line_count(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "6";
	return 0;
}

int get_max_session_per_line(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "1";
	return 0;
}

int get_max_session_count(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "6";
	return 0;
}

int get_signal_protocols(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "SIP";
	return 0;
}

int get_regions(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "AU, BE, BR, CL, CN, CZ, DK, FI, FR, DE, HU, IN, IT, JP, NL, NZ, US, ES, SE, CH, NO, TW, GB, AE, ET, T5";
	return 0;
}

int get_true_value(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "1";
	return 0;
}

int get_false_value(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "0";
	return 0;
}
/*******************end root ***************************/

/**************SIP CAPABILITIES ************************/
int get_sip_role (char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "BackToBackUserAgents";
	return 0;
}

int get_sip_extension(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, SUBSCRIBE, NOTIFY, INFO, PUBLISH";
	return 0;
}

int get_sip_transport(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "UDP, TCP, TLS";
	return 0;
}

int get_sip_tls_auth_protocols(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "MD5";
	return 0;
}

int get_sip_tls_enc_protocols(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "RC4, RC2, DES, 3DES";
	return 0;
}

int get_sip_tls_key_protocols(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "RSA, DSS";
	return 0;
}
/*******************Capabilities.Codecs.***********************************/
int get_entry_id(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct codec_args *codecs = (struct codec_args *)data;
	*value = codecs->id;
	return 0;
}

int get_capabilities_sip_codec(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	struct codec_args *cdcargs = (struct codec_args *)data;
	bool sep = false;
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(capabilities_sip_codecs[i].enumid == cdcargs->enumid) {
			*value = capabilities_sip_codecs[i].c2;
			break;
		}
	}
	return 0;
}

int get_capabilities_sip_bitrate(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	struct codec_args *cdcargs = (struct codec_args *)data;
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(capabilities_sip_codecs[i].enumid == cdcargs->enumid) {
			*value = capabilities_sip_codecs[i].c3;
			break;
		}
	}
	return 0;
}

int get_capabilities_sip_pperiod(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	struct codec_args *cdcargs = (struct codec_args *)data;
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(capabilities_sip_codecs[i].enumid == cdcargs->enumid) {
			*value = capabilities_sip_codecs[i].c4;
			break;
		}
	}
	return 0;
}

/*******************Voiceprofile END **********************************/

int get_voice_service_max_line()
{
	int num = 0;
	json_object *res;
	json_object *brcm = NULL;
	
  //dmubus_call("asterisk.brcm", "dump", UBUS_ARGS{}, 0, &res);
	dmubus_call("asterisk", "status", UBUS_ARGS{}, 0, &res);
	if(res)
		json_select(res, "brcm", -1, NULL, NULL, &brcm);
	if(brcm) {
		json_object_object_foreach(brcm, key, val) {
			if (strstr(key, "brcm"))
				num++;
		}
	}
	return num;
}

int get_voice_profile_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *tmp;
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "enabled", &tmp);
	
	if(strcmp(tmp, "0") == 0)
		*value = "Disabled";
	else
		*value = "Enabled";
	return 0;
}

int set_voice_profile_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if(strcmp(value, "Enabled") == 0)
				dmuci_set_value_by_section(sipargs->sip_section, "enabled", "1");
			else
				dmuci_set_value_by_section(sipargs->sip_section, "enabled", "0");
			return 0;
	}
	return 0;
}

int set_voice_profile_reset(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b) {
				dmubus_call_set("uci", "commit", UBUS_ARGS{{"config", "voice_client"}}, 1);
				return 0;
			}
			return 0;
	}
	return 0;
}

int get_voice_profile_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	dmuci_get_value_by_section_string(sipargs->sip_section, "name", value);
	return 0;
}

int get_voice_profile_signalprotocol(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	*value = "SIP";
	return 0;
}

int set_voice_profile_signaling_protocol(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			return 0;
	}
	return 0;
}
int get_voice_profile_max_sessions(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	json_object *res;
	char *sub_channel = NULL, *num_lines = NULL;
	dmubus_call("asterisk.brcm", "dump", UBUS_ARGS{}, 0, &res);
	DM_ASSERT(res, *value = "");
	json_select(res, "num_subchannels", -1, NULL, &sub_channel, NULL);
	json_select(res, "num_lines", -1, NULL, &num_lines, NULL);
	dmasprintf(value, "%d", atoi(sub_channel) * atoi(num_lines)); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_voice_profile_number_of_lines(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int num = 0;
	json_object *res, *jobj;
	struct uci_section *b_section = NULL;
	struct sip_args *sipargs = (struct sip_args *)data;

	*value = "0";
	dmubus_call("asterisk", "status", UBUS_ARGS{}, 0, &res);
	if (!res)
		return 0;
	uci_foreach_option_eq("voice_client", "brcm_line", "sip_account", section_name(sipargs->sip_section), b_section) {
		json_select(res, "brcm", -1, section_name(b_section), NULL, &jobj);
		if (jobj)
			num++;
	}
	dmasprintf(value, "%d", num); // MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int get_voice_profile_sip_proxyserver(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "sip_proxy", value);
	return 0;
}

int set_voice_profile_sip_proxyserver(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("voice_client", "SIP", "sip_proxy", value);
			return 0;
	}
	return 0;
}

int set_sip_proxy_server_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			//TODO: not implemented in old scripts
			return 0;
	}
	return 0;
}

int get_sip_proxy_server_transport(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
		
	dmuci_get_value_by_section_string(sipargs->sip_section, "transport", value);
	return 0;
}

int set_sip_proxy_server_transport(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "transport", value);
			return 0;
	}
	return 0;
}

int get_voice_profile_sip_registerserver(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "host", value);
	return 0;
}

int set_voice_profile_sip_registerserver(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "host", value);
			return 0;
	}
	return 0;
}

int get_voice_profile_sip_registerserverport(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "port", value);
	return 0;
}

int set_voice_profile_sip_registerserverport(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "port", value);
			return 0;
	}
	return 0;
}

int get_sip_registrar_server_transport(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "transport", value);
	return 0;
}

int set_sip_registrar_server_transport(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "transport", value);
			return 0;
	}
	return 0;
}

int get_sip_user_agent_domain(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "domain", value);
	return 0;
}

int set_sip_user_agent_domain(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "domain", value);
			return 0;
	}
	return 0;
}

int get_sip_user_agent_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "bindport", value);
	return 0;
}

int set_sip_user_agent_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("voice_client", "SIP", "bindport", value);
			return 0;
	}
	return 0;
}

int get_sip_user_agent_transport(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	//dmubus_call("asterisk.sip", "dump", UBUS_ARGS{}, 0, &res);
	char *tmp;
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "transport", &tmp);
	if (tmp[0] == '\0')
		*value = "udp";
	else
		*value = tmp;
	return 0;
}

int set_sip_user_agent_transport(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			//TODO FUNCTION IS EMPTY IN SCRIPT
			return 0;
	}
	return 0;
}

int get_sip_outbound_proxy(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "outboundproxy", value);
	return 0;
}

int set_sip_outbound_proxy(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "outboundproxy", value);
			return 0;
	}
	return 0;
}

int get_sip_outbound_proxy_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "outboundproxyport", value);
	return 0;
}

int set_sip_outbound_proxy_port(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "outboundproxyport", value);
			return 0;
	}
	return 0;
}


int get_sip_registration_period(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "defaultexpiry", value);
	return 0;
}

int set_sip_registration_period(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("voice_client", "SIP", "defaultexpiry", value);
			return 0;
	}
	return 0;
}


int get_sip_re_invite_expires(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_option_value_string("voice_client", "SIP", "registertimeout", value);
	return 0;
}

int set_sip_re_invite_expires(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("voice_client", "SIP", "registertimeout", value);
			return 0;
	}
	return 0;
}


int get_sip_x_002207_call_lines(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "call_lines", value);
	return 0;
}

int set_sip_x_002207_call_lines(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "call_lines", value);
			return 0;
	}
}

int get_voice_profile_sip_dtmfmethod(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *tmp;
	
	dmuci_get_option_value_string("voice_client", "SIP", "dtmfmode", &tmp);
	if(strcmp(tmp, "inband") == 0)
		*value = "InBand";
	else if(strcmp(tmp, "rfc2833") == 0)
		*value = "RFC2833";
	else if(strcmp(tmp, "info") == 0)
		*value = "SIPInfo";
	else
		*value = tmp;
	return 0;
}

int set_voice_profile_sip_dtmfmethod(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if(strcmp(value, "InBand") == 0)
				dmuci_set_value("voice_client", "SIP", "dtmfmode", "inband");
			else if(strcmp(value, "RFC2833") == 0)
				dmuci_set_value("voice_client", "SIP", "dtmfmode", "rfc2833");
			else if(strcmp(value, "SIPInfo") == 0)
				dmuci_set_value("voice_client", "SIP", "dtmfmode", "info");
			return 0;
	}
	return 0;
}

int get_sip_profile_region(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	
	dmuci_get_option_value_string("voice_client", "BRCM", "country", value);
	for (i = 0; i < ARRAY_SIZE(capabilities_regions); i++) {
		if(strcmp(*value, capabilities_regions[i].country) == 0){
			*value = capabilities_regions[i].id;
			return 0;
		}
	}
	return 0;
}

int set_sip_profile_region(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	int i;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			for (i = 0; i < ARRAY_SIZE(capabilities_regions); i++) {
				if(strcasecmp(value, capabilities_regions[i].id) == 0){
					dmuci_set_value("voice_client", "BRCM", "country", capabilities_regions[i].country);
					break;
				}
			}
			return 0;
		}
	return 0;
}

int get_voice_service_serviceproviderinfo_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "provider_name", value);
	if(value[0] == '\0')
		dmuci_get_value_by_section_string(sipargs->sip_section, "domain", value);
	return 0;
}

int set_voice_service_serviceproviderinfo_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "provider_name", value);
			return 0;
	}
	return 0;
}

int get_sip_fax_t38_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	dmuci_get_value_by_section_string(sipargs->sip_section, "is_fax", value);
	return 0;
}

int set_sip_fax_t38_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if (b)
				value = "1";
			else
				value = "0";

			dmuci_set_value_by_section(sipargs->sip_section, "is_fax", value);
			return 0;
	}
	return 0;
}

int get_voice_service_vp_rtp_portmin(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *tmp; 
	
	dmuci_get_option_value_string("voice_client", "SIP", "rtpstart", &tmp);
	if(tmp[0] == '\0')
		*value = "5000";
	else
		*value = tmp;
	return 0;
}

int set_voice_service_vp_rtp_portmin(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("voice_client", "SIP", "rtpstart", value);
			return 0;
	}
	return 0;
}

int get_voice_service_vp_rtp_portmax(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *tmp;
	
	dmuci_get_option_value_string("voice_client", "SIP", "rtpend", &tmp);
	if(tmp[0] == '\0')
		*value = "31000";
	else
		*value = tmp;
	return 0;
}

int set_voice_profile_rtp_localportmax(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("voice_client", "SIP", "rtpend", value);
			return 0;
	}
	return 0;
}

int get_voice_service_vp_rtp_dscp(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	char *tmp;
	*value = "0";

	dmuci_get_option_value_string("voice_client", "SIP", "tos_audio", &tmp);
	for (i = 0; i < ARRAY_SIZE(list_rtp_tos); i++) {
		if(strcmp(tmp, list_rtp_tos[i].key) == 0){
			*value = list_rtp_tos[i].val;
			break;
		}
	}
	return 0;
}

int set_voice_service_vp_rtp_dscp(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	int i;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			for (i = 0; i < ARRAY_SIZE(list_rtp_tos); i++) {
				if(strcmp(value, list_rtp_tos[i].val) == 0){
					dmuci_set_value("voice_client", "SIP", "tos_audio", list_rtp_tos[i].key);
					break;
				}
			}
		return 0;
	}
	return 0;
}

int get_voice_service_vp_rtp_rtcp_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	pid_t pid; 
	
	pid = get_pid("asterisk");
	if(pid < 0)
		*value = "0";
	else
		*value = "1";
	return 0;
}

int get_voice_service_vp_rtp_rtcp_txrepeatinterval(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *tmp;
	
	dmuci_get_option_value_string("voice_client", "SIP", "rtcpinterval", &tmp);
	if(tmp[0] == '\0')
		*value = "5000";
	else
		*value = tmp;
	return 0;
}

int set_voice_service_vp_rtp_rtcp_txrepeatinterval(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value("voice_client", "SIP", "rtcpinterval", value);
			return 0;
	}
	return 0;
}

int get_voice_service_vp_rtp_srtp_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *tmp;
	struct sip_args *sipargs = (struct sip_args *)data;
	
	dmuci_get_value_by_section_string(sipargs->sip_section, "encryption", &tmp);
	if(strcasecmp(tmp, "yes") == 0)
		*value = "1";
	else
		*value = "0";
	return 0;
}

int set_voice_service_vp_rtp_srtp_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	struct sip_args *sipargs = (struct sip_args *)data;
	
	switch (action) {
		case VALUECHECK:
			if (string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(sipargs->sip_section, "encryption", "yes");
			else
				dmuci_set_value_by_section(sipargs->sip_section, "encryption", "");
			return 0;
	}
	return 0;
}

/*******************LINE **********************************/
int get_voice_profile_line_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *tmp;
	struct brcm_args *brcmargs = (struct brcm_args *)data;

	dmuci_get_value_by_section_string(brcmargs->sip_section, "enabled", &tmp);

	if(strcmp(tmp, "0") == 0)
		*value = "Disabled";
	else
		*value = "Enabled";
	return 0;
}

int set_voice_profile_line_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			if(strcmp(value, "Enabled") == 0)
				dmuci_set_value_by_section(brcmargs->sip_section, "enabled", "1");
			else
				dmuci_set_value_by_section(brcmargs->sip_section, "enabled", "0");
			return 0;
	}
	return 0;
}

int get_line_directory_number(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
	dmuci_get_value_by_section_string(brcmargs->brcm_section, "extension", value);
	return 0;
}

int set_line_directory_number(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(brcmargs->brcm_section, "extension", value);
			return 0;
	}
	return 0;
}

int get_voice_profile_line_status(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *status, *sip_name, *q;
		json_object *res;
		char buf[64];
		struct brcm_args *brcmargs = (struct brcm_args *)data;
		*value = "Disabled";
		sip_name = section_name(brcmargs->sip_section);
		q = buf;
		dmstrappendstr(q, "asterisk");
		dmstrappendchr(q, '.');
		dmstrappendstr(q, "sip");
		dmstrappendchr(q, '.');
		dmstrappendstr(q, section_name(brcmargs->sip_section) + 3);
		dmstrappendend(q);
		dmubus_call(buf, "status", UBUS_ARGS{}, 0, &res);
		DM_ASSERT(res, *value = "Disabled");
		if(res) {
			json_select(res, "registered", -1, NULL, &status, NULL);
			if (strcasecmp(status, "true") == 0) {
				*value = "Up";
			}
			else {
				json_select(res, "registry_request_sent", -1, NULL, &status, NULL);
				if(strcasecmp(status, "true") == 0)
					*value = "Registering";
				else
					*value = "Disabled";
			}
		}
		return 0;
}

int get_voice_profile_line_callstate(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{	
	char *tmp, *line_name;
	struct brcm_args *brcmargs = (struct brcm_args *)data;

	line_name = section_name(brcmargs->brcm_section);
	dmuci_get_varstate_string("chan_brcm", line_name, "subchannel_0", &tmp);
	if (strcmp(tmp, "ONHOOK") == 0)
		*value = "idle";
	else if (strcmp(tmp, "OFFHOOK") == 0)
		*value = "Disconnecting";
	else if (strcmp(tmp, "DIALING") == 0)
		*value = "Calling";
	else if (strcmp(tmp, "INCALL") == 0)
		*value = "InCall";
	else if (strcmp(tmp, "RINGING") == 0)
		*value = "Ringing";
	else
		*value = "";
	return 0;
}

int get_line_x_002207_line_profile(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;

	*value = brcmargs->profile_num;
	return 0;
}

int set_line_x_002207_line_profile(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	char call_lines[32];
	char *str;
	struct uci_section *sip_s;
	struct brcm_args *brcmargs = (struct brcm_args *)data;
			
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			uci_foreach_option_eq("voice_client", "sip_service_provider", "profileinstance", value, sip_s) {
				break;
			}
			if (!sip_s || strcmp(brcmargs->profile_num, value) == 0)
				return 0;

			delete_line(brcmargs->brcm_section, brcmargs->sip_section);
			str = update_vp_line_instance(brcmargs->brcm_section, section_name(sip_s));
			add_line(brcmargs->brcm_section, section_name(sip_s));

			dmuci_get_value_by_section_string(sip_s, "call_lines", &value);
			if (value[0] == '\0') {
				value = section_name(brcmargs->brcm_section) + sizeof("brcm") - 1;
				dmuci_set_value_by_section(sip_s, "call_lines", value);
			}
			else {
				str = (section_name(brcmargs->brcm_section) + sizeof("brcm") - 1);
				sprintf(call_lines, "%s %s", value, str);
				dmuci_set_value_by_section(sip_s, "call_lines", call_lines);
			}
			return 0;
	}
	return 0;
}

int get_line_x_002207_brcm_line(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *line_name;
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
	line_name = section_name(brcmargs->brcm_section);
	*value = dmstrdup(line_name + sizeof("brcm") - 1); //  MEM WILL BE FREED IN DMMEMCLEAN
	return 0;
}

int set_line_x_002207_brcm_line(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	int error;
	char bname[8], *stype = NULL, *sipaccount = NULL;
	char *lineinstance = NULL;
	struct brcm_args *brcmargs = (struct brcm_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			sprintf(bname, "brcm%s", value);
			error = dmuci_get_section_type("voice_client", bname, &stype);
			if(error){
				return 0;
			}
			dmuci_get_option_value_string("voice_client", bname, "sip_account", &sipaccount);
			dmuci_get_option_value_string("voice_client", bname, "lineinstance", &lineinstance);
			if ((sipaccount[0] != '\0' && sipaccount[0] != '-'))
				return 0;
			dmuci_get_value_by_section_string(brcmargs->brcm_section, "sip_account", &sipaccount);
			dmuci_get_value_by_section_string(brcmargs->brcm_section, "lineinstance", &lineinstance);
			dmuci_set_value_by_section(brcmargs->brcm_section, "lineinstance", "");
			dmuci_set_value_by_section(brcmargs->brcm_section, "linealias", "");
			dmuci_set_value_by_section(brcmargs->brcm_section, "sip_account", "-");
			dmuci_set_value("voice_client", bname, "sip_account", sipaccount);
			dmuci_set_value("voice_client", bname, "lineinstance", lineinstance);
			return 0;
	}
	return 0;
}

int get_line_calling_features_caller_id_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
	dmuci_get_value_by_section_string(brcmargs->sip_section, "displayname", value);
	return 0;
}

int set_line_calling_features_caller_id_name(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(brcmargs->sip_section, "displayname", value);
			return 0;
	}
	return 0;
}

int get_line_calling_features_callwaiting(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
	dmuci_get_value_by_section_string(brcmargs->brcm_section, "callwaiting", value);
	if((*value)[0] == '\0')
		*value = "0";
	return 0;
}

int set_line_calling_features_callwaiting(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
	switch (action) {
		case VALUECHECK:
			if(string_to_bool(value, &b))
				return FAULT_9007;
			return 0;
		case VALUESET:
			string_to_bool(value, &b);
			if(b)
				dmuci_set_value_by_section(brcmargs->brcm_section, "callwaiting", "1");
			else
				dmuci_set_value_by_section(brcmargs->brcm_section, "callwaiting", "");
			return 0;
	}
	return 0;
}

int get_line_sip_auth_username(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
	dmuci_get_value_by_section_string(brcmargs->sip_section, "authuser", value);
	return 0;
}

int set_line_sip_auth_username(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
  switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(brcmargs->sip_section, "authuser", value);
			return 0;
	}
	return 0;
}

int set_line_sip_auth_password(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct brcm_args *brcmargs = (struct brcm_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(brcmargs->sip_section, "secret", value);
			return 0;
	}
	return 0;
}

int get_line_sip_uri(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	char *domain = NULL, *user = NULL;
	struct brcm_args *brcmargs = (struct brcm_args *)data;

	dmuci_get_value_by_section_string(brcmargs->sip_section, "domain", &domain);
	dmuci_get_value_by_section_string(brcmargs->sip_section, "user", &user);
	if (user && domain)
		dmasprintf(value, "%s@%s", user, domain); // MEM WILL BE FREED IN DMMEMCLEAN
	else
		*value = "";
  return 0;
}

int set_line_sip_uri(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	char *pch, *spch, *str1;
	struct brcm_args *brcmargs = (struct brcm_args *)data;
	
  switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			str1 = dmstrdup(value);
			pch = strtok_r(str1, "@", &spch);
			dmuci_set_value_by_section(brcmargs->sip_section, "user", pch);
			pch = strtok_r(NULL, "@", &spch);
			dmuci_set_value_by_section(brcmargs->sip_section, "domain", pch);
			dmfree(str1);
			return 0;
	}
	return 0;
}

/******************Line codec ***************************************/

int codec_compare(const void *s1, const void *s2)
{
	struct codec *sc1 = (struct codec *)s1;
	struct codec *sc2 = (struct codec *)s2;
	if (!sc1->priority) return 1;
	if (!sc2->priority) return -1;
	return (atoi(sc1->priority) - atoi(sc2->priority));
}

void codec_priority_sort(struct uci_section *sip_section, char *new_codec)
{
	int j, k = 0, h = 0, size = ARRAY_SIZE(codec_option_array);
	char *ucodec, *coption, *poption;
	bool found;
	struct codec sipcodec[ARRAY_SIZE(codec_option_array)+1] = {0};

	for (j = 0; j < ARRAY_SIZE(codec_option_array); j++) {
		dmuci_get_value_by_section_string(sip_section, codec_option_array[j], &ucodec);
		if(ucodec[0] != '\0') {
			found = false;
			for (k = 0; k < available_sip_codecs; k++) {
				if(strcmp(ucodec, allowed_sip_codecs[k].allowed_cdc) == 0) {
					found = true;
					break;
				}
			}
			if (found) {
				sipcodec[j].cdc = allowed_sip_codecs[k].allowed_cdc;
				dmuci_get_value_by_section_string(sip_section, allowed_sip_codecs[k].priority_cdc, &(sipcodec[j].priority));
			}
			sipcodec[j].id = codec_option_array[j];
		}
		else {
			sipcodec[j].id = codec_option_array[j];
		}
	}
	if (new_codec) {
		sipcodec[size].id = "codec5";
		found = false;
		for (k = 0; k < available_sip_codecs; k++) {
			if(strcmp(new_codec, allowed_sip_codecs[k].allowed_cdc) == 0) {
				found = true;
				break;
			}
		}
		if (found) {
			sipcodec[size].cdc = allowed_sip_codecs[k].allowed_cdc;
			dmuci_get_value_by_section_string(sip_section, allowed_sip_codecs[k].priority_cdc, &(sipcodec[size].priority));
		}
	}
	qsort(sipcodec, ARRAY_SIZE(sipcodec), sizeof(struct codec), codec_compare);

	for (j = 0; j < ARRAY_SIZE(codec_option_array); j++) {
		dmuci_set_value_by_section(sip_section, codec_option_array[j], sipcodec[j].cdc ? sipcodec[j].cdc : "");
	}
}

void codec_priority_update(struct uci_section *sip_section)
{
	bool found;
	int i, j;
	char *priority = NULL;
	char *codec;
	char pid[4] = "1";

	for (i = 0; i < available_sip_codecs; i++) {
		dmuci_get_value_by_section_string(sip_section, allowed_sip_codecs[i].priority_cdc, &priority);
		if( priority[0] != '\0')
			continue;
		found = false;
		for (j = 0; j < ARRAY_SIZE(codec_option_array); j++) {
			dmuci_get_value_by_section_string(sip_section, codec_option_array[j], &codec);
			if(strcmp(codec, allowed_sip_codecs[i].allowed_cdc) == 0) {
				found = true;
				break;
			}
		}
		if (found)
			sprintf(pid, "%d", j+1);
		dmuci_set_value_by_section(sip_section, allowed_sip_codecs[i].priority_cdc, pid);
	}
	codec_priority_sort(sip_section, NULL);
}

int get_codec_entry_id(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;
	
	*value = line_codecargs->id;
	return 0;
}

int capabilities_sip_codecs_get_codec(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;
	
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(capabilities_sip_codecs[i].enumid == line_codecargs->enumid) {
			*value = capabilities_sip_codecs[i].c2;
			break;
		}
	}
	return 0;
}

int capabilities_sip_codecs_get_bitrate(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;
	
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(capabilities_sip_codecs[i].enumid == line_codecargs->enumid) {
			*value = capabilities_sip_codecs[i].c3;
			break;
		}
	}
	return 0;
}

int get_capabilities_sip_codecs_pperiod(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;
	dmuci_get_value_by_section_string(line_codecargs->sip_section, line_codecargs->ptime_cdc, value);
	if ((*value)[0] != '\0')
		return 0;
	for (i = 0; i < ARRAY_SIZE(capabilities_sip_codecs); i++) {
		if(capabilities_sip_codecs[i].enumid == line_codecargs->enumid) {
			*value = capabilities_sip_codecs[i].c5;
			break;
		}
	}
	return 0;
}

int get_line_codec_list_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	int i;
	char *val;
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;
	
	for (i =0; i < ARRAY_SIZE(codec_option_array); i++) {
		dmuci_get_value_by_section_string(line_codecargs->sip_section, codec_option_array[i], &val);
		if (strcmp(val, line_codecargs->cdc) == 0) {
			*value = "1";
			return 0;
		}
	}
	*value = "0";
	return 0;
}

int get_line_codec_list_priority(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;
	dmuci_get_value_by_section_string(line_codecargs->sip_section, line_codecargs->priority_cdc, value);
	return 0;
}

int set_line_codec_list_packetization(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(line_codecargs->sip_section, line_codecargs->ptime_cdc, value);
			return 0;
	}
	return 0;
}

int set_line_codec_list_enable(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	bool b;
	int j;
	char *codec;
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;
	int error = string_to_bool(value, &b);

	switch (action) {
		case VALUECHECK:
			if (error)
				return FAULT_9007;
			return 0;
		case VALUESET:
			if (b) {
				for (j = 0; j < ARRAY_SIZE(codec_option_array); j++) {
					dmuci_get_value_by_section_string(line_codecargs->sip_section, codec_option_array[j], &codec);
					if(strcmp(codec, line_codecargs->cdc) == 0) {
						return 0;
					}
				}
				codec_priority_sort(line_codecargs->sip_section, line_codecargs->cdc);
			}
			else {
				for (j = 0; j < ARRAY_SIZE(codec_option_array); j++) {
					dmuci_get_value_by_section_string(line_codecargs->sip_section, codec_option_array[j], &codec);
					if(strcmp(codec, line_codecargs->cdc) == 0) {
						dmuci_set_value_by_section(line_codecargs->sip_section, codec_option_array[j], "");
					}
				}
			}
			return 0;
	}
	return 0;
}

int set_line_codec_list_priority(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	int i;
	char *val;
	struct line_codec_args *line_codecargs = (struct line_codec_args *)data;

	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(line_codecargs->sip_section, line_codecargs->priority_cdc, value);
			for (i =0; i < ARRAY_SIZE(codec_option_array); i++) {
				dmuci_get_value_by_section_string(line_codecargs->sip_section, codec_option_array[i], &val);
				if (strcmp(val, line_codecargs->cdc) == 0) {
					codec_priority_sort(line_codecargs->sip_section, NULL);
					return 0;
				}
			}
			return 0;
	}
	return 0;
}

//////////////ENABLE SET////////////////
bool dm_service_enable_set(void)
{
	if( access("/etc/init.d/asterisk", F_OK ) != -1 ) {
		return true;
	} else {
		return false;
	}
}
void codec_update_id()
{
	int i = 0;
	int found = 0;
	struct uci_section *s = NULL;
	struct uci_section *ss = NULL;
	for (i = 0; i < available_sip_codecs; i++) {
		update_section_list("dmmap","codec_id", "id", 1, allowed_sip_codecs[i].id, NULL, NULL, NULL, NULL);
	}
	if(i == 0)
	{
		uci_foreach_sections("dmmap", "codec_id", s) {
			if (found != 0) {
				dmuci_delete_by_section(ss, NULL, NULL);
			}
			ss = s;
			found++;
		}
		if (ss != NULL) {
			dmuci_delete_by_section(ss, NULL, NULL);
		}
	}
}
////////////////////////SET AND GET ALIAS/////////////////////////////////
int get_service_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct uci_section *service_section = (struct uci_section *)data;
	dmuci_get_value_by_section_string(service_section, "vsalias", value);
	return 0;
}

int set_service_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct uci_section *service_section = (struct uci_section *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(service_section, "vsalias", value);
			return 0;
	}
	return 0;
}

int get_cap_codec_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct codec_args *cur_codec_args = (struct codec_args *)data;
	dmuci_get_value_by_section_string(cur_codec_args->codec_section, "codecalias", value);
	return 0;
}

int set_cap_codec_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct codec_args *cur_codec_args = (struct codec_args *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(cur_codec_args->codec_section, "codecalias", value);
			return 0;
	}
	return 0;
}

int get_voice_profile_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	dmuci_get_value_by_section_string(sipargs->sip_section, "profilealias", value);
	return 0;
}

int set_voice_profile_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct sip_args *sipargs = (struct sip_args *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(sipargs->sip_section, "profilealias", value);
			return 0;
	}
	return 0;
}

int get_line_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	struct brcm_args *brcmarg = (struct brcm_args *)data;
	dmuci_get_value_by_section_string(brcmarg->brcm_section, "linealias", value);
	return 0;
}

int set_line_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	struct brcm_args *brcmarg = (struct brcm_args *)data;
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(brcmarg->brcm_section, "linealias", value);
			return 0;
	}
	return 0;
}

int get_line_codec_list_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char **value)
{
	dmuci_get_value_by_section_string(((struct line_codec_args *)data)->codec_sec, "codecalias", value);
	return 0;
}

int set_line_codec_list_alias(char *refparam, struct dmctx *ctx, void *data, char *instance, char *value, int action)
{
	switch (action) {
		case VALUECHECK:
			return 0;
		case VALUESET:
			dmuci_set_value_by_section(((struct line_codec_args *)data)->codec_sec, "codecalias", value);
			return 0;
	}
	return 0;
}
///////////////////////////////////////

/*** DMROOT.Services. ****/
DMOBJ tServiceObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"VoiceService", &DMREAD, NULL, NULL, NULL, browseVoiceServiceInst, NULL, NULL, tVoiceServiceObj, tVoiceServiceParam, NULL},
{0}
};

/*** VoiceService.{i}. ****/
DMOBJ tVoiceServiceObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
//{"Capabilities", &DMREAD, NULL, NULL, NULL, NULL, NULL, tCapabilitiesParams, NULL},
{"Capabilities", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tCapabilitiesObj, tCapabilitiesParams, NULL},
{"VoiceProfile", &DMWRITE, add_profile_object, delete_profile_object, NULL, browseProfileInst, NULL, NULL, tProfileObj, tProfileParam, NULL},
{0}
};

DMLEAF tVoiceServiceParam[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Alias", &DMWRITE, DMT_STRING, get_service_alias, set_service_alias, NULL, NULL},
{0}
};

/*** VoiceService.{i}.Capabilities. ****/
DMOBJ tCapabilitiesObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"SIP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tSIPParams, NULL},
{"Codecs", &DMREAD, NULL, NULL, NULL, browseCodecsInst, NULL, NULL, NULL, tCodecsParams, NULL},
{0}
};

DMLEAF tCapabilitiesParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"MaxProfileCount", &DMREAD, DMT_UNINT, get_max_profile_count, NULL, NULL, NULL},
{"MaxLineCount", &DMREAD, DMT_UNINT, get_max_line_count, NULL, NULL, NULL},
{"MaxSessionsPerLine", &DMREAD, DMT_UNINT, get_true_value, NULL, NULL, NULL},
{"MaxSessionCount", &DMREAD, DMT_UNINT, get_max_session_count, NULL, NULL, NULL},
{"SignalingProtocols", &DMREAD, DMT_STRING, get_signal_protocols, NULL, NULL, NULL},
{"Regions", &DMREAD, DMT_STRING, get_regions, NULL, NULL, NULL},
{"RTCP", &DMREAD, DMT_BOOL, get_true_value, NULL, NULL, NULL},
{"SRTP", &DMREAD, DMT_BOOL, get_true_value, NULL, NULL, NULL},
{"RTPRedundancy", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"PSTNSoftSwitchOver", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"FaxT38", &DMREAD, DMT_BOOL, get_true_value, NULL, NULL, NULL},
{"FaxPassThrough", &DMREAD, DMT_BOOL, get_true_value, NULL, NULL, NULL},
{"ModemPassThrough", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"ToneGeneration", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"ToneDescriptionsEditable", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"PatternBasedToneGeneration", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"FileBasedToneGeneration", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"ToneFileFormats", &DMREAD, DMT_STRING, get_empty, NULL, NULL, NULL},
{"RingGeneration", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"RingDescriptionsEditable", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"PatternBasedRingGeneration", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"RingPatternEditable", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"FileBasedRingGeneration", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"RingFileFormats", &DMREAD, DMT_STRING, get_empty, NULL, NULL, NULL},
{"DigitMap", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"NumberingPlan", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"ButtonMap", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"VoicePortTests", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{0}
};

/*** VoiceService.{i}.Capabilities.SIP. ****/
DMLEAF tSIPParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Role", &DMREAD, DMT_STRING, get_sip_role, NULL, NULL, NULL},
{"Extensions", &DMREAD, DMT_STRING, get_sip_extension, NULL, NULL, NULL},
{"Transports", &DMREAD, DMT_STRING, get_sip_transport, NULL, NULL, NULL},
{"URISchemes", &DMREAD, DMT_STRING, get_empty, NULL, NULL, NULL},
{"EventSubscription", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"ResponseMap", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"TLSAuthenticationProtocols", &DMREAD, DMT_STRING, get_sip_tls_auth_protocols, NULL, NULL, NULL},
{"TLSEncryptionProtocols", &DMREAD, DMT_STRING, get_sip_tls_enc_protocols, NULL, NULL, NULL},
{"TLSKeyExchangeProtocols", &DMREAD, DMT_STRING, get_sip_tls_key_protocols, NULL, NULL, NULL},
{0}
};

/*** VoiceService.{i}.Capabilities.Codecs.{i}. ****/
DMLEAF tCodecsParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Alias", &DMWRITE, DMT_STRING, get_cap_codec_alias, set_cap_codec_alias, NULL, NULL},
{"EntryID", &DMREAD, DMT_UNINT, get_entry_id, NULL, NULL, NULL},
{"Codec", &DMREAD, DMT_STRING, get_capabilities_sip_codec, NULL, NULL, NULL},
{"BitRate", &DMREAD, DMT_UNINT, get_capabilities_sip_bitrate, NULL, NULL, NULL},
{"PacketizationPeriod", &DMREAD, DMT_STRING, get_capabilities_sip_pperiod, NULL, NULL, NULL},
{"SilenceSuppression", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}. ****/
DMOBJ tProfileObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"SIP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tProfileSIPParams, NULL},
{"ServiceProviderInfo", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tServiceProviderInfoParams, NULL},
{"FaxT38", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tFaxT38Params, NULL},
{"RTP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tRTPObj, tRTPParams, NULL},
{"Line", &DMWRITE, add_line_object, delete_line_object, NULL, browseLineInst, NULL, NULL, tLineObj, tLineParams, NULL},
{0}
};

DMLEAF tProfileParam[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Alias", &DMWRITE, DMT_STRING, get_voice_profile_alias, set_voice_profile_alias, NULL, NULL},
{"Enable", &DMWRITE, DMT_STRING, get_voice_profile_enable, set_voice_profile_enable, NULL, NULL},
{"Reset", &DMWRITE, DMT_BOOL, get_false_value, set_voice_profile_reset, NULL, NULL},
{"Name", &DMREAD, DMT_STRING, get_voice_profile_name, NULL, NULL, NULL},
{"SignalingProtocol", &DMWRITE, DMT_STRING, get_voice_profile_signalprotocol, set_voice_profile_signaling_protocol, NULL, NULL},
{"MaxSessions", &DMREAD, DMT_UNINT, get_voice_profile_max_sessions, NULL, NULL, NULL},
{"NumberOfLines", &DMREAD, DMT_UNINT, get_voice_profile_number_of_lines, NULL, NULL, NULL},
{"DTMFMethod", &DMWRITE, DMT_STRING, get_voice_profile_sip_dtmfmethod, set_voice_profile_sip_dtmfmethod, NULL, NULL},
{"Region", &DMWRITE, DMT_STRING, get_sip_profile_region, set_sip_profile_region, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.SIP. ***/
DMLEAF tProfileSIPParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"ProxyServer", &DMWRITE, DMT_STRING, get_voice_profile_sip_proxyserver, set_voice_profile_sip_proxyserver, NULL, NULL},
{"ProxyServerPort", &DMWRITE, DMT_UNINT, get_empty, set_sip_proxy_server_port, NULL, NULL},
{"ProxyServerTransport", &DMWRITE, DMT_STRING, get_sip_proxy_server_transport, set_sip_proxy_server_transport, NULL, NULL},
{"RegistrarServer", &DMWRITE, DMT_STRING, get_voice_profile_sip_registerserver, set_voice_profile_sip_registerserver, NULL, NULL},
{"RegistrarServerPort", &DMWRITE, DMT_UNINT, get_voice_profile_sip_registerserverport, set_voice_profile_sip_registerserverport, NULL, NULL},
{"RegistrarServerTransport", &DMWRITE, DMT_STRING, get_sip_registrar_server_transport, set_sip_registrar_server_transport, NULL, NULL},
{"UserAgentDomain", &DMWRITE, DMT_STRING, get_sip_user_agent_domain, set_sip_user_agent_domain, NULL, NULL},
{"UserAgentPort", &DMWRITE, DMT_UNINT, get_sip_user_agent_port, set_sip_user_agent_port, NULL, NULL},
{"UserAgentTransport", &DMWRITE, DMT_STRING, get_sip_user_agent_transport, set_sip_user_agent_transport, NULL, NULL},
{"OutboundProxy", &DMWRITE, DMT_STRING, get_sip_outbound_proxy, set_sip_outbound_proxy, NULL, NULL},
{"OutboundProxyPort", &DMWRITE, DMT_UNINT, get_sip_outbound_proxy_port, set_sip_outbound_proxy_port, NULL, NULL},
{"RegistrationPeriod", &DMWRITE, DMT_UNINT, get_sip_registration_period, set_sip_registration_period, NULL, NULL},
{"ReInviteExpires", &DMWRITE, DMT_UNINT, get_sip_re_invite_expires, set_sip_re_invite_expires, NULL, NULL},
{"RegisterExpires", &DMWRITE, DMT_UNINT, get_sip_re_invite_expires, set_sip_re_invite_expires, NULL, NULL},
{"RegisterRetryInterval", &DMWRITE, DMT_UNINT, get_sip_re_invite_expires, set_sip_re_invite_expires, NULL, NULL},
{"X_002207_CallLines", &DMWRITE, DMT_STRING, get_sip_x_002207_call_lines, set_sip_x_002207_call_lines, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.ServiceProviderInfo. ***/
DMLEAF tServiceProviderInfoParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Name", &DMWRITE, DMT_STRING, get_voice_service_serviceproviderinfo_name, set_voice_service_serviceproviderinfo_name, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.FaxT38. ***/
DMLEAF tFaxT38Params[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Enable", &DMWRITE, DMT_BOOL, get_sip_fax_t38_enable, set_sip_fax_t38_enable, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.RTP. ***/
DMOBJ tRTPObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"RTCP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tRTCPParams, NULL},
{"SRTP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tSRTPParam, NULL},
{0}
};

DMLEAF tRTPParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"LocalPortMin", &DMWRITE, DMT_UNINT, get_voice_service_vp_rtp_portmin, set_voice_service_vp_rtp_portmin, NULL, NULL},
{"LocalPortMax", &DMWRITE, DMT_UNINT, get_voice_service_vp_rtp_portmax, set_voice_profile_rtp_localportmax, NULL, NULL},
{"DSCPMark", &DMWRITE, DMT_UNINT, get_voice_service_vp_rtp_dscp, set_voice_service_vp_rtp_dscp, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.RTP.RTCP. ***/
DMLEAF tRTCPParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Enable", &DMREAD, DMT_BOOL, get_voice_service_vp_rtp_rtcp_enable, NULL, NULL, NULL},
{"TxRepeatInterval", &DMWRITE, DMT_UNINT, get_voice_service_vp_rtp_rtcp_txrepeatinterval, set_voice_service_vp_rtp_rtcp_txrepeatinterval, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.RTP.SRTP. ***/
DMLEAF tSRTPParam[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Enable", &DMWRITE, DMT_BOOL, get_voice_service_vp_rtp_srtp_enable, set_voice_service_vp_rtp_srtp_enable, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.Line.{i}. ***/
DMOBJ tLineObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"CallingFeatures", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tCallingFeaturesParams, NULL},
{"SIP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tLineSIPParams, NULL},
{"Codec", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tLineCodecObj, NULL, NULL},
{0}
};

DMLEAF tLineParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Alias", &DMWRITE, DMT_STRING, get_line_alias, set_line_alias, NULL, NULL},
{"Enable", &DMWRITE, DMT_STRING, get_voice_profile_line_enable, set_voice_profile_line_enable, NULL, NULL},
{"DirectoryNumber", &DMWRITE, DMT_STRING, get_line_directory_number, set_line_directory_number, NULL, NULL},
{"Status", &DMREAD, DMT_STRING, get_voice_profile_line_status, set_line_alias, NULL, NULL},
{"CallState", &DMREAD, DMT_STRING, get_voice_profile_line_callstate, set_line_alias, NULL, NULL},
{"X_002207_LineProfile", &DMWRITE, DMT_STRING, get_line_x_002207_line_profile, set_line_x_002207_line_profile, NULL, NULL},
{"X_002207_BRCMLine", &DMWRITE, DMT_STRING, get_line_x_002207_brcm_line, set_line_x_002207_brcm_line, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.Line.{i}.CallingFeatures. ***/
DMLEAF tCallingFeaturesParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"CallerIDName", &DMWRITE, DMT_STRING, get_line_calling_features_caller_id_name, set_line_calling_features_caller_id_name, NULL, NULL},
{"CallWaitingEnable", &DMWRITE, DMT_BOOL, get_line_calling_features_callwaiting, set_line_calling_features_callwaiting, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.Line.{i}.SIP. ***/
DMLEAF tLineSIPParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"AuthUserName", &DMWRITE, DMT_STRING, get_line_sip_auth_username, set_line_sip_auth_username, NULL, NULL},
{"AuthPassword", &DMWRITE, DMT_STRING, get_empty, set_line_sip_auth_password, NULL, NULL},
{"URI", &DMWRITE, DMT_STRING, get_line_sip_uri, set_line_sip_uri, NULL, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Codec. ***/
DMOBJ tLineCodecObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, notification, nextobj, leaf*/
{"List", &DMREAD, NULL, NULL, NULL, browseLineCodecListInst, NULL, NULL, NULL, tLineCodecListParams, NULL},
{0}
};

/*** VoiceService.{i}.VoiceProfile.{i}.Line.{i}.Codec.List.{i}. ***/
DMLEAF tLineCodecListParams[] = {
/* PARAM, permission, type, getvlue, setvalue, forced_inform, notification , linker*/
{"Alias", &DMWRITE, DMT_STRING, get_line_codec_list_alias, set_line_codec_list_alias, NULL, NULL},
{"EntryID", &DMREAD, DMT_UNINT, get_codec_entry_id, NULL, NULL, NULL},
{"Codec", &DMREAD, DMT_STRING, capabilities_sip_codecs_get_codec, NULL, NULL, NULL},
{"BitRate", &DMREAD, DMT_UNINT, capabilities_sip_codecs_get_bitrate, NULL, NULL, NULL},
{"PacketizationPeriod", &DMWRITE, DMT_STRING, get_capabilities_sip_codecs_pperiod, set_line_codec_list_packetization, NULL, NULL},
{"SilenceSuppression", &DMREAD, DMT_BOOL, get_false_value, NULL, NULL, NULL},
{"Enable", &DMWRITE, DMT_BOOL, get_line_codec_list_enable, set_line_codec_list_enable, NULL, NULL},
{"Priority", &DMWRITE, DMT_UNINT, get_line_codec_list_priority, set_line_codec_list_priority, NULL, NULL},
{0}
};

int browseVoiceServiceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *s = NULL;
	char *vs = NULL, *vs_last = NULL;

	update_section_list("dmmap","voice_service", NULL, 1, NULL, NULL, NULL, NULL, NULL);
	uci_foreach_sections("dmmap", "voice_service", s) {
		vs = handle_update_instance(1, dmctx, &vs_last, update_instance_alias, 3, s, "vsinstance", "vsalias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)s, vs) == DM_STOP)
			break;
	}
	return 0;
}

int browseCodecsInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	int i = 0;
	char *id, *id_last = NULL;
	struct uci_section *code_sec;
	struct codec_args curr_codec_args = {0};

	init_allowed_sip_codecs();
	codec_update_id();
	uci_foreach_sections("dmmap", "codec_id", code_sec) {
		init_codec_args(&curr_codec_args, allowed_sip_codecs[i].allowed_cdc, allowed_sip_codecs[i].id, allowed_sip_codecs[i].enumid, code_sec);
		id = handle_update_instance(2, dmctx, &id_last, update_instance_alias, 3, code_sec, "codecinstance", "codecalias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_codec_args, id) == DM_STOP) {
			fini_codec_args(&curr_codec_args);
			break;
		}
		fini_codec_args(&curr_codec_args);
		i++;
	}
	return 0;
}

int browseProfileInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	struct uci_section *sip_section;
	char *profile_num = NULL, *profile_num_last = NULL;
	struct sip_args curr_sip_args = {0};

	wait_voice_service_up();
	uci_foreach_sections("voice_client", "sip_service_provider", sip_section) {
		profile_num = handle_update_instance(2, dmctx, &profile_num_last, update_instance_alias, 3, sip_section, "profileinstance", "profilealias");
		init_sip_args(&curr_sip_args, sip_section, profile_num_last);
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_sip_args, profile_num) == DM_STOP)
			break;
	}
	return 0;
}

int browseLineInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	int maxLine, line_id = 0;
	char *line_num = NULL, *last_inst = NULL;
	struct uci_section *b_section = NULL;
	json_object *res, *jobj;
	struct sip_args *sipargs = (struct sip_args *)prev_data;
	struct brcm_args curr_brcm_args = {0};
	maxLine = get_voice_service_max_line();
	uci_foreach_option_eq("voice_client", "brcm_line", "sip_account", section_name(sipargs->sip_section), b_section) {
		line_id = atoi(section_name(b_section) + sizeof("brcm") - 1);
		if ( line_id >= maxLine )
			continue;
		line_num = handle_update_instance(3, dmctx, &last_inst, update_vp_line_instance_alias, 2, b_section, section_name(sipargs->sip_section));
		init_brcm_args(&curr_brcm_args, b_section, sipargs->sip_section, sipargs->profile_num); //check difference between sipargs->profile_num and profile_num
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_brcm_args, line_num) == DM_STOP)
			break;
	}
	return 0;
}

int browseLineCodecListInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance)
{
	int i = 0;
	char *id = NULL , *id_last = NULL;
	struct brcm_args *brcmargs = (struct brcm_args *)prev_data;
	struct uci_section *code_sec = NULL;
	struct line_codec_args curr_line_codec_args = {0};

	codec_update_id();
	codec_priority_update(brcmargs->sip_section);
	uci_foreach_sections("dmmap", "codec_id", code_sec) {
		init_line_code_args(&curr_line_codec_args, i, brcmargs->sip_section, code_sec);
		id = handle_update_instance(4, dmctx, &id_last, update_instance_alias, 3, code_sec, "codecinstance", "codecalias");
		if (DM_LINK_INST_OBJ(dmctx, parent_node, (void *)&curr_line_codec_args, id) == DM_STOP)
			break;
		i++;
	}
	return 0;
}
