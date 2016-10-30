#!/bin/sh

# make partition table by fdisk command
# reserve part for fex binaries download 0~204799
# partition1 /dev/sdc1 vfat 204800~327679
# partition2 /dev/sdc2 ext4 327680~end

die() {
        echo "$*" >&2
        exit 1
}

[ $# -eq 1 ] || die "Usage: $0 /dev/sdc"

[ -s "./env.sh" ] || die "please run ./configure first."

. ./env.sh

O=$1
#P=../out/${TARGET_PRODUCT}/image/
P=$TOPDIR/out/${TARGET_PRODUCT}/image/

sudo dd if=$P/boot0_sdcard.fex 	of=$O bs=1k seek=8
sudo dd if=$P/boot_package.fex 	of=$O bs=1k seek=16400
sudo dd if=$P/sunxi_mbr.fex 	of=$O bs=1k seek=20480
sudo dd if=$P/boot-resource.fex	of=$O bs=1k seek=36864
sudo dd if=$P/env.fex 		of=$O bs=1k seek=53248
#sudo dd if=$P/boot.fex 		of=$O bs=1k seek=54272

