#ifndef __XMPP_H
#define __XMPP_H

struct connectionargs
{
	struct uci_section *connsection;
};

char *get_xmpp_server_enable(char *instance);
char *get_xmpp_username(char *instance);
char *get_xmpp_password(char *instance);
char *get_xmpp_domain(char *instance);
char *get_xmpp_resource(char *instance);
char *get_xmpp_keepalive_interval(char *instance);
char *get_xmpp_connect_attempts(char *instance);
char *get_xmpp_connect_initial_retry_interval(char *instance);
char *get_xmpp_connect_retry_interval_multiplier(char *instance);
char *get_xmpp_connect_retry_max_interval(char *instance);
extern DMOBJ tXMPPObj[];
extern DMLEAF tXMPPParams[];
extern DMLEAF tConnectionParams[];
#endif
