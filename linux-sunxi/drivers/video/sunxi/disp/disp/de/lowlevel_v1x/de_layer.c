/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "de_be.h"

__s32 DE_BE_Format_To_Bpp(__u32 sel, __u8 format)
{
	__u8 bpp = 0;

	switch (format) {
	case DE_MONO_1BPP:
		bpp = 1;
		break;

	case DE_MONO_2BPP:
		bpp = 2;
		break;

	case DE_MONO_4BPP:
		bpp = 4;
		break;

	case DE_MONO_8BPP:
		bpp = 8;
		break;

	case DE_COLOR_RGB655:
	case DE_COLOR_RGB565:
	case DE_COLOR_RGB556:
	case DE_COLOR_ARGB1555:
	case DE_COLOR_RGBA5551:
	case DE_COLOR_ARGB4444:
		bpp = 16;
		break;

	case DE_COLOR_RGB0888:
		bpp = 32;
		break;

	case DE_COLOR_ARGB8888:
		bpp = 32;
		break;

	case DE_COLOR_RGB888:
		bpp = 24;
		break;

	default:
		bpp = 0;
		break;
	}

	return bpp;
}

__u32 DE_BE_Offset_To_Addr(__u32 src_addr,
			   __u32 width, __u32 x, __u32 y, __u32 bpp)
{
	__u32 addr;

	addr = src_addr + ((y * (width * bpp)) >> 3) + ((x * bpp) >> 3);

	return addr;
}

__u32 DE_BE_Addr_To_Offset(__u32 src_addr,
			   __u32 off_addr, __u32 width, __u32 bpp,
			   disp_position *pos)
{
	__u32 dist;
	disp_position offset;

	dist = off_addr - src_addr;
	offset.y = (dist << 3) / (width * bpp);
	offset.x = ((dist << 3) % (width * bpp)) / bpp;
	pos->x = offset.x;
	pos->y = offset.y;

	return 0;

}

__s32 DE_BE_Layer_Set_Work_Mode(__u32 sel, __u8 layidx, __u8 mode)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, (tmp & 0xff3fffff) | (mode & 3) << 22);

	return 0;
}

static __s32 DE_BE_Layer_Set_Addr(__u32 sel, __u8 layidx, __u32 addr)
{
	__u32 tmp_l, tmp_h, tmp;

	tmp_l = addr << 3;
	tmp_h = (addr & 0xe0000000) >> 29;
	DE_BE_WUINT32IDX(sel, DE_BE_FRMBUF_LOW32ADDR_OFF, layidx, tmp_l);

	tmp = DE_BE_RUINT32(sel, DE_BE_FRMBUF_HIGH4ADDR_OFF)
	    & (~(0xff << (layidx * 8)));
	DE_BE_WUINT32(sel, DE_BE_FRMBUF_HIGH4ADDR_OFF, tmp |
		      (tmp_h << (layidx * 8)));

	return 0;
}

static __s32 DE_BE_Layer_Set_Line_Width(__u32 sel, __u8 layidx, __u32 width)
{
	DE_BE_WUINT32IDX(sel, DE_BE_FRMBUF_WLINE_OFF, layidx, width);
	return 0;
}

__s32 DE_BE_Layer_Set_Format(__u32 sel, __u8 layidx,
			     __u8 format, bool br_swap, __u8 order,
			     bool pre_multiply)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF1, layidx);
	tmp &= 0x83f;		/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF1,
			 layidx, (tmp & 0xfffff000) | ((format & 0xf) << 8) |
			 ((br_swap & 0x1) << 2) | (order & 0x3));

	tmp = DE_BE_RUINT32IDX(sel,
			       DE_BE_LAYER_ATTRCTL_OFF0, layidx) & (~(3 << 20));
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, tmp | ((pre_multiply & 3) << 20));

	return 0;
}

__s32 DE_BE_Layer_Set_Framebuffer(__u32 sel, __u8 layidx,
				  struct layer_src_t *layer_fb)
{
	__s32 bpp;
	__u32 addr;

