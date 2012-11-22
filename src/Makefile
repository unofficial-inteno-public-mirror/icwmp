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
PCFLAGS+=-Iinc -DCWMP_VERSION=\"$(CWMP_VERSION)\" -DWITH_COOKIES -DWITH_DEV_DEBUG -DWITH_CWMP_DEBUG -DWITH_DM_DEBUG
PLDFLAGS+=-DCWMP_VERSION=\"$(CWMP_VERSION)\" -DWITH_COOKIES -DWITH_DEV_DEBUG -DWITH_CWMP_DEBUG -DWITH_DM_DEBUG -luci -lexpat -lpthread -lssl -lcurl

ifneq ($(CWMP_REVISION)_,_)
ifneq ($(CWMP_REVISION),exported)
	PCFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
	PLDFLAGS+=-DCWMP_REVISION=\"$(CWMP_REVISION)\"
endif
endif

CWMPD_OBJECT_FILES=cwmp.o stdsoap2.o soapC.o soapClient.o backupSession.o\
 log.o TransferComplete.o Reboot.o GetParameterNames.o GetParameterValues.o\
 SetParameterValues.o FactoryReset.o ScheduleInform.o connectionRequest.o dm.o\
 config.o Inform.o event.o GetRPCMethods_acs.o Fault_cpe.o GetRPCMethods_cpe.o\
 dm_rpc.o httpda.o md5evp.o cwmp_api.o GetParameterAttributes.o SetParameterAttributes.o\
 Download.o AddObject.o DeleteObject.o

all: libcwmp.$(SHLIB_EXT) cwmpd cwmp_value_change kcwmp


cwmp_lib.o: cwmp_lib.c
	$(CC) $(PCFLAGS) $(CFLAGS) $(FPIC) -c -o $@ $<

libcwmp.$(SHLIB_EXT): cwmp_lib.o
	$(LINK) $(SHLIB_FLAGS) -o $(SHLIB_FILE) $< $(LIBS)
	ln -sf $(SHLIB_FILE) $@

%.o: %.c
	$(CC) $(PCFLAGS) $(CFLAGS) -c -o $@ $^

cwmpd: $(CWMPD_OBJECT_FILES)
	$(CC) $(PLDFLAGS) $(LDFLAGS) -o $@ $^

cwmp_value_change: cwmp_value_change.o
	$(CC) $(LDFLAGS) $(PLDFLAGS) -o $@ $<


obj-m += kcwmp.o

kcwmp:
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS) modules

clean:
	rm -f *.o *.so* cwmpd  cwmp_value_change
	$(MAKE) -C $(LINUX_DIR) $(MAKE_OPTS) clean
