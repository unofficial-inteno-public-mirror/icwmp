/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2016 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 */
#ifndef __DOWNLOAD_DIAGNOSTIC_H
#define __DOWNLOAD_DIAGNOSTIC_H

int entry_method_root_Download_Diagnostics(struct dmctx *ctx);
static inline char *download_diagnostic_get(char *option, char *def);
int get_download_diagnostics_state(char *refparam, struct dmctx *ctx, char **value);
int set_download_diagnostics_state(char *refparam, struct dmctx *ctx, int action, char *value);
int get_download_diagnostics_interface(char *refparam, struct dmctx *ctx, char **value);
int set_download_diagnostics_interface(char *refparam, struct dmctx *ctx, int action, char *value);
int get_download_diagnostics_url(char *refparam, struct dmctx *ctx, char **value);
int set_download_diagnostics_url(char *refparam, struct dmctx *ctx, int action, char *value);
int get_download_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, char **value);
int set_download_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, int action, char *value);
int get_download_diagnostic_romtime(char *refparam, struct dmctx *ctx, char **value);
int get_download_diagnostic_bomtime(char *refparam, struct dmctx *ctx, char **value);
int get_download_diagnostic_eomtime(char *refparam, struct dmctx *ctx, char **value);
int get_download_diagnostic_testbytes(char *refparam, struct dmctx *ctx, char **value);
int get_download_diagnostic_totalbytes(char *refparam, struct dmctx *ctx, char **value);
int get_download_diagnostic_tcp_open_request_time(char *refparam, struct dmctx *ctx, char **value);
int get_download_diagnostic_tcp_open_response_time(char *refparam, struct dmctx *ctx, char **value);

extern DMLEAF tDownloadDiagnosticsParam[];
#endif
