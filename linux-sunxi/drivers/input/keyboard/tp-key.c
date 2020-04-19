/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
*
* Copyright (c) 2014
*
* ChangeLog
*
*
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <linux/io.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>

#define REG_TP_KEY_CTL0			0X00
#define REG_TP_KEY_CTL1			0X04
#define REG_TP_KEY_CTL2			0X08
#define REG_TP_KEY_CTL3			0X0C
#define REG_TP_KEY_INT_CTL		0X10
#define REG_TP_KEY_INT_STS		0X14
#define REG_TP_KEY_COM_DAT		0X1C
#define REG_TP_KEY_ADC_DAT		0X24

#define TP_KEY_SHORT_DLY_CNT	2

#ifdef PWR_KEY
static s32 pwr_key_gpio;
#endif

struct tp_key_data {
	struct clk		*clk;
	struct input_dev *input_dev;
	void __iomem	*reg_base;
	int				irq_num;
	int				key_num;
	unsigned int	*key_code;
	int				*key_ad_val;
	struct timer_list gpiokey_timer;
	unsigned int gpio;
	unsigned int irq;
};

#ifdef PWR_KEY
static void buttons_timer_function(unsigned long data)
{
	struct tp_key_data *pdata = (struct tp_key_data *)data;
	int val = 0;

	if (!pdata)
		return;

	val = gpio_get_value(pwr_key_gpio);
	if (val == 1) {
		input_event(pdata->input_dev, EV_KEY, KEY_VOLUMEUP, 1);
		input_sync(pdata->input_dev);
	} else {
		input_event(pdata->input_dev, EV_KEY, KEY_VOLUMEUP, 0);
		input_sync(pdata->input_dev);
	}
}

static irqreturn_t gpio_keys_irq_isr(int irq, void *dev_id)
{
	struct tp_key_data *pdata = (struct tp_key_data *)dev_id;

	mod_timer(&pdata->gpiokey_timer, jiffies+2*HZ/100);
	return IRQ_HANDLED;
}
#endif

static int tp_key_which_key(struct tp_key_data *key_data, u32 adc)
{
	int i = 0;

	if (!key_data) {
		pr_info("%s:invalid key data\n", __func__);
		return -1;
	}

	for (i = 0; i < key_data->key_num; i++) {
		if (adc < key_data->key_ad_val[i])
			break;
	}

	if (i >= key_data->key_num)
		return -1;

	return i;
}

static irqreturn_t tp_key_isr(int irq, void *dummy)
{
	struct tp_key_data *key_data = (struct tp_key_data *)dummy;
	u32  reg_val = 0, reg_data = 0;
	int index = 0;
	static int key_cnt, prev_key = -1, key_down_flag;

	reg_val = readl((void __iomem *)
		(key_data->reg_base + REG_TP_KEY_INT_STS));
	reg_data = readl((void __iomem *)
		(key_data->reg_base + REG_TP_KEY_ADC_DAT));
	writel(reg_val, (void __iomem *)
		(key_data->reg_base + REG_TP_KEY_INT_STS));

	index = tp_key_which_key(key_data, reg_data);
	if (prev_key == -1 && index == -1)
		return IRQ_HANDLED;

	if (key_down_flag && prev_key >= 0 && prev_key != index) {
		/*if key down and current key is not equal
		 *to prev key, then the key up */
		input_report_key(key_data->input_dev,
			key_data->key_code[prev_key], 0);
		/* report key up event */
		input_sync(key_data->input_dev);
		key_down_flag = 0;
		prev_key = -1;
		key_cnt = 0;
		return IRQ_HANDLED;
	}

	if (prev_key == -1 && index >= 0)
		/* save first key index */
		prev_key = index;

	if (prev_key >= 0 && index >= 0 && prev_key != index) {
		pr_info("fatal: must clear all flag!\n");
		/* it is an error and clear all */
		prev_key = -1;
		key_cnt = 0;
		if (key_down_flag) {
			input_report_key(key_data->input_dev,
			key_data->key_code[index], 0);
			input_sync(key_data->input_dev);
		}
		key_down_flag = 0;
		return IRQ_HANDLED;
	}

	if (index >= 0) {
		if (!key_down_flag)
			key_cnt++;
	}
	if (key_cnt < TP_KEY_SHORT_DLY_CNT) {
		if (index == -1) { /* is a fake key */
			key_down_flag = 0;
			prev_key = -1;
			key_cnt = 0;
		}
		return IRQ_HANDLED;
	}

	if (!key_down_flag && prev_key == index) {
		/* if the current key equal prev key index,
		 * then the key down */
		input_report_key(key_data->input_dev,
				key_data->key_code[prev_key], 1);
		/* report key down event */
		input_sync(key_data->input_dev);
		key_down_flag = 1;
	}

	return IRQ_HANDLED;
}

