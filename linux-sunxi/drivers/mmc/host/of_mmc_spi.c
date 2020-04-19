/*
 * OpenFirmware bindings for the MMC-over-SPI driver
 *
 * Copyright (c) MontaVista Software, Inc. 2008.
 *
 * Author: Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/spi/spi.h>
#include <linux/spi/mmc_spi.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/sys_config.h>
#include <linux/pinctrl/pinconf-sunxi.h>

/* For archs that don't support NO_IRQ (such as mips), provide a dummy value */
#ifndef NO_IRQ
#define NO_IRQ 0
#endif

MODULE_LICENSE("GPL");

enum {
	CD_GPIO = 0,
	WP_GPIO,
	NUM_GPIOS,
};

struct of_mmc_spi {
	int gpios[NUM_GPIOS];
	bool alow_gpios[NUM_GPIOS];
	int detect_irq;
	struct mmc_spi_platform_data pdata;
};

static struct of_mmc_spi *to_of_mmc_spi(struct device *dev)
{
	return container_of(dev->platform_data, struct of_mmc_spi, pdata);
}

static int of_mmc_spi_read_gpio(struct device *dev, int gpio_num)
{
	struct of_mmc_spi *oms = to_of_mmc_spi(dev);
	bool active_low = oms->alow_gpios[gpio_num];
	bool value;

#if defined(CONFIG_ARCH_SUN3IW1P1_R6) && defined(CONFIG_IO_EXPAND)
	u32 use_io_expand;
	struct device_node *np = dev->of_node;
	if (!of_property_read_u32(np, "cd-io_expand", &use_io_expand)
			&& use_io_expand)
		value = gpio_get_value_cansleep(oms->gpios[gpio_num]);
	else
#endif
		value = gpio_get_value(oms->gpios[gpio_num]);

	return active_low ^ value;
}

static int of_mmc_spi_get_cd(struct device *dev)
{
	return of_mmc_spi_read_gpio(dev, CD_GPIO);
}

static int of_mmc_spi_get_ro(struct device *dev)
{
	return of_mmc_spi_read_gpio(dev, WP_GPIO);
}

static int of_mmc_spi_init(struct device *dev,
			   irqreturn_t (*irqhandler)(int, void *), void *mmc)
{
	struct of_mmc_spi *oms = to_of_mmc_spi(dev);

#if defined(CONFIG_ARCH_SUNXI)
	return request_irq(oms->detect_irq, irqhandler,
					IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
					dev_name(dev), mmc);
#else
	return request_threaded_irq(oms->detect_irq, NULL, irqhandler, 0,
					dev_name(dev), mmc);
#endif
}

static void of_mmc_spi_exit(struct device *dev, void *mmc)
{
	struct of_mmc_spi *oms = to_of_mmc_spi(dev);

	free_irq(oms->detect_irq, mmc);
}

