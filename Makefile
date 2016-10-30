.PHONY: all clean help
.PHONY: u-boot kernel kernel-config
.PHONY: linux pack

include chosen_board.mk

SUDO=sudo
CROSS_COMPILE=arm-linux-gnueabi-
U_CROSS_COMPILE=$(CROSS_COMPILE)
K_CROSS_COMPILE=$(CROSS_COMPILE)

OUTPUT_DIR=$(CURDIR)/output

U_CONFIG_H=$(U_O_PATH)/include/config.h
K_DOT_CONFIG=$(K_O_PATH)/.config

LICHEE_KDIR=$(CURDIR)/linux-sunxi
ROOTFS=$(CURDIR)/rootfs/linux/default_linux_rootfs.tar.gz

Q=
J=$(shell expr `grep ^processor /proc/cpuinfo  | wc -l` \* 2)

all: bsp

## DK, if u-boot and kernel KBUILD_OUT issue fix, u-boot-clean and kernel-clean
## are no more needed
clean: u-boot-clean kernel-clean
	rm -f chosen_board.mk

## pack
pack: sunxi-pack
	$(Q)scripts/mk_pack.sh

# u-boot
$(U_CONFIG_H): u-boot-sunxi
	$(Q)$(MAKE) -C u-boot-sunxi $(UBOOT_CONFIG)_config CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot: $(U_CONFIG_H)
	$(Q)$(MAKE) -C u-boot-sunxi all CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot-clean:
	$(Q)$(MAKE) -C u-boot-sunxi CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J distclean

## linux
$(K_DOT_CONFIG): linux-sunxi
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm $(KERNEL_CONFIG)

kernel: $(K_DOT_CONFIG)
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output uImage dtbs
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules
	$(Q)$(MAKE) -C linux-sunxi/modules/gpu/mali400/kernel_mode/driver/src/devicedrv/mali CROSS_COMPILE=$(K_CROSS_COMPILE) ARCH=arm TARGET_PLATFORM="" KDIR=${LICHEE_KDIR} LICHEE_KDIR=${LICHEE_KDIR} USING_DT=1 BUILD=release USING_UMP=0
#	$(Q)$(MAKE) -C linux-sunxi/modules/gpu CROSS_COMPILE=$(K_CROSS_COMPILE) ARCH=arm TARGET_PLATFORM="" KDIR=${LICHEE_KDIR}  LICHEE_KDIR=${LICHEE_KDIR} USING_DT=1 BUILD=release USING_UMP=1 install V=1
#	$(Q)$(MAKE) -C linux-sunxi/modules/mali CROSS_COMPILE=$(K_CROSS_COMPILE) ARCH=arm TARGET_PLATFORM="" KDIR=${LICHEE_KDIR}  LICHEE_KDIR=${LICHEE_KDIR} USING_DT=1 BUILD=release USING_UMP=1 install V=1
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules_install
#	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J headers_install

kernel-clean:
#	$(Q)$(MAKE) -C linux-sunxi/modules/gpu CROSS_COMPILE=$(K_CROSS_COMPILE) ARCH=arm LICHEE_KDIR=${LICHEE_KDIR} clean
	$(Q)$(MAKE) -C linux-sunxi/modules/gpu/mali400/kernel_mode/driver/src/devicedrv/mali CROSS_COMPILE=$(K_CROSS_COMPILE) ARCH=arm TARGET_PLATFORM="" KDIR=${LICHEE_KDIR} LICHEE_KDIR=${LICHEE_KDIR} USING_DT=1 BUILD=release USING_UMP=0 clean
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J distclean
	rm -rf linux-sunxi/output/

kernel-config: $(K_DOT_CONFIG)
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm CROSS_COMPILE=${K_CROSS_COMPILE} -j$J menuconfig
	cp linux-sunxi/.config linux-sunxi/arch/arm/configs/$(KERNEL_CONFIG)

## bsp
bsp: u-boot kernel

## linux
linux: 
	$(Q)scripts/mk_linux.sh $(ROOTFS)

help:
	@echo ""
	@echo "Usage:"
	@echo "  make bsp             - Default 'make'"
	@echo "  make linux         - Build target for linux platform, as ubuntu, need permisstion confirm during the build process"
	@echo "   Arguments:"
	@echo "    ROOTFS=            - Source rootfs (ie. rootfs.tar.gz with absolute path)"
	@echo ""
	@echo "  make pack            - pack the images and rootfs to a PhenixCard download image."
	@echo "  make clean"
	@echo ""
	@echo "Optional targets:"
	@echo "  make kernel           - Builds linux kernel"
	@echo "  make kernel-config    - Menuconfig"
	@echo "  make u-boot          - Builds u-boot"
	@echo ""

