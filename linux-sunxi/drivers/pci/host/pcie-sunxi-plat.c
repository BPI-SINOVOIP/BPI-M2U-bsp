/*
 * PCIe RC driver for Allwinner Core
 *
 * Copyright (C) 2016 Allwinner Co., Ltd.
 *
 * Author: wangjx <wangjx@allwinnertech.com>
 *	   ysn <yangshounan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/sys_config.h>

#include "pcie-sunxi.h"

#define USER_DEFINED_REGISTER_LIST	0x1000
#define PCIE_LTSSM_ENABLE		0x00
#define PCIE_INT_PENDING		0x18
#define PCIE_AWMISC_INF0_CTRL		0x30
#define PCIE_ARMISC_INF0_CTRL		0X34
#define PCIE_LINK_STATUS		0X43C
#define PCIE_PHY_CFG			0x800
#define SYS_CLK				0
#define PAD_CLK				1
#define RDLH_LINK_UP			(1<<1)
#define SMLH_LINK_UP			(1<<0)
#define PCIE_LINK_TRAINING		(1<<0)
#define PCIE_LINK_UP_MASK		(0x3<<16)

struct sunxi_pcie {
	void __iomem		*dbi_base;
	void __iomem		*app_base;
	int			link_irq;
	int			msi_irq;
	int			speed_gen;
	struct pcie_port	pp;
	struct clk		*pcie_ref;
	struct clk		*pcie_axi;
	struct clk		*pcie_aux;
	struct clk		*pcie_bus;
};
#define to_sunxi_pcie(x)  container_of((x), struct sunxi_pcie, pp)

static inline u32 sunxi_pcie_readl(struct sunxi_pcie *pcie, u32 offset)
{
	return readl(pcie->app_base + offset);
}

static inline void sunxi_pcie_writel(u32 val, struct sunxi_pcie *pcie, u32 offset)
{
	writel(val, pcie->app_base + offset);
}

static inline void sunxi_pcie_writel_rcl(struct pcie_port *pp, u32 val, u32 reg)
{
	writel(val, pp->dbi_base + reg);
}

static inline u32 sunxi_pcie_readl_rcl(struct pcie_port *pp, u32 reg)
{
	return readl(pp->dbi_base + reg);
}

static void sunxi_pcie_clk_clect(struct pcie_port *pp, char on)
{
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);
	u32 val;

	val = sunxi_pcie_readl(pcie, PCIE_PHY_CFG);
	if (on)
		val |= 0x1<<31;
	else
		val &= ~(0x1<<31);
	sunxi_pcie_writel(val, pcie, PCIE_PHY_CFG);
}

static void sunxi_pcie_ltssm_enable(struct pcie_port *pp)
{
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);
	u32 val;

	val = sunxi_pcie_readl(pcie, PCIE_LTSSM_ENABLE);
	val |= PCIE_LINK_TRAINING;
	sunxi_pcie_writel(val, pcie, PCIE_LTSSM_ENABLE);
}

static void sunxi_pcie_irqpending(struct pcie_port *pp)
{
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);
	u32 val;

	val = sunxi_pcie_readl(pcie, PCIE_INT_PENDING);
	val &= ~(0x3<<16);
	sunxi_pcie_writel(val, pcie, PCIE_INT_PENDING);
}

static void sunxi_pcie_irqmask(struct pcie_port *pp)
{
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);
	u32 val;

	val = sunxi_pcie_readl(pcie, PCIE_INT_PENDING);
	val |= 0x3<<16;
	sunxi_pcie_writel(val, pcie, PCIE_INT_PENDING);
}

static void sunxi_pcie_ltssm_disable(struct pcie_port *pp)
{
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);
	u32 val;

	val = sunxi_pcie_readl(pcie, PCIE_LTSSM_ENABLE);
	val &= ~PCIE_LINK_TRAINING;
	sunxi_pcie_writel(val, pcie, PCIE_LTSSM_ENABLE);
}

static int sunxi_pcie_wait_for_speed_change(struct pcie_port *pp)
{
	u32 tmp;
	unsigned int retries;

	for (retries = 0; retries < 200; retries++) {
		tmp = sunxi_pcie_readl_rcl(pp, PCIE_LINK_WIDTH_SPEED_CONTROL);
		/* Test if the speed change finished. */
		if (!(tmp & PORT_LOGIC_SPEED_CHANGE))
			return 0;
		usleep_range(100, 1000);
	}

	dev_err(pp->dev, "Speed change timeout\n");
	return -EINVAL;
}

int sunxi_pcie_wait_for_link(struct pcie_port *pp)
{
	int retries;
	/* check if the link is up or not */
	for (retries = 0; retries < LINK_WAIT_MAX_RETRIES; retries++) {
		if (sunxi_pcie_link_up(pp)) {
			dev_info(pp->dev, "link up\n");
			return 0;
		}
		usleep_range(LINK_WAIT_USLEEP_MIN, LINK_WAIT_USLEEP_MAX);
	}

	dev_err(pp->dev, "phy link never came up\n");

	return -ETIMEDOUT;
}

