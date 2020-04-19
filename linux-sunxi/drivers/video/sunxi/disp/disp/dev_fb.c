/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "drv_disp_i.h"
#include "dev_disp.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>

#define FBHANDTOID(handle)  ((handle) - 100)
#define FBIDTOHAND(ID)  ((ID) + 100)

static struct __fb_addr_para g_fb_addr;

s32 sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para)
{
	if (fb_addr_para) {
		fb_addr_para->fb_paddr = g_fb_addr.fb_paddr;
		fb_addr_para->fb_size = g_fb_addr.fb_size;
		return 0;
	}

	return -1;
}
EXPORT_SYMBOL(sunxi_get_fb_addr_para);

u32 get_fastboot_mode(void)
{
	return fastboot;
}
EXPORT_SYMBOL(get_fastboot_mode);

static u32 Fb_reset_disp_rsl(disp_init_para *init_para)
{
	u32 value;
	u8 type, mode, channel;
	u8 valid = 1;

	/* get the parameters of display_resolution */
	/* from cmdline passed by boot */
	value = disp_get_disp_rsl();
	type = (value >> 16) & 0xFF;
	mode = (value >> 8) & 0xFF;
	channel = value & 0xFF;

	if (type == DISP_OUTPUT_TYPE_HDMI) {
		valid &= ((mode < DISP_TV_MODE_NUM) ? 1 : 0);
	} else if (type == DISP_OUTPUT_TYPE_TV) {
		valid &= (((mode == DISP_TV_MOD_PAL) ||
			   (mode == DISP_TV_MOD_NTSC)) ? 1 : 0);
	} else if (type == DISP_OUTPUT_TYPE_VGA) {
		/* todo */
		valid = 0;
	} else {
		valid = 0;
	}
	if (valid != 0) {
		init_para->disp_mode = channel;
		init_para->output_type[channel] = type;
		init_para->output_mode[channel] = mode;
	}

	return 0;
}

/* 0:ARGB    1:BRGA    2:ABGR    3:RGBA */
/* seq           ARGB        BRGA       ARGB       BRGA */
/* br_swqp    0              0            1              1 */
static s32 parser_disp_init_para(const struct device_node *np,
				 disp_init_para *init_para)
{
	int value;
	int i;

	memset(init_para, 0, sizeof(disp_init_para));

	if (disp_sys_script_get_item("disp",
				     "disp_init_enable", &value, 1) == 0) {
		pr_warn("fetch script datadisp.disp_init_enable fail\n");
		return -1;
	}
	init_para->b_init = value;

	if (disp_sys_script_get_item("disp", "disp_mode", &value, 1) == 0) {
		pr_warn("fetch script datadisp.disp_mode fail\n");
		return -1;
	}
	init_para->disp_mode = value;

	/* screen0 */
	if (disp_sys_script_get_item("disp",
				     "screen0_output_type", &value, 1) == 0) {
		pr_warn("fetch script datadisp.screen0_output_type fail\n");
		return -1;
	}
	if (value == 0) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_NONE;
	} else if (value == 1) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_LCD;
	} else if (value == 2) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_TV;
	} else if (value == 3) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_HDMI;
	} else if (value == 4) {
		init_para->output_type[0] = DISP_OUTPUT_TYPE_VGA;
	} else {
		pr_warn("invalid screen0_output_type %d\n",
			init_para->output_type[0]);
		return -1;
	}

	if (disp_sys_script_get_item("disp",
				     "screen0_output_mode", &value, 1) == 0) {
		pr_warn("fetch script datadisp.screen0_output_mode fail\n");
		return -1;
	}
	if (init_para->output_type[0] ==
	    DISP_OUTPUT_TYPE_TV ||
	    init_para->output_type[0] ==
	    DISP_OUTPUT_TYPE_HDMI
	    || init_para->output_type[0] == DISP_OUTPUT_TYPE_VGA) {
		init_para->output_mode[0] = value;
	}

	/* screen1 */
	if (disp_sys_script_get_item("disp",
				     "screen1_output_type", &value, 1) == 0) {
		pr_warn("fetch script datadisp.screen1_output_type fail\n");
		return -1;
	}
	if (value == 0) {
		init_para->output_type[1] = DISP_OUTPUT_TYPE_NONE;
	} else if (value == 1) {
		init_para->output_type[1] = DISP_OUTPUT_TYPE_LCD;
	} else if (value == 2) {
		init_para->output_type[1] = DISP_OUTPUT_TYPE_TV;
	} else if (value == 3) {
		init_para->output_type[1] = DISP_OUTPUT_TYPE_HDMI;
	} else if (value == 4) {
		init_para->output_type[1] = DISP_OUTPUT_TYPE_VGA;
	} else {
		pr_warn("invalid screen1_output_type %d\n",
			init_para->output_type[1]);
		return -1;
	}

	if (disp_sys_script_get_item("disp",
				     "screen1_output_mode", &value, 1) == 0) {
		pr_warn("fetch script datadisp.screen1_output_mode fail\n");
		return -1;
	}
	if (init_para->output_type[1] ==
	    DISP_OUTPUT_TYPE_TV ||
	    init_para->output_type[1] ==
	    DISP_OUTPUT_TYPE_HDMI
	    || init_para->output_type[1] == DISP_OUTPUT_TYPE_VGA) {
		init_para->output_mode[1] = value;
	}

	/* screen2 */
	if (disp_sys_script_get_item("disp",
				     "screen2_output_type", &value, 1) == 0) {
		pr_warn("fetch script datadisp.screen2_output_type fail\n");
	}
	if (value == 0) {
		init_para->output_type[2] = DISP_OUTPUT_TYPE_NONE;
	} else if (value == 1) {
		init_para->output_type[2] = DISP_OUTPUT_TYPE_LCD;
	} else if (value == 2) {
		init_para->output_type[2] = DISP_OUTPUT_TYPE_TV;
	} else if (value == 3) {
		init_para->output_type[2] = DISP_OUTPUT_TYPE_HDMI;
	} else if (value == 4) {
		init_para->output_type[2] = DISP_OUTPUT_TYPE_VGA;
	} else {
		pr_warn("invalid screen0_output_type %d\n",
			init_para->output_type[2]);
	}

	if (disp_sys_script_get_item("disp",
				     "screen2_output_mode", &value, 1) == 0) {
		pr_warn("fetch script datadisp.screen2_output_mode fail\n");
	}
	if (init_para->output_type[2] ==
	    DISP_OUTPUT_TYPE_TV ||
	    init_para->output_type[2] ==
	    DISP_OUTPUT_TYPE_HDMI
	    || init_para->output_type[2] == DISP_OUTPUT_TYPE_VGA) {
		init_para->output_mode[2] = value;
	}

	/* fb0 */
	init_para->buffer_num[0] = 2;

	if (disp_sys_script_get_item("disp", "fb0_format", &value, 1) == 0) {
		pr_warn("fetch script datadisp.fb0_format fail\n");
		return -1;
	}
	init_para->format[0] = value;

	if (disp_sys_script_get_item("disp",
				     "fb0_scaler_mode_enable", &value,
				     1) == 0) {
		pr_warn("fetch script datadisp.fb0_scaler_mode_enable fail\n");
		return -1;
	}
	init_para->scaler_mode[0] = value;

	if (disp_sys_script_get_item("disp", "fb0_width", &value, 1) == 0) {
		pr_warn("fetch scriptdata disp.fb0_width fail\n");
		return -1;
	}
	init_para->fb_width[0] = value;

	if (disp_sys_script_get_item("disp", "fb0_height", &value, 1) == 0) {
		pr_warn("fetch scriptdata disp.fb0_height fail\n");
		return -1;
	}
	init_para->fb_height[0] = value;

	/* fb1 */
	init_para->buffer_num[1] = 2;

	if (disp_sys_script_get_item("disp", "fb1_format", &value, 1) == 0)
		pr_warn("fetch scriptdata disp.fb1_format fail\n");

	init_para->format[1] = value;

	if (disp_sys_script_get_item("disp", "fb1_scaler_mode_enable",
				     &value, 1) == 0)
		pr_warn("fetch script datadisp.fb1_scaler_mode_enable fail\n");

	init_para->scaler_mode[1] = value;

	if (disp_sys_script_get_item("disp", "fb1_width", &value, 1) == 0)
		pr_warn("fetch script datadisp.fb1_width fail\n");

	init_para->fb_width[1] = value;

	if (disp_sys_script_get_item("disp", "fb1_height", &value, 1) == 0)
		pr_warn("fetch script datadisp.fb1_height fail\n");

	init_para->fb_height[1] = value;

	/* fb2 */
	init_para->buffer_num[2] = 2;

	if (disp_sys_script_get_item("disp", "fb2_format", &value, 1) == 0)
		pr_warn("fetch script datadisp.fb2_format fail\n");

	init_para->format[2] = value;

	if (disp_sys_script_get_item("disp", "fb2_scaler_mode_enable",
				     &value, 1) == 0)
		pr_warn("fetch script datadisp.fb2_scaler_mode_enable fail\n");

	init_para->scaler_mode[2] = value;

	if (disp_sys_script_get_item("disp", "fb2_width", &value, 1) == 0)
		pr_warn("fetch script datadisp.fb2_width fail\n");

	init_para->fb_width[2] = value;

	if (disp_sys_script_get_item("disp", "fb2_height", &value, 1) == 0)
		pr_warn("fetch script datadisp.fb2_height fail\n");

	init_para->fb_height[2] = value;

	__inf("====display init para begin====\n");
	__inf("b_init:%d\n", init_para->b_init);
	__inf("disp_mode:%d\n\n", init_para->disp_mode);
	for (i = 0; i < 3; i++) {
		__inf("output_type[%d]:%d\n", i, init_para->output_type[i]);
		__inf("output_mode[%d]:%d\n", i, init_para->output_mode[i]);
	}
	for (i = 0; i < 3; i++) {
		__inf("buffer_num[%d]:%d\n", i, init_para->buffer_num[i]);
		__inf("format[%d]:%d\n", i, init_para->format[i]);
		__inf("b_scaler_mode[%d]:%d\n", i, init_para->scaler_mode[i]);
		__inf("fb_width[%d]:%d\n", i, init_para->fb_width[i]);
		__inf("fb_height[%d]:%d\n", i, init_para->fb_height[i]);
	}

	Fb_reset_disp_rsl(init_para);

	__inf("====display init para end====\n");

	return 0;
}

