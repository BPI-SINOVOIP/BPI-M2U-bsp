/*
 * A V4L2 driver for Superpix sp0a19 cameras.
 *
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


MODULE_AUTHOR("zhaoyuyang");
MODULE_DESCRIPTION("A low-level driver for Superpix sp0a19 sensors");
MODULE_LICENSE("GPL");



/*for internel driver debug*/
#define DEV_DBG_EN				0
#if (DEV_DBG_EN == 1)
#define vfe_dev_dbg(x, arg...) printk("[CSI_DEBUG][sp0a19]"x, ##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...) printk("[CSI_ERR][sp0a19]"x, ##arg)
#define vfe_dev_print(x, arg...) printk("[CSI][sp0a19]"x, ##arg)

#define LOG_ERR_RET(x)  { \
	int ret;  \
	ret = x; \
	if (ret < 0) {\
		vfe_dev_err("error at %s\n", __func__);  \
		return ret; \
	} \
}

/*define module timing*/
#define MCLK (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x0a19

/*define the voltage level of control signal*/
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

#define VGA_WIDTH		640
#define VGA_HEIGHT		480
#define QVGA_WIDTH		320
#define QVGA_HEIGHT	240
#define CIF_WIDTH		352
#define CIF_HEIGHT		288
#define QCIF_WIDTH		176
#define	QCIF_HEIGHT	144

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 20

/*
 * The sp0a19 sits on i2c with ID 0x42
 */
#define I2C_ADDR 0x42
#define SENSOR_NAME "sp0a19"

/* Registers */
/*AE target*/
#define	SP0A19_P0_0xf7  0x88
#define	SP0A19_P0_0xf8  0x80
#define	SP0A19_P0_0xf9  0x70
#define	SP0A19_P0_0xfa  0x68

#define	SP0A19_P0_0xdd	0x7c
#define	SP0A19_P0_0xde	0xc4

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;  /* coming later */

struct cfg_array { /* coming later */
	struct regval_list *regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

static struct regval_list sensor_default_regs[] = {
	/*[Sensor]*/
	{0xfd, 0x00},
	{0x1c, 0x28},
	{0x32, 0x00},
	{0x0f, 0x2f},
	{0x10, 0x2e},
	{0x11, 0x00},
	{0x12, 0x18},
	{0x13, 0x2f},
	{0x14, 0x00},
	{0x15, 0x3f},
	{0x16, 0x00},
	{0x17, 0x18},
	{0x25, 0x40},
	{0x1a, 0x0b},
	{0x1b, 0xc },
	{0x1e, 0xb },
	{0x20, 0x3f},
	{0x21, 0x13},
	{0x22, 0x19},
	{0x26, 0x1a},
	{0x27, 0xab},
	{0x28, 0xfd},
	{0x30, 0x00},
	{0x31, 0x00},
	{0xfb, 0x33},
	{0x1f, 0x08},

	/*Blacklevel*/
	{0xfd, 0x00},
	{0x65, 0x00},/*06*/
	{0x66, 0x00},/*06*/
	{0x67, 0x00},/*06*/
	{0x68, 0x00},/*06*/
	{0x45, 0x00},
	{0x46, 0x0f},

	/*ae setting*/
	{0xfd, 0x00},
	{0x03, 0x01},
	{0x04, 0x32},
	{0x06, 0x00},
	{0x09, 0x01},
	{0x0a, 0x46},
	{0xf0, 0x66},
	{0xf1, 0x00},
	{0xfd, 0x01},
	{0x90, 0x0c},
	{0x92, 0x01},
	{0x98, 0x66},
	{0x99, 0x00},
	{0x9a, 0x01},
	{0x9b, 0x00},

	/*Status*/
	{0xfd, 0x01},
	{0xce, 0xc8},
	{0xcf, 0x04},
	{0xd0, 0xc8},
	{0xd1, 0x04},

	{0xfd, 0x01},
	{0xc4, 0x56},
	{0xc5, 0x8f},
	{0xca, 0x30},
	{0xcb, 0x45},
	{0xcc, 0x70},
	{0xcd, 0x48},
	{0xfd, 0x00},

	/*lsc  for st*/
	{0xfd, 0x01},
	{0x35, 0x15},
	{0x36, 0x15},
	{0x37, 0x15},
	{0x38, 0x15},
	{0x39, 0x15},
	{0x3a, 0x15},
	{0x3b, 0x13},
	{0x3c, 0x15},
	{0x3d, 0x15},
	{0x3e, 0x15},
	{0x3f, 0x15},
	{0x40, 0x18},
	{0x41, 0x00},
	{0x42, 0x04},
	{0x43, 0x04},
	{0x44, 0x00},
	{0x45, 0x00},
	{0x46, 0x00},
	{0x47, 0x00},
	{0x48, 0x00},
	{0x49, 0xfd},
	{0x4a, 0x00},
	{0x4b, 0x00},
	{0x4c, 0xfd},
	{0xfd, 0x00},
	/*awb1*/
	{0xfd, 0x01},
	{0x28, 0xc5},
	{0x29, 0x9b},
	{0x2e, 0x02},
	{0x2f, 0x16},
	{0x17, 0x17},
	{0x18, 0x19},
	{0x19, 0x45},
	{0x2a, 0xef},
	{0x2b, 0x15},
	/*awb2*/
	{0xfd, 0x01},
	{0x73, 0x80},
	{0x1a, 0x80},
	{0x1b, 0x80},
	/*d65*/
	{0x65, 0xd5},
	{0x66, 0xfa},
	{0x67, 0x72},
	{0x68, 0x8a},
	/*indoor*/
	{0x69, 0xc6},
	{0x6a, 0xee},
	{0x6b, 0x94},
	{0x6c, 0xab},
	/*f*/
	{0x61, 0x7a},
	{0x62, 0x98},
	{0x63, 0xc5},
	{0x64, 0xe6},
	/*cwf*/
	{0x6d, 0xb9},
	{0x6e, 0xde},
	{0x6f, 0xb2},
	{0x70, 0xd5},