	bpp = DE_BE_Format_To_Bpp(sel, layer_fb->format);
	if (bpp <= 0)
		return -1;

	addr = DE_BE_Offset_To_Addr(layer_fb->fb_addr,
				    layer_fb->fb_width, layer_fb->offset_x,
				    layer_fb->offset_y, bpp);
	DE_BE_Layer_Set_Format(sel, layidx, layer_fb->format,
			       layer_fb->br_swap, layer_fb->pixseq,
			       layer_fb->pre_multiply);

	DE_BE_Layer_Set_Addr(sel, layidx, addr);
	DE_BE_Layer_Set_Line_Width(sel, layidx, layer_fb->fb_width * bpp);

	return 0;
}

__s32 DE_BE_Layer_Set_Screen_Win(__u32 sel, __u8 layidx, disp_window *win)
{
	__u32 tmp;

	tmp = ((((__u32) (win->y)) >> 31) << 31) |
	    ((((__u32) (win->y)) & 0x7fff) << 16) |
	    ((((__u32) (win->x)) >> 31) << 15) | (((__u32) (win->x)) & 0x7fff);
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_CRD_CTL_OFF, layidx, tmp);
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_SIZE_OFF,
			 layidx, (win->height - 1) << 16 | (win->width - 1));

	return 0;
}

__s32 DE_BE_Layer_Video_Enable(__u32 sel, __u8 layidx, bool video_en)
{

	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, (tmp & 0xfffffffd) | (video_en & 1) << 1);

	return 0;
}

__s32 DE_BE_Layer_Video_Ch_Sel(__u32 sel, __u8 layidx, bool scaler_index)
{

	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, (tmp & 0xffffffef) | (scaler_index & 1) << 4);

	return 0;
}

__s32 DE_BE_Layer_Yuv_Ch_Enable(__u32 sel, __u8 layidx, bool yuv_en)
{

	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, (tmp & 0xfffffffb) | (yuv_en & 1) << 2);

	return 0;
}

__s32 DE_BE_Layer_Set_Prio(__u32 sel, __u8 layidx, __u8 prio)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, (tmp & 0xfffff3ff) | (prio & 3) << 10);

	return 0;
}

__s32 DE_BE_Layer_Set_Pipe(__u32 sel, __u8 layidx, __u8 pipe)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, (tmp & 0xffff7fff) | (pipe & 1) << 15);

	return 0;
}

__s32 DE_BE_Layer_ColorKey_Enable(__u32 sel, __u8 layidx, bool enable)
{

	__u32 tmp;

	if (enable) {
		tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
		tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
		DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
				 layidx, (tmp & 0xfff3ffff) | 1 << 18);
	} else {
		tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
		tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
		DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
				 layidx, (tmp & 0xfff3ffff));
	}

	return 0;
}

__s32 DE_BE_Layer_Alpha_Enable(__u32 sel, __u8 layidx, bool enable)
{
	__u32 tmp;

	if (enable) {
		tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
		tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
		DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
				 layidx, (tmp & 0xfffffffe) | 0x01);
	} else {
		tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
		tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
		DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
				 layidx, (tmp & 0xfffffffe));
	}

	return 0;
}

__s32 DE_BE_Layer_Set_Alpha_Value(__u32 sel, __u8 layidx, __u8 alpha_val)
{

	__u32 tmp;

	tmp = DE_BE_RUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0, layidx);
	tmp &= 0xfffc8c17;	/* Invalid bits set to zero */
	DE_BE_WUINT32IDX(sel, DE_BE_LAYER_ATTRCTL_OFF0,
			 layidx, (tmp & 0x00ffffff) | (alpha_val & 0xff) << 24);

	return 0;
}

