/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *	Contributed by Inteno Broadband Technology AB
 *
 *	Copyright (C) 2013 Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	Copyright (C) 2013 Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "cwmp.h"
#include "log.h"

#include <libubox/uloop.h>
#include <libubox/usock.h>

#ifdef HTTP_CURL
#include <curl/curl.h>
#endif

#ifdef HTTP_ZSTREAM
#include <zstream.h>
#include <zstream/http.h>
#endif

#include "http.h"

#include "digestauth.h"

#define REALM "authenticate@cwmp"
#define OPAQUE "11733b200778ce33060f31c9af70a870ba96ddd4"

static struct http_client http_c;
static struct http_server http_s;

#ifdef HTTP_CURL
static CURL *curl;
#endif

int
http_client_init(struct cwmp *cwmp)
{
#ifdef HTTP_CURL
	if (asprintf(&http_c.url, "%s",
		     cwmp->conf.acsurl) == -1)
		return -1;
#endif

#ifdef HTTP_ZSTREAM
	char *add = strstr(cwmp->conf.acsurl,"://");
	if(!add) return -1;
	if (asprintf(&http_c.url, "%.*s://%s:%s@%s",
			 add-cwmp->conf.acsurl,
			 cwmp->conf.acsurl,
			 cwmp->conf.acs_userid,
			 cwmp->conf.acs_passwd,
		     add+3) == -1)
		return -1;
#endif

	CWMP_LOG(INFO, "ACS url: %s", http_c.url);

	/* TODO debug ssl config from freecwmp*/

#ifdef HTTP_CURL
	http_c.header_list = NULL;
	http_c.header_list = curl_slist_append(http_c.header_list, "User-Agent: cwmp");
	if (!http_c.header_list) return -1;
	http_c.header_list = curl_slist_append(http_c.header_list, "Content-Type: text/xml");
	if (!http_c.header_list) return -1;
# ifdef ACS_FUSION
	char *expect_header = "Expect:";
	http_c.header_list = curl_slist_append(http_c.header_list, expect_header);
	if (!http_c.header_list) return -1;
# endif /* ACS_FUSION */
	curl = curl_easy_init();
	if (!curl) return -1;

#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	http_c.stream = zstream_open(http_c.url, ZSTREAM_POST);
	if (!http_c.stream)
		return -1;

# ifdef ACS_HDM
	if (zstream_http_configure(http_c.stream, ZSTREAM_HTTP_COOKIES, 1))
# elif ACS_MULTI
	if (zstream_http_configure(http_c.stream, ZSTREAM_HTTP_COOKIES, 3))
# endif
		return -1;

	if (zstream_http_addheader(http_c.stream, "User-Agent", "cwmp"))
		return -1;

	if (zstream_http_addheader(http_c.stream, "Content-Type", "text/xml"))
		return -1;
#endif /* HTTP_ZSTREAM */

	return 0;
}

void
http_client_exit(void)
{
	FREE(http_c.url);

#ifdef HTTP_CURL
	if (http_c.header_list) {
		curl_slist_free_all(http_c.header_list);
		http_c.header_list = NULL;
	}
	if (access(fc_cookies, W_OK) == 0)
		remove(fc_cookies);
	curl_easy_cleanup(curl);
	curl_global_cleanup();

#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	if (http_c.stream) {
		zstream_close(http_c.stream);
		http_c.stream = NULL;
	}
#endif /* HTTP_ZSTREAM */
}

#ifdef HTTP_CURL
static size_t
http_get_response(void *buffer, size_t size, size_t rxed, char **msg_in)
{
	char *c;

	if (asprintf(&c, "%s%.*s", *msg_in, size * rxed, buffer) == -1) {
		FREE(*msg_in);
		return -1;
	}

	free(*msg_in);
	*msg_in = c;

	return size * rxed;
}
#endif /* HTTP_CURL */

int
http_send_message(struct cwmp *cwmp, char *msg_out, char **msg_in)
{
#ifdef HTTP_CURL
	CURLcode res;
	long http_code = 0;
	curl_easy_setopt(curl, CURLOPT_URL, http_c.url);
	curl_easy_setopt(curl, CURLOPT_USERNAME, cwmp->conf.acs_userid);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, cwmp->conf.acs_passwd);
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC|CURLAUTH_DIGEST);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_c.header_list);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, HTTP_TIMEOUT);

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, msg_out);
	if (msg_out)
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(msg_out));
	else
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_get_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, msg_in);

# ifdef DEVEL
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
# endif

	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, fc_cookies);
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, fc_cookies);

	/* TODO: ssl config (from freecwmp) and test it with real ACS configuration */

	*msg_in = (char *) calloc (1, sizeof(char));

	res = curl_easy_perform(curl);

	if (!strlen(*msg_in))
		FREE(*msg_in);

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	if(http_code == 204)
	{
		CWMP_LOG (INFO,"Receive HTTP 204 No Content");
	}

	if (http_code != 200 && http_code != 204)
		goto error;

	/* TODO add check for 301, 302 and 307 HTTP Redirect*/

	curl_easy_reset(curl);

	if (res) goto error;

#endif /* HTTP_CURL */

