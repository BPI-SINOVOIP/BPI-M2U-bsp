#!/bin/bash

die() {
        echo "$*" >&2
        exit 1
}

[ -s "./env.sh" ] || die "please run ./configure first."

. ./env.sh

board=$(echo ${BOARD%-*} | tr '[A-Z]' '[a-z]')

echo "pack $board"

BOOTLOADER=${TOPDIR}/SD/${board}/100MB
BOOT=${TOPDIR}/SD/${board}/BPI-BOOT
ROOT=${TOPDIR}/SD/${board}/BPI-ROOT
CONFIG_DIR=${TOPDIR}/sunxi-pack/${MACH}/configs
KERN_DIR=${TOPDIR}/linux-sunxi

if [ -d $TOPDIR/SD ]; then
  rm -rf $TOPDIR/SD
fi

mkdir -p $BOOTLOADER
mkdir -p $BOOT
mkdir -p $ROOT

PACK_ROOT="$TOPDIR/sunxi-pack"
PLATFORM="linux"

# change sd to emmc if board bootup with emmc and sdcard is used as storage.
STORAGE="sd"

pack_bootloader()
{
  echo "pack bootloader"

  BOARD_LIST=`(cd sunxi-pack/${MACH}/configs ; ls -1d ${BOARD%-*}-*)`
 
  for BOARD in $BOARD_LIST ; do
	echo "MACH=$MACH, PLATFORM=$PLATFORM, BOARD=$BOARD"
	scripts/pack_img.sh -c ${MACH} -p ${PLATFORM} -b ${BOARD} -d uart0 -s none -s none -t $TOPDIR -r ${STORAGE}

	$TOPDIR/scripts/bootloader.sh $BOARD
  done 

  cp -a ${TOPDIR}/out/100MB/${BOARD%-*}-* $BOOTLOADER/
}

pack_boot()
{
  echo "pack boot"

  mkdir -p ${BOOT}/bananapi/${board}/linux
  cp -a ${CONFIG_DIR}/default/linux/* ${BOOT}/bananapi/${board}/linux/
  cp -a ${CONFIG_DIR}/${BOARD%-*}-*/linux/* ${BOOT}/bananapi/${board}/linux/
  cp -a ${KERN_DIR}/arch/$ARCH/boot/uImage ${BOOT}/bananapi/${board}/linux/uImage
}

pack_root()
{
  echo "pack root"

  # bootloader files
  mkdir -p ${ROOT}/usr/lib/u-boot/bananapi/${board}
  cp -a ${BOOTLOADER}/*.gz ${ROOT}/usr/lib/u-boot/bananapi/${board}/

  # kernel modules files
  mkdir -p ${ROOT}/lib/modules
  cp -a ${KERN_DIR}/output/lib/modules/${KERNEL_MODULES} ${ROOT}/lib/modules

  # kernel headers files
  mkdir -p ${ROOT}/usr/src
  cp -a ${KERN_DIR}/output/usr/src/${KERNEL_HEADERS} ${ROOT}/usr/src/
}

tar_packages()
{
  echo "tar download packages"

  (cd $BOOT ; tar czvf ${TOPDIR}/SD/${board}/BPI-BOOT-${board}.tgz .)

  # split for github 100M limitation
  #(cd $ROOT ; tar czvf ${TOPDIR}/SD/${board}/${KERNEL_MODULES}.tgz lib/modules)
  (cd $ROOT ; tar czvf ${TOPDIR}/SD/${board}/${KERNEL_MODULES}-net.tgz lib/modules/${KERNEL_MODULES}/kernel/net)
  (cd $ROOT ; mv lib/modules/${KERNEL_MODULES}/kernel/net ${ROOT}/net)
  (cd $ROOT ; tar czvf ${TOPDIR}/SD/${board}/${KERNEL_MODULES}.tgz lib/modules)
  (cd $ROOT ; mv ${ROOT}/net lib/modules/${KERNEL_MODULES}/kernel/net)

  (cd $ROOT ; tar czvf ${TOPDIR}/SD/${board}/${KERNEL_HEADERS}.tgz usr/src/${KERNEL_HEADERS})
  (cd $ROOT ; tar czvf ${TOPDIR}/SD/${board}/BOOTLOADER-${board}.tgz usr/lib/u-boot/bananapi)
}

pack_bootloader
pack_boot
pack_root
tar_packages

echo "pack finish"
