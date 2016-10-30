/*
 * drivers/thermal/sunxi-temperature.c
 *
 * Copyright (C) 2013-2024 allwinner.
 *	Guoqing Yang<yanggq@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
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
#include <linux/irq.h>
#include <linux/thermal.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "sunxi_ths.h"
#include "sun50iw2_ths.h"
#include <linux/sunxi-sid.h>

struct sunxi_ths_data {
	void __iomem *base_addr;
	struct platform_device *pdev;
	struct sunxi_ths_controller *ctrl;
	struct clk *mclk;
	struct clk *pclk;
	int irq_num;
	int shut_temp;
	int sensor_cnt;
};

#define thsprintk(level_mask, fmt, arg...) do {	\
	if (unlikely(thermal_debug_mask & level_mask)) \
		printk(fmt , ## arg); \
	} while (0)

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_ERR = 1U << 4,
};

/* default = 0 */
static u32 thermal_debug_mask;
static struct sunxi_ths_controller *main_ctrl;

static char *id_name_mapping[] = {
	"cpu",
	"gpu"
};

static u32 MULPA_H[] = {
	CPU_MULPA,
	GPU_MULPA
};

static u32 DIVPA_H[] = {
	CPU_DIVPA,
	GPU_DIVPA
};

static u32 MINUPA_H[] = {
	CPU_MINUPA,
	GPU_MINUPA
};

static long sun8_th_reg_to_temp(u32 reg_data, int id)
{
	s32 t;
	if (reg_data <= 0x500) {
		t = (MINUPA_H[id] - reg_data * MULPA_H[id]);
		t >>= DIVPA_H[id];
	} else {
		t = (MINUPA - reg_data * MULPA);
		t >>= DIVPA;
	}
	return (long)t;
}

static u32 sun8_th_temp_to_reg(long temp, int id)
{
	s32 reg;
	if (temp >= 70) {
		reg = (MINUPA_H[id] - (temp << DIVPA_H[id]));
		reg = reg / MULPA_H[id];
	} else {
		reg = (MINUPA - (temp << DIVPA));
		reg = reg / MULPA;
	}
	return (u32) reg;
}

static void ths_sensor_init(struct sunxi_ths_data *ths_data)
{
	u32 reg_value, i;
	u32 ths_cal_data[2];

	thsprintk(DEBUG_INIT, "ths_sensor_init: ths setup start!!\n");

	writel(THS_CTRL0_VALUE, ths_data->base_addr + THS_CTRL0_REG);
	writel(THS_CTRL2_VALUE, ths_data->base_addr + THS_CTRL2_REG);
	writel(THS_CLEAR_INT_STA, ths_data->base_addr + THS_INT_STA_REG);
	writel(THS_FILT_CTRL_VALUE, ths_data->base_addr + THS_FILT_CTRL_REG);

	sunxi_efuse_read(EFUSE_THM_SENSOR_NAME, (void *)(ths_cal_data));
	if (ths_cal_data[0] != 0)
		writel(ths_cal_data[0], ths_data->base_addr + THS_0_1_CDATA_REG);

	for (i = 0; i < ths_data->sensor_cnt; i++) {
		reg_value = sun8_th_temp_to_reg(ths_data->shut_temp, i);
		reg_value = (reg_value << 16);
		writel(reg_value,
		       ths_data->base_addr + THS_INT_SHUT_TH_REG0 + i * 4);
	}

	writel(THS_INT_CTRL_VALUE, ths_data->base_addr + THS_INT_CTRL_REG);

	reg_value = readl(ths_data->base_addr + THS_CTRL2_REG);
	reg_value |= SENS0_ENABLE_BIT | SENS1_ENABLE_BIT;
	writel(reg_value, ths_data->base_addr + THS_CTRL2_REG);

	thsprintk(DEBUG_INIT, "THS_CTRL_REG0 = 0x%x\n",
		  readl(ths_data->base_addr + THS_CTRL0_REG));
	thsprintk(DEBUG_INIT, "THS_CTRL_REG2 = 0x%x\n",
		  readl(ths_data->base_addr + THS_CTRL2_REG));
	thsprintk(DEBUG_INIT, "THS_INT_CTRL_REG = 0x%x\n",
		  readl(ths_data->base_addr + THS_INT_CTRL_REG));
	thsprintk(DEBUG_INIT, "THS_INT_STA_REG = 0x%x\n",
		  readl(ths_data->base_addr + THS_INT_STA_REG));
	thsprintk(DEBUG_INIT, "THS_FILT_CTRL_REG = 0x%x\n",
		  readl(ths_data->base_addr + THS_FILT_CTRL_REG));
	thsprintk(DEBUG_INIT, "THS_0_1_CDATA_REG = 0x%x\n",
		  readl(ths_data->base_addr + THS_0_1_CDATA_REG));

	thsprintk(DEBUG_INIT, "ths_sensor_init: ths setup end!!\n");
	return;
}

