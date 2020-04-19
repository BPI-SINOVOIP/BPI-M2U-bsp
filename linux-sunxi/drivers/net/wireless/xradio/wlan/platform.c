/*
 * Platform interfaces for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/ioport.h>

#include <linux/regulator/consumer.h>

#include "xradio.h"
#include "platform.h"
#include "sbus.h"
#include <linux/sys_config.h>
#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/power/scenelock.h>
#include <linux/power/aw_pm.h>

#define PLAT_ALLWINNER_SUNXI

/* default 0, set 1 keep PMU power on when wlan is closed. */
#define PMU_POWER_WLAN_RETAIN  0
/* default 0, set 1 if you want to get config from sys, like dts. */
#define PLATFORM_SYSCONFIG     0

#define MAX_POWER_NUM          3

#if (PLATFORM_SYSCONFIG)
/* WLAN platform configure.*/
#define WIFI_CONFIG        "wifi_para"
static char *axp_name[MAX_POWER_NUM];
static u32 power_level[MAX_POWER_NUM];

static u32 wlan_reset_gpio;
static u32 wlan_irq_gpio;
static u32 lpo_use_apclk;

#else
#if 0
/* If no sysconfig, configure param of your platform.*/
static char *axp_name[MAX_POWER_NUM] = {
	"axp22_dldo1",
	"axp22_eldo1",
	NULL,
};
static u32 power_level[MAX_POWER_NUM] = {
	3300000,
	1800000,
	0
};
static u32 wlan_reset_gpio = GPIOL(8);
static u32 wlan_irq_gpio   = GPIOL(9);
static u32 lpo_use_apclk   = 1;
#endif
#endif

#define PLATFORM_RFKILL_PM 1

static int wlan_bus_id;

#ifdef PLATFORM_RFKILL_PM
extern int sunxi_wlan_get_bus_index(void);
extern void sunxi_wlan_set_power(int on_off);
extern int sunxi_wlan_get_oob_irq(void);
#endif

/********************* platform Interfaces *********************/
#define xradio_msg(...)  do {printk("[xradio_plat]: "__VA_ARGS__); } while (0)
#if (PLATFORM_SYSCONFIG)
#define SYS_CONFIG_INT(name, value, ret) do { \
	type = script_get_item(WIFI_CONFIG, name, &val); \
	if (SCIRPT_ITEM_VALUE_TYPE_INT == type) { \
		value = val.val; \
		xradio_msg("%s=%d\n", name, value); \
	} else { \
		ret = -1; \
		xradio_msg("%s not config.\n", name); \
	} \
} while (0)

#define SYS_CONFIG_STR(name, ptr, ret) do { \
	type = script_get_item(WIFI_CONFIG, name, &val); \
	if (SCIRPT_ITEM_VALUE_TYPE_STR == type && \
		*(char *)val.str != '\0') { \
		ptr = val.str; \
		xradio_msg("%s=%s\n", name, ptr); \
	} else { \
		ret = -1; \
		xradio_msg("%s not config.\n", name); \
	} \
} while (0)

#define SYS_CONFIG_PIO(name, pio, ret) do { \
	type = script_get_item(WIFI_CONFIG, name, &val); \
	if (SCIRPT_ITEM_VALUE_TYPE_PIO == type) { \
		pio = val.gpio.gpio; \
		xradio_msg("%s=%d\n", name, pio); \
	} else { \
		ret = -1; \
		xradio_msg("%s not config.\n", name); \
	} \
} while (0)

static int plat_get_syscfg(void)
{
	int ret = 0;
	script_item_u val;
	script_item_value_type_e type;

	/* Get SDIO/USB config. */
#if defined(CONFIG_XRADIO_SDIO)
	SYS_CONFIG_INT("wifi_sdc_id", wlan_bus_id, ret);
#elif defined(CONFIG_XRADIO_USB)
	SYS_CONFIG_INT("wifi_usbc_id", wlan_bus_id, ret);
#endif
	if (ret)
		return ret;

	/* Get GPIO config. */
	SYS_CONFIG_PIO("xradio_wl_regon", wlan_reset_gpio, ret);
	if (ret)
		return ret;
	SYS_CONFIG_PIO("xradio_wl_host_wake", wlan_irq_gpio, ret);
	if (ret)
		return ret;

	/* Get wifi_power config. */
	SYS_CONFIG_STR("wifi_power", axp_name[0], ret);
	SYS_CONFIG_INT("xr_wl_power_vol", power_level[0], ret);

	SYS_CONFIG_STR("wifi_power_ext1", axp_name[1], ret);
	SYS_CONFIG_INT("xr_wl_power_vol1", power_level[1], ret);

	SYS_CONFIG_STR("wifi_power_ext2", axp_name[2], ret);
	SYS_CONFIG_INT("xr_wl_power_vol2", power_level[2], ret);

	/* Get other config. */
	SYS_CONFIG_INT("xradio_lpo_use_apclk", lpo_use_apclk, ret);

	return 0;
}
#endif