__s32 DE_BE_Layer_Enable(__u32 sel, __u8 layidx, bool enable)
{
	if (enable) {
		DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
			      DE_BE_RUINT32(sel,
					    DE_BE_MODE_CTL_OFF) | (1 << layidx)
			      << 8);
	} else {
		DE_BE_WUINT32(sel, DE_BE_MODE_CTL_OFF,
			      DE_BE_RUINT32(sel,
					    DE_BE_MODE_CTL_OFF) &
			      (~((1 << layidx) << 8)));
	}

	return 0;
}

static __s32 DE_BE_YUV_CH_Cfg_Csc_Coeff(__u32 sel, __u8 cs_mode)
{				/* todo */
	__u32 csc_coef_off;
	__u32 *pdest_end;
	__u32 *psrc_cur;
	__u32 *pdest_cur;
	__u32 temp;

	csc_coef_off = (((cs_mode & 0x3) << 7) +
			((cs_mode & 0x3) << 6)) + 0 /*yuv in */  +
	    0 /*rgb out */;

	pdest_cur = (__u32 *) (DE_Get_Reg_Base(sel) + DE_BE_YG_COEFF_OFF);
	psrc_cur = (__u32 *) (&csc_tab[csc_coef_off >> 2]);
	pdest_end = pdest_cur + 12;

	while (pdest_cur < pdest_end) {
		temp = readl((void *)pdest_cur);
		temp &= 0xffff0000;
		writel(((*psrc_cur++) & 0xffff) | temp, (void *)pdest_cur);
		pdest_cur++;
	}

	return 0;
}

/* ========================================================= */
/* function name:    DE_BE_YUV_CH_Set_Format */
/* author: */
/* date:             2009-9-28 */
/* description:      de be input YUV channel format setting */
/* parameters:	----format(0-4) */
/* 0:	planar YUV 411 */
/* 1:	planar YUV 422 */
/* 2:	planar YUV 444 */
/* 3:	interleaved YUV 422 */
/* 4:	interleaved YUV 444 */
/* ----pixel_seq(0-3) */
/* in planar data format mode */
/* 0:Y3Y2Y1Y0 */
/* 1:Y0Y1Y2Y3 */
/* in interleaved YUV 422 data format mode */
/* 0:DE_SCAL_UYVY */
/* 1:DE_SCAL_YUYV */
/* 2:DE_SCAL_VYUY */
/* 3:DE_SCAL_YVYU */
/* in interleaved YUV 444 format mode */
/* 0:DE_SCAL_AYUV */
/* 1:DE_SCAL_VUYA */
/* return:           if success return DIS_SUCCESS */
/* if fail return the number of fail */
/* modify history: */
/* ====================================================== */
static __s32 DE_BE_YUV_CH_Set_Format(__u32 sel, __u8 format, __u8 pixel_seq)
{
	__u32 tmp;

	tmp = DE_BE_RUINT32(sel, DE_BE_YUV_CTRL_OFF);
	tmp &= 0xffff8cff;	/* clear bit14:12, bit9:8 */
	DE_BE_WUINT32(sel, DE_BE_YUV_CTRL_OFF,
		      tmp | (format << 12) | (pixel_seq << 8));

	return 0;
}

static __s32 DE_BE_YUV_CH_Set_Addr(__u32 sel, __u8 ch_no, __u32 addr)
{
	DE_BE_WUINT32IDX(sel, DE_BE_YUV_ADDR_OFF, ch_no, addr);
	return 0;
}

static __s32 DE_BE_YUV_CH_Set_Line_Width(__u32 sel, __u8 ch_no, __u32 width)
{
	DE_BE_WUINT32IDX(sel, DE_BE_YUV_LINE_WIDTH_OFF, ch_no, width);
	return 0;
}

