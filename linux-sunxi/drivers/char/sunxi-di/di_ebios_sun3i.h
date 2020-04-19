/*
 * Allwinner SoCs de-interlace driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DI_EBIOS_SUN3I_H__
#define __DI_EBIOS_SUN3I_H__
#include <linux/types.h>

#define MAX_SIZE_IN_HORIZONAL 736
#define MAX_SIZE_IN_VERTICAL 288
#define PIXELS_PER_WORLD 16

typedef struct __SCAL_SRC_TYPE {
	/* 0:plannar;
	 * 1: interleaved;
	 * 2: plannar uv combined;
	 * 4: plannar mb;
	 * 6: uv combined mb
	 */
	__u8  mod;
	/* 0:yuv444; 1: yuv422;
	 * 2: yuv420; 3:yuv411;
	 * 4: csi rgb; 5:rgb888
	 */
	__u8  fmt;
	__u8  ps;
} __di_src_type_t;

typedef struct __SCAL_OUT_TYPE {
	/* 0:plannar rgb; 1: argb(byte0,byte1, byte2, byte3);
	 * 2:bgra; 4:yuv444; 5:yuv420; 6:yuv422; 7:yuv411
	 */
	__u8    fmt;
	__u8    ps;
	/* output alpha channel enable, valid when rgb888fmt */
	__u8    alpha_en;
} __di_out_type_t;

typedef struct __SCAL_SRC_SIZE {
	__u32   src_width;
	__u32   src_height;
	__u32   scal_width;
	__u32   scal_height;
} __di_src_size_t;

typedef struct __SCAL_OUT_SIZE {
	__u32   width;
	__u32   height;
	__u32   fb_width;
	__u32   fb_height;
} __di_out_size_t;

typedef struct _SCAL_BUF_ADDR {
	__u32   ch0_addr;
	__u32   ch1_addr;
	__u32   ch2_addr;
} __di_buf_addr_t;

typedef enum {
	DI_BGRA = 0,
	DI_ARGB = 1,
	DI_AYUV = 0,
	DI_VUYA = 1,
	DI_UVUV = 0,
	DI_VUVU = 1,
	DI_UYVY = 0,
	DI_YUYV = 1,
	DI_VYUY = 2,
	DI_YVYU = 3,
	DI_RGB565 = 0,
	DI_BGR565 = 1,
	DI_ARGB4444 = 0,
	DI_BGRA4444 = 1,
	DI_ARGB1555 = 0,
	DI_BGRA5551 = 1
} __di_ps_t;

typedef enum {
	DI_PLANNAR = 0,
	DI_INTERLEAVED,
	DI_UVCOMBINED,
	DI_PLANNARMB = 4,
	DI_UVCOMBINEDMB = 6
} __di_inmode_t;

typedef enum {
	DI_INYUV444 = 0,
	DI_INYUV422,
	DI_INYUV420,
	DI_INYUV411,
	DI_INRGB565,
	DI_INRGB888,
	DI_INRGB4444,
	DI_INRGB1555
} __di_infmt_t;

typedef enum {
	DI_OUTPRGB888 = 0,
	DI_OUTI0RGB888,
	DI_OUTI1RGB888,
	DI_OUTPYUV444 = 4,
	DI_OUTPYUV420,
	DI_OUTPYUV422,
	DI_OUTPYUV411,
	DI_OUTUVCYUV420 = 13
} __di_outfmt_t;

typedef union {
	u32 dwval;
	struct {
		u32 start:1; /* Default: 0x0; */
		u32 res0:30;
		u32 reset:1; /* Default: 0x0; */
	} bits;
} DI_CTRL_REG;

typedef union {
	u32 dwval;
	struct {
		u32 int_en:1; /* Default: 0x0; */
		u32 res0:31;
	} bits;
} DI_INT_CTRL_REG;

typedef union {
	u32 dwval;
	struct {
		u32 finish_sts:1; /* Default: 0x0;*/
		u32 res0:7;
		u32 busy:1; /* Default: 0x0; */
		u32 res1:7;
		u32 line_cnt:10; /* Default: 0x0; */
		u32 res2:6;
	} bits;
} DI_STATUS_REG;

typedef union {
	u32 dwval;
	struct {
		u32 width:10; /* Default: 0x0; */
		u32 res0:6;
		u32 height:10; /* Default: 0x0; */
		u32 res1:6;
	} bits;
} DI_SIZE_REG;

