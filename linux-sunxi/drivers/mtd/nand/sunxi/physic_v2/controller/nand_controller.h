/*
**************************************************************************
* eNand
* Nand flash driver scan module
*
* Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
* All Rights Reserved
*
* File Name : nand_controller.h
*
* Author :
*
* Version : v0.1
*
* Date : 2013-11-20
*
* Description :
*
* Others : None at present.
*
*
*
**************************************************************************
*/

#ifndef _NAND_CONTROLLER_H
#define _NAND_CONTROLLER_H

#include "../nand_drv_cfg.h"

#define MAX_CHANNEL		(NAND_GetMaxChannelCnt())
#define MAX_CHIP_PER_CHANNEL	4
#define MAX_CMD_PER_LIST	16

typedef enum _nand_if_type {
	SDR = 0,
	ONFI_DDR = 0x2,
	ONFI_DDR2 = 0x12,
	TOG_DDR = 0x3,
	TOG_DDR2 = 0x13,
} nand_if_type;

enum _cmd_type {
	CMD_TYPE_NORMAL = 0,
	CMD_TYPE_BATCH,
	CMD_TYPE_MAX_CNT,
};

enum _ecc_layout_type {
	ECC_LAYOUT_INTERLEAVE = 0,
	ECC_LAYOUT_SEQUENCE,
	ECC_LAYOUT_TYPE_MAX_CNT,
};

enum _dma_mode {
	DMA_MODE_GENERAL_DMA = 0,
	DMA_MODE_MBUS_DMA,
	DMA_MODE_MAX_CNT
};

enum _ndfc_version {
	NDFC_VERSION_V1 = 1,
	NDFC_VERSION_V2,
	NDFC_VERSION_MAX_CNT
};

struct _nctri_cmd {
	u32	  cmd_valid;

	u32	  cmd;
	u32	  cmd_send;
	u32	  cmd_wait_rb;

	u8	  cmd_addr[MAX_CMD_PER_LIST];
	u32	  cmd_acnt;

	u32	  cmd_trans_data_nand_bus;
	u32	  cmd_swap_data;
	u32	  cmd_swap_data_dma;
	u32	  cmd_direction;
	u32	  cmd_mdata_len;
/* u32	cmd_mdata_addr; */
	u8	  *cmd_mdata_addr;
};

struct _nctri_cmd_seq {
	u32	  cmd_type;
	u32	  ecc_layout;
	u32	  row_addr_auto_inc;
	struct _nctri_cmd nctri_cmd[MAX_CMD_PER_LIST];
	u32	  re_start_cmd;
	u32	  re_end_cmd;
	u32	  re_cmd_times;
};

struct _nand_controller_reg {
	volatile u32 *reg_ctl;
	volatile u32 *reg_sta;
	volatile u32 *reg_int;
	volatile u32 *reg_timing_ctl;
	volatile u32 *reg_timing_cfg;
	volatile u32 *reg_addr_low;
	volatile u32 *reg_addr_high;
	volatile u32 *reg_sect_num;
	volatile u32 *reg_byte_cnt;
	volatile u32 *reg_cmd;
	volatile u32 *reg_read_cmd_set;
	volatile u32 *reg_write_cmd_set;
	volatile u32 *reg_io;
	volatile u32 *reg_ecc_ctl;
	volatile u32 *reg_ecc_sta;
	volatile u32 *reg_debug;
	volatile u32 *reg_err_cnt0;
	volatile u32 *reg_err_cnt1;
	volatile u32 *reg_err_cnt2;
	volatile u32 *reg_err_cnt3;
	volatile u32 *reg_user_data_base;
	volatile u32 *reg_efnand_sta;
	volatile u32 *reg_spare_area;
	volatile u32 *reg_pat_id;
	volatile u32 *reg_mbus_dma_addr;
	volatile u32 *reg_dma_cnt;
	volatile u32 *reg_ram0_base;
	volatile u32 *reg_ram1_base;
	volatile u32 *reg_dma_sta;
};