struct mmc_spi_platform_data *mmc_spi_get_pdata(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct device_node *np = dev->of_node;
	struct of_mmc_spi *oms;
	const u32 *voltage_ranges;
	int num_ranges;
	int i;
	int ret = -EINVAL;
#if defined(CONFIG_ARCH_SUNXI)
	struct gpio_config flags;
	int len;
#endif

	if (dev->platform_data || !np)
		return dev->platform_data;

	oms = kzalloc(sizeof(*oms), GFP_KERNEL);
	if (!oms)
		return NULL;

	voltage_ranges = of_get_property(np, "voltage-ranges", &num_ranges);
	num_ranges = num_ranges / sizeof(*voltage_ranges) / 2;
	if (!voltage_ranges || !num_ranges) {
		dev_err(dev, "OF: voltage-ranges unspecified\n");
		goto err_ocr;
	}

	for (i = 0; i < num_ranges; i++) {
		const int j = i * 2;
		u32 mask;

		mask = mmc_vddrange_to_ocrmask(be32_to_cpu(voltage_ranges[j]),
						be32_to_cpu(voltage_ranges[j + 1]));
		if (!mask) {
			ret = -EINVAL;
			dev_err(dev, "OF: voltage-range #%d is invalid\n", i);
			goto err_ocr;
		}
		oms->pdata.ocr_mask |= mask;
	}

#if defined(CONFIG_ARCH_SUNXI)
#if defined(CONFIG_ARCH_SUN3IW1P1_R6) && defined(CONFIG_IO_EXPAND)
	u32 use_io_expand;
	if (!of_property_read_u32(np, "cd-io_expand", &use_io_expand)) {
		if (use_io_expand) {
			if (!of_property_read_u32(np, "cd-gpio",
						&(oms->gpios[CD_GPIO]))) {
				if (!gpio_is_valid(oms->gpios[CD_GPIO]) ||
					gpio_request(oms->gpios[CD_GPIO],
							dev_name(dev)) < 0) {
					dev_err(dev, "\"cd-gpio\" property is invalid\n");
					oms->gpios[CD_GPIO] = -EINVAL;
				}
				oms->alow_gpios[CD_GPIO] = true;
			} else {
				dev_err(dev, "\"cd-gpio\" property is missing\n");
				oms->gpios[CD_GPIO] = -EINVAL;
			}
		} else {
#endif
			oms->gpios[CD_GPIO] =
				of_get_named_gpio_flags(np, "cd-gpio", 0, (enum of_gpio_flags *)&flags);
			if (gpio_is_valid(oms->gpios[CD_GPIO])) {
				char pin_name[SUNXI_PIN_NAME_MAX_LEN];
				long unsigned int config_set;
				sunxi_gpio_to_name(oms->gpios[CD_GPIO], pin_name);
				/* function */
				config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, flags.mul_sel);
				pin_config_set(SUNXI_PINCTRL, pin_name, config_set);
				/* pull */
				config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, flags.pull);
				pin_config_set(SUNXI_PINCTRL, pin_name, config_set);

				oms->alow_gpios[CD_GPIO] = true;
			}
#if defined(CONFIG_ARCH_SUN3IW1P1_R6) && defined(CONFIG_IO_EXPAND)
		}
	}
#endif
	oms->gpios[WP_GPIO] = -EINVAL;
#else
	for (i = 0; i < ARRAY_SIZE(oms->gpios); i++) {
		enum of_gpio_flags gpio_flags;

		oms->gpios[i] = of_get_gpio_flags(np, i, &gpio_flags);
		if (!gpio_is_valid(oms->gpios[i]))
			continue;

		ret = gpio_request(oms->gpios[i], dev_name(dev));
		if (ret < 0) {
			oms->gpios[i] = -EINVAL;
			continue;
		}

		if (gpio_flags & OF_GPIO_ACTIVE_LOW)
			oms->alow_gpios[i] = true;
	}
#endif

	if (gpio_is_valid(oms->gpios[CD_GPIO]))
		oms->pdata.get_cd = of_mmc_spi_get_cd;
	if (gpio_is_valid(oms->gpios[WP_GPIO]))
		oms->pdata.get_ro = of_mmc_spi_get_ro;

#if defined(CONFIG_ARCH_SUNXI)
	if (of_find_property(np, "interrupt-cd", &len))
		oms->detect_irq = gpio_to_irq(oms->gpios[CD_GPIO]);
#else
	oms->detect_irq = irq_of_parse_and_map(np, 0);
#endif
	if (oms->detect_irq != 0) {
		oms->pdata.init = of_mmc_spi_init;
		oms->pdata.exit = of_mmc_spi_exit;
	} else {
		oms->pdata.caps |= MMC_CAP_NEEDS_POLL;
	}

	dev->platform_data = &oms->pdata;
	return dev->platform_data;
err_ocr:
	kfree(oms);
	return NULL;
}
EXPORT_SYMBOL(mmc_spi_get_pdata);

void mmc_spi_put_pdata(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct device_node *np = dev->of_node;
	struct of_mmc_spi *oms = to_of_mmc_spi(dev);
	int i;

	if (!dev->platform_data || !np)
		return;

	for (i = 0; i < ARRAY_SIZE(oms->gpios); i++) {
		if (gpio_is_valid(oms->gpios[i]))
			gpio_free(oms->gpios[i]);
	}
	kfree(oms);
	dev->platform_data = NULL;
}
EXPORT_SYMBOL(mmc_spi_put_pdata);
