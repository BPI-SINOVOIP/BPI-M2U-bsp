/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Description: MMC driver for general mmc operations
 * Author: Aaron <leafy.myeh@allwinnertech.com>
 * Date: 2012-2-3 14:18:18
 */

#ifndef _MMC_DEF_H_
#define _MMC_DEF_H_

#include <common.h>
#include <asm/io.h>
#include <asm/arch/ccmu.h>
#include "asm/arch/archdef.h"

#define SUNXI_MMC0_BASE             (SUNXI_SMHC0_BASE)
#define SUNXI_MMC1_BASE             (SUNXI_SMHC1_BASE)
#define SUNXI_MMC2_BASE             (SUNXI_SMHC2_BASE)


#define MAX_MMC_NUM         4

#define MMC_TRANS_BY_DMA
//#define MMC_DEBUG
#define MMC_REG_FIFO_OS     (0x200)

#define MMC_REG_BASE        SUNXI_MMC0_BASE
#define CCMU_HCLKGATE0_BASE CCMU_BUS_CLK_GATING_REG0
#define CCMU_HCLKRST0_BASE  CCMU_BUS_SOFT_RST_REG0
#define CCMU_MMC0_CLK_BASE  CCMU_SDMMC0_CLK_REG
#define CCMU_MMC2_CLK_BASE  CCMU_SDMMC2_CLK_REG
#define CCMU_MMC3_CLK_BASE  CCMU_SDMMC3_CLK_REG



//#define CCMU_PLL5_CLK_BASE    0x01c20020
#define __mmc_be32_to_cpu(x)    ((0x000000ff&((x)>>24)) | (0x0000ff00&((x)>>8)) |           \
                                 (0x00ff0000&((x)<< 8)) | (0xff000000&((x)<<24)))

#ifndef NULL
#define NULL (void*)0
#endif

#ifdef MMC_DEBUG
#define mmcinfo(fmt...) printf("[mmc]: "fmt)
#define mmcdbg(fmt...)  printf("[mmc]: "fmt)
#define mmcmsg(fmt...)  printf(fmt)
#else
#define mmcinfo(fmt...) printf("[mmc]: "fmt)
#define mmcdbg(fmt...)
#define mmcmsg(fmt...)
#endif

#define DMAC_DES_BASE_IN_SRAM       (0x20000 + 0xC000)
#if !defined(CONFIG_ARCH_SUN3IW1P1)
#define DMAC_DES_BASE_IN_SDRAM      (0x42000000)
#define DRAM_START_ADDR             (0x40000000)
#else
#define DMAC_DES_BASE_IN_SDRAM      (0x82000000)
#define DRAM_START_ADDR             (0x80000000)
#endif

#define DRIVER_VER  "2015-09-24 17:25"

#endif /* _MMC_H_ */
