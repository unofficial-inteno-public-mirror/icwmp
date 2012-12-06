#
# Copyright (C) 2011 Inteno
#
CWMP_VERSION=1.3
CWMP_REVISION=$(shell svnversion $(PWD) -n|cut -f2 -d:)

LINK=$(CC)
SHLIB_EXT=so
SHLIB_FLAGS=-shared -Wl,-soname,$(SHLIB_FILE)
FPIC=-fPIC

LIBS=-lc

SHLIB_FILE=libcwmp.$(SHLIB_EXT).$(CWMP_VERSION)
PCFLAGS+=-Iinc -DCWMP_VERSION=\"$(CWMP_VERSION)\" -DWITH_COOKIES -DWITH_CWMP_DEBUG -DWITH_DEV_DEBUG -DACS_MULTI=1
PLDFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\" -DWITH_COOKIES -DWITH_CWMP_DEBUG -DWITH_DEV_DEBUG  -luci -lmicroxml -lpthread -lssl -lcurl -lubox -lubus

ifneq ($(CWMP_REVISION)_,_)
ifneq ($(CWMP_REVISION),exported)
	PCFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
	PLDFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
endif
endif

CWMPD_OBJECT_FILES=cwmp.o stdsoap2.o soapC.o soapClient.o backupSession.o\
 log.o TransferComplete.o Reboot.o GetParameterNames.o GetParameterValues.o\
 SetParameterValues.o FactoryReset.o ScheduleInform.o connectionRequest.o \
 config.o Inform.o event.o GetRPCMethods_acs.o Fault_cpe.o GetRPCMethods_cpe.o\
 httpda.o md5evp.o GetParameterAttributes.o SetParameterAttributes.o\
 Download.o AddObject.o DeleteObject.o ubus.o external.o

all: cwmpd


%.o: %.c
	$(CC) $(PCFLAGS) $(CFLAGS) -c -o $@ $^

cwmpd: $(CWMPD_OBJECT_FILES)
	$(CC) $(PLDFLAGS) $(LDFLAGS) -o $@ $^



#obj-m += kcwmp.o

#kcwmp:
#	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS) modules

clean:
	rm -f *.o cwmpd
#	$(MAKE) -C $(LINUX_DIR) $(MAKE_OPTS) clean