static int sunxi_pcie_establish_link(struct pcie_port *pp)
{
	if (sunxi_pcie_link_up(pp)) {
		dev_err(pp->dev, "link is already up\n");
		return 0;
	}
	sunxi_pcie_ltssm_enable(pp);
	sunxi_pcie_wait_for_link(pp);

	return 1;
}

static int sunxi_pcie_link_up_status(struct pcie_port *pp)
{
	u32 rc;
	int ret;
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);

	rc = sunxi_pcie_readl(pcie, PCIE_LINK_STATUS);
	if ((rc&RDLH_LINK_UP) && (rc&SMLH_LINK_UP))
		ret = 1;
	else
		ret = 0;

	return ret;
}

static int sunxi_pcie_speed_chang(struct pcie_port *pp, int gen)
{
	int val;
	int ret;

	val = sunxi_pcie_readl_rcl(pp, LINK_CONTROL2_LINK_STATUS2);
	val &=  ~0xf;
	val |= gen;
	sunxi_pcie_writel_rcl(pp, val, LINK_CONTROL2_LINK_STATUS2);

	val = sunxi_pcie_readl_rcl(pp, PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_SPEED_CHANGE;
	sunxi_pcie_writel_rcl(pp, val, PCIE_LINK_WIDTH_SPEED_CONTROL);

	val = sunxi_pcie_readl_rcl(pp, PCIE_LINK_WIDTH_SPEED_CONTROL);
	val |= PORT_LOGIC_SPEED_CHANGE;
	sunxi_pcie_writel_rcl(pp, val, PCIE_LINK_WIDTH_SPEED_CONTROL);

	ret = sunxi_pcie_wait_for_speed_change(pp);
	if (!ret)
		dev_info(pp->dev, "PCI-e speed of Gen%d\n", gen);

	return 0;
}

static int sunxi_pcie_clk_setup(struct pcie_port *pp)
{
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);
	int ret;

#ifndef CONFIG_PCIE_SUNXI_EXTERNAL_CLOCK
	ret = clk_prepare_enable(pcie->pcie_ref);
	if (ret) {
		dev_err(pp->dev, "unable to enable pcie_ref clock\n");
		goto err_pcie_ref;
	}
#endif
	ret = clk_prepare_enable(pcie->pcie_axi);
	if (ret) {
		dev_err(pp->dev, "unable to enable pcie_axi clock\n");
		goto err_pcie_axi;
	}

	ret = clk_prepare_enable(pcie->pcie_aux);
	if (ret) {
		dev_err(pp->dev, "unable to enable pcie_aux clock\n");
		goto err_pcie_aux;
	}

	ret = clk_prepare_enable(pcie->pcie_bus);
	if (ret) {
		dev_err(pp->dev, "unable to enable pcie_bus clock\n");
		goto err_pcie_bus;
	}

	return 0;

err_pcie_bus:
	clk_disable_unprepare(pcie->pcie_aux);
err_pcie_aux:
	clk_disable_unprepare(pcie->pcie_axi);
err_pcie_axi:
#ifndef CONFIG_PCIE_SUNXI_EXTERNAL_CLOCK
	clk_disable_unprepare(pcie->pcie_ref);
err_pcie_ref:
#endif
	return ret;
}

static irqreturn_t sunxi_plat_pcie_msi_irq_handler(int irq, void *arg)
{
	struct pcie_port *pp = (struct pcie_port *)arg;

	return sunxi_handle_msi_irq(pp);
}

static irqreturn_t sunxi_plat_pcie_linkup_handler(int irq, void *arg)
{
	struct pcie_port *pp = (struct pcie_port *)arg;

	sunxi_pcie_irqpending(pp);

	return IRQ_HANDLED;
}

static int sunxi_pcie_gpio_reset(struct platform_device *pdev)
{
	struct gpio_config pcie_rst_gpio;
	struct device_node *np = pdev->dev.of_node;

	pcie_rst_gpio.gpio = of_get_named_gpio_flags(np, "pcie_perst", 0,
				(enum of_gpio_flags *)(&(pcie_rst_gpio)));

	if (0 != gpio_request(pcie_rst_gpio.gpio, NULL)) {
		pr_err("pcie_perst gpio_request is failed\n");
		return 0;
	}

	if (0 != gpio_direction_output(pcie_rst_gpio.gpio, 0)) {
		pr_err("pcie_perst gpio set 0 err!");
		return 0;
	}
	msleep(10);
	if (0 != gpio_direction_output(pcie_rst_gpio.gpio, 1)) {
		pr_err("pcie_perst gpio set 1 err!");
		return 0;
	}

	return 1;
}

static void sunxi_plat_pcie_host_init(struct pcie_port *pp)
{
	struct sunxi_pcie *pcie = to_sunxi_pcie(pp);

	sunxi_pcie_ltssm_disable(pp);
	sunxi_pcie_clk_clect(pp, PAD_CLK);

	sunxi_pcie_setup_rc(pp);

	sunxi_pcie_establish_link(pp);
	if (IS_ENABLED(CONFIG_PCI_MSI))
		sunxi_pcie_msi_init(pp);
	sunxi_pcie_speed_chang(pp, pcie->speed_gen);
}

