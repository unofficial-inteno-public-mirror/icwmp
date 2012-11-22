#
# Copyright (C) 2011 Inteno
#


include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
PKG_NAME:=cwmpd
PKG_RELEASE:=0
PKG_VERSION:=1.3
CWMP_BKP_FILE:=/etc/cwmpd/.cwmpd_backup_session.xml
OTHER_MENU:=Other modules
include $(INCLUDE_DIR)/package.mk


define KernelPackage/kcwmp
  SUBMENU:=$(OTHER_MENU)
  TITLE:=Modules for TR-069 value change notification
  KCONFIG:=
  FILES:=$(PKG_BUILD_DIR)/kcwmp.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,1,kcwmp)
endef

define KernelPackage/kcwmp/description
 TR-069 kernel modules for value change notification
endef

EXTRA_KCONFIG:= \
    CONFIG_HELLO_MOD=m

EXTRA_CFLAGS:= \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \
    
MAKE_OPTS:= \
    ARCH="$(LINUX_KARCH)" \
    CROSS_COMPILE="$(TARGET_CROSS)" \
    SUBDIRS="$(PKG_BUILD_DIR)" \
    EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
    $(EXTRA_KCONFIG)

define Package/cwmpd
  CATEGORY:=Utilities
  TITLE:=TR-069 client
  DEPENDS:=PACKAGE_libuci:libuci PACKAGE_libexpat:libexpat PACKAGE_libcurl:libcurl PACKAGE_libpthread:libpthread PACKAGE_libopenssl:libopenssl
endef

define Package/libcwmp
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=C library for the TR-069 client
endef

define Package/cwmpd/description
	TR-069 client
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) LINUX_DIR=$(LINUX_DIR) MAKE_OPTS=$(MAKE_OPTS) LDFLAGS="-L$(STAGING_DIR)/usr/lib" CFLAGS="$(TARGET_CFLAGS) -I$(LINUX_DIR)/include -I$(STAGING_DIR)/usr/include -DCWMP_BKP_FILE=\\\"$(CWMP_BKP_FILE)\\\""
endef

define Package/libcwmp/install
	$(INSTALL_DIR) $(1)/lib
	$(CP) $(PKG_BUILD_DIR)/libcwmp.so* $(1)/lib/
endef

define Package/cwmpd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cwmpd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cwmp_value_change $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DATA) -m0644 $(PKG_BUILD_DIR)/config/cwmp $(1)/etc/config/
	$(INSTALL_DIR) $(1)/etc/cwmpd/dm
	$(INSTALL_DATA) -m0644 $(PKG_BUILD_DIR)/dm/xml/* $(1)/etc/cwmpd/dm
	$(INSTALL_DIR) $(1)/etc/cwmpd/scripts
	$(INSTALL_DATA) -m0755 $(PKG_BUILD_DIR)/dm/scripts/* $(1)/etc/cwmpd/scripts
	$(INSTALL_DATA) -m0755 $(PKG_BUILD_DIR)/init/iccu_enable $(1)/etc/cwmpd/scripts/iccu_enable
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DATA) -m0755 $(PKG_BUILD_DIR)/init/cwmpd.init $(1)/etc/init.d/cwmpd
	$(INSTALL_DATA) -m0755 $(PKG_BUILD_DIR)/init/iccu.init $(1)/etc/init.d/iccu
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/inc/cwmp_lib.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/inc/cwmp_kernel.h $(LINUX_DIR)/include/linux
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libcwmp.so* $(1)/usr/lib
endef

define Package/cwmpd/postinst
	#!/bin/sh
	echo "$(CWMP_BKP_FILE)" >> $${IPKG_INSTROOT}/etc/sysupgrade.conf
	echo "/etc/cwmpd/.iccu/cwmp" >> $${IPKG_INSTROOT}/etc/sysupgrade.conf
	if [ -z "$${IPKG_INSTROOT}" ]; then
		echo "Enabling rc.d symlink for cwmpd"
		/etc/init.d/cwmpd enable
		/etc/init.d/iccu enable
	fi
	exit 0
endef

define Package/cwmpd/prerm
	#!/bin/sh
	if [ -z "$${IPKG_INSTROOT}" ]; then
		echo "Disabling rc.d symlink for cwmpd"
		/etc/init.d/cwmpd disable
		/etc/init.d/iccu disable
	fi
	exit 0
endef

$(eval $(call BuildPackage,cwmpd))
$(eval $(call BuildPackage,libcwmp))
$(eval $(call KernelPackage,kcwmp))
