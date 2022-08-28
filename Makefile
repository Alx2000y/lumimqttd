include $(TOPDIR)/rules.mk

PKG_NPM_NAME:=lumimqttd
PKG_NAME:=$(PKG_NPM_NAME)
PKG_VERSION:=0.2.7
PKG_RELEASE:=1

PKG_MAINTAINER:=Alexey Sadkov <alx.v.sadkov@gmail.com>
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/lumimqttd-mpd
  SECTION:=Lumi
  CATEGORY:=Lumi
  TITLE:=Lumimqtt mpd
  DEFAULT:=y
  DEPENDS+=+alsa-lib +libmosquitto +libjson-c +libpthread +libmpdclient +libwebsockets-openssl \
  +LUMIMQTTD_BLE:bluez-libs
  VARIANT:=mpd
endef

define Package/lumimqttd-mpd/description
 Lumimqtt mpd sound
endef

define Package/lumimqttd
  SECTION:=Lumi
  CATEGORY:=Lumi
  TITLE:=Lumimqttd alsa
  DEFAULT:=y
  DEPENDS+=+alsa-lib +libmosquitto +libjson-c +libpthread +libmpg123 +libout123 +libwebsockets-openssl \
  +LUMIMQTTD_BLE:bluez-libs
  VARIANT:=alsa
endef
define Package/lumimqttd/config
    source "$(SOURCE)/Config.in"
endef

PKG_BUILD_DEPENDS:= mosquitto alsa-lib libjson-c mpg123 libwebsockets \
    $(if $(CONFIG_PACKAGE_lumimqttd-mpd),libmpdclient,) \
    $(if $(LUMIMQTTD_BLE),bluez-libs,)

define Package/lumimqttd/description
 Lumimqtt alsa sound
endef

TARGET_CPPFLAGS += -DVERSION=\\\"$(PKG_VERSION)\\\"
TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include
TARGET_LDLAGS += -L$(STAGING_DIR)/usr/lib
#TARGET_CFLAGS += -ggdb3
define Build/Configure
    $(call Build/Configure/Default,)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CPPFLAGS:= -I$(STAGING_DIR)/usr/include $(TARGET_CPPFLAGS) \
  $(if $(CONFIG_LUMIMQTTD_BLE),-DUSE_BLE,) \
  $(if $(CONFIG_LUMIMQTTD_CPUTEMP),-DUSE_CPUTEMP,)

ifeq ($(BUILD_VARIANT),mpd)
  TARGET_CPPFLAGS += -DUSE_MPD
  MAKE_FLAGS += USE_MPD=1

endif

MAKE_FLAGS += \
  $(if $(CONFIG_LUMIMQTTD_BLE),USE_BLE=1,) \
  $(if $(CONFIG_LUMIMQTTD_CPUTEMP),USE_CPUTEMP=1,) \
	CFLAGS="$(TARGET_CPPFLAGS) $(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS) -Wl,--gc-sections" 

define Package/lumimqttd/conffiles
/etc/lumimqttd.json
endef

define Package/lumimqttd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lumimqttd $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_CONF) ./files/lumimqttd.json $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lumimqttd.init $(1)/etc/init.d/lumimqttd
endef
Package/lumimqttd-mpd/install=$(Package/lumimqttd/install)

$(eval $(call BuildPackage,lumimqttd-mpd))
$(eval $(call BuildPackage,lumimqttd))
