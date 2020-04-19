/*
 * drivers/input/sensor/sunxi_gpadc.c
 *
 * Copyright (C) 2016 Allwinner.
 * fuzhaoke <fuzhaoke@allwinnertech.com>
 *
 * SUNXI GPADC Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>

#include "sunxi_gpadc.h"

static struct sunxi_gpadc *sunxi_gpadc;

static unsigned char keypad_mapindex[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0,		/* key 1, 0-8 */
	1, 1, 1, 1, 1,				/* key 2, 9-13 */
	2, 2, 2, 2, 2, 2,			/* key 3, 14-19 */
	3, 3, 3, 3, 3, 3,			/* key 4, 20-25 */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,	/* key 5, 26-36 */
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,	/* key 6, 37-39 */
	6, 6, 6, 6, 6, 6, 6, 6, 6,		/* key 7, 40-49 */
	7, 7, 7, 7, 7, 7, 7			/* key 8, 50-63 */
};

#define KEY0_GPIO	0x01
#define KEY1_GPIO	0x02
#define KEY2_GPIO	0x04

/* clk_in: source clock, round_clk: sample rate */
void sunxi_gpadc_sample_rate_set(void __iomem *reg_base, u32 clk_in, u32 round_clk)
{
	u32 div, reg_val;
	if (round_clk > clk_in)
		pr_err("%s, invalid round clk!\n", __func__);
	div = clk_in / round_clk - 1 ;
	reg_val = readl(reg_base + GP_SR_REG);
	reg_val &= ~GP_SR_CON;
	reg_val |= (div << 16);
	writel(reg_val, reg_base + GP_SR_REG);
}

void sunxi_gpadc_ctrl_set(void __iomem *reg_base, u32 ctrl_para)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val |= ctrl_para;
	writel(reg_val, reg_base + GP_CTRL_REG);
}

static u32 sunxi_gpadc_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CS_EN_REG);

	return 0;
}

static u32 sunxi_gpadc_ch_deselect(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CS_EN_REG);

	return 0;
}

static u32 sunxi_gpadc_ch_cmp(void __iomem *reg_base, enum gp_channel_id id,
							u32 low_uv, u32 hig_uv)
{
	u32 reg_val = 0, low = 0, hig = 0, unit = 0;

	if ((low_uv > hig_uv) || (hig_uv > VOL_RANGE)) {
		pr_err("%s, invalid compare value!", __func__);
		return -EINVAL;
	}

	/* anolog voltage range 0~1.8v, 12bits sample rate, unit=1.8v/(2^12) */
	unit = VOL_RANGE / 4096; /* 12bits sample rate */
	low = low_uv / unit;
	hig = hig_uv / unit;
	reg_val = low + (hig << 16);
	switch (id) {
	case GP_CH_0:
		writel(reg_val, reg_base + GP_CH0_CMP_DATA_REG);
		break;
	case GP_CH_1:
		writel(reg_val, reg_base + GP_CH1_CMP_DATA_REG);
		break;
	case GP_CH_2:
		writel(reg_val, reg_base + GP_CH2_CMP_DATA_REG);
		break;
	case GP_CH_3:
		writel(reg_val, reg_base + GP_CH3_CMP_DATA_REG);
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}

	return 0;
}

static u32 gpadc_cmp_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CS_EN_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_CMP_EN;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_CMP_EN;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_CMP_EN;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_CMP_EN;
		break;
	default:
		pr_err("%s, invalid value!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CS_EN_REG);

	return 0;
}

static u32 sunxi_gpadc_cmp_deselect(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_CMP_EN;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_CMP_EN;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_CMP_EN;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_CMP_EN;
		break;
	default:
		pr_err("%s, invalid value!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}

static u32 sunxi_gpadc_mode_select(void __iomem *reg_base, enum gp_select_mode mode)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val &= ~GP_MODE_SELECT;
	reg_val |= (mode << 18);
	writel(reg_val, reg_base + GP_CTRL_REG);

	return 0;
}

/* enable gpadc function, true:enable, false:disable */
static void sunxi_gpadc_enable(void __iomem *reg_base, bool onoff)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_CTRL_REG);
	if (true == onoff)
		reg_val |= GP_ADC_EN;
	else
		reg_val &= ~GP_ADC_EN;
	writel(reg_val, reg_base + GP_CTRL_REG);
}

