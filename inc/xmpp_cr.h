/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2013 Inteno Broadband Technology AB
 *	  Author Mohamed Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Ahmed Zribi <ahmed.zribi@pivasoftware.com>
 *
 */

#ifndef _XMPP_CR_H__
#define _XMPP_CR_H__

#include <stdio.h>
#include <stdlib.h>
#ifdef XMPP_ENABLE
#include <strophe.h>
#endif

#define DEFAULT_XMPP_RECONNECTION_RETRY			5
void cwmp_xmpp_connect_client();
void cwmp_xmpp_exit();

#endif /* _XMPP_CR_H__ */
