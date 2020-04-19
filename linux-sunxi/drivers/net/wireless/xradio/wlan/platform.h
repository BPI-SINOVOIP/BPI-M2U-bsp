/*
 * platform interfaces for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef XRADIO_PLAT_H_INCLUDED
#define XRADIO_PLAT_H_INCLUDED

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/mmc/host.h>

/* interfaces to check ready in if need.*/
#define MCI_CHECK_READY(h, t) 0

/* platform interfaces */
int xradio_plat_init(void);
void xradio_plat_deinit(void);
int xradio_sdio_detect(int enable);
int  xradio_request_gpio_irq(struct device *dev, void *sbus_priv);
void xradio_free_gpio_irq(struct device *dev, void *sbus_priv);
int xradio_wlan_power(int on);

#endif /* XRADIO_PLAT_H_INCLUDED */
