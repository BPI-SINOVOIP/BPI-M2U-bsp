#
# Copyright (C) 2015-2016 Allwinner
#
# This is free software, licensed under the GNU General Public License v2.
# See /build/LICENSE for more information.

define KernelPackage/sunxi-vfe
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=sunxi-vfe support
  FILES:=$(LINUX_DIR)/drivers/media/v4l2-core/videobuf2-core.ko
  FILES+=$(LINUX_DIR)/drivers/media/v4l2-core/videobuf2-dma-contig.ko
  FILES+=$(LINUX_DIR)/drivers/media/v4l2-core/videobuf2-memops.ko
  FILES+=$(LINUX_DIR)/drivers/media/platform/sunxi-vfe/vfe_io.ko
  FILES+=$(LINUX_DIR)/drivers/media/platform/sunxi-vfe/device/ov5640.ko
  FILES+=$(LINUX_DIR)/drivers/media/platform/sunxi-vfe/vfe_v4l2.ko
  AUTOLOAD:=$(call AutoLoad,90,videobuf2-core videobuf2-dma-contig videobuf2-memops vfe_io ov5640 vfe_v4l2)
endef

define KernelPackage/sunxi-vfe/description
  Kernel modules for sunxi-vfe support
endef

$(eval $(call KernelPackage,sunxi-vfe))

define KernelPackage/sunxi-uvc
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=sunxi-uvc support
  FILES:=$(LINUX_DIR)/drivers/media/v4l2-core/videobuf2-core.ko
  FILES+=$(LINUX_DIR)/drivers/media/v4l2-core/videobuf2-memops.ko
  FILES+=$(LINUX_DIR)/drivers/media/v4l2-core/videobuf2-vmalloc.ko
  FILES+=$(LINUX_DIR)/drivers/media/usb/uvc/uvcvideo.ko
  KCONFIG:=CONFIG_USB_VIDEO_CLASS
  AUTOLOAD:=$(call AutoLoad,95,videobuf2-core videobuf2-memops videobuf2_vmalloc uvcvideo)
endef

define KernelPackage/sunxi-uvc/description
  Kernel modules for sunxi-uvc support
endef

$(eval $(call KernelPackage,sunxi-uvc))

define KernelPackage/leds-sunxi
  SUBMENU:=$(LEDS_MENU)
  TITLE:=leds-sunxi support
  FILES:=$(LINUX_DIR)/drivers/leds/leds-sunxi.ko
  KCONFIG:=CONFIG_LEDS_SUNXI
  AUTOLOAD:=$(call AutoLoad,60,leds-sunxi)
endef

define KernelPackage/leds-sunxi/description
  Kernel modules for leds sunxi support
endef

$(eval $(call KernelPackage,leds-sunxi))

define KernelPackage/ledtrig-doubleflash
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED DoubleFlash Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_DOUBLEFLASH
  FILES:=$(LINUX_DIR)/drivers/leds/ledtrig-doubleflash.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-doubleflash)
endef

define KernelPackage/ledtrig-doubleflash/description
 Kernel module that allows LEDs to be controlled by a programmable doubleflash
 via sysfs
endef

$(eval $(call KernelPackage,ledtrig-doubleflash))

define KernelPackage/crypto-sunxi
  SUBMENU:=$(CRYPTO_MENU)
  TITLE:=crypto sunxi kernel driver
  KCONFIG:=CONFIG_CRYPTO_SUNXI
  FILES:=$(LINUX_DIR)/drivers/crypto/sunxi/ss.ko
  AUTOLOAD:=$(call AutoLoad,90,ss)
endef

$(eval $(call KernelPackage,crypto-sunxi))

define KernelPackage/sunxi-disp
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=sunxi-disp support
  FILES+=$(LINUX_DIR)/drivers/video/sunxi/disp2/disp/disp.ko
  AUTOLOAD:=$(call AutoLoad,10,disp)
endef

define KernelPackage/sunxi-disp/description
  Kernel modules for sunxi-disp support
endef

$(eval $(call KernelPackage,sunxi-disp))

define KernelPackage/sunxi-tv
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=sunxi-tv support
  DEPENDS:=+kmod-sunxi-disp
  FILES+=$(LINUX_DIR)/drivers/video/sunxi/disp2/tv/tv.ko
  AUTOLOAD:=$(call AutoLoad,15,tv)
endef

define KernelPackage/sunxi-tv/description
  Kernel modules for sunxi-tv support
endef

$(eval $(call KernelPackage,sunxi-tv))

define KernelPackage/sunxi-hdmi
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=sunxi-hdmi support
  DEPENDS:=+kmod-sunxi-disp
  FILES+=$(LINUX_DIR)/drivers/video/sunxi/disp2/hdmi/hdmi.ko
  AUTOLOAD:=$(call AutoLoad,15,hdmi)
endef

define KernelPackage/sunxi-hdmi/description
  Kernel modules for sunxi-disp support
endef

$(eval $(call KernelPackage,sunxi-hdmi))

define KernelPackage/net-ap6212
  SUBMENU:=$(WIRELESS_MENU)
  TITLE:=ap6212 support
  DEPENDS:=@LINUX_3_10 +ap6212-firmware
  FILES:=$(LINUX_DIR)/drivers/net/wireless/bcmdhd/bcmdhd.ko
  AUTOLOAD:=$(call AutoProbe,bcmdhd)
endef

define KernelPackage/net-ap6212/description
 Kernel modules for Broadcom AP6212  support
endef

$(eval $(call KernelPackage,net-ap6212))
