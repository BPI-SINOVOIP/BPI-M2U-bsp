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

SUFFIX=""
ARCH=arm

export PATH=${PACK_TOPDIR}/sunxi-pack/common/host/bin/:$PATH

ROOT_DIR=${PACK_TOPDIR}/out/${PACK_BOARD}
COMMON_DIR="${PACK_TOPDIR}/sunxi-pack/common"
BOARD_DIR="${PACK_TOPDIR}/sunxi-pack/${PACK_CHIP}"

tools_file_list=(
${COMMON_DIR}/tools/split_xxxx.fex
${COMMON_DIR}/tools/usbtool_test.fex
${COMMON_DIR}/tools/cardscript.fex
${COMMON_DIR}/tools/cardscript_secure.fex
${COMMON_DIR}/tools/cardtool.fex
${COMMON_DIR}/tools/usbtool.fex
${COMMON_DIR}/tools/aultls32.fex
${COMMON_DIR}/tools/aultools.fex
)

configs_file_list=(
${COMMON_DIR}/toc/toc1.fex
${COMMON_DIR}/toc/toc0.fex
${COMMON_DIR}/toc/boot_package.fex
${COMMON_DIR}/configs/*.fex
${COMMON_DIR}/configs/*.cfg
${BOARD_DIR}/configs/default/*.fex
${BOARD_DIR}/configs/default/*.cfg
${BOARD_DIR}/configs/${PACK_BOARD}/*.fex
${BOARD_DIR}/configs/${PACK_BOARD}/*.cfg
)

boot_resource_list=(
${COMMON_DIR}/boot-resource/boot-resource:image/
${COMMON_DIR}/boot-resource/boot-resource.ini:image/
${BOARD_DIR}/configs/default/*.bmp:image/boot-resource/
${BOARD_DIR}/configs/${PACK_BOARD}/*.bmp:image/boot-resource/
)

boot_file_list=(
${BOARD_DIR}/bin/boot0_nand_${PACK_CHIP}.bin:image/boot0_nand.fex
${BOARD_DIR}/bin/boot0_sdcard_${PACK_CHIP}.bin:image/boot0_sdcard.fex
${BOARD_DIR}/bin/boot0_spinor_${PACK_CHIP}.bin:image/boot0_spinor.fex
${BOARD_DIR}/bin/fes1_${PACK_CHIP}.bin:image/fes1.fex
${BOARD_DIR}/bin/u-boot-${PACK_CHIP}.bin:image/u-boot.fex
${BOARD_DIR}/bin/u-boot-spinor-${PACK_CHIP}.bin:image/u-boot-spinor.fex
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
	local DEBUG_DIR="${COMMON_DIR}/debug"
	rm -rf ${ROOT_DIR}/image/awk_debug_card0
	touch ${ROOT_DIR}/image/awk_debug_card0
	TX=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${DEBUG_DIR}/card_debug_pin`
	RX=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${DEBUG_DIR}/card_debug_pin`
	PORT=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${DEBUG_DIR}/card_debug_pin`
	MS=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${DEBUG_DIR}/card_debug_pin`
	CK=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${DEBUG_DIR}/card_debug_pin`
	DO=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${DEBUG_DIR}/card_debug_pin`
	DI=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${DEBUG_DIR}/card_debug_pin`

	BOOT_UART_ST=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${DEBUG_DIR}/card_debug_string`
	BOOT_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${DEBUG_DIR}/card_debug_string`
	BOOT_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${DEBUG_DIR}/card_debug_string`
	BOOT_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${DEBUG_DIR}/card_debug_string`
	UART0_ST=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${DEBUG_DIR}/card_debug_string`
	UART0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${DEBUG_DIR}/card_debug_string`
	UART0_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${DEBUG_DIR}/card_debug_string`
	UART0_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $9}' ${DEBUG_DIR}/card_debug_string`
	UART0_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $10}' ${DEBUG_DIR}/card_debug_string`
	UART1_ST=`awk  '$0~"'$PACK_CHIP'"{print $11}' ${DEBUG_DIR}/card_debug_string`
	JTAG_ST=`awk  '$0~"'$PACK_CHIP'"{print $12}' ${DEBUG_DIR}/card_debug_string`
	MS_ST=`awk  '$0~"'$PACK_CHIP'"{print $13}' ${DEBUG_DIR}/card_debug_string`
	CK_ST=`awk  '$0~"'$PACK_CHIP'"{print $14}' ${DEBUG_DIR}/card_debug_string`
	DO_ST=`awk  '$0~"'$PACK_CHIP'"{print $15}' ${DEBUG_DIR}/card_debug_string`
	DI_ST=`awk  '$0~"'$PACK_CHIP'"{print $16}' ${DEBUG_DIR}/card_debug_string`
	MMC0_ST=`awk  '$0~"'$PACK_CHIP'"{print $17}' ${DEBUG_DIR}/card_debug_string`
	MMC0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $18}' ${DEBUG_DIR}/card_debug_string`

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
ENV_SUFFIX=
function do_early_prepare()
{
	# Cleanup
	rm -rf ${ROOT_DIR}/image
	
	mkdir -p ${ROOT_DIR}/image
}
function do_prepare()
{
	if [ -z "${PACK_CHIP}" -o -z "${PACK_PLATFORM}" -o -z "${PACK_BOARD}" ] ; then
		pack_error "Invalid parameters Chip: ${PACK_CHIP}, \
			Platform: ${PACK_PLATFORM}, Board: ${PACK_BOARD}"
		show_boards
		exit 1
	fi

	if [ ! -d ${BOARD_DIR}/configs/${PACK_BOARD} ] ; then
		pack_error "Board's directory \
			\"${BOARD_DIR}/configs/${PACK_BOARD}\" is not exist."
		show_boards
		exit 1
	fi

	PACK_KERN="linux-sunxi"

	printf "copying tools file\n"
	for file in ${tools_file_list[@]} ; do
		cp -f $file ${ROOT_DIR}/image/ 2> /dev/null
	done

	printf "copying configs file\n"
	for file in ${configs_file_list[@]} ; do
		cp -f $file ${ROOT_DIR}/image/ 2> /dev/null
	done
	# If platform config files exist, we will cover the default files
	# For example, mv out/image_linux.cfg out/image.cfg
	cd ${ROOT_DIR}
	find image/* -type f -a \( -name "*.fex" -o -name "*.cfg" \) -print | \
		sed "s#\(.*\)_${PACK_PLATFORM}\(\..*\)#mv -fv & \1\2#e"
	cd -

	printf "copying boot resource\n"
	for file in ${boot_resource_list[@]} ; do
		cp -rf `echo $file | awk -F: '{print $1}'` \
			${ROOT_DIR}/`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done

	printf "copying boot file\n"
	for file in ${boot_file_list[@]} ; do
		cp -f `echo $file | awk -F: '{print $1}'` \
			${ROOT_DIR}/`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done

	sed -i '/^burn_secure_mod/d' ${ROOT_DIR}/image/sys_config.fex
	sed -i '/^secure_without_OS/d' ${ROOT_DIR}/image/sys_config.fex

	# Here, we can switch uart to card or normal
	if [ "x${PACK_DEBUG}" = "xcard0" ] ; then \
		uart_switch
	fi

	sed -i 's/\\boot-resource/\/boot-resource/g' ${ROOT_DIR}/image/boot-resource.ini
	sed -i 's/\\\\/\//g' ${ROOT_DIR}/image/image.cfg
	sed -i 's/^imagename/;imagename/g' ${ROOT_DIR}/image/image.cfg

	IMG_NAME="${PACK_PLATFORM}_${PACK_BOARD}_${PACK_DEBUG}"

	echo "imagename = $IMG_NAME" >> ${ROOT_DIR}/image/image.cfg
	echo "" >> ${ROOT_DIR}/image/image.cfg
}

function do_ini_to_dts()
{
	PACK_DTB=$(echo ${PACK_BOARD%-*} | tr '[A-Z]' '[a-z]')
	local DTC_DEP_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-${PACK_BOARD}.dtb.d.dtc.tmp
	local DTC_SRC_PATH=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/
	local DTC_SRC_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-${PACK_BOARD}.dtb.dts.tmp

	local DTC_COMPILER=${PACK_TOPDIR}/${PACK_KERN}/scripts/dtc/dtc
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
		DTC_SRC_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-sd.dtb.dts.tmp
		DTC_DEP_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-sd.dtb.d.dtc.tmp
	    elif [ "x${PACK_STORAGE}" = "xemmc" ]; then
		DTC_SRC_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-emmc.dtb.dts.tmp
		DTC_DEP_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc-emmc.dtb.d.dtc.tmp
	    else
		DTC_SRC_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc.dtb.dts.tmp
		DTC_DEP_FILE=${PACK_TOPDIR}/${PACK_KERN}/arch/$ARCH/boot/dts/.${PACK_CHIP}-soc.dtb.d.dtc.tmp
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
	u_boot_env_gen env.cfg env.fex > /dev/null
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
}


function do_pack_linux()
{
	printf "packing for linux\n"
}


do_early_prepare
do_prepare
do_ini_to_dts
do_common
do_pack_${PACK_PLATFORM}
do_finish
