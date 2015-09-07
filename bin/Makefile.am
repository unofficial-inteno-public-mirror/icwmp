bin_PROGRAMS = cwmpd
CWMP_VERSION = 3.0

cwmpd_SOURCES =				\
	../backupSession.c		\
	../config.c				\
	../cwmp.c				\
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
	../dm/dmcwmp.c			\
	../dm/dmentry.c			\
	../dm/dmmem.c			\
	../dm/dmubus.c			\
	../dm/dmuci.c			\
	../dm/dmcommon.c		\
	../dm/wepkey.c			\
	../dm/dmtree/deviceinfo.c					\
	../dm/dmtree/lan_interfaces.c				\
	../dm/dmtree/landevice.c					\
	../dm/dmtree/layer_2_bridging.c				\
	../dm/dmtree/layer_3_forwarding.c			\
	../dm/dmtree/managementserver.c				\	
	../dm/dmtree/root.c							\
	../dm/dmtree/times.c						\
	../dm/dmtree/upnp.c							\
	../dm/dmtree/voice_services.c				\
	../dm/dmtree/wandevice.c					\
	../dm/dmtree/x_inteno_se_ice.c				\
	../dm/dmtree/x_inteno_se_igmp.c				\
	../dm/dmtree/x_inteno_se_ipacccfg.c			\
	../dm/dmtree/x_inteno_se_logincfg.c			\
	../dm/dmtree/x_inteno_se_power_mgmt.c		\
	../dm/dmtree/x_inteno_se_wifi.c			

cwmpd_CFLAGS =			\
	$(AM_CFLAGS)		\
	$(LIBUCI_CFLAGS)	\
	$(LIBUBOX_CFLAGS)	\
	$(LIBUBUS_CFLAGS)	\
	$(MICROXML_CFLAGS)	\
	$(LIBCURL_CFLAGS)	\
	$(LIBZSTREAM_CFLAGS)

cwmpd_LDFLAGS =			\
	$(AM_LDFLAGS)		\
	$(LIBUCI_LDFLAGS)	\
	$(LIBUBOX_LDFLAGS)	\
	$(LIBUBUS_LDFLAGS)	\
	$(MICROXML_LDFLAGS)	\
	$(LIBCURL_LDFLAGS)	\
	$(LIBZSTREAM_LDFLAGS)

cwmpd_LDADD =			\
	$(AM_LIBS)			\
	$(LIBUCI_LIBS)		\
	$(LIBUBOX_LIBS)		\
	$(LIBUBUS_LIBS)		\
	$(MICROXML_LIBS)	\
	$(LIBCURL_LIBS)		\
	$(LIBZSTREAM_LIBS)	\
	$(LIBPTHREAD_LIBS)	\
	$(LIBJSON_LIBS)		\
	$(LBLOBMSG_LIBS)

cwmpd_CFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\"
cwmpd_LDFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\"

CWMP_BKP_FILE=/etc/cwmpd/.cwmpd_backup_session.xml
cwmpd_CFLAGS+=-DCWMP_BKP_FILE=\"$(CWMP_BKP_FILE)\"
cwmpd_CFLAGS+=-I../inc
cwmpd_CFLAGS+=-I../dm/
cwmpd_CFLAGS+=-I../dm/dmtree