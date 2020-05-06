# Copyright (c) 2015 OpenWrt.org
#
include $(TOPDIR)/rules.mk

ARCH:=arm
BOARD:=mtk
BOARDNAME:=MTK ARM panther
SUBTARGETS:=panther
FEATURES:=squashfs nand ramdisk
MAINTAINER:=John Crispin <john@phrozen.org>

KERNEL_PATCHVER:=5.4

include $(INCLUDE_DIR)/target.mk

$(eval $(call BuildTarget))
