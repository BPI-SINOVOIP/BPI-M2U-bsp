/*
 * Driver for sunxi SD/MMC host controllers
 * (C) Copyright 2012-2017 lixiang <lixiang@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifdef CONFIG_ARCH_SUN8IW10P1

#ifndef __SUNXI_MMC_SUN8IW10P1_2_H__
#define __SUNXI_MMC_SUN8IW10P1_2_H__


void sunxi_mmc_init_priv_v5px(struct sunxi_mmc_host *host,
			       struct platform_device *pdev, int phy_index);
int sunxi_mmc_oclk_onoff(struct sunxi_mmc_host *host, u32 oclk_en);

#endif

#endif