#ifdef HTTP_ZSTREAM
	char buffer[BUFSIZ];
	ssize_t rxed;
	if (zstream_reopen(http_c.stream, http_c.url, ZSTREAM_POST)) {
		/* something not good, let's try recreate */
		http_client_exit();
		if (http_client_init(cwmp)) return -1;
	}


	if (msg_out) {
		zstream_write(http_c.stream, msg_out, strlen(msg_out));
	} else {
		zstream_write(http_c.stream, NULL , 0);
	}

	*msg_in = (char *) calloc (1, sizeof(char));
	while ((rxed = zstream_read(http_c.stream, buffer, sizeof(buffer))) > 0) {
		*msg_in = (char *) realloc(*msg_in, (strlen(*msg_in) + rxed + 1) * sizeof(char));
		if (!(*msg_in)) return -1;
		bzero(*msg_in + strlen(*msg_in), rxed + 1);
		memcpy(*msg_in + strlen(*msg_in), buffer, rxed);
	}

	/* we got no response, that is ok and defined in documentation */
	if (!strlen(*msg_in)) {
		FREE(*msg_in);
	}

	if (rxed < 0)
		goto error;

#endif /* HTTP_ZSTREAM */


	return 0;

error:
	FREE(*msg_in);
	return -1;
}

void http_server_init(void)
{
	char	port[16];

	sprintf(port,"%d",cwmp_main.conf.connection_request_port);
	http_s.http_event.cb = http_new_client;
	http_s.http_event.fd = usock(USOCK_TCP | USOCK_SERVER, "0.0.0.0", port);
	uloop_fd_add(&http_s.http_event, ULOOP_READ | ULOOP_EDGE_TRIGGER);
}

static void http_new_client(struct uloop_fd *ufd, unsigned events)
{
	int status;
	struct timeval t;
    static int      cr_request = 0;
    static time_t   restrict_start_time = 0;
    time_t          current_time;
    bool			service_available;

	t.tv_sec = 60;
	t.tv_usec = 0;

	for (;;) {
		int client = accept(ufd->fd, NULL, NULL);

		/* set one minute timeout */
		if (setsockopt(ufd->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof t)) {
			CWMP_LOG(ERROR,"setsockopt() failed");
		}

		if (client == -1)
			break;

	    current_time = time(NULL);
	    service_available = true;
	    if ((restrict_start_time==0) ||
	    	((current_time-restrict_start_time) > CONNECTION_REQUEST_RESTRICT_PERIOD))
	    {
	        restrict_start_time = current_time;
	        cr_request          = 1;
	    }
	    else
	    {
	        cr_request++;
	        if (cr_request>CONNECTION_REQUEST_RESTRICT_REQUEST)
	        {
	            restrict_start_time = current_time;
	            service_available = false;
	        }
	    }

		struct uloop_process *uproc = calloc(1, sizeof(*uproc));
		if (!uproc || (uproc->pid = fork()) == -1) {
			free(uproc);
			close(client);
		}

		if (uproc->pid != 0) {
			/* parent */
			/* register an event handler for when the child terminates */
			uproc->cb = http_del_client;
			uloop_process_add(uproc);
			close(client);
		} else {
			/* child */
			FILE *fp;
			char buffer[BUFSIZ];
			int8_t auth_status = 0;

			fp = fdopen(client, "r+");

			if(!service_available)
				goto http_end_child;

			while (fgets(buffer, sizeof(buffer), fp)) {
				if (!strncasecmp(buffer, "Authorization: Digest ", strlen("Authorization: Digest "))) {
					char *username = cwmp_main.conf.cpe_userid;
					char *password = cwmp_main.conf.cpe_passwd;

					if (!username || !password) {
						// if we dont have username or password configured proceed with connecting to ACS
						service_available = false;
						goto http_end_child;
					}

					if (http_digest_auth_check("GET", "/", buffer + strlen("Authorization: Digest "), REALM, username, password, 300) == MHD_YES)
						auth_status = 1;
					else
						auth_status = 0;
				}

				if (buffer[0] == '\r' || buffer[0] == '\n') {
					/* end of http request (empty line) */
					goto http_end_child;
				}

			}
error_child:
			/* here we are because of an error, e.g. timeout */
			status = ETIMEDOUT|ENOMEM;
			goto done_child;

http_end_child:
			fflush(fp);
			if (auth_status) {
				CWMP_LOG (INFO,"Receive Connection Request: success authentication");
				status = 0;
				fputs("HTTP/1.1 200 OK\r\n", fp);
				fputs("Content-Length: 0\r\n", fp);
			} else if (!service_available) {
				CWMP_LOG (INFO,"Receive Connection Request: Return 503 Service Unavailable");
				status = EACCES;
				fputs("HTTP/1.1 503 Service Unavailable\r\n", fp);
				fputs("Connection: close\r\n", fp);
			} else {
				CWMP_LOG (INFO,"Receive Connection Request: Return 401 Unauthorized");
				status = EACCES;
				fputs("HTTP/1.1 401 Unauthorized\r\n", fp);
				fputs("Connection: close\r\n", fp);
				http_digest_auth_fail_response(fp, "GET", "/", REALM, OPAQUE);
				fputs("\r\n", fp);
			}
			fputs("\r\n", fp);
			goto done_child;

done_child:
			fclose(fp);
			free(uproc);
			exit(status);
		}
	}
}

static void
http_del_client(struct uloop_process *uproc, int ret)
{
	free(uproc);
	struct event_container  *event_container;
	/* child terminated ; check return code */
	if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        CWMP_LOG(INFO,"Connection Request thread: add connection request event in the queue");
        pthread_mutex_lock (&(cwmp_main.mutex_session_queue));
        event_container = cwmp_add_event_container (&cwmp_main, EVENT_IDX_6CONNECTION_REQUEST, "");
        pthread_mutex_unlock (&(cwmp_main.mutex_session_queue));
        pthread_cond_signal(&(cwmp_main.threshold_session_send));
	}
}
