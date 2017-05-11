/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
*		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Feten Besbes <feten.besbes@pivasoftware.com>
 *		Author: Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 */
#ifndef __VOICE_H
#define __VOICE_H

extern DMOBJ tServiceObj[];
extern DMOBJ tVoiceServiceObj[];
extern DMLEAF tVoiceServiceParam[];
extern DMLEAF tCapabilitiesParams[];
extern DMOBJ tCapabilitiesObj[];
extern DMLEAF tSIPParams[];
extern DMLEAF tCodecsParams[] ;
extern DMOBJ tProfileObj[] ;
extern DMLEAF tProfileSIPParams[];
extern DMLEAF tServiceProviderInfoParams[];
extern DMLEAF tProfileParam[];
extern DMOBJ tLineObj[];
extern DMOBJ tLineCodecObj[];
extern DMLEAF tLineCodecListParams[];
extern DMLEAF tLineSIPParams[];
extern DMLEAF tCallingFeaturesParams[];
extern DMLEAF tLineParams[];
extern DMLEAF tRTPParams[];
extern DMOBJ tRTPObj[];
extern DMLEAF tSRTPParam[];
extern DMLEAF tRTCPParams[];
extern DMLEAF tFaxT38Params[];

/*Args of get_voice_service_capabilities_codecs_generic*/
struct codec_args
{
	char *cdc;
	char *id;
	int enumid;
	struct uci_section *codec_section;
};

struct rtp_tos
{
	char *key;
	char *val;
};

struct cap_sip_codec
{
	int enumid;
	char *c1;
	char *c2;
	char *c3;
	char *c4;
	char *c5;
};

struct sip_args
{
	struct uci_section *sip_section;
	char *profile_num;
};

struct brcm_args
{
	struct uci_section *brcm_section;
	struct uci_section *sip_section;
	char *profile_num;
};

struct allow_sip_codec
{
	int enumid;
	char *id;
	char *allowed_cdc;
	char *priority_cdc;
	char *ptime_cdc;
};

struct line_codec_args
{
	int enumid;
	char *sip_id;
	char *cdc;
	char *id;
	char *priority_cdc;
	char *ptime_cdc;
	struct uci_section *sip_section;
	struct uci_section *codec_sec;
};

struct region
{
	char *country;
	char *id;
};

struct codec
{
	char *cdc;
	char *id;
	char *priority;
};

enum enum_cap_sip_codecs {
	SIP_CODEC_G723,
	SIP_CODEC_GSM,
	SIP_CODEC_ULAW,
	SIP_CODEC_ALAW,
	SIP_CODEC_G726AAL2,
	SIP_CODEC_ADPCM,
	SIP_CODEC_SLIN,
	SIP_CODEC_LPC10,
	SIP_CODEC_G729,
	SIP_CODEC_SPEEX,
	SIP_CODEC_ILBC,
	SIP_CODEC_G726,
	SIP_CODEC_G722,
	SIP_CODEC_SIREN7,
	SIP_CODEC_SIREN14,
	SIP_CODEC_SLIN16,
	SIP_CODEC_G719,
	SIP_CODEC_SPEEX16,
	SIP_CODEC_TESTLAW
};

bool dm_service_enable_set(void);
int browseVoiceServiceInst(struct dmctx *dmctx, DMNODE *parent_node, void *prev_data, char *prev_instance);
#endif
