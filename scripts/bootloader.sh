#!/bin/sh

#gunzip -c BPI_M3_1080P.img.gz | dd of=/dev/mmcblk0 conv=sync,noerror bs=1k

die() {
        echo "$*" >&2
        exit 1
}

[ -s "./env.sh" ] || die "please run ./configure first."

. ./env.sh

O=$1
P=$TOPDIR/out/${TARGET_PRODUCT}/image/

TMP_FILE=$TOPDIR/out/${TARGET_PRODUCT}/${BOARD}.tmp
IMG_FILE=$TOPDIR/out/${TARGET_PRODUCT}/${BOARD}.img

sudo dd if=/dev/zero of=${TMP_FILE} bs=1M count=100
LOOP_DEV=`sudo losetup -f --show ${TMP_FILE}`

sudo dd if=$P/boot0_sdcard.fex 	of=${LOOP_DEV} bs=1k seek=8 2>&1 >/dev/null
sudo dd if=$P/boot_package.fex 	of=${LOOP_DEV} bs=1k seek=16400 2>&1 >/dev/null
sudo dd if=$P/sunxi_mbr.fex 	of=${LOOP_DEV} bs=1k seek=20480 2>&1 >/dev/null
sudo dd if=$P/boot-resource.fex	of=${LOOP_DEV} bs=1k seek=36864 2>&1 >/dev/null
sudo dd if=$P/env.fex 		of=${LOOP_DEV} bs=1k seek=53248 2>&1 >/dev/null
sudo dd if=$P/boot.fex 		of=${LOOP_DEV} bs=1k seek=54272 2>&1 >/dev/null

sudo sync

sudo losetup -d ${LOOP_DEV}

dd if=${TMP_FILE} of=${IMG_FILE} bs=1024 skip=8 count=102392 status=noxfer

rm -f ${IMG_FILE}.gz
echo "gzip ${IMG_FILE}"
gzip ${IMG_FILE}
rm -f ${TMP_FILE}