static struct pcie_host_ops sunxi_plat_pcie_host_ops = {
	.link_up = sunxi_pcie_link_up_status,
	.host_init = sunxi_plat_pcie_host_init,
};

static int sunxi_plat_add_pcie_port(struct pcie_port *pp,
				 struct platform_device *pdev)
{
	int ret;

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		pp->msi_irq = platform_get_irq(pdev, 0);
		if (pp->msi_irq < 0)
			return pp->msi_irq;

		ret = devm_request_irq(&pdev->dev, pp->msi_irq,
					sunxi_plat_pcie_msi_irq_handler,
					IRQF_SHARED, "sunxi-plat-pcie-msi", pp);
		if (ret) {
			dev_err(&pdev->dev, "failed to request MSI IRQ\n");
			return ret;
		}
	}

	pp->root_bus_nr = -1;
	pp->ops = &sunxi_plat_pcie_host_ops;

	ret = sunxi_pcie_host_init(pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to initialize host\n");
		return ret;
	}

	return 0;
}

static int __init sunxi_plat_pcie_probe(struct platform_device *pdev)
{
	struct sunxi_pcie *sunxi_pcie;
	struct pcie_port *pp;
	struct resource *res;  /* Resource from DT */
	int ret;

	sunxi_pcie = devm_kzalloc(&pdev->dev, sizeof(*sunxi_pcie),
					GFP_KERNEL);
	if (!sunxi_pcie)
		return -ENOMEM;

	pp = &sunxi_pcie->pp;
	pp->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	sunxi_pcie->dbi_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sunxi_pcie->dbi_base))
		return PTR_ERR(sunxi_pcie->dbi_base);

	pp->dbi_base = sunxi_pcie->dbi_base;
	sunxi_pcie->app_base = sunxi_pcie->dbi_base + USER_DEFINED_REGISTER_LIST;

	sunxi_pcie->pcie_ref = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR_OR_NULL(sunxi_pcie->pcie_ref)) {
		dev_err(&pdev->dev, "%s:get pcie ref  clk failed\n", __func__);
		return -1;
	}

	sunxi_pcie->pcie_axi = of_clk_get(pdev->dev.of_node, 1);
	if (IS_ERR_OR_NULL(sunxi_pcie->pcie_axi)) {
		dev_err(&pdev->dev, "%s:get pcie axi clk failed\n", __func__);
		return -1;
	}

	sunxi_pcie->pcie_aux = of_clk_get(pdev->dev.of_node, 2);
	if (IS_ERR_OR_NULL(sunxi_pcie->pcie_aux)) {
		dev_err(&pdev->dev, "%s:get pcie aux clk failed\n", __func__);
		return -1;
	}

	sunxi_pcie->pcie_bus = of_clk_get(pdev->dev.of_node, 3);
	if (IS_ERR_OR_NULL(sunxi_pcie->pcie_bus)) {
		dev_err(&pdev->dev, "%s:get pcie bus clk failed\n", __func__);
		return -1;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "speed_gen", &sunxi_pcie->speed_gen);
	if (ret) {
		dev_info(&pdev->dev, "get speed Gen failed\n");
		sunxi_pcie->speed_gen = 0x1;
	}
	sunxi_pcie_clk_setup(pp);
	sunxi_pcie_irqmask(pp);
	sunxi_pcie->link_irq = platform_get_irq(pdev, 1);
	ret = devm_request_irq(&pdev->dev, sunxi_pcie->link_irq,
				 sunxi_plat_pcie_linkup_handler,
					IRQF_SHARED, "sunxi-pcie-linkup", pp);
	if (ret) {
		dev_err(&pdev->dev, "failed to request linkup IRQ\n");
		return ret;
	}
	sunxi_pcie_gpio_reset(pdev);
	ret = sunxi_plat_add_pcie_port(pp, pdev);
	if (ret < 0)
		return ret;

	platform_set_drvdata(pdev, sunxi_pcie);

	return 0;
}

static int __exit sunxi_plat_pcie_remove(struct platform_device *pdev)
{

	return 0;
}

static const struct of_device_id sunxi_plat_pcie_of_match[] = {
	{ .compatible = "allwinner,sun50i-pcie", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_plat_pcie_of_match);

static struct platform_driver sunxi_plat_pcie_driver = {
	.driver = {
		.name	= "sunxi-pcie",
		.owner	= THIS_MODULE,
		.of_match_table = sunxi_plat_pcie_of_match,
	},
	.remove = __exit_p(sunxi_plat_pcie_remove),
};

static int __init pcie_init(void)
{
	return platform_driver_probe(&sunxi_plat_pcie_driver, sunxi_plat_pcie_probe);
}
subsys_initcall(pcie_init);

MODULE_AUTHOR("wangjx <wangjx@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner PCIe host controller glue platform driver");
MODULE_LICENSE("GPL v2");
