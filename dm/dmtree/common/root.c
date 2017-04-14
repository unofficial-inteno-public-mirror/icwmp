/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Copyright (C) 2012-2016 PIVA SOFTWARE (www.pivasoftware.com)
 *		Author: Imen Bhiri <imen.bhiri@pivasoftware.com>
 *		Author: Anis Ellouze <anis.ellouze@pivasoftware.com>
 */

#include "dmuci.h"
#include "dmcwmp.h"
#include "root.h"
#include "deviceinfo.h"
#include "managementserver.h"
#include "times.h"
#include "upnp.h"
#include "voice_services.h"
#include "x_inteno_se_ice.h"
#include "x_inteno_se_igmp.h"
#include "x_inteno_se_ipacccfg.h"
#include "x_inteno_se_logincfg.h"
#include "x_inteno_se_power_mgmt.h"
#include "x_inteno_syslog.h"
#include "softwaremodules.h"
#include "xmpp.h"
#include "x_inteno_se_owsd.h"
#include "x_inteno_se_dropbear.h"
#include "x_inteno_se_buttons.h"
#ifdef DATAMODEL_TR181
#include "ip.h"
#include "ethernet.h"
#include "bridging.h"
#include "wifi.h"
#include "wan.h"
#include "dhcp.h"
#include "hosts.h"
#include "nat.h"
#include "ppp.h"
#include "routing.h"
#endif
#ifdef DATAMODEL_TR098
#include "landevice.h"
#include "wandevice.h"
#include "ippingdiagnostics.h"
#include "lan_interfaces.h"
#include "layer_3_forwarding.h"
#include "x_inteno_se_wifi.h"
#include "layer_2_bridging.h"
#include "downloaddiagnostic.h"
#include "uploaddiagnostic.h"
#endif

/* *** CWMP *** */
DMOBJ tEntryObj[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, NOTIFICATION, nextobj, leaf, linker*/
{(char *)&DMROOT, &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE, tRootObj, NULL, NULL},
{0}
};

