/*
 * Uboot_head.h
 *
 * Copyright (C) 2017 AllWinnertech Ltd.
 * Author: zhongguizhao <zhongguizhao@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef UBOOT_HEAD
#define UBOOT_HEAD


#define TOC_MAIN_INFO_MAGIC	0x89119800
#define PHY_INFO_SIZE		0x4000
#define PHY_INFO_MAGIC		0xaa55a5a5
#define UBOOT_STAMP_VALUE	0xAE15BC34

#define TOC_LEN_OFFSET_BY_INT	9
#define TOC_MAGIC_OFFSET_BY_INT	4


#define NAND_BOOT0_BLK_START	0
#define NAND_BOOT0_BLK_CNT	2
#define NAND_UBOOT_BLK_START	(NAND_BOOT0_BLK_START+NAND_BOOT0_BLK_CNT)


#define NAND_UBOOT_BLK_CNT		6
#define NAND_BOOT0_PAGE_CNT_PER_COPY	64
#define NAND_BOOT0_PAGE_CNT_PER_COPY_2	128
#define BOOT0_MAX_COPY_CNT		(8)


/* #define UBOOT_START_BLOCK		4 */
#define UBOOT_MAX_BLOCK_NUM		40

#define	 PHYSIC_RECV_BLOCK		10


struct _uboot_info {
	unsigned int  sys_mode;
	unsigned int  use_lsb_page;
	unsigned int  copys;

	unsigned int  uboot_len;
	unsigned int  total_len;
	unsigned int  uboot_pages;
	unsigned int  total_pages;

	unsigned int  blocks_per_total;
	unsigned int  page_offset_for_nand_info;
	unsigned int  byte_offset_for_nand_info;
	unsigned int  uboot_block[30];

	unsigned int  nboot_copys;
	unsigned int  nboot_len;
	unsigned int  nboot_data_per_page;

	unsigned int  nouse[64 - 43];
};

#endif

