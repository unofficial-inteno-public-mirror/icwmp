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
};

struct rtp_tos
{
	char *key;
	char *val;
};
struct cap_sip_codec
{
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
	char *id;
	char *allowed_cdc;
};

struct line_codec_args
{
	char *sip_id;
	char *cdc;
	char *id;
	struct uci_section *sip_section;
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
	char *pid;
};


int entry_method_root_Service(struct dmctx *ctx);
bool dm_service_enable_set(void);
#endif