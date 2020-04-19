
/*
 * mipi subdev driver module
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include "bsp_mipi_csi.h"
#include "combo_rx/combo_rx_reg.h"
#include "combo_rx/combo_rx_reg_i.h"
#include "sunxi_mipi.h"
#include "../platform/platform_cfg.h"
#define MIPI_MODULE_NAME "vin_mipi"

#define IS_FLAG(x, y) (((x)&(y)) == y)

static LIST_HEAD(mipi_drv_list);

static struct combo_format sunxi_mipi_formats[] = {
	{
		.code = V4L2_MBUS_FMT_SBGGR8_1X8,
		.bit_width = RAW8,
	}, {
		.code = V4L2_MBUS_FMT_SGBRG8_1X8,
		.bit_width = RAW8,
	}, {
		.code = V4L2_MBUS_FMT_SGRBG8_1X8,
		.bit_width = RAW8,
	}, {
		.code = V4L2_MBUS_FMT_SRGGB8_1X8,
		.bit_width = RAW8,
	}, {
		.code = V4L2_MBUS_FMT_SBGGR10_1X10,
		.bit_width = RAW10,
	}, {
		.code = V4L2_MBUS_FMT_SGBRG10_1X10,
		.bit_width = RAW10,
	}, {
		.code = V4L2_MBUS_FMT_SGRBG10_1X10,
		.bit_width = RAW10,
	}, {
		.code = V4L2_MBUS_FMT_SRGGB10_1X10,
		.bit_width = RAW10,
	}, {
		.code = V4L2_MBUS_FMT_SBGGR12_1X12,
		.bit_width = RAW12,
	}, {
		.code = V4L2_MBUS_FMT_SGBRG12_1X12,
		.bit_width = RAW12,
	}, {
		.code = V4L2_MBUS_FMT_SGRBG12_1X12,
		.bit_width = RAW12,
	}, {
		.code = V4L2_MBUS_FMT_SRGGB12_1X12,
		.bit_width = RAW12,
	}
};

void combo_rx_mipi_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct mipi_ctr mipi_ctr;
	struct mipi_lane_map mipi_map;

	memset(&mipi_ctr, 0, sizeof(mipi_ctr));
	mipi_ctr.mipi_lane_num = mipi->cmb_cfg.mipi_ln;
	mipi_ctr.mipi_msb_lsb_sel = 1;

	if (mipi->cmb_mode == MIPI_NORMAL_MODE) {
		mipi_ctr.mipi_wdr_mode_sel = 0;
	} else if (mipi->cmb_mode == MIPI_VC_WDR_MODE) {
		mipi_ctr.mipi_wdr_mode_sel = 0;
		mipi_ctr.mipi_open_multi_ch = 1;
		mipi_ctr.mipi_ch0_height = mipi->format.height;
		mipi_ctr.mipi_ch1_height = mipi->format.height;
		mipi_ctr.mipi_ch2_height = mipi->format.height;
		mipi_ctr.mipi_ch3_height = mipi->format.height;
	} else if (mipi->cmb_mode == MIPI_DOL_WDR_MODE) {
		mipi_ctr.mipi_wdr_mode_sel = 2;
	}

	mipi_map.mipi_lane0 = MIPI_IN_L0_USE_PAD_LANE0;
	mipi_map.mipi_lane1 = MIPI_IN_L1_USE_PAD_LANE1;
	mipi_map.mipi_lane2 = MIPI_IN_L2_USE_PAD_LANE2;
	mipi_map.mipi_lane3 = MIPI_IN_L3_USE_PAD_LANE3;

	cmb_rx_mode_sel(mipi->id, D_PHY);
	cmb_rx_app_pixel_out(mipi->id, TWO_PIXEL);
	cmb_rx_mipi_ctr(mipi->id, &mipi_ctr);
	cmb_rx_mipi_dphy_mapping(mipi->id, &mipi_map);
}

void combo_rx_sub_lvds_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct lvds_ctr lvds_ctr;

	memset(&lvds_ctr, 0, sizeof(lvds_ctr));
	lvds_ctr.lvds_bit_width = mipi->cmb_fmt->bit_width;
	lvds_ctr.lvds_lane_num = mipi->cmb_cfg.lvds_ln;

	if (mipi->cmb_mode == LVDS_NORMAL_MODE) {
		lvds_ctr.lvds_line_code_mode = 1;
	} else if (mipi->cmb_mode == LVDS_4CODE_WDR_MODE) {
		lvds_ctr.lvds_line_code_mode = mipi->wdr_cfg.line_code_mode;

		lvds_ctr.lvds_wdr_lbl_sel = 1;
		lvds_ctr.lvds_sync_code_line_cnt = mipi->wdr_cfg.line_cnt;

		lvds_ctr.lvds_code_mask = mipi->wdr_cfg.code_mask;
		lvds_ctr.lvds_wdr_fid_mode_sel = mipi->wdr_cfg.wdr_fid_mode_sel;
		lvds_ctr.lvds_wdr_fid_map_en = mipi->wdr_cfg.wdr_fid_map_en;
		lvds_ctr.lvds_wdr_fid0_map_sel = mipi->wdr_cfg.wdr_fid0_map_sel;
		lvds_ctr.lvds_wdr_fid1_map_sel = mipi->wdr_cfg.wdr_fid1_map_sel;
		lvds_ctr.lvds_wdr_fid2_map_sel = mipi->wdr_cfg.wdr_fid2_map_sel;
		lvds_ctr.lvds_wdr_fid3_map_sel = mipi->wdr_cfg.wdr_fid3_map_sel;

		lvds_ctr.lvds_wdr_en_multi_ch = mipi->wdr_cfg.wdr_en_multi_ch;
		lvds_ctr.lvds_wdr_ch0_height = mipi->wdr_cfg.wdr_ch0_height;
		lvds_ctr.lvds_wdr_ch1_height = mipi->wdr_cfg.wdr_ch1_height;
		lvds_ctr.lvds_wdr_ch2_height = mipi->wdr_cfg.wdr_ch2_height;
		lvds_ctr.lvds_wdr_ch3_height = mipi->wdr_cfg.wdr_ch3_height;
	} else if (mipi->cmb_mode == LVDS_5CODE_WDR_MODE) {
		lvds_ctr.lvds_line_code_mode = mipi->wdr_cfg.line_code_mode;
		lvds_ctr.lvds_wdr_lbl_sel = 2;
		lvds_ctr.lvds_sync_code_line_cnt = mipi->wdr_cfg.line_cnt;

		lvds_ctr.lvds_code_mask = mipi->wdr_cfg.code_mask;
	}

	cmb_rx_mode_sel(mipi->id, SUB_LVDS);
	cmb_rx_app_pixel_out(mipi->id, ONE_PIXEL);
	cmb_rx_lvds_ctr(mipi->id, &lvds_ctr);
	cmb_rx_lvds_mapping(mipi->id, &mipi->lvds_map);
	cmb_rx_lvds_sync_code(mipi->id, &mipi->sync_code);
}

void combo_rx_hispi_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct lvds_ctr lvds_ctr;
	struct hispi_ctr hispi_ctr;

	memset(&hispi_ctr, 0, sizeof(hispi_ctr));
	memset(&lvds_ctr, 0, sizeof(lvds_ctr));
	lvds_ctr.lvds_bit_width = mipi->cmb_fmt->bit_width;
	lvds_ctr.lvds_lane_num = mipi->cmb_cfg.lvds_ln;

	if (mipi->cmb_mode == HISPI_NORMAL_MODE) {
		lvds_ctr.lvds_line_code_mode = 0;
		lvds_ctr.lvds_pix_lsb = 1;

		hispi_ctr.hispi_normal = 1;
		hispi_ctr.hispi_trans_mode = PACKETIZED_SP;
	} else if (mipi->cmb_mode == HISPI_WDR_MODE) {
		lvds_ctr.lvds_line_code_mode = mipi->wdr_cfg.line_code_mode;

		lvds_ctr.lvds_wdr_lbl_sel = 1;

		lvds_ctr.lvds_pix_lsb = mipi->wdr_cfg.pix_lsb;
		lvds_ctr.lvds_sync_code_line_cnt = mipi->wdr_cfg.line_cnt;

		lvds_ctr.lvds_wdr_fid_mode_sel = mipi->wdr_cfg.wdr_fid_mode_sel;
		lvds_ctr.lvds_wdr_fid_map_en = mipi->wdr_cfg.wdr_fid_map_en;
		lvds_ctr.lvds_wdr_fid0_map_sel = mipi->wdr_cfg.wdr_fid0_map_sel;
		lvds_ctr.lvds_wdr_fid1_map_sel = mipi->wdr_cfg.wdr_fid1_map_sel;
		lvds_ctr.lvds_wdr_fid2_map_sel = mipi->wdr_cfg.wdr_fid2_map_sel;
		lvds_ctr.lvds_wdr_fid3_map_sel = mipi->wdr_cfg.wdr_fid3_map_sel;

		hispi_ctr.hispi_normal = 1;
		hispi_ctr.hispi_trans_mode = PACKETIZED_SP;
		hispi_ctr.hispi_wdr_en = 1;
		hispi_ctr.hispi_wdr_sof_fild = mipi->wdr_cfg.wdr_sof_fild;
		hispi_ctr.hispi_wdr_eof_fild = mipi->wdr_cfg.wdr_eof_fild;
		hispi_ctr.hispi_code_mask = mipi->wdr_cfg.code_mask;
	}

	cmb_rx_mode_sel(mipi->id, SUB_LVDS);
	cmb_rx_app_pixel_out(mipi->id, ONE_PIXEL);
	cmb_rx_lvds_ctr(mipi->id, &lvds_ctr);
	cmb_rx_lvds_mapping(mipi->id, &mipi->lvds_map);
	cmb_rx_lvds_sync_code(mipi->id, &mipi->sync_code);

	cmb_rx_hispi_ctr(mipi->id, &hispi_ctr);
}

void combo_rx_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	/*comnbo rx phya init*/
	cmb_rx_phya_config(mipi->id);

	if (mipi->terminal_resistance) {
		vin_log(VIN_LOG_MIPI, "open combo terminal resitance!\n");
		cmb_rx_phya_a_d0_en(mipi->id, 1);
		cmb_rx_phya_b_d0_en(mipi->id, 1);
		cmb_rx_phya_c_d0_en(mipi->id, 1);
		cmb_rx_phya_a_d1_en(mipi->id, 1);
		cmb_rx_phya_b_d1_en(mipi->id, 1);
		cmb_rx_phya_c_d1_en(mipi->id, 1);
		cmb_rx_phya_a_d2_en(mipi->id, 1);
		cmb_rx_phya_b_d2_en(mipi->id, 1);
		cmb_rx_phya_c_d2_en(mipi->id, 1);
		cmb_rx_phya_a_d3_en(mipi->id, 1);
		cmb_rx_phya_b_d3_en(mipi->id, 1);
		cmb_rx_phya_c_d3_en(mipi->id, 1);
		cmb_rx_phya_a_ck_en(mipi->id, 1);
		cmb_rx_phya_b_ck_en(mipi->id, 1);
		cmb_rx_phya_c_ck_en(mipi->id, 1);
	}
	cmb_rx_phya_singal_dly_en(mipi->id, 0);
	cmb_rx_phya_offset(mipi->id, mipi->pyha_offset);

	switch (mipi->if_type) {
	case V4L2_MBUS_PARALLEL:
		cmb_rx_mode_sel(mipi->id, CMOS);
		cmb_rx_app_pixel_out(mipi->id, ONE_PIXEL);
		break;
	case V4L2_MBUS_CSI2:
		cmb_rx_phya_ck_mode(mipi->id, 0);
		combo_rx_mipi_init(sd);
		break;
	case V4L2_MBUS_SUBLVDS:
		cmb_rx_phya_ck_mode(mipi->id, 1);
		combo_rx_sub_lvds_init(sd);
		break;
	case V4L2_MBUS_HISPI:
		cmb_rx_phya_ck_mode(mipi->id, 0);
		combo_rx_hispi_init(sd);
		break;
	default:
		combo_rx_mipi_init(sd);
		break;
	}

	cmb_rx_enable(mipi->id);
}

