/* g2d_bsp.h
 *
 * Copyright (c)	2016 Allwinnertech Co., Ltd.
 *					2016 gqs
 *
 * G2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 */

#ifndef __G2D_BSP_H
#define __G2D_BSP_H

#include "linux/kernel.h"
#include "linux/mm.h"
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include "asm-generic/int-ll64.h"
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#ifndef __G2D_BSP_DRV_H
#define __G2D_BSP_DRV_H

#define G2D_FINISH_IRQ		(1<<8)
#define G2D_ERROR_IRQ			(1<<9)
/* data format */
typedef enum {
	G2D_FORMAT_ARGB8888,
	G2D_FORMAT_ABGR8888,
	G2D_FORMAT_RGBA8888,
	G2D_FORMAT_BGRA8888,
	G2D_FORMAT_XRGB8888,
	G2D_FORMAT_XBGR8888,
	G2D_FORMAT_RGBX8888,
	G2D_FORMAT_BGRX8888,
	G2D_FORMAT_RGB888,
	G2D_FORMAT_BGR888,
	G2D_FORMAT_RGB565,
	G2D_FORMAT_BGR565,
	G2D_FORMAT_ARGB4444,
	G2D_FORMAT_ABGR4444,
	G2D_FORMAT_RGBA4444,
	G2D_FORMAT_BGRA4444,
	G2D_FORMAT_ARGB1555,
	G2D_FORMAT_ABGR1555,
	G2D_FORMAT_RGBA5551,
	G2D_FORMAT_BGRA5551,
	G2D_FORMAT_ARGB2101010,
	G2D_FORMAT_ABGR2101010,
	G2D_FORMAT_RGBA1010102,
	G2D_FORMAT_BGRA1010102,

	/* invailed for UI channel */
	G2D_FORMAT_IYUV422_V0Y1U0Y0 = 0x20,
	G2D_FORMAT_IYUV422_Y1V0Y0U0,
	G2D_FORMAT_IYUV422_U0Y1V0Y0,
	G2D_FORMAT_IYUV422_Y1U0Y0V0,

	G2D_FORMAT_YUV422UVC_V1U1V0U0,
	G2D_FORMAT_YUV422UVC_U1V1U0V0,
	G2D_FORMAT_YUV422_PLANAR,

	G2D_FORMAT_YUV420UVC_V1U1V0U0 = 0x28,
	G2D_FORMAT_YUV420UVC_U1V1U0V0,
	G2D_FORMAT_YUV420_PLANAR,

	G2D_FORMAT_YUV411UVC_V1U1V0U0 = 0x2c,
	G2D_FORMAT_YUV411UVC_U1V1U0V0,
	G2D_FORMAT_YUV411_PLANAR,

	G2D_FORMAT_Y8 = 0x30,

	/* YUV 10bit format */
	G2D_FORMAT_YVU10_P010 = 0x34,

	G2D_FORMAT_YVU10_P210 = 0x36,

	G2D_FORMAT_YVU10_444 = 0x38,
	G2D_FORMAT_YUV10_444 = 0x39,
} g2d_fmt_enh;