static void sunxi_gpadc_calibration_enable(void __iomem *reg_base)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_CTRL_REG);
	reg_val |= GP_CALIBRATION_ENABLE;
	writel(reg_val, reg_base + GP_CTRL_REG);

}

static u32 sunxi_enable_lowirq_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATAL_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAL_INTC_REG);

	return 0;
}

static u32 sunxi_disable_lowirq_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATAL_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAL_INTC_REG);

	return 0;
}

static u32 sunxi_enable_higirq_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val = 0;


	reg_val = readl(reg_base + GP_DATAH_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAH_INTC_REG);

	return 0;
}

static u32 sunxi_disable_higirq_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val = 0;

	reg_val = readl(reg_base + GP_DATAH_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATAH_INTC_REG);

	return 0;
}

static u32 sunxi_enable_irq_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATA_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val |= GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val |= GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val |= GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val |= GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATA_INTC_REG);

	return 0;
}

static u32 sunxi_disable_irq_ch_select(void __iomem *reg_base, enum gp_channel_id id)
{
	u32 reg_val;

	reg_val = readl(reg_base + GP_DATA_INTC_REG);
	switch (id) {
	case GP_CH_0:
		reg_val &= ~GP_CH0_SELECT;
		break;
	case GP_CH_1:
		reg_val &= ~GP_CH1_SELECT;
		break;
	case GP_CH_2:
		reg_val &= ~GP_CH2_SELECT;
		break;
	case GP_CH_3:
		reg_val &= ~GP_CH3_SELECT;
		break;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
	writel(reg_val, reg_base + GP_DATA_INTC_REG);

	return 0;
}

static u32 sunxi_ch_lowirq_status(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATAL_INTS_REG);
}

static void sunxi_ch_lowirq_clear_flags(void __iomem *reg_base, u32 flags)
{
	writel(flags, reg_base + GP_DATAL_INTS_REG);
}

static u32 sunxi_ch_higirq_status(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATAH_INTS_REG);
}

static void sunxi_ch_higirq_clear_flags(void __iomem *reg_base, u32 flags)
{
	writel(flags, reg_base + GP_DATAH_INTS_REG);
}

static u32 sunxi_ch_irq_status(void __iomem *reg_base)
{
	return readl(reg_base + GP_DATA_INTS_REG);
}

static void sunxi_ch_irq_clear_flags(void __iomem *reg_base, u32 flags)
{
	writel(flags, reg_base + GP_DATA_INTS_REG);
}

static u32 sunxi_gpadc_read_data(void __iomem *reg_base, enum gp_channel_id id)
{
	switch (id) {
	case GP_CH_0:
		return readl(reg_base + GP_CH0_DATA_REG) & GP_CH0_DATA_MASK;
	case GP_CH_1:
		return readl(reg_base + GP_CH1_DATA_REG) & GP_CH1_DATA_MASK;
	case GP_CH_2:
		return readl(reg_base + GP_CH2_DATA_REG) & GP_CH2_DATA_MASK;
	case GP_CH_3:
		return readl(reg_base + GP_CH3_DATA_REG) & GP_CH3_DATA_MASK;
	default:
		pr_err("%s, invalid channel id!", __func__);
		return -EINVAL;
	}
}