static enum pkt_fmt get_pkt_fmt(u32 code)
{
	switch (code) {
	case V4L2_MBUS_FMT_RGB565_2X8_BE:
		return MIPI_RGB565;
	case V4L2_MBUS_FMT_UYVY8_1X16:
		return MIPI_YUV422;
	case V4L2_MBUS_FMT_YUYV10_2X10:
		return MIPI_YUV422_10;
	case V4L2_MBUS_FMT_SBGGR8_1X8:
	case V4L2_MBUS_FMT_SGBRG8_1X8:
	case V4L2_MBUS_FMT_SGRBG8_1X8:
	case V4L2_MBUS_FMT_SRGGB8_1X8:
		return MIPI_RAW8;
	case V4L2_MBUS_FMT_SBGGR10_1X10:
	case V4L2_MBUS_FMT_SGBRG10_1X10:
	case V4L2_MBUS_FMT_SGRBG10_1X10:
	case V4L2_MBUS_FMT_SRGGB10_1X10:
		return MIPI_RAW10;
	case V4L2_MBUS_FMT_SBGGR12_1X12:
	case V4L2_MBUS_FMT_SGBRG12_1X12:
	case V4L2_MBUS_FMT_SGRBG12_1X12:
	case V4L2_MBUS_FMT_SRGGB12_1X12:
		return MIPI_RAW12;
	default:
		return MIPI_RAW8;
	}
}

