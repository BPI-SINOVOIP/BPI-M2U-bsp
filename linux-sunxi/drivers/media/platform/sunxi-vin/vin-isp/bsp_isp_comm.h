
/*
 ******************************************************************************
 *
 * bsp_isp_comm.h
 *
 * Hawkview ISP - bsp_isp_comm.h module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/12/25	    First Version
 *
 ******************************************************************************
 */

#ifndef __BSP__ISP__COMM__H
#define __BSP__ISP__COMM__H

#define ALIGN_4K(x)	(((x) + (4095)) & ~(4095))
#define ALIGN_32B(x)	(((x) + (31)) & ~(31))
#define ALIGN_16B(x)	(((x) + (15)) & ~(15))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define CLIP(a, i, s)	(((a) > (s)) ? (s) : MAX(a, i))

enum isp_platform {
	ISP_PLATFORM_SUN8IW12P1,
	ISP_PLATFORM_SUN8IW17P1,

	ISP_PLATFORM_NUM,
};
/*
 *  update table
 */
#define LUT_UPDATE	(1 << 3)
#define LINEAR_UPDATE	(1 << 3)
#define LENS_UPDATE	(1 << 4)
#define GAMMA_UPDATE	(1 << 5)
#define DRC_UPDATE	(1 << 6)
#define DISC_UPDATE	(1 << 7)
#define SATU_UPDATE	(1 << 8)
#define WDR_UPDATE	(1 << 9)
#define TDNF_UPDATE	(1 << 10)
#define PLTM_UPDATE	(1 << 11)
#define CEM_UPDATE	(1 << 12)
#define CONTRAST_UPDATE	(1 << 13)

#define TABLE_UPDATE_ALL 0xffffffff

/*
 *  ISP Module enable
 */
#define ISP_AE_EN	(1 << 0)
#define OBC_EN		(1 << 1)
#define LC_EN		(1 << 1)
#define LUT_DPC_EN	(1 << 2)
#define OTF_DPC_EN	(1 << 3)
#define BDNF_EN		(1 << 4)
#define ISP_AWB_EN	(1 << 6)
#define WB_EN		(1 << 7)
#define LSC_EN		(1 << 8)
#define BGC_EN		(1 << 9)
#define SAP_EN		(1 << 10)
#define AF_EN		(1 << 11)
#define RGB2RGB_EN	(1 << 12)
#define RGB_DRC_EN	(1 << 13)
#define TDF_EN		(1 << 15)
#define AFS_EN		(1 << 16)
#define HIST_EN		(1 << 17)
#define YCbCr_GAIN_OFFSET_EN	(1 << 18)
#define YCbCr_DRC_EN	(1 << 19)
#define TG_EN		(1 << 20)
#define ROT_EN		(1 << 21)
#define CONTRAST_EN	(1 << 22)
#define CNR_EN		(1 << 23)
#define SATU_EN		(1 << 24)
#define DISC_EN		(1 << 25)
#define SRC1_EN		(1 << 30)
#define SRC0_EN		(1 << 31)
#define ISP_MODULE_EN_ALL	(0xffffffff)

/*
 *  ISP interrupt enable
 */
#define FINISH_INT_EN		(1 << 0)
#define START_INT_EN		(1 << 1)
#define PARA_SAVE_INT_EN	(1 << 2)
#define PARA_LOAD_INT_EN	(1 << 3)
#define SRC0_FIFO_INT_EN	(1 << 4)
#define N_LINE_START_INT_EN	(1 << 7)
#define FRAME_ERROR_INT_EN	(1 << 8)
#define FRAME_LOST_INT_EN	(1 << 14)

#define ISP_IRQ_EN_ALL	0xffffffff

/*
 *  ISP interrupt status
 */
#define FINISH_PD	(1 << 0)
#define START_PD	(1 << 1)
#define PARA_SAVE_PD	(1 << 2)
#define PARA_LOAD_PD	(1 << 3)
#define SRC0_FIFO_OF_PD	(1 << 4)
#define N_LINE_START_PD	(1 << 7)
#define FRAME_ERROR_PD	(1 << 8)
#define CIN_FIFO_OF_PD	(1 << 9)
#define DPC_FIFO_OF_PD	(1 << 10)
#define D2D_FIFO_OF_PD	(1 << 11)
#define BIS_FIFO_OF_PD	(1 << 12)
#define CNR_FIFO_OF_PD	(1 << 13)
#define FRAME_LOST_PD	(1 << 14)
#define HB_SHROT_PD	(1 << 16)
#define D3D_HB_PD	(1 << 24)
#define PLTM_FIFO_OF_PD	(1 << 25)
#define D3D_WRITE_FIFO_OF_PD	(1 << 26)
#define D3D_READ_FIFO_OF_PD	(1 << 27)
#define D3D_WT2CMP_FIFO_OF_PD	(1 << 28)
#define WDR_WRITE_FIFO_OF_PD	(1 << 29)
#define WDR_WT2CMP_FIFO_OF_PD	(1 << 30)
#define WDR_READ_FIFO_OF_PD	(1 << 31)

#define ISP_IRQ_STATUS_ALL	0xffffffff

enum isp_channel {
	ISP_CH0 = 0,
	ISP_CH1 = 1,
	ISP_CH2 = 2,
	ISP_CH3 = 3,
	ISP_MAX_CH_NUM,
};

struct isp_size {
	unsigned int width;
	unsigned int height;
};

struct coor {
	unsigned int hor;
	unsigned int ver;
};

struct isp_size_settings {
	struct coor ob_start;
	struct isp_size ob_black;
	struct isp_size ob_valid;
};

enum ready_flag {
	PARA_NOT_READY = 0,
	PARA_READY = 1,
};

enum enable_flag {
	DISABLE = 0,
	ENABLE = 1,
};

enum isp_input_tables {
	LENS_GAMMA_TABLE = 0,
	DRC_TABLE = 1,
};

enum isp_input_seq {
	ISP_BGGR = 4,
	ISP_RGGB = 5,
	ISP_GBRG = 6,
	ISP_GRBG = 7,
};

enum isp_output_speed {
	ISP_OUTPUT_SPEED_0 = 0,
	ISP_OUTPUT_SPEED_1 = 1,
	ISP_OUTPUT_SPEED_2 = 2,
	ISP_OUTPUT_SPEED_3 = 3,
};

#endif