static void ths_sensor_exit(struct sunxi_ths_data *ths_data)
{
	thsprintk(DEBUG_INIT, "ths_sensor_exit: clear config!!\n");
	writel(0, ths_data->base_addr + THS_CTRL2_REG);
	return;
}

static void ths_clk_cfg(struct sunxi_ths_data *ths_data)
{
	unsigned long rate = 0;

	rate = clk_get_rate(ths_data->pclk);
	thsprintk(DEBUG_INIT, "%s: get ths_clk_source rate %dHZ\n", __func__,
		  (__u32) rate);

	if (clk_set_parent(ths_data->mclk, ths_data->pclk))
		pr_err("%s: set ths_clk parent to ths_clk_source failed!\n",
		       __func__);

	if (clk_set_rate(ths_data->mclk, THS_CLK)) {
		pr_err("set ths clock freq to 4M failed!\n");
	}
	rate = clk_get_rate(ths_data->mclk);
	thsprintk(DEBUG_INIT, "%s: get ths_clk rate %dHZ\n", __func__,
		  (__u32) rate);

	if (clk_prepare_enable(ths_data->mclk)) {
		pr_err("try to enable ths_clk failed!\n");
	}

	return;
}

static void ths_clk_uncfg(struct sunxi_ths_data *ths_data)
{

	if (NULL == ths_data->mclk || IS_ERR(ths_data->mclk)) {
		pr_err("ths_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(ths_data->mclk);
		clk_put(ths_data->mclk);
		ths_data->mclk = NULL;
	}

	if (NULL == ths_data->pclk || IS_ERR(ths_data->pclk)) {
		pr_err("ths_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(ths_data->pclk);
		ths_data->pclk = NULL;
	}
	return;
}

static int sunxi_ths_startup(struct sunxi_ths_data *ths_data,
			     struct device *dev)
{
	struct device_node *np = NULL;
	int ret = 0;

	np = dev->of_node;

	ths_data->base_addr = of_iomap(np, 0);
	if (NULL == ths_data->base_addr) {
		pr_err("%s:Failed to ioremap() io memory region.\n", __func__);
		ret = -EBUSY;
	} else
		thsprintk(DEBUG_INIT, "ths base: %p !\n", ths_data->base_addr);
	ths_data->irq_num = irq_of_parse_and_map(np, 0);
	if (0 == ths_data->irq_num) {
		pr_err("%s:Failed to map irq.\n", __func__);
		ret = -EBUSY;
	} else
		thsprintk(DEBUG_INIT, "ths irq num: %d !\n", ths_data->irq_num);
	if (of_property_read_u32(np, "sensor_num", &ths_data->sensor_cnt)) {
		pr_err("%s: get sensor_num failed\n", __func__);
		ret = -EBUSY;
	}
	if (of_property_read_u32(np, "shut_temp", &ths_data->shut_temp)) {
		pr_err("%s: get int temp failed\n", __func__);
		ths_data->shut_temp = 120;
	}
	ths_data->pclk = of_clk_get(np, 0);
	ths_data->mclk = of_clk_get(np, 1);
	if (NULL == ths_data->pclk || IS_ERR(ths_data->pclk)
	    || NULL == ths_data->mclk || IS_ERR(ths_data->mclk)) {
		pr_err("%s:Failed to get clk.\n", __func__);
		ret = -EBUSY;
	}

	return ret;
}

static int sun8i_th_get_temp(struct sunxi_ths_controller *controller, u32 id,
			     long *temp)
{
	u32 reg_data;
	long t = 0;
	struct sunxi_ths_data *ths_data =
	    (struct sunxi_ths_data *)controller->data;
	if (SENSOR_CNT <= id)
		return -1;
	reg_data = readl(ths_data->base_addr + THS_INT_STA_REG);
	if (reg_data & (0x100 << id)) {
		reg_data = readl(ths_data->base_addr + THS_DATA_REG0 + id * 4);
		t = sun8_th_reg_to_temp(reg_data, id);
		thsprintk(DEBUG_DATA_INFO,
			  "THS data[%d] for %s = 0x%x, temp is %ld\n", id,
			  id_name_mapping[id], reg_data, t);
		if (-40 > t || 180 < t)
			return -1;
		*temp = t;
		return 0;
	} else
		return -1;
}

int sunxi_get_sensor_temp(u32 sensor_num, long *temperature)
{
	return sun8i_th_get_temp(main_ctrl, sensor_num, temperature);
}

EXPORT_SYMBOL(sunxi_get_sensor_temp);

static int sun8i_th_suspend(struct sunxi_ths_controller *controller)
{
	struct sunxi_ths_data *ths_data =
	    (struct sunxi_ths_data *)controller->data;
	thsprintk(DEBUG_SUSPEND, "enter: sunxi_ths_controller_suspend. \n");
	atomic_set(&controller->is_suspend, 1);
	ths_sensor_exit(ths_data);
	clk_disable_unprepare(ths_data->mclk);
	return 0;
}

static int sun8i_th_resume(struct sunxi_ths_controller *controller)
{
	struct sunxi_ths_data *ths_data =
	    (struct sunxi_ths_data *)controller->data;
	thsprintk(DEBUG_SUSPEND, "enter: sunxi_ths_controller_resume. \n");
	clk_prepare_enable(ths_data->mclk);
	ths_sensor_init(ths_data);
	atomic_set(&controller->is_suspend, 0);
	return 0;
}

struct sunxi_ths_controller_ops sunxi_ths_ops = {
	.suspend = sun8i_th_suspend,
	.resume = sun8i_th_resume,
	.get_temp = sun8i_th_get_temp,
};

static int sunxi_ths_probe(struct platform_device *pdev)
{
	int err = 0;
	struct sunxi_ths_data *ths_data;
	struct sunxi_ths_controller *ctrl;

	thsprintk(DEBUG_INIT, "sunxi ths sensor probe start !\n");
	if (!pdev->dev.of_node) {
		pr_err("%s:sunxi ths device tree err!\n", __func__);
		return -EBUSY;
	}
	ths_data = kzalloc(sizeof(*ths_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ths_data)) {
		pr_err("ths_data: not enough memory for ths data\n");
		return -ENOMEM;
	}

	err = sunxi_ths_startup(ths_data, &pdev->dev);
	if (err)
		goto fail;

	platform_set_drvdata(pdev, ths_data);
	ths_clk_cfg(ths_data);
	ths_sensor_init(ths_data);

	ctrl =
	    sunxi_ths_controller_register(&pdev->dev, &sunxi_ths_ops, ths_data);
	if (!ctrl) {
		pr_err("ths_data: thermal controller register err.\n");
		err = -ENOMEM;
		goto fail;
	}
	ths_data->ctrl = ctrl;
	main_ctrl = ctrl;
	return 0;
      fail:
	kfree(ths_data);
	return err;
}

static int sunxi_ths_remove(struct platform_device *pdev)
{
	struct sunxi_ths_data *ths_data = platform_get_drvdata(pdev);
	sunxi_ths_controller_unregister(ths_data->ctrl);
	ths_sensor_exit(ths_data);
	ths_clk_uncfg(ths_data);
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
#else /* !CONFIG_OF */
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
static int __init sunxi_ths_sensor_init(void)
{
	return platform_driver_register(&sunxi_ths_driver);
}

static void __exit sunxi_ths_sensor_exit(void)
{
	platform_driver_unregister(&sunxi_ths_driver);
}

subsys_initcall_sync(sunxi_ths_sensor_init);
module_exit(sunxi_ths_sensor_exit);
module_param_named(debug_mask, thermal_debug_mask, int, 0644);
MODULE_DESCRIPTION("SUNXI thermal sensor driver");
MODULE_AUTHOR("GQYang");
MODULE_LICENSE("GPL v2");