s32 fb_draw_colorbar(u32 base,
		     u32 width, u32 height, struct fb_var_screeninfo *var)
{
	u32 i = 0, j = 0;

	if (!base)
		return -1;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width / 4; j++) {
			u32 offset = 0;

			if (var->bits_per_pixel == 32) {
				offset = width * i + j;
				sys_put_wvalue(base + offset * 4,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->red.length) - 1)
						  << var->red.offset));

				offset = width * i + j + width / 4;
				sys_put_wvalue(base + offset * 4,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->green.length) - 1)
						  << var->green.offset));

				offset = width * i + j + width / 4 * 2;
				sys_put_wvalue(base + offset * 4,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->blue.length) - 1)
						  << var->blue.offset));

				offset = width * i + j + width / 4 * 3;
				sys_put_wvalue(base + offset * 4,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->red.length) - 1)
						  << var->red.offset)
					       | (((1 << var->green.length) - 1)
						  << var->green.offset));
			} else if (var->bits_per_pixel == 16) {
				offset = width * i + j;
				sys_put_hvalue(base + offset * 2,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->red.length) - 1)
						  << var->red.offset));

				offset = width * i + j + width / 4;
				sys_put_hvalue(base + offset * 2,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->green.length) - 1)
						  << var->green.offset));

				offset = width * i + j + width / 4 * 2;
				sys_put_hvalue(base + offset * 2,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->blue.length) - 1)
						  << var->blue.offset));

				offset = width * i + j + width / 4 * 3;
				sys_put_hvalue(base + offset * 2,
					       (((1 << var->transp.length) -
						 1) << var->transp.offset)
					       | (((1 << var->red.length) - 1)
						  << var->red.offset)
					       | (((1 << var->green.length) - 1)
						  << var->green.offset));
			}
		}
	}

	return 0;
}

s32 fb_draw_gray_pictures(u32 base,
			  u32 width, u32 height, struct fb_var_screeninfo *var)
{
	u32 time = 0;

	for (time = 0; time < 18; time++) {
		u32 i = 0, j = 0;

		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				u32 addr = base + (i * width + j) * 4;
				u32 value = (0xff << 24) |
				    ((time * 15) << 16) |
				    ((time * 15) << 8) | (time * 15);

				sys_put_wvalue(addr, value);
			}
		}
	}
	return 0;
}

static int Fb_map_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);
	struct page *page;

	page = alloc_pages(GFP_KERNEL, get_order(map_size));
	if (page != NULL) {
		info->screen_base = page_address(page);
		info->fix.smem_start = virt_to_phys(info->screen_base);
		memset(info->screen_base, 0x0, info->fix.smem_len);
		__inf("Fb_map_video_memory(alloc pages),pa=0x%08lx size:0x%x\n",
		      info->fix.smem_start, info->fix.smem_len);
	} else {
		pr_warn("alloc_pages fail! size:0x%x\n", info->fix.smem_len);
		return -ENOMEM;
	}
#else
	info->screen_base =
	    (char __iomem *)disp_malloc(info->fix.smem_len,
					(u32 *) (&info->fix.smem_start));
	if (info->screen_base) {
		__inf("Fb_map_video_memory(reserve),pa=0x%x size:0x%x\n",
		      (unsigned int)info->fix.smem_start,
		      (unsigned int)info->fix.smem_len);
		memset((void *__force)info->screen_base,
		       0x0, info->fix.smem_len);

		g_fb_addr.fb_paddr = (unsigned int)info->fix.smem_start;
		g_fb_addr.fb_size = info->fix.smem_len;

	} else {
		pr_warn("disp_malloc fail!\n");
		return -ENOMEM;
	}

	return 0;
#endif
}

static inline void Fb_unmap_video_memory(struct fb_info *info)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(info->fix.smem_len);

	free_pages((unsigned long)info->screen_base, get_order(map_size));
#else
	disp_free((void *__force)info->screen_base,
		  (void *)info->fix.smem_start, info->fix.smem_len);
#endif
}

