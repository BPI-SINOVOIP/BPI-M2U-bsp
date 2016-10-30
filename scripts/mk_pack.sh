
die() {
        echo "$*" >&2
        exit 1
}

[ -s "./env.sh" ] || die "please run ./configure first."

set -e

. ./env.sh

PACK_ROOT="sunxi-pack"
#PLATFORM="linux"
#PLATFORM="dragonboard"
PLATFORM="tina"

echo "MACH=$MACH, PLATFORM=$PLATFORM, BOARD=$BOARD"

scripts/pack_img.sh -c ${MACH} -p ${PLATFORM} -b ${TARGET_PRODUCT} -d uart0 -s none -s none -t $TOPDIR

scripts/bootloader.sh
