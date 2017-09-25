#!/bin/bash
#
# pack/pack
# (c) Copyright 2013 - 2016 Allwinner
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
# Trace Wong <wangyaliang@allwinnertech.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

echo "BPI: $0 $*"
echo "BPI: BOARD=$BOARD"


############################ Notice #####################################
# a. Some config files priority is as follows:
#    - xxx_${platform}.{cfg|fex} > xxx.{cfg|fex}
#    - ${chip}/${board}/*.{cfg|fex} > ${chip}/default/*.{cfg|fex}
#    - ${chip}/default/*.cfg > common/imagecfg/*.cfg
#    - ${chip}/default/*.fex > common/partition/*.fex
#  e.g. sun8iw7p1/configs/perf/image_linux.cfg > sun8iw7p1/configs/default/image_linux.cfg
#       > common/imagecfg/image_linux.cfg > sun8iw7p1/configs/perf/image.cfg
#       > sun8iw7p1/configs/default/image.cfg > common/imagecfg/image.cfg
#
# b. Support Nor storages rule:
#    - Need to create sys_partition_nor.fex or sys_partition_nor_${platform}.fex
#    - Add "{filename = "full_img.fex",     maintype = "12345678", \
#      subtype = "FULLIMG_00000000",}" to image[_${platform}].cfg
#
# c. Switch uart port
#    - Need to add your chip configs into ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin
#    - Call pack with 'debug' parameters

function pack_error()
{
	echo -e "\033[47;31mERROR: $*\033[0m"
}

function pack_warn()
{
	echo -e "\033[47;34mWARN: $*\033[0m"
}

function pack_info()
{
	echo -e "\033[47;30mINFO: $*\033[0m"
}

source scripts/shflags

# define option, format:
#   'long option' 'default value' 'help message' 'short option'
DEFINE_string 'chip' '' 'chip to build, e.g. sun7i' 'c'
DEFINE_string 'platform' '' 'platform to build, e.g. linux, android, camdroid' 'p'
DEFINE_string 'board' '' 'board to build, e.g. evb' 'b'
DEFINE_string 'kernel' '' 'kernel to build, e.g. linux-3.4, linux-3.10' 'k'
DEFINE_string 'debug_mode' 'uart0' 'config debug mode, e.g. uart0, card0' 'd'
DEFINE_string 'signture' 'none' 'pack boot signture to do secure boot' 's'
DEFINE_string 'secure' 'none' 'pack secure boot with -v arg' 'v'
DEFINE_string 'mode' 'normal' 'pack dump firmware' 'm'
DEFINE_string 'function' 'android' 'pack private firmware' 'f'
DEFINE_string 'topdir' 'none' 'sdk top dir' 't'
DEFINE_string 'storage' 'none' 'storage type, emmc or sd' 'r'

# parse the command-line
FLAGS "$@" || exit $?
eval set -- "${FLAGS_ARGV}"

PACK_CHIP=${FLAGS_chip}
PACK_PLATFORM=${FLAGS_platform}
PACK_BOARD=${FLAGS_board}
PACK_KERN=${FLAGS_kernel}
PACK_DEBUG=${FLAGS_debug_mode}
PACK_SIG=${FLAGS_signture}
PACK_SECURE=${FLAGS_secure}
PACK_MODE=${FLAGS_mode}
PACK_FUNC=${FLAGS_function}
PACK_TOPDIR=${FLAGS_topdir}
PACK_STORAGE=${FLAGS_storage}

ROOT_DIR=${PACK_TOPDIR}/out/${PACK_BOARD}
OTA_TEST_NAME="ota_test"
export PATH=${PACK_TOPDIR}/out/host/bin/:$PATH

if [ "x${PACK_CHIP}" = "xsun5i" ]; then
	PACK_BOARD_PLATFORM=unclear
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw5p1" ]; then
	PACK_BOARD_PLATFORM=astar
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw6p1" ]; then
	PACK_BOARD_PLATFORM=octopus
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun8iw11p1" ]; then
	PACK_BOARD_PLATFORM=azalea
	ARCH=arm
elif [ "x${PACK_CHIP}" = "xsun50iw1p1" ]; then
	PACK_BOARD_PLATFORM=tulip
	ARCH=arm64
