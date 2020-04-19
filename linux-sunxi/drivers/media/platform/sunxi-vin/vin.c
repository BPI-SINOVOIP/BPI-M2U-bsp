
/*
 * vin.c for all v4l2 subdev manage
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/freezer.h>

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>

#include <linux/regulator/consumer.h>

#include "utility/bsp_common.h"
#include "vin-cci/bsp_cci.h"
#include "vin-cci/cci_helper.h"
#include "utility/config.h"
#include "modules/sensor/camera_cfg.h"
#include "utility/sensor_info.h"
#include "utility/vin_io.h"
#include "top_reg.h"
#include "vin.h"

#define VIN_MODULE_NAME "sunxi-vin-media"

char ccm[I2C_NAME_SIZE] = "";
uint i2c_addr = 0xff;

char act_name[I2C_NAME_SIZE] = "";
uint act_slave = 0xff;
uint use_sensor_list = 0xff;
uint vin_i2c_dbg;

module_param_string(ccm, ccm, sizeof(ccm), S_IRUGO | S_IWUSR);
module_param(i2c_addr, uint, S_IRUGO | S_IWUSR);

module_param_string(act_name, act_name, sizeof(act_name), S_IRUGO | S_IWUSR);
module_param(act_slave, uint, S_IRUGO | S_IWUSR);
module_param(use_sensor_list, uint, S_IRUGO | S_IWUSR);
module_param(vin_i2c_dbg, uint, S_IRUGO | S_IWUSR);

static void vin_md_prepare_pipeline(struct vin_pipeline *p,
				  struct media_entity *me)
{
	struct v4l2_subdev *sd;
	int i;

	for (i = 0; i < VIN_IND_ACTUATOR; i++)
		p->sd[i] = NULL;

	while (1) {
		struct media_pad *pad = NULL;

		/* Find remote source pad */
		for (i = 0; i < me->num_pads; i++) {
			struct media_pad *spad = &me->pads[i];
			if (!(spad->flags & MEDIA_PAD_FL_SINK))
				continue;
			pad = media_entity_remote_source(spad);
			if (pad)
				break;
		}

		if (pad == NULL ||
		    media_entity_type(pad->entity) != MEDIA_ENT_T_V4L2_SUBDEV)
			break;

		sd = media_entity_to_v4l2_subdev(pad->entity);
		vin_log(VIN_LOG_MD, "%s entity is %s, group id is 0x%x\n",
			__func__, pad->entity->name, sd->grp_id);

		switch (sd->grp_id) {
		case VIN_GRP_ID_SENSOR:
			p->sd[VIN_IND_SENSOR] = sd;
			break;
		case VIN_GRP_ID_MIPI:
			p->sd[VIN_IND_MIPI] = sd;
			break;
		case VIN_GRP_ID_CSI:
			p->sd[VIN_IND_CSI] = sd;
			break;
		case VIN_GRP_ID_ISP:
			p->sd[VIN_IND_ISP] = sd;
			break;
		case VIN_GRP_ID_SCALER:
			p->sd[VIN_IND_SCALER] = sd;
			break;
		case VIN_GRP_ID_CAPTURE:
			p->sd[VIN_IND_CAPTURE] = sd;
			break;
		default:
			break;
		}
		me = &sd->entity;
		if (me->num_pads == 1)
			break;
	}
}

static int vin_mclk_pin_release(struct vin_md *vind)
{
#ifndef FPGA_VER
	int i;

	for (i = 0; i < VIN_MAX_CCI; i++) {
		if (!IS_ERR_OR_NULL(vind->mclk[i].pin))
			devm_pinctrl_put(vind->mclk[i].pin);
	}
#endif
	return 0;
}

static int vin_md_get_clocks(struct vin_md *vind)
{
#ifndef FPGA_VER
	struct device_node *np = vind->pdev->dev.of_node;
	unsigned int core_clk;
	int i;

	vind->clk[VIN_TOP_CLK].clock = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(vind->clk[VIN_TOP_CLK].clock)) {
		vin_err("Get top clk failed!\n");
		vind->clk[VIN_TOP_CLK].clock = NULL;
		return 0;
	}

	vind->clk[VIN_TOP_CLK_SRC].clock = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(vind->clk[VIN_TOP_CLK_SRC].clock)) {
		vin_err("Get top clk source failed!\n");
		vind->clk[VIN_TOP_CLK_SRC].clock = NULL;
		return 0;
	}

	for (i = 0; i < VIN_MAX_CCI; i++) {
		vind->mclk[i].mclk = of_clk_get(np, 3 * i + 2);
		if (IS_ERR_OR_NULL(vind->mclk[i].mclk)) {
			vin_err("Get mclk%d failed!\n", i);
			vind->mclk[i].mclk = NULL;
			break;
		}
		vind->mclk[i].clk_24m = of_clk_get(np, 3 * i + 3);
		if (IS_ERR_OR_NULL(vind->mclk[i].clk_24m)) {
			vin_err("Get mclk%d_24m failed!\n", i);
			vind->mclk[i].clk_24m = NULL;
			break;
		}
		vind->mclk[i].clk_pll = of_clk_get(np, 3 * i + 4);
		if (IS_ERR_OR_NULL(vind->mclk[i].clk_pll)) {
			vin_err("Get mclk%d_pll failed!\n", i);
			vind->mclk[i].clk_pll = NULL;
			break;
		}
	}

	vind->isp_clk.clock = of_clk_get(np, 3 * i + 2);
	if (IS_ERR_OR_NULL(vind->isp_clk.clock)) {
		vin_warn("Get isp clk failed!\n");
		vind->isp_clk.clock = NULL;
	}

	vind->mipi_clk[VIN_MIPI_CLK].clock = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(vind->mipi_clk[VIN_MIPI_CLK].clock)) {
		vin_warn("Get mipi clk failed!\n");
		vind->mipi_clk[VIN_MIPI_CLK].clock = NULL;
	}

	vind->mipi_clk[VIN_MIPI_CLK_SRC].clock = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(vind->mipi_clk[VIN_MIPI_CLK_SRC].clock)) {
		vin_warn("Get mipi clk source failed!\n");
		vind->mipi_clk[VIN_MIPI_CLK_SRC].clock = NULL;
	}

	if (vind->mipi_clk[VIN_MIPI_CLK].clock &&
	    vind->mipi_clk[VIN_MIPI_CLK_SRC].clock) {
		if (clk_set_parent(vind->mipi_clk[VIN_MIPI_CLK].clock,
		    vind->mipi_clk[VIN_MIPI_CLK_SRC].clock)) {
			vin_err("set vin mipi clock source failed\n");
			return -1;
		}
		if (clk_set_rate(vind->mipi_clk[VIN_MIPI_CLK].clock, DPHY_CLK)) {
			vin_err("set mipi clock rate error\n");
			return -1;
		}
	}

	if (clk_set_parent(vind->clk[VIN_TOP_CLK].clock, vind->clk[VIN_TOP_CLK_SRC].clock)) {
		vin_err("vin top clock set parent failed \n");
		return -1;
	}

	if (of_property_read_u32(np, "vind0_clk", &core_clk)) {
		vin_err("vin failed to get core clk\n");
		vind->clk[VIN_TOP_CLK].frequency = VIN_CLK_RATE;
	} else {
		vin_log(VIN_LOG_MD, "vin get core clk = %d\n", core_clk);
		vind->clk[VIN_TOP_CLK].frequency = core_clk;
	}

