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

int get_max_profile_count(char *refparam, struct dmctx *ctx, char **value);
int get_max_line_count(char *refparam, struct dmctx *ctx, char **value);
int get_max_session_per_line(char *refparam, struct dmctx *ctx, char **value);
int get_max_session_count(char *refparam, struct dmctx *ctx, char **value);
int get_signal_protocols(char *refparam, struct dmctx *ctx, char **value);
int get_regions(char *refparam, struct dmctx *ctx, char **value);
int get_true_value (char *refparam, struct dmctx *ctx, char **value);
int get_false_value (char *refparam, struct dmctx *ctx, char **value);
int get_sip_role (char *refparam, struct dmctx *ctx, char **value);
int get_sip_extension (char *refparam, struct dmctx *ctx, char **value);
int get_sip_transport (char *refparam, struct dmctx *ctx, char **value);
int get_sip_tls_auth_protocols (char *refparam, struct dmctx *ctx, char **value);
int get_sip_tls_enc_protocols (char *refparam, struct dmctx *ctx, char **value);
int get_sip_tls_key_protocols (char *refparam, struct dmctx *ctx, char **value);
int get_entry_id (char *refparam, struct dmctx *ctx, char **value);
int get_capabilities_sip_codecs (char *refparam, struct dmctx *ctx, char **value);
int get_capabilities_sip_bitrate (char *refparam, struct dmctx *ctx, char **value);
int get_capabilities_sip_pperiod (char *refparam, struct dmctx *ctx, char **value);

int get_voice_service_max_line ();
int get_voice_profile_enable (char *refparam, struct dmctx *ctx, char **value);
int set_voice_profile_enable (char *refparam, struct dmctx *ctx, int action, char *value);
int get_voice_profile_name (char *refparam, struct dmctx *ctx, char **value);



inline int get_voice_service_capabilities_codecs_generic(struct dmctx *ctx, char *idev, char *id);

int entry_method_root_Service(struct dmctx *ctx);
#endif