typedef enum {
	/* share data format */
	G2D_FMT_ARGB_AYUV8888 = (0x0),
	G2D_FMT_BGRA_VUYA8888 = (0x1),
	G2D_FMT_ABGR_AVUY8888 = (0x2),
	G2D_FMT_RGBA_YUVA8888 = (0x3),

	G2D_FMT_XRGB8888 = (0x4),
	G2D_FMT_BGRX8888 = (0x5),
	G2D_FMT_XBGR8888 = (0x6),
	G2D_FMT_RGBX8888 = (0x7),

	G2D_FMT_ARGB4444 = (0x8),
	G2D_FMT_ABGR4444 = (0x9),
	G2D_FMT_RGBA4444 = (0xA),
	G2D_FMT_BGRA4444 = (0xB),

	G2D_FMT_ARGB1555 = (0xC),
	G2D_FMT_ABGR1555 = (0xD),
	G2D_FMT_RGBA5551 = (0xE),
	G2D_FMT_BGRA5551 = (0xF),

	G2D_FMT_RGB565 = (0x10),
	G2D_FMT_BGR565 = (0x11),

	G2D_FMT_IYUV422 = (0x12),

	G2D_FMT_8BPP_MONO = (0x13),
	G2D_FMT_4BPP_MONO = (0x14),
	G2D_FMT_2BPP_MONO = (0x15),
	G2D_FMT_1BPP_MONO = (0x16),

	G2D_FMT_PYUV422UVC = (0x17),
	G2D_FMT_PYUV420UVC = (0x18),
	G2D_FMT_PYUV411UVC = (0x19),

	/* just for output format */
	G2D_FMT_PYUV422 = (0x1A),
	G2D_FMT_PYUV420 = (0x1B),
	G2D_FMT_PYUV411 = (0x1C),

	/* just for input format */
	G2D_FMT_8BPP_PALETTE = (0x1D),
	G2D_FMT_4BPP_PALETTE = (0x1E),
	G2D_FMT_2BPP_PALETTE = (0x1F),
	G2D_FMT_1BPP_PALETTE = (0x20),

	G2D_FMT_PYUV422UVC_MB16 = (0x21),
	G2D_FMT_PYUV420UVC_MB16 = (0x22),
	G2D_FMT_PYUV411UVC_MB16 = (0x23),
	G2D_FMT_PYUV422UVC_MB32 = (0x24),
	G2D_FMT_PYUV420UVC_MB32 = (0x25),
	G2D_FMT_PYUV411UVC_MB32 = (0x26),
	G2D_FMT_PYUV422UVC_MB64 = (0x27),
	G2D_FMT_PYUV420UVC_MB64 = (0x28),
	G2D_FMT_PYUV411UVC_MB64 = (0x29),
	G2D_FMT_PYUV422UVC_MB128 = (0x2A),
	G2D_FMT_PYUV420UVC_MB128 = (0x2B),
	G2D_FMT_PYUV411UVC_MB128 = (0x2C),

} g2d_data_fmt;

typedef enum {
	G2D_SEQ_NORMAL = 0x0,

	/* for interleaved yuv422 */
	G2D_SEQ_VYUY = 0x1,
	G2D_SEQ_YVYU = 0x2,

	/* for uv_combined yuv420 */
	G2D_SEQ_VUVU = 0x3,

	/* for 16bpp rgb */
	G2D_SEQ_P10 = 0x4,
	G2D_SEQ_P01 = 0x5,

	/* planar format or 8bpp rgb */
	G2D_SEQ_P3210 = 0x6,
	G2D_SEQ_P0123 = 0x7,

	/* for 4bpp rgb */
	G2D_SEQ_P76543210 = 0x8,	/* 7,6,5,4,3,2,1,0 */
	G2D_SEQ_P67452301 = 0x9,	/* 6,7,4,5,2,3,0,1 */
	G2D_SEQ_P10325476 = 0xA,	/* 1,0,3,2,5,4,7,6 */
	G2D_SEQ_P01234567 = 0xB,	/* 0,1,2,3,4,5,6,7 */

	/* for 2bpp rgb */
	/* 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 */
	G2D_SEQ_2BPP_BIG_BIG = 0xC,
	/*12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3*/
	G2D_SEQ_2BPP_BIG_LITTER = 0xD,
	/*3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12*/
	G2D_SEQ_2BPP_LITTER_BIG = 0xE,
	/* 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 */
	G2D_SEQ_2BPP_LITTER_LITTER = 0xF,

	/* for 1bpp rgb */
	/* 31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,
	 * 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
	 */
	G2D_SEQ_1BPP_BIG_BIG = 0x10,
	/* 24,25,26,27,28,29,30,31,16,17,18,19,20,21,
	 * 22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7
	 */
	G2D_SEQ_1BPP_BIG_LITTER = 0x11,
	/* 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,
	 * 22,21,20,19,18,17,16,31,30,29,28,27,26,25,24
	 */
	G2D_SEQ_1BPP_LITTER_BIG = 0x12,
	 /* 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
	  * 17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	  */
	G2D_SEQ_1BPP_LITTER_LITTER = 0x13,
} g2d_pixel_seq;

typedef enum {
	G2D_FIL_NONE = 0x00000000,
	G2D_FIL_PIXEL_ALPHA = 0x00000001,
	G2D_FIL_PLANE_ALPHA = 0x00000002,
	G2D_FIL_MULTI_ALPHA = 0x00000004,
} g2d_fillrect_flags;