#if 0
static int first = 1;
static int plat_module_power(int onoff)
{
	struct regulator *wifi_ldo = NULL;
	struct clk *ap_32k = NULL;
	int ret = 0;
	int i = 0;

	xradio_msg("xradio module power set by axp.\n");
	for (i = 0; i < MAX_POWER_NUM; i++) {
		if (axp_name[i]) {
			wifi_ldo = regulator_get(NULL, axp_name[i]);
			if (!wifi_ldo) {
				xradio_msg("regulator_get %s failed.\n",
					   axp_name[i]);
				return -ret;
			}

			if (first) {
				xradio_msg("first time\n");
				ret = regulator_force_disable(wifi_ldo);
				if (ret < 0) {
					xradio_msg("regulator_force_disable fail," \
						   "return %d.\n", ret);
					regulator_put(wifi_ldo);
					return ret;
				}
				first = 0;
			}

			if (onoff) {
				xradio_msg("%s regulator on=%d.\n",
					   axp_name[i], power_level[i]);
				ret = regulator_set_voltage(wifi_ldo, power_level[i],
							    power_level[i]);
				if (ret < 0) {
					xradio_msg("regulator_set_voltage fail," \
						   "return %d.\n", ret);
					regulator_put(wifi_ldo);
					return ret;
				}
				ret = regulator_enable(wifi_ldo);
				if (ret < 0) {
					xradio_msg("regulator_enable fail, return %d.\n", ret);
					regulator_put(wifi_ldo);
					return ret;
				}
			} else {
				xradio_msg("%s regulator off.\n", axp_name[i]);
				ret = regulator_disable(wifi_ldo);
				if (ret < 0) {
					xradio_msg("regulator_disable fail, return %d.\n", ret);
					regulator_put(wifi_ldo);
					return ret;
				}
			}
			regulator_put(wifi_ldo);
		}
	}

	/* use external 32k clock.*/
	if (lpo_use_apclk) {
		ap_32k = clk_get(NULL, "losc_out");
		if (!ap_32k) {
			xradio_msg("Get ap 32k clk out failed!\n");
			return -1;
		}
		ret = clk_prepare_enable(ap_32k);
		if (ret) {
			xradio_msg("losc out enable failed!\n");
		}
	} else {
		clk_disable(ap_32k); /* disable 32k clk. */
	}

	return ret;
}
#endif

/*********************Interfaces called by xradio wlan. *********************/
static u32 gpio_irq_handle;
#ifdef PLAT_ALLWINNER_SUNXI
static irqreturn_t xradio_gpio_irq_handler(int irq, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	unsigned long flags;

	SYS_BUG(!self);
	spin_lock_irqsave(&self->lock, flags);
	if (self->irq_handler)
		self->irq_handler(self->irq_priv);
	spin_unlock_irqrestore(&self->lock, flags);
	return IRQ_HANDLED;
}

