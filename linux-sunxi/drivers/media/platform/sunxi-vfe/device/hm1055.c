/*
 * HM1055 sensor driver
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
MODULE_DESCRIPTION("A low-level driver for HM1055 sensors");
MODULE_LICENSE("GPL");

static struct v4l2_subdev *hm1055_sd;

#define DEV_DBG_EN   		0
#if (DEV_DBG_EN == 1)
#define vfe_dev_dbg(x, arg...) printk("[CSI_DEBUG][HM1055]"x, ##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...) printk("[CSI_ERR][HM1055]"x, ##arg)
#define vfe_dev_print(x, arg...) printk("[CSI][HM1055]"x, ##arg)

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
#define V4L2_IDENT_SENSOR 0x0955

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

#define HM1055_0X3022 0x28

#if 0
#define HM1055_REG_0X32B8	0x3F
#define HM1055_REG_0X32B9	0x31
#define HM1055_REG_0X32BC	0x38
#define HM1055_REG_0X32BD	0x3C
#define HM1055_REG_0X32BE	0x34
#endif
#if 0
#define HM1055_REG_0X32B8	0x3D
#define HM1055_REG_0X32B9	0x2F
#define HM1055_REG_0X32BC	0x36
#define HM1055_REG_0X32BD	0x3A
#define HM1055_REG_0X32BE	0x32
#endif
#if 1
#define HM1055_REG_0X32B8	0x3B
#define HM1055_REG_0X32B9	0x2D
#define HM1055_REG_0X32BC	0x34
#define HM1055_REG_0X32BD	0x38
#define HM1055_REG_0X32BE	0x30
#endif
#if	0
#define HM1055_REG_0X32B8	0x39
#define HM1055_REG_0X32B9	0x2B
#define HM1055_REG_0X32BC	0x32
#define HM1055_REG_0X32BD	0x36
#define HM1055_REG_0X32BE	0x2E
#endif
#if 0
#define HM1055_REG_0X32B8	0x36
#define HM1055_REG_0X32B9	0x2A
#define HM1055_REG_0X32BC	0x30
#define HM1055_REG_0X32BD	0x33
#define HM1055_REG_0X32BE	0x2D
#endif
#define SENSOR_FRAME_RATE 15

#define I2C_ADDR 0x48
#define SENSOR_NAME "hm1055"

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

	{0x0022, 0x00},
	{0x0026, 0x77},
	{0x002A, 0x44},
	{0x002B, 0x00},
	{0x002C, 0x00},
	{0x0025, 0x00},
	{0x0020, 0x08},
	{0x0027, 0x30},
	{0x0028, 0xC0},
	{0x0004, 0x10},
	{0x0006, 0x00},
	{0x0012, 0x0F},
	{0x0044, 0x04},
	{0x004A, 0x2A},
	{0x004B, 0x72},
	{0x004E, 0x30},
	{0x0070, 0x2A},
	{0x0071, 0x57},
	{0x0072, 0x55},
	{0x0073, 0x30},
	{0x0077, 0x04},
	{0x0080, 0xC2},
	{0x0082, 0xA2},
	{0x0083, 0xF0},
	{0x0085, 0x11},
	{0x0086, 0x22},
	{0x0087, 0x08},
	{0x0088, 0x6e},
	{0x0089, 0x2A},
	{0x008A, 0x2F},
	{0x008D, 0x20},
	{0x008f, 0x77},
	{0x0090, 0x01},
	{0x0091, 0x02},
	{0x0092, 0x03},
	{0x0093, 0x04},
	{0x0094, 0x14},
	{0x0095, 0x09},
	{0x0096, 0x0A},
	{0x0097, 0x0B},
	{0x0098, 0x0C},
	{0x0099, 0x04},
	{0x009A, 0x14},
	{0x009B, 0x34},
	{0x00A0, 0x00},
	{0x00A1, 0x00},
	{0x0B3B, 0x0B},
	{0x0040, 0x0A},
	{0x0053, 0x0A},
	{0x0120, 0x36},
	{0x0121, 0x80},
	{0x0122, 0xEB},
	{0x0123, 0xCC},
	{0x0124, 0xDE},
	{0x0125, 0xDF},
	{0x0126, 0x70},
	{0x0128, 0x1F},
	{0x0129, 0x8F},
	{0x0132, 0xF8},
	{0x011F, 0x08},
	{0x0144, 0x04},
	{0x0145, 0x00},
	{0x0146, 0x20},
	{0x0147, 0x20},
	{0x0148, 0x14},
	{0x0149, 0x14},
	{0x0156, 0x0C},
	{0x0157, 0x0C},
	{0x0158, 0x0A},
	{0x0159, 0x0A},
	{0x015A, 0x03},
	{0x015B, 0x40},
	{0x015C, 0x21},
	{0x015E, 0x0F},
	{0x0168, 0xC8},
	{0x0169, 0xC8},
	{0x016A, 0x96},
	{0x016B, 0x96},
	{0x016C, 0x64},
	{0x016D, 0x64},
	{0x016E, 0x32},
	{0x016F, 0x32},
	{0x01EF, 0xF1},
	{0x0131, 0x44},
	{0x014C, 0x60},
	{0x014D, 0x24},
	{0x015D, 0x90},
	{0x01D8, 0x40},
	{0x01D9, 0x20},
	{0x01DA, 0x23},
	{0x0150, 0x05},
	{0x0155, 0x07},
	{0x0178, 0x10},
	{0x017A, 0x10},
	{0x01BA, 0x10},
	{0x0176, 0x00},
	{0x0179, 0x10},
	{0x017B, 0x10},
	{0x01BB, 0x10},
	{0x0177, 0x00},
	{0x01E7, 0x20},
	{0x01E8, 0x30},
	{0x01E9, 0x50},
	{0x01E4, 0x18},
	{0x01E5, 0x20},
	{0x01E6, 0x04},
	{0x0210, 0x21},
	{0x0211, 0x0A},
	{0x0212, 0x21},
	{0x01DB, 0x04},
	{0x01DC, 0x14},
	{0x0151, 0x08},
	{0x01F2, 0x18},
	{0x01F8, 0x3C},
	{0x01FE, 0x24},
	{0x0213, 0x03},
	{0x0214, 0x03},
	{0x0215, 0x10},
	{0x0216, 0x08},
	{0x0217, 0x05},
	{0x0218, 0xB8},
	{0x0219, 0x01},
	{0x021A, 0xB8},
	{0x021B, 0x01},
	{0x021C, 0xB8},
	{0x021D, 0x01},
	{0x021E, 0xB8},
	{0x021F, 0x01},
	{0x0220, 0xF1},
	{0x0221, 0x5D},
	{0x0222, 0x0A},
	{0x0223, 0x80},
	{0x0224, 0x50},
	{0x0225, 0x09},
	{0x0226, 0x80},
	{0x022A, 0x56},
	{0x022B, 0x13},
	{0x022C, 0x80},
	{0x022D, 0x11},
	{0x022E, 0x08},
	{0x022F, 0x11},
	{0x0230, 0x08},
	{0x0233, 0x11},
	{0x0234, 0x08},
	{0x0235, 0x88},
	{0x0236, 0x02},
	{0x0237, 0x88},
	{0x0238, 0x02},
	{0x023B, 0x88},
	{0x023C, 0x02},
	{0x023D, 0x68},
	{0x023E, 0x01},
	{0x023F, 0x68},
	{0x0240, 0x01},
	{0x0243, 0x68},
	{0x0244, 0x01},
	{0x0251, 0x0F},
	{0x0252, 0x00},
	{0x0260, 0x00},
	{0x0261, 0x4A},
	{0x0262, 0x2C},
	{0x0263, 0x68},
	{0x0264, 0x40},
	{0x0265, 0x2C},
	{0x0266, 0x6A},
	{0x026A, 0x40},
	{0x026B, 0x30},
	{0x026C, 0x66},
	{0x0278, 0x98},
	{0x0279, 0x20},
	{0x027A, 0x80},
	{0x027B, 0x73},
	{0x027C, 0x08},
	{0x027D, 0x80},
	{0x0280, 0x0D},
	{0x0282, 0x1A},
	{0x0284, 0x30},
	{0x0286, 0x53},
	{0x0288, 0x62},
	{0x028a, 0x6E},
	{0x028c, 0x7A},
	{0x028e, 0x83},
	{0x0290, 0x8B},
	{0x0292, 0x92},
	{0x0294, 0x9D},
	{0x0296, 0xA8},
	{0x0298, 0xBC},
	{0x029a, 0xCF},
	{0x029c, 0xE2},
	{0x029e, 0x2A},
	{0x02A0, 0x02},
	{0x02C0, 0x7D},
	{0x02C1, 0x01},
	{0x02C2, 0x7C},
	{0x02C3, 0x04},
	{0x02C4, 0x01},
	{0x02C5, 0x04},
	{0x02C6, 0x3E},
	{0x02C7, 0x04},
	{0x02C8, 0x90},
	{0x02C9, 0x01},
	{0x02CA, 0x52},
	{0x02CB, 0x04},
	{0x02CC, 0x04},
	{0x02CD, 0x04},
	{0x02CE, 0xA9},
	{0x02CF, 0x04},
	{0x02D0, 0xAD},
	{0x02D1, 0x01},
	{0x0302, 0x00},
	{0x0303, 0x00},
	{0x0304, 0x00},
	{0x02e0, 0x04},
	{0x02F0, 0x4E},
	{0x02F1, 0x04},
	{0x02F2, 0xB1},
	{0x02F3, 0x00},
	{0x02F4, 0x63},
	{0x02F5, 0x04},
	{0x02F6, 0x28},
	{0x02F7, 0x04},
	{0x02F8, 0x29},
	{0x02F9, 0x04},
	{0x02FA, 0x51},
	{0x02FB, 0x00},
	{0x02FC, 0x64},
	{0x02FD, 0x04},
	{0x02FE, 0x6B},
	{0x02FF, 0x04},
	{0x0300, 0xCF},
	{0x0301, 0x00},
	{0x0305, 0x08},
	{0x0306, 0x40},
	{0x0307, 0x00},
	{0x032D, 0x70},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},
	{0x0331, 0x70},
	{0x0332, 0x01},
	{0x0333, 0x82},
	{0x0334, 0x82},
	{0x0335, 0x86},
	{0x0340, 0x30},
	{0x0341, 0x44},
	{0x0342, 0x4A},
	{0x0343, 0x3C},
	{0x0344, 0x83},
	{0x0345, 0x4D},
	{0x0346, 0x75},
	{0x0347, 0x56},
	{0x0348, 0x68},
	{0x0349, 0x5E},
	{0x034A, 0x5C},
	{0x034B, 0x65},
	{0x034C, 0x52},
	{0x0350, 0x88},
	{0x0352, 0x18},
	{0x0354, 0x80},
	{0x0355, 0x50},
	{0x0356, 0x88},
	{0x0357, 0xE0},
	{0x0358, 0x00},
	{0x035A, 0x00},
	{0x035B, 0xAC},
	{0x0360, 0x02},
	{0x0361, 0x18},
	{0x0362, 0x50},
	{0x0363, 0x6C},
	{0x0364, 0x00},
	{0x0365, 0xF0},
	{0x0366, 0x08},
	{0x036A, 0x10},
	{0x036B, 0x18},
	{0x036E, 0x10},
	{0x0370, 0x10},
	{0x0371, 0x18},
	{0x0372, 0x0C},
	{0x0373, 0x38},
	{0x0374, 0x3A},
	{0x0375, 0x12},
	{0x0376, 0x20},
	{0x0380, 0xFF},
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038A, 0x80},
	{0x038B, 0x0A},
	{0x038C, 0xC1},
	{0x038E, 0x3C},
	{0x038F, 0x09},
	{0x0390, 0xE0},
	{0x0391, 0x01},
	{0x0392, 0x03},
	{0x0393, 0x80},
	{0x0395, 0x22},
	{0x0398, 0x02},
	{0x0399, 0xF0},
	{0x039A, 0x03},
	{0x039B, 0xAC},
	{0x039C, 0x04},
	{0x039D, 0x68},
	{0x039E, 0x05},
	{0x039F, 0xE0},
	{0x03A0, 0x07},
	{0x03A1, 0x58},
	{0x03A2, 0x08},
	{0x03A3, 0xD0},
	{0x03A4, 0x0B},
	{0x03A5, 0xC0},
	{0x03A6, 0x18},
	{0x03A7, 0x1C},
	{0x03A8, 0x20},
	{0x03A9, 0x24},
	{0x03AA, 0x28},
	{0x03AB, 0x30},
	{0x03AC, 0x24},
	{0x03AD, 0x21},
	{0x03AE, 0x1C},
	{0x03AF, 0x18},
	{0x03B0, 0x17},
	{0x03B1, 0x13},
	{0x03B7, 0x64},
	{0x03B8, 0x00},
	{0x03B9, 0xB4},
	{0x03BA, 0x00},
	{0x03bb, 0xff},
	{0x03bc, 0xff},
	{0x03bd, 0xff},
	{0x03be, 0xff},
	{0x03bf, 0xff},
	{0x03c0, 0xff},
	{0x03c1, 0x01},
	{0x03e0, 0x04},
	{0x03e1, 0x11},
	{0x03e2, 0x01},
	{0x03e3, 0x04},
	{0x03e4, 0x10},
	{0x03e5, 0x21},
	{0x03e6, 0x11},
	{0x03e7, 0x00},
	{0x03e8, 0x11},
	{0x03e9, 0x32},
	{0x03ea, 0x12},
	{0x03eb, 0x01},
	{0x03ec, 0x21},
	{0x03ed, 0x33},
	{0x03ee, 0x23},
	{0x03ef, 0x01},
	{0x03f0, 0x11},
	{0x03f1, 0x32},
	{0x03f2, 0x12},
	{0x03f3, 0x01},
	{0x03f4, 0x10},
	{0x03f5, 0x21},
	{0x03f6, 0x11},
	{0x03f7, 0x00},
	{0x03f8, 0x04},
	{0x03f9, 0x11},
	{0x03fa, 0x01},
	{0x03fb, 0x04},
	{0x03DC, 0x47},
	{0x03DD, 0x5A},
	{0x03DE, 0x41},
	{0x03DF, 0x53},
	{0x0420, 0x82},
	{0x0421, 0x00},
	{0x0422, 0x00},
	{0x0423, 0x88},
	{0x0430, 0x08},
	{0x0431, 0x30},
	{0x0432, 0x0c},
	{0x0433, 0x04},
	{0x0435, 0x08},
	{0x0450, 0xFF},
	{0x0451, 0xD0},
	{0x0452, 0xB8},
	{0x0453, 0x88},
	{0x0454, 0x00},
	{0x0458, 0x80},
	{0x0459, 0x03},
	{0x045A, 0x00},
	{0x045B, 0x50},
	{0x045C, 0x00},
	{0x045D, 0x90},
	{0x0465, 0x02},
	{0x0466, 0x14},
	{0x047A, 0x00},
	{0x047B, 0x00},
	{0x047C, 0x04},
	{0x047D, 0x50},
	{0x047E, 0x04},
	{0x047F, 0x90},
	{0x0480, 0x58},
	{0x0481, 0x06},
	{0x0482, 0x08},
	{0x04B0, 0x50},
	{0x04B6, 0x30},
	{0x04B9, 0x10},
	{0x04B3, 0x00},
	{0x04B1, 0x85},
	{0x04B4, 0x00},
	{0x0540, 0x00},
	{0x0541, 0xBC},
	{0x0542, 0x00},
	{0x0543, 0xE1},
	{0x0580, 0x04},
	{0x0581, 0x0F},
	{0x0582, 0x04},
	{0x05A1, 0x0A},
	{0x05A2, 0x21},
	{0x05A3, 0x84},
	{0x05A4, 0x24},
	{0x05A5, 0xFF},
	{0x05A6, 0x00},
	{0x05A7, 0x24},
	{0x05A8, 0x24},
	{0x05A9, 0x02},
	{0x05B1, 0x24},
	{0x05B2, 0x0C},
	{0x05B4, 0x1F},
	{0x05AE, 0x75},
	{0x05AF, 0x78},
	{0x05B6, 0x00},
	{0x05B7, 0x10},
	{0x05BF, 0x20},
	{0x05C1, 0x06},
	{0x05C2, 0x18},
	{0x05C7, 0x00},
	{0x05CC, 0x04},
	{0x05CD, 0x00},
	{0x05CE, 0x03},
	{0x05E4, 0x08},
	{0x05E5, 0x00},
	{0x05E6, 0x07},
	{0x05E7, 0x05},
	{0x05E8, 0x08},
	{0x05E9, 0x00},
	{0x05EA, 0xD7},
	{0x05EB, 0x02},
	{0x0660, 0x00},
	{0x0661, 0x16},
	{0x0662, 0x07},
	{0x0663, 0xf1},
	{0x0664, 0x07},
	{0x0665, 0xde},
	{0x0666, 0x07},
	{0x0667, 0xe7},
	{0x0668, 0x00},
	{0x0669, 0x35},
	{0x066a, 0x07},
	{0x066b, 0xf9},
	{0x066c, 0x07},
	{0x066d, 0xb7},
	{0x066e, 0x00},
	{0x066f, 0x27},
	{0x0670, 0x07},
	{0x0671, 0xf3},
	{0x0672, 0x07},
	{0x0673, 0xc5},
	{0x0674, 0x07},
	{0x0675, 0xee},
	{0x0676, 0x00},
	{0x0677, 0x16},
	{0x0678, 0x01},
	{0x0679, 0x80},
	{0x067a, 0x00},
	{0x067b, 0x85},
	{0x067c, 0x07},
	{0x067d, 0xe1},
	{0x067e, 0x07},
	{0x067f, 0xf5},
	{0x0680, 0x07},
	{0x0681, 0xb9},
	{0x0682, 0x00},
	{0x0683, 0x31},
	{0x0684, 0x07},
	{0x0685, 0xe6},
	{0x0686, 0x07},
	{0x0687, 0xd3},
	{0x0688, 0x00},
	{0x0689, 0x18},
	{0x068a, 0x07},
	{0x068b, 0xfa},
	{0x068c, 0x07},
	{0x068d, 0xd2},
	{0x068e, 0x00},
	{0x068f, 0x08},
	{0x0690, 0x00},
	{0x0691, 0x02},
	{0xAFD0, 0x03},
	{0xAFD3, 0x18},
	{0xAFD4, 0x04},
	{0xAFD5, 0xB8},
	{0xAFD6, 0x02},
	{0xAFD7, 0x44},
	{0xAFD8, 0x02},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01},
};

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs[] = {
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x0540, 0x00},
	{0x0541, 0xBC},
	{0x0542, 0x00},
	{0x0543, 0xE1},
	{0x0020, 0x00},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};
#else

static struct regval_list sensor_720P_regs[] = {
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x0540, 0x00},
	{0x0541, 0xBC},
	{0x0542, 0x00},
	{0x0543, 0xE1},
	{0x0020, 0x08},
	{0x0398, 0x02},
	{0x0399, 0xF0},
	{0x039A, 0x03},
	{0x039B, 0xAC},
	{0x039C, 0x04},
	{0x039D, 0x68},
	{0x039E, 0x05},
	{0x039F, 0xE0},
	{0x03A0, 0x07},
	{0x03A1, 0x58},
	{0x03A2, 0x07},
	{0x03A3, 0x58},
	{0x03A4, 0x07},
	{0x03A5, 0x58},
	{0x03A6, 0x18},
	{0x03A7, 0x1C},
	{0x03A8, 0x20},
	{0x03A9, 0x24},
	{0x03AA, 0x28},
	{0x03AB, 0x30},
	{0x03AC, 0x24},
	{0x03AD, 0x21},
	{0x03AE, 0x1C},
	{0x03AF, 0x18},
	{0x03B0, 0x17},
	{0x03B1, 0x13},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_15fps[] = {
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x002B, 0x00},
	{0x0540, 0x00},
	{0x0541, 0x5E},
	{0x0542, 0x00},
	{0x0543, 0x71},
	{0x0020, 0x00},
	{0x0025, 0x01},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},

};
#else

static struct regval_list sensor_720P_regs_15fps[] = {
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x002B, 0x00},
	{0x0540, 0x00},
	{0x0541, 0x5E},
	{0x0542, 0x00},
	{0x0543, 0x71},
	{0x0020, 0x00},
	{0x0025, 0x01},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},

};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_20fps[] = {
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x002B, 0x01},
	{0x0540, 0x00},
	{0x0541, 0x7D},
	{0x0542, 0x00},
	{0x0543, 0x96},
	{0x0020, 0x00},
	{0x0025, 0x01},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};
#else

static struct regval_list sensor_720P_regs_20fps[] = {
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x002B, 0x01},
	{0x0540, 0x00},
	{0x0541, 0x7D},
	{0x0542, 0x00},
	{0x0543, 0x96},
	{0x0020, 0x00},
	{0x0025, 0x01},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_25fps[] = {
	{0x0025, 0x00},
	{0x0026, 0x73},
	{0x002B, 0x01},
	{0x0540, 0x00},
	{0x0541, 0x9C},
	{0x0542, 0x00},
	{0x0543, 0xBC},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},

};
#else

static struct regval_list sensor_720P_regs_25fps[] = {

	{0x0025, 0x00},
	{0x0026, 0x73},
	{0x002B, 0x01},
	{0x0540, 0x00},
	{0x0541, 0x9C},
	{0x0542, 0x00},
	{0x0543, 0xBC},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};
#endif

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_720P_regs_30fps[] = {
	{0x0005, 0x00},
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x0540, 0x00},
	{0x0541, 0xBC},
	{0x0542, 0x00},
	{0x0543, 0xE1},
	{0x0020, 0x00},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
	{0x0005, 0x01},
};
#else

static struct regval_list sensor_720P_regs_30fps[] = {

	{0x0005, 0x00},
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x0540, 0x00},
	{0x0541, 0xBC},
	{0x0542, 0x00},
	{0x0543, 0xE1},

	{0x0122, 0xEB},
	{0x0125, 0xDF},
	{0x0126, 0x70},

	{0x05E0, 0x64},
	{0x05E1, 0x00},
	{0x05E2, 0x00},
	{0x05E3, 0x01},
	{0x05E4, 0x08},
	{0x05E5, 0x00},
	{0x05E6, 0x07},
	{0x05E7, 0x05},
	{0x05E8, 0x08},
	{0x05E9, 0x00},
	{0x05EA, 0xD7},
	{0x05EB, 0x02},
	{0x0020, 0x00},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
	{0x0005, 0x01},

};
#endif

static struct regval_list reg_640x480_30Fps_74Mhz_MCLK24Mhz[] = {
	{0x0005, 0x00},
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x002B, 0x00},
	{0x0006, 0x10},
	{0x000D, 0x00},
	{0x000E, 0x00},
	{0x0122, 0x6B},
	{0x0125, 0xFF},
	{0x0126, 0x70},
	{0x05E0, 0xC1},
	{0x05E1, 0x00},
	{0x05E2, 0xC1},
	{0x05E3, 0x00},
	{0x05E4, 0x03},
	{0x05E5, 0x00},
	{0x05E6, 0x82},
	{0x05E7, 0x02},
	{0x05E8, 0x04},
	{0x05E9, 0x00},
	{0x05EA, 0xE3},
	{0x05EB, 0x01},
	{0x0020, 0x08},
	{0x038F, 0x04},
	{0x0390, 0x68},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0005, 0x01},

};

static struct regval_list sensor_wb_manual[] = {
};

static struct regval_list sensor_wb_auto_regs[] = {
	{0x0380, 0xFF},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	{0x0380, 0xFD},
	{0x032D, 0x4E},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},
	{0x0331, 0x93},
	{0x0332, 0x01},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	{0x0380, 0xFD},
	{0x032D, 0x7E},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},
	{0x0331, 0x4A},
	{0x0332, 0x01},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_wb_tungsten_regs[] = {

	{0x0380, 0xFD},
	{0x032D, 0x01},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},
	{0x0331, 0xB2},
	{0x0332, 0x01},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_wb_horizon[] = {
};

static struct regval_list sensor_wb_daylight_regs[] = {
	{0x0380, 0xFD},
	{0x032D, 0x8F},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},
	{0x0331, 0x1D},
	{0x0332, 0x01},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_wb_flash[] = {
};

static struct regval_list sensor_wb_cloud_regs[] = {

	{0x0380, 0xFD},
	{0x032D, 0x22},
	{0x032E, 0x01},
	{0x032F, 0x00},
	{0x0330, 0x01},
	{0x0331, 0x7D},
	{0x0332, 0x01},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
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
	{0x0120, 0x36},
	{0x0488, 0x10},
	{0x0486, 0x00},
	{0x0487, 0xFF},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{0x0120, 0x26},
	{0x0488, 0x11},
	{0x0486, 0x80},
	{0x0487, 0x80},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{0x0120, 0x36},
	{0x0488, 0x11},
	{0x0486, 0x40},
	{0x0487, 0xA0},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{0x0120, 0x36},
	{0x0488, 0x12},
	{0x0486, 0x00},
	{0x0487, 0x00},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{0x0120, 0x26},
	{0x0488, 0x11},
	{0x0486, 0xB0},
	{0x0487, 0x30},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{0x0120, 0x26},
	{0x0488, 0x11},
	{0x0486, 0x70},
	{0x0487, 0x50},
	{0x0000, 0x01},
	{0x0100, 0x01},
	{0x0101, 0x01},
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
	{0x0005, 0x00},
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x002B, 0x00},
	{0x0006, 0x00},
	{0x000D, 0x01},
	{0x000E, 0x11},
	{0x0122, 0xEB},
	{0x0125, 0xFF},
	{0x05E0, 0xBE},
	{0x05E1, 0x00},
	{0x05E2, 0xBE},
	{0x05E3, 0x00},
	{0x05E4, 0x3A},
	{0x05E5, 0x00},
	{0x05E6, 0x79},
	{0x05E7, 0x01},
	{0x05E8, 0x04},
	{0x05E9, 0x00},
	{0x05EA, 0xF3},
	{0x05EB, 0x00},
	{0x0000, 0x01},
	{0x0101, 0x01},
	{0x0100, 0x01},
	{0x0005, 0x01},

};


static struct regval_list sensor_xga_regs[] = {
	{0x32BF, 0x60},
	{0x32C0, 0x6A},
	{0x32C1, 0x6A},
	{0x32C2, 0x6A},
	{0x32C3, 0x00},
	{0x32C4, 0x20},
	{0x32C5, 0x20},
	{0x32C6, 0x20},
	{0x32C7, 0x00},
	{0x32C8, 0xB9},
	{0x32C9, 0x6A},
	{0x32CA, 0x8A},
	{0x32CB, 0x8A},
	{0x32CC, 0x8A},
	{0x32CD, 0x8A},
	{0x32DB, 0x77},
	{0x32E0, 0x04},
	{0x32E1, 0x00},
	{0x32E2, 0x02},
	{0x32E3, 0x40},
	{0x32E4, 0x00},
	{0x32E5, 0x00},
	{0x32E6, 0x00},
	{0x32E7, 0x00},
	{0x3200, 0x3E},
	{0x3201, 0x0F},
	{0x3028, 0x24},
	{0x3029, 0x20},
	{0x302A, 0x04},
	{0x3022, HM1055_0X3022},
	{0x3023, 0x24},
	{0x3002, 0x00},
	{0x3003, 0x84},
	{0x3004, 0x00},
	{0x3005, 0x4C},
	{0x3006, 0x04},
	{0x3007, 0x83},
	{0x3008, 0x02},
	{0x3009, 0x8B},
	{0x300A, 0x07},
	{0x300B, 0xD0},
	{0x300C, 0x02},
	{0x300D, 0xE4},
	{0x300E, 0x04},
	{0x300F, 0x00},
	{0x3010, 0x02},
	{0x3011, 0x40},
	{0x32BB, 0x87},
	{0x3201, 0x7F},
	{0x3021, 0x06},
	{0x3060, 0x01},
};

#if defined(FIXED_FRAME_RATE)

static struct regval_list sensor_vga_regs[] = {

	{0x0005, 0x00},
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x0540, 0x00},
	{0x0541, 0xBC},
	{0x0542, 0x00},
	{0x0543, 0xE1},

	{0x0122, 0xEB},
	{0x0125, 0xDF},
	{0x0126, 0x70},

	{0x05E0, 0x64},
	{0x05E1, 0x00},
	{0x05E2, 0x00},
	{0x05E3, 0x01},
	{0x05E4, 0x08},
	{0x05E5, 0x00},
	{0x05E6, 0x07},
	{0x05E7, 0x05},
	{0x05E8, 0x08},
	{0x05E9, 0x00},
	{0x05EA, 0xD7},
	{0x05EB, 0x02},
	{0x0020, 0x00},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
	{0x0005, 0x01},
};
#else

static struct regval_list sensor_vga_regs[] = {

	{0x0005, 0x00},
	{0x0025, 0x00},
	{0x0026, 0x77},
	{0x0540, 0x00},
	{0x0541, 0xBC},
	{0x0542, 0x00},
	{0x0543, 0xE1},

	{0x0122, 0xEB},
	{0x0125, 0xDF},
	{0x0126, 0x70},

	{0x05E0, 0x64},
	{0x05E1, 0x00},
	{0x05E2, 0x00},
	{0x05E3, 0x01},
	{0x05E4, 0x08},
	{0x05E5, 0x00},
	{0x05E6, 0x07},
	{0x05E7, 0x05},
	{0x05E8, 0x08},
	{0x05E9, 0x00},
	{0x05EA, 0xD7},
	{0x05EB, 0x02},
	{0x0020, 0x00},
	{0x038F, 0x02},
	{0x0390, 0xF0},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
	{0x0005, 0x01},
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
	 .regs = sensor_svga_regs,
	 .size = ARRAY_SIZE(sensor_svga_regs),
	 },
	 {
	 .regs = sensor_vga_regs,
	 .size = ARRAY_SIZE(sensor_vga_regs),
	 },
	 {
	 .regs = sensor_qvga_regs,
	 .size = ARRAY_SIZE(sensor_qvga_regs),
	 },
};

static struct regval_list sensor_brightness_neg4_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0xB0},
};

static struct regval_list sensor_brightness_neg3_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0xA0},
};

static struct regval_list sensor_brightness_neg2_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0x90},
};

static struct regval_list sensor_brightness_neg1_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0x80},
};

static struct regval_list sensor_brightness_zero_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0x00},
};

static struct regval_list sensor_brightness_pos1_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0x10},
};

static struct regval_list sensor_brightness_pos2_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0x20},
};

static struct regval_list sensor_brightness_pos3_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0x30},
};

static struct regval_list sensor_brightness_pos4_regs[] = {
	{0x0125, 0xDF},
	{0x04C0, 0x40},
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
	{0x0480, 0x10},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_neg3_regs[] = {
	{0x0480, 0x20},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_neg2_regs[] = {
	{0x0480, 0x30},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_neg1_regs[] = {
	{0x0480, 0x40},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_zero_regs[] = {
	{0x0480, 0x50},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_pos1_regs[] = {
	{0x0480, 0x60},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_pos2_regs[] = {
	{0x0480, 0x70},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_pos3_regs[] = {
	{0x0480, 0x80},
	{0x0000, 0x01},
};

static struct regval_list sensor_saturation_pos4_regs[] = {
	{0x0480, 0x90},
	{0x0000, 0x01},
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
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_zero_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0x0381, 0x44},
	{0x0382, 0x34},
	{0x038E, 0x3C},
	{0x0100, 0x01},
	{0x0101, 0x01},
	{0x0000, 0x01},
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
	{0x0027, 0x30},
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0x0027, 0x70},
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0x0027, 0x18},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0x0027, 0x10},
};

static struct regval_list sensor_fmt_raw[] = {
	{0x0027, 0x20},
};


static int sensor_read_hm1055(struct v4l2_subdev *sd, unsigned short reg,
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

static int sensor_write_hm1055(struct v4l2_subdev *sd, unsigned short reg,
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

static int sensor_write_hm1055_array(struct v4l2_subdev *sd,
				     struct regval_list *regs, int array_size)
{
	int i = 0;

	if (!regs)
		return 0;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			msleep(regs->data);
		} else {
			LOG_ERR_RET(sensor_write_hm1055
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

	return 0;

	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_hm1055(sd, 0x0006, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_g_hflip!\n");
		return ret;
	}
	vfe_dev_dbg("HM1055 sensor_read_hm1055 sensor_g_hflip(%x)\n", val);

	val &= (1 << 1);
	val = val >> 0;

	*value = val;

	info->hflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	return 0;

	struct sensor_info *info = to_state(sd);
	unsigned char val;
	ret = sensor_read_hm1055(sd, 0x0006, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_s_hflip!\n");
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

	ret = sensor_write_hm1055(sd, 0x0006, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_hm1055 err at sensor_s_hflip!\n");
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

	ret = sensor_read_hm1055(sd, 0x0006, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_g_vflip!\n");
		return ret;
	}
	vfe_dev_dbg("HM1055 sensor_read_hm1055 sensor_g_vflip(%x)\n", val);

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
	ret = sensor_read_hm1055(sd, 0x0006, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_s_vflip!\n");
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
	ret = sensor_write_hm1055(sd, 0x0006, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_hm1055 err at sensor_s_vflip!\n");
		return ret;
	}

	mdelay(20);

	info->vflip = value;
	return 0;
}

static int sensor_s_flip(struct v4l2_subdev *sd, int value)
{
	int ret;
	return 0;

	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_hm1055(sd, 0x0006, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_s_vflip!\n");
		return ret;
	}

	switch (value) {
	case 0:
		val &= 0xfe;
		break;
	case 1:
		val |= 0x01;
		break;
	case 2:
		val &= 0xfd;
		break;
	case 3:
		val |= 0x02;
		break;
	default:
		return -EINVAL;
	}
	ret = sensor_write_hm1055(sd, 0x0006, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_hm1055 err at sensor_s_vflip!\n");
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
	return 0;

	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_hm1055(sd, 0x0380, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_g_autoexp!\n");
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
	return 0;

	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_hm1055(sd, 0x0380, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_s_autoexp!\n");
		return ret;
	}

	switch (value) {
	case V4L2_EXPOSURE_AUTO:
		val |= 0x01;
		break;
	case V4L2_EXPOSURE_MANUAL:
		val &= 0xfe;
		break;
	case V4L2_EXPOSURE_SHUTTER_PRIORITY:
		return -EINVAL;
	case V4L2_EXPOSURE_APERTURE_PRIORITY:
		return -EINVAL;
	default:
		return -EINVAL;
	}

	ret = sensor_write_hm1055(sd, 0x0380, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_hm1055 err at sensor_s_autoexp!\n");
		return ret;
	}

	mdelay(20);

	info->autoexp = value;
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	int ret;
	return 0;

	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_read_hm1055(sd, 0x0380, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_g_autowb!\n");
		return ret;
	}

	val = ((val & 0x02) >> 1);


	*value = val;
	info->autowb = *value;

	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	return 0;

	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret =
	    sensor_write_hm1055_array(sd, sensor_wb_auto_regs,
				      ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		vfe_dev_err
		    ("sensor_write_hm1055_array err at sensor_s_autowb!\n");
		return ret;
	}

	ret = sensor_read_hm1055(sd, 0x0380, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_s_autowb!\n");
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
		break;
	}
	ret = sensor_write_hm1055(sd, 0x0380, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write_hm1055 err at sensor_s_autowb!\n");
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

	LOG_ERR_RET(sensor_write_hm1055_array
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

	LOG_ERR_RET(sensor_write_hm1055_array
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

	LOG_ERR_RET(sensor_write_hm1055_array
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

	LOG_ERR_RET(sensor_write_hm1055_array
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

	LOG_ERR_RET(sensor_write_hm1055_array
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

	if (value == V4L2_RESOLUTION_1080P) {
		printk("########Can't Support 1080p############\n");
		value = info->resolution;
	}

	LOG_ERR_RET(sensor_write_hm1055_array
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
		vfe_dev_dbg("CSI_SUBDEV_PWR_ON\n");

		cci_lock(sd);
		info->streaming = 0;

		vfe_gpio_set_status(sd, PWDN, 1);
		vfe_gpio_set_status(sd, RESET, 1);
		vfe_gpio_set_status(sd, POWER_EN, 1);
		mdelay(10);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		vfe_gpio_write(sd, POWER_EN, 1);
		vfe_gpio_write(sd, PWDN, 0);
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
		vfe_dev_dbg("CSI_SUBDEV_PWR_OFF\n");
		cci_lock(sd);
		mdelay(10);
		vfe_gpio_set_status(sd, PWDN, 1);
		vfe_gpio_set_status(sd, RESET, 1);
		vfe_gpio_set_status(sd, POWER_EN, 1);
		mdelay(10);
		vfe_gpio_write(sd, POWER_EN, 0);
		vfe_gpio_write(sd, PWDN, 1);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		mdelay(10);
		vfe_set_mclk(sd, OFF);
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

	ret = sensor_read_hm1055(sd, 0x0001, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_detect!\n");
		return ret;
	}

	SENSOR_ID |= (val << 8);

	ret = sensor_read_hm1055(sd, 0x0002, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read_hm1055 err at sensor_detect!\n");
		return ret;
	}

	printk("############ HM1055 SENSOR_ID=0x%x\n", SENSOR_ID);

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
	return sensor_write_hm1055_array(sd, sensor_default_regs,
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

	sensor_write_hm1055_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	ret = 0;
	if (wsize->regs) {
		ret =
		    sensor_write_hm1055_array(sd, wsize->regs,
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
		{0x0020, 0x00},
		{0x0025, 0x01},
		{0x0100, 0x01},
		{0x0101, 0x01},
		{0x0000, 0x01},
	};
	struct regval_list daylight_regs[] = {

		{0x0020, 0x00},
		{0x0025, 0x00},
		{0x0100, 0x01},
		{0x0101, 0x01},
		{0x0000, 0x01},
	};

	if (!info->streaming) {
		return;
	}

	ret = sensor_read_hm1055(sd, 0x0018, &val);
	if (ret < 0) {
		printk("auto_scene_worker error.\n");
		return;
	}
	if ((val > 0x02) && (info->night_mode == 0)) {
		ret =
		    sensor_write_hm1055_array(sd, night_regs,
					      ARRAY_SIZE(night_regs));
		if (ret < 0) {
			printk("auto_scene error, return %x!\n", ret);
			return;
		}
		info->night_mode = 1;
		printk("debug switch to night mode\n");
	} else if ((val < 0x02) && (info->night_mode == 1)) {
		ret =
		    sensor_write_hm1055_array(sd, daylight_regs,
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

	hm1055_sd = sd;

	info->wq = create_singlethread_workqueue("kworkqueue_hm1055");
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
	{.compatible = "allwinner,sensor_hm1055",},
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
