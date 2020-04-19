/*
 * drivers/thermal/sunxi_thermal/sunxi_ths_driver.c
 *
 * Copyright (C) 2013-2024 allwinner.
 *	JiaRui Xiao<xiaojiarui@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#define NEED_DEBUG (0)

#if NEED_DEBUG
#define DEBUG
#endif

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/irq.h>
#include <linux/thermal.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/sunxi-sid.h>
#include "sunxi_ths.h"
#include "sunxi_ths_core.h"
#include "sunxi_ths_driver.h"

static struct sunxi_ths_controller *main_ctrl;
#define THS_INFO(fmt, arg...) \
	pr_warn("[ths]: %s()%d - "fmt, __func__, __LINE__, ##arg)

/**
 *Init the thermal sensor and show them value in screen
 */
static void sunxi_ths_reg_init(struct sunxi_ths_data *ths_data)
{
	ths_driver_init_reg(ths_data, ths_data->ths_coefficent);
	ths_driver_reg_debug(ths_data, ths_data->ths_coefficent);
	return;
}

static void sunxi_ths_exit(struct sunxi_ths_data *ths_data)
{
	ths_driver_disable_reg(ths_data);
	return;
}


#ifdef SUNXI_THERMAL_SUPPORT_IRQ
static void sunxi_ths_set_alarm_threshold_temp(struct sunxi_ths_data *ths_data,
									u32 id)
{
	u32 reg, alarm_threshold_temp = 0, alarm_threshold_value = 0;
	struct thermal_sensor_info *sensor_info =
		(struct thermal_sensor_info *)ths_data->data;

	if (sensor_info[id].alarm_irq_type == THS_LOW_TEMP_ALARM)
		alarm_threshold_temp = ths_data->alarm_low_temp;
	else if (sensor_info[id].alarm_irq_type == THS_HIGH_TEMP_ALARM)
		alarm_threshold_temp = ths_data->alarm_high_temp;

	alarm_threshold_value = ths_driver_temp_to_reg(alarm_threshold_temp,
				id, ths_data->ths_coefficent->calcular_para);

	reg = readl(ths_data->base_addr
			+ ths_hw_sensor[id].alarm->threshold.reg);
	reg = SET_BITS(ths_hw_sensor[id].alarm->threshold.shift,
		ths_hw_sensor[id].alarm->threshold.width,
		reg, alarm_threshold_value);
	writel(reg, ths_data->base_addr
			+ ths_hw_sensor[id].alarm->threshold.reg);

	/* Don't need alarm off intterup, so set the alarm off temp 0 */
	alarm_threshold_value = ths_driver_temp_to_reg(0,
				id, ths_data->ths_coefficent->calcular_para);
	reg = readl(ths_data->base_addr
			+ ths_hw_sensor[id].alarm->threshold.reg);
	reg = SET_BITS(ths_hw_sensor[id].alarm_off->threshold.shift,
		ths_hw_sensor[id].alarm->threshold.width,
		reg, alarm_threshold_value);
	writel(reg, ths_data->base_addr
			+ ths_hw_sensor[id].alarm->threshold.reg);
}

static int sunxi_ths_set_shut_threshold_temp(struct sunxi_ths_data *ths_data,
									u32 id)
{
	u32 reg, shut_threshold_value = 0;

	shut_threshold_value = ths_driver_temp_to_reg(ths_data->shut_temp,
				id, ths_data->ths_coefficent->calcular_para);

	reg = readl(ths_data->base_addr
			+ ths_hw_sensor[id].shut->threshold.reg);
	reg = SET_BITS(ths_hw_sensor[id].shut->threshold.shift,
		ths_hw_sensor[id].shut->threshold.width,
		reg, shut_threshold_value);
	writel(reg, ths_data->base_addr
			+ ths_hw_sensor[id].shut->threshold.reg);

	return 0;
}

static void sunxi_ths_clr_alarm_irq_pending(struct sunxi_ths_data *ths_data,
									u32 id)
{
	u32 reg;

	reg = readl(ths_data->base_addr +
			ths_hw_sensor[id].alarm->irq_status.reg);
	reg = SET_BITS(ths_hw_sensor[id].alarm->irq_status.shift, 1, reg, 0x1);
	writel(reg, ths_data->base_addr +
			ths_hw_sensor[id].alarm->irq_status.reg);

	reg = readl(ths_data->base_addr +
			ths_hw_sensor[id].alarm_off->irq_status.reg);
	reg = SET_BITS(ths_hw_sensor[id].alarm_off->irq_status.shift, 1,
								reg, 0x1);
	writel(reg, ths_data->base_addr +
			ths_hw_sensor[id].alarm_off->irq_status.reg);
}