struct _nand_controller_reg_bak {
	u32 reg_ctl;
	u32 reg_sta;
	u32 reg_int;
	u32 reg_timing_ctl;
	u32 reg_timing_cfg;
	u32 reg_addr_low;
	u32 reg_addr_high;
	u32 reg_sect_num;
	u32 reg_byte_cnt;
	u32 reg_cmd;
	u32 reg_read_cmd_set;
	u32 reg_write_cmd_set;
	u32 reg_io;
	u32 reg_ecc_ctl;
	u32 reg_ecc_sta;
	u32 reg_debug;
	u32 reg_err_cnt0;
	u32 reg_err_cnt1;
	u32 reg_err_cnt2;
	u32 reg_err_cnt3;
	u32 reg_user_data_base;
	u32 reg_efnand_sta;
	u32 reg_spare_area;
	u32 reg_pat_id;
	u32 reg_mbus_dma_addr;
	u32 reg_dma_cnt;
	u32 reg_ram0_base;
	u32 reg_ram1_base;
	u32 reg_dma_sta;
};

/* define the nand flash storage system information */
struct _nand_controller_info {
	#define MAX_ECC_BLK_CNT		16

	struct _nand_controller_info *next;
	u32 type;	/* ndfc type */
	u32 channel_id; /* 0: channel 0; 1: channel 1; */
	u32 chip_cnt;
	u32 chip_connect_info;
	u32 rb_connect_info;
	u32 max_ecc_level;

	struct _nctri_cmd_seq nctri_cmd_seq;

	u32 ce[8];
	u32 rb[8];
	u32 dma_type; /* 0: general dma; 1: mbus dma; */
	u32 dma_addr;

	u32 current_op_type;
	u32 write_wait_rb_before_cmd_io;
	u32 write_wait_rb_mode;
	u32 write_wait_dma_mode;
	u32 rb_ready_flag;
	u32 dma_ready_flag;
	u32 dma_channel;
	u32 nctri_flag;
	u32 ddr_timing_ctl[MAX_CHIP_PER_CHANNEL];
	u32 ddr_scan_blk_no[MAX_CHIP_PER_CHANNEL];

	struct _nand_controller_reg nreg;
	struct _nand_controller_reg_bak nreg_bak;

	struct _nand_chip_info *nci;
};

#define NDFC_READ_REG(n)	(*((volatile u32 *)(n)))
#define NDFC_WRITE_REG(n, c)	(*((volatile u32 *)(n)) = (c))

/* define bit use in NFC_CTL */
#define NDFC_EN			(1 << 0)
#define NDFC_RESET		(1 << 1)
#define NDFC_BUS_WIDYH		(1 << 2)
#define NDFC_RB_SEL		(0x3 << 3)	/* 1 --> 0x3 */
#define NDFC_CE_SEL		(0xf << 24)	/* 7 --> 0xf */
#define NDFC_CE_CTL		(1 << 6)
#define NDFC_CE_CTL1		(1 << 7)
#define NDFC_PAGE_SIZE		(0xf << 8)
/* #define NDFC_SAM		(1 << 12) */
#define NDFC_RAM_METHOD		(1 << 14)
#define NDFC_DEBUG_CTL		(1 << 31)

/* define	bit use in NFC_ST */
#define NDFC_RB_B2R		(1 << 0)
#define NDFC_CMD_INT_FLAG	(1 << 1)
#define NDFC_DMA_INT_FLAG	(1 << 2)
#define NDFC_CMD_FIFO_STATUS	(1 << 3)
#define NDFC_STA		(1 << 4)
/* #define NDFC_NATCH_INT_FLAG	(1 << 5) */
#define NDFC_RB_STATE0		(1 << 8)
#define NDFC_RB_STATE1		(1 << 9)
#define NDFC_RB_STATE2		(1 << 10)
#define NDFC_RB_STATE3		(1 << 11)

/* define bit use in NFC_INT */
#define NDFC_B2R_INT_ENABLE	(1 << 0)
#define NDFC_CMD_INT_ENABLE	(1 << 1)
#define NDFC_DMA_INT_ENABLE	(1 << 2)

