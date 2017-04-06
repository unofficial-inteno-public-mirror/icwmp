/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com> *	  
 *
 */

#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include "cwmp.h"
#include "backupSession.h"
#include "xml.h"
#include "log.h"
#include "external.h"	
#include "dmentry.h"
#include "dmcommon.h"
#include "ubus.h"
#include "diagnostic.h"
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <libtrace.h>
#include <libpacketdump.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <time.h>


int read_next;
struct download_diag download_stats = {0};
struct upload_diagnostic_stats upload_stats = {0};

static void http_upload_per_packet(libtrace_packet_t *packet);
static void libtrace_cleanup(libtrace_t *trace, libtrace_packet_t *packet);
static void set_download_stats();
static void set_upload_stats();
int init_download_stats()
{
	memset(&download_stats, 0, sizeof(download_stats));
}

int init_upload_stats()
{
	memset(&upload_stats, 0, sizeof(upload_stats));
}

static void ftp_download_per_packet(libtrace_packet_t *packet, int first_packet)
{
	struct tm lt;
	struct timeval ts;
	libtrace_tcp_t *tcp;
	char tcp_flag[16] = "";
	char *nexthdr;
	libtrace_ip_t *ip;
	char s_now[default_date_size];
	uint8_t proto;
	uint32_t remaining;
	tcp = trace_get_transport(packet, &proto, &remaining);

	if (tcp == NULL)
	{
		return;
	}
	else
	{
		nexthdr = trace_get_payload_from_tcp(tcp, &remaining);
	}
	if (tcp->ecn_ns) strcat(tcp_flag, "ECN_NS ");
	if (tcp->cwr) strcat(tcp_flag, "CWR ");
	if (tcp->ece) strcat(tcp_flag, "ECE ");
	if (tcp->fin) strcat(tcp_flag, "FIN ");
	if (tcp->syn) strcat(tcp_flag, "SYN ");
	if (tcp->rst) strcat(tcp_flag, "RST ");
	if (tcp->psh) strcat(tcp_flag, "PSH ");
	if (tcp->ack) strcat(tcp_flag, "ACK ");
	if (tcp->urg) strcat(tcp_flag, "URG ");
	if (strcmp(tcp_flag, "PSH ACK ") == 0 && strlen(nexthdr) > strlen(FTP_SIZE_RESPONSE) && strncmp(nexthdr, FTP_SIZE_RESPONSE, strlen(FTP_SIZE_RESPONSE)) == 0)
	{
		char *val = strstr(nexthdr,"213");
		char *pch, *pchr;
		val += strlen("213 ");
		pch=strtok_r(val, " \r\n\t", &pchr);
		download_stats.test_bytes_received = atoi(pch);
	}
	if(strcmp(tcp_flag, "PSH ACK ") == 0 && strlen(nexthdr) > strlen(FTP_PASV_RESPONSE) && strncmp(nexthdr, FTP_PASV_RESPONSE, strlen(FTP_PASV_RESPONSE)) == 0)
	{
		download_stats.ftp_syn = 1;
		return;
	}
    if (download_stats.random_seq == 0 && strcmp(tcp_flag, "SYN ") == 0 && download_stats.ftp_syn == 1)
	{
    	ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		download_stats.random_seq = ntohl(tcp->seq);
		sprintf((download_stats.tcpopenrequesttime),"%s.%06ld", s_now, (long) ts.tv_usec);
	}
	if (strcmp(tcp_flag, "SYN ACK ") == 0 && download_stats.random_seq != 0 && (ntohl(tcp->ack_seq) - 1 ) == download_stats.random_seq)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((download_stats.tcpopenresponsetime),"%s.%06ld", s_now, (long) ts.tv_usec);
		download_stats.random_seq = ntohl(tcp->ack_seq);
		sprintf((download_stats.tcpopenresponsetime),"%s.%06ld", s_now, (long) ts.tv_usec);
	}

	if (strcmp(tcp_flag, "PSH ACK ") == 0 && strlen(nexthdr) > strlen(FTP_RETR_REQUEST) && strncmp(nexthdr, FTP_RETR_REQUEST, strlen(FTP_RETR_REQUEST)) == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((download_stats.romtime),"%s.%06ld", s_now, (long) ts.tv_usec);
	}
	if(strcmp(tcp_flag, "ACK ") == 0 && ntohl(tcp->seq) == download_stats.random_seq && download_stats.ack_seq == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		download_stats.ack_seq = ntohl(tcp->seq);
		return;
	}
	if(strcmp(tcp_flag, "ACK ") == 0 && ntohl(tcp->ack_seq) == download_stats.ack_seq )
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (download_stats.first_data == 0)
		{
			sprintf((download_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
		}
		download_stats.first_data = 1;
		ip = trace_get_ip(packet);
		if (ip != NULL)
			download_stats.total_bytes_received += ntohs(ip->ip_len);
	}
	if ( (strcmp(tcp_flag, "PSH ACK ") == 0 || strcmp(tcp_flag, "FIN PSH ACK ") == 0) &&  ntohl(tcp->ack_seq) == download_stats.ack_seq)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (download_stats.first_data == 0)
		{
			sprintf((download_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
			download_stats.first_data = 1;
		}
		sprintf((download_stats.eomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
		ip = trace_get_ip(packet);
		if (ip != NULL)
			download_stats.total_bytes_received += ntohs(ip->ip_len);
	}
}


static void http_download_per_packet(libtrace_packet_t *packet, int first_packet)
{
	struct tm lt;
	struct timeval ts;
	libtrace_tcp_t *tcp;
	uint32_t seq = 0;
	char tcp_flag[16] = "";
	char *nexthdr;
	libtrace_ip_t *ip;
	char s_now[default_date_size];

	uint8_t proto;
	uint32_t remaining;
	tcp = trace_get_transport(packet, &proto, &remaining);

	if (tcp == NULL)
	{
		return;
	}
	else
	{
		nexthdr = trace_get_payload_from_tcp(tcp, &remaining);
	}
	if (tcp->ecn_ns) strcat(tcp_flag, "ECN_NS ");
	if (tcp->cwr) strcat(tcp_flag, "CWR ");
	if (tcp->ece) strcat(tcp_flag, "ECE ");
	if (tcp->fin) strcat(tcp_flag, "FIN ");
	if (tcp->syn) strcat(tcp_flag, "SYN ");
	if (tcp->rst) strcat(tcp_flag, "RST ");
	if (tcp->psh) strcat(tcp_flag, "PSH ");
	if (tcp->ack) strcat(tcp_flag, "ACK ");
	if (tcp->urg) strcat(tcp_flag, "URG ");
    if (strcmp(tcp_flag, "SYN ") == 0 && download_stats.random_seq == 0)
	{
    	ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((download_stats.tcpopenrequesttime),"%s.%06ld", s_now, (long) ts.tv_usec);
		download_stats.random_seq = ntohl(tcp->seq);
		return;
	}
	if (strcmp(tcp_flag, "SYN ACK ") == 0 && download_stats.random_seq != 0 && (ntohl(tcp->ack_seq) - 1 ) == download_stats.random_seq)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((download_stats.tcpopenresponsetime),"%s.%06ld", s_now, (long) ts.tv_usec);
		download_stats.random_seq = ntohl(tcp->seq);
		return;
	}

	if (strcmp(tcp_flag, "PSH ACK ") == 0 && strncmp(nexthdr, "GET", 3) == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((download_stats.romtime),"%s.%06ld", s_now, (long) ts.tv_usec);
		download_stats.get_ack = ntohl(tcp->ack_seq);
		return;
	}
	if(strcmp(tcp_flag, "ACK ") == 0 && ntohl(tcp->seq) == download_stats.get_ack && download_stats.ack_seq == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		download_stats.ack_seq = ntohl(tcp->ack_seq);
		return;
	}
	if(strcmp(tcp_flag, "ACK ") == 0 && ntohl(tcp->ack_seq) == download_stats.ack_seq )
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (download_stats.first_data == 0)
		{
			sprintf((download_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
			char *val = strstr(nexthdr,"Content-Length");
			char *pch, *pchr;
			val += strlen("Content-Length: ");
			pch=strtok_r(val, " \r\n\t", &pchr);
			download_stats.test_bytes_received = atoi(pch);
			download_stats.first_data = 1;
		}
		ip = trace_get_ip(packet);
		if (ip != NULL)
		{
			download_stats.total_bytes_received += ntohs(ip->ip_len);
		}
		return;
	}
	if ( (strcmp(tcp_flag, "PSH ACK ") == 0 || strcmp(tcp_flag, "FIN PSH ACK ") == 0) &&  ntohl(tcp->ack_seq) == download_stats.ack_seq)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (download_stats.first_data == 0)
		{
			sprintf((download_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
			char *val = strstr(nexthdr,"Content-Length");
			char *pch, *pchr;
			val += strlen("Content-Length: ");
			pch=strtok_r(val, " \r\n\t", &pchr);
			download_stats.test_bytes_received = atoi(pch);
			download_stats.first_data = 1;
		}

		sprintf((download_stats.eomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
		ip = trace_get_ip(packet);
		if (ip != NULL)
		{
			download_stats.total_bytes_received += ntohs(ip->ip_len);
		}
		return;
	}
}

static void libtrace_cleanup(libtrace_t *trace, libtrace_packet_t *packet)
{
	if (trace)
		trace_destroy(trace);

	if (packet)
		trace_destroy_packet(packet);
}

static void set_download_stats()
{
	char buf[128];
	sprintf(buf,"cwmp.@downloaddiagnostic[0].ROMtime=%s", download_stats.romtime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@downloaddiagnostic[0].BOMtime=%s", download_stats.bomtime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@downloaddiagnostic[0].EOMtime=%s", download_stats.eomtime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@downloaddiagnostic[0].TCPOpenRequestTime=%s", download_stats.tcpopenrequesttime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@downloaddiagnostic[0].TCPOpenResponseTime=%s", download_stats.tcpopenresponsetime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@downloaddiagnostic[0].TestBytesReceived=%d", download_stats.test_bytes_received);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@downloaddiagnostic[0].TotalBytesReceived=%lu", download_stats.total_bytes_received);
	uci_set_state_value(buf);
}

static void set_upload_stats()
{
	
	char buf[128];
	sprintf(buf,"cwmp.@uploaddiagnostic[0].ROMtime=%s", upload_stats.romtime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@uploaddiagnostic[0].BOMtime=%s", upload_stats.bomtime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@uploaddiagnostic[0].EOMtime=%s", upload_stats.eomtime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@uploaddiagnostic[0].TCPOpenRequestTime=%s", upload_stats.tcpopenrequesttime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@uploaddiagnostic[0].TCPOpenResponseTime=%s", upload_stats.tcpopenresponsetime);
	uci_set_state_value(buf);
	sprintf(buf,"cwmp.@uploaddiagnostic[0].TotalBytesSent=%lu", upload_stats.total_bytes_sent);
	uci_set_state_value(buf);
}

static void http_upload_per_packet(libtrace_packet_t *packet)
{
	struct tm lt;
	struct timeval ts;
	libtrace_tcp_t *tcp;
	//uint32_t remaining;
	char tcp_flag[16] = "";
	char *nexthdr;

	libtrace_ip_t *ip;
	char s_now[default_date_size];
	uint8_t proto;
	uint32_t remaining;
	tcp = trace_get_transport(packet, &proto, &remaining);
	if (tcp == NULL)
	{
		return;
	}
	else
	{
		nexthdr = trace_get_payload_from_tcp(tcp, &remaining);
	}
	if (tcp->ecn_ns) strcat(tcp_flag, "ECN_NS ");
	if (tcp->cwr) strcat(tcp_flag, "CWR ");
	if (tcp->ece) strcat(tcp_flag, "ECE ");
	if (tcp->fin) strcat(tcp_flag, "FIN ");
	if (tcp->syn) strcat(tcp_flag, "SYN ");
	if (tcp->rst) strcat(tcp_flag, "RST ");
	if (tcp->psh) strcat(tcp_flag, "PSH ");
	if (tcp->ack) strcat(tcp_flag, "ACK ");
	if (tcp->urg) strcat(tcp_flag, "URG ");
	if (strcmp(tcp_flag, "SYN ") == 0 && download_stats.random_seq == 0)
	{
    	ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((upload_stats.tcpopenrequesttime),"%s.%06ld", s_now, (long) ts.tv_usec);
		upload_stats.random_seq = ntohl(tcp->seq);
	}
	if (strcmp(tcp_flag, "SYN ACK ") == 0 && upload_stats.random_seq != 0 && (ntohl(tcp->ack_seq) - 1 ) == upload_stats.random_seq)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((upload_stats.tcpopenresponsetime),"%s.%06ld", s_now, (long) ts.tv_usec);
		upload_stats.random_seq = ntohl(tcp->seq);
	}
	if (strcmp(tcp_flag, "PSH ACK ") == 0 && strncmp(nexthdr, "PUT", 3) == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((upload_stats.romtime),"%s.%06ld", s_now, (long) ts.tv_usec);
		if (strstr(nexthdr, "Expect: 100-continue"))
		{
			upload_stats.tmp=1;
			upload_stats.ack_seq = ntohl(tcp->ack_seq);			
		}
		else
			upload_stats.ack_seq = ntohl(tcp->ack_seq);
		return;
	}
	if (strcmp(tcp_flag, "PSH ACK ") == 0 && upload_stats.tmp == 1 && strstr(nexthdr, "100 Continue"))
	{
		upload_stats.tmp = 2;
		upload_stats.ack_seq = ntohl(tcp->ack_seq);
		return;
	}

	if (strcmp(tcp_flag, "ACK ") == 0 && upload_stats.tmp == 2 && ntohl(tcp->seq) == upload_stats.ack_seq)
	{
		upload_stats.tmp = 0;
		upload_stats.ack_seq = ntohl(tcp->ack_seq);
		return;
	}

	if(strcmp(tcp_flag, "ACK ") == 0 && ntohl(tcp->ack_seq) == upload_stats.ack_seq && upload_stats.tmp == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (upload_stats.first_data == 0)
		{
			sprintf((upload_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
			upload_stats.first_data = 1;
		}
		ip = trace_get_ip(packet);
		if (ip != NULL)
			upload_stats.total_bytes_sent += ntohs(ip->ip_len);
	}
	if ( (strcmp(tcp_flag, "PSH ACK ") == 0 || strcmp(tcp_flag, "FIN PSH ACK ") == 0) &&  ntohl(tcp->ack_seq) == upload_stats.ack_seq && upload_stats.tmp == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (upload_stats.first_data == 0)
		{

			sprintf((upload_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
			upload_stats.first_data = 1;
		}
		ip = trace_get_ip(packet);
		if (ip != NULL)
			upload_stats.total_bytes_sent += ntohs(ip->ip_len);
	}
	if ( (strcmp(tcp_flag, "PSH ACK ") == 0 || strcmp(tcp_flag, "FIN PSH ACK ") == 0) &&  ntohl(tcp->seq) == upload_stats.ack_seq && upload_stats.tmp == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((upload_stats.eomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
		ip = trace_get_ip(packet);
	}
}

static void ftp_upload_per_packet(libtrace_packet_t *packet)
{
struct tm lt;
	struct timeval ts;
	libtrace_tcp_t *tcp;
	uint8_t proto;
	uint32_t remaining;
	char tcp_flag[16] = "";
	char *nexthdr;
	libtrace_ip_t *ip;
	char s_now[default_date_size];
	tcp = trace_get_transport(packet, &proto, &remaining);
	if (tcp == NULL)
	{
		return;
	}
	else
	{
		nexthdr = trace_get_payload_from_tcp(tcp, &remaining);
	}

	if (tcp->ecn_ns) strcat(tcp_flag, "ECN_NS ");
	if (tcp->cwr) strcat(tcp_flag, "CWR ");
	if (tcp->ece) strcat(tcp_flag, "ECE ");
	if (tcp->fin) strcat(tcp_flag, "FIN ");
	if (tcp->syn) strcat(tcp_flag, "SYN ");
	if (tcp->rst) strcat(tcp_flag, "RST ");
	if (tcp->psh) strcat(tcp_flag, "PSH ");
	if (tcp->ack) strcat(tcp_flag, "ACK ");
	if (tcp->urg) strcat(tcp_flag, "URG ");
	if(strcmp(tcp_flag, "PSH ACK ") == 0 && strlen(nexthdr) > strlen(FTP_PASV_RESPONSE)
			&& strncmp(nexthdr, FTP_PASV_RESPONSE, strlen(FTP_PASV_RESPONSE)) == 0)
	{
		upload_stats.ftp_syn = 1;
		return;
	}
    if (strcmp(tcp_flag, "SYN ") == 0 && upload_stats.ftp_syn == 1)
	{
    	ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		upload_stats.random_seq = ntohl(tcp->seq);
		sprintf((upload_stats.tcpopenrequesttime),"%s.%06ld", s_now, (long) ts.tv_usec);
	}
	if (strcmp(tcp_flag, "SYN ACK ") == 0 && upload_stats.random_seq != 0 && (ntohl(tcp->ack_seq) - 1 ) == upload_stats.random_seq)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((upload_stats.tcpopenresponsetime),"%s.%06ld", s_now, (long) ts.tv_usec);
		upload_stats.random_seq = ntohl(tcp->ack_seq);
	}

	if (strcmp(tcp_flag, "PSH ACK ") == 0 && strlen(nexthdr) > strlen(FTP_STOR_REQUEST) && strncmp(nexthdr, FTP_STOR_REQUEST, strlen(FTP_STOR_REQUEST)) == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((upload_stats.romtime),"%s.%06ld", s_now, (long) ts.tv_usec);
	}
	if(strcmp(tcp_flag, "ACK ") == 0 && ntohl(tcp->seq) == upload_stats.random_seq && upload_stats.ack_seq == 0)
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		upload_stats.ack_seq = ntohl(tcp->ack_seq);
		return;
	}
	if(strcmp(tcp_flag, "ACK ") == 0 && ntohl(tcp->ack_seq) == upload_stats.ack_seq )
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (upload_stats.first_data == 0)
		{
			sprintf((upload_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
			upload_stats.first_data = 1;
		}

		ip = trace_get_ip(packet);
		if (ip != NULL)
			upload_stats.total_bytes_sent += ntohs(ip->ip_len);
	}
	if ( (strcmp(tcp_flag, "PSH ACK ") == 0 || strcmp(tcp_flag, "FIN PSH ACK ") == 0) &&  ntohl(tcp->ack_seq) == upload_stats.ack_seq) //&& strlen(nexthdr) > 16 && strncmp(nexthdr, "HTTP/1.1 200 OK", 16) == 0
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		if (upload_stats.first_data == 0)
		{
			sprintf((upload_stats.bomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
			upload_stats.first_data = 1;
		}
		ip = trace_get_ip(packet);
		if (ip != NULL)
			upload_stats.total_bytes_sent += ntohs(ip->ip_len);
	}
	if ( (strcmp(tcp_flag, "PSH ACK ") == 0 || strcmp(tcp_flag, "FIN PSH ACK ") == 0) &&  strlen(nexthdr) > strlen(FTP_TRANSFERT_COMPLETE) && strncmp(nexthdr, FTP_TRANSFERT_COMPLETE, strlen(FTP_TRANSFERT_COMPLETE)) == 0) //&& strlen(nexthdr) > 16 && strncmp(nexthdr, "HTTP/1.1 200 OK", 16) == 0
	{
		ts = trace_get_timeval(packet);
		(void) localtime_r(&(ts.tv_sec), &lt);
		strftime(s_now, sizeof s_now, "%Y-%m-%dT%H:%M:%S", &lt);
		sprintf((upload_stats.eomtime),"%s.%06ld", s_now, (long) ts.tv_usec);
		read_next = 0;
	}
}

int extract_stats(char *dump_file, int proto, int diagnostic_type)
{
	libtrace_t *trace = NULL;
	libtrace_packet_t *packet = NULL;
	read_next = 1;
	int first_packet = 1;
	packet = trace_create_packet();
	if (packet == NULL) {
		perror("Creating libtrace packet");
		libtrace_cleanup(trace, packet);
		return 1;
	}
	trace = trace_create(dump_file);
	if (!trace) {
		return -1;
	}
	if (trace_is_err(trace)) {
		trace_perror(trace,"Opening trace file");
		libtrace_cleanup(trace, packet);
		return 1;
	}
	if (trace_start(trace) == -1) {
		trace_perror(trace,"Starting trace");
		libtrace_cleanup(trace, packet);
		return 1;
	}
	if (proto == DOWNLOAD_DIAGNOSTIC_HTTP && diagnostic_type == DOWNLOAD_DIAGNOSTIC)
	{
		while (trace_read_packet(trace,packet)>0 && read_next == 1) {
			http_download_per_packet(packet, first_packet);
			first_packet = 0;
			continue;
		}
		set_download_stats();
	}
	else if (proto == DOWNLOAD_DIAGNOSTIC_FTP && diagnostic_type == DOWNLOAD_DIAGNOSTIC)
	{
		while (trace_read_packet(trace,packet)>0 && read_next == 1) {
			ftp_download_per_packet(packet, first_packet);
			first_packet = 0;
			continue;
		}
		set_download_stats();
	}
	else if (proto == DOWNLOAD_DIAGNOSTIC_HTTP && diagnostic_type == UPLOAD_DIAGNOSTIC)
	{
		while (trace_read_packet(trace,packet)>0 && read_next == 1) {
			http_upload_per_packet(packet);
			first_packet = 0;
			continue;
		}
		set_upload_stats();
	}
	else
	{
		while (trace_read_packet(trace,packet)>0 && read_next == 1) {
			ftp_upload_per_packet(packet);
			first_packet = 0;
			continue;
		}
		set_upload_stats();
	}
	libtrace_cleanup(trace, packet);
	return 0;
}

int get_default_gateway_device( char **gw )
{
    FILE *f;
    char line[100] , *p , *c, *saveptr;

    f = fopen("/proc/net/route" , "r");
	if (f != NULL)
	{
		while(fgets(line , 100 , f))
		{
			p = strtok_r(line , " \t", &saveptr);
			c = strtok_r(NULL , " \t", &saveptr);
			if(p!=NULL && c!=NULL)
			{
				if(strcmp(c , "00000000") == 0)
				{
					asprintf(gw, "%s", p);
					fclose(f);
					return 0;
				}
			}
		}
		fclose(f);
	}
    return -1;
}

int cwmp_ip_ping_diagnostic() 
{
	dmcmd_no_wait("/bin/sh", 2, FUNCTION_PATH, "run");
    return 0;	
}

int cwmp_start_diagnostic(int diagnostic_type)
{
	char *url = NULL;
	char *interface = NULL;
	char *size = NULL;
	int error;
	char *status;
	if (diagnostic_type == DOWNLOAD_DIAGNOSTIC)
		uci_get_state_value("cwmp.@downloaddiagnostic[0].url", &url);
	else
	{
		uci_get_state_value("cwmp.@uploaddiagnostic[0].url", &url);
		uci_get_state_value("cwmp.@uploaddiagnostic[0].TestFileLength", &size);
	}

	CWMP_LOG(DEBUG,"diagnostic url %s", url);
	uci_get_state_value("cwmp.@downloaddiagnostic[0].interface", &interface);
	if( url == NULL || ((url != NULL) && (strcmp(url,"")==0))
		|| (strncmp(url,DOWNLOAD_PROTOCOL_FTP,strlen(DOWNLOAD_PROTOCOL_FTP))!=0) &&
		(strstr(url,"@") != NULL && strncmp(url,DOWNLOAD_PROTOCOL_HTTP,strlen(DOWNLOAD_PROTOCOL_HTTP)) == 0)
	)
	{
		CWMP_LOG(ERROR,"Invalid URL %s", url);
		return -1;
	}
	if ( interface == NULL || interface[0] == '\0' )
	{
		error = get_default_gateway_device(&interface);
		if (error == -1)
		{
			CWMP_LOG(ERROR,"Interface value: Empty");
			return -1;
		}
	}
	if (diagnostic_type == DOWNLOAD_DIAGNOSTIC)
	{
		dmcmd("/bin/sh", 4, DOWNLOAD_DIAGNOSTIC_PATH, "run", url, interface);
		uci_get_state_value("cwmp.@downloaddiagnostic[0].DiagnosticState", &status);
		if (status && strcmp(status, "Completed") == 0)
		{
			init_download_stats();
			CWMP_LOG(INFO,"Extract download stats");
			if(strncmp(url,DOWNLOAD_PROTOCOL_HTTP,strlen(DOWNLOAD_PROTOCOL_HTTP)) == 0)
				extract_stats(DOWNLOAD_DUMP_FILE, DOWNLOAD_DIAGNOSTIC_HTTP, DOWNLOAD_DIAGNOSTIC);
			if(strncmp(url,DOWNLOAD_PROTOCOL_FTP,strlen(DOWNLOAD_PROTOCOL_FTP)) == 0)
				extract_stats(DOWNLOAD_DUMP_FILE, DOWNLOAD_DIAGNOSTIC_FTP, DOWNLOAD_DIAGNOSTIC);
			cwmp_root_cause_event_ipdiagnostic(&cwmp_main);
		}
		else if (status && strncmp(status, "Error_", strlen("Error_")) == 0)
			cwmp_root_cause_event_ipdiagnostic(&cwmp_main);
		free(status);
	}
	else
	{
		dmcmd("/bin/sh", 5, UPLOAD_DIAGNOSTIC_PATH, "run", url, interface, size);
		uci_get_state_value("cwmp.@uploaddiagnostic[0].DiagnosticState", &status);
		if (status && strcmp(status, "Completed") == 0)
		{
			init_upload_stats();
			CWMP_LOG(INFO,"Extract upload stats");
			if(strncmp(url,DOWNLOAD_PROTOCOL_HTTP,strlen(DOWNLOAD_PROTOCOL_HTTP)) == 0)
				extract_stats(UPLOAD_DUMP_FILE, DOWNLOAD_DIAGNOSTIC_HTTP, UPLOAD_DIAGNOSTIC);
			if(strncmp(url,DOWNLOAD_PROTOCOL_FTP,strlen(DOWNLOAD_PROTOCOL_FTP)) == 0)
				extract_stats(UPLOAD_DUMP_FILE, DOWNLOAD_DIAGNOSTIC_FTP, UPLOAD_DIAGNOSTIC);
			cwmp_root_cause_event_ipdiagnostic(&cwmp_main);
		}
		else if (status && strncmp(status, "Error_", strlen("Error_")) == 0)
			cwmp_root_cause_event_ipdiagnostic(&cwmp_main);
		free(status);
		free(size);
	}
	free(url);
	free(interface);
	return 0;
}