void *Fb_map_kernel(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr;
	int i;

	if (!pages)
		return 0;

	for (i = 0; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = pgprot_noncached(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

#if 0
static void Fb_unmap_kernel(void *vaddr)
{
	vunmap(vaddr);
}

static int Fb_map_boot_logo(__u32 sel, struct fb_info *info)
{
	u32 src_phy_addr;
	u32 *src_vir_addr, *psrc;
	u32 src_width, src_height, src_widthstep;
	u32 *pdst, *psrc_end;

	/* get logo address and size */
	if (info->var.bits_per_pixel != 32) {
		pr_warn("<0>only deal with 32bits image!\n");
		return 0;
	}

	bsp_disp_layer_get_size(2, 0, &src_width, &src_height);
	if ((src_width > info->var.xres) || (src_height > info->var.yres)) {
		pr_warn("[FB]bootlogo size is larger than fb size!");
		return -EINVAL;
	}
	src_widthstep = src_width << 2;
	src_phy_addr = bsp_disp_layer_get_addr(2, 0);	/* fixme */
	pr_warn("[fb_map_boot_logo]w=%d,h=%d,addr=0x%x\n",
		src_width, src_height, src_phy_addr);
	src_vir_addr =
	    (u32 *) Fb_map_kernel(src_phy_addr, src_widthstep * src_height);
	if (!src_vir_addr) {
		pr_warn("%s err: Fb_map_kernel failed!\n", __func__);
		return -EINVAL;
	}
	psrc = src_vir_addr;
	pdst = (u32 *) info->screen_base + ((info->var.xres - src_width) >> 1)
	    + info->var.xres * ((info->var.yres - src_height) >> 1);
#if 1
	psrc_end = psrc + src_width * src_height;
	for (; psrc != psrc_end; psrc += src_width) {
		memcpy((void *)pdst, (void *)psrc, src_widthstep);
		pdst += info->var.xres;
	}
#else				/* for debug */
	u32 *pdst_end = pdst + info->var.xres * (info->var.yres >> 1);

	for (; pdst != pdst_end; pdst += info->var.xres) {
		u8 *p_fb = (u8 *) pdst;
		int j;

		for (j = 0; j < info->var.xres / 2; j++) {
			*p_fb++ = 0xFF;	/* B */
			*p_fb++ = 0x80;	/* G */
			*p_fb++ = 0x80;	/* R */
			*p_fb++ = 0xFF;	/* A */
		}
	}
#endif
	Fb_unmap_kernel((void *)src_vir_addr);

	return 0;
}
#endif

#if 0
static s32 disp_fb_to_var(disp_pixel_format format,
			  struct fb_var_screeninfo *var)
{
	switch (format) {
	case DISP_FORMAT_ARGB_8888:
		var->bits_per_pixel = 32;
		var->transp.length = 8;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		var->transp.offset = var->red.offset + var->red.length;
		break;
	case DISP_FORMAT_ABGR_8888:
		var->bits_per_pixel = 32;
		var->transp.length = 8;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->red.offset = 0;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;
		var->transp.offset = var->blue.offset + var->blue.length;
		break;
	case DISP_FORMAT_RGBA_8888:
		var->bits_per_pixel = 32;
		var->transp.length = 8;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->blue.offset = var->transp.offset + var->transp.length;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		break;
	case DISP_FORMAT_BGRA_8888:
		var->bits_per_pixel = 32;
		var->transp.length = 8;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->red.offset = var->transp.offset + var->transp.length;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;
		break;
	case DISP_FORMAT_RGB_888:
		var->bits_per_pixel = 24;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;

		break;
	case DISP_FORMAT_BGR_888:
		var->bits_per_pixel = 24;
		var->transp.length = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->red.offset = 0;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;

		break;
	case DISP_FORMAT_RGB_565:
		var->bits_per_pixel = 16;
		var->transp.length = 0;
		var->red.length = 5;
		var->green.length = 6;
		var->blue.length = 5;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;

		break;
	case DISP_FORMAT_BGR_565:
		var->bits_per_pixel = 16;
		var->transp.length = 0;
		var->red.length = 5;
		var->green.length = 6;
		var->blue.length = 5;
		var->red.offset = 0;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;

		break;
	case DISP_FORMAT_ARGB_4444:
		var->bits_per_pixel = 16;
		var->transp.length = 4;
		var->red.length = 4;
		var->green.length = 4;
		var->blue.length = 4;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		var->transp.offset = var->red.offset + var->red.length;

		break;
	case DISP_FORMAT_ABGR_4444:
		var->bits_per_pixel = 16;
		var->transp.length = 4;
		var->red.length = 4;
		var->green.length = 4;
		var->blue.length = 4;
		var->red.offset = 0;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;
		var->transp.offset = var->blue.offset + var->blue.length;

		break;
	case DISP_FORMAT_RGBA_4444:
		var->bits_per_pixel = 16;
		var->transp.length = 4;
		var->red.length = 4;
		var->green.length = 5;
		var->blue.length = 4;
		var->transp.offset = 0;
		var->blue.offset = var->transp.offset + var->transp.length;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;

		break;
	case DISP_FORMAT_BGRA_4444:
		var->bits_per_pixel = 16;
		var->transp.length = 4;
		var->red.length = 4;
		var->green.length = 4;
		var->blue.length = 4;
		var->transp.offset = 0;
		var->red.offset = var->transp.offset + var->transp.length;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;

		break;
	case DISP_FORMAT_ARGB_1555:
		var->bits_per_pixel = 16;
		var->transp.length = 1;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		var->blue.offset = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;
		var->transp.offset = var->red.offset + var->red.length;

		break;
	case DISP_FORMAT_ABGR_1555:
		var->bits_per_pixel = 16;
		var->transp.length = 1;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		var->red.offset = 0;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;
		var->transp.offset = var->blue.offset + var->blue.length;

		break;
	case DISP_FORMAT_RGBA_5551:
		var->bits_per_pixel = 16;
		var->transp.length = 1;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->blue.offset = var->transp.offset + var->transp.length;
		var->green.offset = var->blue.offset + var->blue.length;
		var->red.offset = var->green.offset + var->green.length;

		break;
	case DISP_FORMAT_BGRA_5551:
		var->bits_per_pixel = 16;
		var->transp.length = 1;
		var->red.length = 5;
		var->green.length = 5;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->red.offset = var->transp.offset + var->transp.length;
		var->green.offset = var->red.offset + var->red.length;
		var->blue.offset = var->green.offset + var->green.length;

		break;
	default:
		pr_warn("[FB]not support format %d\n", format);
	}

	return 0;
}
#endif

static s32 var_to_disp_fb(disp_fb_info *fb,
			  struct fb_var_screeninfo *var,
			  struct fb_fix_screeninfo *fix)
{
	if (var->nonstd == 0) {
		switch (var->bits_per_pixel) {
		case 32:
			if (var->red.offset == 16 &&
			    var->green.offset == 8 && var->blue.offset == 0)
				fb->format = DISP_FORMAT_ARGB_8888;
			else if (var->blue.offset == 24 &&
				 var->green.offset == 16 &&
				 var->red.offset == 8)
				fb->format = DISP_FORMAT_BGRA_8888;
			else if (var->blue.offset == 16 &&
				 var->green.offset == 8 && var->red.offset == 0)
				fb->format = DISP_FORMAT_ABGR_8888;
			else if (var->red.offset == 24 &&
				 var->green.offset == 16 &&
				 var->blue.offset == 8)
				fb->format = DISP_FORMAT_RGBA_8888;
			else
				pr_warn
				    ("[FB]invalid argbformat<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",
				     var->transp.offset, var->red.offset,
				     var->green.offset, var->blue.offset);

			break;
		case 24:
			if (var->red.offset == 16 &&
			    var->green.offset == 8 &&
			    var->blue.offset == 0) {	/* rgb */
				fb->format = DISP_FORMAT_RGB_888;
			} else if (var->blue.offset == 16 &&
				   var->green.offset == 8 &&
				   var->red.offset == 0) {	/* bgr */
				fb->format = DISP_FORMAT_BGR_888;
			} else {
				pr_warn
				    ("[FB]invalidformat<transp.offset:%d,red.offset:%d,green.offset:%d,blue.offset:%d>\n",
				     var->transp.offset, var->red.offset,
				     var->green.offset, var->blue.offset);
			}

			break;
		case 16:
			if (var->red.offset == 11 &&
			    var->green.offset == 5 && var->blue.offset == 0) {
				fb->format = DISP_FORMAT_RGB_565;
			} else if (var->blue.offset == 11 &&
				   var->green.offset == 5 &&
				   var->red.offset == 0) {
				fb->format = DISP_FORMAT_BGR_565;
			} else if (var->transp.offset == 12 &&
				   var->red.offset == 8 &&
				   var->green.offset == 4 &&
				   var->blue.offset == 0) {
				fb->format = DISP_FORMAT_ARGB_4444;
			} else if (var->transp.offset == 12 &&
				   var->blue.offset == 8 &&
				   var->green.offset == 4 &&
				   var->red.offset == 0) {
				fb->format = DISP_FORMAT_ABGR_4444;
			} else if (var->red.offset == 12 &&
				   var->green.offset == 8 &&
				   var->blue.offset == 4 &&
				   var->transp.offset == 0) {
				fb->format = DISP_FORMAT_RGBA_4444;
			} else if (var->blue.offset == 12 &&
				   var->green.offset == 8 &&
				   var->red.offset == 4 &&
				   var->transp.offset == 0) {
				fb->format = DISP_FORMAT_BGRA_4444;
			} else if (var->transp.offset == 15 &&
				   var->red.offset == 10 &&
				   var->green.offset == 5 &&
				   var->blue.offset == 0) {
				fb->format = DISP_FORMAT_ARGB_1555;
			} else if (var->transp.offset == 15 &&
				   var->blue.offset == 10 &&
				   var->green.offset == 5 &&
				   var->red.offset == 0) {
				fb->format = DISP_FORMAT_ABGR_1555;
			} else if (var->red.offset == 11 &&
				   var->green.offset == 6 &&
				   var->blue.offset == 1 &&
				   var->transp.offset == 0) {
				fb->format = DISP_FORMAT_RGBA_5551;
			} else if (var->blue.offset == 11 &&
				   var->green.offset == 6 &&
				   var->red.offset == 1 &&
				   var->transp.offset == 0) {
				fb->format = DISP_FORMAT_BGRA_5551;
			} else {
				pr_warn("transp.offset:%d", var->transp.offset);
				pr_warn("red.offset:%d", var->red.offset);
				pr_warn("green.offset:%d", var->green.offset);
				pr_warn("blue.offset:%d", var->blue.offset);
			}

			break;

		default:
			pr_warn("invalid bits_per_pixel :%d\n",
				var->bits_per_pixel);
			return -EINVAL;
		}
	}

	__inf("var_to_disp_fb, format%d ", (int)fb->format);
	__inf("para: %dbpp ", (int)var->bits_per_pixel);
	__inf("alpha(%d,%d) ", (int)var->transp.offset,
	      (int)var->transp.length);
	__inf("reg(%d,%d) ", (int)var->red.offset, (int)var->red.length);
	__inf("green(%d,%d) ", (int)var->green.offset, (int)var->green.length);
	__inf("blue(%d,%d) ", (int)var->blue.offset, (int)var->blue.length);

	fb->size.width = var->xres_virtual;

	fix->line_length = (var->xres_virtual * var->bits_per_pixel) / 8;

	return 0;
}

static int Fb_open(struct fb_info *info, int user)
{
	return 0;
}
static int Fb_release(struct fb_info *info, int user)
{
	return 0;
}

int Fb_wait_for_vsync(struct fb_info *info)
{
	unsigned long count;
	u32 sel = 0;
	int ret;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for (sel = 0; sel < num_screens; sel++) {
		if (((sel == 0) &&
		     ((g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0)))
		    || ((sel == 1) &&
			((g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)))
		    || ((sel == 2) &&
			((g_fbi.fb_mode[info->node] == FB_MODE_SCREEN2)))) {
			if (bsp_disp_get_output_type(sel) ==
			    DISP_OUTPUT_TYPE_NONE) {
				return 0;
			}

			count = g_fbi.wait_count[sel];
			ret =
			    wait_event_interruptible_timeout(g_fbi.wait[sel],
							     count !=
							     g_fbi.
							     wait_count[sel],
							     msecs_to_jiffies
							     (50));
			if (ret == 0) {
				__inf("timeout\n");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static int Fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	u32 sel = 0;
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	/* __inf("Fb_pan_display\n"); */

	for (sel = 0; sel < num_screens; sel++) {
		if (((sel == 0) &&
		     ((g_fbi.fb_mode[info->node] == FB_MODE_SCREEN0)))
		    || ((sel == 1) &&
			((g_fbi.fb_mode[info->node] == FB_MODE_SCREEN1)))
		    || ((sel == 2) &&
			((g_fbi.fb_mode[info->node] == FB_MODE_SCREEN2)))) {
			s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
			disp_layer_info layer_para;
			u32 buffer_num = 1;
			u32 y_offset = 0;

			if (g_fbi.fb_mode[info->node] ==
			    FB_MODE_DUAL_SAME_SCREEN_TB) {
				buffer_num = 2;
			}
			if ((sel == 0) &&
			    (g_fbi.fb_mode[info->node] ==
			     FB_MODE_DUAL_SAME_SCREEN_TB)) {
				y_offset = var->yres / 2;
			}

			bsp_disp_layer_get_info(sel, layer_hdl, &layer_para);

			if (layer_para.mode == DISP_LAYER_WORK_MODE_SCALER) {
				layer_para.fb.src_win.x = var->xoffset;
				layer_para.fb.src_win.y =
				    var->yoffset + y_offset;
				layer_para.fb.src_win.width = var->xres;
				layer_para.fb.src_win.height =
				    var->yres / buffer_num;
			} else {
				layer_para.fb.src_win.x = var->xoffset;
				layer_para.fb.src_win.y =
				    var->yoffset + y_offset;
				layer_para.fb.src_win.width = var->xres;
				layer_para.fb.src_win.height =
				    var->yres / buffer_num;

				layer_para.screen_win.width = var->xres;
				layer_para.screen_win.height =
				    var->yres / buffer_num;
			}
			bsp_disp_layer_set_info(sel, layer_hdl, &layer_para);
		}
	}

	Fb_wait_for_vsync(info);

	return 0;
}

static int Fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
#if 0
	switch (var->bits_per_pixel) {
	case 16:
		disp_fb_to_var(DISP_FORMAT_ARGB_1555, var);
		break;
	case 24:
		disp_fb_to_var(DISP_FORMAT_RGB_888, var);
		break;
	case 32:
		disp_fb_to_var(DISP_FORMAT_ARGB_8888, var);
		break;
	default:
		return -EINVAL;
	}
#endif
	return 0;
}

static int Fb_set_par(struct fb_info *info)
{
	u32 sel = 0;
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Fb_set_par\n");

	for (sel = 0; sel < num_screens; sel++) {
		if (((sel == 0) &&
		     (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1)
		     && (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN2))
		    || ((sel == 1) &&
			(g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)
			&& (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN2))
		    || ((sel == 2) &&
			(g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0)
			&& (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))) {
			struct fb_var_screeninfo *var = &info->var;
			struct fb_fix_screeninfo *fix = &info->fix;
			s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];
			disp_layer_info layer_para;
			u32 buffer_num = 1;
			u32 y_offset = 0;

			if (g_fbi.fb_mode[info->node] ==
			    FB_MODE_DUAL_SAME_SCREEN_TB) {
				buffer_num = 2;
			}
			if ((sel == 0) &&
			    (g_fbi.fb_mode[info->node] ==
			     FB_MODE_DUAL_SAME_SCREEN_TB)) {
				y_offset = var->yres / 2;
			}
			bsp_disp_layer_get_info(sel, layer_hdl, &layer_para);

			var_to_disp_fb(&(layer_para.fb), var, fix);
			layer_para.fb.src_win.x = var->xoffset;
			layer_para.fb.src_win.y = var->yoffset + y_offset;
			layer_para.fb.src_win.width = var->xres;
			layer_para.fb.src_win.height = var->yres / buffer_num;
			if (layer_para.mode != DISP_LAYER_WORK_MODE_SCALER) {
				layer_para.screen_win.width =
				    layer_para.fb.src_win.width;
				layer_para.screen_win.height =
				    layer_para.fb.src_win.height;
			}
			bsp_disp_layer_set_info(sel, layer_hdl, &layer_para);
		}
	}
	return 0;
}

static int Fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned long off;
	unsigned long start;
	u32 len;

	off = vma->vm_pgoff << PAGE_SHIFT;

	/* frame buffer memory */
	start = info->fix.smem_start;
	if (start == 0)
		return -EINVAL;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len);
	if (off >= len) {
		/* memory mapped io */
		off -= len;
		if (info->var.accel_flags)
			return -EINVAL;
		start = info->fix.mmio_start;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.mmio_len);
	}
	start &= PAGE_MASK;
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO;	/* | VM_RESERVED;  ????? */
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	/*fb_pgprotect */
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
	return 0;

}

static int Fb_blank(int blank_mode, struct fb_info *info)
{
	u32 sel = 0;
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("Fb_blank,mode:%d\n", blank_mode);

	for (sel = 0; sel < num_screens; sel++) {
		if (((sel == 0) &&
		     (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1))
		    || ((sel == 1) &&
			(g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0))) {
			s32 layer_hdl = g_fbi.layer_hdl[info->node][sel];

			if (blank_mode == FB_BLANK_POWERDOWN)
				bsp_disp_layer_disable(sel, layer_hdl);
			else
				bsp_disp_layer_enable(sel, layer_hdl);

			/* DRV_disp_wait_cmd_finish(sel); */
		}
	}
	return 0;
}

static int Fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	__inf("Fb_cursor\n");

	return 0;
}

