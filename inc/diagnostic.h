/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2014 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 */
#ifndef __DIAGNOSTIC__H
#define __DIAGNOSTIC__H

#define default_date_format "AAAA-MM-JJTHH:MM:SS.000000Z"
#define default_date_size sizeof(default_date_format) + 1
#define FTP_SIZE_RESPONSE "213"
#define FTP_PASV_RESPONSE "227 Entering Passive"
#define FTP_TRANSFERT_COMPLETE "226 Transfer"
#define FTP_RETR_REQUEST "RETR"
#define FTP_STOR_REQUEST "STOR"

struct download_diag
{
	char romtime[default_date_size];
	char bomtime[default_date_size];
	char eomtime[default_date_size];
	int test_bytes_received;
	unsigned long total_bytes_received;
	char tcpopenrequesttime[default_date_size];
	char tcpopenresponsetime[default_date_size];
	int tmp;
	int first_data;
	uint16_t ip_len;
	uint32_t ack_seq;
	uint32_t random_seq;
	uint32_t get_ack;
	uint32_t ftp_syn;
};

struct upload_diagnostic_stats
{
	char romtime[default_date_size];
	char bomtime[default_date_size];
	char eomtime[default_date_size];
	int test_bytes_received;
	unsigned long total_bytes_sent;
	char tcpopenrequesttime[default_date_size];
	char tcpopenresponsetime[default_date_size];
	int tmp;
	int first_data;
	uint16_t ip_len;
	uint32_t ack_seq;
	uint32_t random_seq;
	uint32_t get_ack;
	uint32_t ftp_syn;
};


enum download_diagnostic_protocol {
	DOWNLOAD_DIAGNOSTIC_HTTP = 1,
	DOWNLOAD_DIAGNOSTIC_FTP
};

enum diagnostic_type {
	DOWNLOAD_DIAGNOSTIC = 1,
	UPLOAD_DIAGNOSTIC
};

int cwmp_ip_ping_diagnostic();
int cwmp_start_diagnostic(int diagnostic_type);

#endif
