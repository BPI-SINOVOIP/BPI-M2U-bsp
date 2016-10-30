#!/bin/sh

die() {
	echo "$*" >&2
	exit 1
}

[ -s "./chosen_board.mk" ] || die "please run ./configure first."

set -e

. ./chosen_board.mk

export PATH=$COMPILE_TOOL:$PATH
export PATH=$COMPILE_TOOL/../../tools:$PATH

U_O_PATH="u-boot-sunxi"
K_O_PATH="linux-sunxi"
LINUXFS_DIR="rootfs/linux"
ROOTFS_PACKAGE=

ABI=armhf


gen_ramfs() {
	local kerneldir="$1"

	cp linux-sunxi/rootfs/rootfs.cpio.gz $kerneldir/

        mkbootimg --kernel $kerneldir/bImage \
                  --ramdisk $kerneldir/rootfs.cpio.gz \
                  --board 'a83t' \
                  --base 0x40000000 \
                  -o $kerneldir/boot.img
        rm  $kerneldir/rootfs.cpio.gz
}

gen_linuxfs()
{	
	local rootfs="$1"
	local linuxfs=$LINUXFS_DIR
	local rootfs_tmp=$rootfs/rootfs_tmp

	## build rootfs.ext4
	cd $linuxfs && ./mk_linux_ext4_rootfs.sh $ROOTFS_PACKAGE
	cd -
	mv $linuxfs/rootfs.ext4 $rootfs/rootfs.ext4

	mkdir -p "$rootfs_tmp"
	sudo umount $rootfs_tmp || true
        sudo mount -t ext4 $rootfs/rootfs.ext4 $rootfs_tmp

	## kernel modules
        sudo cp -r "$K_O_PATH/output/lib/modules" "$rootfs_tmp/lib/"
        sudo rm -f "$rootfs_tmp/lib/modules"/*/source
        sudo rm -f "$rootfs_tmp/lib/modules"/*/build

	sudo umount $rootfs_tmp || true
        sudo sudo rm -rf $rootfs_tmp
}

create_linuxfs() {

	local rootfs="output/${BOARD}/rootfs"
	local kerneldir="output/${BOARD}/kernel"
	local bootloader="output/${BOARD}/bootloader"
	local f=

	rm -rf "output/${BOARD}"

	## kernel
	mkdir -p "$kerneldir"
	cp -r "$K_O_PATH"/arch/arm/boot/uImage "$kerneldir/"
	cp -r "$K_O_PATH"/bImage "$kerneldir/"
	cp -r "$K_O_PATH"/drivers/arisc/binary/arisc "$kerneldir/"

	## boot.img
	gen_ramfs "$kerneldir"

	## bootloader
	mkdir -p "$bootloader"
	cp -r "$U_O_PATH/u-boot-${UBOOT_CONFIG}.bin" "$bootloader/"
	cp -r "$U_O_PATH/u-boot.bin" "$bootloader/"

	## rootfs
	mkdir -p "$rootfs"
	#gen_linuxfs "$rootfs"
	cp $LINUXFS_DIR/rootfs.ext4 $rootfs/rootfs.ext4

	echo "Done."
}

#[ $# -eq 1 ] || die "Usage: $0 linuxfs.tar.gz"

ROOTFS_PACKAGE=$1

#SKIP
#create_linuxfs

echo  "\033[0;31;1m###########################################\033[0m"
echo  "\033[0;31;1m#          create $BOARD success          #\033[0m"
echo  "\033[0;31;1m###########################################\033[0m"
