/*
 * Driver for sunxi SD/MMC host controllers
 * (C) Copyright 2012-2017 lixiang <lixiang@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_MMC_DEBUG_H__
#define __SUNXI_MMC_DEBUG_H__
#include <linux/clk.h>
#include <linux/clk/sunxi.h>

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>

void sunxi_mmc_dumphex32(struct sunxi_mmc_host* host, char* name, char* base, int len);
void sunxi_mmc_dump_des(struct sunxi_mmc_host* host, char* base, int len);
int sunxi_mmc_uperf_stat(struct sunxi_mmc_host *host,
						struct mmc_data *data,
						struct mmc_request *mrq_busy,
						bool bhalf);



int mmc_create_sys_fs(struct sunxi_mmc_host* host,struct platform_device *pdev);
void mmc_remove_sys_fs(struct sunxi_mmc_host* host,struct platform_device *pdev);
void sunxi_dump_reg(struct mmc_host *mmc);

#endif