int xradio_request_gpio_irq(struct device *dev, void *sbus_priv)
{
	int ret = -1;
	if (!gpio_irq_handle) {
		gpio_irq_handle = sunxi_wlan_get_oob_irq();
		ret = devm_request_irq(dev, gpio_irq_handle,
				      (irq_handler_t)xradio_gpio_irq_handler,
				       IRQF_TRIGGER_RISING, "xradio_irq", sbus_priv);
		if (IS_ERR_VALUE(ret)) {
			gpio_irq_handle = 0;
		}
	} else {
		xradio_dbg(XRADIO_DBG_ERROR,
			   "%s: error, irq exist already!\n", __func__);
	}

	if (gpio_irq_handle) {
		xradio_dbg(XRADIO_DBG_NIY, "%s: request_irq sucess! irq=0x%08x\n",
			   __func__, gpio_irq_handle);
#if !defined (CONFIG_ARCH_SUN3IW1) && !defined (CONFIG_ARCH_SUN8IW8)
		ret = enable_wakeup_src(CPUS_WLAN_SRC, 0);
		if (ret < 0)
			xradio_dbg(XRADIO_DBG_ERROR, "%s: enable_wakeup_src failed!\n",
				   __func__);
		else
			xradio_dbg(XRADIO_DBG_NIY, "%s: enable_wakeup_src sucess!\n",
				   __func__);

#else
		ret = 0;
#endif
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: request_irq err: %d\n",
			   __func__, ret);
		ret = -1;
	}
	return ret;
}
void xradio_free_gpio_irq(struct device *dev, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	if (gpio_irq_handle) {
		/* for linux3.4 */
		devm_free_irq(dev, gpio_irq_handle, self);
		gpio_irq_handle = 0;
#if !defined (CONFIG_ARCH_SUN3IW1) && !defined (CONFIG_ARCH_SUN8IW8)
		disable_wakeup_src(CPUS_WLAN_SRC, 0);
#endif
	}
}
#else
#if 0
static u32 xradio_gpio_irq_handler(void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	unsigned long flags;

	SYS_BUG(!self);
	spin_lock_irqsave(&self->lock, flags);
	if (self->irq_handler)
		self->irq_handler(self->irq_priv);
	spin_unlock_irqrestore(&self->lock, flags);
	return 0;
}
int xradio_request_gpio_irq(struct device *dev, void *sbus_priv)
{
	int ret = -1;
	if (!gpio_irq_handle) {
		gpio_irq_handle = sw_gpio_irq_request(wlan_irq_gpio,
		    TRIG_EDGE_POSITIVE, (peint_handle)xradio_gpio_irq_handler,
		    sbus_priv);
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: error, irq exist already!\n",
			   __func__);
	}

	if (gpio_irq_handle) {
		xradio_dbg(XRADIO_DBG_NIY, "%s: request_irq sucess! irq=0x%08x\n",
			   __func__, gpio_irq_handle);
		ret = 0;
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: request_irq err: %d\n",
			   __func__, ret);
		ret = -1;
	}
	return ret;
}
void xradio_free_gpio_irq(struct device *dev, void *sbus_priv)
{
	struct sbus_priv *self = (struct sbus_priv *)sbus_priv;
	if (gpio_irq_handle) {
		sw_gpio_irq_free(gpio_irq_handle);
		gpio_irq_handle = 0;
	}
}
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
extern void sunxi_mmc_rescan_card(unsigned id);
#else
extern void sw_mci_rescan_card(unsigned id, unsigned insert);
#endif
int xradio_sdio_detect(int enable)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0))
	sunxi_mmc_rescan_card(wlan_bus_id);
#else
	int insert = enable;
	sw_mci_rescan_card(wlan_bus_id, insert);
#endif
	xradio_dbg(XRADIO_DBG_ALWY, "%s SDIO card %d\n",
		   enable ? "Detect" : "Remove", wlan_bus_id);
	mdelay(10);
	return 0;
}

int xradio_wlan_power(int on)
{
	int ret = 0;
#if 0
	ret = gpio_request(wlan_reset_gpio, "wlan_reset");
	if (!ret) {
		if (on) {  /* power up. */
#if (!PMU_POWER_WLAN_RETAIN)
			plat_module_power(1);
			mdelay(50);
#endif
			gpio_direction_output(wlan_reset_gpio, 1);
			mdelay(50);
			gpio_direction_output(wlan_reset_gpio, 0);
			mdelay(2);
			gpio_direction_output(wlan_reset_gpio, 1);
			mdelay(50);
		} else { /* power down. */
			gpio_direction_output(wlan_reset_gpio, 0);
#if (!PMU_POWER_WLAN_RETAIN)
			plat_module_power(0);
#endif
		}
		gpio_free(wlan_reset_gpio);
	} else {
		xradio_dbg(XRADIO_DBG_ERROR, "%s: gpio_request err:%d\n",
			   __func__, ret);
	}
#endif

	sunxi_wlan_set_power(on);

	return ret;
}

int  xradio_plat_init(void)
{
	int ret = 0;
#if 0
#if (PLATFORM_SYSCONFIG)
	ret = plat_get_syscfg();
#endif

#if (PMU_POWER_WLAN_RETAIN)
	ret = plat_module_power(1);
#endif
#endif
	wlan_bus_id = sunxi_wlan_get_bus_index();
	return ret;
}

void xradio_plat_deinit(void)
{
#if 0
#if (PMU_POWER_WLAN_RETAIN)
	plat_module_power(0);
#endif
#endif
	return;
}

/**************************************************************************/
