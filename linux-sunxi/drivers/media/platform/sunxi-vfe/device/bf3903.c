/*
 * BF3903 sensor driver
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 * Author: raymonxiu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"

MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for GalaxyCore BF3903 sensors");
MODULE_LICENSE("GPL");

#define DEV_DBG_EN   		1
#if (DEV_DBG_EN == 1)
#define vfe_dev_dbg(x, arg...) printk("[CSI_DEBUG][BF3903]"x, ##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...) printk("[CSI_ERR][BF3903]"x, ##arg)
#define vfe_dev_print(x, arg...) printk("[CSI][BF3903]"x, ##arg)

#define LOG_ERR_RET(x)  { \
							int ret;  \
							ret = x; \
							if (ret < 0) {\
								vfe_dev_err("error at %s\n", __func__);  \
								return ret; \
						} \
						}

#define MCLK (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL              V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x3903

#define CSI_STBY_ON			1
#define CSI_STBY_OFF 		0
#define CSI_RST_ON			0
#define CSI_RST_OFF			1
#define CSI_PWR_ON			1
#define CSI_PWR_OFF			0

#define regval_list reg_list_a8_d8
#define REG_TERM 0xff
#define VAL_TERM 0xff
#define REG_DLY  0xffff

#define SENSOR_FRAME_RATE 30

#define I2C_ADDR 0xdc

#define BF3903_OUTPUT_MODE_CCIR601_8BIT

struct sensor_format_struct;

struct cfg_array {
	struct regval_list *regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}


static struct regval_list sensor_default_regs_24M[] = {

};

static struct regval_list sensor_default_regs[] = {
	{0x12, 0x80},
	{0x11, 0x80},
	{0x09, 0x01},
	{0x15, 0x00},
	{0x3a, 0x00},
	{0x13, 0x00},
	{0x01, 0x11},
	{0x02, 0x22},
	{0x8c, 0x02},
	{0x8d, 0x64},
	{0x9d, 0x2f},
	{0x13, 0x07},
	{0x20, 0x89},
	{0x2f, 0xc5},
	{0x16, 0xa1},
	{0x06, 0x68},
	{0x08, 0x10},
	{0x36, 0x45},
	{0x35, 0x46},
	{0x65, 0x46},
	{0x66, 0x46},
	{0xbe, 0x44},
	{0xbd, 0xf4},
	{0xbc, 0x0d},
	{0x9c, 0x44},
	{0x9b, 0xf4},
	{0x37, 0xf4},
	{0x38, 0x44},
	{0x6e, 0x10},
	{0x70, 0x0f},
	{0x72, 0x2f},
	{0x73, 0x2f},
	{0x74, 0x27},
	{0x75, 0x35},
	{0x76, 0x90},
	{0x79, 0x2d},
	{0x7a, 0x20},
	{0x7e, 0x1a},
	{0x7c, 0x88},
	{0x7d, 0xba},
	{0x5b, 0xc2},
	{0x7b, 0x55},
	{0x25, 0x88},
	{0x80, 0x82},
	{0x81, 0xa0},
	{0x82, 0x2d},
	{0x83, 0x5a},
	{0x84, 0x30},
	{0x85, 0x40},
	{0x86, 0x40},
	{0x89, 0x1d},
	{0x8a, 0x99},
	{0x8b, 0x7f},
	{0x8e, 0x2c},
	{0x8f, 0x82},
	{0x97, 0x78},
	{0x98, 0x12},
	{0x24, 0xa0},
	{0x95, 0x80},
	{0x9a, 0xc0},
	{0x9e, 0x50},
	{0x9f, 0x50},
	{0x39, 0x80},
	{0x3f, 0x80},
	{0x90, 0x20},
	{0x91, 0x1c},
	{0x3b, 0x60},
	{0x3c, 0x20},
	{0x55, 0x00},
	{0x56, 0x40},
	{0x40, 0x80},
	{0x41, 0x80},
	{0x42, 0x68},
	{0x43, 0x53},
	{0x44, 0x48},
	{0x45, 0x41},
	{0x46, 0x3b},
	{0x47, 0x36},
	{0x48, 0x32},
	{0x49, 0x2f},
	{0x4b, 0x2c},
	{0x4c, 0x2a},
	{0x4e, 0x25},
	{0x4f, 0x22},
	{0x50, 0x20},
	{0x5a, 0x56},
	{0x51, 0x08},
	{0x52, 0x1a},
	{0x53, 0xb4},
	{0x54, 0x8d},
	{0x57, 0xab},
	{0x58, 0x40},
	{0x5a, 0xd6},
	{0x51, 0x03},
	{0x52, 0x0d},
	{0x53, 0x77},
	{0x54, 0x66},
	{0x57, 0x8b},
	{0x58, 0x37},
	{0x5b, 0xc2},
	{0x5c, 0x28},
	{0xb0, 0xd0},
	{0xb4, 0x63},
	{0xb1, 0xc0},
	{0xb2, 0xc0},
	{0xb4, 0xe3},
	{0xb1, 0xff},
	{0xb2, 0xa8},
	{0xb3, 0x4c},
	{0x6a, 0x81},
	{0x23, 0x66},
	{0xa0, 0xd0},
	{0xa1, 0x31},
	{0xa2, 0x0b},
	{0xa3, 0x26},
	{0xa4, 0x09},
	{0xa5, 0x26},
	{0xa6, 0x06},
	{0xa7, 0x97},
	{0xa8, 0x16},
	{0xa9, 0x50},
	{0xaa, 0x50},
	{0xab, 0x1a},
	{0xac, 0x3c},
	{0xad, 0xf0},
	{0xae, 0x37},
	{0xc5, 0xaa},
	{0xc6, 0xaa},
	{0xc8, 0x0d},
	{0xc9, 0x0f},
	{0xd0, 0x00},
	{0xd1, 0x00},
	{0xd2, 0x78},
	{0xd3, 0x09},
	{0xd4, 0x24},
	{0x69, 0x00},
	{0x6f, 0x5f},

	{0x62, 0x00},
	{0x6b, 0x02},
	{0x1b, 0x80},
	{0x11, 0x90},
	{0x17, 0x00},
	{0x18, 0xa0},
	{0x19, 0x00},
	{0x1a, 0x78},
	{0x03, 0x00},

	{0x55, 0x08},
	{0x1e, 0x00},
	{0x89, 0x7d},
	{0x20, 0x40},
	{0x09, 0xc0},
};

static struct regval_list sensor_wb_manual[] = {
};

static struct regval_list sensor_wb_auto_regs[] = {
	{0x13, 0x07},
	{0x01, 0x15},
	{0x02, 0x24},
	{0x6a, 0x81},
	{0xff, 0xff},

};

static struct regval_list sensor_wb_incandescence_regs[] = {
	{0x13, 0x05},
	{0x01, 0x1f},
	{0x02, 0x15},
	{0x6a, 0x01},
	{0xff, 0xff},
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	{0x13, 0x05},
	{0x01, 0x1a},
	{0x02, 0x1e},
	{0x6a, 0x01},
	{0xff, 0xff},
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	{0x13, 0x05},
	{0x01, 0x28},
	{0x02, 0x0a},
};

static struct regval_list sensor_wb_horizon[] = {
};

static struct regval_list sensor_wb_daylight_regs[] = {

	{0x13, 0x05},
	{0x01, 0x13},
	{0x02, 0x26},
	{0x6a, 0x01},
	{0xff, 0xff},
};

static struct regval_list sensor_wb_flash[] = {
};

static struct regval_list sensor_wb_cloud_regs[] = {

	{0x13, 0x05},
	{0x01, 0x10},
	{0x02, 0x28},
	{0x6a, 0x01},
	{0xff, 0xff},
};

static struct regval_list sensor_wb_shade[] = {
};

static struct cfg_array sensor_wb[] = {
	{
	 .regs = sensor_wb_manual,
	 .size = ARRAY_SIZE(sensor_wb_manual),
	 },
	{
	 .regs = sensor_wb_auto_regs,
	 .size = ARRAY_SIZE(sensor_wb_auto_regs),
	 },
	{
	 .regs = sensor_wb_incandescence_regs,
	 .size = ARRAY_SIZE(sensor_wb_incandescence_regs),
	 },
	{
	 .regs = sensor_wb_fluorescent_regs,
	 .size = ARRAY_SIZE(sensor_wb_fluorescent_regs),
	 },
	{
	 .regs = sensor_wb_tungsten_regs,
	 .size = ARRAY_SIZE(sensor_wb_tungsten_regs),
	 },
	{
	 .regs = sensor_wb_horizon,
	 .size = ARRAY_SIZE(sensor_wb_horizon),
	 },
	{
	 .regs = sensor_wb_daylight_regs,
	 .size = ARRAY_SIZE(sensor_wb_daylight_regs),
	 },
	{
	 .regs = sensor_wb_flash,
	 .size = ARRAY_SIZE(sensor_wb_flash),
	 },
	{
	 .regs = sensor_wb_cloud_regs,
	 .size = ARRAY_SIZE(sensor_wb_cloud_regs),
	 },
	{
	 .regs = sensor_wb_shade,
	 .size = ARRAY_SIZE(sensor_wb_shade),
	 },
};

static struct regval_list sensor_colorfx_none_regs[] = {

	{0x70, 0x0f},
	{0xb4, 0x60},
	{0x98, 0x02},
	{0x69, 0x1a},
	{0x67, 0x80},
	{0x68, 0x80},
	{0xff, 0xff},
};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{0x70, 0x0f},
	{0xb4, 0x00},
	{0x98, 0x82},
	{0x69, 0x20},
	{0x67, 0x80},
	{0x68, 0x80},
	{0xff, 0xff},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {

	{0x70, 0x0f},
	{0xb4, 0x00},
	{0x98, 0x82},
	{0x69, 0x20},
	{0x67, 0x60},
	{0x68, 0x98},
	{0xff, 0xff},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{0x70, 0x0f},
	{0xb4, 0x60},
	{0x98, 0x02},
	{0x69, 0x40},
	{0x67, 0x80},
	{0x68, 0x80},
	{0xff, 0xff},

};

static struct regval_list sensor_colorfx_emboss_regs[] = {
	{0xff, 0xff},
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
	{0xff, 0xff},

};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{0x70, 0x0f},
	{0xb4, 0x00},
	{0x98, 0x82},
	{0x69, 0x20},
	{0x67, 0xc0},
	{0x68, 0x80},
	{0xff, 0xff},
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {

	{0x70, 0x0f},
	{0xb4, 0x00},
	{0x98, 0x82},
	{0x69, 0x20},
	{0x67, 0x40},
	{0x68, 0x40},
	{0xff, 0xff}
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
};

static struct regval_list sensor_colorfx_aqua_regs[] = {
};

static struct regval_list sensor_colorfx_art_freeze_regs[] = {
};

static struct regval_list sensor_colorfx_silhouette_regs[] = {
};

static struct regval_list sensor_colorfx_solarization_regs[] = {
};

static struct regval_list sensor_colorfx_antique_regs[] = {
};

static struct regval_list sensor_colorfx_set_cbcr_regs[] = {
};

static struct cfg_array sensor_colorfx[] = {
	{
	 .regs = sensor_colorfx_none_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_none_regs),
	 },
	{
	 .regs = sensor_colorfx_bw_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_bw_regs),
	 },
	{
	 .regs = sensor_colorfx_sepia_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sepia_regs),
	 },
	{
	 .regs = sensor_colorfx_negative_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_negative_regs),
	 },
	{
	 .regs = sensor_colorfx_emboss_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_emboss_regs),
	 },
	{
	 .regs = sensor_colorfx_sketch_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sketch_regs),
	 },
	{
	 .regs = sensor_colorfx_sky_blue_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sky_blue_regs),
	 },
	{
	 .regs = sensor_colorfx_grass_green_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_grass_green_regs),
	 },
	{
	 .regs = sensor_colorfx_skin_whiten_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_skin_whiten_regs),
	 },
	{
	 .regs = sensor_colorfx_vivid_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_vivid_regs),
	 },
	{
	 .regs = sensor_colorfx_aqua_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_aqua_regs),
	 },
	{
	 .regs = sensor_colorfx_art_freeze_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_art_freeze_regs),
	 },
	{
	 .regs = sensor_colorfx_silhouette_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_silhouette_regs),
	 },
	{
	 .regs = sensor_colorfx_solarization_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_solarization_regs),
	 },
	{
	 .regs = sensor_colorfx_antique_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_antique_regs),
	 },
	{
	 .regs = sensor_colorfx_set_cbcr_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_set_cbcr_regs),
	 },
};

static struct regval_list sensor_brightness_neg4_regs[] = {
};

static struct regval_list sensor_brightness_neg3_regs[] = {
};

static struct regval_list sensor_brightness_neg2_regs[] = {
};

static struct regval_list sensor_brightness_neg1_regs[] = {
};

static struct regval_list sensor_brightness_zero_regs[] = {
};

static struct regval_list sensor_brightness_pos1_regs[] = {
};

static struct regval_list sensor_brightness_pos2_regs[] = {
};

static struct regval_list sensor_brightness_pos3_regs[] = {
};

static struct regval_list sensor_brightness_pos4_regs[] = {
};

static struct cfg_array sensor_brightness[] = {
	{
	 .regs = sensor_brightness_neg4_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg4_regs),
	 },
	{
	 .regs = sensor_brightness_neg3_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg3_regs),
	 },
	{
	 .regs = sensor_brightness_neg2_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg2_regs),
	 },
	{
	 .regs = sensor_brightness_neg1_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg1_regs),
	 },
	{
	 .regs = sensor_brightness_zero_regs,
	 .size = ARRAY_SIZE(sensor_brightness_zero_regs),
	 },
	{
	 .regs = sensor_brightness_pos1_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos1_regs),
	 },
	{
	 .regs = sensor_brightness_pos2_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos2_regs),
	 },
	{
	 .regs = sensor_brightness_pos3_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos3_regs),
	 },
	{
	 .regs = sensor_brightness_pos4_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos4_regs),
	 },
};

static struct regval_list sensor_contrast_neg4_regs[] = {
};

static struct regval_list sensor_contrast_neg3_regs[] = {
};

static struct regval_list sensor_contrast_neg2_regs[] = {
};

static struct regval_list sensor_contrast_neg1_regs[] = {
};

static struct regval_list sensor_contrast_zero_regs[] = {
};

static struct regval_list sensor_contrast_pos1_regs[] = {
};

static struct regval_list sensor_contrast_pos2_regs[] = {
};

static struct regval_list sensor_contrast_pos3_regs[] = {
};

static struct regval_list sensor_contrast_pos4_regs[] = {
};

static struct cfg_array sensor_contrast[] = {
	{
	 .regs = sensor_contrast_neg4_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg4_regs),
	 },
	{
	 .regs = sensor_contrast_neg3_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg3_regs),
	 },
	{
	 .regs = sensor_contrast_neg2_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg2_regs),
	 },
	{
	 .regs = sensor_contrast_neg1_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg1_regs),
	 },
	{
	 .regs = sensor_contrast_zero_regs,
	 .size = ARRAY_SIZE(sensor_contrast_zero_regs),
	 },
	{
	 .regs = sensor_contrast_pos1_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos1_regs),
	 },
	{
	 .regs = sensor_contrast_pos2_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos2_regs),
	 },
	{
	 .regs = sensor_contrast_pos3_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos3_regs),
	 },
	{
	 .regs = sensor_contrast_pos4_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos4_regs),
	 },
};

static struct regval_list sensor_saturation_neg4_regs[] = {
};

static struct regval_list sensor_saturation_neg3_regs[] = {
};

static struct regval_list sensor_saturation_neg2_regs[] = {
};

static struct regval_list sensor_saturation_neg1_regs[] = {
};

static struct regval_list sensor_saturation_zero_regs[] = {
};

static struct regval_list sensor_saturation_pos1_regs[] = {
};

static struct regval_list sensor_saturation_pos2_regs[] = {
};

static struct regval_list sensor_saturation_pos3_regs[] = {
};

static struct regval_list sensor_saturation_pos4_regs[] = {
};

static struct cfg_array sensor_saturation[] = {
	{
	 .regs = sensor_saturation_neg4_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg4_regs),
	 },
	{
	 .regs = sensor_saturation_neg3_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg3_regs),
	 },
	{
	 .regs = sensor_saturation_neg2_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg2_regs),
	 },
	{
	 .regs = sensor_saturation_neg1_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg1_regs),
	 },
	{
	 .regs = sensor_saturation_zero_regs,
	 .size = ARRAY_SIZE(sensor_saturation_zero_regs),
	 },
	{
	 .regs = sensor_saturation_pos1_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos1_regs),
	 },
	{
	 .regs = sensor_saturation_pos2_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos2_regs),
	 },
	{
	 .regs = sensor_saturation_pos3_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos3_regs),
	 },
	{
	 .regs = sensor_saturation_pos4_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos4_regs),
	 },
};

static struct regval_list sensor_ev_neg4_regs[] = {
	{0x55, 0xe8},

};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0x55, 0xe8},

};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0x55, 0xd8},

};

static struct regval_list sensor_ev_neg1_regs[] = {
	{0x55, 0xb8},

};

static struct regval_list sensor_ev_zero_regs[] = {
	{0x55, 0x08},
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0x55, 0x28},
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0x55, 0x38},
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0x55, 0x58},
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0x55, 0x78},
};

extern void i2c_del_driver(struct i2c_driver *driver);

static struct cfg_array sensor_ev[] = {
	{
	 .regs = sensor_ev_neg4_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg4_regs),
	 },
	{
	 .regs = sensor_ev_neg3_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg3_regs),
	 },
	{
	 .regs = sensor_ev_neg2_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg2_regs),
	 },
	{
	 .regs = sensor_ev_neg1_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg1_regs),
	 },
	{
	 .regs = sensor_ev_zero_regs,
	 .size = ARRAY_SIZE(sensor_ev_zero_regs),
	 },
	{
	 .regs = sensor_ev_pos1_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos1_regs),
	 },
	{
	 .regs = sensor_ev_pos2_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos2_regs),
	 },
	{
	 .regs = sensor_ev_pos3_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos3_regs),
	 },
	{
	 .regs = sensor_ev_pos4_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos4_regs),
	 },
};


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
};

static struct regval_list sensor_fmt_raw[] = {

};


static int bf3903_sensor_read(struct v4l2_subdev *sd, unsigned char reg, unsigned char *value)
{
	int ret = 0;
	int cnt = 0;
	ret = cci_read_a8_d8(sd, reg, value);
	while (ret != 0 && cnt < 2) {
		ret = cci_read_a8_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		vfe_dev_dbg("sensor read retry=%d\n", cnt);

	return ret;
}

static int bf3903_sensor_write(struct v4l2_subdev *sd, unsigned char reg,
			       unsigned char value)
{
	int ret = 0;
	int cnt = 0;

	ret = cci_write_a8_d8(sd, reg, value);
	while (ret != 0 && cnt < 2) {
		ret = cci_write_a8_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		vfe_dev_dbg("sensor write retry=%d\n", cnt);

	return ret;
}

static int bf3903_sensor_write_array(struct v4l2_subdev *sd,
				     struct regval_list *regs, int array_size)
{
	int i = 0;

	if (!regs)
		return 0;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			msleep(regs->data);
		} else {
			LOG_ERR_RET(bf3903_sensor_write
				    (sd, regs->addr, regs->data))
		}
		i++;
		regs++;
	}
	return 0;
}

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	usleep_range(10000, 12000);

	info->hflip = value;
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
			    enum v4l2_exposure_auto_type value)
{
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}



static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->brightness == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	LOG_ERR_RET(bf3903_sensor_write_array
		    (sd, sensor_brightness[value + 4].regs,
		     sensor_brightness[value + 4].size))

	    info->brightness = value;
	return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->contrast == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	LOG_ERR_RET(bf3903_sensor_write_array
		    (sd, sensor_contrast[value + 4].regs,
		     sensor_contrast[value + 4].size))

	    info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->saturation == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	LOG_ERR_RET(bf3903_sensor_write_array
		    (sd, sensor_saturation[value + 4].regs,
		     sensor_saturation[value + 4].size))

	    info->saturation = value;
	return 0;
}

static int sensor_g_exp_bias(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp_bias;
	return 0;
}

static int sensor_s_exp_bias(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->exp_bias == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	LOG_ERR_RET(bf3903_sensor_write_array
		    (sd, sensor_ev[value + 4].regs, sensor_ev[value + 4].size))

	    info->exp_bias = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_auto_n_preset_white_balance *wb_type =
	    (enum v4l2_auto_n_preset_white_balance *)value;

	*wb_type = info->wb;

	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
		       enum v4l2_auto_n_preset_white_balance value)
{
	struct sensor_info *info = to_state(sd);

	if (info->capture_mode == V4L2_MODE_IMAGE)
		return 0;

	if (info->wb == value)
		return 0;

	LOG_ERR_RET(bf3903_sensor_write_array
		    (sd, sensor_wb[value].regs, sensor_wb[value].size))

	    if (value == V4L2_WHITE_BALANCE_AUTO)
		info->autowb = 1;
	else
		info->autowb = 0;

	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx *)value;

	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd, enum v4l2_colorfx value)
{
	struct sensor_info *info = to_state(sd);

	if (info->clrfx == value)
		return 0;

	LOG_ERR_RET(bf3903_sensor_write_array
		    (sd, sensor_colorfx[value].regs,
		     sensor_colorfx[value].size))

	    info->clrfx = value;
	return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_flash_led_mode *flash_mode =
	    (enum v4l2_flash_led_mode *)value;

	*flash_mode = info->flash_mode;
	return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
			       enum v4l2_flash_led_mode value)
{
	struct sensor_info *info = to_state(sd);

	info->flash_mode = value;
	return 0;
}



static int sensor_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	i2c_lock_adapter(client->adapter);

	switch (on) {
	case CSI_SUBDEV_STBY_ON:
		vfe_dev_dbg("CSI_SUBDEV_STBY_ON\n");
		vfe_gpio_write(sd, PWDN, CSI_STBY_ON);
		usleep_range(30000, 31000);
		vfe_set_mclk(sd, OFF);
		break;
	case CSI_SUBDEV_STBY_OFF:
		vfe_dev_dbg("CSI_SUBDEV_STBY_OFF\n");
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(30000, 31000);
		vfe_gpio_write(sd, PWDN, CSI_STBY_OFF);
		usleep_range(10000, 12000);
		break;
	case CSI_SUBDEV_PWR_ON:
		vfe_dev_dbg("CSI_SUBDEV_PWR_ON\n");
		vfe_gpio_set_status(sd, PWDN, 1);
		vfe_gpio_set_status(sd, RESET, 1);
		vfe_gpio_set_status(sd, POWER_EN, 1);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		vfe_gpio_write(sd, POWER_EN, CSI_PWR_ON);
		usleep_range(10000, 12000);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		usleep_range(30000, 31000);
		vfe_gpio_write(sd, RESET, CSI_RST_OFF);
		usleep_range(30000, 31000);
		break;
	case CSI_SUBDEV_PWR_OFF:
		vfe_dev_dbg("CSI_SUBDEV_PWR_OFF\n");
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		usleep_range(10000, 12000);
		vfe_set_mclk(sd, OFF);
		vfe_gpio_write(sd, POWER_EN, CSI_PWR_OFF);
		usleep_range(10000, 12000);
		vfe_gpio_set_status(sd, RESET, 0);
		vfe_gpio_set_status(sd, PWDN, 0);
		break;
	default:
		return -EINVAL;
	}

	i2c_unlock_adapter(client->adapter);
	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vfe_gpio_write(sd, RESET, CSI_RST_OFF);
		usleep_range(10000, 12000);
		break;
	case 1:
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

void bf3903_power_on(struct v4l2_subdev *sd)
{
	vfe_dev_dbg("CSI_SUBDEV_PWR_ON\n");
	vfe_gpio_set_status(sd, PWDN, 1);
	vfe_gpio_set_status(sd, RESET, 1);
	vfe_gpio_set_status(sd, POWER_EN, 1);
	usleep_range(10000, 12000);
	vfe_gpio_write(sd, RESET, CSI_RST_ON);
	vfe_gpio_write(sd, POWER_EN, CSI_PWR_ON);
	usleep_range(10000, 12000);
	vfe_set_mclk_freq(sd, MCLK);
	vfe_set_mclk(sd, ON);
	usleep_range(10000, 12000);
	vfe_gpio_write(sd, RESET, CSI_RST_ON);
	usleep_range(30000, 31000);
	vfe_gpio_write(sd, RESET, CSI_RST_OFF);
	usleep_range(30000, 31000);
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret;
	unsigned char val;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	client->addr = (I2C_ADDR >> 1);

	bf3903_power_on(sd);
	ret = bf3903_sensor_read(sd, 0xfc, &val);
	if (ret < 0) {
		vfe_dev_err("bf3903_sensor_read err at sensor_detect!\n");
		return ret;
	}

	if (val != 0x38)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	vfe_dev_dbg("sensor_init\n");


	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target chip.\n");
		return ret;
	}
	bf3903_sensor_write_array(sd, sensor_default_regs_24M,
				  ARRAY_SIZE(sensor_default_regs_24M));
	return bf3903_sensor_write_array(sd, sensor_default_regs,
					 ARRAY_SIZE(sensor_default_regs));
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	return ret;
}

static struct sensor_format_struct {
	__u8 *desc;
	enum v4l2_mbus_pixelcode mbus_code;
	struct regval_list *regs;
	int regs_size;
	int bpp;
} sensor_formats[] = {
	{
		.desc = "YUYV 4:2:2", .mbus_code = V4L2_MBUS_FMT_YUYV8_2X8,
	.regs = sensor_fmt_yuv422_yuyv, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_yuyv), .bpp = 2,}, {
		.desc = "YVYU 4:2:2", .mbus_code = V4L2_MBUS_FMT_YVYU8_2X8,
	.regs = sensor_fmt_yuv422_yvyu, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_yvyu), .bpp = 2,}, {
		.desc = "UYVY 4:2:2", .mbus_code = V4L2_MBUS_FMT_UYVY8_2X8,
	.regs = sensor_fmt_yuv422_uyvy, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_uyvy), .bpp = 2,}, {
		.desc = "VYUY 4:2:2", .mbus_code = V4L2_MBUS_FMT_VYUY8_2X8,
	.regs = sensor_fmt_yuv422_vyuy, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_vyuy), .bpp = 2,}, {
		.desc = "Raw RGB Bayer", .mbus_code = V4L2_MBUS_FMT_SBGGR8_1X8,
.regs = sensor_fmt_raw, .regs_size =
		    ARRAY_SIZE(sensor_fmt_raw), .bpp = 1},};
#define N_FMTS ARRAY_SIZE(sensor_formats)


static struct sensor_win_size sensor_win_sizes[] = {

	{
	 .width = VGA_WIDTH,
	 .height = VGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = NULL,
	 .regs_size = 0,
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_FMTS)
		return -EINVAL;

	*code = sensor_formats[index].mbus_code;
	return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd,
			    struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index > N_WIN_SIZES - 1)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = sensor_win_sizes[fsize->index].width;
	fsize->discrete.height = sensor_win_sizes[fsize->index].height;

	return 0;
}

static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
				   struct v4l2_mbus_framefmt *fmt,
				   struct sensor_format_struct **ret_fmt,
				   struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;


	fmt->field = V4L2_FIELD_NONE;

	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	fmt->width = wsize->width;
	fmt->height = wsize->height;

	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

static int sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);
	vfe_dev_dbg("sensor_s_fmt\n");
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;

	bf3903_sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	ret = 0;
	if (wsize->regs) {
		ret =
		    bf3903_sensor_write_array(sd, wsize->regs,
					      wsize->regs_size);
		if (ret < 0)
			return ret;
	}

	if (wsize->set_size) {
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);
	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;
	cp->timeperframe.numerator = info->tpf.numerator;
	cp->timeperframe.denominator = info->tpf.denominator;
	printk("[King]----------------- info->tpf.denominator=%d\n",
	       info->tpf.denominator);

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{



	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;
	int fps = 0;
	printk("sensor_s_parm\n");
	printk("((([%d])))\n", tpf->denominator);
	fps = tpf->denominator;

	return 0;
}



static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{




	switch (qc->id) {
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 15, 1, 0);
	case V4L2_CID_FLASH_LED_MODE:
		return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_g_exp_bias(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_g_autoexp(sd, &ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd, &ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_g_flash_mode(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (qc.type == V4L2_CTRL_TYPE_MENU ||
	    qc.type == V4L2_CTRL_TYPE_INTEGER ||
	    qc.type == V4L2_CTRL_TYPE_BOOLEAN) {
		if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
			return -ERANGE;
		}
	}
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_s_exp_bias(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexp(sd,
					(enum v4l2_exposure_auto_type)ctrl->
					value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd,
				   (enum v4l2_auto_n_preset_white_balance)ctrl->
				   value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd, (enum v4l2_colorfx)ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_s_flash_mode(sd,
					   (enum v4l2_flash_led_mode)ctrl->
					   value);
	}
	return -EINVAL;
}

static int sensor_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}



static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.enum_mbus_fmt = sensor_enum_fmt,
	.enum_framesizes = sensor_enum_size,
	.try_mbus_fmt = sensor_try_fmt,
	.s_mbus_fmt = sensor_s_fmt,
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
};



static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	printk("==============sensor_probe  bf3903 ================");
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &sensor_ops);
	client->addr = 0xDC >> 1;
	info->fmt = &sensor_formats[0];
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp = 0;
	info->autoexp = 0;
	info->autowb = 1;
	info->wb = 0;
	info->clrfx = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{"bf3903", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "bf3903",
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	return i2c_add_driver(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	i2c_del_driver(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
