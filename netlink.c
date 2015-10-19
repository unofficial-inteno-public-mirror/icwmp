/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *	Copyright (C) 2011-2012 Luka Perkov <freecwmp@lukaperkov.net>
 *
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
#include "log.h"

static void netlink_new_msg(struct uloop_fd *ufd, unsigned events);
static struct uloop_fd netlink_event = { .cb = netlink_new_msg };

static void freecwmp_netlink_interface(struct nlmsghdr *nlh)
{
	struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
	struct rtattr *rth = IFA_RTA(ifa);
	int rtl = IFA_PAYLOAD(nlh);
	char if_name[IFNAMSIZ], if_addr[INET_ADDRSTRLEN];
	char *c;

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
		if (strncmp(cwmp_main.conf.interface, if_name, IFNAMSIZ)) {
			rth = RTA_NEXT(rth, rtl);
			continue;
		}

		inet_ntop(AF_INET, &(addr), if_addr, INET_ADDRSTRLEN);

		if (cwmp_main.conf.ip) FREE(cwmp_main.conf.ip);
		cwmp_main.conf.ip = strdup(if_addr);
		if (asprintf(&c,"cwmp.cpe.ip=%s",cwmp_main.conf.ip) != -1)
		{
			uci_set_state_value(c);
			free(c);
		}
		connection_request_ip_value_change(&cwmp_main);
		break;
	}

	if (strlen(if_addr) == 0) return;
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

int netlink_init(void)
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