	/*skin detect*/
	{0xfd, 0x01},
	{0x08, 0x15},
	{0x09, 0x04},
	{0x0a, 0x20},
	{0x0b, 0x12},
	{0x0c, 0x27},
	{0x0d, 0x06},
	{0x0f, 0x63},
	/*BPC_grad*/
	{0xfd, 0x00},
	{0x79, 0xf0},
	{0x7a, 0x80},  /*f0*/
	{0x7b, 0x80},  /*f0*/
	{0x7c, 0x20},  /*f0*/

	/*smooth*/
	{0xfd, 0x00},
	/*��ͨ����ƽ����ֵ*/
	{0x57, 0x08},	/*raw_dif_thr_outdoor*/
	{0x58, 0x0c}, /*raw_dif_thr_normal*/
	{0x56, 0x0e}, /*raw_dif_thr_dummy*/
	{0x59, 0x12}, /*raw_dif_thr_lowlight*/
	/*GrGbƽ����ֵ*/
	{0x89, 0x08},	/*raw_grgb_thr_outdoor*/
	{0x8a, 0x0c}, /*raw_grgb_thr_normal*/
	{0x9c, 0x0e}, /*raw_grgb_thr_dummy*/
	{0x9d, 0x12}, /*raw_grgb_thr_lowlight*/

	/*Gr\Gb֮��ƽ��ǿ��*/
	{0x81, 0xe0},    /*raw_gflt_fac_outdoor*/
	{0x82, 0x80}, /*80  raw_gflt_fac_normal*/
	{0x83, 0x40},    /*raw_gflt_fac_dummy*/
	{0x84, 0x20},    /*raw_gflt_fac_lowlight*/
	/*Gr��Gb��ͨ����ƽ��ǿ��*/
	{0x85, 0xe0}, /*raw_gf_fac_outdoor*/
	{0x86, 0x80}, /*raw_gf_fac_normal*/
	{0x87, 0x40}, /*raw_gf_fac_dummy*/
	{0x88, 0x20}, /*raw_gf_fac_lowlight*/
	/*R��Bƽ��ǿ��*/
	{0x5a, 0xff},		 /*raw_rb_fac_outdoor*/
	{0x5b, 0xe0}, /*40raw_rb_fac_normal*/
	{0x5c, 0x80}, 	 /*raw_rb_fac_dummy*/
	{0x5d, 0x20}, 	 /*raw_rb_fac_lowlight*/

	/*sharpen*/
	{0xfd, 0x01},
	{0xe2, 0x50},	/*sharpen_y_base*/
	{0xe4, 0xa0},	/*sharpen_y_max*/

	{0xe5, 0x08}, /*rangek_neg_outdoor*/
	{0xd3, 0x08}, /*rangek_pos_outdoor*/
	{0xd7, 0x08}, /*range_base_outdoor*/

	{0xe6, 0x0a}, /*rangek_neg_normal*/
	{0xd4, 0x0a}, /*rangek_pos_normal*/
	{0xd8, 0x0a}, /*range_base_normal*/

	{0xe7, 0x12}, /*rangek_neg_dummy*/
	{0xd5, 0x12}, /*rangek_pos_dummy*/
	{0xd9, 0x12}, /*range_base_dummy*/

	{0xd2, 0x15}, /*rangek_neg_lowlight*/
	{0xd6, 0x15}, /*rangek_pos_lowlight*/
	{0xda, 0x15}, /*range_base_lowlight*/

	{0xe8, 0x20},/*sharp_fac_pos_outdoor*/
	{0xec, 0x2c},/*sharp_fac_neg_outdoor*/
	{0xe9, 0x20},/*sharp_fac_pos_nr*/
	{0xed, 0x2c},/*sharp_fac_neg_nr*/
	{0xea, 0x18},/*sharp_fac_pos_dummy*/
	{0xef, 0x1c},/*sharp_fac_neg_dummy*/
	{0xeb, 0x15},/*sharp_fac_pos_low*/
	{0xf0, 0x18},/*sharp_fac_neg_low*/

	/*CCM*/
	{0xfd, 0x01},
	{0xa0, 0x66},/*80(��ɫ�ӽ�����ɫ������)*/
	{0xa1, 0x0 },/*0*/
	{0xa2, 0x19},/*0*/
	{0xa3, 0xf3},/*f0*/
	{0xa4, 0x8e},/*a6*/
	{0xa5, 0x0 },/*ea*/
	{0xa6, 0x0 },/*0*/
	{0xa7, 0xe6},/*e6*/
	{0xa8, 0x9a},/*9a*/
	{0xa9, 0x0 },/*0*/
	{0xaa, 0x3 },/*33*/
	{0xab, 0xc },/*c*/
	{0xfd, 0x00},

	/*gamma*/

	{0xfd, 0x00},
	{0x8b, 0x0 },/*0*/
	{0x8c, 0xC },/*11*/
	{0x8d, 0x19},/*19*/
	{0x8e, 0x2C},/*28*/
	{0x8f, 0x49},/*46*/
	{0x90, 0x61},/*61*/
	{0x91, 0x77},/*78*/
	{0x92, 0x8A},/*8A*/
	{0x93, 0x9B},/*9B*/
	{0x94, 0xA9},/*A9*/
	{0x95, 0xB5},/*B5*/
	{0x96, 0xC0},/*C0*/
	{0x97, 0xCA},/*CA*/
	{0x98, 0xD4},/*D4*/
	{0x99, 0xDD},/*DD*/
	{0x9a, 0xE6},/*E6*/
	{0x9b, 0xEF},/*EF*/
	{0xfd, 0x01},/*01*/
	{0x8d, 0xF7},/*F7*/
	{0x8e, 0xFF},/*FF*/

