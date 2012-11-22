#
# Copyright (C) 2011 Inteno
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk
PKG_NAME:=cwmpd
PKG_RELEASE:=0
PKG_VERSION:=1.3
CWMP_BKP_FILE:=/etc/cwmpd/.cwmpd_backup_session.xml
include $(INCLUDE_DIR)/package.mk

define Package/cwmpd
  CATEGORY:=Utilities
  TITLE:=TR-069 client
  DEPENDS:=PACKAGE_libuci:libuci PACKAGE_libexpat:libexpat PACKAGE_libcurl:libcurl PACKAGE_libpthread:libpthread PACKAGE_libopenssl:libopenssl PACKAGE_libubus:libubus PACKAGE_libubox:libubox
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
	$(CP) $(PKG_BUILD_DIR)/scripts/freecwmp.sh $(1)/usr/sbin/freecwmp
	$(CP) $(PKG_BUILD_DIR)/scripts/defaults $(1)/usr/share/defaults
	$(INSTALL_DIR) $(1)/usr/share/freecwmp/functions
	$(CP) $(PKG_BUILD_DIR)/scripts/functions/* $(1)/usr/share/freecwmp/functions/
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