#endif
	return 0;
}

static void vin_md_put_clocks(struct vin_md *vind)
{
#ifndef FPGA_VER
	int i;
	for (i = 0; i < VIN_MAX_CLK; i++) {
		if (vind->clk[i].clock)
			clk_put(vind->clk[i].clock);
	}

	for (i = 0; i < VIN_MAX_CCI; i++) {
		if (vind->mclk[i].mclk)
			clk_put(vind->mclk[i].mclk);
		if (vind->mclk[i].clk_24m)
			clk_put(vind->mclk[i].clk_24m);
		if (vind->mclk[i].clk_pll)
			clk_put(vind->mclk[i].clk_pll);
	}

	if (vind->isp_clk.clock)
		clk_put(vind->isp_clk.clock);

	for (i = 0; i < VIN_MIPI_MAX_CLK; i++) {
		if (vind->mipi_clk[i].clock)
			clk_put(vind->mipi_clk[i].clock);
	}
#endif
}

static int __vin_set_top_clk_rate(struct vin_md *vind, unsigned int rate)
{
	if (rate >= 300)
		vind->clk[VIN_TOP_CLK_SRC].frequency = rate;
	else if (rate >= 150)
		vind->clk[VIN_TOP_CLK_SRC].frequency = rate * 2;
	else if (rate >= 75)
		vind->clk[VIN_TOP_CLK_SRC].frequency = rate * 4;
	else
		vind->clk[VIN_TOP_CLK_SRC].frequency = VIN_CLK_RATE;

	if (clk_set_rate(vind->clk[VIN_TOP_CLK_SRC].clock,
	    vind->clk[VIN_TOP_CLK_SRC].frequency)) {
		vin_err("set vin top clock source rate error\n");
		return -1;
	}

	if (clk_set_rate(vind->clk[VIN_TOP_CLK].clock, rate)) {
		vin_err("set vin top clock rate error\n");
		return -1;
	}
	vin_log(VIN_LOG_MD, "vin top clk get rate = %ld\n",
		clk_get_rate(vind->clk[VIN_TOP_CLK].clock));

	return 0;
}

static int vin_md_clk_enable(struct vin_md *vind)
{
#ifndef FPGA_VER
	if (vind->clk[VIN_TOP_CLK].clock) {
		__vin_set_top_clk_rate(vind, vind->clk[VIN_TOP_CLK].frequency);
		clk_prepare_enable(vind->clk[VIN_TOP_CLK].clock);
		sunxi_periph_reset_deassert(vind->clk[VIN_TOP_CLK].clock);
	}

	if (vind->isp_clk.clock) {
		clk_prepare_enable(vind->isp_clk.clock);
		sunxi_periph_reset_deassert(vind->isp_clk.clock);
	}

	if (vind->mipi_clk[VIN_MIPI_CLK].clock)
		clk_prepare_enable(vind->mipi_clk[VIN_MIPI_CLK].clock);
#else
	void __iomem *clk_base;
	void __iomem *gpio_base;

	vin_log(VIN_LOG_MD, "directly write pin and clk config @ FPGA\n");

	clk_base = ioremap(0x03001000, 0xf00);
	if (!clk_base) {
		vin_print("clk base ioremap failed\n");
		return -EIO;
	}
	writel(0xffffffff, (clk_base + 0x804));/*mbus gating*/
	writel(0xffffffff, (clk_base + 0xc2c));/*csi gating reset*/
	writel(0xffffffff, (clk_base + 0xc00));/*misc clk*/
	writel(0x80000000, (clk_base + 0xc04));/*top clk*/
	writel(0x80000000, (clk_base + 0xc08));/*master0 clk*/
	writel(0x80000000, (clk_base + 0xc0c));/*master1 clk*/
	writel(0x80000000, (clk_base + 0xc10));/*master2 clk*/
	writel(0x80000000, (clk_base + 0xc14));/*master3 clk*/

	gpio_base = ioremap(0x0300b000, 0x120);
	if (!gpio_base) {
		vin_print("gpio base ioremap failed\n");
		return -EIO;
	}
	writel(0x00330000, (gpio_base + 0x24));/*PB SDA SCL*/
	writel(0x33333333, (gpio_base + 0x90));/*PE CSI DATA*/
	writel(0x33333333, (gpio_base + 0x94));
	writel(0x33333333, (gpio_base + 0x98));
#endif
	return 0;
}

static void vin_md_clk_disable(struct vin_md *vind)
{
#ifndef FPGA_VER
	if (vind->clk[VIN_TOP_CLK].clock) {
		clk_disable_unprepare(vind->clk[VIN_TOP_CLK].clock);
		sunxi_periph_reset_assert(vind->clk[VIN_TOP_CLK].clock);
	}

	if (vind->isp_clk.clock) {
		clk_disable_unprepare(vind->isp_clk.clock);
		sunxi_periph_reset_assert(vind->isp_clk.clock);
	}

	if (vind->mipi_clk[VIN_MIPI_CLK].clock)
		clk_disable_unprepare(vind->mipi_clk[VIN_MIPI_CLK].clock);
#endif
}

static void vin_md_set_power(struct vin_md *vind, int on)
{
	if (on && (vind->use_count)++ > 0)
		return;
	else if (!on && (vind->use_count == 0 || --(vind->use_count) > 0))
		return;

	if (on) {
		vin_md_clk_enable(vind);
		usleep_range(100, 120);
		csic_top_enable(vind->id);
	} else {
		csic_top_disable(vind->id);
		vin_md_clk_disable(vind);
	}
}