	/*rpc*/
	{0xfd, 0x00},
	{0xe0, 0x4c},
	{0xe1, 0x3c},
	{0xe2, 0x34},
	{0xe3, 0x2e},
	{0xe4, 0x2e},
	{0xe5, 0x2c},
	{0xe6, 0x2c},
	{0xe8, 0x2a},
	{0xe9, 0x2a},
	{0xea, 0x2a},
	{0xeb, 0x28},
	{0xf5, 0x28},
	{0xf6, 0x28},
	/*ae min gain*/
	{0xfd, 0x01},
	{0x94, 0xa0},/*rpc_max_indr*/
	{0x95, 0x28},/*1e rpc_min_indr*/
	{0x9c, 0xa0},/*rpc_max_outdr*/
	{0x9d, 0x28},/*rpc_min_outdr*/

	/*ae target*/
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7+0x04}, /*80*/
	{0xf7, SP0A19_P0_0xf7},
	{0xf8, SP0A19_P0_0xf8},
	{0xec, SP0A19_P0_0xf8-0x04}, /*6c*/

	{0xef, SP0A19_P0_0xf9+0x04}, /*99*/
	{0xf9, SP0A19_P0_0xf9},
	{0xfa, SP0A19_P0_0xfa},
	{0xee, SP0A19_P0_0xfa-0x04}, /*78*/

	/*gray detect*/
	{0xfd, 0x01},
	{0x30, 0x40},
	{0x31, 0x70},/**/
	{0x32, 0x40},/*80*/
	{0x33, 0xef},/**/
	{0x34, 0x05},/*04*/
	{0x4d, 0x2f},/*40*/
	{0x4e, 0x20},/**/
	{0x4f, 0x16},/*13*/
	/*lowlight lum*/
	{0xfd, 0x00},
	{0xb2, 0x10},/*lum_limit*/
	{0xb3, 0x1f},/*lum_set*/
	{0xb4, 0x30},/*black_vt*/
	{0xb5, 0x45},/*white_vt*/
	/*saturation*/
	{0xfd, 0x00},
	{0xbe, 0xff},
	{0xbf, 0x01},
	{0xc0, 0xff},
	{0xc1, 0xd8},
	{0xd3, 0x80},/*0x78*/
	{0xd4, 0x80},/*0x78*/
	{0xd6, 0x70},/*0x78*/
	{0xd7, 0x50},/*0x78*/
	/*HEQ*/
	{0xfd, 0x00},
	{0xdc, 0x00},
	{0xdd, SP0A19_P0_0xdd},
	{0xde, SP0A19_P0_0xde}, /*80*/
	{0xdf, 0x80},
	/*func enable*/
	{0xfd, 0x00},
	{0x32, 0x15},/*d*/
	{0x34, 0x76},/*16*/
	{0x33, 0xef},
	{0x5f, 0x51},
	{0x35, 0x40},  /*0x0d*/
	/* {{SENSOR_WRITE_DELAY, 200},delay 200ms*/
	{0xff, 0xff}
};



/*
 * The white balance settings
 * Here only tune the R G B channel gain.
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */
static struct regval_list sensor_wb_manual[] = {
	/*null*/
};

static struct regval_list sensor_wb_auto_regs[] = {
	{0xfd, 0x01},
	{0x28, 0xc5},
	{0x29, 0x9b},
	{0xfd, 0x00},
	{0x32, 0x15},   /*awb & ae  opened*/
	{0xfd, 0x00}

};

static struct regval_list sensor_wb_cloud_regs[] = {
	{0xfd, 0x00},
	{0x32, 0x05},
	{0xfd, 0x01},
	{0x28, 0xbf},
	{0x29, 0x89},
	{0xfd, 0x00}
};

static struct regval_list sensor_wb_daylight_regs[] = {
	/*tai yang guang*/
	{0xfd, 0x00},
	{0x32, 0x05},
	{0xfd, 0x01},
	{0x28, 0xbc},
	{0x29, 0x5d},
	{0xfd, 0x00}
};

static struct regval_list sensor_wb_incandescence_regs[] = {
	/*bai re guang*/
	{0xfd, 0x00},
	{0x32, 0x05},      /*awb closed, ae opened*/
	{0xfd, 0x01},
	{0x28, 0x89},
	{0x29, 0xb8},
	{0xfd, 0x00}
};

static struct regval_list sensor_wb_horizon[] = {
	/*null*/
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
	/*ri guang deng*/
	{0xfd, 0x00},
	{0x32, 0x05},
	{0xfd, 0x01},
	{0x28, 0xaf},
	{0x29, 0x99},
	{0xfd, 0x00}
};

static struct regval_list sensor_wb_flash[] = {
	/*null*/
};

static struct regval_list sensor_wb_tungsten_regs[] = {
	/*wu si deng U30*/
	{0xfd, 0x00},
	{0x32, 0x05},
	{0xfd, 0x01},
	{0x28, 0x90},
	{0x29, 0xc7},
	{0xfd, 0x00}
};

static struct regval_list sensor_wb_shade[] = {
	/*null*/
};

