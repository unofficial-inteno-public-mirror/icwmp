/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2011 Luka Perkov <freecwmp@lukaperkov.net>
 */

#ifndef _FREECWMP_HTTP_H__
#define _FREECWMP_HTTP_H__

#include <stdint.h>

#include <libubox/uloop.h>

#ifdef HTTP_CURL
#include <curl/curl.h>
#endif

#ifdef HTTP_ZSTREAM
#include <zstream.h>
#endif

#ifdef DUMMY_MODE
static char *fc_cookies = "./ext/tmp/icwmp_cookies";
#else
static char *fc_cookies = "/tmp/icwmp_cookies";
#endif

#define HTTP_TIMEOUT 30

struct http_client
{
#ifdef HTTP_CURL
	struct curl_slist *header_list;
#endif /* HTTP_CURL */
#ifdef HTTP_ZSTREAM
	zstream_t *stream;
#endif /* HTTP_ZSTREAM */
	char *url;
};

#ifdef HTTP_CURL
static size_t http_get_response(void *buffer, size_t size, size_t rxed, char **msg_in);
#endif /* HTTP_CURL */

int http_client_init(struct cwmp *cwmp);
void http_client_exit(void);
int http_send_message(struct cwmp *cwmp, char *msg_out, char **msg_in);

void http_server_init(void);
void http_server_listen(void);

#endif

