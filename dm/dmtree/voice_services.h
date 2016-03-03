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

struct service_args
{
	struct uci_section *service_section;
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

int entry_method_root_Service(struct dmctx *ctx);
bool dm_service_enable_set(void);
#endif