static void sunxi_ths_disable_alarm_irq(struct sunxi_ths_data *ths_data, u32 id)
{
	u32 reg;
	struct thermal_sensor_info *sensor_info =
		(struct thermal_sensor_info *)ths_data->data;

	reg = readl(ths_data->base_addr +
			ths_hw_sensor[id].alarm->irq_enable.reg);
	reg = SET_BITS(ths_hw_sensor[id].alarm->irq_enable.shift, 1, reg, 0x0);
	writel(reg, ths_data->base_addr +
			ths_hw_sensor[id].alarm->irq_enable.reg);
	sensor_info[id].irq_enabled = false;

}

static void sunxi_ths_disable_shut_irq(struct sunxi_ths_data *ths_data, u32 id)
{
	u32 reg;

	reg = readl(ths_data->base_addr +
			ths_hw_sensor[id].shut->irq_enable.reg);
	reg = SET_BITS(ths_hw_sensor[id].shut->irq_enable.shift, 1, reg, 0x0);
	writel(reg, ths_data->base_addr +
			ths_hw_sensor[id].shut->irq_enable.reg);
}

static void sunxi_ths_clr_shut_irq_pending(struct sunxi_ths_data *ths_data)
{
	u32 reg, id;

	/* read irq status,if the bit is 1, then write 1 to clear it */
	for (id = 0; id < ths_data->sensor_cnt; id++) {
		reg = readl(ths_data->base_addr +
				ths_hw_sensor[id].shut->irq_status.reg);
		writel(reg, ths_data->base_addr +
				ths_hw_sensor[id].shut->irq_status.reg);
	}
}

static int sunxi_ths_enable_alarm_irq_sensor(struct sunxi_ths_data *data, u32 i)
{
	u32 reg;
	struct thermal_sensor_info *sensor_info =
		(struct thermal_sensor_info *)data->data;

	/* clear irq pending before enable irq */
	sunxi_ths_clr_alarm_irq_pending(data, i);

	/*enable irq control*/
	reg = readl(data->base_addr + ths_hw_sensor[i].alarm->irq_enable.reg);

	reg = SET_BITS(ths_hw_sensor[i].alarm->irq_enable.shift, 1, reg, 0x1);
	writel(reg, data->base_addr + ths_hw_sensor[i].alarm->irq_enable.reg);
	sensor_info[i].irq_enabled = true;

	return 0;
}

/* init enable alarm irq ctrl */
static void sunxi_ths_enable_alarm_irq_ctrl(struct sunxi_ths_data *ths_data)
{
	u32 i;
	struct thermal_sensor_info *sensor_info =
		(struct thermal_sensor_info *)ths_data->data;

	for (i = 0; i < ths_data->sensor_cnt; i++) {
		sensor_info[i].alarm_irq_type = THS_LOW_TEMP_ALARM;
		sunxi_ths_set_alarm_threshold_temp(ths_data, i);
	}
	for (i = 0; i < ths_data->sensor_cnt; i++)
		sunxi_ths_enable_alarm_irq_sensor(ths_data, i);
}

static int sunxi_ths_enable_sensor_shut_irq(struct sunxi_ths_data *ths_data,
									u32 id)
{
	u32 reg;

	sunxi_ths_clr_shut_irq_pending(ths_data);
	reg = readl(ths_data->base_addr +
			ths_hw_sensor[id].shut->irq_enable.reg);
	reg = SET_BITS(ths_hw_sensor[id].alarm->irq_enable.shift, 1, reg, 0x1);
	writel(reg, ths_data->base_addr +
			ths_hw_sensor[id].shut->irq_enable.reg);

	return 0;
}

static void sunxi_ths_enable_shut_irq_ctrl(struct sunxi_ths_data *ths_data)
{
	u32 i;

	for (i = 0; i < ths_data->sensor_cnt; i++)
		sunxi_ths_set_shut_threshold_temp(ths_data, i);

	for (i = 0; i < ths_data->sensor_cnt; i++)
		sunxi_ths_enable_sensor_shut_irq(ths_data, i);

}

static u32 sunxi_ths_query_alarmirq_pending(struct sunxi_ths_data *data, u32 id)
{
	u32 reg, irq_mask;

	irq_mask = 1 << ths_hw_sensor[id].alarm->irq_status.shift;
	reg = readl(data->base_addr +
			ths_hw_sensor[id].alarm->irq_status.reg);

	return reg&irq_mask;
}