__s32 DE_BE_YUV_CH_Set_Src(__u32 sel, struct de_yuv_ch_src_t *in_src)
{
	__u32 ch0_base, ch1_base, ch2_base;
	__u32 image_w;
	__u32 offset_x, offset_y;
	__u8 in_fmt, in_mode, pixseq;
	__u32 ch0_addr, ch1_addr, ch2_addr;
	__u32 ch0_line_stride, ch1_line_stride, ch2_line_stride;
	__u8 w_shift, h_shift;
	__u32 de_scal_ch0_offset;
	__u32 de_scal_ch1_offset;
	__u32 de_scal_ch2_offset;

	ch0_base = in_src->ch0_base;
	ch1_base = in_src->ch1_base;
	ch2_base = in_src->ch2_base;
	image_w = in_src->line_width;
	offset_x = in_src->offset_x;
	offset_y = in_src->offset_y;
	in_fmt = in_src->format;
	in_mode = in_src->mode;
	pixseq = in_src->pixseq;

	w_shift = (in_fmt == 0x1 || in_fmt == 0x3) ?
	    1 : ((in_fmt == 0x0) ? 2 : 0);
	h_shift = 0;
	/* modify offset and input size */
	offset_x = (offset_x >> w_shift) << w_shift;
	offset_y = (offset_y >> h_shift) << h_shift;
	image_w = ((image_w + ((1 << w_shift) - 1)) >> w_shift) << w_shift;
	/* compute buffer address */
	/* --the size ratio of Y/G to UV/RB must be */
	/*fit with input format and mode &&&& */
	if (in_mode == 0x00) {	/* non macro block plannar */
		/* line stride */
		ch0_line_stride = image_w;
		ch1_line_stride = image_w >> (w_shift);
		ch2_line_stride = image_w >> (w_shift);
		/* buffer address */
		de_scal_ch0_offset = image_w * offset_y + offset_x;
		de_scal_ch1_offset = (image_w >> w_shift) *
			(offset_y >> h_shift) + (offset_x >> w_shift);
		de_scal_ch2_offset = (image_w >> w_shift) *
			(offset_y >> h_shift) + (offset_x >> w_shift);

		ch0_addr = ch0_base + de_scal_ch0_offset;
		ch1_addr = ch1_base + de_scal_ch1_offset;
		ch2_addr = ch2_base + de_scal_ch2_offset;
	} else if (in_mode == 0x01) {	/* interleaved data */
		/* line stride */
		ch0_line_stride = image_w << (0x02 - w_shift);
		ch1_line_stride = 0x00;
		ch2_line_stride = 0x00;
		/* buffer address */
		de_scal_ch0_offset =
		    ((image_w * offset_y + offset_x) << (0x02 - w_shift));
		de_scal_ch1_offset = 0x0;
		de_scal_ch2_offset = 0x0;

		ch0_addr = ch0_base + de_scal_ch0_offset;
		ch1_addr = 0x00;
		ch2_addr = 0x00;
	} else {
		return 0;
	}

	DE_BE_YUV_CH_Set_Format(sel, in_fmt, pixseq);
	/* set line stride */
	DE_BE_YUV_CH_Set_Line_Width(sel, 0x00, ch0_line_stride << 3);
	DE_BE_YUV_CH_Set_Line_Width(sel, 0x01, ch1_line_stride << 3);
	DE_BE_YUV_CH_Set_Line_Width(sel, 0x02, ch2_line_stride << 3);
	/* set buffer address */
	DE_BE_YUV_CH_Set_Addr(sel, 0x00, ch0_addr);
	DE_BE_YUV_CH_Set_Addr(sel, 0x01, ch1_addr);
	DE_BE_YUV_CH_Set_Addr(sel, 0x02, ch2_addr);

	DE_BE_YUV_CH_Cfg_Csc_Coeff(sel, in_src->cs_mode);
	return 0;
}

__s32 DE_BE_YUV_CH_Enable(__u32 sel, bool enable)
{
	if (enable) {
		DE_BE_WUINT32(sel, DE_BE_YUV_CTRL_OFF,
			      DE_BE_RUINT32(sel,
					    DE_BE_YUV_CTRL_OFF) | 0x00000001);
	} else {
		DE_BE_WUINT32(sel, DE_BE_YUV_CTRL_OFF,
			      DE_BE_RUINT32(sel,
					    DE_BE_YUV_CTRL_OFF) & 0xfffffffe);
	}
	return 0;
}