static int vin_gpio_request(struct vin_md *vind)
{
#ifndef FPGA_VER
	unsigned int i, num;
	struct gpio_config *gc = NULL;
	struct sensor_list *sl = NULL;

	for (num = 0; num < VIN_MAX_DEV; num++) {
		sl = &vind->modules[num].sensors;

		for (i = 0; i < MAX_GPIO_NUM; i++) {
			gc = &sl->gpio[i];
			if (gc == NULL || gc->gpio == GPIO_INDEX_INVALID)
				continue;

			if (gpio_request(gc->gpio, NULL) < 0) {
				vin_log(VIN_LOG_MD, "gpio%d request failed!\n", gc->gpio);
				continue;
			}
			vin_log(VIN_LOG_MD, "gpio%d request success!\n", gc->gpio);
		}
	}
#endif
	return 0;
}

static void vin_gpio_release(struct vin_md *vind)
{
#ifndef FPGA_VER
	unsigned int i, num;
	struct gpio_config *gc = NULL;
	struct sensor_list *sl = NULL;

	for (num = 0; num < VIN_MAX_DEV; num++) {
		sl = &vind->modules[num].sensors;

		for (i = 0; i < MAX_GPIO_NUM; i++) {
			gc = &sl->gpio[i];
			if (gc->gpio != GPIO_INDEX_INVALID)
				gpio_free(gc->gpio);
		}
	}
#endif
}

static int __vin_subdev_set_power(struct v4l2_subdev *sd, int on)
{
	int *use_count;
	int ret;

	if (sd == NULL)
		return -ENXIO;

	use_count = &sd->entity.use_count;
	if (on && (*use_count)++ > 0)
		return 0;
	else if (!on && (*use_count == 0 || --(*use_count) > 0))
		return 0;
	ret = v4l2_subdev_call(sd, core, s_power, on);
#if 0
	if (ret == 0 || ret == -ENOIOCTLCMD) {
		if (on || sd->grp_id != VIN_GRP_ID_SENSOR)
			ret = v4l2_subdev_call(sd, core, init, on);
	}
#endif
	return ret != -ENOIOCTLCMD ? ret : 0;
}

static int vin_pipeline_s_power(struct vin_pipeline *p, bool on)
{
	static const u8 seq[2][VIN_IND_MAX] = {
		{ VIN_IND_ISP, VIN_IND_SENSOR, VIN_IND_CSI, VIN_IND_MIPI,
			VIN_IND_SCALER, VIN_IND_CAPTURE  },
		{ VIN_IND_CAPTURE, VIN_IND_MIPI, VIN_IND_CSI, VIN_IND_SENSOR,
			VIN_IND_ISP, VIN_IND_SCALER},
	};
	int i, ret = 0;

	if (p->sd[VIN_IND_SENSOR] == NULL)
		return -ENXIO;

	for (i = 0; i < VIN_IND_MAX; i++) {
		unsigned int idx = seq[on][i];
		if (NULL == p->sd[idx])
			continue;
		mutex_lock(&p->sd[idx]->entity.parent->graph_mutex);
		ret = __vin_subdev_set_power(p->sd[idx], on);
		mutex_unlock(&p->sd[idx]->entity.parent->graph_mutex);
		if (ret < 0 && ret != -ENXIO)
			goto error;
	}
	return 0;
error:
	for (; i >= 0; i--) {
		unsigned int idx = seq[on][i];
		if (NULL == p->sd[idx])
			continue;
		mutex_lock(&p->sd[idx]->entity.parent->graph_mutex);
		__vin_subdev_set_power(p->sd[idx], !on);
		mutex_unlock(&p->sd[idx]->entity.parent->graph_mutex);
	}
	return ret;
}

static int __vin_pipeline_open(struct vin_pipeline *p,
				struct media_entity *me, bool prepare)
{
	struct vin_md *vind;
	struct v4l2_subdev *sd;
	struct sensor_item sensor;
	int ret;

	if (WARN_ON(p == NULL || me == NULL))
		return -EINVAL;

	if (prepare)
		vin_md_prepare_pipeline(p, me);

	sd = p->sd[VIN_IND_SENSOR];
	if (sd == NULL)
		return -EINVAL;

	vind = entity_to_vin_mdev(me);
	if (vind) {
		/*set vin core clk rate for each sensor!*/
		if (get_sensor_info(sd->name, &sensor) == 0)
			vind->clk[VIN_TOP_CLK].frequency = sensor.core_clk;
		vin_md_set_power(vind, 1);
	}

	ret = vin_pipeline_s_power(p, 1);
	if (!ret)
		return 0;
	return ret;
}

static int __vin_pipeline_close(struct vin_pipeline *p)
{
	struct v4l2_subdev *sd = p ? p->sd[VIN_IND_SENSOR] : NULL;
	struct vin_md *vind;
	int ret = 0;

	if (WARN_ON(sd == NULL))
		return -EINVAL;

	if (p->sd[VIN_IND_SENSOR]) {
		ret = vin_pipeline_s_power(p, 0);
	}

	vind = entity_to_vin_mdev(&sd->entity);
	if (vind)
		vin_md_set_power(vind, 0);

	return ret == -ENXIO ? 0 : ret;
}

static int __vin_subdev_set_stream(struct v4l2_subdev *sd, int on)
{
	int *stream_count;
	int ret;

	if (sd == NULL)
		return -ENODEV;

	stream_count = &sd->entity.stream_count;
	if (on && (*stream_count)++ > 0)
		return 0;
	else if (!on && (*stream_count == 0 || --(*stream_count) > 0))
		return 0;
	ret = v4l2_subdev_call(sd, video, s_stream, on);

	return ret != -ENOIOCTLCMD ? ret : 0;
}