static int tp_key_dts_parse(struct device *pdev,
		struct tp_key_data *key_data)
{
	struct device_node *np = NULL;
	u32 val[2] = {0, 0};
	int ret = 0, i = 0;
	char key_name[16];
	if (!pdev) {
		pr_info("%s:invalid device pointer\n", __func__);
		return -EINVAL;
	}
	if (!key_data) {
		pr_info("%s:invalid tp key data pointer\n", __func__);
		return -EINVAL;
	}

	np = pdev->of_node;
	if (!np) {
		pr_info("%s:invalid device node\n", __func__);
		return -EINVAL;
	}

	key_data->reg_base = of_iomap(np, 0);
	if (NULL == key_data->reg_base) {
		pr_info("%s:Failed to ioremap io memory region.\n",
				__func__);
		ret = -ENODEV;
	} else
		pr_info("key base: %p\n", key_data->reg_base);

	key_data->irq_num = irq_of_parse_and_map(np, 0);
	if (0 == key_data->irq_num) {
		pr_info("%s:Failed to map irq.\n", __func__);
		ret = -ENODEV;
	} else
		pr_info("irq num: %d !\n", key_data->irq_num);

	if (of_property_read_u32(np, "key_cnt", &key_data->key_num)) {
		pr_info("%s: get key count failed\n", __func__);
		return -ENODEV;
	}

	if (key_data->key_num <= 0) {
		pr_info("key num is not right\n");
		return -ENODEV;
	}

	key_data->key_code = (int *)devm_kzalloc(pdev,
			sizeof(int)*key_data->key_num, GFP_KERNEL);
	if (IS_ERR_OR_NULL(key_data->key_code)) {
		pr_info("key_data: not enough memory for key code array\n");
		return -ENOMEM;
	}

	key_data->key_ad_val = (int *)devm_kzalloc(pdev,
			sizeof(int)*key_data->key_num, GFP_KERNEL);
	if (IS_ERR_OR_NULL(key_data->key_ad_val)) {
		pr_info("key_data: not enough memory for key adc value array\n");
		return -ENOMEM;
	}

	for (i = 1; i <= key_data->key_num; i++) {
		sprintf(key_name, "key%d", i);
		if (of_property_read_u32_array(np, key_name,
			val, ARRAY_SIZE(val))) {
			pr_err("%s: get %s err!\n", __func__, key_name);
			return -EBUSY;
		}
		key_data->key_ad_val[i-1] = val[0];
		key_data->key_code[i-1] = val[1];
		pr_info("key adc value:%d key code:%d\n",
			key_data->key_ad_val[i-1], key_data->key_code[i-1]);
	}

	return 0;
}

static int tp_key_hw_init(void __iomem *reg_base)
{
	u32 val = 0;

	val = 0XF<<24;
	val |= 1<<23;
	val &= ~(1<<22); /*sellect HOSC(24MHz)*/
	val |= 0x3<<20; /*00:CLK_IN/2,01:CLK_IN/3,10:CLK_IN/6,11:CLK_IN/1*/
	val |= 0x7f<<0; /*FS DIV*/
	writel(val, (void __iomem *)(reg_base + REG_TP_KEY_CTL0));

	val = 1<<4 | 1<<5; /* select adc mode and enable tp */
	writel(val, (void __iomem *)(reg_base + REG_TP_KEY_CTL1));

	val = 1<<16; /* enable date irq */
	writel(val, (void __iomem *)(reg_base + REG_TP_KEY_INT_CTL));

	val = readl((void __iomem *)(reg_base + REG_TP_KEY_CTL1));
	val &= ~(0x0f);
	val |= 1<<0;   /* select X2 */
	writel(val, (void __iomem *)(reg_base + REG_TP_KEY_CTL1));

	/* clear fifo */
	val = readl((void __iomem *)(reg_base + REG_TP_KEY_INT_CTL));
	val |= 1<<4;
	writel(val, (void __iomem *)(reg_base + REG_TP_KEY_INT_CTL));

	return 0;
}
#ifdef PWR_KEY
static int _tp_key_open(struct input_dev *dev)
{
	struct tp_key_data *key_data = (struct tp_key_data *)
		dev_get_drvdata(&dev->dev);
	int i;

	for (i = 0; i < key_data->key_num; i++) {
		input_report_key(dev, key_data->key_code[i], 0);
		input_sync(dev);
		input_report_key(dev, key_data->key_code[i], 1);
		input_sync(dev);
	}
	input_sync(dev);
	return 0;
}

static void _tp_key_close(struct input_dev *dev)
{
	struct tp_key_data *key_data = (struct tp_key_data *)
		dev_get_drvdata(&dev->dev);

	pr_info("%s:%d\n", __func__, key_data->irq_num);
}
#endif