static struct cfg_array sensor_wb[] = {
	{
		.regs = sensor_wb_manual,             /*V4L2_WHITE_BALANCE_MANUAL*/
		.size = ARRAY_SIZE(sensor_wb_manual),
	},
	{
		.regs = sensor_wb_auto_regs,          /*V4L2_WHITE_BALANCE_AUTO*/
		.size = ARRAY_SIZE(sensor_wb_auto_regs),
	},
	{
		.regs = sensor_wb_incandescence_regs, /*V4L2_WHITE_BALANCE_INCANDESCENT*/
		.size = ARRAY_SIZE(sensor_wb_incandescence_regs),
	},
	{
		.regs = sensor_wb_fluorescent_regs,   /*V4L2_WHITE_BALANCE_FLUORESCENT*/
		.size = ARRAY_SIZE(sensor_wb_fluorescent_regs),
	},
	{
		.regs = sensor_wb_tungsten_regs,      /*V4L2_WHITE_BALANCE_FLUORESCENT_H*/
		.size = ARRAY_SIZE(sensor_wb_tungsten_regs),
	},
	{
		.regs = sensor_wb_horizon,            /*V4L2_WHITE_BALANCE_HORIZON*/
		.size = ARRAY_SIZE(sensor_wb_horizon),
	},
	{
		.regs = sensor_wb_daylight_regs,      /*V4L2_WHITE_BALANCE_DAYLIGHT*/
		.size = ARRAY_SIZE(sensor_wb_daylight_regs),
	},
	{
		.regs = sensor_wb_flash,              /*V4L2_WHITE_BALANCE_FLASH*/
		.size = ARRAY_SIZE(sensor_wb_flash),
	},
	{
		.regs = sensor_wb_cloud_regs,         /*V4L2_WHITE_BALANCE_CLOUDY*/
		.size = ARRAY_SIZE(sensor_wb_cloud_regs),
	},
	{
		.regs = sensor_wb_shade,              /*V4L2_WHITE_BALANCE_SHADE*/
		.size = ARRAY_SIZE(sensor_wb_shade),
	},
	/*  {
		.regs = NULL,
		.size = 0,
	  },*/
};


/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x00},
	{0x63, 0x80},
	{0x64, 0x80}
};

static struct regval_list sensor_colorfx_bw_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x20},
	{0x63, 0x80},
	{0x64, 0x80}
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x10},
	{0x63, 0xc0},
	{0x64, 0x20}
};

static struct regval_list sensor_colorfx_negative_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x04},
	{0x63, 0x80},
	{0x64, 0x80}
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x01},
	{0x63, 0x80},
	{0x64, 0x80}
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x02},
	{0x63, 0x80},
	{0x64, 0x80}
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x10},
	{0x63, 0x20},
	{0x64, 0xf0}
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
	{0xfd, 0x00},
	{0x62, 0x10},
	{0x63, 0x20},
	{0x64, 0x20}
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_colorfx_aqua_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_colorfx_art_freeze_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_colorfx_silhouette_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_colorfx_solarization_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_colorfx_antique_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_colorfx_set_cbcr_regs[] = {
	/*NULL*/
};

static struct cfg_array sensor_colorfx[] = {
	{
		.regs = sensor_colorfx_none_regs,         /*V4L2_COLORFX_NONE = 0,*/
		.size = ARRAY_SIZE(sensor_colorfx_none_regs),
	},
	{
		.regs = sensor_colorfx_bw_regs,           /*V4L2_COLORFX_BW   = 1,*/
		.size = ARRAY_SIZE(sensor_colorfx_bw_regs),
	},
	{
		.regs = sensor_colorfx_sepia_regs,        /*V4L2_COLORFX_SEPIA  = 2,*/
		.size = ARRAY_SIZE(sensor_colorfx_sepia_regs),
	},
	{
		.regs = sensor_colorfx_negative_regs,     /*V4L2_COLORFX_NEGATIVE = 3,*/
		.size = ARRAY_SIZE(sensor_colorfx_negative_regs),
	},
	{
		.regs = sensor_colorfx_emboss_regs,       /*V4L2_COLORFX_EMBOSS = 4,*/
		.size = ARRAY_SIZE(sensor_colorfx_emboss_regs),
	},
	{
		.regs = sensor_colorfx_sketch_regs,       /*V4L2_COLORFX_SKETCH = 5,*/
		.size = ARRAY_SIZE(sensor_colorfx_sketch_regs),
	},
	{
		.regs = sensor_colorfx_sky_blue_regs,     /*V4L2_COLORFX_SKY_BLUE = 6,*/
		.size = ARRAY_SIZE(sensor_colorfx_sky_blue_regs),
	},
	{
		.regs = sensor_colorfx_grass_green_regs,  /*V4L2_COLORFX_GRASS_GREEN = 7,*/
		.size = ARRAY_SIZE(sensor_colorfx_grass_green_regs),
	},
	{
		.regs = sensor_colorfx_skin_whiten_regs,  /*V4L2_COLORFX_SKIN_WHITEN = 8,*/
		.size = ARRAY_SIZE(sensor_colorfx_skin_whiten_regs),
	},
	{
		.regs = sensor_colorfx_vivid_regs,        /*V4L2_COLORFX_VIVID = 9,*/
		.size = ARRAY_SIZE(sensor_colorfx_vivid_regs),
	},
	{
		.regs = sensor_colorfx_aqua_regs,         /*V4L2_COLORFX_AQUA = 10,*/
		.size = ARRAY_SIZE(sensor_colorfx_aqua_regs),
	},
	{
		.regs = sensor_colorfx_art_freeze_regs,   /*V4L2_COLORFX_ART_FREEZE = 11,*/
		.size = ARRAY_SIZE(sensor_colorfx_art_freeze_regs),
	},
	{
		.regs = sensor_colorfx_silhouette_regs,   /*V4L2_COLORFX_SILHOUETTE = 12,*/
		.size = ARRAY_SIZE(sensor_colorfx_silhouette_regs),
	},
	{
		.regs = sensor_colorfx_solarization_regs, /*V4L2_COLORFX_SOLARIZATION = 13,*/
		.size = ARRAY_SIZE(sensor_colorfx_solarization_regs),
	},
	{
		.regs = sensor_colorfx_antique_regs,      /*V4L2_COLORFX_ANTIQUE = 14,*/
		.size = ARRAY_SIZE(sensor_colorfx_antique_regs),
	},
	{
		.regs = sensor_colorfx_set_cbcr_regs,     /*V4L2_COLORFX_SET_CBCR = 15,*/
		.size = ARRAY_SIZE(sensor_colorfx_set_cbcr_regs),
	},
};



/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0xc0}
};

static struct regval_list sensor_brightness_neg3_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0xd0}
};

static struct regval_list sensor_brightness_neg2_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0xe0}
};

