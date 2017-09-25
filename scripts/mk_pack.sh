#!/bin/bash

die() {
        echo "$*" >&2
        exit 1
}

[ -s "./env.sh" ] || die "please run ./configure first."

set -e

. ./env.sh

PACK_ROOT="$TOPDIR/sunxi-pack"
#PLATFORM="linux"
#PLATFORM="dragonboard"
PLATFORM="tina"

# change sd to emmc if board bootup with emmc and sdcard is used as storage.
STORAGE="sd"

pack_bootloader()
{
  BOARD=$1
  (
  echo "MACH=$MACH, PLATFORM=$PLATFORM, TARGET_PRODUCT=${TARGET_PRODUCT} BOARD=$BOARD"
  scripts/pack_img.sh -c ${MACH} -p ${PLATFORM} -b ${TARGET_PRODUCT} -d uart0 -s none -s none -t $TOPDIR -r ${STORAGE}
  )
  $TOPDIR/scripts/bootloader.sh $BOARD
}

BOARDS=`(cd sunxi-pack/allwinner/${TARGET_PRODUCT}/configs ; ls -1d BPI*)`
for IN in $BOARDS ; do
  pack_bootloader $IN
done 