static int __vin_pipeline_s_stream(struct vin_pipeline *p, int on_idx)
{
	static const u8 seq[3][VIN_IND_MAX] = {
		{ VIN_IND_CAPTURE, VIN_IND_ISP, VIN_IND_SENSOR, VIN_IND_CSI,
			VIN_IND_MIPI, VIN_IND_SCALER },
		{ VIN_IND_SENSOR, VIN_IND_MIPI, VIN_IND_ISP,
			VIN_IND_SCALER, VIN_IND_CAPTURE, VIN_IND_CSI},
		{ VIN_IND_MIPI, VIN_IND_SENSOR, VIN_IND_ISP,
			VIN_IND_SCALER, VIN_IND_CAPTURE, VIN_IND_CSI},
	};
	struct vin_vid_cap *vin_cap = NULL;
	struct vin_md *vind = NULL;
	int i, on, ret = 0;

	if (p == NULL) {
		vin_err("pipeline is NULL, cannot s_stream\n");
		return -ENODEV;
	}

	if (p->sd[VIN_IND_SENSOR] == NULL)
		return -ENODEV;

	vind = entity_to_vin_mdev(&p->sd[VIN_IND_SENSOR]->entity);
	if (vind == NULL) {
		vin_err("vin media is NULL, cannot s_stream\n");
		return -ENODEV;
	}

	vin_cap = pipe_to_vin_video(p);
	if ((vin_cap == NULL) || (vin_cap->vinc == NULL)) {
		vin_err("vin video is NULL, cannot s_stream\n");
		return -ENODEV;
	}
	/*vin change top clk rate*/
	if ((vin_cap->vinc->vin_clk) && (vin_cap->vinc->vin_clk != vind->clk[VIN_TOP_CLK].frequency)) {
		__vin_set_top_clk_rate(vind, vin_cap->vinc->vin_clk);
		vind->clk[VIN_TOP_CLK].frequency = vin_cap->vinc->vin_clk;
	}

	for (i = 0; i < vin_cap->vinc->total_rx_ch; i++)
		csic_isp_input_select(vind->id, vin_cap->vinc->isp_sel, i,
				vin_cap->vinc->csi_sel, i);

	csic_vipp_input_select(vind->id, vin_cap->vinc->vipp_sel,
			vin_cap->vinc->isp_sel, vin_cap->vinc->isp_tx_ch);

	on = on_idx ? 1 : 0;
	for (i = 0; i < VIN_IND_ACTUATOR; i++) {
		unsigned int idx = seq[on_idx][i];
		if (NULL == p->sd[idx])
			continue;
		mutex_lock(&p->sd[idx]->entity.parent->graph_mutex);
		ret = __vin_subdev_set_stream(p->sd[idx], on);
		mutex_unlock(&p->sd[idx]->entity.parent->graph_mutex);
		if (ret < 0 && ret != -ENODEV) {
			vin_err("%s error!\n", __func__);
			goto error;
		}
		usleep_range(100, 120);
	}
	return 0;
error:
	for (; i >= 0; i--) {
		unsigned int idx = seq[on_idx][i];
		if (NULL == p->sd[idx])
			continue;
		mutex_lock(&p->sd[idx]->entity.parent->graph_mutex);
		__vin_subdev_set_stream(p->sd[idx], !on);
		mutex_unlock(&p->sd[idx]->entity.parent->graph_mutex);
	}
	return ret;
}

static const struct vin_pipeline_ops vin_pipe_ops = {
	.open		= __vin_pipeline_open,
	.close		= __vin_pipeline_close,
	.set_stream	= __vin_pipeline_s_stream,
};

static struct v4l2_subdev *__vin_subdev_register(struct vin_md *vind,
			struct modules_config *module,
			char *name, u8 addr, enum module_type type)
{
	struct v4l2_device *v4l2_dev = &vind->v4l2_dev;
	struct v4l2_subdev *sd = NULL;

	if (type == VIN_MODULE_TYPE_CCI) {
		sd = cci_bus_match(name, module->bus_sel, addr);
		if (IS_ERR_OR_NULL(sd)) {
			vin_err("registering %s, No such device!\n", name);
			return NULL;
		} else {
			if (v4l2_device_register_subdev(v4l2_dev, sd)) {
				struct cci_driver *cd = v4l2_get_subdevdata(sd);
				cci_bus_match_cancel(cd);
				vin_log(VIN_LOG_MD, "%s register failed!\n", name);
				return NULL;
			}
			vin_log(VIN_LOG_MD, "%s register OK!\n", name);
		}
	} else if (type == VIN_MODULE_TYPE_I2C) {
		struct i2c_adapter *adapter = i2c_get_adapter(module->bus_sel);
		if (adapter == NULL) {
			vin_err("request i2c adapter failed!\n");
			return NULL;
		}
		sd = v4l2_i2c_new_subdev(v4l2_dev, adapter, name, addr, NULL);
		if (IS_ERR_OR_NULL(sd)) {
			i2c_put_adapter(adapter);
			vin_err("registering %s, No such device!\n", name);
			return NULL;
		} else {
			vin_log(VIN_LOG_MD, "%s register OK!\n", name);
		}
	} else if (type == VIN_MODULE_TYPE_SPI) {
#if defined(CONFIG_SPI)
		struct spi_master *master =
				spi_busnum_to_master(module->bus_sel);
		/*if use struct spi_board_info info diretly, maybe leadto stack overflow!*/
		struct spi_board_info *info = NULL;

		if (master == NULL) {
			vin_err("request spi master failed!\n");
			return NULL;
		}

		info = kzalloc(sizeof(struct spi_board_info), GFP_KERNEL);
		if (info == NULL)
			return NULL;
		strlcpy(info->modalias, name, sizeof(info->modalias));
		info->bus_num = module->bus_sel;
		info->chip_select = 0;
		info->max_speed_hz = 12000000;
		info->mode = 0x0b; /*0x08 (little end) | 0x03*/
		sd = v4l2_spi_new_subdev(v4l2_dev, master, info);
		kfree(info);
		if (IS_ERR_OR_NULL(sd)) {
			spi_master_put(master);
			vin_err("registering %s, No such device!\n", name);
			return NULL;
		} else {
			vin_log(VIN_LOG_MD, "%s register OK!\n", name);
		}
#endif
	} else if (type == VIN_MODULE_TYPE_GPIO) {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return NULL;
	} else {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return NULL;
	}

	return sd;
}

static int __vin_subdev_unregister(struct v4l2_subdev *sd,
				enum module_type type)
{
	if (IS_ERR_OR_NULL(sd)) {
		vin_log(VIN_LOG_MD, "%s sd = NULL!\n", __func__);
		return -1;
	}

	if (type == VIN_MODULE_TYPE_CCI) {
		struct cci_driver *cci_driv = v4l2_get_subdevdata(sd);
		if (IS_ERR_OR_NULL(cci_driv))
			return -ENODEV;
		vin_log(VIN_LOG_MD, "vin sd %s unregister!\n", sd->name);
		v4l2_device_unregister_subdev(sd);
		cci_bus_match_cancel(cci_driv);
	} else if (type == VIN_MODULE_TYPE_I2C) {
		struct i2c_adapter *adapter;
		struct i2c_client *client = v4l2_get_subdevdata(sd);
		if (!client)
			return -ENODEV;
		vin_log(VIN_LOG_MD, "vin sd %s unregister!\n", sd->name);
		v4l2_device_unregister_subdev(sd);
		adapter = client->adapter;
		i2c_unregister_device(client);
		if (adapter)
			i2c_put_adapter(adapter);
	} else if (type == VIN_MODULE_TYPE_SPI) {
		struct spi_master *master;
		struct spi_device *spi = v4l2_get_subdevdata(sd);
		if (!spi)
			return -ENODEV;
		vin_log(VIN_LOG_MD, "vin sd %s unregister!\n", sd->name);
		v4l2_device_unregister_subdev(sd);
		master = spi->master;
		spi_unregister_device(spi);
		if (master)
			spi_master_put(master);
	} else if (type == VIN_MODULE_TYPE_GPIO) {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return -EFAULT;
	} else {
		vin_log(VIN_LOG_MD, "Sensor type error, type = %d!\n", type);
		return -EFAULT;
	}