static u32 sunxi_ths_query_shutirq_pending(struct sunxi_ths_data *ths_data)
{
	u32 reg, i;

	for (i = 0; i < ths_data->sensor_cnt; i++) {
		reg = readl(ths_data->base_addr +
				ths_hw_sensor[i].shut->irq_status.reg);
		if (reg)
			return reg;
	}

	return 0;
}

static void sunxi_ths_handle_irq_pending(struct sunxi_ths_data *ths_data)
{
	u32 id;
	struct thermal_sensor_info *sensor_info =
		(struct thermal_sensor_info *)ths_data->data;

	for (id = 0; id < ths_data->sensor_cnt; id++) {
		if (sensor_info[id].irq_enabled) {
			if (sunxi_ths_query_alarmirq_pending(ths_data, id)) {
				THS_INFO("sensor[%d]&irq-type=%d\n", id,
					sensor_info[id].alarm_irq_type);
				if (sensor_info[id].alarm_irq_type ==
							THS_LOW_TEMP_ALARM) {
					sensor_info[id].alarm_irq_type =
						THS_HIGH_TEMP_ALARM;
					sunxi_ths_set_alarm_threshold_temp(
								ths_data, id);
				} else
					sunxi_ths_disable_alarm_irq(
								ths_data, id);
			}
		}
		sunxi_ths_clr_alarm_irq_pending(ths_data, id);
	}

	/* query shutdowm irq status and handle it*/
	if (sunxi_ths_query_shutirq_pending(ths_data)) {
		THS_INFO("thermal:shutdown intterupt!\n");
		for (id = 0; id < ths_data->sensor_cnt; id++)
			sunxi_ths_disable_shut_irq(ths_data, id);
		sunxi_ths_clr_shut_irq_pending(ths_data);
	}
}

