/*
 * ACPI support for Intel Lynxpoint LPSS.
 *
 * Copyright (C) 2013, Intel Corporation
 * Authors: Mika Westerberg <mika.westerberg@linux.intel.com>
 *          Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/platform_data/clk-lpss.h>
#include <linux/pm_runtime.h>

#include "internal.h"

ACPI_MODULE_NAME("acpi_lpss");

#define LPSS_CLK_SIZE	0x04
#define LPSS_LTR_SIZE	0x18

/* Offsets relative to LPSS_PRIVATE_OFFSET */
#define LPSS_GENERAL			0x08
#define LPSS_GENERAL_LTR_MODE_SW	BIT(2)
#define LPSS_SW_LTR			0x10
#define LPSS_AUTO_LTR			0x14

struct lpss_device_desc {
	bool clk_required;
	const char *clkdev_name;
	bool ltr_required;
	unsigned int prv_offset;
};

static struct lpss_device_desc lpss_dma_desc = {
	.clk_required = true,
	.clkdev_name = "hclk",
};

struct lpss_private_data {
	void __iomem *mmio_base;
	resource_size_t mmio_size;
	struct clk *clk;
	const struct lpss_device_desc *dev_desc;
};

static struct lpss_device_desc lpt_dev_desc = {
	.clk_required = true,
	.prv_offset = 0x800,
	.ltr_required = true,
};

static struct lpss_device_desc lpt_sdio_dev_desc = {
	.prv_offset = 0x1000,
	.ltr_required = true,
};

static const struct acpi_device_id acpi_lpss_device_ids[] = {
	/* Generic LPSS devices */
	{ "INTL9C60", (unsigned long)&lpss_dma_desc },

	/* Lynxpoint LPSS devices */
	{ "INT33C0", (unsigned long)&lpt_dev_desc },
	{ "INT33C1", (unsigned long)&lpt_dev_desc },
	{ "INT33C2", (unsigned long)&lpt_dev_desc },
	{ "INT33C3", (unsigned long)&lpt_dev_desc },
	{ "INT33C4", (unsigned long)&lpt_dev_desc },
	{ "INT33C5", (unsigned long)&lpt_dev_desc },
	{ "INT33C6", (unsigned long)&lpt_sdio_dev_desc },
	{ "INT33C7", },

	{ }
};

static int is_memory(struct acpi_resource *res, void *not_used)
{
	struct resource r;
	return !acpi_dev_resource_memory(res, &r);
}

/* LPSS main clock device. */
static struct platform_device *lpss_clk_dev;

static inline void lpt_register_clock_device(void)
{
	lpss_clk_dev = platform_device_register_simple("clk-lpt", -1, NULL, 0);
}

static int register_device_clock(struct acpi_device *adev,
				 struct lpss_private_data *pdata)
{
	const struct lpss_device_desc *dev_desc = pdata->dev_desc;
	struct lpss_clk_data *clk_data;

	if (!lpss_clk_dev)
		lpt_register_clock_device();

	clk_data = platform_get_drvdata(lpss_clk_dev);
	if (!clk_data)
		return -ENODEV;

	if (dev_desc->clkdev_name) {
		clk_register_clkdev(clk_data->clk, dev_desc->clkdev_name,
				    dev_name(&adev->dev));
		return 0;
	}

	if (!pdata->mmio_base
	    || pdata->mmio_size < dev_desc->prv_offset + LPSS_CLK_SIZE)
		return -ENODATA;

	pdata->clk = clk_register_gate(NULL, dev_name(&adev->dev),
				       clk_data->name, 0,
				       pdata->mmio_base + dev_desc->prv_offset,
				       0, 0, NULL);
	if (IS_ERR(pdata->clk))
		return PTR_ERR(pdata->clk);

	clk_register_clkdev(pdata->clk, NULL, dev_name(&adev->dev));
	return 0;
}

static int acpi_lpss_create_device(struct acpi_device *adev,
				   const struct acpi_device_id *id)
{
	struct lpss_device_desc *dev_desc;
	struct lpss_private_data *pdata;
	struct resource_list_entry *rentry;
	struct list_head resource_list;
	int ret;

	dev_desc = (struct lpss_device_desc *)id->driver_data;
	if (!dev_desc)
		return acpi_create_platform_device(adev, id);

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	INIT_LIST_HEAD(&resource_list);
	ret = acpi_dev_get_resources(adev, &resource_list, is_memory, NULL);
	if (ret < 0)
		goto err_out;

	list_for_each_entry(rentry, &resource_list, node)
		if (resource_type(&rentry->res) == IORESOURCE_MEM) {
			pdata->mmio_size = resource_size(&rentry->res);
			pdata->mmio_base = ioremap(rentry->res.start,
						   pdata->mmio_size);
			break;
		}

	acpi_dev_free_resource_list(&resource_list);

	pdata->dev_desc = dev_desc;

	if (dev_desc->clk_required) {
		ret = register_device_clock(adev, pdata);
		if (ret) {
			/* Skip the device, but continue the namespace scan. */
			ret = 0;
			goto err_out;
		}
	}

	/*
	 * This works around a known issue in ACPI tables where LPSS devices
	 * have _PS0 and _PS3 without _PSC (and no power resources), so
	 * acpi_bus_init_power() will assume that the BIOS has put them into D0.
	 */
	ret = acpi_device_fix_up_power(adev);
	if (ret) {
		/* Skip the device, but continue the namespace scan. */
		ret = 0;
		goto err_out;
	}

	adev->driver_data = pdata;
	ret = acpi_create_platform_device(adev, id);
	if (ret > 0)
		return ret;

	adev->driver_data = NULL;

 err_out:
	kfree(pdata);
	return ret;
}