static int sunxi_gpadc0_input_register(struct sunxi_gpadc *sunxi_gpadc)
{
	static struct input_dev *input_dev;
	int i, ret;

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("input_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	input_dev->name = "sunxi-gpadc0";
	input_dev->phys = "sunxigpadc0/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;

	input_dev->evbit[0] = BIT_MASK(EV_KEY);
	for (i = 0; i < KEY_MAX_CNT; i++)
		set_bit(sunxi_gpadc->scankeycodes[i], input_dev->keybit);
	sunxi_gpadc->input_key = input_dev;
	ret = input_register_device(sunxi_gpadc->input_key);
	if (ret) {
		pr_err("input register fail\n");
		goto fail;
	}

	return 0;
fail:
	input_free_device(sunxi_gpadc->input_key);
	return ret;
}

static int sunxi_gpadc1_input_register(struct sunxi_gpadc *sunxi_gpadc)
{
	static struct input_dev *input_dev;
	int ret;

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("input_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	input_dev->name = "sunxi-gpadc1";
	input_dev->phys = "sunxigpadc1/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;

	input_dev->evbit[0] = BIT_MASK(EV_MSC);
	set_bit(EV_MSC, input_dev->evbit);
	set_bit(MSC_SCAN, input_dev->mscbit);
	sunxi_gpadc->input_gpadc = input_dev;
	ret = input_register_device(sunxi_gpadc->input_gpadc);
	if (ret) {
		pr_err("input register fail\n");
		goto fail;
	}

	return 0;
fail:
	input_free_device(sunxi_gpadc->input_gpadc);
	return ret;
}

static int sunxi_gpadc2_input_register(struct sunxi_gpadc *sunxi_gpadc)
{
	static struct input_dev *input_dev;
	int ret;

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("input_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	input_dev->name = "sunxi-gpadc2";
	input_dev->phys = "sunxigpadc2/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;

	input_dev->evbit[0] = BIT_MASK(EV_MSC);
	set_bit(EV_MSC, input_dev->evbit);
	set_bit(MSC_SCAN, input_dev->mscbit);

	sunxi_gpadc->input_electricity = input_dev;
	ret = input_register_device(sunxi_gpadc->input_electricity);
	if (ret) {
		pr_err("input register fail\n");
		goto fail;
	}

	return 0;
fail:
	input_free_device(sunxi_gpadc->input_electricity);
	return ret;
}

static int sunxi_gpadc3_input_register(struct sunxi_gpadc *sunxi_gpadc)
{
	static struct input_dev *input_dev;
	int ret;

	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("input_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	input_dev->name = "sunxi-gpadc3";
	input_dev->phys = "sunxigpadc3/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;

	input_dev->evbit[0] = BIT_MASK(EV_MSC);
	set_bit(EV_MSC, input_dev->evbit);
	set_bit(MSC_SCAN, input_dev->mscbit);

	sunxi_gpadc->input_sensor = input_dev;
	ret = input_register_device(sunxi_gpadc->input_sensor);
	if (ret) {
		pr_err("input register fail\n");
		goto fail;
	}

	return 0;
fail:
	input_free_device(sunxi_gpadc->input_sensor);
	return ret;
}

static int sunxi_channel_select_input_register(struct sunxi_gpadc *sunxi_gpadc, enum gp_channel_id id)
{
	switch (id) {
	case GP_CH_0:
		sunxi_gpadc0_input_register(sunxi_gpadc);
		break;
	case GP_CH_1:
		sunxi_gpadc1_input_register(sunxi_gpadc);
		break;
	case GP_CH_2:
		sunxi_gpadc2_input_register(sunxi_gpadc);
		break;
	case GP_CH_3:
		sunxi_gpadc3_input_register(sunxi_gpadc);
		break;
	default:
		pr_err("%s, invalid channel id of input_register!", __func__);
		return -EINVAL;
	}

	return 0;
}

static int sunxi_key_init(struct sunxi_gpadc *sunxi_gpadc, struct platform_device *pdev)
{
	struct device_node *np = NULL;
	unsigned char i, j = 0;
	u32 val[2] = {0, 0};
	char key_property_name[16];
	u32 key_num = 0;
	u32 key_vol[VOL_NUM];

	np = pdev->dev.of_node;
	if (of_property_read_u32(np, "key_cnt", &key_num)) {
		pr_err("%s: get key count failed", __func__);
		return -EBUSY;
	}

	pr_debug("%s key number = %d.\n", __func__, key_num);
	if (key_num < 1 || key_num > VOL_NUM) {
		pr_err("incorrect key number.\n");
		return -1;
	}

	for (i = 0; i < key_num; i++) {
		sprintf(key_property_name, "key%d_vol", i);
			if (of_property_read_u32(np, key_property_name, &val[0])) {
				pr_err("%s:get%s err!\n", __func__, key_property_name);
				return -EBUSY;
			}
			key_vol[i] = val[0];

			sprintf(key_property_name, "key%d_val", i);
			if (of_property_read_u32(np, key_property_name, &val[1])) {
				pr_err("%s:get%s err!\n", __func__, key_property_name);
				return -EBUSY;
			}
			sunxi_gpadc->scankeycodes[i] = val[1];
			pr_debug("%s: key%d vol= %d code= %d\n", __func__, i,
					key_vol[i], sunxi_gpadc->scankeycodes[i]);
	}

	key_vol[key_num] = MAXIMUM_INPUT_VOLTAGE;
	for (i = 0; i < key_num; i++)
		key_vol[i] += (key_vol[i+1] - key_vol[i])/2;

	for (i = 0; i < 128; i++) {
		if (i * SCALE_UNIT > key_vol[j])
			j++;
		keypad_mapindex[i] = j;
	}

	return 0;
}

static int sunxi_gpadc_input_register_setup(struct sunxi_gpadc *sunxi_gpadc)
{
	int i;

	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		if (sunxi_gpadc->channel[i] == 1)
			sunxi_channel_select_input_register(sunxi_gpadc, i);
	}

	return 0;
}

#ifdef CONFIG_KEY_GPIO
static irqreturn_t  key0_gpio_interrupt(int irq, void *dev_id)
{
	struct sunxi_gpadc *sunxi_gpadc = (struct sunxi_gpadc *)dev_id;

	sunxi_gpadc->key_result |= KEY0_GPIO;
	mod_timer(&sunxi_gpadc->key_timer, jiffies + (HZ/120));

	return IRQ_HANDLED;
}

static irqreturn_t  key1_gpio_interrupt(int irq, void *dev_id)
{
	struct sunxi_gpadc *sunxi_gpadc = (struct sunxi_gpadc *)dev_id;

	sunxi_gpadc->key_result |= KEY1_GPIO;
	mod_timer(&sunxi_gpadc->key_timer, jiffies + (HZ/120));

	return IRQ_HANDLED;
}

static irqreturn_t  key2_gpio_interrupt(int irq, void *dev_id)
{
	struct sunxi_gpadc *sunxi_gpadc = (struct sunxi_gpadc *)dev_id;

	sunxi_gpadc->key_result |= KEY2_GPIO;
	mod_timer(&sunxi_gpadc->key_timer, jiffies + (HZ/120));

	return IRQ_HANDLED;
}

static void sunxi_key_do_timer(unsigned long arg)
{
	struct sunxi_gpadc *sunxi_gpadc = (struct sunxi_gpadc *)arg;
	int gval;

	if (sunxi_gpadc->key_result & KEY0_GPIO) {
		gval = gpio_get_value(sunxi_gpadc->key0_gpio);
		gval = !gval;
		input_report_key(sunxi_gpadc->input_key, KEY_ENTER, gval);
		input_sync(sunxi_gpadc->input_key);
		sunxi_gpadc->key_result &= ~KEY0_GPIO;
	}
	if (sunxi_gpadc->key_result & KEY1_GPIO) {
		gval = gpio_get_value(sunxi_gpadc->key1_gpio);
		gval = !gval;
		input_report_key(sunxi_gpadc->input_key, KEY_DOWN, gval);
		input_sync(sunxi_gpadc->input_key);
		sunxi_gpadc->key_result &= ~KEY1_GPIO;
	}
	if (sunxi_gpadc->key_result & KEY2_GPIO) {
		gval = gpio_get_value(sunxi_gpadc->key2_gpio);
		gval = !gval;
		input_report_key(sunxi_gpadc->input_key, KEY_RIGHT, gval);
		input_sync(sunxi_gpadc->input_key);
		sunxi_gpadc->key_result &= ~KEY2_GPIO;
	}
}

static int gpio_init_key(struct platform_device *pdev, struct input_dev *input_dev, struct sunxi_gpadc *sunxi_gpadc)
{
	struct gpio_config val;
	struct device_node *np = pdev->dev.of_node;

	val.gpio = of_get_named_gpio_flags(np, "KEY0_GPIO", 0, (enum of_gpio_flags *)(&(val)));
	if (!gpio_is_valid(val.gpio))
		pr_err("%s: KEY0_GPIO is invalid.\n", __func__);
	sunxi_gpadc->key0_gpio = val.gpio;
	if (0 != gpio_request(sunxi_gpadc->key0_gpio, NULL)) {
		pr_err("gpio request failed.\n");
		goto script_get_err;
	}
	gpio_direction_input(sunxi_gpadc->key0_gpio);

	val.gpio = of_get_named_gpio_flags(np, "KEY1_GPIO", 0, (enum of_gpio_flags *)(&(val)));
	if (!gpio_is_valid(val.gpio))
		pr_err("%s: KEY1_GPIO is invalid.\n", __func__);
	sunxi_gpadc->key1_gpio = val.gpio;
	if (0 != gpio_request(sunxi_gpadc->key1_gpio, NULL)) {
		pr_err("gpio request failed.\n");
		goto script_get_err1;
	}
	gpio_direction_input(sunxi_gpadc->key1_gpio);

	val.gpio = of_get_named_gpio_flags(np, "KEY2_GPIO", 0, (enum of_gpio_flags *)(&(val)));
	if (!gpio_is_valid(val.gpio))
		pr_err("%s: KEY2_GPIO is invalid. \n", __func__);
	sunxi_gpadc->key2_gpio = val.gpio;
	if (0 != gpio_request(sunxi_gpadc->key2_gpio, NULL)) {
		pr_err("gpio request failed.\n");
		goto script_get_err2;
	}
	gpio_direction_input(sunxi_gpadc->key2_gpio);

	set_bit(KEY_DOWN, input_dev->keybit);
	set_bit(KEY_ENTER, input_dev->keybit);
	set_bit(KEY_RIGHT, input_dev->keybit);

	init_timer(&sunxi_gpadc->key_timer);
	sunxi_gpadc->key_timer.function = &sunxi_key_do_timer;
	sunxi_gpadc->key_timer.data = (unsigned long)sunxi_gpadc;
	sunxi_gpadc->key_timer.expires = jiffies + (HZ/120);
	add_timer(&sunxi_gpadc->key_timer);

	if (request_irq(__gpio_to_irq(sunxi_gpadc->key0_gpio), key0_gpio_interrupt,
			IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "key0_gpio", sunxi_gpadc)) {
		pr_err("Unable to request pulley lkey.\n");
		goto script_get_err3;
	}

	if (request_irq(__gpio_to_irq(sunxi_gpadc->key1_gpio), key1_gpio_interrupt,
			IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "key1_gpio", sunxi_gpadc)) {
		pr_err("Unable to request pulley lkey.\n");
		goto script_get_err4;
	}

	if (request_irq(__gpio_to_irq(sunxi_gpadc->key2_gpio), key2_gpio_interrupt,
			IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "key2_gpio", sunxi_gpadc)) {
		pr_err("Unable to request pulley lkey.\n");
		goto script_get_err5;
	}

	return 0;

script_get_err5:
	free_irq(__gpio_to_irq(sunxi_gpadc->key2_gpio), NULL);
script_get_err4:
	free_irq(__gpio_to_irq(sunxi_gpadc->key1_gpio), NULL);
script_get_err3:
	free_irq(__gpio_to_irq(sunxi_gpadc->key0_gpio), NULL);
script_get_err2:
	gpio_free(sunxi_gpadc->key0_gpio);
script_get_err1:
	gpio_free(sunxi_gpadc->key1_gpio);
script_get_err:
	gpio_free(sunxi_gpadc->key2_gpio);
	return -1;
}
#endif

static int sunxi_gpadc_setup(struct platform_device *pdev,
					struct sunxi_gpadc *sunxi_gpadc)
{
	struct device_node *np = NULL;
	u32 val;
	int i;
	unsigned char name[30];

	np = pdev->dev.of_node;
	if (of_property_read_u32(np, "channel_num", &sunxi_gpadc->channel_num)) {
		pr_err("%s: get channel num failed\n", __func__);
		return -EBUSY;
	}

	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		sprintf(name, "channel%d", i);
		if (of_property_read_u32(np, name, &val)) {
			pr_err("%s:get %s err!\n", __func__, name);
			val = 0;
		}
		sunxi_gpadc->channel[i] = val;

		sprintf(name, "channel%d_compare", i);
		if (of_property_read_u32(np, name, &val)) {
			pr_err("%s:get %s err!\n", __func__, name);
			val = 0;
		}
		sunxi_gpadc->channel_compare[i] = val;

		sprintf(name, "channel%d_compare_lowdata", i);
		if (of_property_read_u32(np, name, &val)) {
			pr_err("%s:get %s err!\n", __func__, name);
			val = 0;
		}
		sunxi_gpadc->channel_compare_lowdata[i] = val;

		sprintf(name, "channel%d_compare_higdata", i);
		if (of_property_read_u32(np, name, &val)) {
			pr_err("%s:get %s err!\n", __func__, name);
			val = 0;
		}
		sunxi_gpadc->channel_compare_higdata[i] = val;
	}

	return 0;
}

static int sunxi_gpadc_hw_init(struct platform_device *pdev, struct sunxi_gpadc *sunxi_gpadc)
{
	int i;

	sunxi_gpadc_sample_rate_set(sunxi_gpadc->reg_base, OSC_24MHZ, 1000);
	sunxi_gpadc_setup(pdev, sunxi_gpadc);
	for (i = 0; i < sunxi_gpadc->channel_num; i++) {
		if (sunxi_gpadc->channel[i] == 1) {
			sunxi_gpadc_ch_select(sunxi_gpadc->reg_base, i);
			sunxi_enable_irq_ch_select(sunxi_gpadc->reg_base, i);
			if (sunxi_gpadc->channel_compare[i] == 1) {
				gpadc_cmp_select(sunxi_gpadc->reg_base, i);
				sunxi_enable_lowirq_ch_select(sunxi_gpadc->reg_base, i);
				sunxi_enable_higirq_ch_select(sunxi_gpadc->reg_base, i);
				sunxi_gpadc_ch_cmp(sunxi_gpadc->reg_base, i,
					sunxi_gpadc->channel_compare_lowdata[i],
					sunxi_gpadc->channel_compare_higdata[i]);
			} else {
				sunxi_gpadc_cmp_deselect(sunxi_gpadc->reg_base, i);
				sunxi_disable_lowirq_ch_select(sunxi_gpadc->reg_base, i);
				sunxi_disable_higirq_ch_select(sunxi_gpadc->reg_base, i);
				sunxi_gpadc_ch_cmp(sunxi_gpadc->reg_base, i, 0, 0);
			}
		} else {
			sunxi_gpadc_ch_deselect(sunxi_gpadc->reg_base, i);
			sunxi_disable_irq_ch_select(sunxi_gpadc->reg_base, i);
			sunxi_gpadc_cmp_deselect(sunxi_gpadc->reg_base, i);
			sunxi_disable_lowirq_ch_select(sunxi_gpadc->reg_base, i);
			sunxi_disable_higirq_ch_select(sunxi_gpadc->reg_base, i);
			sunxi_gpadc_ch_cmp(sunxi_gpadc->reg_base, i, 0, 0);
		}
	}
	sunxi_gpadc_calibration_enable(sunxi_gpadc->reg_base);
	sunxi_gpadc_mode_select(sunxi_gpadc->reg_base, GP_CONTINUOUS_MODE);
	sunxi_gpadc_enable(sunxi_gpadc->reg_base, true);

	return 0;
}

static irqreturn_t sunxi_gpadc_interrupt(int irqno, void *dev_id)
{
	struct sunxi_gpadc *sunxi_gpadc = (struct sunxi_gpadc *)dev_id;
	u32  reg_val, reg_low, reg_hig;
	u32 data, vol_data;

	reg_val = sunxi_ch_irq_status(sunxi_gpadc->reg_base);
	sunxi_ch_irq_clear_flags(sunxi_gpadc->reg_base, reg_val);
	reg_low = sunxi_ch_lowirq_status(sunxi_gpadc->reg_base);
	sunxi_ch_lowirq_clear_flags(sunxi_gpadc->reg_base, reg_low);
	reg_hig = sunxi_ch_higirq_status(sunxi_gpadc->reg_base);
	sunxi_ch_higirq_clear_flags(sunxi_gpadc->reg_base, reg_hig);

	if (reg_val & GP_CH0_DATA) {
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, GP_CH_0);
		data = ((VOL_RANGE / 4096)*data); /* 12bits sample rate */
		vol_data = data / 1000;
		sunxi_gpadc->sampling++;
		if (sunxi_gpadc->sampling == SAMPLING_FREQUENCY) {
			sunxi_gpadc->sampling = 0;
			if (vol_data < SUNXIKEY_DOWN) {
				sunxi_gpadc->key_cnt++;
				sunxi_gpadc->compare_before = ((data / SCALE_UNIT) / 1000)&0xff;
				if (sunxi_gpadc->compare_before == sunxi_gpadc->compare_later) {
					sunxi_gpadc->compare_later = sunxi_gpadc->compare_before;
					sunxi_gpadc->key_code = keypad_mapindex[sunxi_gpadc->compare_before];
					input_report_key(sunxi_gpadc->input_key, sunxi_gpadc->scankeycodes[sunxi_gpadc->key_code], 1);
					input_sync(sunxi_gpadc->input_key);
					pr_debug("%s: key%d vol= %d, key_code = %d\n", __func__,
						sunxi_gpadc->scankeycodes[sunxi_gpadc->key_code], vol_data, sunxi_gpadc->key_code);
					sunxi_gpadc->key_cnt = 0;
					sunxi_gpadc->key_down = 1;
				}
				if (sunxi_gpadc->key_cnt == 2) {
					sunxi_gpadc->compare_later = sunxi_gpadc->compare_before;
					sunxi_gpadc->key_cnt = 0;
				}
			}
			if ((vol_data >= SUNXIKEY_UP) && (sunxi_gpadc->key_down == 1)) {
				sunxi_gpadc->key_cnt++;
				if (sunxi_gpadc->compare_before == sunxi_gpadc->compare_later) {
					sunxi_gpadc->compare_later = sunxi_gpadc->compare_before;
					input_report_key(sunxi_gpadc->input_key, sunxi_gpadc->scankeycodes[sunxi_gpadc->key_code], 0);
					input_sync(sunxi_gpadc->input_key);
					pr_debug("%s: key%d vol= %d, key_code = %d\n", __func__,
						sunxi_gpadc->scankeycodes[sunxi_gpadc->key_code], vol_data, sunxi_gpadc->key_code);
					sunxi_gpadc->key_cnt = 0;
					sunxi_gpadc->key_down = 0;
				}
				if (sunxi_gpadc->key_cnt == 2) {
					sunxi_gpadc->compare_later = sunxi_gpadc->compare_before;
					sunxi_gpadc->key_cnt = 0;
				}
			}
		}
	}

	if (reg_val & GP_CH1_DATA) {
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, GP_CH_1);
		input_event(sunxi_gpadc->input_gpadc, EV_MSC, MSC_SCAN, data);
		input_sync(sunxi_gpadc->input_gpadc);
	}

	if (reg_val & GP_CH2_DATA) {
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, GP_CH_2);
		input_event(sunxi_gpadc->input_electricity, EV_MSC, MSC_SCAN, data);
		input_sync(sunxi_gpadc->input_electricity);
	}

	if (reg_val & GP_CH3_DATA) {
		data = sunxi_gpadc_read_data(sunxi_gpadc->reg_base, GP_CH_3);
		input_event(sunxi_gpadc->input_sensor, EV_MSC, MSC_SCAN, data);
		input_sync(sunxi_gpadc->input_sensor);
	}

	if (reg_low & GP_CH0_LOW)
		pr_debug("channel 0 low pend\n");

	if (reg_low & GP_CH1_LOW)
		pr_debug("channel 1 low pend\n");

	if (reg_low & GP_CH2_LOW)
		pr_debug("channel 2 low pend\n");

	if (reg_low & GP_CH3_LOW)
		pr_debug("channel 3 low pend\n");

	if (reg_hig & GP_CH0_HIG)
		pr_debug("channel 0 hight pend\n");

	if (reg_hig & GP_CH1_HIG)
		pr_debug("channel 1 hight pend\n");

	if (reg_hig & GP_CH2_HIG)
		pr_debug("channel 2 hight pend\n");

	if (reg_hig & GP_CH3_HIG)
		pr_debug("channel 3 hight pend\n");

	return IRQ_HANDLED;
}