else
	echo "board_platform($PACK_CHIP) not support"
fi

#BPI
#if [ -f ${PACK_TOPDIR}/env.sh ] ; then
#	source ${PACK_TOPDIR}/env.sh
#fi

tools_file_list=(
generic/tools/split_xxxx.fex
${PACK_BOARD_PLATFORM}-common/tools/split_xxxx.fex
generic/tools/usbtool_test.fex
${PACK_BOARD_PLATFORM}-common/tools/usbtool_test.fex
generic/tools/cardscript.fex
generic/tools/cardscript_secure.fex
${PACK_BOARD_PLATFORM}-common/tools/cardscript.fex
${PACK_BOARD_PLATFORM}-common/tools/cardscript_secure.fex
generic/tools/cardtool.fex
${PACK_BOARD_PLATFORM}-common/tools/cardtool.fex
generic/tools/usbtool.fex
${PACK_BOARD_PLATFORM}-common/tools/usbtool.fex
generic/tools/aultls32.fex
${PACK_BOARD_PLATFORM}-common/tools/aultls32.fex
generic/tools/aultools.fex
${PACK_BOARD_PLATFORM}-common/tools/aultools.fex
)

#BPI
#
#${PACK_BOARD}/configs/*.fex
#${PACK_BOARD}/configs/*.cfg
#
configs_file_list=(
generic/toc/toc1.fex
generic/toc/toc0.fex
generic/toc/boot_package.fex
generic/dtb/sunxi.fex
generic/configs/*.fex
generic/configs/*.cfg
${PACK_BOARD_PLATFORM}-common/configs/*.fex
${PACK_BOARD_PLATFORM}-common/configs/*.cfg
${PACK_BOARD}/configs/default/*.fex
${PACK_BOARD}/configs/default/*.cfg
${PACK_BOARD}/configs/${BOARD}/*.fex
${PACK_BOARD}/configs/${BOARD}/*.cfg
)

#BPI
boot_resource_list=(
generic/boot-resource/boot-resource:image/
generic/boot-resource/boot-resource.ini:image/
${PACK_BOARD_PLATFORM}-common/boot-resource:image/
${PACK_BOARD_PLATFORM}-common/boot-resource.ini:image/
${PACK_BOARD}/configs/default/*.bmp:image/boot-resource/
${PACK_BOARD}/configs/${BOARD}/*.bmp:image/boot-resource/
)

boot_file_list=(
${PACK_BOARD_PLATFORM}-common/bin/boot0_nand_${PACK_CHIP}.bin:image/boot0_nand.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_sdcard_${PACK_CHIP}.bin:image/boot0_sdcard.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_spinor_${PACK_CHIP}.bin:image/boot0_spinor.fex
${PACK_BOARD_PLATFORM}-common/bin/fes1_${PACK_CHIP}.bin:image/fes1.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-${PACK_CHIP}.bin:image/u-boot.fex
${PACK_BOARD_PLATFORM}-common/bin/bl31.bin:image/monitor.fex
${PACK_BOARD_PLATFORM}-common/bin/scp.bin:image/scp.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-spinor-${PACK_CHIP}.bin:image/u-boot-spinor.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_nand_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_nand-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_sdcard_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_sdcard-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/boot0_spinor_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/boot0_spinor-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/u-boot-${OTA_TEST_NAME}.fex
${PACK_BOARD_PLATFORM}-common/bin/u-boot-spinor-${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/u-boot-spinor-${OTA_TEST_NAME}.fex
)

boot_file_secure=(
${PACK_BOARD_PLATFORM}-common/bin/semelis_${PACK_CHIP}.bin:image/semelis.bin
${PACK_BOARD_PLATFORM}-common/bin/sboot_${PACK_CHIP}.bin:image/sboot.bin
${PACK_BOARD_PLATFORM}-common/bin/sboot_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/sboot-${OTA_TEST_NAME}.bin
)

a64_boot_file_secure=(
${PACK_BOARD_PLATFORM}/bin/optee_${PACK_CHIP}.bin:image/optee.fex
${PACK_BOARD_PLATFORM}/bin/sboot_${PACK_CHIP}.bin:image/sboot.bin
${PACK_BOARD_PLATFORM}/bin/sboot_${PACK_CHIP}-${OTA_TEST_NAME}.bin:image/sboot-${OTA_TEST_NAME}.bin
)

function show_boards()
{
	printf "\nAll avaiable chips, platforms and boards:\n\n"
	printf "Chip            Board\n"
	for chipdir in $(find chips/ -mindepth 1 -maxdepth 1 -type d) ; do
		chip=`basename ${chipdir}`
		printf "${chip}\n"
		for boarddir in $(find chips/${chip}/configs/${platform} \
			-mindepth 1 -maxdepth 1 -type d) ; do
			board=`basename ${boarddir}`
			printf "                ${board}\n"
		done
	done
	printf "\nFor Usage:\n"
	printf "     $(basename $0) -h\n\n"
}

function uart_switch()
{
	rm -rf ${ROOT_DIR}/image/awk_debug_card0
	touch ${ROOT_DIR}/image/awk_debug_card0
	TX=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	RX=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	PORT=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	MS=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	CK=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	DO=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`
	DI=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_pin`

	BOOT_UART_ST=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	BOOT_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	BOOT_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	BOOT_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_ST=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $9}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART0_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $10}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	UART1_ST=`awk  '$0~"'$PACK_CHIP'"{print $11}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	JTAG_ST=`awk  '$0~"'$PACK_CHIP'"{print $12}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	MS_ST=`awk  '$0~"'$PACK_CHIP'"{print $13}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	CK_ST=`awk  '$0~"'$PACK_CHIP'"{print $14}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	DO_ST=`awk  '$0~"'$PACK_CHIP'"{print $15}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	DI_ST=`awk  '$0~"'$PACK_CHIP'"{print $16}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	MMC0_ST=`awk  '$0~"'$PACK_CHIP'"{print $17}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`
	MMC0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $18}' ${PACK_TOPDIR}/target/allwinner/generic/debug/card_debug_string`

	echo '$0!~";" && $0~"'$BOOT_TX_ST'"{if(C)$0="'$BOOT_TX_ST' = '$TX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$BOOT_RX_ST'"{if(C)$0="'$BOOT_RX_ST' = '$RX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$BOOT_PORT_ST'"{if(C)$0="'$BOOT_PORT_ST' = '$PORT'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$MMC0_USED_ST'"{if(A)$0="'$MMC0_USED_ST' = 0";A=0} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$MMC0_ST'\\]"{A=1}  \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_TX_ST'"{if(B)$0="'$UART0_TX_ST' = '$TX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_RX_ST'"{if(B)$0="'$UART0_RX_ST' = '$RX'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$UART0_ST'\\]"{B=1} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_USED_ST'"{if(B)$0="'$UART0_USED_ST' = 1"}  \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '/^'$UART0_PORT_ST'/{next} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$UART1_ST'\\]"{B=0} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$BOOT_UART_ST'\\]"{C=1} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"\\['$JTAG_ST'\\]"{C=0} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$MS_ST'"{$0="'$MS_ST' = '$MS'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$CK_ST'"{$0="'$CK_ST' = '$CK'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$DO_ST'"{$0="'$DO_ST' = '$DO'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '$0!~";" && $0~"'$DI_ST'"{$0="'$DI_ST' = '$DI'"} \' >> ${ROOT_DIR}/image/awk_debug_card0
	echo '1' >> ${ROOT_DIR}/image/awk_debug_card0

	awk -f ${ROOT_DIR}/image/awk_debug_card0 ${ROOT_DIR}/image/sys_config.fex > ${ROOT_DIR}/image/sys_config_debug.fex
	rm -f ${ROOT_DIR}/image/sys_config.fex
	mv ${ROOT_DIR}/image/sys_config_debug.fex ${ROOT_DIR}/image/sys_config.fex
	echo "uart -> card0"
}

function copy_ota_test_file()
{
	printf "ota test bootloader by diff bootlogo\n"
	mv ${ROOT_DIR}/image/boot-resource/bootlogo_ota_test.bmp ${ROOT_DIR}/image/boot-resource/bootlogo.bmp

	printf "copying ota test boot file\n"
	if [ -f ${ROOT_DIR}/image/sys_partition_nor.fex -o \
	-f ${ROOT_DIR}/image/sys_partition_nor_${PACK_PLATFORM}.fex ];  then
		mv ${ROOT_DIR}/image/boot0_spinor-${OTA_TEST_NAME}.fex	${ROOT_DIR}/image/boot0_spinor.fex
		mv ${ROOT_DIR}/image/u-boot-spinor-${OTA_TEST_NAME}.fex	${ROOT_DIR}/image/u-boot-spinor.fex
	else
		mv ${ROOT_DIR}/image/boot0_nand-${OTA_TEST_NAME}.fex		${ROOT_DIR}/image/boot0_nand.fex
		mv ${ROOT_DIR}/image/boot0_sdcard-${OTA_TEST_NAME}.fex	${ROOT_DIR}/image/boot0_sdcard.fex
		mv ${ROOT_DIR}/image/u-boot-${OTA_TEST_NAME}.fex		${ROOT_DIR}/image/u-boot.fex
	fi

	if [ "x${PACK_SECURE}" = "xsecure" -o  "x${PACK_SIG}" = "prev_refurbish"] ; then
		printf "Copying ota test secure boot file\n"
		mv ${ROOT_DIR}/image/sboot-${OTA_TEST_NAME}.bin ${ROOT_DIR}/image/sboot.bin
	fi

	printf "OTA test env by bootdelay(10) and logolevel(8)\n"
	sed -i 's/\(logolevel=\).*/\18/' ${ROOT_DIR}/image/env.cfg
	sed -i 's/\(bootdelay=\).*/\110/' ${ROOT_DIR}/image/env.cfg
}
ENV_SUFFIX=
function do_prepare()
{
	if [ -z "${PACK_CHIP}" -o -z "${PACK_PLATFORM}" -o -z "${PACK_BOARD}" ] ; then
		pack_error "Invalid parameters Chip: ${PACK_CHIP}, \
			Platform: ${PACK_PLATFORM}, Board: ${PACK_BOARD}"
		show_boards
		exit 1
	fi

	if [ ! -d ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD}/configs ] ; then
		pack_error "Board's directory \
			\"${PACK_TOPDIR}/target/allwinner/${PACK_BOARD}/configs\" is not exist."
		show_boards
		exit 1
	fi

	if [ -z "${PACK_KERN}" ] ; then
		printf "No kernel param, parse it from ${PACK_BOARD_PLATFORM}\n"
		if [ "x${PACK_BOARD_PLATFORM}" = "xtulip" -o "x${PACK_BOARD_PLATFORM}" = "xazalea" ]; then
			PACK_KERN="linux-3.10"
			ENV_SUFFIX=3.10
		else
			PACK_KERN="linux-3.4"
			ENV_SUFFIX=3.4
		fi
		if [ -z "${PACK_KERN}" ] ; then
			pack_error "Failed to parse kernel param from ${PACK_BOARD_PLATFORM}"
			exit 1
		fi
	fi

	# Cleanup
	rm -rf ${ROOT_DIR}/image
	mkdir -p ${ROOT_DIR}/image

	printf "copying tools file\n"
	for file in ${tools_file_list[@]} ; do
		cp -f ${PACK_TOPDIR}/target/allwinner/$file ${ROOT_DIR}/image/ 2> /dev/null
	done

	printf "copying configs file\n"
	for file in ${configs_file_list[@]} ; do
		cp -f ${PACK_TOPDIR}/target/allwinner/$file ${ROOT_DIR}/image/ 2> /dev/null
	done
	# amend env copy
	rm ${ROOT_DIR}/image/env-3*.cfg 2> /dev/null
	cp -f ${PACK_TOPDIR}/target/allwinner/generic/configs/env-${ENV_SUFFIX}.cfg ${ROOT_DIR}/image/env.cfg 2> /dev/null
	#BPI
	cp -f ${PACK_TOPDIR}/target/allwinner/${PACK_BOARD}/configs/${BOARD}/env.cfg ${ROOT_DIR}/image/env.cfg 2> /dev/null
	# If platform config files exist, we will cover the default files
	# For example, mv out/image_linux.cfg out/image.cfg
	cd ${ROOT_DIR}
	find image/* -type f -a \( -name "*.fex" -o -name "*.cfg" \) -print | \
		sed "s#\(.*\)_${PACK_PLATFORM}\(\..*\)#mv -fv & \1\2#e"
	cd -

	if [ "x${PACK_MODE}" = "xdump" ] ; then
		cp -vf ${ROOT_DIR}/image/sys_partition_dump.fex ${ROOT_DIR}/image/sys_partition.fex
		cp -vf ${ROOT_DIR}/image/usbtool_test.fex ${ROOT_DIR}/image/usbtool.fex
	elif [ "x${PACK_FUNC}" = "xprvt" ] ; then
		cp -vf ${ROOT_DIR}/image/sys_partition_private.fex ${ROOT_DIR}/image/sys_partition.fex
	fi

	printf "copying boot resource\n"
	for file in ${boot_resource_list[@]} ; do
		cp -rf ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
			${ROOT_DIR}/`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done

	printf "copying boot file\n"
	for file in ${boot_file_list[@]} ; do
		cp -f ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
			${ROOT_DIR}/`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done

	if [ "x${ARCH}" != "xarm64" ] ; then
		if [ "x${PACK_SECURE}" = "xsecure" -o "x${PACK_SIG}" = "xsecure" -o  "x${PACK_SIG}" = "xprev_refurbish" ] ; then
			printf "copying secure boot file\n"
			for file in ${boot_file_secure[@]} ; do
				cp -f ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
					${ROOT_DIR}/`echo $file | awk -F: '{print $2}'`
			done
		fi
	else
		if [ "x${PACK_SECURE}" = "xsecure" -o "x${PACK_SIG}" = "xsecure" -o  "x${PACK_SIG}" = "xprev_refurbish" ] ; then
			printf "copying arm64 secure boot file\n"
			for file in ${a64_boot_file_secure[@]} ; do
				cp -f ${PACK_TOPDIR}/target/allwinner/`echo $file | awk -F: '{print $1}'` \
					${ROOT_DIR}/`echo $file | awk -F: '{print $2}'`
			done
		fi
	fi

	if [ "x${PACK_SECURE}" = "xsecure"  -o "x${PACK_SIG}" = "xsecure" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' ${ROOT_DIR}/image/sys_config.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=0' ${ROOT_DIR}/image/sys_config.fex
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' ${ROOT_DIR}/image/sys_config.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=1' ${ROOT_DIR}/image/sys_config.fex
	else
		sed -i '/^burn_secure_mod/d' ${ROOT_DIR}/image/sys_config.fex
		sed -i '/^secure_without_OS/d' ${ROOT_DIR}/image/sys_config.fex
	fi

	if [ "x${PACK_MODE}" = "xota_test" ] ; then
		printf "copy ota test file\n"
		copy_ota_test_file
	fi

	# Here, we can switch uart to card or normal
	if [ "x${PACK_DEBUG}" = "xcard0" -a "x${PACK_MODE}" != "xdump" \
		-a "x${PACK_FUNC}" != "xprvt" ] ; then \
		uart_switch
	fi

	sed -i 's/\\boot-resource/\/boot-resource/g' ${ROOT_DIR}/image/boot-resource.ini
	sed -i 's/\\\\/\//g' ${ROOT_DIR}/image/image.cfg
	sed -i 's/^imagename/;imagename/g' ${ROOT_DIR}/image/image.cfg

	IMG_NAME="${PACK_PLATFORM}_${PACK_BOARD}_${PACK_DEBUG}"

	if [ "x${PACK_SIG}" != "xnone" ]; then
		IMG_NAME="${IMG_NAME}_${PACK_SIG}"
	fi

	if [ "x${PACK_MODE}" = "xdump" -o "x${PACK_MODE}" = "xota_test" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_MODE}"
	fi

	if [ "x${PACK_FUNC}" = "xprvt" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_FUNC}"
	fi

	if [ "x${PACK_SECURE}" = "xsecure" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_SECURE}"
	fi

	if [ "x${PACK_FUNC}" = "xprev_refurbish" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_FUNC}"
	fi

	IMG_NAME="${IMG_NAME}.img"

	echo "imagename = $IMG_NAME" >> ${ROOT_DIR}/image/image.cfg
	echo "" >> ${ROOT_DIR}/image/image.cfg
}

function do_ini_to_dts()
{
	if [ "x${PACK_KERN}" != "xlinux-3.10" ] ; then
		return
	fi

	local DTC_COMPILER=${PACK_TOPDIR}/lichee/$PACK_KERN/scripts/dtc/dtc
	local DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-${PACK_BOARD}.dtb.d.dtc.tmp
	local DTC_SRC_PATH=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/
	local DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-${PACK_BOARD}.dtb.dts
	local DTC_INI_FILE_BASE=${ROOT_DIR}/image/sys_config.fex
	local DTC_INI_FILE=${ROOT_DIR}/image/sys_config_fix.fex
	cp $DTC_INI_FILE_BASE $DTC_INI_FILE
	sed -i "s/\(\[dram\)_para\(\]\)/\1\2/g" $DTC_INI_FILE
	sed -i "s/\(\[nand[0-9]\)_para\(\]\)/\1\2/g" $DTC_INI_FILE

	if [ ! -f $DTC_COMPILER ]; then
		pack_error "Script_to_dts: Can not find dtc compiler.\n"
		exit 1
	fi


	if [ ! -f $DTC_DEP_FILE ]; then
	    printf "Script_to_dts: Can not find [%s-%s.dts]. Will use common dts file instead.\n" ${PACK_CHIP} ${PACK_BOARD}
	    printf "package_STORAGE=%s\n" ${PACK_STORAGE}

	    if [ "x${PACK_STORAGE}" = "xsd" ]; then
		DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-sd.dtb.dts
		DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-sd.dtb.d.dtc.tmp
	    elif [ "x${PACK_STORAGE}" = "xemmc" ]; then
		DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-emmc.dtb.dts
		DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-emmc.dtb.d.dtc.tmp
	    else
		DTC_SRC_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc.dtb.dts
		DTC_DEP_FILE=${PACK_TOPDIR}/lichee/$PACK_KERN/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc.dtb.d.dtc.tmp
	    fi
	fi

	$DTC_COMPILER -O dtb -o ${ROOT_DIR}/image/sunxi.dtb	\
		-b 0			\
		-i $DTC_SRC_PATH	\
		-F $DTC_INI_FILE	\
		-d $DTC_DEP_FILE $DTC_SRC_FILE
	if [ $? -ne 0 ]; then
		pack_error "Conver script to dts failed"
		exit 1
	fi

	#restore the orignal dtsi
	if [ "x${ARCH}" = "xarm64" ]; then
		if [ "x${PACK_PLATFORM}" = "xdragonboard" ]; then
			local DTS_PATH=$(PACK_TOPDIR)/lichee/linux-3.10/arch/arm64/boot/dts/
			if [ -f ${DTS_PATH}/sun50iw1p1_bak.dtsi ];then
				rm -f ${DTS_PATH}/sun50iw1p1.dtsi
				mv  ${DTS_PATH}/sun50iw1p1_bak.dtsi  ${DTS_PATH}/sun50iw1p1.dtsi
			fi
		fi
	fi

	printf "Conver script to dts ok.\n"
	return
}

function do_common()
{
	cd ${ROOT_DIR}/image

	busybox unix2dos sys_config.fex
	busybox unix2dos sys_partition.fex
	script  sys_config.fex > /dev/null
	script  sys_partition.fex > /dev/null
	cp -f   sys_config.bin config.fex

	if [ "x${PACK_PLATFORM}" = "xdragonboard" ] ; then
		busybox dos2unix test_config.fex
		cp test_config.fex boot-resource/
		busybox unix2dos test_config.fex
		script test_config.fex > /dev/null
		cp test_config.bin boot-resource/
	fi

	# Those files for SpiNor. We will try to find sys_partition_nor.fex
	if [ -f sys_partition_nor.fex ];  then

		if [ -f "sunxi.dtb" ]; then
			cp sunxi.dtb sunxi.fex
			update_uboot_fdt u-boot-spinor.fex sunxi.fex u-boot-spinor.fex >/dev/null
		fi

		# Here, will create sys_partition_nor.bin
		busybox unix2dos sys_partition_nor.fex
		script  sys_partition_nor.fex > /dev/null
		update_boot0 boot0_spinor.fex   sys_config.bin SDMMC_CARD > /dev/null
		update_uboot u-boot-spinor.fex  sys_config.bin >/dev/null

		if [ -f boot_package_nor.cfg ]; then
			echo "pack boot package"
			busybox unix2dos boot_package.cfg
			dragonsecboot -pack boot_package_nor.cfg
			cp boot_package.fex boot_package_nor.fex
		fi
		# Ugly, but I don't have a better way to change it.
		# We just set env's downloadfile name to env_nor.cfg in sys_partition_nor.fex
		# And if env_nor.cfg is not exist, we should copy one.
		if [ ! -f env_nor.cfg ]; then
			cp -f env.cfg env_nor.cfg >/dev/null 2<&1
		fi

		# Fixup boot mode for SPINor, just can bootm
		sed -i '/^boot_normal/s#\<boota\>#bootm#g' env_nor.cfg

		u_boot_env_gen env_nor.cfg env_nor.fex >/dev/null
	fi


	if [ -f "sunxi.dtb" ]; then
		cp sunxi.dtb sunxi.fex
		update_uboot_fdt u-boot.fex sunxi.fex u-boot.fex >/dev/null
	fi

	# Those files for Nand or Card
	update_boot0 boot0_nand.fex	sys_config.bin NAND > /dev/null
	update_boot0 boot0_sdcard.fex	sys_config.bin SDMMC_CARD > /dev/null
	update_uboot u-boot.fex         sys_config.bin > /dev/null
	update_fes1  fes1.fex           sys_config.bin > /dev/null
	fsbuild	     boot-resource.ini  split_xxxx.fex > /dev/null

	if [ -f boot_package.cfg ]; then
			echo "pack boot package"
			busybox unix2dos boot_package.cfg
			dragonsecboot -pack boot_package.cfg
	fi

	if [ "x${PACK_FUNC}" = "xprvt" ] ; then
		u_boot_env_gen env_burn.cfg env.fex > /dev/null
	else
		u_boot_env_gen env.cfg env.fex > /dev/null
	fi

	if [ -f "arisc" ]; then
		ln -s arisc arisc.fex
	fi
}

function do_finish()
{
	# Yeah, it should contain all files into full_img.fex for spinor
	# Because, as usually, spinor image size is very small.
	# If fail to create full_img.fex, we should fake it empty.

	# WTF, it is so ugly!!! It must be sunxi_mbr.fex & sys_partition.bin,
	# not sunxi_mbr_xxx.fex & sys_partition_xxx.bin. In order to advoid this
	# loathsome thing, we need to backup & copy files. Check whether
	# sys_partition_nor.bin is exist, and create sunxi_mbr.fex for Nor.
	if [ -f sys_partition_nor.bin ]; then
		mv -f sys_partition.bin         sys_partition.bin_back
		cp -f sys_partition_nor.bin     sys_partition.bin
		update_mbr                      sys_partition.bin 1 > /dev/null
		#when use devicetree, the size of uboot+dtb is larger then 256K
		if [ -f boot_package_nor.cfg ]; then
			BOOT1_FILE=boot_package_nor.fex
		else
			BOOT1_FILE=u-boot-spinor.fex
		fi
		LOGIC_START=496 #496+16=512K
		merge_full_img --out full_img.fex \
			--boot0 boot0_spinor.fex \
			--boot1 ${BOOT1_FILE} \
			--mbr sunxi_mbr.fex \
			--logic_start ${LOGIC_START} \
			--partition sys_partition.bin
		if [ $? -ne 0 ]; then
			pack_error "merge_full_img failed"
			exit 1
		fi
		mv -f sys_partition.bin_back    sys_partition.bin
	fi
	if [ ! -f full_img.fex ]; then
		echo "full_img.fex is empty" > full_img.fex
	fi

	update_mbr          sys_partition.bin 4 > /dev/null
	dragon image.cfg    sys_partition.fex
        if [ $? -eq 0 ]; then
	    if [ -e ${IMG_NAME} ]; then
		    mv ${IMG_NAME} ../${IMG_NAME}
		    echo '----------image is at----------'
		    echo -e '\033[0;31;1m'
		    echo ${ROOT_DIR}/${IMG_NAME}
		    echo -e '\033[0m'
	    fi
        fi
	cd ..
	printf "pack finish\n"
}

function do_signature()
{
	printf "prepare for signature by openssl\n"
	if [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		if [ "x${ARCH}" = "xarm64" ] ; then
			cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_a64_no_secureos.cfg dragon_toc.cfg
		else
			cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_no_secureos.cfg dragon_toc.cfg
		fi
	else
		if [ "x${ARCH}" = "xarm64" ] ; then
			if [ "x${PACK_PLATFORM}" = "xdragonboard" ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_a64_linux.cfg dragon_toc.cfg
			else
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_a64.cfg dragon_toc.cfg
			fi
		else
			if [ "x${PACK_PLATFORM}" = "xlinux" ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc.cfg dragon_toc.cfg
			elif [ "x${PACK_PLATFORM}" = "xtina" ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc.cfg dragon_toc.cfg
			elif [ "x${PACK_PLATFORM}" = "xandroid" ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc.cfg dragon_toc.cfg
			elif [ "x${PACK_PLATFORM}" = "xdragonboard" ] ; then
				cp -v ${PACK_TOPDIR}/target/allwinner/generic/sign_config/dragon_toc_linux.cfg dragon_toc.cfg
			fi
		fi
	fi

	if [ $? -ne 0 ]
	then
		pack_error "dragon toc config file is not exist"
		exit 1
	fi

	rm -f cardscript.fex
	mv cardscript_secure.fex cardscript.fex
	if [ $? -ne 0 ]
	then
		pack_error "dragon cardscript_secure.fex file is not exist"
		exit 1
	fi

	dragonsecboot -toc0 dragon_toc.cfg ${ROOT_DIR}/keys  > /dev/null
	if [ $? -ne 0 ]
	then
		pack_error "dragon toc0 run error"
		exit 1
	fi

	update_toc0  toc0.fex           sys_config.bin
	if [ $? -ne 0 ]
	then
		pack_error "update toc0 run error"
		exit 1
	fi

	dragonsecboot -toc1 dragon_toc.cfg ${ROOT_DIR}/keys ${PACK_TOPDIR}/target/allwinner/generic/sign_config/cnf_base.cnf
	if [ $? -ne 0 ]
	then
		pack_error "dragon toc1 run error"
		exit 1
	fi

	echo "secure signature ok!"
}

function do_pack_dragonboard()
{
	printf "packing for dragonboard\n"

	rm -rf vmlinux.fex
	rm -rf boot.fex
	rm -rf rootfs.fex
	rm -rf kernel.fex
	rm -rf rootfs_squashfs.fex

	ln -s ${ROOT_DIR}/boot.img        boot.fex
	ln -s ${ROOT_DIR}/rootfs.img     rootfs.fex

	if [ "x${PACK_SIG}" = "xsecure" ] ; then
		do_signature
	else
		echo "normal"
	fi
}

function do_pack_tina()
{
	printf "packing for tina linux\n"

	rm -rf vmlinux.fex
	rm -rf boot.fex
	rm -rf rootfs.fex
	rm -rf kernel.fex
	rm -rf rootfs_squashfs.fex
	#ln -s ${ROOT_DIR}/vmlinux.tar.bz2 vmlinux.fex
	ln -s ${ROOT_DIR}/boot.img        boot.fex
	ln -s ${ROOT_DIR}/rootfs.img     rootfs.fex

	# Those files is ready for SPINor.
	#ln -s ${ROOT_DIR}/uImage          kernel.fex
	#ln -s ${ROOT_DIR}/rootfs.squashfs rootfs_squashfs.fex

	if [ "x${PACK_SIG}" = "xsecure" ] ; then
		echo "secure"
		do_signature
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		echo "prev_refurbish"
		do_signature
	else
		echo "normal"
	fi
}

do_prepare
do_ini_to_dts
do_common
do_pack_${PACK_PLATFORM}
do_finish