s32 DRV_disp_vsync_event(u32 sel)
{
	g_fbi.vsync_timestamp[sel] = ktime_get();

	schedule_work(&g_fbi.vsync_work[sel]);

	return 0;
}

static void send_vsync_work_0(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "VSYNC0=%llu",
		 ktime_to_ns(g_fbi.vsync_timestamp[0]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&g_fbi.dev->kobj, KOBJ_CHANGE, envp);
	fcount_data.vsync_count[0]++;
}

static void send_vsync_work_1(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "VSYNC1=%llu",
		 ktime_to_ns(g_fbi.vsync_timestamp[1]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&g_fbi.dev->kobj, KOBJ_CHANGE, envp);
	fcount_data.vsync_count[1]++;
}

static void send_vsync_work_2(struct work_struct *work)
{
	char buf[64];
	char *envp[2];

	snprintf(buf, sizeof(buf), "VSYNC2=%llu",
		 ktime_to_ns(g_fbi.vsync_timestamp[2]));
	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&g_fbi.dev->kobj, KOBJ_CHANGE, envp);
	fcount_data.vsync_count[2]++;
}

void DRV_disp_int_process(u32 sel)
{
	g_fbi.wait_count[sel]++;
	wake_up_interruptible(&g_fbi.wait[sel]);
}

