/*
 * NT99142 sensor driver
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
#include <linux/miscdevice.h>

#include "camera.h"
#include "sensor_helper.h"

#define SYS_FS_RW
#define FIXED_FRAME_RATE

MODULE_AUTHOR("raymonxiu");
MODULE_DESCRIPTION("A low-level driver for NT99142 sensors");
MODULE_LICENSE("GPL");

static struct v4l2_subdev *nt99142_sd;

#define DEV_DBG_EN   		0
#if (DEV_DBG_EN == 1)
#define vfe_dev_dbg(x, arg...) printk("[CSI_DEBUG][NT99142]"x, ##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...) printk("[CSI_ERR][NT99142]"x, ##arg)
#define vfe_dev_print(x, arg...) printk("[CSI][NT99142]"x, ##arg)

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
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x1410

#define CSI_STBY_ON			1
#define CSI_STBY_OFF 		0
#define CSI_RST_ON			0
#define CSI_RST_OFF			1
#define CSI_PWR_ON			1
#define CSI_PWR_OFF			0

#define regval_list reg_list_a16_d8
#define REG_TERM 0xff
#define VAL_TERM 0xff
#define REG_DLY  0xffff

#define NT99142_0X3022 00

#if 0
#define NT99142_REG_0X32BC	0x40
#endif
#if 1
#define NT99142_REG_0X32BC	0x3C
#endif
#if 0
#define NT99142_REG_0X32BC	0x38
#endif
#if	0
#define NT99142_REG_0X32BC	0x34
#endif
#if 0
#define NT99142_REG_0X32BC	0x30

#endif
#define SENSOR_FRAME_RATE 15

#define I2C_ADDR 0x54
#define SENSOR_NAME "nt99142"

struct sensor_format_struct;

struct cfg_array {
	struct regval_list *regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

#define NTK_FIX_FPS

static struct regval_list sensor_default_regs[] = {
	{0x3069, 0x01},
	{0x306A, 0x02},
	{0x3100, 0x03},
	{0x3101, 0x00},
	{0x3102, 0x0A},
	{0x3103, 0x00},
	{0x3105, 0x03},
	{0x3106, 0x04},
	{0x3107, 0x10},
	{0x3108, 0x00},
	{0x3109, 0x02},
	{0x307D, 0x01},
	{0x310A, 0x05},
	{0x310C, 0x00},
	{0x310D, 0x80},
	{0x3110, 0x33},
	{0x3111, 0x59},
	{0x3112, 0x44},
	{0x3113, 0x66},
	{0x3114, 0x66},
	{0x311D, 0x40},
	{0x3127, 0x01},
	{0x3129, 0x44},
	{0x3136, 0x59},
	{0x313F, 0x02},
	{0x30A0, 0x03},
	{0x30A1, 0x23},
	{0x30A2, 0x70},
	{0x30A3, 0x01},
	{0x303E, 0x00},
	{0x303F, 0x32},
	{0x3051, 0x3A},
	{0x3052, 0x0F},
	{0x3055, 0x00},
	{0x3056, 0x28},
	{0x305F, 0x33},
	{0x308B, 0x26},
	{0x308C, 0x20},
	{0x308D, 0x38},
	{0x308E, 0x3A},
	{0x308F, 0x0D},
	{0x324F, 0x00},
	{0x3210, 0x10},
	{0x3211, 0x10},
	{0x3212, 0x10},
	{0x3213, 0x10},
	{0x3214, 0x0C},
	{0x3215, 0x0C},
	{0x3216, 0x0C},
	{0x3217, 0x0C},
	{0x3218, 0x0C},
	{0x3219, 0x0C},
	{0x321A, 0x0C},
	{0x321B, 0x0C},
	{0x321C, 0x0B},
	{0x321D, 0x0B},
	{0x321E, 0x0B},
	{0x321F, 0x0B},
	{0x3230, 0x03},
	{0x3231, 0x00},
	{0x3232, 0x00},
	{0x3233, 0x03},
	{0x3234, 0x00},
	{0x3235, 0x00},
	{0x3236, 0x00},
	{0x3237, 0x00},
	{0x3238, 0x18},
	{0x3239, 0x18},
	{0x323A, 0x18},
	{0x3241, 0x44},
	{0x3243, 0xC3},
	{0x3244, 0x00},
	{0x3245, 0x00},
	{0x324F, 0x00},
	{0x3250, 0x30},
	{0x3251, 0x24},
	{0x3252, 0x30},
	{0x3253, 0x24},
	{0x3254, 0x3D},
	{0x3255, 0x28},
	{0x3256, 0x26},
	{0x3257, 0x19},
	{0x3258, 0x51},
	{0x3259, 0x3D},
	{0x325A, 0x21},
	{0x325B, 0x14},
	{0x325C, 0xFF},
	{0x3262, 0x00},
	{0x3268, 0x01},
	{0x3270, 0x00},
	{0x3271, 0x0C},
	{0x3272, 0x18},
	{0x3273, 0x32},
	{0x3274, 0x44},
	{0x3275, 0x54},
	{0x3276, 0x70},
	{0x3277, 0x88},
	{0x3278, 0x9D},
	{0x3279, 0xB0},
	{0x327A, 0xCF},
	{0x327B, 0xE2},
	{0x327C, 0xEF},
	{0x327D, 0xF7},
	{0x327E, 0xFF},
	{0x3290, 0x77},
	{0x3292, 0x73},
	{0x3297, 0x03},
	{0x32B0, 0x46},
	{0x32B1, 0xBB},
	{0x32B2, 0x1E},
	{0x32B3, 0xA0},
	{0x32B4, 0x20},
	{0x32B8, 0x06},
	{0x32B9, 0x06},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32CB, 0x14},
	{0x32CC, 0x70},
	{0x32CD, 0xA0},
	{0x32F1, 0x05},
	{0x32F2, 0x80},
	{0x32FC, 0x00},
	{0x3302, 0x00},
	{0x3303, 0x50},
	{0x3304, 0x00},
	{0x3305, 0xD0},
	{0x3306, 0x07},
	{0x3307, 0xDF},
	{0x3308, 0x07},
	{0x3309, 0xC0},
	{0x330A, 0x06},
	{0x330B, 0x7B},
	{0x330C, 0x01},
	{0x330D, 0xC5},
	{0x330E, 0x01},
	{0x330F, 0x48},
	{0x3310, 0x06},
	{0x3311, 0xE5},
	{0x3312, 0x07},
	{0x3313, 0xD4},
	{0x3326, 0x06},
	{0x3327, 0x03},
	{0x3332, 0x80},
	{0x3360, 0x10},
	{0x3361, 0x1F},
	{0x3362, 0x26},
	{0x3363, 0x31},
	{0x3364, 0x0B},
	{0x3365, 0x04},
	{0x3366, 0x08},
	{0x3367, 0x0C},
	{0x3368, 0x40},
	{0x3369, 0x30},
	{0x336B, 0x20},
	{0x336D, 0x20},
	{0x336E, 0x18},
	{0x3370, 0x10},
	{0x3371, 0x20},
	{0x3372, 0x28},
	{0x3374, 0x30},
	{0x3375, 0x18},
	{0x3376, 0x14},
	{0x3378, 0x10},
	{0x3379, 0x02},
	{0x337A, 0x03},
	{0x337C, 0x04},
	{0x33A0, 0x50},
	{0x33A1, 0x78},
	{0x33A2, 0x10},
	{0x33A3, 0x20},
	{0x33A4, 0x00},
	{0x33A5, 0x74},
	{0x33A7, 0x04},
	{0x33A8, 0x00},
	{0x33A9, 0x02},
	{0x33AA, 0x03},
	{0x33AC, 0x04},
	{0x33AD, 0x03},
	{0x33AE, 0x04},
	{0x33B0, 0x05},
	{0x33B1, 0x00},
	{0x33B4, 0x48},
	{0x33B5, 0x44},
	{0x33B6, 0xA0},
	{0x33B9, 0x03},
	{0x33BD, 0x00},
	{0x33BE, 0x08},
	{0x33BF, 0x10},
	{0x33C0, 0x01},
	{0x3700, 0x08},
	{0x3701, 0x12},
	{0x3702, 0x1D},
	{0x3703, 0x32},
	{0x3704, 0x49},
	{0x3705, 0x61},
	{0x3706, 0x8D},
	{0x3707, 0xB0},
	{0x3708, 0xC9},
	{0x3709, 0xDB},
	{0x370A, 0xF0},
	{0x370B, 0xFB},
	{0x370C, 0xFF},
	{0x370D, 0xFF},
	{0x370E, 0xFF},
	{0x3710, 0x07},
	{0x371E, 0x02},
	{0x371F, 0x02},
	{0x3800, 0x00},
	{0x3813, 0x07},
	{0x3028, 0x02},
};

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE3},
	{0x32D5, 0x7C},
	{0x32D6, 0x00},
	{0x32D7, 0xBD},
	{0x32D8, 0x77},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x17},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},
};
#else

static struct regval_list sensor_720P_regs[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x70},
	{0x32C1, 0x70},
	{0x32C2, 0x70},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE3},
	{0x32D5, 0x7C},
	{0x32D6, 0x00},
	{0x32D7, 0xBD},
	{0x32D8, 0x77},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x17},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},
};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_15fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x6A},
	{0x32C1, 0x6A},
	{0x32C2, 0x6A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0x71},
	{0x32D5, 0x6C},
	{0x32D6, 0x00},
	{0x32D7, 0x5F},
	{0x32D8, 0x67},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x0F},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x08},
	{0x300B, 0x44},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},

};
#else

static struct regval_list sensor_720P_regs_15fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x6A},
	{0x32C1, 0x6A},
	{0x32C2, 0x6A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0x71},
	{0x32D5, 0x6C},
	{0x32D6, 0x00},
	{0x32D7, 0x5F},
	{0x32D8, 0x67},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x0F},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x08},
	{0x300B, 0x44},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},
};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_20fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x64},
	{0x32C1, 0x64},
	{0x32C2, 0x64},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0x97},
	{0x32D5, 0x72},
	{0x32D6, 0x00},
	{0x32D7, 0x7E},
	{0x32D8, 0x6F},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x0F},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},

};
#else

static struct regval_list sensor_720P_regs_20fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x64},
	{0x32C1, 0x64},
	{0x32C2, 0x64},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0x97},
	{0x32D5, 0x72},
	{0x32D6, 0x00},
	{0x32D7, 0x7E},
	{0x32D8, 0x6F},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x0F},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},
};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_25fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x60},
	{0x32C1, 0x60},
	{0x32C2, 0x60},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xBD},
	{0x32D5, 0x77},
	{0x32D6, 0x00},
	{0x32D7, 0x9E},
	{0x32D8, 0x73},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x13},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},

};
#else

static struct regval_list sensor_720P_regs_25fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x60},
	{0x32C1, 0x60},
	{0x32C2, 0x60},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x28},
	{0x32D3, 0x00},
	{0x32D4, 0xBD},
	{0x32D5, 0x77},
	{0x32D6, 0x00},
	{0x32D7, 0x9E},
	{0x32D8, 0x73},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x13},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},

};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_30fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE3},
	{0x32D5, 0x7C},
	{0x32D6, 0x00},
	{0x32D7, 0xBD},
	{0x32D8, 0x77},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x17},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},

};
#else

static struct regval_list sensor_720P_regs_30fps[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE3},
	{0x32D5, 0x7C},
	{0x32D6, 0x00},
	{0x32D7, 0xBD},
	{0x32D8, 0x77},
	{0x32F0, 0x00},
	{0x3200, 0x7E},
	{0x3201, 0x3D},
	{0x302A, 0x80},
	{0x302C, 0x17},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x00},
	{0x3021, 0x02},
	{0x3060, 0x01},
};
#endif

static struct regval_list reg_640x480_30Fps_74Mhz_MCLK24Mhz[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE3},
	{0x32D5, 0x7C},
	{0x32D6, 0x00},
	{0x32D7, 0xBD},
	{0x32D8, 0x77},
	{0x32E0, 0x02},
	{0x32E1, 0x80},
	{0x32E2, 0x01},
	{0x32E3, 0xE0},
	{0x32E4, 0x00},
	{0x32E5, 0x80},
	{0x32E6, 0x00},
	{0x32E7, 0x80},
	{0x32F0, 0x10},
	{0x3200, 0x7E},
	{0x3201, 0x7D},
	{0x302A, 0x80},
	{0x302C, 0x17},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x38},
	{0x3021, 0x02},
	{0x3060, 0x01},

};

static struct regval_list sensor_wb_manual[] = {
};

static struct regval_list sensor_wb_auto_regs[] = {
	{0x3206, 0x03},
	{0x3060, 0x04},
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	{0x3206, 0x02},
	{0x3290, 0x4C},
	{0x3292, 0x72},
	{0x3060, 0x04},
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	{0x3206, 0x02},
	{0x3290, 0x5C},
	{0x3292, 0x7F},
	{0x3060, 0x04},
};

static struct regval_list sensor_wb_tungsten_regs[] = {

	{0x3206, 0x02},
	{0x3290, 0x40},
	{0x3292, 0x8C},
	{0x3060, 0x04},
};

static struct regval_list sensor_wb_horizon[] = {
};

static struct regval_list sensor_wb_daylight_regs[] = {
	{0x3206, 0x02},
	{0x3290, 0x4E},
	{0x3292, 0x5A},
	{0x3060, 0x04},
};

static struct regval_list sensor_wb_flash[] = {
};

static struct regval_list sensor_wb_cloud_regs[] = {

	{0x3206, 0x02},
	{0x3290, 0x45},
	{0x3292, 0x40},
	{0x3060, 0x04},
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
	{0x32f1, 0x00},
	{0x3060, 0x01},
};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{0x32f1, 0x01},
	{0x3060, 0x01},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{0x32f1, 0x02},
	{0x3060, 0x01},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{0x32f1, 0x03},
	{0x3060, 0x01},
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{0x32f1, 0x05},
	{0x32f4, 0xF0},
	{0x32f5, 0x80},
	{0x3060, 0x04},
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{0x32f1, 0x05},
	{0x32f4, 0x60},
	{0x32f5, 0x20},
	{0x3060, 0x04},
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
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
	 .size = ARRAY_SIZE(sensor_colorfx_negative_regs),
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
};

static struct regval_list sensor_qvga_regs[] = {
	{0x32BB, 0x67},
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE9},
	{0x32D5, 0x7D},
	{0x32D6, 0x00},
	{0x32D7, 0xC1},
	{0x32D8, 0x78},
	{0x32E0, 0x01},
	{0x32E1, 0x40},
	{0x32E2, 0x00},
	{0x32E3, 0xF0},
	{0x32E4, 0x02},
	{0x32E5, 0x02},
	{0x32E6, 0x02},
	{0x32E7, 0x03},
	{0x32F0, 0x10},
	{0x3200, 0x7E},
	{0x3201, 0x7D},
	{0x302A, 0x80},
	{0x302C, 0x24},
	{0x302D, 0x21},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x03},
	{0x300D, 0x09},
	{0x320A, 0x38},
	{0x3021, 0x02},
	{0x3060, 0x01},

};


static struct regval_list sensor_xga_regs[] = {

};

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_vga_regs[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x5A},
	{0x32C1, 0x5A},
	{0x32C2, 0x5A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE3},
	{0x32D5, 0x7C},
	{0x32D6, 0x00},
	{0x32D7, 0xBD},
	{0x32D8, 0x77},
	{0x32E0, 0x02},
	{0x32E1, 0x80},
	{0x32E2, 0x01},
	{0x32E3, 0xE0},
	{0x32E4, 0x00},
	{0x32E5, 0x80},
	{0x32E6, 0x00},
	{0x32E7, 0x80},
	{0x32F0, 0x10},
	{0x3200, 0x7E},
	{0x3201, 0x7D},
	{0x302A, 0x80},
	{0x302C, 0x17},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x38},
	{0x3021, 0x02},
	{0x3060, 0x01},

};
#else

static struct regval_list sensor_vga_regs[] = {
	{0x32BB, 0x67},
	{0x32B8, 0x08},
	{0x32B9, 0x08},
	{0x32BC, 0x40},
	{0x32BD, 0x04},
	{0x32BE, 0x04},
	{0x32BF, 0x60},
	{0x32C0, 0x6A},
	{0x32C1, 0x6A},
	{0x32C2, 0x6A},
	{0x32C3, 0x00},
	{0x32C4, 0x30},
	{0x32C5, 0x30},
	{0x32C6, 0x30},
	{0x32D3, 0x00},
	{0x32D4, 0xE3},
	{0x32D5, 0x7C},
	{0x32D6, 0x00},
	{0x32D7, 0xBD},
	{0x32D8, 0x77},
	{0x32E0, 0x02},
	{0x32E1, 0x80},
	{0x32E2, 0x01},
	{0x32E3, 0xE0},
	{0x32E4, 0x00},
	{0x32E5, 0x80},
	{0x32E6, 0x00},
	{0x32E7, 0x80},
	{0x32F0, 0x10},
	{0x3200, 0x7E},
	{0x3201, 0x7D},
	{0x302A, 0x80},
	{0x302C, 0x17},
	{0x302D, 0x11},
	{0x3022, 0x03},
	{0x300A, 0x06},
	{0x300B, 0x32},
	{0x300C, 0x02},
	{0x300D, 0xF4},
	{0x320A, 0x38},
	{0x3021, 0x02},
	{0x3060, 0x01},

};
#endif

static struct regval_list sensor_uxga_regs[] = {
};

static struct regval_list sensor_svga_regs[] = {
};

static struct regval_list sensor_1080P_regs[] = {
};

static struct cfg_array sensor_resolution[] = {
	 {
	 .regs = sensor_1080P_regs,
	 .size = ARRAY_SIZE(sensor_1080P_regs),
	 },
	 {
	 .regs = sensor_uxga_regs,
	 .size = ARRAY_SIZE(sensor_720P_regs),
	 },
	 {
	 .regs = sensor_720P_regs,
	 .size = ARRAY_SIZE(sensor_720P_regs),
	 },
	 {
	 .regs = sensor_xga_regs,
	 .size = ARRAY_SIZE(sensor_xga_regs),
	 },
	 {
	 .regs = reg_640x480_30Fps_74Mhz_MCLK24Mhz,
	 .size = ARRAY_SIZE(reg_640x480_30Fps_74Mhz_MCLK24Mhz),
	 },
	 {
	 .regs = sensor_qvga_regs,
	 .size = ARRAY_SIZE(sensor_qvga_regs),
	 },
	  {
	 .regs = sensor_svga_regs,
	 .size = ARRAY_SIZE(sensor_svga_regs),
	 },


};

static struct regval_list sensor_brightness_neg4_regs[] = {
	{0x32fc, 0x80},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_neg3_regs[] = {
	{0x32FC, 0xc4},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_neg2_regs[] = {
	{0x32FC, 0xCe},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_neg1_regs[] = {
	{0x32FC, 0xD8},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_zero_regs[] = {
	{0x32FC, 0x00},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_pos1_regs[] = {
	{0x32FC, 0xEC},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_pos2_regs[] = {
	{0x32FC, 0xF6},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_pos3_regs[] = {
	{0x32FC, 0x00},
	{0x3060, 0x01},
};

static struct regval_list sensor_brightness_pos4_regs[] = {
	{0x32fc, 0x7f},
	{0x3060, 0x01},
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

static struct regval_list sensor_saturation_neg4_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0x40},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_neg3_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0x50},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_neg2_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0x60},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_neg1_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0x70},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_zero_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0x80},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_pos1_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0x90},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_pos2_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0xA0},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_pos3_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0xB0},
	{0x3060, 0x04},
};

static struct regval_list sensor_saturation_pos4_regs[] = {
	{0x32f1, 0x05},
	{0x32f3, 0xC0},
	{0x3060, 0x04},
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
	{0x32f2, 0x40},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0x32f2, 0x50},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0x32f2, 0x60},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{0x32f2, 0x70},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_zero_regs[] = {
	{0x32f2, 0x80},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0x32f2, 0x90},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0x32f2, 0xa0},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0x32f2, 0xb0},
	{0x3060, 0x01},
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0x32f2, 0xc0},
	{0x3060, 0x01},
};

static struct regval_list reg_band_50hz_reg[] = {
	{0x32BB, 0x67},

};

static struct regval_list reg_band_60hz_reg[] = {
	{0x32BB, 0x77},

};

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
	{0x32f0, 0x01},
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0x32f0, 0x03},
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0x32f0, 0x02},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0x32f0, 0x00},
};

static struct regval_list sensor_fmt_raw[] = {
	{0x32f0, 0x50},
};


static int sensor_read_nt99142(struct v4l2_subdev *sd, unsigned short reg,
			       unsigned char *value)
{
	int ret = 0;
	int cnt = 0;

	ret = cci_read_a16_d8(sd, reg, value);
	while (ret != 0 && cnt < 2) {
		ret = cci_read_a16_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		vfe_dev_dbg("sensor read retry=%d\n", cnt);

	return ret;
}

static int sensor_write_nt99142(struct v4l2_subdev *sd, unsigned short reg,
				unsigned char value)
{
	int ret = 0;
	int cnt = 0;
	ret = cci_write_a16_d8(sd, reg, value);
	while (ret != 0 && cnt < 2) {
		ret = cci_write_a16_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		vfe_dev_dbg("sensor write retry=%d\n", cnt);
	return ret;
}

static int sensor_write_nt99142_array(struct v4l2_subdev *sd,
				      struct regval_list *regs, int array_size)
{
	int i = 0;

	if (!regs)
		return 0;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			msleep(regs->data);
		} else {
			LOG_ERR_RET(sensor_write_nt99142
				    (sd, regs->addr, regs->data))
		}
		i++;
		regs++;
	}
	return 0;
}

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_nt99142(sd, 0x3022, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_g_hflip!\n");
		return ret;
	}
	vfe_dev_dbg("NT99142 sensor_read_nt99142 sensor_g_hflip(%x)\n", val);

	val &= (1 << 1);
	val = val >> 0;

	*value = val;

	info->hflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;
	ret = sensor_read_nt99142(sd, 0x3022, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_s_hflip!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val &= 0xfd;
		break;
	case 1:
		val |= 0x02;
		break;
	default:
		return -EINVAL;
	}

	ret = sensor_write_nt99142(sd, 0x3022, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_nt99142 err at sensor_s_hflip!\n");
		return ret;
	}

	mdelay(20);

	info->hflip = value;
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_nt99142(sd, 0x3022, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_g_vflip!\n");
		return ret;
	}
	vfe_dev_dbg("NT99142 sensor_read_nt99142 sensor_g_vflip(%x)\n", val);

	val &= 0x01;

	*value = val;

	info->vflip = *value;
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;
	ret = sensor_read_nt99142(sd, 0x3022, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_s_vflip!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val &= 0xfe;
		break;
	case 1:
		val |= 0x01;
		break;
	default:
		return -EINVAL;
	}
	ret = sensor_write_nt99142(sd, 0x3022, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_nt99142 err at sensor_s_vflip!\n");
		return ret;
	}

	mdelay(20);

	info->vflip = value;
	return 0;
}

static int sensor_s_flip(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_nt99142(sd, 0x3022, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_s_vflip!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val |= 0x01;
		break;
	case 1:
		val &= 0xfe;
		break;
	case 2:
		val |= 0x02;
		break;
	case 3:
		val &= 0xfd;
		break;
	case 4:
		val |= 0x03;
		break;
	case 5:
		val |= 0xfc;
		break;
	default:
		return -EINVAL;
	}
	ret = sensor_write_nt99142(sd, 0x3022, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_nt99142 err at sensor_s_vflip!\n");
		return ret;
	}

	mdelay(20);
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
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_nt99142(sd, 0x3201, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_g_autoexp!\n");
		return ret;
	}

	val = ((val & 0x20) >> 5);
	if (val == 0x01) {
		*value = V4L2_EXPOSURE_AUTO;
	} else {
		*value = V4L2_EXPOSURE_MANUAL;
	}

	info->autoexp = *value;
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
			    enum v4l2_exposure_auto_type value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_nt99142(sd, 0x3201, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_s_autoexp!\n");
		return ret;
	}

	switch (value) {
	case V4L2_EXPOSURE_AUTO:
		val |= 0x20;
		break;
	case V4L2_EXPOSURE_MANUAL:
		val &= 0xdf;
		break;
	case V4L2_EXPOSURE_SHUTTER_PRIORITY:
		return -EINVAL;
	case V4L2_EXPOSURE_APERTURE_PRIORITY:
		return -EINVAL;
	default:
		return -EINVAL;
	}

	ret = sensor_write_nt99142(sd, 0x3201, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_nt99142 err at sensor_s_autoexp!\n");
		return ret;
	}

	mdelay(20);

	info->autoexp = value;
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_nt99142(sd, 0x3201, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_g_autowb!\n");
		return ret;
	}

	val = ((val & 0x10) >> 4);


	*value = val;
	info->autowb = *value;

	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret =
	    sensor_write_nt99142_array(sd, sensor_wb_auto_regs,
				       ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		vfe_dev_err
		    ("sensor_write_nt99142_array err at sensor_s_autowb!\n");
		return ret;
	}

	ret = sensor_read_nt99142(sd, 0x3201, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_s_autowb!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val &= 0xef;
		break;
	case 1:
		val |= 0x10;
		break;
	default:
		break;
	}
	ret = sensor_write_nt99142(sd, 0x3201, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_nt99142 err at sensor_s_autowb!\n");
		return ret;
	}

	mdelay(10);

	info->autowb = value;
	return 0;
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

	LOG_ERR_RET(sensor_write_nt99142_array
		    (sd, sensor_brightness[value + 4].regs,
		     sensor_brightness[value + 4].size))

	    info->brightness = value;
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

	LOG_ERR_RET(sensor_write_nt99142_array
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

	LOG_ERR_RET(sensor_write_nt99142_array
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

	LOG_ERR_RET(sensor_write_nt99142_array
		    (sd, sensor_wb[value].regs, sensor_wb[value].size))

	    if (value == V4L2_WHITE_BALANCE_AUTO)
		info->autowb = 1;
	else
		info->autowb = 0;

	info->wb = value;
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx *)value;
	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_hue(struct v4l2_subdev *sd, enum v4l2_colorfx value)
{
	struct sensor_info *info = to_state(sd);
	if (info->clrfx == value)
		return 0;

	LOG_ERR_RET(sensor_write_nt99142_array
		    (sd, sensor_colorfx[value].regs,
		     sensor_colorfx[value].size))

	    info->clrfx = value;
	return 0;
}

static int sensor_s_resolution(struct v4l2_subdev *sd,
			       enum v4l2_resolution value)
{
	struct sensor_info *info = to_state(sd);
	if (info->resolution == value)
		return 0;
	printk("%s:value = %d\n", __FUNCTION__, value);
	if (value == V4L2_RESOLUTION_1080P) {
		printk("########Can't Support 1080p############\n");
		value = info->resolution;
	}

	LOG_ERR_RET(sensor_write_nt99142_array
		    (sd, sensor_resolution[value].regs,
		     sensor_resolution[value].size))

	    info->resolution = value;
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
	int ret;
	struct csi_dev *dev =
	    (struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct sensor_info *info = to_state(sd);
	ret = 0;
	switch (on) {
	case CSI_SUBDEV_PWR_ON:
		printk("CSI_SUBDEV_PWR_ON\n");

		cci_lock(sd);
		info->streaming = 0;

		vfe_gpio_set_status(sd, RESET, 1);
		vfe_gpio_set_status(sd, POWER_EN, 1);
		mdelay(10);
		vfe_gpio_write(sd, POWER_EN, 1);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		mdelay(10);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		mdelay(10);
		vfe_gpio_write(sd, RESET, CSI_RST_OFF);
		mdelay(30);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		mdelay(30);
		vfe_gpio_write(sd, RESET, CSI_RST_OFF);
		mdelay(30);
		cci_unlock(sd);
		break;
	case CSI_SUBDEV_PWR_OFF:
		printk("CSI_SUBDEV_PWR_OFF\n");
		cci_lock(sd);
		mdelay(10);
		vfe_gpio_set_status(sd, POWER_EN, 1);
		vfe_gpio_set_status(sd, RESET, 1);
		mdelay(10);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		vfe_gpio_write(sd, POWER_EN, 0);
		mdelay(10);
		vfe_set_mclk(sd, OFF);
		vfe_gpio_set_status(sd, POWER_EN, 0);
		vfe_gpio_set_status(sd, RESET, 0);
		mdelay(10);
		cancel_delayed_work(&info->work);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vfe_gpio_write(sd, RESET, CSI_RST_OFF);
		mdelay(10);
		break;
	case 1:
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		mdelay(10);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret;
	unsigned char val;
	unsigned int SENSOR_ID = 0;

	ret = sensor_read_nt99142(sd, 0x3000, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_detect!\n");
		return ret;
	}

	SENSOR_ID |= (val << 8);

	ret = sensor_read_nt99142(sd, 0x3001, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_nt99142 err at sensor_detect!\n");
		return ret;
	}

	return 0;
}

static int sensor_s_band(struct v4l2_subdev *sd,
			 enum v4l2_power_line_frequency value)
{

	__u16 *reg_table = NULL;
	__u32 reg_table_len = 0;
	vfe_set_mclk_freq(sd, MCLK);
	vfe_set_mclk(sd, ON);
	msleep(20);
	switch (value) {
	case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
		{

			sensor_write_nt99142_array(sd, reg_band_50hz_reg,
						   ARRAY_SIZE
						   (reg_band_50hz_reg));

			break;
		}
	case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
		{

			sensor_write_nt99142_array(sd, reg_band_60hz_reg,
						   ARRAY_SIZE
						   (reg_band_60hz_reg));
			break;
		}
	}

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	int i = 0;
	vfe_dev_dbg("sensor_init\n");


	for (i = 0; i < 3; i++) {
		ret = sensor_detect(sd);

		if (ret) {
			vfe_dev_err("chip found is not an target chip.\n");
			return ret;
		} else
			break;
		msleep(100);
	}
	return sensor_write_nt99142_array(sd, sensor_default_regs,
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
		.desc = "UYVY 4:2:2", .mbus_code = V4L2_MBUS_FMT_UYVY8_2X8,
	.regs = sensor_fmt_yuv422_uyvy, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_uyvy), .bpp = 2,}, {
		.desc = "YUYV 4:2:2", .mbus_code = V4L2_MBUS_FMT_YUYV8_2X8,
	.regs = sensor_fmt_yuv422_yuyv, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_yuyv), .bpp = 2,}, {
		.desc = "YVYU 4:2:2", .mbus_code = V4L2_MBUS_FMT_YVYU8_2X8,
	.regs = sensor_fmt_yuv422_yvyu, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_yvyu), .bpp = 2,}, {
		.desc = "VYUY 4:2:2", .mbus_code = V4L2_MBUS_FMT_VYUY8_2X8,
	.regs = sensor_fmt_yuv422_vyuy, .regs_size =
		    ARRAY_SIZE(sensor_fmt_yuv422_vyuy), .bpp = 2,}, {
		.desc = "Raw RGB Bayer", .mbus_code = V4L2_MBUS_FMT_SBGGR8_1X8,
.regs = sensor_fmt_raw, .regs_size =
		    ARRAY_SIZE(sensor_fmt_raw), .bpp = 1},};
#define N_FMTS ARRAY_SIZE(sensor_formats)


static struct sensor_win_size sensor_win_sizes[] = {

	{
	 .width = HD1080_WIDTH,
	 .height = HD1080_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = sensor_1080P_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080P_regs),
	 .set_size = NULL,
	 },

	{
	 .width = HD720_WIDTH,
	 .height = HD720_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = sensor_720P_regs,
	 .regs_size = ARRAY_SIZE(sensor_720P_regs),
	 .set_size = NULL,
	 },

	{
	 .width = XGA_WIDTH,
	 .height = XGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = sensor_xga_regs,
	 .regs_size = ARRAY_SIZE(sensor_xga_regs),
	 .set_size = NULL,
	 },

	{
	 .width = VGA_WIDTH,
	 .height = VGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = sensor_vga_regs,
	 .regs_size = ARRAY_SIZE(sensor_vga_regs),
	 .set_size = NULL,
	 },

	{
	 .width = QVGA_WIDTH,
	 .height = QVGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = sensor_qvga_regs,
	 .regs_size = ARRAY_SIZE(sensor_qvga_regs),
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

	if (index >= N_FMTS) {

		index = 0;
		fmt->code = sensor_formats[0].mbus_code;
	}
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

	sensor_write_nt99142_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	ret = 0;
	if (wsize->regs) {
		ret =
		    sensor_write_nt99142_array(sd, wsize->regs,
					       wsize->regs_size);
		if (ret < 0)
			return ret;
	}

	msleep(250);

	if (wsize->set_size) {
		ret = wsize->set_size(sd);
		if (ret < 0)
			return ret;
	}

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	info->streaming = 1;
	info->night_mode = 0;
	queue_delayed_work(info->wq, &info->work, msecs_to_jiffies(500));

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

	switch (fps) {
	case 30:
		printk("switch fps to 30.\n");
		sensor_win_sizes[1].regs = sensor_720P_regs_30fps;
		sensor_win_sizes[1].regs_size =
		    ARRAY_SIZE(sensor_720P_regs_30fps);
		break;
	case 25:
		printk("switch fps to 25.\n");
		sensor_win_sizes[1].regs = sensor_720P_regs_25fps;
		sensor_win_sizes[1].regs_size =
		    ARRAY_SIZE(sensor_720P_regs_25fps);
		break;
	case 20:
		printk("switch fps to 20.\n");
		sensor_win_sizes[1].regs = sensor_720P_regs_20fps;
		sensor_win_sizes[1].regs_size =
		    ARRAY_SIZE(sensor_720P_regs_20fps);
		break;
	case 15:
		printk("switch fps to 15.\n");
		sensor_win_sizes[1].regs = sensor_720P_regs_15fps;
		sensor_win_sizes[1].regs_size =
		    ARRAY_SIZE(sensor_720P_regs_15fps);
		break;
	default:
		printk("switch fps to nothing.\n");
		break;
	}

	return 0;
}



static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{



	switch (qc->id) {
	case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_CONTRAST:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_SATURATION:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_HUE:
		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
	case V4L2_CID_AUTOGAIN:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return v4l2_ctrl_query_fill(qc, 0, 3, 1, 0);
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

	if (ctrl->id == V4L2_CID_GAIN)
		ctrl->id = V4L2_CID_RESOLUTION;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, (enum v4l2_colorfx)ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_s_exp_bias(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexp(sd,
					(enum v4l2_exposure_auto_type)
					ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd,
				   (enum v4l2_auto_n_preset_white_balance)
				   ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_flip(sd, ctrl->value);
	case V4L2_CID_RESOLUTION:
		return sensor_s_resolution(sd, ctrl->value);
		break;
	case V4L2_CID_POWER_LINE_FREQUENCY:
		printk
		    (" V4L2_CID_POWER_LINE_FREQUENCY 22222222222222222(nt99142)\n");
		return sensor_s_band(sd, ctrl->value);
		break;
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


static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_8,
};



static void auto_scene_worker(struct work_struct *work)
{
	struct sensor_info *info =
	    container_of(work, struct sensor_info, work.work);
	struct v4l2_subdev *sd = &info->sd;
	struct csi_dev *dev =
	    (struct csi_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	struct regval_list regs;
	int ret;
	unsigned char val;
	struct regval_list night_regs[] = {
		{0x32f2, 0x94},
		{0x32fc, 0x06},
		{0x329e, 0x00},
		{0x329f, 0x00},

		{0x3060, 0x01},

	};
	struct regval_list daylight_regs[] = {
		{0x32f2, 0x88},
		{0x32fc, 0x00},
		{0x329e, 0x00},
		{0x329f, 0x00},

		{0x3060, 0x01},

	};

	if (!info->streaming) {
		return;
	}

	ret = sensor_read_nt99142(sd, 0x32C8, &val);
	if (ret < 0) {
		printk("auto_scene_worker error.\n");
		return;
	}
	if ((val > 0x84) && (info->night_mode == 0)) {
		ret =
		    sensor_write_nt99142_array(sd, night_regs,
					       ARRAY_SIZE(night_regs));
		if (ret < 0) {
			printk("auto_scene error, return %x!\n", ret);
			return;
		}
		info->night_mode = 1;
		printk("debug switch to night mode\n");
	} else if ((val < 0x6D) && (info->night_mode == 1)) {
		ret =
		    sensor_write_nt99142_array(sd, daylight_regs,
					       ARRAY_SIZE(daylight_regs));
		if (ret < 0) {
			printk("auto_scene error, return %x!\n", ret);
			return;
		}
		info->night_mode = 0;
		printk("switch to daylight mode\n");
	}
	queue_delayed_work(info->wq, &info->work, msecs_to_jiffies(500));
}

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);

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

	nt99142_sd = sd;

	info->wq = create_singlethread_workqueue("kworkqueue_nt99142");
	if (!info->wq) {
		dev_err(&client->dev, "Could not create workqueue\n");
	}
	flush_workqueue(info->wq);
	INIT_DELAYED_WORK(&info->work, auto_scene_worker);

	info->night_mode = 0;
	info->streaming = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct sensor_info *info;
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
	info = to_state(sd);
	cancel_delayed_work_sync(&info->work);
	destroy_workqueue(info->wq);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static const struct of_device_id sernsor_match[] = {
	{.compatible = "allwinner,sensor_nt99142",},
	{},
};

static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   .of_match_table = sernsor_match,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{

	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
