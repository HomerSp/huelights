include $(TOPDIR)/rules.mk

PKG_NAME:=huelights
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/huelights-$(PKG_VERSION)

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/huelights
	SECTION:=net
	CATEGORY:=Network
	TITLE:=Philips Hue lights utility
	DEPENDS:=+libcurl +libjson-c +libstdcpp
endef

define Package/huelights/description
	Philips Hue lights utility
	Manage Philips Hue lights from your router.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/huelights/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/huelights $(1)/usr/sbin/

	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/huelights.init $(1)/etc/init.d/huelights
endef

define Host/Prepare
	mkdir -p $(HOST_BUILD_DIR)
	$(CP) ./src/* $(HOST_BUILD_DIR)/
endef

define Host/Install
	
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,huelights))