int dispc_blank(int disp, int blank)
{
	int i = 0;

	if (blank) {
		for (i = 0; i < 4; i++)
			bsp_disp_layer_disable(disp, i);
	}
	g_fbi.blank[disp] = blank;

	return 0;
}

static int Fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	unsigned long layer_hdl = 0;

	switch (cmd) {
	case FBIOGET_LAYER_HDL_0:
		if (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN1) {
			layer_hdl = g_fbi.layer_hdl[info->node][0];
			ret = copy_to_user((void __user *)arg,
					   &layer_hdl, sizeof(unsigned long));
		} else {
			ret = -1;
		}
		break;

	case FBIOGET_LAYER_HDL_1:
		if (g_fbi.fb_mode[info->node] != FB_MODE_SCREEN0) {
			layer_hdl = g_fbi.layer_hdl[info->node][1];
			ret = copy_to_user((void __user *)arg,
					   &layer_hdl, sizeof(unsigned long));
		} else {
			ret = -1;
		}
		break;

#if 0
	case FBIOGET_VBLANK:
		{
			struct fb_vblank vblank;
			disp_video_timing tt;
			u32 line = 0;
			u32 sel;

			sel = (g_fbi.fb_mode[info->node] ==
			       FB_MODE_SCREEN1) ? 1 : 0;
			line = bsp_disp_get_cur_line(sel);
			bsp_disp_get_timming(sel, &tt);

			memset(&vblank, 0, sizeof(struct fb_vblank));
			vblank.flags |= FB_VBLANK_HAVE_VBLANK;
			vblank.flags |= FB_VBLANK_HAVE_VSYNC;
			if (line <= (tt.ver_total_time - tt.y_res))
				vblank.flags |= FB_VBLANK_VBLANKING;

			if ((line > tt.ver_front_porch) &&
			    (line < (tt.ver_front_porch + tt.ver_sync_time))) {
				vblank.flags |= FB_VBLANK_VSYNCING;
			}

			if (copy_to_user((void __user *)arg,
					 &vblank, sizeof(struct fb_vblank)))
				ret = -EFAULT;

			break;
		}
#endif

	case FBIO_WAITFORVSYNC:
		{
			/* ret = Fb_wait_for_vsync(info); */
			break;
		}

	default:
		/* __inf("not supported fb io cmd:%x\n", cmd); */
		break;
	}
	return ret;
}

static inline u32 convert_bitfield(int val, struct fb_bitfield *bf)
{
	u32 mask = ((1 << bf->length) - 1) << bf->offset;

	return (val << bf->offset) & mask;
}