/* define bit use in NFC_CMD */
#define NDFC_CMD_LOW_BYTE	(0xff << 0)
/* #define NDFC_CMD_HIGH_BYTE	(0xff << 8) */
#define NDFC_ADR_NUM		(0x7 << 16)
#define NDFC_SEND_ADR		(1 << 19)
#define NDFC_ACCESS_DIR		(1 << 20)
#define NDFC_DATA_TRANS		(1 << 21)
#define NDFC_SEND_CMD1		(1 << 22)
#define NDFC_WAIT_FLAG		(1 << 23)
#define NDFC_SEND_CMD2		(1 << 24)
#define NDFC_SEQ		(1 << 25)
#define NDFC_DATA_SWAP_METHOD	(1 << 26)
#define NDFC_ROW_AUTO_INC	(1 << 27)
#define NDFC_SEND_CMD3		(1 << 28)
#define NDFC_SEND_CMD4		(1 << 29)
#define NDFC_CMD_TYPE		(3 << 30)

/* define bit use in NFC_RCMD_SET */
#define NDFC_READ_CMD		(0xff << 0)
#define NDFC_RANDOM_READ_CMD0	(0xff << 8)
#define NDFC_RANDOM_READ_CMD1	(0xff << 16)

/* define bit use in NFC_WCMD_SET */
#define NDFC_PROGRAM_CMD	(0xff << 0)
#define NDFC_RANDOM_WRITE_CMD	(0xff << 8)
#define NDFC_READ_CMD0		(0xff << 16)
#define NDFC_READ_CMD1		(0xff << 24)

/* define bit use in NFC_ECC_CTL */
#define NDFC_ECC_EN		(1 << 0)
#define NDFC_ECC_PIPELINE	(1 << 3)
#define NDFC_ECC_EXCEPTION	(1 << 4)
#define NDFC_ECC_BLOCK_SIZE	(1 << 5)
#define NDFC_RANDOM_EN		(1 << 9)
#define NDFC_RANDOM_DIRECTION	(1 << 10)
#define NDFC_ECC_MODE		(0xf << 12)
#define NDFC_RANDOM_SEED	(0x7fff << 16)

#define NDFC_IRQ_MAJOR		13
/* cmd flag bit */
#define NDFC_PAGE_MODE		0x1
#define NDFC_NORMAL_MODE	0x0

#define NDFC_DATA_FETCH		0x1
#define NDFC_NO_DATA_FETCH	0x0
#define NDFC_MAIN_DATA_FETCH	0x1
#define NDFC_SPARE_DATA_FETCH	0x0
#define NDFC_WAIT_RB		0x1
#define NDFC_NO_WAIT_RB		0x0
#define NDFC_IGNORE		0x0

#define NDFC_INT_RB		0
#define NDFC_INT_CMD		1
#define NDFC_INT_DMA		2
#define NDFC_INT_BATCh		5

/* define the mask for the nand flash operation status */
#define NAND_OPERATE_FAIL	(1<<0)
#define NAND_CACHE_READY	(1<<5)
/* #define NAND_STATUS_READY	(1<<6) */
#define NAND_WRITE_PROTECT	(1<<7)

#define CMD_RESET		0xFF
#define CMD_SYNC_RESET		0xFC
#define CMD_READ_STA		0x70
#define CMD_READ_ID		0x90
#define CMD_SET_FEATURE		0xEF
#define CMD_GET_FEATURE		0xEE
#define CMD_READ_PAGE_CMD1	0x00
#define CMD_READ_PAGE_CMD2	0x30
#define CMD_CHANGE_READ_ADDR_CMD1	0x05
#define CMD_CHANGE_READ_ADDR_CMD2	0xE0
#define CMD_WRITE_PAGE_CMD1		0x80
#define CMD_WRITE_PAGE_CMD2		0x10
#define CMD_CHANGE_WRITE_ADDR_CMD	0x85
#define CMD_ERASE_CMD1			0x60
#define CMD_ERASE_CMD2			0xD0

#endif /* _NAND_CONTROLLER_H */