static struct regval_list sensor_brightness_neg1_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0xf0}
};

static struct regval_list sensor_brightness_zero_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0x00}
};

static struct regval_list sensor_brightness_pos1_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0x10}
};

static struct regval_list sensor_brightness_pos2_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0x20}
};

static struct regval_list sensor_brightness_pos3_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0x30}
};

static struct regval_list sensor_brightness_pos4_regs[] = {
	{0xfd, 0x00},
	{0xdc, 0x40}
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

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd-0x40},	/*level -4*/
	{0xde,  SP0A19_P0_0xde-0x40}
};

static struct regval_list sensor_contrast_neg3_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd-0x30},	/*level -3*/
	{0xde,  SP0A19_P0_0xde-0x30}
};

static struct regval_list sensor_contrast_neg2_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd-0x20},	/*level -2*/
	{0xde,  SP0A19_P0_0xde-0x20}
};

static struct regval_list sensor_contrast_neg1_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd-0x10},	/*level -1*/
	{0xde,  SP0A19_P0_0xde-0x10}
};

static struct regval_list sensor_contrast_zero_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd},		/*level 0*/
	{0xde,  SP0A19_P0_0xde},
};

static struct regval_list sensor_contrast_pos1_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd+0x10},	/*level +1*/
	{0xde,  SP0A19_P0_0xde+0x10}
};

static struct regval_list sensor_contrast_pos2_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd+0x20},	/*level +2*/
	{0xde,  SP0A19_P0_0xde+0x20}
};

static struct regval_list sensor_contrast_pos3_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd+0x30},	/*level +3*/
	{0xde,  SP0A19_P0_0xde+0x30}
};

static struct regval_list sensor_contrast_pos4_regs[] = {
	{0xfd, 0x00},
	{0xdd,  SP0A19_P0_0xdd+0x40},	/*level +4*/
	{0xde,  SP0A19_P0_0xde+0x3B}
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

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_saturation_neg3_regs[] = {
	/*NULL*/

};

static struct regval_list sensor_saturation_neg2_regs[] = {
	/*NULL*/

};

static struct regval_list sensor_saturation_neg1_regs[] = {
	/*NULL*/

};

static struct regval_list sensor_saturation_zero_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_saturation_pos1_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_saturation_pos2_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_saturation_pos3_regs[] = {
	/*NULL*/
};

static struct regval_list sensor_saturation_pos4_regs[] = {
	/*NULL*/
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

/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7-0x40+0x04},
	{0xf7, SP0A19_P0_0xf7-0x40},
	{0xf8, SP0A19_P0_0xf8-0x40},
	{0xec, SP0A19_P0_0xf8-0x40-0x04},

	{0xef, SP0A19_P0_0xf9-0x40+0x04},
	{0xf9, SP0A19_P0_0xf9-0x40},
	{0xfa, SP0A19_P0_0xfa-0x40},
	{0xee, SP0A19_P0_0xfa-0x40-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_neg3_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7-0x30+0x04},
	{0xf7, SP0A19_P0_0xf7-0x30},
	{0xf8, SP0A19_P0_0xf8-0x30},
	{0xec, SP0A19_P0_0xf8-0x30-0x04},

	{0xef, SP0A19_P0_0xf9-0x30+0x04},
	{0xf9, SP0A19_P0_0xf9-0x30},
	{0xfa, SP0A19_P0_0xfa-0x30},
	{0xee, SP0A19_P0_0xfa-0x30-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_neg2_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7-0x20+0x04},
	{0xf7, SP0A19_P0_0xf7-0x20},
	{0xf8, SP0A19_P0_0xf8-0x20},
	{0xec, SP0A19_P0_0xf8-0x20-0x04},

	{0xef, SP0A19_P0_0xf9-0x20+0x04},
	{0xf9, SP0A19_P0_0xf9-0x20},
	{0xfa, SP0A19_P0_0xfa-0x20},
	{0xee, SP0A19_P0_0xfa-0x20-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_neg1_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7-0x10+0x04},
	{0xf7, SP0A19_P0_0xf7-0x10},
	{0xf8, SP0A19_P0_0xf8-0x10},
	{0xec, SP0A19_P0_0xf8-0x10-0x04},

	{0xef, SP0A19_P0_0xf9-0x10+0x04},
	{0xf9, SP0A19_P0_0xf9-0x10},
	{0xfa, SP0A19_P0_0xfa-0x10},
	{0xee, SP0A19_P0_0xfa-0x10-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_zero_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7+0x04},
	{0xf7, SP0A19_P0_0xf7},
	{0xf8, SP0A19_P0_0xf8},
	{0xec, SP0A19_P0_0xf8-0x04},

	{0xef, SP0A19_P0_0xf9+0x04},
	{0xf9, SP0A19_P0_0xf9},
	{0xfa, SP0A19_P0_0xfa},
	{0xee, SP0A19_P0_0xfa-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_pos1_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7+0x10+0x04},
	{0xf7, SP0A19_P0_0xf7+0x10},
	{0xf8, SP0A19_P0_0xf8+0x10},
	{0xec, SP0A19_P0_0xf8+0x10-0x04},

	{0xef, SP0A19_P0_0xf9+0x10+0x04},
	{0xf9, SP0A19_P0_0xf9+0x10},
	{0xfa, SP0A19_P0_0xfa+0x10},
	{0xee, SP0A19_P0_0xfa+0x10-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_pos2_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7+0x20+0x04},
	{0xf7, SP0A19_P0_0xf7+0x20},
	{0xf8, SP0A19_P0_0xf8+0x20},
	{0xec, SP0A19_P0_0xf8+0x20-0x04},

	{0xef, SP0A19_P0_0xf9+0x20+0x04},
	{0xf9, SP0A19_P0_0xf9+0x20},
	{0xfa, SP0A19_P0_0xfa+0x20},
	{0xee, SP0A19_P0_0xfa+0x20-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_pos3_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7+0x30+0x04},
	{0xf7, SP0A19_P0_0xf7+0x30},
	{0xf8, SP0A19_P0_0xf8+0x30},
	{0xec, SP0A19_P0_0xf8+0x30-0x04},

