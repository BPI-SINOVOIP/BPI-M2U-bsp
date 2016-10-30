/* g2d_bsp.h
 *
 * Copyright (c)	2011 Allwinnertech Co., Ltd.
 *					2011 Yupu Tang
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
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

/* mixer data format */
typedef enum {
	/* share data format */
	G2D_FMT_ARGB_AYUV8888	= (0x0),
	G2D_FMT_BGRA_VUYA8888	= (0x1),
	G2D_FMT_ABGR_AVUY8888	= (0x2),
	G2D_FMT_RGBA_YUVA8888	= (0x3),

	G2D_FMT_XRGB8888		= (0x4),
	G2D_FMT_BGRX8888		= (0x5),
	G2D_FMT_XBGR8888		= (0x6),
	G2D_FMT_RGBX8888		= (0x7),

	G2D_FMT_ARGB4444		= (0x8),
	G2D_FMT_ABGR4444		= (0x9),
	G2D_FMT_RGBA4444		= (0xA),
	G2D_FMT_BGRA4444		= (0xB),

	G2D_FMT_ARGB1555		= (0xC),
	G2D_FMT_ABGR1555		= (0xD),
	G2D_FMT_RGBA5551		= (0xE),
	G2D_FMT_BGRA5551		= (0xF),

	G2D_FMT_RGB565			= (0x10),
	G2D_FMT_BGR565			= (0x11),

	G2D_FMT_IYUV422			= (0x12),

	G2D_FMT_8BPP_MONO		= (0x13),
	G2D_FMT_4BPP_MONO		= (0x14),
	G2D_FMT_2BPP_MONO		= (0x15),
	G2D_FMT_1BPP_MONO		= (0x16),

	G2D_FMT_PYUV422UVC		= (0x17),
	G2D_FMT_PYUV420UVC		= (0x18),
	G2D_FMT_PYUV411UVC		= (0x19),

	/* just for output format */
	G2D_FMT_PYUV422			= (0x1A),
	G2D_FMT_PYUV420			= (0x1B),
	G2D_FMT_PYUV411			= (0x1C),

	/* just for input format */
	G2D_FMT_8BPP_PALETTE	= (0x1D),
	G2D_FMT_4BPP_PALETTE	= (0x1E),
	G2D_FMT_2BPP_PALETTE	= (0x1F),
	G2D_FMT_1BPP_PALETTE	= (0x20),

	G2D_FMT_PYUV422UVC_MB16	= (0x21),
	G2D_FMT_PYUV420UVC_MB16	= (0x22),
	G2D_FMT_PYUV411UVC_MB16	= (0x23),
	G2D_FMT_PYUV422UVC_MB32	= (0x24),
	G2D_FMT_PYUV420UVC_MB32	= (0x25),
	G2D_FMT_PYUV411UVC_MB32	= (0x26),
	G2D_FMT_PYUV422UVC_MB64	= (0x27),
	G2D_FMT_PYUV420UVC_MB64	= (0x28),
	G2D_FMT_PYUV411UVC_MB64	= (0x29),
	G2D_FMT_PYUV422UVC_MB128= (0x2A),
	G2D_FMT_PYUV420UVC_MB128= (0x2B),
	G2D_FMT_PYUV411UVC_MB128= (0x2C),

}g2d_data_fmt;

typedef enum {
	G2D_SEQ_NORMAL = 0x0,

	/* for interleaved yuv422 */
	G2D_SEQ_VYUY   = 0x1,				/* pixel 0�ڵ�16λ */
	G2D_SEQ_YVYU   = 0x2,				/* pixel 1�ڵ�16λ */

	/* for uv_combined yuv420 */
	G2D_SEQ_VUVU   = 0x3,

	/* for 16bpp rgb */
	G2D_SEQ_P10	= 0x4,				/* pixel 0�ڵ�16λ */
	G2D_SEQ_P01	= 0x5,				/* pixel 1�ڵ�16λ */

	/* planar format or 8bpp rgb */
	G2D_SEQ_P3210  = 0x6,				/* pixel 0�ڵ�8λ */
	G2D_SEQ_P0123  = 0x7,				/* pixel 3�ڵ�8λ */

	/* for 4bpp rgb */
	G2D_SEQ_P76543210  = 0x8,			/* 7,6,5,4,3,2,1,0 */
	G2D_SEQ_P67452301  = 0x9,			/* 6,7,4,5,2,3,0,1 */
	G2D_SEQ_P10325476  = 0xA,			/* 1,0,3,2,5,4,7,6 */
	G2D_SEQ_P01234567  = 0xB,			/* 0,1,2,3,4,5,6,7 */

	/* for 2bpp rgb */
	G2D_SEQ_2BPP_BIG_BIG	   = 0xC,	/* 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 */
	G2D_SEQ_2BPP_BIG_LITTER	= 0xD,	/* 12,13,14,15,8,9,10,11,4,5,6,7,0,1,2,3 */
	G2D_SEQ_2BPP_LITTER_BIG	= 0xE,	/* 3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12 */
	G2D_SEQ_2BPP_LITTER_LITTER = 0xF,	/* 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 */

	/* for 1bpp rgb */
	G2D_SEQ_1BPP_BIG_BIG	   = 0x10,	/* 31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 */
	G2D_SEQ_1BPP_BIG_LITTER	= 0x11,	/* 24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7 */
	G2D_SEQ_1BPP_LITTER_BIG	= 0x12,	/* 7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24 */
	G2D_SEQ_1BPP_LITTER_LITTER = 0x13,	/* 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 */
}g2d_pixel_seq;


