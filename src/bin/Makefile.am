bin_PROGRAMS = cwmpd
CWMP_VERSION = 2.2

cwmpd_SOURCES =				\
	../inc/b64.h			\
	../b64.c				\
	../inc/backupSession.h	\
	../backupSession.c		\
	../config.c				\
	../connectionRequest.c	\
	../inc/cwmp.h			\
	../cwmp.c				\
	../	event.c				\
	../inc/external.h		\
	../external.c			\
	../inc/http.h			\
	../http.c				\
	../inc/log.h			\
	../log.c				\
	../inc/messages.h		\
	../netlink.c			\
	../time.c				\
	../inc/ubus.h			\
	../ubus.c				\
	../inc/xml.h			\
	../xml.c

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
	$(LIBFREECWMP_LIBS)	\
	$(LIBUCI_LIBS)		\
	$(LIBUBOX_LIBS)		\
	$(LIBUBUS_LIBS)		\
	$(MICROXML_LIBS)	\
	$(LIBCURL_LIBS)		\
	$(LIBZSTREAM_LIBS)	\
	$(LIBPTHREAD_LIBS)

CWMP_REVISION=$(shell svnversion $(PWD) -n|cut -f2 -d:)

cwmpd_CFLAGS+=-Iinc -DCWMP_VERSION=\"$(CWMP_VERSION)\"
cwmpd_LDFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\"

ifneq ($(CWMP_REVISION)_,_)
ifneq ($(CWMP_REVISION),exported)
	cwmpd_CFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
	cwmpd_LDFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
endif
endif


