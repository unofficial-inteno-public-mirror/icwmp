/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 */
#ifndef __UPLOAD_DIAGNOSTIC_H
#define __UPLOAD_DIAGNOSTIC_H

int entry_method_root_Upload_Diagnostics(struct dmctx *ctx);
extern DMLEAF tUploadDiagnosticsParam[];
static inline char *upload_diagnostic_get(char *option, char *def);
int get_upload_diagnostics_state(char *refparam, struct dmctx *ctx, char **value);
int set_upload_diagnostics_state(char *refparam, struct dmctx *ctx, int action, char *value);
int get_upload_diagnostics_interface(char *refparam, struct dmctx *ctx, char **value);
int set_upload_diagnostics_interface(char *refparam, struct dmctx *ctx, int action, char *value);
int get_upload_diagnostics_url(char *refparam, struct dmctx *ctx, char **value);
int set_upload_diagnostics_url(char *refparam, struct dmctx *ctx, int action, char *value);
int get_upload_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, char **value);
int set_upload_diagnostics_ethernet_priority(char *refparam, struct dmctx *ctx, int action, char *value);
int get_upload_diagnostic_romtime(char *refparam, struct dmctx *ctx, char **value);
int get_upload_diagnostic_bomtime(char *refparam, struct dmctx *ctx, char **value);
int get_upload_diagnostic_eomtime(char *refparam, struct dmctx *ctx, char **value);
int get_upload_diagnostic_test_file_length(char *refparam, struct dmctx *ctx, char **value);
int set_upload_diagnostic_test_file_length(char *refparam, struct dmctx *ctx, int action, char *value);
int get_upload_diagnostic_totalbytes(char *refparam, struct dmctx *ctx, char **value);
int get_upload_diagnostic_tcp_open_request_time(char *refparam, struct dmctx *ctx, char **value);
int get_upload_diagnostic_tcp_open_response_time(char *refparam, struct dmctx *ctx, char **value);

#endif
