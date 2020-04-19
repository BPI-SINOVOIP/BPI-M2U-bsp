#
# linux/drivers/mtd/nand/sunxi/Makefile
#

obj-$(CONFIG_MTD_NAND) += sunxi_mtd.o

sunxi_mtd-objs += sunxi_nand_mtd.o

NAND_LIB_DIR = ../../../../modules/nand/${LICHEE_CHIP}

sunxi_mtd-objs += \
	${NAND_LIB_DIR}/nand_interface.o \
	${NAND_LIB_DIR}/nfd/nand_ota_burn.o \
	${NAND_LIB_DIR}/nfd/nand_base.o \
	${NAND_LIB_DIR}/nfd/nand_blk.o \
	${NAND_LIB_DIR}/nfd/nand_dev.o \
	${NAND_LIB_DIR}/nfd/nand_class.o \
	${NAND_LIB_DIR}/nfd/nand_test.o \
	${NAND_LIB_DIR}/nfd/nand_osal_for_linux.o \
	${NAND_LIB_DIR}/nfd/nand_lib_cfg.o \
	${NAND_LIB_DIR}/libnand_sun50iw1p1.a