static int Fb_setcolreg(unsigned regno,
			unsigned red, unsigned green,
			unsigned blue, unsigned transp, struct fb_info *info)
{
	u32 val;
	u32 ret = 0;

	switch (info->fix.visual) {
	case FB_VISUAL_PSEUDOCOLOR:
		ret = -EINVAL;
		break;
	case FB_VISUAL_TRUECOLOR:
		if (regno < 16) {
			val = convert_bitfield(transp, &info->var.transp) |
			    convert_bitfield(red, &info->var.red) |
			    convert_bitfield(green, &info->var.green) |
			    convert_bitfield(blue, &info->var.blue);
			__inf("Fb_setcolreg,regno=%2d,a=%2X\n", regno, transp);
			__inf("r=%2X,g=%2X,b=%2X, result=%08X\n",
			      red, green, blue, val);
			((u32 *) info->pseudo_palette)[regno] = val;
		} else {
			ret = 0;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int Fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	unsigned int j, r = 0;
	unsigned char hred, hgreen, hblue, htransp = 0xff;
	unsigned short *red, *green, *blue, *transp;

	__inf("Fb_setcmap, cmap start:%d len:%d, %dbpp\n", cmap->start,
	      cmap->len, info->var.bits_per_pixel);

	red = cmap->red;
	green = cmap->green;
	blue = cmap->blue;
	transp = cmap->transp;

	for (j = 0; j < cmap->len; j++) {
		hred = *red++;
		hgreen = *green++;
		hblue = *blue++;
		if (transp)
			htransp = (*transp++) & 0xff;
		else
			htransp = 0xff;

		r = Fb_setcolreg(cmap->start + j, hred, hgreen, hblue, htransp,
				 info);
		if (r)
			return r;
	}

	return 0;
}

static struct fb_ops dispfb_ops = {

	.owner = THIS_MODULE,
	.fb_open = Fb_open,
	.fb_release = Fb_release,
	.fb_pan_display = Fb_pan_display,
	.fb_ioctl = Fb_ioctl,
	.fb_check_var = Fb_check_var,
	.fb_set_par = Fb_set_par,
	.fb_setcolreg = Fb_setcolreg,
	.fb_setcmap = Fb_setcmap,
	.fb_blank = Fb_blank,
	.fb_mmap = Fb_mmap,
#if defined(CONFIG_FB_CONSOLE_SUNXI)
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
#endif
	.fb_cursor = Fb_cursor,
};

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN3IW1P1)
/* Greatest common divisor of x and y */
static unsigned long GCD(unsigned long x, unsigned long y)
{
	while (y != 0) {
		unsigned long r = x % y;

		x = y;
		y = r;
	}

	return x;
}

/* Least common multiple of x and y */
static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}

/* Round x up to a multiple of y */
static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}
#endif

s32 Display_Fb_Request(u32 fb_id, disp_fb_create_info *fb_para)
{
	struct fb_info *info = NULL;
	s32 hdl = 0;
	disp_layer_info layer_para;
	u32 sel;
	u32 xres, yres;
	u32 num_screens;
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN3IW1P1)
	unsigned long ulLCM;
#endif

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("%s,fb_id:%d\n", __func__, fb_id);

	if (g_fbi.fb_enable[fb_id]) {
		pr_warn("%s, fb%d is already requested!\n", __func__, fb_id);
		return DIS_NO_RES;
	}
	info = g_fbi.fbinfo[fb_id];

	xres = fb_para->width;
	yres = fb_para->height;
	if ((xres == 0) || (yres == 0)) {
		pr_warn("invalid paras xres(%d, yres(%d) in %s\n",
			xres, yres, __func__);
		return DIS_FAIL;
	}

	info->var.xoffset = 0;
	info->var.yoffset = 0;
	info->var.xres = xres;
	info->var.yres = yres;
	info->var.xres_virtual = xres;
#if !defined(CONFIG_ARCH_SUN8IW5P1)
	info->fix.line_length =
	    (fb_para->width * info->var.bits_per_pixel) >> 3;
	info->fix.smem_len =
	    info->fix.line_length * fb_para->height * fb_para->buffer_num;
	info->var.yres_virtual = info->fix.smem_len / info->fix.line_length;
#else
	info->fix.line_length =
	    (RoundUpToMultiple(fb_para->width, 16) *
	     info->var.bits_per_pixel) >> 3;
	ulLCM = LCM(info->fix.line_length, PAGE_SIZE);
	info->fix.smem_len =
	    RoundUpToMultiple(info->fix.line_length *
			      RoundUpToMultiple(fb_para->height, 16),
			      ulLCM) * fb_para->buffer_num;
	info->var.yres_virtual = info->fix.smem_len / info->fix.line_length;
#endif

	Fb_map_video_memory(info);

	for (sel = 0; sel < num_screens; sel++) {
		if (((sel == 0) && (fb_para->fb_mode != FB_MODE_SCREEN1))
		    || ((sel == 1) && (fb_para->fb_mode != FB_MODE_SCREEN0))
		    || ((sel == 2) && (fb_para->fb_mode == FB_MODE_SCREEN2))) {
			u32 y_offset = 0, src_width = xres, src_height = yres;

			if (((sel == 0) && (fb_para->fb_mode ==
					    FB_MODE_SCREEN0
					    || fb_para->fb_mode ==
					    FB_MODE_DUAL_SAME_SCREEN_TB))
			    || ((sel == 1) && (fb_para->fb_mode ==
					       FB_MODE_SCREEN1))
			    || ((sel == 2) && (fb_para->fb_mode ==
					       FB_MODE_SCREEN2))
			    || ((sel == fb_para->primary_screen_id) &&
				(fb_para->fb_mode ==
				 FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS))) {
				disp_video_timing tt;

				if (bsp_disp_get_timming(sel, &tt) >= 0) {
					if (tt.pixel_clk != 0)
						info->var.pixclock =
						    1000000000 / tt.pixel_clk;
					info->var.left_margin =
					    tt.hor_back_porch;
					info->var.right_margin =
					    tt.hor_front_porch;
					info->var.upper_margin =
					    tt.ver_back_porch;
					info->var.lower_margin =
					    tt.ver_front_porch;
					info->var.hsync_len = tt.hor_sync_time;
					info->var.vsync_len = tt.ver_sync_time;
				}
				info->var.width =
				    bsp_disp_get_screen_physical_width(sel);
				info->var.height =
				    bsp_disp_get_screen_physical_height(sel);
			}

			if (fb_para->fb_mode == FB_MODE_DUAL_SAME_SCREEN_TB) {
				src_height = yres / 2;
				if (sel == 0)
					y_offset = yres / 2;
			}

			memset(&layer_para, 0, sizeof(disp_layer_info));
			layer_para.mode = fb_para->mode;
			layer_para.screen_win.width = src_width;
			layer_para.screen_win.height = src_height;
			if (fb_para->fb_mode ==
			    FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS) {
				if (sel != fb_para->primary_screen_id) {
					layer_para.mode =
					    DISP_LAYER_WORK_MODE_SCALER;
					layer_para.screen_win.width =
					    fb_para->aux_output_width;
					layer_para.screen_win.height =
					    fb_para->aux_output_height;
				} else if (fb_para->mode ==
					   DISP_LAYER_WORK_MODE_SCALER) {
					layer_para.screen_win.width =
					    fb_para->output_width;
					layer_para.screen_win.height =
					    fb_para->output_height;
				}
			} else if (fb_para->mode
				   == DISP_LAYER_WORK_MODE_SCALER) {
				layer_para.screen_win.width =
				    fb_para->output_width;
				layer_para.screen_win.height =
				    fb_para->output_height;
			}

			hdl = 3;	/* fb bound to layer 3 */

			layer_para.pipe = 1;
			layer_para.alpha_mode = 0;
			layer_para.alpha_value = 0xff;
			layer_para.ck_enable = 0;
			layer_para.fb.src_win.x = 0;
			layer_para.fb.src_win.y = y_offset;
			layer_para.fb.src_win.width = src_width;
			layer_para.fb.src_win.height = src_height;
			layer_para.screen_win.x = 0;
			layer_para.screen_win.y = 0;
			var_to_disp_fb(&(layer_para.fb),
				       &(info->var), &(info->fix));
			layer_para.fb.addr[0] = (u32) info->fix.smem_start;
			layer_para.fb.addr[1] = 0;
			layer_para.fb.addr[2] = 0;
			layer_para.fb.size.width = fb_para->width;
			layer_para.fb.size.height = fb_para->height;
			layer_para.fb.cs_mode = DISP_BT601;

			/* Fb_map_boot_logo(sel, info); */
			bsp_disp_layer_set_info(sel, hdl, &layer_para);

			bsp_disp_layer_enable(sel, hdl);

			g_fbi.layer_hdl[fb_id][sel] = hdl;
		}
	}

	g_fbi.fb_enable[fb_id] = 1;
	g_fbi.fb_mode[fb_id] = fb_para->fb_mode;
	memcpy(&g_fbi.fb_para[fb_id], fb_para, sizeof(disp_fb_create_info));

	return DIS_SUCCESS;
}

