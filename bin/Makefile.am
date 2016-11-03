bin_PROGRAMS = icwmpd
CWMP_VERSION = 3.0

icwmpd_SOURCES =			\
	../backupSession.c		\
	../config.c				\
	../cwmp.c				\
	../ipping.c				\
	../digestauth.c			\
	../event.c				\
	../external.c			\
	../http.c				\
	../jshn.c				\
	../log.c				\
	../md5.c				\
	../netlink.c			\
	../time.c				\
	../ubus.c				\
	../xml.c				\
	../zlib.c				\
	../dm/dmcwmp.c			\
	../dm/dmentry.c			\
	../dm/dmmem.c			\
	../dm/dmubus.c			\
	../dm/dmuci.c			\
	../dm/dmcommon.c		\
	../dm/wepkey.c			\
	../dm/dmtree/common/deviceinfo.c				\
	../dm/dmtree/common/managementserver.c			\
	../dm/dmtree/common/root.c						\
	../dm/dmtree/common/times.c						\
	../dm/dmtree/common/upnp.c						\
	../dm/dmtree/common/voice_services.c			\
	../dm/dmtree/common/x_inteno_se_ice.c			\
	../dm/dmtree/common/x_inteno_se_igmp.c			\
	../dm/dmtree/common/x_inteno_se_ipacccfg.c		\
	../dm/dmtree/common/x_inteno_se_logincfg.c		\
	../dm/dmtree/common/x_inteno_se_power_mgmt.c	\
	../dm/dmtree/common/softwaremodules.c			\
	../dm/dmtree/common/x_inteno_se_owsd.c			\
	../dm/dmtree/common/x_inteno_syslog.c			

if XMPP_ENABLE
icwmpd_SOURCES += 									\
	../dm/dmtree/common/xmpp.c						\
	../xmpp_cr.c					
endif

if DATAMODEL_TR098
icwmpd_SOURCES += 									\
	../dm/dmtree/tr098/lan_interfaces.c				\
	../dm/dmtree/tr098/landevice.c					\
	../dm/dmtree/tr098/layer_2_bridging.c			\
	../dm/dmtree/tr098/wandevice.c					\
	../dm/dmtree/tr098/x_inteno_se_wifi.c			\
	../dm/dmtree/tr098/ippingdiagnostics.c			\	
	../dm/dmtree/tr098/layer_3_forwarding.c	
endif
if DATAMODEL_TR181
icwmpd_SOURCES +=									\
	../dm/dmtree/tr181/wifi.c						\
	../dm/dmtree/tr181/ethernet.c					\
	../dm/dmtree/tr181/wan.c						\
	../dm/dmtree/tr181/bridging.c					\
	../dm/dmtree/tr181/hosts.c						\
	../dm/dmtree/tr181/dhcp.c						\
	../dm/dmtree/tr181/ip.c							\
	../dm/dmtree/tr181/ppp.c						\
	../dm/dmtree/tr181/nat.c						\
	../dm/dmtree/tr181/routing.c
endif
icwmpd_CFLAGS =			\
	$(AM_CFLAGS)		\
	$(LIBUCI_CFLAGS)	\
	$(LIBUBOX_CFLAGS)	\
	$(LIBUBUS_CFLAGS)	\
	$(MICROXML_CFLAGS)	\
	$(LIBCURL_CFLAGS)	\
	$(LIBZSTREAM_CFLAGS)

icwmpd_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	$(LIBUCI_LDFLAGS)	\
	$(LIBUBOX_LDFLAGS)	\
	$(LIBUBUS_LDFLAGS)	\
	$(MICROXML_LDFLAGS)	\
	$(LIBCURL_LDFLAGS)	\
	$(LIBZSTREAM_LDFLAGS)

icwmpd_LDADD =			\
	$(AM_LIBS)			\
	$(LIBUCI_LIBS)		\
	$(LIBUBOX_LIBS)		\
	$(LIBUBUS_LIBS)		\
	$(MICROXML_LIBS)	\
	$(LIBCURL_LIBS)		\
	$(LIBZSTREAM_LIBS)	\
	$(LIBPTHREAD_LIBS)	\
	$(LCRYPTO_LIBS)		\
	$(LSSL_LIBS)		\
	$(LIBJSON_LIBS)		\
	$(LBLOBMSG_LIBS)	\
	$(LIBZ_LIBS)		\
	$(LIBM_LIBS)

icwmpd_CFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\"
icwmpd_LDFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\"

CWMP_BKP_FILE=/etc/icwmpd/.icwmpd_backup_session.xml
icwmpd_CFLAGS+=-DCWMP_BKP_FILE=\"$(CWMP_BKP_FILE)\"
icwmpd_CFLAGS+=-I../inc
icwmpd_CFLAGS+=-I../dm/
icwmpd_CFLAGS+=-I../dm/dmtree/
icwmpd_CFLAGS+=-I../dm/dmtree/common
icwmpd_CFLAGS+=-I../dm/dmtree/tr098
icwmpd_CFLAGS+=-I../dm/dmtree/tr181