DMOBJ tRootObj[] = {
/* OBJ permission, addobj, delobj, browseinstobj, finform, nextobj, leaf, notification, linker*/
{"DeviceInfo", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,tDeviceInfoObj, tDeviceInfoParams, NULL},
{"ManagementServer", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tManagementServerParams, NULL},
{"Time", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tTimeParams, NULL},
{"UPnP", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,tUPnPObj, NULL, NULL},
{"Services", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,tServiceObj, NULL, NULL},
{"X_INTENO_SE_ICE", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_IceParam, NULL},
{"X_INTENO_SE_IGMP", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_IgmpParam, NULL},
{"X_INTENO_SE_IpAccCfg", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,tSe_IpAccObj, NULL, NULL},
{"X_INTENO_SE_LoginCfg", &DMREAD, NULL, NULL, NULL, NULL,NULL, &DMNONE,NULL, tSe_LoginCfgParam, NULL},
{"X_INTENO_SE_PowerManagement", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_PowerManagementParam, NULL},
{"X_INTENO_SE_SyslogCfg", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_SyslogCfgParam, NULL},
{"SoftwareModules", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,tSoftwareModulesObj, NULL, NULL},
{"X_INTENO_SE_Owsd", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,XIntenoSeOwsdObj, XIntenoSeOwsdParams, NULL},
{"X_INTENO_SE_Dropbear", &DMWRITE, add_dropbear_instance, delete_dropbear_instance, NULL, browseXIntenoDropbear, NULL, &DMNONE, NULL, X_INTENO_SE_DropbearParams, NULL},
{"X_INTENO_SE_Buttons", &DMREAD, NULL, NULL, NULL, browseXIntenoButton, NULL, &DMNONE, NULL, X_INTENO_SE_ButtonParams, NULL},
#ifdef DATAMODEL_TR098
{"LANDevice", &DMREAD, NULL, NULL, NULL, browselandeviceInst, &DMFINFRM, &DMNONE,tLANDeviceObj, tLANDeviceParam, NULL},
{"WANDevice", &DMREAD, NULL, NULL, NULL, browsewandeviceInst, &DMFINFRM, &DMWANConnectionDevicenotif,tWANDeviceObj, tWANDeviceParam, NULL},
{"LANInterfaces", &DMREAD, NULL, NULL, check_laninterfaces, NULL, &DMFINFRM, &DMNONE,tLANInterfacesObj, tLANInterfacesParam, NULL},
{"IPPingDiagnostics", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tIPPingDiagnosticsParam, NULL},
{"Layer3Forwarding", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,tLayer3ForwardingObj, NULL, NULL},
{"X_INTENO_SE_Wifi", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,tsewifiObj, NULL, NULL},
{"DownloadDiagnostics", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tDownloadDiagnosticsParam, NULL},
{"UploadDiagnostics", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tUploadDiagnosticsParam, NULL},

#endif
#ifdef XMPP_ENABLE
{"XMPP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL,tXMPPObj, tXMPPParams, NULL},
#endif
#ifdef DATAMODEL_TR181
{"Bridging",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tBridgObj, NULL, NULL},
{"WiFi",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tWifiObj, NULL, NULL},
{"IP",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tIPObj, NULL, NULL},
{"Ethernet", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tEthernetObj, NULL, NULL},
{"DSL",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tDslObj, NULL, NULL},
{"ATM",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tAtmObj, NULL, NULL},
{"PTM", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tPtmObj, NULL, NULL},
{"DHCPv4", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tDhcpServerObj, NULL, NULL},
{"Hosts", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, thostsObj, thostsParam, NULL},
{"NAT", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tnatObj, NULL, NULL},
{"PPP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tpppObj, NULL, NULL},
{"Routing", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tRoutingObj, NULL, NULL},
#endif
{0}
};

/*** UPNP ***/
DMOBJ tEntryObjUPNP[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, NOTIFICATION, nextobj, leaf, linker*/
{(char *)&DMROOT, &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE, tRootObjUPNP, NULL, NULL},
{0}
};

DMOBJ tRootObjUPNP[] = {
/* OBJ, permission, addobj, delobj, browseinstobj, finform, NOTIFICATION, nextobj, leaf, linker*/
{"BBF", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE, tRootObjUPNPBBF, NULL, NULL},
{0}
};

DMOBJ tRootObjUPNPBBF[] = {
/* OBJ permission, addobj, delobj, browseinstobj, finform, nextobj, leaf, notification, linker*/
{"DeviceInfo", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,tDeviceInfoObj, tDeviceInfoParams, NULL},
{"ManagementServer", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tManagementServerParams, NULL},
{"Time", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tTimeParams, NULL},
{"UPnP", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,tUPnPObj, NULL, NULL},
{"VoiceService", &DMREAD, NULL, NULL, NULL, browseVoiceServiceInst, NULL, NULL, tVoiceServiceObj, tVoiceServiceParam, NULL},
{"X_INTENO_SE_ICE", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_IceParam, NULL},
{"X_INTENO_SE_IGMP", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_IgmpParam, NULL},
{"X_INTENO_SE_IpAccCfg", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,tSe_IpAccObj, NULL, NULL},
{"X_INTENO_SE_LoginCfg", &DMREAD, NULL, NULL, NULL, NULL,NULL, &DMNONE,NULL, tSe_LoginCfgParam, NULL},
{"X_INTENO_SE_PowerManagement", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_PowerManagementParam, NULL},
{"X_INTENO_SE_SyslogCfg", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,NULL, tSe_SyslogCfgParam, NULL},
{"SoftwareModules", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,tSoftwareModulesObj, NULL, NULL},
{"X_INTENO_SE_Owsd", &DMREAD, NULL, NULL, NULL, NULL, NULL, &DMNONE,XIntenoSeOwsdObj, XIntenoSeOwsdParams, NULL},
{"X_INTENO_SE_Dropbear", &DMWRITE, add_dropbear_instance, delete_dropbear_instance, NULL, browseXIntenoDropbear, NULL, &DMNONE, NULL, X_INTENO_SE_DropbearParams, NULL},
{"X_INTENO_SE_Buttons", &DMREAD, NULL, NULL, NULL, browseXIntenoButton, NULL, &DMNONE, NULL, X_INTENO_SE_ButtonParams, NULL},
#ifdef DATAMODEL_TR098
{"LANDevice", &DMREAD, NULL, NULL, NULL, browselandeviceInst, &DMFINFRM, &DMNONE,tLANDeviceObj, tLANDeviceParam, NULL},
{"WANDevice", &DMREAD, NULL, NULL, NULL, browsewandeviceInst, &DMFINFRM, &DMWANConnectionDevicenotif,tWANDeviceObj, tWANDeviceParam, NULL},
{"LANInterfaces", &DMREAD, NULL, NULL, check_laninterfaces, NULL, &DMFINFRM, &DMNONE,tLANInterfacesObj, tLANInterfacesParam, NULL},
{"IPPingDiagnostics", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tIPPingDiagnosticsParam, NULL},
{"Layer3Forwarding", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,tLayer3ForwardingObj, NULL, NULL},
{"X_INTENO_SE_Wifi", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,tsewifiObj, NULL, NULL},
{"DownloadDiagnostics", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tDownloadDiagnosticsParam, NULL},
{"UploadDiagnostics", &DMREAD, NULL, NULL, NULL, NULL, &DMFINFRM, &DMNONE,NULL, tUploadDiagnosticsParam, NULL},

#endif
#ifdef XMPP_ENABLE
{"XMPP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL,tXMPPObj, tXMPPParams, NULL},
#endif
#ifdef DATAMODEL_TR181
{"Bridging",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tBridgObj, NULL, NULL},
{"WiFi",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tWifiObj, NULL, NULL},
{"IP",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tIPObj, NULL, NULL},
{"Ethernet", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tEthernetObj, NULL, NULL},
{"DSL",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tDslObj, NULL, NULL},
{"ATM",&DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tAtmObj, NULL, NULL},
{"PTM", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tPtmObj, NULL, NULL},
{"DHCPv4", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tDhcpServerObj, NULL, NULL},
{"Hosts", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, thostsObj, thostsParam, NULL},
{"NAT", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tnatObj, NULL, NULL},
{"PPP", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tpppObj, NULL, NULL},
{"Routing", &DMREAD, NULL, NULL, NULL, NULL, NULL, NULL, tRoutingObj, NULL, NULL},
#endif
{0}
};