	{0xef, SP0A19_P0_0xf9+0x30+0x04},
	{0xf9, SP0A19_P0_0xf9+0x30},
	{0xfa, SP0A19_P0_0xfa+0x30},
	{0xee, SP0A19_P0_0xfa+0x30-0x04},
	/*{0xdc, 0x00}*/
};

static struct regval_list sensor_ev_pos4_regs[] = {
	{0xfd, 0x00},
	{0xed, SP0A19_P0_0xf7+0x38+0x04},
	{0xf7, SP0A19_P0_0xf7+0x38},
	{0xf8, SP0A19_P0_0xf8+0x38},
	{0xec, SP0A19_P0_0xf8+0x38-0x04},

	{0xef, SP0A19_P0_0xf9+0x38+0x04},
	{0xf9, SP0A19_P0_0xf9+0x38},
	{0xfa, SP0A19_P0_0xfa+0x38},
	{0xee, SP0A19_P0_0xfa+0x38-0x04},
	/*{0xdc, 0x00}*/
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

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */


static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0xfd, 0x00},/*Page 0*/
	{0x35, 0x40}	/*YCbYCr*/
};


static struct regval_list sensor_fmt_yuv422_yvyu[] = {
	{0xfd, 0x00},/*Page 0*/
	{0x35, 0x41}	/*YCrYCb*/
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
	{0xfd, 0x00},/*Page 0*/
	{0x35, 0x01}	/*CrYCbY*/
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0xfd, 0x00},/*Page 0*/
	{0x35, 0x00}	/*CbYCrY*/
};
/*
static struct regval_list sensor_fmt_raw[] = {
	raw
};
*/


/*
 * Low-level register I/O.
 *
 */


/*
 * On most platforms, we'd rather do straight i2c I/O.
 */
static int sensor_read(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value) /*!!!!be careful of the para type!!!*/
{
	int ret = 0;
	int cnt = 0;

	/*struct i2c_client *client = v4l2_get_subdevdata(sd);*/
	ret = cci_read_a8_d8(sd, reg, value);
	while (ret != 0 && cnt < 2) {
		ret = cci_read_a8_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		vfe_dev_dbg("sensor read retry=%d\n", cnt);

	return ret;
}

static int sensor_write(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	int ret = 0;
	int cnt = 0;

	/*struct i2c_client *client = v4l2_get_subdevdata(sd);*/

	ret = cci_write_a8_d8(sd, reg, value);
	while (ret != 0 && cnt < 2) {
		ret = cci_write_a8_d8(sd, reg, value);
		cnt++;
	}
	if (cnt > 0)
		vfe_dev_dbg("sensor write retry=%d\n", cnt);

	return ret;
}

/*
 * Write a list of register settings;
 */
static int sensor_write_array (struct v4l2_subdev *sd, struct regval_list *regs, int array_size)
{
	int i = 0;

	if (!regs)
		return 0;
	/*return -EINVAL;*/

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			msleep(regs->data);
		} else {
			/*printk("write 0x%x=0x%x\n", regs->addr, regs->data);*/
			LOG_ERR_RET(sensor_write(sd, regs->addr, regs->data))
		}
		i++;
		regs++;
	}
	return 0;
}

static int sensor_s_hflip_vflip(struct v4l2_subdev *sd, int h_value, int v_value)
{
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;
	LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) /*page 0*/
	LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))
	switch (h_value) {
	case 0:
		rdval &= 0xdf;
		break;
	case 1:
		rdval |= 0x20;
		break;
	default:
		return -EINVAL;
	}

	switch (v_value) {
	case 0:
		rdval &= 0xbf;
		break;
	case 1:
		rdval |= 0x40;
		break;
	default:
		return -EINVAL;
	}
	LOG_ERR_RET(sensor_write(sd, 0x31, rdval))

	usleep_range(10000, 12000);
	info->hflip = h_value;
	info->vflip = v_value;
	return 0;
}

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;

	LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) /*page 0*/
	LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))

	rdval &= (1<<5);
	*value = (rdval>>5);

	info->vflip = *value;
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;

	LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) /*page 0*/
	LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))

	switch (value) {
	case 0:
		rdval &= 0xdf;
		break;
	case 1:
		rdval |= 0x20;
		break;
	default:
		return -EINVAL;
	}

	LOG_ERR_RET(sensor_write(sd, 0x31, rdval))

	usleep_range(10000, 12000);
	info->hflip = value;
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;

	LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) /*page 0*/
		LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))

		rdval &= (1<<6);
	*value = (rdval>>6);

	info->vflip = *value;
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);
	unsigned char rdval;

	LOG_ERR_RET(sensor_write(sd, 0xfd, 0x00)) /*page 0*/
	LOG_ERR_RET(sensor_read(sd, 0x31, &rdval))

	switch (value) {
	case 0:
		rdval &= 0xbf;
		break;
	case 1:
		rdval |= 0x40;
		break;
	default:
		return -EINVAL;
	}

	LOG_ERR_RET(sensor_write(sd, 0x31, rdval))

	usleep_range(10000, 12000);
	info->vflip = value;
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
	/*	int ret;
		struct sensor_info *info = to_state(sd);
		struct regval_list regs;
		*/
	return 0;
}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	/*	int ret;
		struct sensor_info *info = to_state(sd);
		struct regval_list regs;
		*/
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{
	/*	int ret;
		struct sensor_info *info = to_state(sd);
		struct regval_list regs;
		*/
	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	unsigned char val;

	ret = sensor_write_array(sd, sensor_wb_auto_regs, ARRAY_SIZE(sensor_wb_auto_regs));
	if (ret < 0) {
		vfe_dev_err("sensor_write_array err at sensor_s_autowb!\n");
		return ret;
	}

	ret = sensor_write(sd, 0xfd, 0x00);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}
	ret = sensor_read(sd, 0x32, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_s_autowb!\n");
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
	ret = sensor_write(sd, 0x32, val);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_s_autowb!\n");
		return ret;
	}

	mdelay(10);

	info->autowb = value;
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
/* *********************************************end of ******************************************** */

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

	LOG_ERR_RET(sensor_write_array(sd, sensor_brightness[value+4].regs, sensor_brightness[value+4].size))

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

	LOG_ERR_RET(sensor_write_array(sd, sensor_contrast[value+4].regs, sensor_contrast[value+4].size))

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

	LOG_ERR_RET(sensor_write_array(sd, sensor_saturation[value+4].regs, sensor_saturation[value+4].size))

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

	LOG_ERR_RET(sensor_write_array(sd, sensor_ev[value+4].regs, sensor_ev[value+4].size))

		info->exp_bias = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_auto_n_preset_white_balance *wb_type = (enum v4l2_auto_n_preset_white_balance *) value;

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

	LOG_ERR_RET(sensor_write_array(sd, sensor_wb[value].regs , sensor_wb[value].size))

	if (value == V4L2_WHITE_BALANCE_AUTO)
		info->autowb = 1;
	else
		info->autowb = 0;

	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd,
		__s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx *) value;

	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd,
		enum v4l2_colorfx value)
{
	struct sensor_info *info = to_state(sd);