typedef enum {
	G2D_BLT_NONE = 0x00000000,
	G2D_BLT_PIXEL_ALPHA = 0x00000001,
	G2D_BLT_PLANE_ALPHA = 0x00000002,
	G2D_BLT_MULTI_ALPHA = 0x00000004,
	G2D_BLT_SRC_COLORKEY = 0x00000008,
	G2D_BLT_DST_COLORKEY = 0x00000010,
	G2D_BLT_FLIP_HORIZONTAL = 0x00000020,
	G2D_BLT_FLIP_VERTICAL = 0x00000040,
	G2D_BLT_ROTATE90 = 0x00000080,
	G2D_BLT_ROTATE180 = 0x00000100,
	G2D_BLT_ROTATE270 = 0x00000200,
	G2D_BLT_MIRROR45 = 0x00000400,
	G2D_BLT_MIRROR135 = 0x00000800,
	G2D_BLT_SRC_PREMULTIPLY = 0x00001000,
	G2D_BLT_DST_PREMULTIPLY = 0x00002000,
} g2d_blt_flags;

enum g2d_scan_order {
	G2D_SM_TDLR = 0x00000000,
	G2D_SM_TDRL = 0x00000001,
	G2D_SM_DTLR = 0x00000002,
	G2D_SM_DTRL = 0x00000003,
};

/*  */
typedef enum {
	G2D_BLT_NONE_0 = 0x0,
	G2D_BLT_BLACKNESS,
	G2D_BLT_NOTMERGEPEN,
	G2D_BLT_MASKNOTPEN,
	G2D_BLT_NOTCOPYPEN,
	G2D_BLT_MASKPENNOT,
	G2D_BLT_NOT,
	G2D_BLT_XORPEN,
	G2D_BLT_NOTMASKPEN,
	G2D_BLT_MASKPEN,
	G2D_BLT_NOTXORPEN,
	G2D_BLT_NOP,
	G2D_BLT_MERGENOTPEN,
	G2D_BLT_COPYPEN,
	G2D_BLT_MERGEPENNOT,
	G2D_BLT_MERGEPEN,
	G2D_BLT_WHITENESS = 0x000000ff,

	G2D_ROT_90 = 0x00000100,
	G2D_ROT_180 = 0x00000200,
	G2D_ROT_270 = 0x00000300,
	G2D_ROT_H = 0x00001000,
	G2D_ROT_V = 0x00002000,

/*	G2D_SM_TDLR_1  =    0x10000000, */
	G2D_SM_DTLR_1 = 0x10000000,
/*	G2D_SM_TDRL_1  =    0x20000000,
	G2D_SM_DTRL_1  =    0x30000000, */
} g2d_blt_flags_h;

/* ROP3 command */
typedef enum {
	G2D_ROP3_BLACKNESS = 0x00,
	G2D_ROP3_NOTSRCERASE = 0x11,
	G2D_ROP3_NOTSRCCOPY = 0x33,
	G2D_ROP3_SRCERASE = 0x44,
	G2D_ROP3_DSTINVERT = 0x55,
	G2D_ROP3_PATINVERT = 0x5A,
	G2D_ROP3_SRCINVERT = 0x66,
	G2D_ROP3_SRCAND = 0x88,
	G2D_ROP3_MERGEPAINT = 0xBB,
	G2D_ROP3_MERGECOPY = 0xC0,
	G2D_ROP3_SRCCOPY = 0xCC,
	G2D_ROP3_SRCPAINT = 0xEE,
	G2D_ROP3_PATCOPY = 0xF0,
	G2D_ROP3_PATPAINT = 0xFB,
	G2D_ROP3_WHITENESS = 0xFF,
} g2d_rop3_cmd_flag;

/* BLD LAYER ALPHA MODE*/
typedef enum {
	G2D_PIXEL_ALPHA,
	G2D_GLOBAL_ALPHA,
	G2D_MIXER_ALPHA,
} g2d_alpha_mode_enh;

/* g2d rect */
typedef struct {
	__s32 x;
	__s32 y;
	__u32 w;
	__u32 h;
} g2d_rect;

