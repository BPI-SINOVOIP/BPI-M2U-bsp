
/*
  * sensor_helper.h: helper function for sensors.
  *
  * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
  *
  * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  */

#ifndef __SENSOR__HELPER__H__
#define __SENSOR__HELPER__H__

#include <media/v4l2-subdev.h>
#include <linux/videodev2.h>
#include "../../vin-video/vin_core.h"
#include "../../utility/vin_supply.h"
#include "../../vin-cci/cci_helper.h"
#include "camera_cfg.h"
#include "camera.h"
#include "../../platform/platform_cfg.h"

#define REG_DLY  0xffff

struct regval_list {
	addr_type addr;
	data_type data;
};

#define DEV_DBG_EN   		0
#if (DEV_DBG_EN == 1)
#define sensor_dbg(x, arg...) printk(KERN_DEBUG "[%s]"x, SENSOR_NAME, ##arg)
#else
#define sensor_dbg(x, arg...)
#endif

#define sensor_err(x, arg...) printk(KERN_ERR "[%s] error, "x, SENSOR_NAME, ##arg)
#define sensor_print(x, arg...) printk(KERN_NOTICE "[%s]"x, SENSOR_NAME, ##arg)

extern int sensor_read(struct v4l2_subdev *sd, addr_type addr,
		       data_type *value);
extern int sensor_write(struct v4l2_subdev *sd, addr_type addr,
			data_type value);
extern int sensor_write_array(struct v4l2_subdev *sd,
				struct regval_list *regs,
				int array_size);
#ifdef CONFIG_COMPAT
extern long sensor_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg);
#endif
extern struct sensor_info *to_state(struct v4l2_subdev *sd);
extern void sensor_cfg_req(struct v4l2_subdev *sd, struct sensor_config *cfg);
extern void sensor_isp_input(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf);

extern int sensor_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_fh *fh,
				struct v4l2_subdev_mbus_code_enum *code);
extern int sensor_enum_frame_size(struct v4l2_subdev *sd,
			struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_frame_size_enum *fse);
extern int sensor_get_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_format *fmt);
extern int sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_fh *fh,
			struct v4l2_subdev_format *fmt);
extern int sensor_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms);
extern int sensor_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms);

#endif
