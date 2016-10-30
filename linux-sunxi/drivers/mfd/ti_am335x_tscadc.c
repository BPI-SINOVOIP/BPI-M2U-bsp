/*
 * TI Touch Screen / ADC MFD driver
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/mfd/core.h>
#include <linux/pm_runtime.h>

#include <linux/mfd/ti_am335x_tscadc.h>
#include <linux/input/ti_am335x_tsc.h>
#include <linux/platform_data/ti_am335x_adc.h>

static unsigned int tscadc_readl(struct ti_tscadc_dev *tsadc, unsigned int reg)
{
	unsigned int val;

	regmap_read(tsadc->regmap_tscadc, reg, &val);
	return val;
}

static void tscadc_writel(struct ti_tscadc_dev *tsadc, unsigned int reg,
					unsigned int val)
{
	regmap_write(tsadc->regmap_tscadc, reg, val);
}

static const struct regmap_config tscadc_regmap_config = {
	.name = "ti_tscadc",
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
};

static void tscadc_idle_config(struct ti_tscadc_dev *config)
{
	unsigned int idleconfig;

	idleconfig = STEPCONFIG_YNN | STEPCONFIG_INM_ADCREFM |
			STEPCONFIG_INP_ADCREFM | STEPCONFIG_YPN;

	tscadc_writel(config, REG_IDLECONFIG, idleconfig);
}

static	int ti_tscadc_probe(struct platform_device *pdev)
{
	struct ti_tscadc_dev	*tscadc;
	struct resource		*res;
	struct clk		*clk;
	struct mfd_tscadc_board	*pdata = pdev->dev.platform_data;
	struct mfd_cell		*cell;
	int			err, ctrl;
	int			clk_value, clock_rate;
	int			tsc_wires, adc_channels = 0, total_channels;

	if (!pdata) {
		dev_err(&pdev->dev, "Could not find platform data\n");
		return -EINVAL;
	}

	if (pdata->adc_init)
		adc_channels = pdata->adc_init->adc_channels;

	tsc_wires = pdata->tsc_init->wires;
	total_channels = tsc_wires + adc_channels;

	if (total_channels > 8) {
		dev_err(&pdev->dev, "Number of i/p channels more than 8\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource defined.\n");
		return -EINVAL;
	}

	/* Allocate memory for device */
	tscadc = devm_kzalloc(&pdev->dev,
			sizeof(struct ti_tscadc_dev), GFP_KERNEL);
	if (!tscadc) {
		dev_err(&pdev->dev, "failed to allocate memory.\n");
		return -ENOMEM;
	}
	tscadc->dev = &pdev->dev;

	err = platform_get_irq(pdev, 0);
	if (err < 0) {
		dev_err(&pdev->dev, "no irq ID is specified.\n");
		goto ret;
	} else
		tscadc->irq = err;

	res = devm_request_mem_region(&pdev->dev,
			res->start, resource_size(res), pdev->name);
	if (!res) {
		dev_err(&pdev->dev, "failed to reserve registers.\n");
		return -EBUSY;
	}

	tscadc->tscadc_base = devm_ioremap(&pdev->dev,
			res->start, resource_size(res));
	if (!tscadc->tscadc_base) {
		dev_err(&pdev->dev, "failed to map registers.\n");
		return -ENOMEM;
	}

	tscadc->regmap_tscadc = devm_regmap_init_mmio(&pdev->dev,
			tscadc->tscadc_base, &tscadc_regmap_config);
	if (IS_ERR(tscadc->regmap_tscadc)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		err = PTR_ERR(tscadc->regmap_tscadc);
		goto ret;
	}

	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	/*
	 * The TSC_ADC_Subsystem has 2 clock domains
	 * OCP_CLK and ADC_CLK.
	 * The ADC clock is expected to run at target of 3MHz,
	 * and expected to capture 12-bit data at a rate of 200 KSPS.
	 * The TSC_ADC_SS controller design assumes the OCP clock is
	 * at least 6x faster than the ADC clock.
	 */
	clk = clk_get(&pdev->dev, "adc_tsc_fck");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get TSC fck\n");
		err = PTR_ERR(clk);
		goto err_disable_clk;
	}
	clock_rate = clk_get_rate(clk);
	clk_put(clk);
	clk_value = clock_rate / ADC_CLK;
	if (clk_value < MAX_CLK_DIV) {
		dev_err(&pdev->dev, "clock input less than min clock requirement\n");
		err = -EINVAL;
		goto err_disable_clk;
	}
	/* TSCADC_CLKDIV needs to be configured to the value minus 1 */
	clk_value = clk_value - 1;
	tscadc_writel(tscadc, REG_CLKDIV, clk_value);

	/* Set the control register bits */
	ctrl = CNTRLREG_STEPCONFIGWRT |
			CNTRLREG_TSCENB |
			CNTRLREG_STEPID |
			CNTRLREG_4WIRE;
	tscadc_writel(tscadc, REG_CTRL, ctrl);

	/* Set register bits for Idle Config Mode */
	tscadc_idle_config(tscadc);

	/* Enable the TSC module enable bit */
	ctrl = tscadc_readl(tscadc, REG_CTRL);
	ctrl |= CNTRLREG_TSCSSENB;
	tscadc_writel(tscadc, REG_CTRL, ctrl);

	/* TSC Cell */
	cell = &tscadc->cells[TSC_CELL];
	cell->name = "tsc";
	cell->platform_data = tscadc;
	cell->pdata_size = sizeof(*tscadc);

	/* ADC Cell */
	cell = &tscadc->cells[ADC_CELL];
	cell->name = "tiadc";
	cell->platform_data = tscadc;
	cell->pdata_size = sizeof(*tscadc);

	err = mfd_add_devices(&pdev->dev, pdev->id, tscadc->cells,
			TSCADC_CELLS, NULL, 0, NULL);
	if (err < 0)
		goto err_disable_clk;

	device_init_wakeup(&pdev->dev, true);
	platform_set_drvdata(pdev, tscadc);

	return 0;