	return 0;
}

static int __vin_handle_sensor_info(struct sensor_instance *inst)
{
	if (inst->cam_type == SENSOR_RAW) {
		inst->is_bayer_raw = 1;
		inst->is_isp_used = 1;
	} else if (inst->cam_type == SENSOR_YUV) {
		inst->is_bayer_raw = 0;
		inst->is_isp_used = 0;
	} else {
		inst->is_bayer_raw = 0;
		inst->is_isp_used = 0;
	}
	return 0;
}

static void __vin_verify_sensor_info(struct sensor_instance *inst)
{
	struct sensor_item sensor_info;
	char *sensor_type_name[] = {"YUV", "RAW", NULL,};
	if (get_sensor_info(inst->cam_name, &sensor_info) == 0) {
		if (inst->cam_addr != sensor_info.i2c_addr) {
			vin_warn("%s i2c_addr is different from device_tree!\n",
			     sensor_info.sensor_name);
		}
		if (inst->is_bayer_raw != sensor_info.sensor_type) {
			vin_warn("%s fmt is different from device_tree!\n",
			     sensor_type_name[sensor_info.sensor_type]);
			vin_warn("detect fmt %d replace device_tree fmt %d!\n",
			     sensor_info.sensor_type, inst->is_bayer_raw);
			inst->is_bayer_raw = sensor_info.sensor_type;
		}
		vin_log(VIN_LOG_MD, "find sensor name is %s, address is %x, type is %s\n",
		     sensor_info.sensor_name, sensor_info.i2c_addr,
		     sensor_type_name[sensor_info.sensor_type]);
	}

}

static struct v4l2_subdev *__vin_register_module(struct vin_md *vind,
			struct modules_config *module, int i)
{
	struct sensor_instance *inst = &module->sensors.inst[i];
	struct vin_module_info *modules = &module->modules;

	if (!strcmp(inst->cam_name, "")) {
		vin_err("Sensor name is NULL!\n");
		modules->sensor[i].sd = NULL;
		return modules->sensor[i].sd;
	}

	/*camera sensor register. */
	modules->sensor[i].sd = __vin_subdev_register(vind, module,
						inst->cam_name,
						inst->cam_addr >> 1,
						modules->sensor[i].type);
	if (!module->act_used) {
		modules->act[i].sd = NULL;
		return modules->sensor[i].sd;
	}
	/*camera act register. */
	modules->act[i].sd = __vin_subdev_register(vind, module,
						inst->act_name,
						inst->act_addr >> 1,
						modules->act[i].type);
	return modules->sensor[i].sd;
}

static void __vin_unregister_module(struct modules_config *module, int i)
{
	struct vin_module_info *modules = &module->modules;

	/*camera subdev unregister */
	__vin_subdev_unregister(modules->sensor[i].sd,
		modules->sensor[i].type);
	__vin_subdev_unregister(modules->act[i].sd,
		modules->act[i].type);
	vin_log(VIN_LOG_MD, "%s!\n", __func__);
	modules->sensor[i].sd = NULL;
	modules->act[i].sd = NULL;
}

static int vin_md_link_notify(struct media_pad *source,
				struct media_pad *sink, u32 flags)
{
	vin_print("%s: source %s, sink %s, flag %d\n", __func__,
		source->entity->name, sink->entity->name, flags);
	return 0;
}

static ssize_t vin_md_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct vin_md *vind = platform_get_drvdata(pdev);

	if (vind->user_subdev_api)
		return strlcpy(buf, "Sub-device API (sub-dev)\n", PAGE_SIZE);

	return strlcpy(buf, "V4L2 video node only API (vid-dev)\n", PAGE_SIZE);
}

static ssize_t vin_md_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct vin_md *vind = platform_get_drvdata(pdev);
	bool subdev_api;
	int i;

	if (!strcmp(buf, "vid-dev\n"))
		subdev_api = false;
	else if (!strcmp(buf, "sub-dev\n"))
		subdev_api = true;
	else
		return count;

	vind->user_subdev_api = subdev_api;
	for (i = 0; i < VIN_MAX_DEV; i++)
		if (vind->vinc[i])
			vind->vinc[i]->vid_cap.user_subdev_api = subdev_api;
	return count;
}

static DEVICE_ATTR(subdev_api, S_IWUSR | S_IRUGO,
		   vin_md_sysfs_show, vin_md_sysfs_store);

static int vin_md_register_core_entity(struct vin_md *vind,
					struct vin_core *vinc)
{
	struct v4l2_subdev *sd;
	int ret;

	if (WARN_ON(vinc->id >= VIN_MAX_DEV))
		return -EBUSY;

	sd = &vinc->vid_cap.subdev;
	v4l2_set_subdev_hostdata(sd, (void *)&vin_pipe_ops);

	ret = v4l2_device_register_subdev(&vind->v4l2_dev, sd);
	if (!ret) {
		vind->vinc[vinc->id] = vinc;
		vinc->vid_cap.user_subdev_api = vind->user_subdev_api;
	} else {
		vin_err("Failed to register vin_cap.%d (%d)\n",
			 vinc->id, ret);
	}
	return ret;
}

static int vin_md_register_entities(struct vin_md *vind,
						struct device_node *parent)
{
	int i, j, ret;

	vin_log(VIN_LOG_MD, "%s\n", __func__);

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct modules_config *module = NULL;
		struct sensor_list *sensors = NULL;

		module = &vind->modules[i];
		sensors = &vind->modules[i].sensors;

		sensors->valid_idx = NO_VALID_SENSOR;
		for (j = 0; j < sensors->detect_num; j++) {
			if (sensors->use_sensor_list == 1)
				__vin_handle_sensor_info(&sensors->inst[j]);
			__vin_verify_sensor_info(&sensors->inst[j]);

			if (__vin_register_module(vind, module, j)) {
				sensors->valid_idx = j;
				break;
			}
		}
		vin_log(VIN_LOG_MD, "list%d valid sensor index %d\n",
			i, sensors->valid_idx);

		if (sensors->valid_idx == NO_VALID_SENSOR || !module->flash_used)
			continue;