int sunxi_gpadc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	if (!of_device_is_available(np)) {
		pr_err("%s: sunxi gpadc is disable\n", __func__);
		return -EPERM;
	}

	sunxi_gpadc = kzalloc(sizeof(struct sunxi_gpadc), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_gpadc)) {
		pr_err("not enough memory for sunxi_gpadc\n");
		return -ENOMEM;
	}

	sunxi_gpadc->reg_base = of_iomap(np, 0);
	if (NULL == sunxi_gpadc->reg_base) {
		pr_err("sunxi_gpadc iomap fail\n");
		ret = -EBUSY;
		goto fail1;
	}

	sunxi_gpadc->irq_num = irq_of_parse_and_map(np, 0);
	if (0 == sunxi_gpadc->irq_num) {
		pr_err("sunxi_gpadc fail to map irq\n");
		ret = -EBUSY;
		goto fail2;
	}

	sunxi_gpadc->mclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_gpadc->mclk)) {
		pr_err("sunxi_gpadc has no clk\n");
		ret = -EINVAL;
		goto fail3;
	} else{
		if (clk_prepare_enable(sunxi_gpadc->mclk)) {
			pr_err("enable sunxi_gpadc clock failed!\n");
			ret = -EINVAL;
			goto fail3;
		}
	}

	sunxi_key_init(sunxi_gpadc, pdev);
	sunxi_gpadc_hw_init(pdev, sunxi_gpadc);
	sunxi_gpadc_input_register_setup(sunxi_gpadc);
