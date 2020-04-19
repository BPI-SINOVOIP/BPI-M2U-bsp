
/*
 * Hawkview ISP - sun8iw11p1_vin_cfg.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SUN8IW11P1_VIN_CFG_H_
#define _SUN8IW11P1_VIN_CFG_H_

#define MIPI_CSI_NOT_EXIST

#define CSI0_REGS_BASE        			0x01c09000	/* 0x01c09000 */
#define CSI1_REGS_BASE        			0X01c1D000	/* 0X01c1D000 */

#define CSI0_CCI_REG_BASE			0x01cb3000
#define MIPI_CSI0_REGS_BASE    			0x01cb1000

#define ISP_REGS_BASE           		0x01cb8000
#define GPIO_REGS_VBASE				0x01c20800
#define CPU_DRAM_PADDR_ORG 			0x40000000
#define HW_DMA_OFFSET				0x00000000

/*set vin core clk base on sensor size*/
#define CORE_CLK_RATE_FOR_2M (108*1000*1000)
#define CORE_CLK_RATE_FOR_3M (216*1000*1000)
#define CORE_CLK_RATE_FOR_5M (216*1000*1000)
#define CORE_CLK_RATE_FOR_8M (432*1000*1000)
#define CORE_CLK_RATE_FOR_16M (432*1000*1000)

/*CSI & ISP size configs*/

#define CSI0_REG_SIZE               0x1000
#define MIPI_CSI_REG_SIZE           0x1000
#define MIPI_DPHY_REG_SIZE          0x1000
#define CSI0_CCI_REG_SIZE          0x1000
#define CSI1_REG_SIZE               0x1000
#define CSI1_CCI_REG_SIZE          0x1000
#define ISP_REG_SIZE                0x1000
#define ISP_LOAD_REG_SIZE           0x1000
#define ISP_SAVED_REG_SIZE          0x1000

/*ISP size configs*/

/*stat size configs*/

#define ISP_STAT_TOTAL_SIZE         0xAB00

#define ISP_STAT_HIST_MEM_SIZE      0x0200
#define ISP_STAT_AE_MEM_SIZE        0x4800
#define ISP_STAT_AF_MEM_SIZE        0x0500
#define ISP_STAT_AFS_MEM_SIZE       0x0200
#define ISP_STAT_AWB_RGB_MEM_SIZE   0x4800
#define ISP_STAT_AWB_CNT_MEM_SIZE   0x0C00
#define ISP_STAT_PLTM_LST_MEM_SIZE  0x0600

#define ISP_STAT_HIST_MEM_OFS       0x0000
#define ISP_STAT_AE_MEM_OFS         0x0200
#define ISP_STAT_AF_MEM_OFS         0x4a00
#define ISP_STAT_AFS_MEM_OFS        0x4f00
#define ISP_STAT_AWB_MEM_OFS        0x5100
#define ISP_STAT_PLTM_LST_MEM_OFS   0xa500

/*table size configs*/

#define ISP_TABLE_MAPPING1_SIZE		0x5a00
#define ISP_LSC_MEM_SIZE		(256*8)
#define ISP_GAMMA_MEM_SIZE		(256*4)
#define ISP_LINEAR_MEM_SIZE		(256*6)
#define ISP_WDR_GAMMA_FE_MEM_SIZE	(4096*2)
#define ISP_WDR_GAMMA_BE_MEM_SIZE	(4096*2)
#define ISP_TDNF_DIFF_MEM_SIZE		(256*1)
#define ISP_PLTM_H_MEM_SIZE		(256*1)
#define ISP_PLTM_V_MEM_SIZE		(256*1)
#define ISP_PLTM_POW_MEM_SIZE		(256*2)
#define ISP_PLTM_F_MEM_SIZE		(256*2)
#define ISP_CONTRAST_PE_MEM_SIZE	(128*2)

#define ISP_TABLE_MAPPING2_SIZE		0x1f00
#define ISP_DRC_MEM_SIZE		(256*2)
#define ISP_SATURATION_MEM_SIZE		(256*2)
#define ISP_CEM_MEM_SIZE		(736*8)

#define ISP_LSC_MEM_OFS			0x0
#define ISP_GAMMA_MEM_OFS		0x0800
#define ISP_LINEAR_MEM_OFS		0x0c00
#define ISP_WDR_GAMMA_FE_MEM_OFS	0x1200
#define ISP_WDR_GAMMA_BE_MEM_OFS	0x3200
#define ISP_TDNF_DIFF_MEM_OFS		0x5200
#define ISP_PLTM_H_MEM_OFS		0x5300
#define ISP_PLTM_V_MEM_OFS		0x5400
#define ISP_PLTM_POW_MEM_OFS		0x5500
#define ISP_PLTM_F_MEM_OFS		0x5700
#define ISP_CONTRAST_PE_MEM_OFS		0x5900

#define ISP_DRC_MEM_OFS			0x0
#define ISP_SATURATION_MEM_OFS		0x0600
#define ISP_CEM_MEM_OFS			0x0800

#define VIN_MAX_DEV			2
#define VIN_MAX_CSI			2
#define VIN_MAX_CCI			2
#define VIN_MAX_MIPI			0
#define VIN_MAX_ISP			2
#define VIN_MAX_SCALER			2

#define MAX_CH_NUM      4

#endif /*_SUN8IW11P1_VIN_CFG_H_*/
