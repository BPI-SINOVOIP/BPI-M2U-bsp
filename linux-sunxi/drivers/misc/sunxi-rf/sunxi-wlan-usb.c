/*
 * sunxi-wlan-usb.c -- power on/off wlan part of SoC
 *
 * Copyright (c) 2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Wei Li<liwei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/rfkill.h>
#include <linux/capability.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/sys_config.h>

struct sunxi_wlan_usb_platdata {
	char *wlan_usb_power_name;
	
	int usb_bus_index;
	int gpio_wlan_usb_drv;
	int gpio_wlan_usb_det;
	int power_state;
	
	struct regulator *wlan_usb_power;
	struct platform_device *pdev;
};
static struct sunxi_wlan_usb_platdata *wlan_usb_data;

static int sunxi_wlan_usb_on(struct sunxi_wlan_usb_platdata *data, bool on_off);
static DEFINE_MUTEX(sunxi_wlan_usb_mutex);

void sunxi_wlan_usb_set_power(bool on_off)
{
	struct platform_device *pdev;
	int ret = 0;
	if(!wlan_usb_data)
		return;

	pdev = wlan_usb_data->pdev;
	mutex_lock(&sunxi_wlan_usb_mutex);
	if (on_off != wlan_usb_data->power_state) {
		ret = sunxi_wlan_usb_on(wlan_usb_data, on_off);
		if (ret)
			dev_err(&pdev->dev, "set power failed\n");
	}
	mutex_unlock(&sunxi_wlan_usb_mutex);
}
EXPORT_SYMBOL_GPL(sunxi_wlan_usb_set_power);

int sunxi_wlan_usb_get_bus_index(void)
{
	struct platform_device *pdev;
	if (!wlan_usb_data)
		return -EINVAL;

	pdev = wlan_usb_data->pdev;
	dev_info(&pdev->dev, "usb_bus_index: %d\n", wlan_usb_data->usb_bus_index);
	return wlan_usb_data->usb_bus_index;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_usb_get_bus_index);

int sunxi_wlan_usb_get_oob_irq(void)
{
	struct platform_device *pdev;
	int host_oob_irq = 0;
	if (!wlan_usb_data || !gpio_is_valid(wlan_usb_data->gpio_wlan_usb_det))
		return 0;

	pdev = wlan_usb_data->pdev;

	host_oob_irq = gpio_to_irq(wlan_usb_data->gpio_wlan_usb_det);
	if (IS_ERR_VALUE(host_oob_irq))
		dev_err(&pdev->dev, "map gpio [%d] to virq failed, errno = %d\n",
			wlan_usb_data->gpio_wlan_usb_det, host_oob_irq);

	return host_oob_irq;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_usb_get_oob_irq);

int sunxi_wlan_usb_get_oob_irq_flags(void)
{
	int oob_irq_flags;
	if (!wlan_usb_data)
		return 0;

	oob_irq_flags = (IRQF_TRIGGER_HIGH | IRQF_SHARED | IRQF_NO_SUSPEND);

	return oob_irq_flags;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_usb_get_oob_irq_flags);

static int sunxi_wlan_usb_on(struct sunxi_wlan_usb_platdata *data, bool on_off)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (!on_off && gpio_is_valid(data->gpio_wlan_usb_drv))
		gpio_set_value(data->gpio_wlan_usb_drv, 0);

	if (data->wlan_usb_power_name) {
		data->wlan_usb_power = regulator_get(dev, data->wlan_usb_power_name);
		if (!IS_ERR(data->wlan_usb_power)) {
			if (on_off) {
				ret = regulator_enable(data->wlan_usb_power);
				if (ret < 0) {
					dev_err(dev, "regulator wlan_power enable failed\n");
					regulator_put(data->wlan_usb_power);
					return ret;
				}

				ret = regulator_get_voltage(data->wlan_usb_power);
				if (ret < 0) {
					dev_err(dev, "regulator wlan_power get voltage failed\n");
					regulator_put(data->wlan_usb_power);
					return ret;
				}
				dev_info(dev, "check wlan wlan_power voltage: %d\n",
					ret);
			} else{
				ret = regulator_disable(data->wlan_usb_power);
				if (ret < 0) {
					dev_err(dev, "regulator wlan_power disable failed\n");
					regulator_put(data->wlan_usb_power);
					return ret;
				}
			}
			regulator_put(data->wlan_usb_power);
		}
	}

	if (on_off && gpio_is_valid(data->gpio_wlan_usb_drv)) {
		mdelay(10);
		gpio_set_value(data->gpio_wlan_usb_drv, 1);
	}
	wlan_usb_data->power_state = on_off;

	return 0;
}

static ssize_t power_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", wlan_usb_data->power_state);
}

static ssize_t power_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	int err;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	if (state > 1)
		return -EINVAL;

	mutex_lock(&sunxi_wlan_usb_mutex);
	if (state != wlan_usb_data->power_state) {
		err = sunxi_wlan_usb_on(wlan_usb_data, state);
		if (err)
			dev_err(dev, "set power failed\n");
	}
	mutex_unlock(&sunxi_wlan_usb_mutex);

	return count;
}

static DEVICE_ATTR(power_state, S_IRUGO | S_IWUSR,
	power_state_show, power_state_store);

static int sunxi_wlan_usb_probe(struct platform_device *pdev)
{
	struct pinctrl *pinctrl;
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct sunxi_wlan_usb_platdata *data;
	struct gpio_config config;
	const char *power;
	int ret = 0;
	u32 val;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	data->pdev = pdev;
	wlan_usb_data = data;

	data->usb_bus_index = -1;
	if (!of_property_read_u32(np, "wlan_usb_busnum", &val)) {
		switch (val) {
		case 0:
		case 1:
		case 2:
			data->usb_bus_index = val;
			break;
		default:
			dev_err(dev, "unsupported wlan_busnum (%u)\n", val);
			return -EINVAL;
		}
	}
	dev_info(dev, "wlan_usb_busnum (%u)\n", val);

	if (of_property_read_string(np, "wlan_usb_power", &power)) {
		dev_warn(dev, "Missing wlan_power.\n");
	} else{
		data->wlan_usb_power_name = devm_kzalloc(dev, 64, GFP_KERNEL);
		if (!data->wlan_usb_power_name)
			return -ENOMEM;
		else
			strcpy(data->wlan_usb_power_name, power);
	}
	dev_info(dev, "wlan_usb_power_name (%s)\n", data->wlan_usb_power_name);

	/* request device pinctrl, set as default state */
	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR_OR_NULL(pinctrl)) {
		dev_err(dev, "request pincrtl handle for device [%s] failed\n",
			dev_name(&pdev->dev));
	}

	data->gpio_wlan_usb_drv = of_get_named_gpio_flags(np, "wlan_usb_drv", 0,
		(enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_wlan_usb_drv)) {
		dev_err(dev, "get gpio wlan_regon failed\n");
	} else{
		dev_info(dev,
				"wlan_regon gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_wlan_usb_drv,
				"wlan_usb_drv");
		if (ret < 0) {
			dev_err(dev, "can't request gpio_wlan_usb_drv gpio %d\n",
				data->gpio_wlan_usb_drv);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_wlan_usb_drv, 0);
		if (ret < 0) {
			dev_err(dev, "can't request output direction gpio_wlan_usb_drv gpio %d\n",
				data->gpio_wlan_usb_drv);
			return ret;
		}
	}

	data->gpio_wlan_usb_det = of_get_named_gpio_flags(np, "wlan_usb_det",
		0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_wlan_usb_det)) {
		dev_err(dev, "get gpio wlan_hostwake failed\n");
	} else{
		dev_info(dev, "wlan_hostwake gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_wlan_usb_det,
			"wlan_usb_det");
		if (ret < 0) {
			dev_err(dev, "can't request gpio_wlan_usb_detgpio %d\n",
				data->gpio_wlan_usb_det);
			return ret;
		}

		gpio_direction_input(data->gpio_wlan_usb_det);
		if (ret < 0) {
			dev_err(dev, "can't request input direction gpio_wlan_usb_det gpio %d\n",
				data->gpio_wlan_usb_det);
			return ret;
		}
	}

	device_create_file(dev, &dev_attr_power_state);
	data->power_state = 0;

	return 0;
}

static int sunxi_wlan_usb_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_power_state);
	return 0;
}

static const struct of_device_id sunxi_wlan_usb_ids[] = {
	{ .compatible = "allwinner,sunxi-wlan-usb" },
	{ /* Sentinel */ }
};

static struct platform_driver sunxi_wlan_usb_driver = {
	.probe		= sunxi_wlan_usb_probe,
	.remove	= sunxi_wlan_usb_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "sunxi-wlan-usb",
		.of_match_table	= sunxi_wlan_usb_ids,
	},
};

module_platform_driver(sunxi_wlan_usb_driver);

MODULE_DESCRIPTION("sunxi usb wlan driver");
MODULE_LICENSE(GPL);