static int sunxi_mipi_subdev_s_power(struct v4l2_subdev *sd, int enable)
{
	return 0;
}
static int sunxi_mipi_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &mipi->format;
	struct mbus_framefmt_res *res = (void *)mf->reserved;

	int i;

	mipi->csi2_cfg.bps = res->res_mipi_bps;
	mipi->csi2_cfg.auto_check_bps = 0;
	mipi->csi2_cfg.dphy_freq = DPHY_CLK;

	for (i = 0; i < mipi->csi2_cfg.total_rx_ch; i++) {
		mipi->csi2_fmt.packet_fmt[i] = get_pkt_fmt(mf->code);
		mipi->csi2_fmt.field[i] = mf->field;
		mipi->csi2_fmt.vc[i] = i;
	}

	mipi->cmb_mode = res->res_combo_mode & 0xf;
	mipi->terminal_resistance = res->res_combo_mode & CMB_TERMINAL_RES;
	mipi->pyha_offset = (res->res_combo_mode & 0x70) >> 4;

	if (enable) {
		combo_rx_init(sd);
		bsp_mipi_csi_dphy_init(mipi->id);
		bsp_mipi_csi_set_para(mipi->id, &mipi->csi2_cfg);
		bsp_mipi_csi_set_fmt(mipi->id, mipi->csi2_cfg.total_rx_ch,
				     &mipi->csi2_fmt);

		/*for dphy clock async*/
		bsp_mipi_csi_dphy_disable(mipi->id);
		bsp_mipi_csi_dphy_enable(mipi->id);
		bsp_mipi_csi_protocol_enable(mipi->id);
	} else {
		cmb_rx_disable(mipi->id);
		bsp_mipi_csi_dphy_disable(mipi->id);
		bsp_mipi_csi_protocol_disable(mipi->id);
		bsp_mipi_csi_dphy_exit(mipi->id);
	}

	vin_log(VIN_LOG_FMT, "%s%d %s, lane_num %d, code: %x field: %d\n",
		mipi->if_name, mipi->id, enable ? "stream on" : "stream off",
		mipi->cmb_cfg.lane_num, mf->code, mf->field);

	return 0;
}