s32 Display_Fb_Release(u32 fb_id)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("%s, fb_id:%d\n", __func__, fb_id);

	if (g_fbi.fb_enable[fb_id]) {
		u32 sel = 0;
		struct fb_info *info = g_fbi.fbinfo[fb_id];

		for (sel = 0; sel < num_screens; sel++) {
			if (((sel == 0) &&
			     (g_fbi.fb_mode[fb_id] != FB_MODE_SCREEN1))
			    || ((sel == 1) &&
				(g_fbi.fb_mode[fb_id] != FB_MODE_SCREEN0))) {
				s32 layer_hdl = g_fbi.layer_hdl[fb_id][sel];
				disp_layer_info layer_para;

				/* bsp_disp_layer_release(sel, layer_hdl); */
				bsp_disp_layer_get_info(sel,
							layer_hdl, &layer_para);
				memset(&layer_para, 0, sizeof(disp_layer_info));
				bsp_disp_layer_set_info(sel, layer_hdl,
							&layer_para);
			}
		}
		g_fbi.layer_hdl[fb_id][0] = 0;
		g_fbi.layer_hdl[fb_id][1] = 0;
		g_fbi.fb_mode[fb_id] = FB_MODE_SCREEN0;
		memset(&g_fbi.fb_para[fb_id], 0, sizeof(disp_fb_create_info));
		g_fbi.fb_enable[fb_id] = 0;

		fb_dealloc_cmap(&info->cmap);
		Fb_unmap_video_memory(info);
	} else {
		pr_warn("invalid paras fb_id:%d in %s\n", fb_id, __func__);
		return DIS_FAIL;
	}
	return DIS_SUCCESS;
}

s32 Display_set_fb_timming(u32 sel)
{
	u8 fb_id = 0;

	for (fb_id = 0; fb_id < FB_MAX; fb_id++) {
		if (g_fbi.fb_enable[fb_id]) {
			if (((sel == 0) &&
			     (g_fbi.fb_mode[fb_id] ==
			      FB_MODE_SCREEN0 ||
			      g_fbi.fb_mode[fb_id] ==
			      FB_MODE_DUAL_SAME_SCREEN_TB))
			    || ((sel == 1) && (g_fbi.fb_mode[fb_id] ==
					       FB_MODE_SCREEN1)) ||
			    ((sel == g_fbi.fb_para[fb_id].primary_screen_id)
			     && (g_fbi.fb_mode[fb_id] ==
				 FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS))) {
				disp_video_timing tt;

				if (bsp_disp_get_timming(sel, &tt)
				    >= 0) {
					if (tt.pixel_clk != 0)
						g_fbi.fbinfo[fb_id]->var.
						    pixclock =
						    1000000000 / tt.pixel_clk;
					g_fbi.fbinfo[fb_id]->var.left_margin =
					    tt.hor_back_porch;
					g_fbi.fbinfo[fb_id]->var.right_margin =
					    tt.hor_front_porch;
					g_fbi.fbinfo[fb_id]->var.upper_margin =
					    tt.ver_back_porch;
					g_fbi.fbinfo[fb_id]->var.lower_margin =
					    tt.ver_front_porch;
					g_fbi.fbinfo[fb_id]->var.hsync_len =
					    tt.hor_sync_time;
					g_fbi.fbinfo[fb_id]->var.vsync_len =
					    tt.ver_sync_time;
				}
			}
		}
	}

	return 0;
}

