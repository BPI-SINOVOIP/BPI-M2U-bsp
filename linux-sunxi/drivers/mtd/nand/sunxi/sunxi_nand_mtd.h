/*
 * sunxi_nand_mtd.h
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
#ifndef __SUNXI_NAND_H__
#define __SUNXI_NAND_H__

#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_mtd.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include "physic_v2/nand_physic_inc.h"

#define CONFIG_SUNXI_NAND_MTD_DEBUG

#if 1
#define	SUNXI_PRINTK_DEBUG
#endif

/*
 * Debugging macro and defines
 */
#define MTD_DEBUG_LEVEL0	(0)	/* Quiet   */
#define MTD_DEBUG_LEVEL1	(1) /* Audible */
#define MTD_DEBUG_LEVEL2	(2) /* Loud    */
#define MTD_DEBUG_LEVEL3	(3) /* Noisy   */

#define CONFIG_MTD_DEBUG_VERBOSE	7

#ifdef CONFIG_SUNXI_NAND_MTD_DEBUG

#ifdef SUNXI_PRINTK_DEBUG
#define SUNXI_DBG(args...)	MTD_DEBUG(MTD_DEBUG_LEVEL0, args)
#else
#define SUNXI_DBG(args...)
#endif

#define MTD_DEBUG(n, args...)				\
	do {						\
		if (n <= CONFIG_MTD_DEBUG_VERBOSE)	\
			printk(KERN_INFO args);		\
	} while (0)
#else /* CONFIG_SUNXI_NAND_MTD_DEBUG */
#define SUNXI_DBG(args...)
#define MTD_DEBUG(n, args...)				\
	do {						\
		if (0)					\
			printk(KERN_INFO args);		\
	} while (0)
#endif /* CONFIG_SUNXI_NAND_MTD_DEBUG */

/*
 * #define pr_info(args...)	MTDDEBUG(MTD_DEBUG_LEVEL0, args)
 * #define pr_warn(args...)	MTDDEBUG(MTD_DEBUG_LEVEL0, args)
 * #define pr_err(args...)	MTDDEBUG(MTD_DEBUG_LEVEL0, args)
 */

#define MAX_NAND_DATA_PAGE_SHIFT	4
#define MAX_NAND_DATA_PAGE_NUM		(1UL << MAX_NAND_DATA_PAGE_SHIFT)
#define MAX_NAND_DATA_LEN		(MAX_NAND_DATA_PAGE_NUM << 12)

#define SUNXI_MAX_SECS_PER_PAGE		64
#define SUNXI_MAX_PAGE_SIZE		(SUNXI_MAX_SECS_PER_PAGE * 1024)
#define SUNXI_NAND_W_FLAG		0x01
#define BOOT0_1_USED_PHY_NAND_BLK_CNT	8

#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern void	*NDFC0_BASE_ADDR;
extern struct device *ndfc_dev;
extern struct platform_device *plat_dev_nand;

extern spinlock_t nand_int_lock;

extern void nand_cfg_setting(void);
extern int nand_wait_rb_mode(void);
extern  int nand_wait_dma_mode(void);
extern void do_nand_interrupt(u32 no);
extern int NandHwExit(void);

extern int nand_info_init(struct _nand_info *nand_info, uchar chip,
	uint16 start_block, uchar *mbr_data);
extern int PHY_VirtualPageWrite(unsigned int nDieNum, unsigned int nBlkNum,
	unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare);
extern int PHY_VirtualPageRead(unsigned int nDieNum, unsigned int nBlkNum,
	unsigned int nPage, uint64 SectBitmap, void *pBuf, void *pSpare);
extern int PHY_VirtualBlockErase(unsigned int nDieNum, unsigned int nBlkNum);
extern int PHY_VirtualBadBlockCheck(unsigned int nDieNum, unsigned int nBlkNum);
extern int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum);
extern int NandHwSuperResume(void);
extern int NandHwSuperStandby(void);

#endif
