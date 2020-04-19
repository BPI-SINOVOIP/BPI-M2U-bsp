
/*
 * configs for cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CAMERA_CFG_H__
#define __CAMERA_CFG_H__

enum sensor_pad {
	/*SENSOR_PAD_SINK,*/
	SENSOR_PAD_SOURCE,
	SENSOR_PAD_NUM,
};

typedef enum tag_CAMERA_IO_CMD {
	GET_CURRENT_WIN_CFG,
	SET_FPS,
	SET_FLASH_CTRL,
	ISP_SET_EXP_GAIN,
	GET_SENSOR_EXIF,
	SET_AUTO_FOCUS_WIN,
	SET_AUTO_EXPOSURE_WIN,
	GET_FLASH_FLAG,
	GET_COMBO_SYNC_CODE,
	GET_COMBO_LANE_MAP,
	GET_COMBO_WDR_CFG,
} __camera_cmd_t;

struct sensor_exif_attribute {
	__u32 exposure_time_num;
	__u32 exposure_time_den;
	__u32 fnumber;
	__u32 focal_length;
	__u32 iso_speed;
	__u32 flash_fire;
	__u32 brightness;
};

struct sensor_win_size {
	/*sensor output w = width + 2*hoffset, h = height + 2*voffset */
	unsigned int width;	/*csi parser crop width */
	unsigned int height;	/*csi parser crop height */
	unsigned int hoffset;	/*csi parser crop hoffset*/
	unsigned int voffset;	/*csi parser crop voffset*/
	unsigned int hts;	/*h size of timing, unit: pclk      */
	unsigned int vts;	/*v size of timing, unit: line      */
	unsigned int pclk;	/*pixel clock in Hz                 */
	unsigned int mipi_bps;	/*mipi clock in bps, config for mipi*/
	unsigned int fps_fixed;	/*fps mode 1=fixed fps              */
	unsigned int if_mode;	/*interface mode, normal = 0, wdr != 0  */
	unsigned int wdr_mode; /*wdr mode, normal = 0, dol = 1, command = 2  */
	unsigned int bin_factor;/*binning factor                    */
	unsigned int intg_min;	/*integration min, unit: line, Q4   */
	unsigned int intg_max;	/*integration max, unit: line, Q4   */
	unsigned int gain_min;	/*sensor gain min, Q4               */
	unsigned int gain_max;	/*sensor gain max, Q4               */
	unsigned int width_input; /*isp width input, after isp crop*/
	unsigned int height_input; /*isp height input, after isp crop*/
	unsigned int vipp_hoff;	/*vipp crop hoffset */
	unsigned int vipp_voff;	/*vipp crop voffset */

	void *regs;		/* Regs to tweak */
	int regs_size;
	int (*set_size) (struct v4l2_subdev *sd);
};

#endif /*__CAMERA_CFG_H__*/