#ifdef CONFIG_KEY_GPIO
	gpio_init_key(pdev, sunxi_gpadc->input_key, sunxi_gpadc);
#endif
	if (request_irq(sunxi_gpadc->irq_num, sunxi_gpadc_interrupt,
			IRQF_TRIGGER_NONE, "sunxi-gpadc", sunxi_gpadc)) {
		pr_err("sunxi_gpadc request irq failure\n");
		return -1;
	}
	platform_set_drvdata(pdev, sunxi_gpadc);

	return 0;

fail3:
	free_irq(sunxi_gpadc->irq_num, sunxi_gpadc);
fail2:
	iounmap(sunxi_gpadc->reg_base);
fail1:
	kfree(sunxi_gpadc);

	return ret;
}

int sunxi_gpadc_remove(struct platform_device *pdev)

{
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

#ifdef CONFIG_KEY_GPIO
	gpio_free(sunxi_gpadc->key0_gpio);
	gpio_free(sunxi_gpadc->key1_gpio);
	gpio_free(sunxi_gpadc->key2_gpio);
#endif
	sunxi_gpadc_enable(sunxi_gpadc->reg_base, false);
	free_irq(sunxi_gpadc->irq_num, sunxi_gpadc);
	clk_disable_unprepare(sunxi_gpadc->mclk);
	iounmap(sunxi_gpadc->reg_base);
	kfree(sunxi_gpadc);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_gpadc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

	disable_irq_nosync(sunxi_gpadc->irq_num);
	sunxi_gpadc_enable(sunxi_gpadc->reg_base, false);
	clk_disable_unprepare(sunxi_gpadc->mclk);

	return 0;
}