static int sunxi_mipi_enum_mbus_code(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_mbus_code_enum *code)
{
	return 0;
}

static struct v4l2_mbus_framefmt *__mipi_get_format(
		struct mipi_dev *mipi, struct v4l2_subdev_fh *fh,
		enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return fh ? v4l2_subdev_get_try_format(fh, 0) : NULL;

	return &mipi->format;
}

static int sunxi_mipi_subdev_get_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = __mipi_get_format(mipi, fh, fmt->which);
	if (!mf)
		return -EINVAL;

	mutex_lock(&mipi->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&mipi->subdev_lock);
	return 0;
}

static struct combo_format *__mipi_find_format(
	struct v4l2_mbus_framefmt *mf)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sunxi_mipi_formats); i++)
		if (mf->code == sunxi_mipi_formats[i].code)
			return &sunxi_mipi_formats[i];
	return NULL;
}

static struct combo_format *__mipi_try_format(struct v4l2_mbus_framefmt *mf)
{
	struct combo_format *mipi_fmt;

	mipi_fmt = __mipi_find_format(mf);
	if (mipi_fmt == NULL)
		mipi_fmt = &sunxi_mipi_formats[0];

	mf->code = mipi_fmt->code;

	return mipi_fmt;
}

static int sunxi_mipi_subdev_set_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_fh *fh,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct combo_format *mipi_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = __mipi_get_format(mipi, fh, fmt->which);

	if (fmt->pad == MIPI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&mipi->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&mipi->subdev_lock);
		}
		return 0;
	}

	mipi_fmt = __mipi_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&mipi->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			mipi->cmb_fmt = mipi_fmt;
		mutex_unlock(&mipi->subdev_lock);
	}

	return 0;
}

