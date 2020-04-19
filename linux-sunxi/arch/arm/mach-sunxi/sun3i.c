/*
 * Device Tree support for Allwinner A1X SoCs
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clocksource.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/i2c/pcf857x.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/system_misc.h>
#include <linux/sys_config.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/of_gpio.h>

#include "sunxi.h"

static void __iomem *wdt_base;

static void sun3i_restart(char mode, const char *cmd)
{

#define SUN3I_WATCHDOG_CTRL_REG			0x10
#define SUN3I_WATCHDOG_CTRL_RESTART		((1 << 0) | 0xA57)
#define SUN3I_WATCHDOG_CONFIG_REG		0x14
#define SUN3I_WATCHDOG_CONFIG_WHOLE_SYS	(1 << 0)
#define SUN3I_WATCHDOG_MODE_REG			0x18
#define SUN3I_WATCHDOG_MODE_ENABLE		(1 << 0)
	if (!wdt_base)
		return;
	pr_debug("func:%s,para: mode=%s,cmd=%s\n", __func__, &mode, cmd);
	/* config watchdog reset whole system.*/
	writel(SUN3I_WATCHDOG_CONFIG_WHOLE_SYS,
		   wdt_base + SUN3I_WATCHDOG_CONFIG_REG);

	/*
	 * start the watchdog. The default (and lowest) interval
	 * value for the watchdog is 0.5s.
	 */
	writel(SUN3I_WATCHDOG_MODE_ENABLE, wdt_base + SUN3I_WATCHDOG_MODE_REG);
}

static struct of_device_id sunxi_restart_ids[] = {
	{ .compatible = "allwinner,sun3i-wdt", .data = sun3i_restart },
	{ /*sentinel*/ }
};

static void sunxi_setup_restart(void)
{
	const struct of_device_id *of_id;
	struct device_node *np;

	np = of_find_matching_node(NULL, sunxi_restart_ids);
	if (WARN(!np, "unable to setup watchdog restart"))
		return;

	wdt_base = of_iomap(np, 0);
	WARN(!wdt_base, "failed to map watchdog base address");

	of_id = of_match_node(sunxi_restart_ids, np);
	WARN(!of_id, "restart function not available");

	arm_pm_restart = of_id->data;
}

static struct map_desc sunxi_io_desc[] __initdata = {
	{
		.virtual = (unsigned long) IO_ADDRESS(SUNXI_IO_PBASE),
		.pfn	 = __phys_to_pfn(SUNXI_IO_PBASE),
		.length	 = SUNXI_IO_SIZE,
		.type	 = MT_DEVICE,
	},
#ifdef CONFIG_ARCH_SUN3IW1P1
	{
		.virtual = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A1_PBASE),
		.pfn	 = __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		.length	 = SUNXI_SRAM_A1_SIZE,
		.type	 = MT_MEMORY_ITCM,
	},
#endif
};

void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));
}

static void __init sunxi_timer_init(void)
{
	of_clk_init(NULL);
#ifdef CONFIG_COMMON_CLK_ENABLE_SYNCBOOT_EARLY
	clk_syncboot();
#endif
	clocksource_of_init();
}

static void sunxi_power_off(void)
{
	struct device_node *node = NULL;
	struct gpio_config config;
	char pin_name[128] = {0};
	int cfg_val = 0;
	int gpio = 0;
	int ret = -1;

	node = of_find_node_by_type(NULL, "power_ctrl");
	if (NULL == node) {
		pr_err("%s: fail to find power_ctrl node\n", __func__);
		return;
	}

	gpio = of_get_named_gpio_flags(node, "power_on",
			0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(gpio))
		return;

	ret = gpio_request(config.gpio, NULL);
	if (0 != ret) {
		pr_err("%s: reques gpio=%d fail, ret=%d\n",
			__func__, config.gpio, ret);
		return;
	}

	sunxi_gpio_to_name(config.gpio, pin_name);
	cfg_val = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, config.mul_sel);
	pin_config_set(SUNXI_PINCTRL, pin_name, cfg_val);
	if (config.pull != GPIO_PULL_DEFAULT) {
		cfg_val = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, config.pull);
		pin_config_set(SUNXI_PINCTRL, pin_name, cfg_val);
	}
	if (config.drv_level != GPIO_DRVLVL_DEFAULT) {
		cfg_val = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV,
			config.drv_level);
		pin_config_set(SUNXI_PINCTRL, pin_name, cfg_val);
	}
	if (config.data != GPIO_DATA_DEFAULT) {
		cfg_val = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, config.data);
		pin_config_set(SUNXI_PINCTRL, pin_name, cfg_val);
	}

	gpio_free(config.gpio);
	return;
}

static void sunxi_power_off_prepare(void)
{
	return;
}

static void __init sunxi_dt_init(void)
{
	sunxi_setup_restart();
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);

	if (!pm_power_off)
		pm_power_off = sunxi_power_off;

	if (!pm_power_off_prepare)
		pm_power_off_prepare = sunxi_power_off_prepare;
}

static const char * const sunxi_board_dt_compat[] = {
	"arm,sun3iw1p1",
	NULL,
};

DT_MACHINE_START(SUNXI_DT, "Allwinner A1X (Device Tree)")
	.init_machine	= sunxi_dt_init,
	.map_io		= sunxi_map_io,
	.init_irq	= irqchip_init,
	.init_time	= sunxi_timer_init,
	.dt_compat	= sunxi_board_dt_compat,
MACHINE_END