static int tp_key_open(struct tp_key_data *key_data)
{
	pr_info("%s:%d\n", __func__, key_data->irq_num);

	if (!key_data)
		return -EINVAL;

	return request_irq(key_data->irq_num,
			tp_key_isr, 0, "tp key", key_data);
}

static void tp_key_close(struct tp_key_data *key_data)
{
	if (NULL == key_data)
		return;
	free_irq(key_data->irq_num, key_data);
}

#ifdef PWR_KEY
static ssize_t sunxi_pwr_key_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val = 0;

	val = gpio_get_value_cansleep(pwr_key_gpio);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static struct device_attribute sunxi_power_key_attr =
	__ATTR_RO(sunxi_pwr_key);
#endif

static int tp_key_probe(struct platform_device *pdev)
{
	int i;
#ifdef PWR_KEY
	int err = 0;
#endif
	int ret = -1;
	struct tp_key_data *key_data = NULL;
	struct input_dev *input_dev = NULL;
#ifdef PWR_KEY
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
#endif
	pr_info("%s:tp_key_probe\n", __func__);
	key_data = (struct tp_key_data *)devm_kzalloc(&pdev->dev,
			sizeof(struct tp_key_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(key_data)) {
		pr_info("key_data: not enough memory for key data\n");
		return -ENOMEM;
	}

	ret = tp_key_dts_parse(&pdev->dev, key_data);
	if (ret != 0)
		return ret;
#ifdef PWR_KEY
	of_property_read_u32(np, "pwr-key", &key_data->gpio);
	pwr_key_gpio = key_data->gpio;
	if (!gpio_is_valid(key_data->gpio)) {
		dev_err(dev, "failed to get gpio\n");
		return -EINVAL;
	}

	ret = devm_gpio_request(&pdev->dev, pwr_key_gpio, "pwr-key");
	if (ret) {
		pr_err("failed to request pwr-key gpio\n");
		return -EINVAL;
	} else
		ret = gpio_direction_input(pwr_key_gpio);

	key_data->irq = gpio_to_irq(key_data->gpio);

	init_timer(&key_data->gpiokey_timer);
	setup_timer(&key_data->gpiokey_timer,
			buttons_timer_function, (unsigned long)key_data);
	add_timer(&key_data->gpiokey_timer);
#endif
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_info("%s:allocate input device fail\n", __func__);
		return -ENOMEM;
	}

	input_dev->name = "tp key device";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;

	__set_bit(EV_REP, input_dev->evbit); /* support key repeat */
	__set_bit(EV_KEY, input_dev->evbit); /* support key repeat */

	for (i = 0; i < key_data->key_num; i++)
		__set_bit(key_data->key_code[i], input_dev->keybit);

	 key_data->input_dev = input_dev;

	/* hardware setting */
	if (key_data->clk)
		clk_prepare_enable(key_data->clk);

	tp_key_hw_init(key_data->reg_base);

	platform_set_drvdata(pdev, key_data);
	dev_set_drvdata(&input_dev->dev, key_data);

	if (tp_key_open(key_data)) {
		input_free_device(input_dev);
		return -EINVAL;
	}
#ifdef PWR_KEY
	err = request_irq(key_data->irq,
			(irq_handler_t)gpio_keys_irq_isr,
			IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
			"wireless_key_irq", key_data);
	if (err) {
		input_free_device(key_data->input_dev);
		platform_set_drvdata(pdev, NULL);
	}
#endif
	ret = input_register_device(input_dev);
	if (ret) {
		pr_info("%s:register input device error\n", __func__);
		tp_key_close(key_data);
		input_free_device(input_dev);
		return ret;
	}
#ifdef PWR_KEY
	device_create_file(&pdev->dev, &sunxi_power_key_attr);
#endif
	return 0;
}

static int tp_key_remove(struct platform_device *pdev)
{
	struct tp_key_data *key_data = platform_get_drvdata(pdev);

	input_unregister_device(key_data->input_dev);
	input_free_device(key_data->input_dev);
#ifdef PWR_KEY
	free_irq(key_data->irq, key_data);
	del_timer(&key_data->gpiokey_timer);
#endif
	tp_key_close(key_data);
	if (NULL != key_data)
		kfree(key_data);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id tp_key_of_match[] = {
	{ .compatible = "allwinner,tp_key",},
	{ },
};
MODULE_DEVICE_TABLE(of, tp_key_of_match);
#else /* !CONFIG_OF */
#endif

static struct platform_driver tp_key_driver = {
	.probe  = tp_key_probe,
	.remove = tp_key_remove,
	.driver = {
		.name   = "tp-key",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(tp_key_of_match),
	},
};

module_platform_driver(tp_key_driver);

MODULE_AUTHOR(" Qin");
MODULE_DESCRIPTION("sunxi-keyboard driver");
MODULE_LICENSE("GPL");
