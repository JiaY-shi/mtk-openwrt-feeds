include $(TOPDIR)/rules.mk

PKG_NAME:=ephy-utils
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/ephy-utils
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  SUBMENU:=Misc
  TITLE:=Ethernet Phy Utilities
  VERSION:=$(PKG_RELEASE)-$(REVISION)
  DEPENDS:=+mii_mgr +switch
endef

define Package/ephy-utils/description
  MediaTek Rebb ethernet phy utilities
endef

define Build/Compile/Default
endef

Build/Compile = $(Build/Compile/Default)

define Package/ephy-utils/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/sbin/ephy-utils

	cp ./files/ephy-utils.init $(1)/etc/init.d/ephy-utils
	cp -r ./files/*.sh $(1)/usr/sbin/ephy-utils/
	cp ./files/mtk_vct $(1)/usr/sbin/mtk_vct
endef

$(eval $(call BuildPackage,ephy-utils))