	if (info->clrfx == value)
		return 0;

	LOG_ERR_RET(sensor_write_array(sd, sensor_colorfx[value].regs, sensor_colorfx[value].size))

	info->clrfx = value;
	return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd,
		__s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_flash_led_mode *flash_mode = (enum v4l2_flash_led_mode *) value;

	*flash_mode = info->flash_mode;
	return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
		enum v4l2_flash_led_mode value)
{
	struct sensor_info *info = to_state(sd);
	/*struct vfe_dev *dev=(struct vfe_dev *)dev_get_drvdata(sd->v4l2_dev->dev);
	  int flash_on,flash_off;
	  flash_on = (dev->flash_pol!=0)?1:0;
	  flash_off = (flash_on==1)?0:1;
	  switch (value) {
	  case V4L2_FLASH_MODE_OFF:
	    os_gpio_write(&dev->flash_io,flash_off);
	    break;
	  case V4L2_FLASH_MODE_AUTO:
	    return -EINVAL;
	    break;
	  case V4L2_FLASH_MODE_ON:
	    os_gpio_write(&dev->flash_io,flash_on);
	    break;
	  case V4L2_FLASH_MODE_TORCH:
	    return -EINVAL;
	    break;
	  case V4L2_FLASH_MODE_RED_EYE:
	    return -EINVAL;
	    break;
	  default:
	    return -EINVAL;
	  }*/

	info->flash_mode = value;
	return 0;
}

/*static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret=0;
	unsigned char rdval;

	ret=sensor_read(sd, 0x00, &rdval);
	if(ret!=0)
		return ret;

	if(on_off==CSI_STBY_ON)sw stby on
	{
		ret=sensor_write(sd, 0x00, rdval&0x7f);
	}
	else sw stby off
	{
		ret=sensor_write(sd, 0x00, rdval|0x80);
	}
	return ret;
}*/

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	cci_lock(sd);
	switch (on) {
	case CSI_SUBDEV_STBY_ON:
		vfe_dev_dbg("CSI_SUBDEV_STBY_ON\n");
		/*standby on io*/
		vfe_gpio_write(sd, PWDN, CSI_STBY_ON);
		mdelay(30);
		/*inactive mclk after stadby in*/
		vfe_set_mclk(sd, OFF);
		break;
	case CSI_SUBDEV_STBY_OFF:
		vfe_dev_dbg("CSI_SUBDEV_STBY_OFF\n");
		/*active mclk before stadby out*/
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		mdelay(30);
		/*standby off io*/
		vfe_gpio_write(sd, PWDN, CSI_STBY_OFF);
		mdelay(10);
		/*reset off io*/
		/*csi_gpio_write(sd,&dev->reset_io,CSI_RST_OFF);*/
		/*mdelay(30);*/
		break;
	case CSI_SUBDEV_PWR_ON:
		vfe_dev_dbg("CSI_SUBDEV_PWR_ON\n");
		/*power on reset*/
		vfe_gpio_set_status(sd, PWDN, 1);/*set the gpio to output*/
		vfe_gpio_set_status(sd, RESET, 1);/*set the gpio to output*/

		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		/*reset on io*/
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		/*power supply*/
		vfe_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vfe_set_pmu_channel(sd, IOVDD, ON);
		vfe_set_pmu_channel(sd, AVDD, ON);
		usleep_range(5000, 6000);
		vfe_set_pmu_channel(sd, DVDD, ON);
		vfe_set_pmu_channel(sd, AFVDD, ON);
		usleep_range(5000, 6000);
		/*standby off io*/
		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(5000, 6000);
		/*active mclk before power on*/
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(100000, 120000);

		/*reset after power on*/
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case CSI_SUBDEV_PWR_OFF:
		vfe_dev_dbg("CSI_SUBDEV_PWR_OFF\n");
		/*reset io*/
		mdelay(10);
		vfe_gpio_write(sd, RESET, CSI_RST_ON);
		mdelay(10);
		/*inactive mclk after power off*/
		vfe_set_mclk(sd, OFF);
		/*power supply off*/
		vfe_gpio_write(sd, POWER_EN, CSI_PWR_OFF);
		vfe_set_pmu_channel(sd, AFVDD, OFF);
		vfe_set_pmu_channel(sd, DVDD, OFF);
		vfe_set_pmu_channel(sd, AVDD, OFF);
		vfe_set_pmu_channel(sd, IOVDD, OFF);
		mdelay(10);
		/*standby and reset io*/
		/*standby of io*/
		vfe_gpio_write(sd, PWDN, CSI_STBY_ON);
		mdelay(10);
		/*set the io to hi-z*/
		vfe_gpio_set_status(sd, RESET, 0);/*set the gpio to input*/
		vfe_gpio_set_status(sd, PWDN, 0);/*set the gpio to input*/
		break;
	default:
		return -EINVAL;
	}

	/*remember to unlock i2c adapter, so the device can access the i2c bus again*/
	cci_unlock(sd);
	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
  switch (val) {
  case 0:
    vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
    usleep_range(10000, 12000);
    break;
  case 1:
    vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
    usleep_range(10000, 12000);
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

	ret = sensor_write(sd, 0xfd, 0x00);
	if (ret < 0) {
		vfe_dev_err("sensor_write err at sensor_detect!\n");
		return ret;
	}

	ret = sensor_read(sd, 0x02, &val);
	if (ret < 0) {
		vfe_dev_err("sensor_read err at sensor_detect!\n");
		return ret;
	}

	if (val != 0xa6)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	vfe_dev_dbg("sensor_init\n");

	/*Make sure it is a target sensor*/
	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target chip.\n");
		return ret;
	}

	return sensor_write_array(sd, sensor_default_regs , ARRAY_SIZE(sensor_default_regs));
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	return ret;
}