int sunxi_mipi_subdev_init(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sunxi_mipi_s_mbus_config(struct v4l2_subdev *sd,
				    const struct v4l2_mbus_config *cfg)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2) {
		mipi->if_type = V4L2_MBUS_CSI2;
		memcpy(mipi->if_name, "mipi", sizeof("mipi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE)) {
			mipi->csi2_cfg.lane_num = 4;
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.mipi_ln = MIPI_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE)) {
			mipi->csi2_cfg.lane_num = 3;
			mipi->cmb_cfg.lane_num = 3;
			mipi->cmb_cfg.mipi_ln = MIPI_3LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE)) {
			mipi->csi2_cfg.lane_num = 2;
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.mipi_ln = MIPI_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 1;
			mipi->csi2_cfg.lane_num = 1;
			mipi->cmb_cfg.mipi_ln = MIPI_1LANE;
		}
	} else if (cfg->type == V4L2_MBUS_SUBLVDS) {
		mipi->if_type = V4L2_MBUS_SUBLVDS;
		memcpy(mipi->if_name, "sublvds", sizeof("sublvds"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		}
	} else if (cfg->type == V4L2_MBUS_HISPI) {
		mipi->if_type = V4L2_MBUS_HISPI;
		memcpy(mipi->if_name, "hispi", sizeof("hispi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		}
	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
		memcpy(mipi->if_name, "combo_parallel", sizeof("combo_parallel"));
		mipi->if_type = V4L2_MBUS_PARALLEL;
	}

	mipi->csi2_cfg.total_rx_ch = 0;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0))
		mipi->csi2_cfg.total_rx_ch++;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1))
		mipi->csi2_cfg.total_rx_ch++;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2))
		mipi->csi2_cfg.total_rx_ch++;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3))
		mipi->csi2_cfg.total_rx_ch++;

	return 0;
}

static const struct v4l2_subdev_core_ops sunxi_mipi_core_ops = {
	.s_power = sunxi_mipi_subdev_s_power,
	.init = sunxi_mipi_subdev_init,
};

static const struct v4l2_subdev_video_ops sunxi_mipi_subdev_video_ops = {
	.s_stream = sunxi_mipi_subdev_s_stream,
	.s_mbus_config = sunxi_mipi_s_mbus_config,
};

static const struct v4l2_subdev_pad_ops sunxi_mipi_subdev_pad_ops = {
	.enum_mbus_code = sunxi_mipi_enum_mbus_code,
	.get_fmt = sunxi_mipi_subdev_get_fmt,
	.set_fmt = sunxi_mipi_subdev_set_fmt,
};

static struct v4l2_subdev_ops sunxi_mipi_subdev_ops = {
	.core = &sunxi_mipi_core_ops,
	.video = &sunxi_mipi_subdev_video_ops,
	.pad = &sunxi_mipi_subdev_pad_ops,
};