/* g2d color gamut */
typedef enum {
	G2D_BT601,
	G2D_BT709,
	G2D_BT2020,
} g2d_color_gmt;

/* image struct */
typedef struct {
	int bbuff;
	__u32 color;
	g2d_fmt_enh format;
	__u32 laddr[3];
	__u32 haddr[3];
	__u32 width;
	__u32 height;
	__u32 align[3];

	g2d_rect clip_rect;

	__u32 gamut;
	int bpremul;
	__u8 alpha;
	g2d_alpha_mode_enh mode;
} g2d_image_enh;

/* image struct */
typedef struct {
	__u32 addr[3];		/* base addr of image frame buffer in byte */
	__u32 w;		/* width of image frame buffer in pixel */
	__u32 h;		/* height of image frame buffer in pixel */
	g2d_data_fmt format;	/* pixel format of image frame buffer */
	g2d_pixel_seq pixel_seq;/* pixel sequence of image frame buffer */
} g2d_image;

typedef struct {
	g2d_fillrect_flags flag;
	g2d_image dst_image;
	g2d_rect dst_rect;

	__u32 color;		/* fill color */
	__u32 alpha;		/* plane alpha value */
	enum g2d_scan_order scan_mode;

} g2d_fillrect;

typedef struct {
	g2d_image_enh dst_image_h;
	__u32 color;		/* fill color */
} g2d_fillrect_h;

typedef struct {
	g2d_blt_flags flag;
	g2d_image src_image;
	g2d_rect src_rect;

	g2d_image dst_image;
	__s32 dst_x;		/* left top point coordinate x of dst rect */
	__s32 dst_y;		/* left top point coordinate y of dst rect */

	__u32 color;		/* colorkey color */
	__u32 alpha;		/* plane alpha value */
	enum g2d_scan_order scan_mode;

} g2d_blt;

typedef struct {
	g2d_blt_flags_h flag_h;
	g2d_image_enh src_image_h;
	g2d_image_enh dst_image_h;
	__u32 color;		/* colorkey color */
	__u32 alpha;		/* plane alpha value */

} g2d_blt_h;

typedef struct {
	g2d_blt_flags flag;
	g2d_image src_image;
	g2d_rect src_rect;

	g2d_image dst_image;
	g2d_rect dst_rect;

	__u32 color;		/* colorkey color */
	__u32 alpha;		/* plane alpha value */
	enum g2d_scan_order scan_mode;

} g2d_stretchblt;

typedef struct {
	g2d_rop3_cmd_flag back_flag;
	g2d_rop3_cmd_flag fore_flag;

	g2d_image_enh dst_image_h;
	g2d_image_enh src_image_h;
	g2d_image_enh ptn_image_h;
	g2d_image_enh mask_image_h;

} g2d_maskblt;

/* Porter Duff BLD command*/
typedef enum {
	G2D_BLD_CLEAR = 0x00000001,
	G2D_BLD_COPY = 0x00000002,
	G2D_BLD_DST = 0x00000003,
	G2D_BLD_SRCOVER = 0x00000004,
	G2D_BLD_DSTOVER = 0x00000005,
	G2D_BLD_SRCIN = 0x00000006,
	G2D_BLD_DSTIN = 0x00000007,
	G2D_BLD_SRCOUT = 0x00000008,
	G2D_BLD_DSTOUT = 0x00000009,
	G2D_BLD_SRCATOP = 0x0000000a,
	G2D_BLD_DSTATOP = 0x0000000b,
	G2D_BLD_XOR = 0x0000000c,
	G2D_CK_SRC = 0x00010000,
	G2D_CK_DST = 0x00020000,
} g2d_bld_cmd_flag;

typedef struct {
	__u32 *pbuffer;
	__u32 size;

} g2d_palette;

#endif /* __G2D_BSP_DRV_H */

/* CK PARA struct */
typedef struct {
	bool match_rule;
/*	int match_rule; */
	__u32 max_color;
	__u32 min_color;
} g2d_ck;

typedef struct {
	g2d_bld_cmd_flag bld_cmd;
	g2d_image_enh dst_image_h;
	g2d_image_enh src_image_h;
	g2d_ck ck_para;
} g2d_bld;			/* blending enhance */

typedef struct {
	__u32 g2d_base;
} g2d_init_para;

