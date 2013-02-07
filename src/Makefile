#
# Copyright (C) 2011 Inteno
#
CWMP_VERSION=2.2
CWMP_REVISION=$(shell svnversion $(PWD) -n|cut -f2 -d:)

PCFLAGS+=-Iinc -DCWMP_VERSION=\"$(CWMP_VERSION)\" -DWITH_CWMP_DEBUG -DWITH_DEV_DEBUG -DACS_MULTI=1
PLDFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\" -DWITH_CWMP_DEBUG -DWITH_DEV_DEBUG  -luci -lmicroxml -lpthread -lssl -lcurl -lubox -lubus

ifneq ($(CWMP_REVISION)_,_)
ifneq ($(CWMP_REVISION),exported)
	PCFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
	PLDFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
endif
endif

CWMPD_OBJECT_FILES=cwmp.o backupSession.o log.o config.o event.o ubus.o external.o time.o xml.o http.o netlink.o b64.o

all: cwmpd


%.o: %.c
	$(CC) $(PCFLAGS) $(CFLAGS) -c -o $@ $^

cwmpd: $(CWMPD_OBJECT_FILES)
	$(CC) $(PLDFLAGS) $(LDFLAGS) -o $@ $^



clean:
	rm -f *.o cwmpd