static irqreturn_t sunxi_ths_alarm_irq(int irq, void *dev)
{
	struct sunxi_ths_data *ths_data = dev;

	THS_INFO("alarm_irq\n");
	disable_irq_nosync(irq);
	sunxi_ths_handle_irq_pending(ths_data);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t sunxi_ths_alarm_irq_thread(int irq, void *dev)
{
	struct sunxi_ths_data *ths_data = dev;
	int i;
	struct thermal_sensor_info *sensor_info =
		(struct thermal_sensor_info *)ths_data->data;

	for (i = 0; i < ths_data->combine_cnt; i++) {
		thermal_zone_device_update(ths_data->comb_sensor[i]->tz);
		THS_INFO("sensor[%d]_alarmtemp = %d\n", i, sensor_info[i].temp);
	}
	THS_INFO("alarm_irq_therad\n");
	enable_irq(irq);

	return IRQ_HANDLED;
}

static int sunxi_ths_get_sensor_combine_id(struct sunxi_ths_data *ths_data,
								u32 sensor_id)
{
	u32 i, j;
	struct sunxi_ths_combine_disc *combine;

	for (i = 0; i < ths_data->combine_cnt; i++) {
		combine = ths_data->comb_sensor[i]->combine;
		for (j = 0; j < combine->combine_ths_count; j++) {
			if (sensor_id == combine->combine_ths_id[j])
				return i;
		}
	}

	THS_INFO("ths get combind id failed!\n");
	return -1;
}

#else
static void sunxi_ths_enable_alarm_irq_ctrl(struct sunxi_ths_data *ths_data)
{

}

static void sunxi_ths_enable_shut_irq_ctrl(struct sunxi_ths_data *ths_data)
{

}
#endif

static int sunxi_ths_get_temp(struct sunxi_ths_controller *controller, u32 id,
			     int *temp)
{
	int t = 0;
#ifdef SUNXI_THERMAL_SUPPORT_IRQ
	int combine_id;
#endif
	struct sunxi_ths_data *ths_data =
		(struct sunxi_ths_data *)controller->data;
	struct thermal_sensor_info *sensor_info =
		(struct thermal_sensor_info *)ths_data->data;

	if (ths_data->sensor_cnt <= id) {
		thsprintk("ths driver get wrong sensor num!\n");
		return -1;
	}

	/* if sensor data is not ready, return the last time temp */
	t = ths_driver_get_temp(ths_data, id);

	if (t == NO_DATA_INTTERUPT) {
		t = sensor_info[id].temp;
	}

	if (-40 > t || 180 < t) {
		thsprintk("ths driver get unvalid temp!\n");
		return t;
	}

	sensor_info[id].temp = t;
	*temp = t;

	#ifdef SUNXI_THERMAL_SUPPORT_IRQ
	combine_id = sunxi_ths_get_sensor_combine_id(ths_data, id);
	if (combine_id >= 0 &&
		ths_data->comb_sensor[combine_id]->tz->polling_delay == 0) {
		sunxi_ths_disable_alarm_irq(ths_data, id);
		sunxi_ths_disable_shut_irq(ths_data, id);
		return 0;
	}
	if (t < (ths_data->alarm_high_temp - ths_data->alarm_temp_hysteresis)) {
		if (!sensor_info[id].irq_enabled) {
			sunxi_ths_enable_alarm_irq_sensor(ths_data, id);
			sunxi_ths_enable_sensor_shut_irq(ths_data, id);
		}

		if (sensor_info[id].alarm_irq_type == THS_HIGH_TEMP_ALARM) {
			sensor_info[id].alarm_irq_type = THS_LOW_TEMP_ALARM;
			sunxi_ths_set_alarm_threshold_temp(ths_data, id);
		}
	}
#endif

	return 0;
}

static void sunxi_ths_info_init(struct thermal_sensor_info *sensor_info,
				int sensor_num)
{
	sensor_num -= 1;

	while (sensor_num >= 0) {
		sensor_info[sensor_num].id = sensor_num;
		sensor_info[sensor_num].ths_name = id_name_mapping[sensor_num];
		sensor_info[sensor_num].temp = 0;
		sensor_info[sensor_num].alarm_irq_type = THS_LOW_TEMP_ALARM;
		sensor_info[sensor_num].irq_enabled = false;
		thsprintk("sensor_info:id=%d,name=%s,temp=%d!\n",
			sensor_info[sensor_num].id,
			sensor_info[sensor_num].ths_name,
			sensor_info[sensor_num].temp);
		sensor_num--;
	}
	return;
}

static void sunxi_ths_coefficent_init(struct thermal_sensor_coefficent *ths_coe)
{
	ths_coe->calcular_para = thermal_cal_coefficent;
	ths_coe->reg_para = thermal_reg_init;
	return;
}

static void sunxi_ths_para_init(struct sunxi_ths_data *ths_data,
				struct thermal_sensor_info *sensor_info)
{
	sunxi_ths_info_init(sensor_info, ths_data->sensor_cnt);
	sunxi_ths_coefficent_init(ths_data->ths_coefficent);
	ths_data->data = sensor_info;
	return;
}

int sunxi_get_sensor_temp(u32 sensor_num, int *temperature)
{
	return sunxi_ths_get_temp(main_ctrl, sensor_num, temperature);
}
EXPORT_SYMBOL(sunxi_get_sensor_temp);

static int sunxi_ths_suspend(struct sunxi_ths_controller *controller)
{
	struct sunxi_ths_data *ths_data =
	    (struct sunxi_ths_data *)controller->data;

	thsprintk("enter: sunxi_ths_controller_suspend.\n");
	atomic_set(&controller->is_suspend, 1);
	sunxi_ths_exit(ths_data);
	if (ths_data->parent_clk == true)
		clk_disable_unprepare(ths_data->mclk);
	return 0;
}

static int sunxi_ths_resume(struct sunxi_ths_controller *controller)
{
	struct sunxi_ths_data *ths_data =
	    (struct sunxi_ths_data *)controller->data;

	thsprintk("enter: sunxi_ths_controller_resume.\n");
	if (ths_data->parent_clk == true)
		clk_prepare_enable(ths_data->mclk);
	sunxi_ths_reg_init(ths_data);
#ifdef SUNXI_THERMAL_SUPPORT_IRQ
	sunxi_ths_enable_alarm_irq_ctrl(ths_data);
	sunxi_ths_enable_shut_irq_ctrl(ths_data);
#endif
	atomic_set(&controller->is_suspend, 0);
	return 0;
}

struct sunxi_ths_controller_ops sunxi_ths_ops = {
	.suspend = sunxi_ths_suspend,
	.resume = sunxi_ths_resume,
	.get_temp = sunxi_ths_get_temp,
};

static int sunxi_ths_probe(struct platform_device *pdev)
{
	int err = 0;
	struct sunxi_ths_data *ths_data;
	struct sunxi_ths_controller *ctrl;
	struct thermal_sensor_info *sensor_info;

	thsprintk("sunxi ths sensor probe start !\n");
	if (!pdev->dev.of_node) {
		pr_err("%s:sunxi ths device tree err!\n", __func__);
		return -EBUSY;
	}

	ths_data = kzalloc(sizeof(*ths_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ths_data)) {
		pr_err("ths_data: not enough memory for ths_data\n");
		err = -ENOMEM;
		goto fail0;
	}

	ths_data->ths_coefficent =
		kzalloc(sizeof(*ths_data->ths_coefficent), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ths_data->ths_coefficent)) {
		pr_err("ths_coe: not enough memory for ths_coe\n");
		err = -ENOMEM;
		goto fail1;
	}

	mutex_init(&ths_data->ths_mutex_lock);
	ths_data->parent_clk = ENABLE_CLK;
	ths_data->ths_driver_version = THERMAL_VERSION;
	ths_data->cp_ft_flag = 0;
	ths_data->pdev = pdev;

	err = ths_driver_startup(ths_data, &pdev->dev);
	if (err)
		goto fail2;

	ths_data->comb_sensor = kzalloc(ths_data->combine_cnt *
			(sizeof(struct sunxi_ths_sensor *)), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ths_data->comb_sensor)) {
		pr_err("ths_comb_sensor: not enough memory to alloc!\n");
		err = -ENOMEM;
		goto fail3;
	}