static int lpss_reg_read(struct device *dev, unsigned int reg, u32 *val)
{
	struct acpi_device *adev;
	struct lpss_private_data *pdata;
	unsigned long flags;
	int ret;

	ret = acpi_bus_get_device(ACPI_HANDLE(dev), &adev);
	if (WARN_ON(ret))
		return ret;

	spin_lock_irqsave(&dev->power.lock, flags);
	if (pm_runtime_suspended(dev)) {
		ret = -EAGAIN;
		goto out;
	}
	pdata = acpi_driver_data(adev);
	if (WARN_ON(!pdata || !pdata->mmio_base)) {
		ret = -ENODEV;
		goto out;
	}
	*val = readl(pdata->mmio_base + pdata->dev_desc->prv_offset + reg);

 out:
	spin_unlock_irqrestore(&dev->power.lock, flags);
	return ret;
}

static ssize_t lpss_ltr_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	u32 ltr_value = 0;
	unsigned int reg;
	int ret;

	reg = strcmp(attr->attr.name, "auto_ltr") ? LPSS_SW_LTR : LPSS_AUTO_LTR;
	ret = lpss_reg_read(dev, reg, &ltr_value);
	if (ret)
		return ret;

	return snprintf(buf, PAGE_SIZE, "%08x\n", ltr_value);
}

static ssize_t lpss_ltr_mode_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	u32 ltr_mode = 0;
	char *outstr;
	int ret;

	ret = lpss_reg_read(dev, LPSS_GENERAL, &ltr_mode);
	if (ret)
		return ret;

	outstr = (ltr_mode & LPSS_GENERAL_LTR_MODE_SW) ? "sw" : "auto";
	return sprintf(buf, "%s\n", outstr);
}

static DEVICE_ATTR(auto_ltr, S_IRUSR, lpss_ltr_show, NULL);
static DEVICE_ATTR(sw_ltr, S_IRUSR, lpss_ltr_show, NULL);
static DEVICE_ATTR(ltr_mode, S_IRUSR, lpss_ltr_mode_show, NULL);

static struct attribute *lpss_attrs[] = {
	&dev_attr_auto_ltr.attr,
	&dev_attr_sw_ltr.attr,
	&dev_attr_ltr_mode.attr,
	NULL,
};

static struct attribute_group lpss_attr_group = {
	.attrs = lpss_attrs,
	.name = "lpss_ltr",
};

static int acpi_lpss_platform_notify(struct notifier_block *nb,
				     unsigned long action, void *data)
{
	struct platform_device *pdev = to_platform_device(data);
	struct lpss_private_data *pdata;
	struct acpi_device *adev;
	const struct acpi_device_id *id;
	int ret = 0;

	id = acpi_match_device(acpi_lpss_device_ids, &pdev->dev);
	if (!id || !id->driver_data)
		return 0;

	if (acpi_bus_get_device(ACPI_HANDLE(&pdev->dev), &adev))
		return 0;

	pdata = acpi_driver_data(adev);
	if (!pdata || !pdata->mmio_base || !pdata->dev_desc->ltr_required)
		return 0;

	if (pdata->mmio_size < pdata->dev_desc->prv_offset + LPSS_LTR_SIZE) {
		dev_err(&pdev->dev, "MMIO size insufficient to access LTR\n");
		return 0;
	}

	if (action == BUS_NOTIFY_ADD_DEVICE)
		ret = sysfs_create_group(&pdev->dev.kobj, &lpss_attr_group);
	else if (action == BUS_NOTIFY_DEL_DEVICE)
		sysfs_remove_group(&pdev->dev.kobj, &lpss_attr_group);

	return ret;
}

static struct notifier_block acpi_lpss_nb = {
	.notifier_call = acpi_lpss_platform_notify,
};

static struct acpi_scan_handler lpss_handler = {
	.ids = acpi_lpss_device_ids,
	.attach = acpi_lpss_create_device,
};

void __init acpi_lpss_init(void)
{
	if (!lpt_clk_init()) {
		bus_register_notifier(&platform_bus_type, &acpi_lpss_nb);
		acpi_scan_add_handler(&lpss_handler);
	}
}