typedef struct {
	g2d_init_para init_para;
} g2d_dev_t;

typedef enum {
	G2D_RGB2YUV_709,
	G2D_YUV2RGB_709,
	G2D_RGB2YUV_601,
	G2D_YUV2RGB_601,
	G2D_RGB2YUV_2020,
	G2D_YUV2RGB_2020,
} g2d_csc_sel;

typedef enum {
	VSU_FORMAT_YUV422 = 0x00,
	VSU_FORMAT_YUV420 = 0x01,
	VSU_FORMAT_YUV411 = 0x02,
	VSU_FORMAT_RGB = 0x03,
	VSU_FORMAT_BUTT = 0x04,
} vsu_pixel_format;

#define VSU_ZOOM0_SIZE	1
#define VSU_ZOOM1_SIZE	8
#define VSU_ZOOM2_SIZE	4
#define VSU_ZOOM3_SIZE	1
#define VSU_ZOOM4_SIZE	1
#define VSU_ZOOM5_SIZE	1

#define VSU_PHASE_NUM            32
#define VSU_PHASE_FRAC_BITWIDTH  19
#define VSU_PHASE_FRAC_REG_SHIFT 1
#define VSU_FB_FRAC_BITWIDTH     32

#define VI_LAYER_NUMBER 1
#define UI_LAYER_NUMBER 3

/*
typedef enum {
	G2D_CMD_BITBLT			=	0x50,
	G2D_CMD_FILLRECT		=	0x51,
	G2D_CMD_STRETCHBLT		=	0x52,
	G2D_CMD_PALETTE_TBL		=	0x53,
	G2D_CMD_QUEUE			=	0x54,
	G2D_CMD_BITBLT_H		=	0x55,
	G2D_CMD_FILLRECT_H		=	0x56,
	G2D_CMD_BLD_H			=   0x57,
	G2D_CMD_MASK_H			=   0x58,

	G2D_CMD_MEM_REQUEST		=	0x59,
	G2D_CMD_MEM_RELEASE		=	0x5A,
	G2D_CMD_MEM_GETADR		=	0x5B,
	G2D_CMD_MEM_SELIDX		=	0x5C,
	G2D_CMD_MEM_FLUSH_CACHE	=	0x5D,
	G2D_CMD_INVERTED_ORDER	=	0x5E,
} g2d_cmd;
*/
__s32 g2d_bsp_open(void);
__s32 g2d_bsp_close(void);
__s32 g2d_bsp_reset(void);
__s32 g2d_irq_query(void);
__s32 rot_irq_query(void);
__s32 g2d_bsp_bld(g2d_image_enh *, g2d_image_enh *, __u32, g2d_ck *);
__s32 g2d_fillrectangle(g2d_image_enh *dst, __u32 color_value);
__s32 g2d_bsp_maskblt(g2d_image_enh *src, g2d_image_enh *ptn,
		      g2d_image_enh *mask, g2d_image_enh *dst,
		      __u32 back_flag, __u32 fore_flag);
__s32 g2d_bsp_bitblt(g2d_image_enh *src, g2d_image_enh *dst, __u32 flag);
__s32 g2d_byte_cal(__u32 format, __u32 *ycnt, __u32 *ucnt, __u32 *vcnt);

extern int g2d_wait_cmd_finish(void);

__u32 mixer_reg_init(void);
__s32 mixer_blt(g2d_blt *para, enum g2d_scan_order scan_order);
__s32 mixer_fillrectangle(g2d_fillrect *para);
__s32 mixer_stretchblt(g2d_stretchblt *para, enum g2d_scan_order scan_order);
__s32 mixer_maskblt(g2d_maskblt *para);
__u32 mixer_set_palette(g2d_palette *para);
__u64 mixer_get_addr(__u32 buffer_addr, __u32 format,
		     __u32 stride, __u32 x, __u32 y);
__u32 mixer_set_reg_base(__u32 addr);
__u32 mixer_get_irq(void);
__u32 mixer_get_irq0(void);
__u32 mixer_clear_init(void);
__u32 mixer_clear_init0(void);
__s32 mixer_cmdq(__u32 addr);
__u32 mixer_premultiply_set(__u32 flag);
__u32 mixer_micro_block_set(g2d_blt *para);

#endif
