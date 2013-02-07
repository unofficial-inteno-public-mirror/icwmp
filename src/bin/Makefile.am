bin_PROGRAMS = cwmpd
CWMP_VERSION = 2.2

cwmpd_SOURCES =				\
	../b64.c				\
	../backupSession.c		\
	../config.c				\
	../cwmp.c				\
	../event.c				\
	../external.c			\
	../http.c				\
	../log.c				\
	../netlink.c			\
	../time.c				\
	../ubus.c				\
	../xml.c

cwmpd_CFLAGS =			\
	$(AM_CFLAGS)		\
	$(LIBUCI_CFLAGS)	\
	$(LIBUBOX_CFLAGS)	\
	$(LIBUBUS_CFLAGS)	\
	$(MICROXML_CFLAGS)	\
	$(LIBCURL_CFLAGS)	\
	$(LIBZSTREAM_CFLAGS) \
	-I../inc

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
	$(LIBFREECWMP_LIBS)	\
	$(LIBUCI_LIBS)		\
	$(LIBUBOX_LIBS)		\
	$(LIBUBUS_LIBS)		\
	$(MICROXML_LIBS)	\
	$(LIBCURL_LIBS)		\
	$(LIBZSTREAM_LIBS)	\
	$(LIBPTHREAD_LIBS)

CWMP_REVISION=$(shell svnversion $(PWD) -n|cut -f2 -d:)

cwmpd_CFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\"
cwmpd_LDFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\"

CWMP_BKP_FILE=/etc/cwmpd/.cwmpd_backup_session.xml
cwmpd_CFLAGS+=-DCWMP_BKP_FILE=\"$(CWMP_BKP_FILE)\"