#ifdef SUNXI_THERMAL_SUPPORT_IRQ
	err = devm_request_threaded_irq(&pdev->dev, ths_data->irq,
					sunxi_ths_alarm_irq,
					sunxi_ths_alarm_irq_thread,
					0, "sunxi_thermal", ths_data);
	if (err < 0) {
		pr_err("failed to request alarm irq: %d\n", err);
		return err;
	}
#endif

	sensor_info =
		kcalloc(ths_data->sensor_cnt, sizeof(*sensor_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sensor_info)) {
		pr_err("sensor_info: not enough memory for sensor_info\n");
		err = -ENOMEM;
		goto fail4;
	}

	sunxi_ths_para_init(ths_data, sensor_info);
	platform_set_drvdata(pdev, ths_data);
	ths_driver_clk_cfg(ths_data);
	sunxi_ths_reg_init(ths_data);
	sunxi_ths_enable_alarm_irq_ctrl(ths_data);
	sunxi_ths_enable_shut_irq_ctrl(ths_data);

	ths_driver_create_sensor_info_attrs(ths_data, sensor_info);

	ctrl = sunxi_ths_controller_register(&pdev->dev,
					     &sunxi_ths_ops, ths_data);
	if (!ctrl) {
		pr_err("ths_data: thermal controller register err.\n");
		err = -ENOMEM;
		goto fail2;
	}
	ths_data->ctrl = ctrl;
	main_ctrl = ctrl;
	thsprintk("sunxi_ths_controller is ok.\n");

	return 0;
fail4:
	kfree(sensor_info);
	sensor_info = NULL;
fail3:
	kfree(ths_data->comb_sensor);
	ths_data->comb_sensor = NULL;
fail2:
	kfree(ths_data->ths_coefficent);
	ths_data->ths_coefficent = NULL;
fail1:
	kfree(ths_data);
	ths_data = NULL;
fail0:
	return err;
}

static int sunxi_ths_remove(struct platform_device *pdev)
{
	struct sunxi_ths_data *ths_data = platform_get_drvdata(pdev);
	sunxi_ths_controller_unregister(ths_data->ctrl);
	sunxi_ths_exit(ths_data);
	ths_driver_clk_uncfg(ths_data);
	ths_drvier_remove_trip_attrs(ths_data);
	kfree(ths_data->ths_coefficent);
	kfree(ths_data);
	return 0;
}

#ifdef CONFIG_OF
/* Translate OpenFirmware node properties into platform_data */
static struct of_device_id sunxi_ths_of_match[] = {
	{.compatible = "allwinner,thermal_sensor",},
	{},
};

MODULE_DEVICE_TABLE(of, sunxi_ths_of_match);
#endif

static struct platform_driver sunxi_ths_driver = {
	.probe = sunxi_ths_probe,
	.remove = sunxi_ths_remove,
	.driver = {
		   .name = SUNXI_THS_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(sunxi_ths_of_match),
		   },
};
static int __init sunxi_thermal_sensor_init(void)
{
	return platform_driver_register(&sunxi_ths_driver);
}

static void __exit sunxi_thermal_sensor_exit(void)
{
	platform_driver_unregister(&sunxi_ths_driver);
}

subsys_initcall_sync(sunxi_thermal_sensor_init);
module_exit(sunxi_thermal_sensor_exit);

MODULE_DESCRIPTION("SUNXI thermal sensor driver");
MODULE_AUTHOR("JRXiao");
MODULE_LICENSE("GPL v2");
