/*
    connectionRequest.c

    cwmp service client in C

--------------------------------------------------------------------------------
cwmp service client
Copyright (C) 2011-2012, Inteno, Inc. All Rights Reserved.

Any distribution, dissemination, modification, conversion, integral or partial
reproduction, can not be made without the prior written permission of Inteno.
--------------------------------------------------------------------------------
Author contact information:

--------------------------------------------------------------------------------
*/

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libubox/uloop.h>
#include <libubox/usock.h>
#include "cwmp.h"
#include "http.h"

struct event_container *cwmp_add_event_container (struct cwmp *cwmp, int event_idx, char *command_key);
static void netlink_new_msg(struct uloop_fd *ufd, unsigned events);
static void freecwmp_kickoff(struct uloop_timeout *);

static struct uloop_timeout netlink_timer = { .cb = freecwmp_kickoff };
static struct http_server	http_s;
static struct cwmp			*cwmp_cr;

static struct uloop_fd netlink_event = { .cb = netlink_new_msg };

static void freecwmp_kickoff(struct uloop_timeout *timeout)
{
	http_server_init();
}

static void freecwmp_netlink_interface(struct nlmsghdr *nlh)
{
	struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
	struct rtattr *rth = IFA_RTA(ifa);
	int rtl = IFA_PAYLOAD(nlh);
	char if_name[IFNAMSIZ], if_addr[INET_ADDRSTRLEN];

	memset(&if_name, 0, sizeof(if_name));
	memset(&if_addr, 0, sizeof(if_addr));

	while (rtl && RTA_OK(rth, rtl)) {
		if (rth->rta_type != IFA_LOCAL) {
			rth = RTA_NEXT(rth, rtl);
			continue;
		}

		uint32_t addr = htonl(* (uint32_t *)RTA_DATA(rth));
		if (htonl(13) == 13) {
			// running on big endian system
		} else {
			// running on little endian system
			addr = __builtin_bswap32(addr);
		}

		if_indextoname(ifa->ifa_index, if_name);
		if (strncmp(cwmp_cr->conf.interface, if_name, IFNAMSIZ)) {
			rth = RTA_NEXT(rth, rtl);
			continue;
		}

		inet_ntop(AF_INET, &(addr), if_addr, INET_ADDRSTRLEN);

		if (cwmp_cr->conf.ip) FREE(cwmp_cr->conf.ip);
		cwmp_cr->conf.ip = strdup(if_addr);
		break;
	}

	if (strlen(if_addr) == 0) return;

	uloop_timeout_set(&netlink_timer, 2500);
}

static void netlink_new_msg(struct uloop_fd *ufd, unsigned events)
{
	struct nlmsghdr *nlh;
	char buffer[BUFSIZ];
	int msg_size;

	memset(&buffer, 0, sizeof(buffer));

	nlh = (struct nlmsghdr *)buffer;
	if ((msg_size = recv(ufd->fd, nlh, BUFSIZ, 0)) == -1) {
		CWMP_LOG(ERROR,"error receiving netlink message");
		return;
	}

	while (msg_size > sizeof(*nlh)) {
		int len = nlh->nlmsg_len;
		int req_len = len - sizeof(*nlh);

		if (req_len < 0 || len > msg_size) {
			CWMP_LOG(ERROR,"error reading netlink message");
			return;
		}

		if (!NLMSG_OK(nlh, msg_size)) {
			CWMP_LOG(ERROR,"netlink message is not NLMSG_OK");
			return;
		}

		if (nlh->nlmsg_type == RTM_NEWADDR)
			freecwmp_netlink_interface(nlh);

		msg_size -= NLMSG_ALIGN(len);
		nlh = (struct nlmsghdr*)((char*)nlh + NLMSG_ALIGN(len));
	}
}