typedef union {
	u32 dwval;
	struct {
		/* Default: 0x0;
		 * 0:Non-tile-based;
		 * 1:Tile-based)
		 */
		u32 fmt:2;
		u32 res0:2;
		/* Default: 0x0; (Pixel sequence reversion enable:
		 * 0 disable, 1 enable)
		 */
		u32 ps_rev:1;
		u32 res1:27;
	} bits;
} DI_FORMAT_REG;

typedef union {
	u32 dwval;
	struct {
		u32 ls:16; /* Default: 0x0;*/
		u32 res0:16; /* Default: ;*/
	} bits;
} DI_LS_REG;

typedef union {
	u32 dwval;
	struct {
		u32 addr:31; /* Default: 0x0; */
	} bits;
} DI_ADD_REG;

typedef union {
	u32 dwval;
	struct {
		u32 minlumath:8; /* Default: 0x9; */
		u32 avglumashifter:8; /* Default: 0x6; */
		u32 spatial_th2:8; /* Default: 0xa; */
		u32 chromadiffth:8; /* Default: 0x5; */
	} bits;
} DI_PARA_REG;

typedef struct {
	DI_CTRL_REG		ctrl; /* 0x0000 */
	DI_INT_CTRL_REG		intr; /* 0x0004 */
	DI_STATUS_REG		status; /* 0x0008 */
	u32					res0; /* 0x000c */
	DI_SIZE_REG			size; /* 0x0010 */
	DI_FORMAT_REG		fmt; /* 0x0014 */
	u32					res1[2]; /* 0x0018-1c */
	DI_LS_REG			inls0; /* 0x0020 */
	DI_LS_REG			inls1; /* 0x0024 */
	DI_LS_REG			outls0; /* 0x0028 */
	DI_LS_REG			outls1; /* 0x002C */
	DI_LS_REG			flagls; /* 0x0030 */
	u32					res2[3]; /* 0x0034-3c */
	DI_ADD_REG			curadd0; /* 0x0040 */
	DI_ADD_REG			curadd1; /* 0x0044 */
	u32				 res3[2]; /* 0x0048-4c */
	DI_ADD_REG			preadd0; /* 0x0050 */
	DI_ADD_REG			preadd1; /* 0x0054 */
	u32				 res4[2]; /* 0x0058-5c */
	DI_ADD_REG			outadd0; /* 0x0060 */
	DI_ADD_REG			outadd1; /* 0x0064 */
	u32				 res5[2]; /* 0x0068-6c */
	DI_ADD_REG			flagadd; /* 0x0070 */
	u32				 res6[3]; /* 0x0074-7c */
	DI_PARA_REG			para; /* 0x0080 */
} __di_dev_t;

__s32 DI_Init(void);
__s32 DI_Config_Src(__di_buf_addr_t *addr, __di_src_size_t *size,
			__di_src_type_t *type);
__s32 DI_Set_Scaling_Factor(__di_src_size_t *in_size,
			__di_out_size_t *out_size);
__s32 DI_Set_Scaling_Coef(__di_src_size_t *in_size, __di_out_size_t *out_size,
			__di_src_type_t *in_type,  __di_out_type_t *out_type);
__s32 DI_Set_Out_Format(__di_out_type_t *out_type);
__s32 DI_Set_Out_Size(__di_out_size_t *out_size);
__s32 DI_Set_Writeback_Addr(__di_buf_addr_t *addr);
__s32 DI_Set_Writeback_Addr_ex(__di_buf_addr_t *addr, __di_out_size_t *size,
			__di_out_type_t *type);
__s32 DI_Set_Di_Ctrl(__u8 en, __u8 mode, __u8 diagintp_en, __u8 tempdiff_en);
__s32 DI_Set_Di_PreFrame_Addr(__u32 luma_addr, __u32 chroma_addr);
__s32 DI_Set_Di_MafFlag_Src(__u32 cur_addr, __u32 pre_addr, __u32 stride);
__s32 DI_Set_Di_Field(u32 field);
__s32 DI_Set_Reg_Rdy(void);
__s32 DI_Enable(void);
__s32 DI_Module_Enable(void);
__s32 DI_Set_Reset(void);
__s32 DI_Set_Irq_Enable(__u32 enable);
__s32 DI_Clear_irq(void);
__s32 DI_Get_Irq_Status(void);
__s32 DI_Set_Writeback_Start(void);
__s32 DI_Internal_Set_Clk(__u32 enable);
__u32 DI_VAtoPA(__u32 va);

#endif