static int __mipi_init_subdev(struct mipi_dev *mipi)
{
	struct v4l2_subdev *sd = &mipi->subdev;
	int ret;

	mutex_init(&mipi->subdev_lock);
	v4l2_subdev_init(sd, &sunxi_mipi_subdev_ops);
	sd->grp_id = VIN_GRP_ID_MIPI;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_mipi.%u", mipi->id);
	v4l2_set_subdevdata(sd, mipi);

	/*sd->entity->ops = &isp_media_ops;*/
	mipi->mipi_pads[MIPI_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	mipi->mipi_pads[MIPI_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV;

	ret = media_entity_init(&sd->entity, MIPI_PAD_NUM, mipi->mipi_pads, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int mipi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mipi_dev *mipi = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("MIPI failed to get of node\n");
		return -ENODEV;
	}

	mipi = kzalloc(sizeof(struct mipi_dev), GFP_KERNEL);
	if (!mipi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("MIPI failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	mipi->base = of_iomap(np, 0);
	if (!mipi->base) {
		ret = -EIO;
		goto freedev;
	}
	mipi->id = pdev->id;
	mipi->pdev = pdev;

	spin_lock_init(&mipi->slock);

	cmb_rx_set_base_addr(mipi->id, (unsigned long)mipi->base);

	bsp_mipi_csi_set_base_addr(mipi->id, (unsigned long)mipi->base);
	bsp_mipi_dphy_set_base_addr(mipi->id, (unsigned long)mipi->base + 0x1000);

	list_add_tail(&mipi->mipi_list, &mipi_drv_list);

	ret = __mipi_init_subdev(mipi);
	if (ret < 0) {
		vin_err("mipi init error!\n");
		goto unmap;
	}

	platform_set_drvdata(pdev, mipi);
	vin_log(VIN_LOG_MIPI, "mipi%d probe end!\n", mipi->id);
	return 0;

unmap:
	iounmap(mipi->base);
freedev:
	kfree(mipi);
ekzalloc:
	vin_err("mipi probe err!\n");
	return ret;
}

static int mipi_remove(struct platform_device *pdev)
{
	struct mipi_dev *mipi = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &mipi->subdev;
	platform_set_drvdata(pdev, NULL);
	v4l2_set_subdevdata(sd, NULL);
	if (mipi->base)
		iounmap(mipi->base);
	list_del(&mipi->mipi_list);
	kfree(mipi);
	return 0;
}

static const struct of_device_id sunxi_mipi_match[] = {
	{.compatible = "allwinner,sunxi-mipi",},
	{},
};

static struct platform_driver mipi_platform_driver = {
	.probe = mipi_probe,
	.remove = mipi_remove,
	.driver = {
		.name = MIPI_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_mipi_match,
	}
};

void sunxi_combo_set_sync_code(struct v4l2_subdev *sd,
		struct combo_sync_code *sync)
{
	struct mipi_dev *combo = v4l2_get_subdevdata(sd);

	memset(&combo->sync_code, 0, sizeof(combo->sync_code));
	combo->sync_code = *sync;
}

void sunxi_combo_set_lane_map(struct v4l2_subdev *sd,
		struct combo_lane_map *map)
{
	struct mipi_dev *combo = v4l2_get_subdevdata(sd);

	memset(&combo->lvds_map, 0, sizeof(combo->lvds_map));
	combo->lvds_map = *map;
}

void sunxi_combo_wdr_config(struct v4l2_subdev *sd,
		struct combo_wdr_cfg *wdr)
{
	struct mipi_dev *combo = v4l2_get_subdevdata(sd);

	memset(&combo->wdr_cfg, 0, sizeof(combo->wdr_cfg));
	combo->wdr_cfg = *wdr;
}

struct v4l2_subdev *sunxi_mipi_get_subdev(int id)
{
	struct mipi_dev *mipi;
	list_for_each_entry(mipi, &mipi_drv_list, mipi_list) {
		if (mipi->id == id) {
			mipi->use_cnt++;
			return &mipi->subdev;
		}
	}
	return NULL;
}

int sunxi_mipi_platform_register(void)
{
	return platform_driver_register(&mipi_platform_driver);
}

void sunxi_mipi_platform_unregister(void)
{
	platform_driver_unregister(&mipi_platform_driver);
	vin_log(VIN_LOG_MIPI, "mipi_exit end\n");
}