static int netlink_init(void)
{
	struct {
		struct nlmsghdr hdr;
		struct ifaddrmsg msg;
	} req;
	struct sockaddr_nl addr;
	int sock[2];

	memset(&addr, 0, sizeof(addr));
	memset(&req, 0, sizeof(req));

	if ((sock[0] = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {
		CWMP_LOG(ERROR,"couldn't open NETLINK_ROUTE socket");
		return -1;
	}

	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTMGRP_IPV4_IFADDR;
	if ((bind(sock[0], (struct sockaddr *)&addr, sizeof(addr))) == -1) {
		CWMP_LOG(ERROR,"couldn't bind netlink socket");
		return -1;
	}

	netlink_event.fd = sock[0];
	uloop_fd_add(&netlink_event, ULOOP_READ | ULOOP_EDGE_TRIGGER);

	if ((sock[1] = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) == -1) {
		CWMP_LOG(ERROR,"couldn't open NETLINK_ROUTE socket");
		return -1;
	}

	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	req.hdr.nlmsg_type = RTM_GETADDR;
	req.msg.ifa_family = AF_INET;

	if ((send(sock[1], &req, req.hdr.nlmsg_len, 0)) == -1) {
		CWMP_LOG(ERROR,"couldn't send netlink socket");
		return -1;
	}

	struct uloop_fd dummy_event = { .fd = sock[1] };
	netlink_new_msg(&dummy_event, 0);

	return 0;
}

void http_server_init(void)
{
	char	port[16];

	sprintf(port,"%d",cwmp_cr->conf.connection_request_port);
	http_s.http_event.cb = http_new_client;
	http_s.http_event.fd = usock(USOCK_TCP | USOCK_SERVER, cwmp_cr->conf.ip, port);
	uloop_fd_add(&http_s.http_event, ULOOP_READ | ULOOP_EDGE_TRIGGER);
}

static void http_new_client(struct uloop_fd *ufd, unsigned events)
{
	int status;
	struct timeval t;

	t.tv_sec = 60;
	t.tv_usec = 0;

	for (;;) {
		int client = accept(ufd->fd, NULL, NULL);

		/* set one minute timeout */
		if (setsockopt(ufd->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof t)) {
			CWMP_LOG(ERROR,"setsockopt() failed\n");
		}

		if (client == -1)
			break;

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

			while (fgets(buffer, sizeof(buffer), fp)) {
				if (!strncasecmp(buffer, "Authorization: Basic ", strlen("Authorization: Basic "))) {
					const char *c1, *c2, *min, *val;
					char *username = NULL;
					char *password = NULL;
					char *acs_auth_basic = NULL;
					char *auth_basic_check = NULL;
					int len;

					username = cwmp_cr->conf.cpe_userid;
					password = cwmp_cr->conf.cpe_passwd;

					if (!username || !password) {
						// if we dont have username or password configured proceed with connecting to ACS
						FREE(username);
						FREE(password);
						auth_status = 1;
						goto http_end_child;
					}

					c1 = strrchr(buffer, '\r');
					c2 = strrchr(buffer, '\n');

					if (!c1)
						c1 = c2;
					if (!c2)
						c2 = c1;
					if (!c1 || !c2)
						continue;

					min = (c1 < c2) ? c1 : c2;

					val = strrchr(buffer, ' ');
					if (!val)
						continue;

					val += sizeof(char);
					ssize_t size = min - val;

					acs_auth_basic = (char *) zstream_b64decode(val, &size);
					if (!acs_auth_basic)
						continue;

					if (asprintf(&auth_basic_check, "%s:%s", username, password) == -1) {
						FREE(username);
						FREE(password);
						free(acs_auth_basic);
						goto error_child;
					}

					if (size == strlen(auth_basic_check)) {
						len = size;
					} else {
						auth_status = 0;
						goto free_resources;
					}

					if (!memcmp(acs_auth_basic, auth_basic_check, len * sizeof(char)))
						auth_status = 1;
					else
						auth_status = 0;

free_resources:
					free(acs_auth_basic);
					free(auth_basic_check);
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
				status = 0;
				fputs("HTTP/1.1 204 No Content\r\n\r\n", fp);
			} else {
				status = EACCES;
				fputs("HTTP/1.1 401 Unauthorized\r\n", fp);
				fputs("Connection: close\r\n", fp);
				fputs("WWW-Authenticate: Basic realm=\"default\"\r\n", fp);
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
        pthread_mutex_lock (&(cwmp_cr->mutex_session_queue));
        event_container = cwmp_add_event_container (cwmp_cr, EVENT_IDX_6CONNECTION_REQUEST, "");
        pthread_mutex_unlock (&(cwmp_cr->mutex_session_queue));
        pthread_cond_signal(&(cwmp_cr->threshold_session_send));
	}
}

void *thread_connection_request_listener (void *v)
{
	cwmp_cr = (struct cwmp *) v;
	if (netlink_init()) {
		CWMP_LOG(ERROR,"netlink initialization failed");
	}
    return CWMP_OK;
}
