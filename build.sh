#!/bin/bash
# Build script for BPI-M2U-BSP

BOARD=$1
mode=$2

usage() {
	cat <<-EOT >&2
	Usage: $0 <board>
	EOT
	./configure
}

if [ $# -eq 0 ]; then
	usage
	exit 1
fi

./configure $BOARD

if [ -f env.sh ] ; then
	. env.sh
fi

echo "This tool support following building mode(s):"
echo "--------------------------------------------------------------------------------"
echo "	1. Build all, uboot and kernel and pack to download images."
echo "	2. Build uboot only."
echo "	3. Build kernel only."
echo "	4. kernel configure."
echo "	5. Pack the builds to target download image, this step must execute after u-boot,"
echo "	   kernel and rootfs build out"
echo "	6. Update local build to SD with BPI Image flashed"
echo "	7. Clean all build."
echo "--------------------------------------------------------------------------------"

if [ -z "$mode" ]; then
	read -p "Please choose a mode(1-7): " mode
	echo
fi

case "$mode" in
	[!1-7])
	    echo -e "\033[31m Invalid mode, exit   \033[0m"
	    exit 1
	    ;;
	*)
	    ;;
esac

echo -e "\033[31m Now building...\033[0m"
echo
case $mode in
	1) make && 
	   make pack;;
	2) make u-boot;;
	3) make kernel;;
	4) make kernel-config;;
	5) make pack;;
	6) make install;;
	7) make clean;;
esac
echo