/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
	__u8 *desc;
	/*__u32 pixelformat;*/
	enum v4l2_mbus_pixelcode mbus_code;/*linux-3.0*/
	struct regval_list *regs;
	int	regs_size;
	int bpp;   /* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,/*linux-3.0*/
		.regs 		= sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp		= 2,
	},
	{
		.desc		= "YVYU 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_YVYU8_2X8,/*linux-3.0*/
		.regs 		= sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp		= 2,
	},
	{
		.desc		= "UYVY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_UYVY8_2X8,/*linux-3.0*/
		.regs 		= sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp		= 2,
	},
	{
		.desc		= "VYUY 4:2:2",
		.mbus_code	= V4L2_MBUS_FMT_VYUY8_2X8,/*linux-3.0*/
		.regs 		= sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp		= 2,
	},
	/*	{
			.desc		= "Raw RGB Bayer",
			.mbus_code	= V4L2_MBUS_FMT_SBGGR8_1X8,linux-3.0
			.regs 		= sensor_fmt_raw,
			.regs_size = ARRAY_SIZE(sensor_fmt_raw),
			.bpp		= 1
		},*/
};
#define N_FMTS ARRAY_SIZE(sensor_formats)


/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */


static struct sensor_win_size
sensor_win_sizes[] = {
	/* VGA */
	{
		.width      = VGA_WIDTH,
		.height     = VGA_HEIGHT,
		.hoffset    = 0,
		.voffset    = 0,
		.regs       = NULL,
		.regs_size  = 0,
		.set_size   = NULL,
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
	if (fsize->index > N_WIN_SIZES-1)
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

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */

	fmt->field = V4L2_FIELD_NONE;

	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
			wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	/*pix->bytesperline = pix->width*sensor_formats[index].bpp;*/
	/*pix->sizeimage = pix->height*pix->bytesperline;*/

	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt)/*linux-3.0*/
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
		struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL ;

	return 0;
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt)/*linux-3.0*/
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);
	vfe_dev_dbg("sensor_s_fmt\n");
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;


	sensor_write_array(sd, sensor_fmt->regs , sensor_fmt->regs_size);

	ret = 0;
	if (wsize->regs) {
		ret = sensor_write_array(sd, wsize->regs , wsize->regs_size);
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
	sensor_s_hflip_vflip(sd, info->hflip, info->vflip);
	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	/*struct sensor_info *info = to_state(sd);*/

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = SENSOR_FRAME_RATE;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
/*		struct v4l2_captureparm *cp = &parms->parm.capture;
	struct v4l2_fract *tpf = &cp->timeperframe;
	struct sensor_info *info = to_state(sd);
	int div;
		if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
			return -EINVAL;
		if (cp->extendedmode != 0)
			return -EINVAL;
		if (tpf->numerator == 0 || tpf->denominator == 0)
			div = 1;		Reset to full rate
		else
			div = (tpf->numerator*SENSOR_FRAME_RATE)/tpf->denominator;
		if (div == 0)
			div = 1;
		else if (div > CLK_SCALE)
			div = CLK_SCALE;
		info->clkrc = (info->clkrc & 0x80) | div;
		tpf->numerator = 1;
		tpf->denominator = sensor_FRAME_RATE/div;
	sensor_write(sd, REG_CLKRC, info->clkrc);*/
	return 0;
}


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */

	switch (qc->id) {
	/*
	case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_CONTRAST:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_SATURATION:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_HUE:
		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);*/
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	/*
	case V4L2_CID_GAIN:
			return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
	case V4L2_CID_AUTOGAIN:
			return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);*/
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

	/*  vfe_dev_dbg("sensor_s_ctrl ctrl->id=0x%8x\n", ctrl->id);*/
	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (qc.type == V4L2_CTRL_TYPE_MENU ||
			qc.type == V4L2_CTRL_TYPE_INTEGER ||
			qc.type == V4L2_CTRL_TYPE_BOOLEAN){
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
				(enum v4l2_exposure_auto_type) ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd,
				(enum v4l2_auto_n_preset_white_balance) ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd,
				(enum v4l2_colorfx) ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_s_flash_mode(sd,
				(enum v4l2_flash_led_mode) ctrl->value);
	}
	return -EINVAL;
}


static int sensor_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}


/* ----------------------------------------------------------------------- */

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

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_8,
	.data_width = CCI_BITS_8,
};

static int sensor_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	/*	int ret;*/

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

	return 0;
}


static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

/*linux-3.0*/
static struct i2c_driver sensor_driver = {
	.driver = {
		.owner = THIS_MODULE,
	.name = SENSOR_NAME,
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