typedef enum {
	G2D_FIL_NONE			= 0x00000000,
	G2D_FIL_PIXEL_ALPHA		= 0x00000001,
	G2D_FIL_PLANE_ALPHA		= 0x00000002,
	G2D_FIL_MULTI_ALPHA		= 0x00000004,
}g2d_fillrect_flags;

typedef enum {
	G2D_BLT_NONE			= 0x00000000,
	G2D_BLT_PIXEL_ALPHA		= 0x00000001,
	G2D_BLT_PLANE_ALPHA		= 0x00000002,
	G2D_BLT_MULTI_ALPHA		= 0x00000004,
	G2D_BLT_SRC_COLORKEY	= 0x00000008,
	G2D_BLT_DST_COLORKEY	= 0x00000010,
	G2D_BLT_FLIP_HORIZONTAL	= 0x00000020,
	G2D_BLT_FLIP_VERTICAL	= 0x00000040,
	G2D_BLT_ROTATE90		= 0x00000080,
	G2D_BLT_ROTATE180		= 0x00000100,
	G2D_BLT_ROTATE270		= 0x00000200,
	G2D_BLT_MIRROR45		= 0x00000400,
	G2D_BLT_MIRROR135		= 0x00000800,
	G2D_BLT_SRC_PREMULTIPLY	= 0x00001000,
	G2D_BLT_DST_PREMULTIPLY	= 0x00002000,
}g2d_blt_flags;

/* flip rectangle struct */
typedef struct {
	__s32		x;			/* left top point coordinate x */
	__s32		y;			/* left top point coordinate y */
	__u32		w;			/* rectangle width */
	__u32		h;			/* rectangle height */
}g2d_rect;

/* image struct */
typedef struct {
	__u32			 addr[3];	/* base addr of image frame buffer in byte */
	__u32			 w;			/* width of image frame buffer in pixel */
	__u32			 h;			/* height of image frame buffer in pixel */
	g2d_data_fmt	 format;	/* pixel format of image frame buffer */
	g2d_pixel_seq	 pixel_seq;	/* pixel sequence of image frame buffer */
}g2d_image;

/*
 * 0:Top to down, Left to right
 * 1:Top to down, Right to left
 * 2:Down to top, Left to right
 * 3:Down to top, Right to left
 */
enum g2d_scan_order{
	G2D_SM_TDLR = 0x00000000,
	G2D_SM_TDRL = 0x00000001,
	G2D_SM_DTLR = 0x00000002,
	G2D_SM_DTRL = 0x00000003,
};

typedef struct {
	g2d_fillrect_flags	 flag;
	g2d_image			 dst_image;
	g2d_rect			 dst_rect;

	__u32				 color;		/* fill color */
	__u32				 alpha;		/* plane alpha value */

}g2d_fillrect;

typedef struct {
	g2d_blt_flags		 flag;
	g2d_image			 src_image;
	g2d_rect			 src_rect;

	g2d_image			 dst_image;
	__s32				 dst_x;		/* left top point coordinate x of dst rect */
	__s32				 dst_y;		/* left top point coordinate y of dst rect */

	__u32				 color;		/* colorkey color */
	__u32				 alpha;		/* plane alpha value */

}g2d_blt;

typedef struct {
	g2d_blt_flags		 flag;
	g2d_image			 src_image;
	g2d_rect			 src_rect;

	g2d_image			 dst_image;
	g2d_rect			 dst_rect;

	__u32				 color;		/* colorkey color */
	__u32				 alpha;		/* plane alpha value */


}g2d_stretchblt;

typedef struct {
	__u32		 flag;		/* ��դ������ */
	g2d_image	 dst_image;
	g2d_rect	 dst_rect;

	g2d_image	 src_image;
	__u32		 src_x;
	__u32		 src_y;

	g2d_image	 mask_image;
	__u32		 mask_x;
	__u32		 mask_y;

}g2d_maskblt;

typedef struct {
	__u32		*pbuffer;
	__u32		 size;

}g2d_palette;

#endif	/* __G2D_BSP_DRV_H */

typedef struct {
	__u32	g2d_base;
}g2d_init_para;

typedef struct
{
	g2d_init_para init_para;
}g2d_dev_t;

extern int g2d_wait_cmd_finish(void);

__u32	mixer_reg_init(void);
__s32	mixer_blt(g2d_blt *para, enum g2d_scan_order scan_order);
__s32	mixer_fillrectangle(g2d_fillrect *para);
__s32	mixer_stretchblt(g2d_stretchblt *para, enum g2d_scan_order scan_order);
__s32	mixer_maskblt(g2d_maskblt *para);
__u32	mixer_set_palette(g2d_palette *para);
__u64	mixer_get_addr(__u32 buffer_addr, __u32 format, __u32 stride, __u32 x, __u32 y);
__u32	mixer_set_reg_base(__u32 addr);
__u32	mixer_get_irq(void);
__u32	mixer_get_irq0(void);
__u32	mixer_clear_init(void);
__u32	mixer_clear_init0(void);
__s32	mixer_cmdq(__u32 addr);
__u32	mixer_premultiply_set(__u32 flag);
__u32	mixer_micro_block_set(g2d_blt *para);

#endif	/* __G2D_BSP_H */

