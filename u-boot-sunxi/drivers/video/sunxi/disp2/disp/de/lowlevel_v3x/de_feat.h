/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_FEAT_H_
#define _DE_FEAT_H_

#include <common.h>

#define DE_OUTPUT_TYPE_LCD   1
#define DE_OUTPUT_TYPE_TV    2
#define DE_OUTPUT_TYPE_HDMI  4
#define DE_OUTPUT_TYPE_VGA   8
#define DE_OUTPUT_TYPE_EDP 32 /*16 for vdpo module*/

#define de_writel(val, addr) writel(val, (void __iomem *)(addr))
#define de_readl(addr) readl((void __iomem *)(addr))

#if defined(CONFIG_ARCH_SUN50IW3P1)
/* features for sun50iw3 */

#define DEVICE_NUM	3
#define DE_NUM	2
#define CHN_NUM		4
#define VI_CHN_NUM	1
#define UI_CHN_NUM	(CHN_NUM - VI_CHN_NUM)
#define LAYER_NUM_PER_CHN_PER_VI_CHN	4
#define LAYER_NUM_PER_CHN_PER_UI_CHN	4
#define LAYER_MAX_NUM_PER_CHN 4
/* the number of TCON_LCD */
#define DEVICE_LCD_NUM 2

#define SUPPORT_DSI
#define DSI_VERSION_28
#define CLK_NUM_PER_DSI 2

#define DEVICE_DSI_NUM 2
/* #define SUPPORT_SMBL */
#define SUPPORT_ATW
/* #define SUPPORT_HDMI */
/* #define DSI_VERSION_40 */
#define HAVE_DEVICE_COMMON_MODULE
#define DEVICE_COMMON_VERSION2
/* #define SUPPORT_TV */
/* #define SUPPORT_VGA */
#if defined(CONFIG_FPGA_V4_PLATFORM) \
	|| defined(CONFIG_FPGA_V7_PLATFORM)
#define SUPPORT_LVDS
#endif
#define LVDS_REVERT

#elif defined(CONFIG_ARCH_SUN50IW6P1)

/* features for sun50iw6 */

#define DEVICE_NUM	2
#define DE_NUM	2
#define CHN_NUM		4
#define VI_CHN_NUM	1
#define UI_CHN_NUM	(CHN_NUM - VI_CHN_NUM)
#define LAYER_NUM_PER_CHN_PER_VI_CHN	4
#define LAYER_NUM_PER_CHN_PER_UI_CHN	4
#define LAYER_MAX_NUM_PER_CHN 4
/* the number of TCON_LCD */
#define DEVICE_LCD_NUM 1

/* #define SUPPORT_DSI */
/* #define SUPPORT_SMBL */
#define SUPPORT_DE_NOISE
/* CDC: color space & dynamic range convert */
#define SUPPORT_CDC
/* databit & yuv format conversion(yuv444/422/420) */
#define SUPPORT_FORMATTER
/* #define SUPPORT_HDMI */
/* #define DSI_VERSION_40 */
#define HAVE_DEVICE_COMMON_MODULE
#define DEVICE_COMMON_VERSION2
/* #define SUPPORT_TV */
/* #define SUPPORT_VGA */
/* #define SUPPORT_LVDS */
/* #define LVDS_REVERT */

#elif defined(CONFIG_ARCH_SUN8IW11P1)

/* features for sun8iw11, only for verify */

#define DEVICE_NUM	4
#define DE_NUM	2
#define CHN_NUM		4
#define VI_CHN_NUM	1
#define UI_CHN_NUM	(CHN_NUM - VI_CHN_NUM)
#define LAYER_NUM_PER_CHN_PER_VI_CHN	4
#define LAYER_NUM_PER_CHN_PER_UI_CHN	4
#define LAYER_MAX_NUM_PER_CHN 4
/* the number of TCON_LCD */
#define DEVICE_LCD_NUM 2

#define SUPPORT_DSI
#define SUPPORT_SMBL
#define SUPPORT_SMBL
#define SUPPORT_DE_NOISE
/* CDC: color space & dynamic range convert */
#define SUPPORT_CDC
/* databit & yuv format conversion(yuv444/422/420) */
#define SUPPORT_FORMATTER
#define SUPPORT_HDMI
#define DSI_VERSION_40
#define HAVE_DEVICE_COMMON_MODULE
#define SUPPORT_TV
#define SUPPORT_VGA
#define SUPPORT_LVDS
/* #define LVDS_REVERT */

#endif

/*common macro define*/

#ifndef CLK_NUM_PER_DSI
#define CLK_NUM_PER_DSI 1
#endif

#ifndef DEVICE_DSI_NUM
#define DEVICE_DSI_NUM 1
#endif /*endif DEVICE_DSI_NUM */

/* total number of DSI clk */
#define CLK_DSI_NUM  (CLK_NUM_PER_DSI * DEVICE_DSI_NUM)
struct de_feat {
	const int num_screens;
	/* indicate layer manager number */
	const int num_devices;
	/*indicate timing controller number */
	const int *num_chns;
	const int *num_vi_chns;
	const int *num_layers;
	const int *is_support_vep;
	const int *is_support_smbl;
	const int *is_support_wb;
	const int *supported_output_types;
	const int *is_support_scale;
	const int *scale_line_buffer_rgb;
	const int *scale_line_buffer_yuv;
	const int *scale_line_buffer_ed;
	const int *is_support_edscale;
	const int *is_support_de_noise;
	const int *is_support_cdc;
};

int de_feat_init(void);
int de_feat_get_num_screens(void);
int de_feat_get_num_devices(void);
int de_feat_get_num_chns(unsigned int disp);
int de_feat_get_num_vi_chns(unsigned int disp);
int de_feat_get_num_ui_chns(unsigned int disp);
int de_feat_get_num_layers(unsigned int disp);
int de_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn);
int de_feat_is_support_vep(unsigned int disp);
int de_feat_is_support_vep_by_chn(unsigned int disp, unsigned int chn);
int de_feat_is_support_smbl(unsigned int disp);
int de_feat_is_supported_output_types(unsigned int disp,
				      unsigned int output_type);
int de_feat_is_support_wb(unsigned int disp);
int de_feat_is_support_scale(unsigned int disp);
int de_feat_is_support_scale_by_chn(unsigned int disp, unsigned int chn);
int de_feat_is_support_edscale(unsigned int disp);
int de_feat_is_support_edscale_by_chn(unsigned int disp, unsigned int chn);
int de_feat_is_support_de_noise(unsigned int disp);
int de_feat_is_support_de_noise_by_chn(unsigned int disp, unsigned int chn);
int de_feat_is_support_cdc_by_chn(unsigned int disp, unsigned int chn);
int de_feat_get_scale_linebuf_for_yuv(unsigned int disp, unsigned int chn);
int de_feat_get_scale_linebuf_for_rgb(unsigned int disp, unsigned int chn);
int de_feat_get_scale_linebuf_for_ed(unsigned int disp, unsigned int chn);

#endif
