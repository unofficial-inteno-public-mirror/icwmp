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

#ifndef _JSHN_H__
#define _JSHN_H__

int cwmp_handle_downloadFault(char *msg);
int cwmp_handle_uploadFault(char *msg);
int cwmp_handle_dustate_changeFault(char *msg);
int cwmp_handle_uninstallFault(char *msg);
int cwmp_handle_getParamValues(char *msg);
int cwmp_handle_setParamValues(char *msg);
int cwmp_handle_getParamNames(char *msg);
int cwmp_handle_getParamAttributes(char *msg);
int cwmp_handle_setParamAttributes(char *msg);
int cwmp_handle_addObject(char *msg);
int cwmp_handle_delObject(char *msg);

#endif /* _JSHN_H__ */
