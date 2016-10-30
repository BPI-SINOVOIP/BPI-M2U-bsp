#
# Copyright (C) 2015-2016 Allwinner
#
# This is free software, licensed under the GNU General Public License v2.
# See /build/LICENSE for more information.

define KernelPackage/net-ap6212
  SUBMENU:=$(WIRELESS_MENU)
  TITLE:=ap6212 support
  DEPENDS:=@TARGET_astar_parrot +ap6212-firmware
  FILES:=$(LINUX_DIR)/drivers/net/wireless/bcmdhd/bcmdhd.ko
  AUTOLOAD:=$(call AutoProbe,bcmdhd)
endef

define KernelPackage/net-ap6212/description
 Kernel modules for Broadcom AP6212  support
endef

$(eval $(call KernelPackage,net-ap6212))