		/*flash subdev register */
		module->modules.flash.sd = sunxi_flash_get_subdev(
						module->modules.flash.id);

		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
					    module->modules.flash.sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "flash%d register fail!\n",
					module->modules.flash.id);
	}

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct modules_config *module = NULL;

		/*video device register */
		vind->vinc[i] = sunxi_vin_core_get_dev(i);
		if (NULL == vind->vinc[i])
			continue;
		vind->vinc[i]->v4l2_dev = &vind->v4l2_dev;

		module = &vind->modules[vind->vinc[i]->rear_sensor];

		if (module->sensors.valid_idx == NO_VALID_SENSOR) {
			vind->vinc[i] = NULL;
			continue;
		}
		vin_md_register_core_entity(vind, vind->vinc[i]);
	}

	for (i = 0; i < VIN_MAX_CSI; i++) {
		/*Register CSI subdev */
		vind->csi[i].id = i;
		vind->csi[i].sd = sunxi_csi_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->csi[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "csi%d register fail!\n", i);
	}

	for (i = 0; i < VIN_MAX_MIPI; i++) {
		/*Register MIPI subdev */
		vind->mipi[i].id = i;
		vind->mipi[i].sd = sunxi_mipi_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->mipi[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "mipi%d register fail!\n", i);
	}

	for (i = 0; i < VIN_MAX_ISP; i++) {
		/*Register ISP subdev */
		vind->isp[i].id = i;
		vind->isp[i].sd = sunxi_isp_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->isp[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "isp%d register fail!\n", i);
		/*Register STATISTIC BUF subdev */
		vind->stat[i].id = i;
		vind->stat[i].sd = sunxi_stat_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->stat[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "stat%d register fail!\n", i);
	}

	for (i = 0; i < VIN_MAX_SCALER; i++) {
		/*Register SCALER subdev */
		vind->scaler[i].id = i;
		vind->scaler[i].sd = sunxi_scaler_get_subdev(i);
		ret = v4l2_device_register_subdev(&vind->v4l2_dev,
							vind->scaler[i].sd);
		if (ret < 0)
			vin_log(VIN_LOG_MD, "scaler%d register fail!\n", i);
	}

	return 0;
}

static void vin_md_unregister_entities(struct vin_md *vind)
{
	int i;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct vin_module_info *modules = NULL;
		struct sensor_list *sensors = NULL;

		sensors = &vind->modules[i].sensors;
		if (sensors->valid_idx != NO_VALID_SENSOR) {
			__vin_unregister_module(&vind->modules[i],
						sensors->valid_idx);

			modules = &vind->modules[i].modules;
			if (modules->flash.sd == NULL)
				continue;
			v4l2_device_unregister_subdev(modules->flash.sd);
			modules->flash.sd = NULL;
		}

		if (vind->vinc[i] == NULL)
			continue;
		v4l2_device_unregister_subdev(&vind->vinc[i]->vid_cap.subdev);
		vind->vinc[i]->pipeline_ops = NULL;
		vind->vinc[i] = NULL;
	}

	for (i = 0; i < VIN_MAX_CSI; i++) {
		if (vind->csi[i].sd == NULL)
			continue;
		v4l2_device_unregister_subdev(vind->csi[i].sd);
		vind->cci[i].sd = NULL;
	}

	for (i = 0; i < VIN_MAX_MIPI; i++) {
		if (vind->mipi[i].sd == NULL)
			continue;
		v4l2_device_unregister_subdev(vind->mipi[i].sd);
		vind->mipi[i].sd = NULL;
	}

	for (i = 0; i < VIN_MAX_ISP; i++) {
		if (vind->isp[i].sd == NULL)
			continue;
		v4l2_device_unregister_subdev(vind->isp[i].sd);
		vind->isp[i].sd = NULL;
		v4l2_device_unregister_subdev(vind->stat[i].sd);
		vind->stat[i].sd = NULL;
	}

	for (i = 0; i < VIN_MAX_SCALER; i++) {
		if (vind->scaler[i].sd == NULL)
			continue;
		v4l2_device_unregister_subdev(vind->scaler[i].sd);
		vind->scaler[i].sd = NULL;
	}

	vin_log(VIN_LOG_MD, "%s\n", __func__);
}

static int sensor_link_to_mipi_csi(struct modules_config *module,
					struct v4l2_subdev *to)
{
	struct v4l2_subdev *sensor = NULL;
	struct media_entity *source, *sink;
	int ret = 0;

	if (module->sensors.valid_idx == NO_VALID_SENSOR) {
		vin_warn("Pipe line %s sensor subdev is NULL!\n",
			module->sensors.sensor_pos);
		return -1;
	}

	sensor = module->modules.sensor[module->sensors.valid_idx].sd;
	source = &sensor->entity;
	sink = &to->entity;
	ret = media_entity_create_link(source, SENSOR_PAD_SOURCE, sink, 0, 0);

	vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
		source->name, '-', sink->name);
	return ret;
}
static int vin_create_media_links(struct vin_md *vind)
{
	struct v4l2_subdev *mipi, *csi, *isp, *stat, *scaler, *cap_sd;
	struct media_entity *source, *sink;
	struct modules_config *module;
	int i, j, ret = 0;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct vin_core *vinc = NULL;
		vinc = vind->vinc[i];

		if (NULL == vinc)
			continue;

		/*MIPI*/
		if (0xff == vinc->mipi_sel)
			mipi = NULL;
		else
			mipi = vind->mipi[vinc->mipi_sel].sd;
		/*CSI*/
		if (0xff == vinc->csi_sel)
			csi = NULL;
		else
			csi = vind->csi[vinc->csi_sel].sd;

		if (NULL != mipi) {
			/*link MIPI sensor*/
			module = &vind->modules[vinc->rear_sensor];
			sensor_link_to_mipi_csi(module, mipi);
			if (vinc->rear_sensor != vinc->front_sensor) {
				module = &vind->modules[vinc->front_sensor];
				sensor_link_to_mipi_csi(module, mipi);
			}

			if (NULL == csi) {
				vin_err("MIPI Pipe line csi subdev is NULL, "
					"DevID is %d\n", i);
				continue;
			}
			source = &mipi->entity;
			sink = &csi->entity;
			ret = media_entity_create_link(source, MIPI_PAD_SOURCE,
						       sink, CSI_PAD_SINK,
						       MEDIA_LNK_FL_ENABLED);
		} else {
			/*link Bt.601 sensor*/
			if (NULL == csi) {
				vin_err("Bt.601 Pipeline csi subdev is NULL, "
					"DevID is %d\n", i);
				continue;
			}
			module = &vind->modules[vinc->rear_sensor];
			sensor_link_to_mipi_csi(module, csi);
			if (vinc->rear_sensor != vinc->front_sensor) {
				module = &vind->modules[vinc->front_sensor];
				sensor_link_to_mipi_csi(module, csi);
			}
		}

		cap_sd = &vinc->vid_cap.subdev;

		/* SCALER */
		scaler = vind->scaler[i].sd;
		if (scaler == NULL)
			continue;
		/*Link Vin Core*/
		source = &scaler->entity;
		sink = &cap_sd->entity;
		ret = media_entity_create_link(source, SCALER_PAD_SOURCE,
					       sink, VIN_SD_PAD_SINK,
					       MEDIA_LNK_FL_ENABLED);
		if (ret)
			break;

		/* Notify vin core subdev entity */
		ret = media_entity_call(sink, link_setup, &sink->pads[0],
					&source->pads[SCALER_PAD_SOURCE],
					MEDIA_LNK_FL_ENABLED);
		if (ret)
			break;

		vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
			source->name, '=', sink->name);

		source = &cap_sd->entity;
		sink = &vinc->vid_cap.vdev.entity;
		ret = media_entity_create_link(source, VIN_SD_PAD_SOURCE,
					       sink, 0, MEDIA_LNK_FL_ENABLED);
		if (ret)
			break;
		vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
			source->name, '=', sink->name);
	}

	for (i = 0; i < VIN_MAX_CSI; i++) {
		csi = vind->csi[i].sd;
		if (csi == NULL)
			continue;
		source = &csi->entity;
		for (j = 0; j < VIN_MAX_ISP; j++) {
			isp = vind->isp[j].sd;
			if (isp == NULL)
				continue;
			sink = &isp->entity;
			ret = media_entity_create_link(source, CSI_PAD_SOURCE,
						       sink, ISP_PAD_SINK, 0);
			vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
				source->name, '-', sink->name);
		}
	}

	for (i = 0; i < VIN_MAX_ISP; i++) {
		isp = vind->isp[i].sd;
		if (isp == NULL)
			continue;
		source = &isp->entity;
		stat = vind->stat[i].sd;
		sink = &stat->entity;
		ret = media_entity_create_link(source, ISP_PAD_SOURCE_ST,
					       sink, 0,
					       MEDIA_LNK_FL_IMMUTABLE |
					       MEDIA_LNK_FL_ENABLED);
		vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
			source->name, '=', sink->name);

		for (j = 0; j < VIN_MAX_SCALER; j++) {
			scaler = vind->scaler[j].sd;
			if (scaler == NULL)
				continue;
			sink = &scaler->entity;
			ret = media_entity_create_link(source, ISP_PAD_SOURCE,
						sink, SCALER_PAD_SINK, 0);
			vin_log(VIN_LOG_MD, "created link [%s] %c> [%s]\n",
				source->name, '-', sink->name);
		}
	}
	return ret;
}

static int vin_setup_default_links(struct vin_md *vind)
{
	struct v4l2_subdev *csi, *isp, *scaler;
	int i, ret = 0;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		struct vin_core *vinc = NULL;
		struct media_link *link = NULL;
		struct vin_pipeline *p = NULL;

		vinc = vind->vinc[i];
		if (NULL == vinc)
			continue;
		/*CSI*/
		if (0xff == vinc->csi_sel)
			csi = NULL;
		else
			csi = vind->csi[vinc->csi_sel].sd;

		/*ISP*/
		if (0xff == vinc->isp_sel)
			isp = NULL;
		else
			isp = vind->isp[vinc->isp_sel].sd;

		/*SCALER*/
		if (0xff == vinc->vipp_sel)
			scaler = NULL;
		else
			scaler = vind->scaler[vinc->vipp_sel].sd;

		if (csi && isp)
			link = media_entity_find_link(&csi->entity.pads[CSI_PAD_SOURCE],
						      &isp->entity.pads[ISP_PAD_SINK]);

		if (link) {
			vin_log(VIN_LOG_MD, "link: source %s sink %s\n",
				link->source->entity->name,
				link->sink->entity->name);
			ret = media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
			if (ret)
				vin_err("media_entity_setup_link error\n");
		} else {
			vin_err("media_entity_find_link null\n");
			continue;
		}

		if (isp && scaler)
			link = media_entity_find_link(&isp->entity.pads[ISP_PAD_SOURCE],
						      &scaler->entity.pads[SCALER_PAD_SINK]);

		if (link) {
			vin_log(VIN_LOG_MD, "link: source %s sink %s\n",
				link->source->entity->name,
				link->sink->entity->name);
			ret = media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
			if (ret)
				vin_err("media_entity_setup_link error\n");
		} else {
			vin_err("media_entity_find_link null\n");
			continue;
		}

		p = &vinc->vid_cap.pipe;
		vin_md_prepare_pipeline(p, &vinc->vid_cap.vdev.entity);
	}

	return ret;
}

