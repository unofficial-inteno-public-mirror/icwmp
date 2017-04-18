/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2015 Inteno Broadband Technology AB
 *	  Author MOHAMED Kallel <mohamed.kallel@pivasoftware.com>
 *	  Author Imen Bhiri <imen.bhiri@pivasoftware.com>
 *	  Author Feten Besbes <feten.besbes@pivasoftware.com>
 *
 */

#ifndef __ROOT_H
#define __ROOT_H

#include "dmcwmp.h"

extern DMOBJ tEntryObj[];
extern DMOBJ tRootObj[];
extern DMOBJ tEntryObjUPNP[];
extern DMOBJ tRootObjUPNP[];
extern DMOBJ tRootObjUPNPBBF[];
extern UPNP_SUPPORTED_DM tUPNPSupportedDM[];

#ifdef DATAMODEL_TR098
#define DMROOT_URI "urn:broadband-forum-org:tr-098-1-8-0"
#define DMROOT_URL "https://www.broadband-forum.org/cwmp/tr-098-1-8-0.html"
#define DMROOT_DESC "TR-098 InternetGatewayDevice:1 Root Object definition"
#endif
#ifdef DATAMODEL_TR181
#define DMROOT_URI "urn:broadband-forum-org:tr-181-2-11-0"
#define DMROOT_URL "https://www.broadband-forum.org/cwmp/tr-181-2-11-0.html"
#define DMROOT_DESC "TR-181 Device:2 Root Object definition"
#endif

#endif