err_disable_clk:
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);
ret:
	return err;
}

static int ti_tscadc_remove(struct platform_device *pdev)
{
	struct ti_tscadc_dev	*tscadc = platform_get_drvdata(pdev);

	tscadc_writel(tscadc, REG_SE, 0x00);

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	mfd_remove_devices(tscadc->dev);

	return 0;
}

#ifdef CONFIG_PM
static int tscadc_suspend(struct device *dev)
{
	struct ti_tscadc_dev	*tscadc_dev = dev_get_drvdata(dev);

	tscadc_writel(tscadc_dev, REG_SE, 0x00);
	pm_runtime_put_sync(dev);

	return 0;
}

static int tscadc_resume(struct device *dev)
{
	struct ti_tscadc_dev	*tscadc_dev = dev_get_drvdata(dev);
	unsigned int restore, ctrl;

	pm_runtime_get_sync(dev);

	/* context restore */
	ctrl = CNTRLREG_STEPCONFIGWRT | CNTRLREG_TSCENB |
			CNTRLREG_STEPID | CNTRLREG_4WIRE;
	tscadc_writel(tscadc_dev, REG_CTRL, ctrl);
	tscadc_idle_config(tscadc_dev);
	tscadc_writel(tscadc_dev, REG_SE, STPENB_STEPENB);
	restore = tscadc_readl(tscadc_dev, REG_CTRL);
	tscadc_writel(tscadc_dev, REG_CTRL,
			(restore | CNTRLREG_TSCSSENB));

	return 0;
}

static const struct dev_pm_ops tscadc_pm_ops = {
	.suspend = tscadc_suspend,
	.resume = tscadc_resume,
};
#define TSCADC_PM_OPS (&tscadc_pm_ops)
#else
#define TSCADC_PM_OPS NULL
#endif

static struct platform_driver ti_tscadc_driver = {
	.driver = {
		.name   = "ti_tscadc",
		.owner	= THIS_MODULE,
		.pm	= TSCADC_PM_OPS,
	},
	.probe	= ti_tscadc_probe,
	.remove	= ti_tscadc_remove,

};

module_platform_driver(ti_tscadc_driver);

MODULE_DESCRIPTION("TI touchscreen / ADC MFD controller driver");
MODULE_AUTHOR("Rachna Patil <rachna@ti.com>");
MODULE_LICENSE("GPL");