static int vin_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	struct v4l2_device *v4l2_dev;
	struct vin_md *vind;
	enum module_type type = VIN_MODULE_TYPE_CCI;
	int ret, i, num;

	vind = devm_kzalloc(dev, sizeof(*vind), GFP_KERNEL);
	if (!vind)
		return -ENOMEM;

	spin_lock_init(&vind->slock);

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("vin media failed to get device id\n");
		pdev->id = 0;
	}

	vind->id = pdev->id;
	vind->pdev = pdev;

	vind->base = of_iomap(np, 0);
	if (!vind->base) {
		vind->is_empty = 1;
		vind->base = kzalloc(0x400, GFP_KERNEL);
		if (!vind->base) {
			ret = -EIO;
			goto freedev;
		}
	}
	csic_top_set_base_addr(vind->id, (unsigned long)vind->base);

	for (num = 0; num < VIN_MAX_DEV; num++) {
		vind->modules[num].modules.flash.type = VIN_MODULE_TYPE_GPIO;
		vind->modules[num].sensors.inst[0].cam_addr = i2c_addr;
		strcpy(vind->modules[num].sensors.inst[0].cam_name, ccm);
		vind->modules[num].sensors.inst[0].act_addr = act_slave;
		strcpy(vind->modules[num].sensors.inst[0].act_name, act_name);
		vind->modules[num].sensors.use_sensor_list = use_sensor_list;
		for (i = 0; i < MAX_GPIO_NUM; i++)
			vind->modules[num].sensors.gpio[i].gpio = GPIO_INDEX_INVALID;
	}

	parse_modules_from_device_tree(vind);

	for (num = 0; num < VIN_MAX_DEV; num++) {
#if defined(CONFIG_CCI_MODULE) || defined(CONFIG_CCI)
		type = VIN_MODULE_TYPE_CCI;
#else
		if (vind->modules[num].sensors.twi_cci_spi == 2)
			type = VIN_MODULE_TYPE_SPI;
		else
			type = VIN_MODULE_TYPE_I2C;
#endif
		for (i = 0; i < MAX_DETECT_NUM; i++) {
			vind->modules[num].modules.sensor[i].type = type;
			vind->modules[num].modules.act[i].type = type;
		}
	}

	vin_gpio_request(vind);

	strlcpy(vind->media_dev.model, "Allwinner Vin",
		sizeof(vind->media_dev.model));
	vind->media_dev.link_notify = vin_md_link_notify;
	vind->media_dev.dev = dev;

	v4l2_dev = &vind->v4l2_dev;
	v4l2_dev->mdev = &vind->media_dev;
	strlcpy(v4l2_dev->name, "sunxi-vin", sizeof(v4l2_dev->name));

	ret = v4l2_device_register(dev, &vind->v4l2_dev);
	if (ret < 0) {
		vin_err("Failed to register v4l2_device: %d\n", ret);
		goto unmap;
	}
	ret = media_device_register(&vind->media_dev);
	if (ret < 0) {
		vin_err("Failed to register media device: %d\n",
			 ret);
		goto err_md;
	}

	platform_set_drvdata(pdev, vind);

	ret = vin_md_get_clocks(vind);
	if (ret)
		goto err_clk;

	vind->user_subdev_api = 0;

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_enable(&pdev->dev);
#endif

	vin_md_clk_enable(vind);
	if (dev->of_node) {
		ret = vin_md_register_entities(vind, dev->of_node);
	} else {
		vin_err("Device tree of_node is NULL!\n");
		ret = -ENOSYS;
		goto err_clk;
	}
	vin_md_clk_disable(vind);

	mutex_lock(&vind->media_dev.graph_mutex);
	ret = vin_create_media_links(vind);
	mutex_unlock(&vind->media_dev.graph_mutex);
	if (ret) {
		vin_err("vin_create_media_links error\n");
		goto err_clk;
	}

	/*
	 * when use media_entity_setup_link we should
	 * pay attention to graph_mutex dead lock.
	 */
	ret = vin_setup_default_links(vind);
	if (ret) {
		vin_err("vin_setup_default_links error\n");
		goto err_clk;
	}

	ret = v4l2_device_register_subdev_nodes(&vind->v4l2_dev);
	if (ret)
		goto err_clk;

	ret = device_create_file(&pdev->dev, &dev_attr_subdev_api);
	if (ret)
		goto err_clk;


	vin_log(VIN_LOG_MD, "%s ok!\n", __func__);
	return 0;

err_clk:
	vin_md_put_clocks(vind);
	vin_md_unregister_entities(vind);
	media_device_unregister(&vind->media_dev);
err_md:
	v4l2_device_unregister(&vind->v4l2_dev);
unmap:
	if (!vind->is_empty)
		iounmap(vind->base);
	else
		kfree(vind->base);
freedev:
	devm_kfree(dev, vind);
	return ret;
}

static int vin_remove(struct platform_device *pdev)
{
	struct vin_md *vind = (struct vin_md *)dev_get_drvdata(&pdev->dev);

	device_remove_file(&pdev->dev, &dev_attr_subdev_api);
	vin_md_put_clocks(vind);
	vin_mclk_pin_release(vind);
	vin_gpio_release(vind);
	vin_md_unregister_entities(vind);
	media_device_unregister(&vind->media_dev);
	v4l2_device_unregister(&vind->v4l2_dev);
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&pdev->dev);
#endif
	if (vind->base) {
		if (!vind->is_empty)
			iounmap(vind->base);
		else
			kfree(vind->base);
	}

	devm_kfree(&pdev->dev, vind);
	vin_log(VIN_LOG_MD, "%s ok!\n", __func__);
	return 0;
}

static void vin_shutdown(struct platform_device *pdev)
{
	vin_log(VIN_LOG_MD, "%s!\n", __func__);
}

#ifdef CONFIG_PM_RUNTIME

int vin_runtime_suspend(struct device *d)
{
	return 0;
}
int vin_runtime_resume(struct device *d)
{
	return 0;
}

int vin_runtime_idle(struct device *d)
{
	return 0;
}

#endif

int vin_suspend(struct device *d)
{
	return 0;
}

int vin_resume(struct device *d)
{
	return 0;
}

static const struct dev_pm_ops vin_runtime_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(vin_suspend, vin_resume)
	SET_RUNTIME_PM_OPS(vin_runtime_suspend, vin_runtime_resume,
				vin_runtime_idle)
};

static const struct of_device_id sunxi_vin_match[] = {
	{.compatible = "allwinner,sunxi-vin-media",},
	{},
};

static struct platform_driver vin_driver = {
	.probe = vin_probe,
	.remove = vin_remove,
	.shutdown = vin_shutdown,
	.driver = {
		   .name = VIN_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_vin_match,
		   .pm = &vin_runtime_pm_ops,
	}
};
static int __init vin_init(void)
{
	int ret;
	vin_log(VIN_LOG_MD, "Welcome to Video Input driver\n");
	ret = sunxi_csi_platform_register();
	if (ret) {
		vin_err("Sunxi csi driver register failed\n");
		return ret;
	}

	ret = sunxi_isp_platform_register();
	if (ret) {
		vin_err("Sunxi isp driver register failed\n");
		return ret;
	}

	ret = sunxi_mipi_platform_register();
	if (ret) {
		vin_err("Sunxi mipi driver register failed\n");
		return ret;
	}

	ret = sunxi_flash_platform_register();
	if (ret) {
		vin_err("Sunxi flash driver register failed\n");
		return ret;
	}

	ret = sunxi_scaler_platform_register();
	if (ret) {
		vin_err("Sunxi scaler driver register failed\n");
		return ret;
	}

	ret = sunxi_vin_core_register_driver();
	if (ret) {
		vin_err("Sunxi vin register driver failed!\n");
		return ret;
	}

	ret = sunxi_vin_debug_register_driver();
	if (ret) {
		vin_err("Sunxi vin debug register driver failed!\n");
		return ret;
	}

	ret = platform_driver_register(&vin_driver);
	if (ret) {
		vin_err("Sunxi vin register driver failed!\n");
		return ret;
	}
	vin_log(VIN_LOG_MD, "vin init end\n");
	return ret;
}

static void __exit vin_exit(void)
{
	vin_log(VIN_LOG_MD, "vin_exit\n");
	platform_driver_unregister(&vin_driver);
	sunxi_vin_debug_unregister_driver();
	sunxi_vin_core_unregister_driver();
	sunxi_csi_platform_unregister();
	sunxi_isp_platform_unregister();
	sunxi_mipi_platform_unregister();
	sunxi_flash_platform_unregister();
	sunxi_scaler_platform_unregister();
	vin_log(VIN_LOG_MD, "vin_exit end\n");
}

module_init(vin_init);
module_exit(vin_exit);

MODULE_AUTHOR("yangfeng");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Video Input Module for Allwinner");