s32 Fb_Init(struct platform_device *pdev)
{
	disp_fb_create_info fb_para;
	s32 i;
	bool need_open_hdmi = 0;
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	__inf("[DISP]%s\n", __func__);

#if defined(CONFIG_ARCH_SUN6I) || \
	defined(CONFIG_ARCH_SUN8IW3P1) || \
	defined(CONFIG_ARCH_SUN8IW5P1) || \
	defined(CONFIG_ARCH_SUN3IW1P1) || \
	defined(CONFIG_ARCH_SUN9IW1P1)
	composer_init();
#endif

	INIT_WORK(&g_fbi.vsync_work[0], send_vsync_work_0);
	INIT_WORK(&g_fbi.vsync_work[1], send_vsync_work_1);
	INIT_WORK(&g_fbi.vsync_work[2], send_vsync_work_2);
	disp_register_sync_finish_proc(DRV_disp_int_process);

	for (i = 0; i < 8; i++) {
		g_fbi.fbinfo[i] = framebuffer_alloc(0, g_fbi.dev);
		g_fbi.fbinfo[i]->fbops = &dispfb_ops;
		g_fbi.fbinfo[i]->flags = 0;
		g_fbi.fbinfo[i]->device = g_fbi.dev;
		g_fbi.fbinfo[i]->par = &g_fbi;
		g_fbi.fbinfo[i]->var.xoffset = 0;
		g_fbi.fbinfo[i]->var.yoffset = 0;
		g_fbi.fbinfo[i]->var.xres = 800;
		g_fbi.fbinfo[i]->var.yres = 480;
		g_fbi.fbinfo[i]->var.xres_virtual = 800;
		g_fbi.fbinfo[i]->var.yres_virtual = 480 * 2;
		g_fbi.fbinfo[i]->var.nonstd = 0;
		g_fbi.fbinfo[i]->var.bits_per_pixel = 32;
		g_fbi.fbinfo[i]->var.transp.length = 8;
		g_fbi.fbinfo[i]->var.red.length = 8;
		g_fbi.fbinfo[i]->var.green.length = 8;
		g_fbi.fbinfo[i]->var.blue.length = 8;
		g_fbi.fbinfo[i]->var.transp.offset = 24;
		g_fbi.fbinfo[i]->var.red.offset = 16;
		g_fbi.fbinfo[i]->var.green.offset = 8;
		g_fbi.fbinfo[i]->var.blue.offset = 0;
		g_fbi.fbinfo[i]->var.activate = FB_ACTIVATE_FORCE;
		g_fbi.fbinfo[i]->fix.type = FB_TYPE_PACKED_PIXELS;
		g_fbi.fbinfo[i]->fix.type_aux = 0;
		g_fbi.fbinfo[i]->fix.visual = FB_VISUAL_TRUECOLOR;
		g_fbi.fbinfo[i]->fix.xpanstep = 1;
		g_fbi.fbinfo[i]->fix.ypanstep = 1;
		g_fbi.fbinfo[i]->fix.ywrapstep = 0;
		g_fbi.fbinfo[i]->fix.accel = FB_ACCEL_NONE;
		g_fbi.fbinfo[i]->fix.line_length =
		    g_fbi.fbinfo[i]->var.xres_virtual * 4;
		g_fbi.fbinfo[i]->fix.smem_len =
		    g_fbi.fbinfo[i]->fix.line_length *
		    g_fbi.fbinfo[i]->var.yres_virtual * 2;
		g_fbi.fbinfo[i]->screen_base = NULL;
		g_fbi.fbinfo[i]->pseudo_palette = g_fbi.pseudo_palette[i];
		g_fbi.fbinfo[i]->fix.smem_start = 0x0;
		g_fbi.fbinfo[i]->fix.mmio_start = 0;
		g_fbi.fbinfo[i]->fix.mmio_len = 0;

		if (fb_alloc_cmap(&g_fbi.fbinfo[i]->cmap, 256, 1) < 0)
			return -ENOMEM;

	}

	parser_disp_init_para(pdev->dev.of_node, &(g_fbi.disp_init));

	if (g_fbi.disp_init.b_init) {
		u32 sel = 0;

		for (sel = 0; sel < num_screens; sel++) {
			if ((sel == 0) &&
			    (g_fbi.disp_init.disp_mode ==
			     DISP_INIT_MODE_SCREEN0)) {
				if (g_fbi.disp_init.output_type[sel] ==
				    DISP_OUTPUT_TYPE_HDMI) {
					need_open_hdmi = 1;
				}
			}
		}
	}

	if (g_fbi.disp_init.b_init) {
		u32 fb_num = 0;

		fb_num = (g_fbi.disp_init.disp_mode ==
			  DISP_INIT_MODE_TWO_DIFF_SCREEN) ? 2 : 1;
		for (i = 0; i < fb_num; i++) {
			u32 screen_id = i;

			if (g_fbi.disp_init.disp_mode
			    == DISP_INIT_MODE_SCREEN1)
				screen_id = 1;

			if (g_fbi.disp_init.disp_mode
			    == DISP_INIT_MODE_SCREEN2)
				screen_id = 2;

			fb_para.buffer_num = g_fbi.disp_init.buffer_num[i];
			if ((g_fbi.disp_init.fb_width[i] == 0) ||
			    (g_fbi.disp_init.fb_height[i] == 0)) {
				fb_para.width =
				    bsp_disp_get_screen_width_from_output_type
				    (screen_id,
				     g_fbi.disp_init.output_type[screen_id],
				     g_fbi.disp_init.output_mode[screen_id]);
				fb_para.height =
				    bsp_disp_get_screen_height_from_output_type
				    (screen_id,
				     g_fbi.disp_init.output_type[screen_id],
				     g_fbi.disp_init.output_mode[screen_id]);
			} else {
				fb_para.width = g_fbi.disp_init.fb_width[i];
				fb_para.height = g_fbi.disp_init.fb_height[i];
			}
			fb_para.output_width =
			    bsp_disp_get_screen_width_from_output_type
			    (screen_id, g_fbi.disp_init.output_type[screen_id],
			     g_fbi.disp_init.output_mode[screen_id]);
			fb_para.output_height =
			    bsp_disp_get_screen_height_from_output_type
			    (screen_id, g_fbi.disp_init.output_type[screen_id],
			     g_fbi.disp_init.output_mode[screen_id]);
			fb_para.mode =
			    (g_fbi.disp_init.scaler_mode[i] ==
			     0) ? DISP_LAYER_WORK_MODE_NORMAL :
			    DISP_LAYER_WORK_MODE_SCALER;
			if (g_fbi.disp_init.disp_mode
			    == DISP_INIT_MODE_SCREEN0) {
				fb_para.fb_mode = FB_MODE_SCREEN0;
			} else if (g_fbi.disp_init.disp_mode ==
				   DISP_INIT_MODE_SCREEN1) {
				fb_para.fb_mode = FB_MODE_SCREEN1;
			} else if (g_fbi.disp_init.disp_mode ==
				   DISP_INIT_MODE_TWO_DIFF_SCREEN) {
				if (i == 0)
					fb_para.fb_mode = FB_MODE_SCREEN0;
				else
					fb_para.fb_mode = FB_MODE_SCREEN1;
			} else if (g_fbi.disp_init.disp_mode ==
				   DISP_INIT_MODE_TWO_SAME_SCREEN) {
				fb_para.fb_mode = FB_MODE_DUAL_SAME_SCREEN_TB;
				fb_para.height *= 2;
				fb_para.output_height *= 2;
			} else if (g_fbi.disp_init.disp_mode ==
				DISP_INIT_MODE_TWO_DIFF_SCREEN_SAME_CONTENTS) {
				fb_para.fb_mode =
				    FB_MODE_DUAL_DIFF_SCREEN_SAME_CONTENTS;
				fb_para.output_width =
				    bsp_disp_get_screen_width_from_output_type
				    (fb_para.primary_screen_id,
				     g_fbi.disp_init.output_type[fb_para.primary_screen_id],
				     g_fbi.disp_init.output_mode[fb_para.primary_screen_id]);
				fb_para.output_height =
				    bsp_disp_get_screen_height_from_output_type
				    (fb_para.primary_screen_id,
				     g_fbi.disp_init.output_type[fb_para.primary_screen_id],
				     g_fbi.disp_init.output_mode[fb_para.primary_screen_id]);
				fb_para.aux_output_width =
				    bsp_disp_get_screen_width(1 -
					      fb_para.primary_screen_id);
				fb_para.aux_output_height =
				    bsp_disp_get_screen_width_from_output_type(
					       1 - fb_para.primary_screen_id,
					       g_fbi.disp_init.output_type[1 - fb_para.primary_screen_id],
					       g_fbi.disp_init.output_mode[1 - fb_para.primary_screen_id]);
			} else if (g_fbi.disp_init.disp_mode ==
				   DISP_INIT_MODE_SCREEN2) {
				fb_para.fb_mode = FB_MODE_SCREEN2;
			}
			Display_Fb_Request(i, &fb_para);

			for (i = 0; i < 8; i++)
				register_framebuffer(g_fbi.fbinfo[i]);
		}

#if 0
		if (g_fbi.disp_init.scaler_mode[0])
			bsp_disp_print_reg(0, DISP_MOD_FE0, 0);

		if (g_fbi.disp_init.scaler_mode[1])
			bsp_disp_print_reg(0, DISP_MOD_FE1, 0);

		if (g_fbi.disp_init.disp_mode != DISP_INIT_MODE_SCREEN1) {
			bsp_disp_print_reg(0, DISP_MOD_BE0, 0);
			bsp_disp_print_reg(0, DISP_MOD_LCD0, 0);
			if ((g_fbi.disp_init.output_type[0] ==
			     DISP_OUTPUT_TYPE_TV)
			    || (g_fbi.disp_init.output_type[0] ==
				DISP_OUTPUT_TYPE_VGA))
				bsp_disp_print_reg(0, DISP_MOD_TVE0, 0);
		}
		if (g_fbi.disp_init.disp_mode != DISP_INIT_MODE_SCREEN0) {
			bsp_disp_print_reg(0, DISP_MOD_BE1, 0);
			bsp_disp_print_reg(0, DISP_MOD_LCD1, 0);
			if ((g_fbi.disp_init.output_type[1] ==
			     DISP_OUTPUT_TYPE_TV)
			    || (g_fbi.disp_init.output_type[1] ==
				DISP_OUTPUT_TYPE_VGA))
				bsp_disp_print_reg(0, DISP_MOD_TVE1, 0);
		}
		bsp_disp_print_reg(0, DISP_MOD_CCMU, 0);
		bsp_disp_print_reg(0, DISP_MOD_PWM, 0);
		bsp_disp_print_reg(0, DISP_MOD_PIOC, 0);
#endif
	}

	return 0;
}

s32 Fb_Exit(void)
{
	u8 fb_id = 0;

	for (fb_id = 0; fb_id < FB_MAX; fb_id++) {
		if (g_fbi.fbinfo[fb_id] != NULL)
			Display_Fb_Release(FBIDTOHAND(fb_id));
	}

	for (fb_id = 0; fb_id < 8; fb_id++) {
		unregister_framebuffer(g_fbi.fbinfo[fb_id]);
		framebuffer_release(g_fbi.fbinfo[fb_id]);
		g_fbi.fbinfo[fb_id] = NULL;
	}

	return 0;
}