static int sunxi_gpadc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_gpadc *sunxi_gpadc = platform_get_drvdata(pdev);

	enable_irq(sunxi_gpadc->irq_num);
	clk_prepare_enable(sunxi_gpadc->mclk);
	sunxi_gpadc_hw_init(pdev, sunxi_gpadc);

	return 0;
}

static const struct dev_pm_ops sunxi_gpadc_dev_pm_ops = {
	.suspend = sunxi_gpadc_suspend,
	.resume = sunxi_gpadc_resume,
};

#define SUNXI_GPADC_DEV_PM_OPS (&sunxi_gpadc_dev_pm_ops)
#else
#define SUNXI_GPADC_DEV_PM_OPS NULL
#endif

static struct of_device_id sunxi_gpadc_of_match[] = {
	{ .compatible = "allwinner,sunxi-gpadc"},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_gpadc_of_match);

static struct platform_driver sunxi_gpadc_driver = {
	.probe  = sunxi_gpadc_probe,
	.remove = sunxi_gpadc_remove,
	.driver = {
		.name   = "sunxi-gpadc",
		.owner  = THIS_MODULE,
		.pm = SUNXI_GPADC_DEV_PM_OPS,
		.of_match_table = of_match_ptr(sunxi_gpadc_of_match),
	},
};
module_platform_driver(sunxi_gpadc_driver);

MODULE_AUTHOR("Fuzhaoke");
MODULE_DESCRIPTION("sunxi-gpadc driver");
MODULE_LICENSE("GPL");
