/*
 * Driver for sunxi SD/MMC host controllers
 * (C) Copyright 2012-2017 lixiang <lixiang@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SUNXI_MMC_EXPORT_H__
#define __SUNXI_MMC_EXPORT_H__
void sunxi_mmc_rescan_card(unsigned id);
void sunxi_mmc_reg_ex_res_inter(struct sunxi_mmc_host *host,u32 phy_id);
int sunxi_mmc_check_r1_ready(struct mmc_host* mmc, unsigned ms);


#endif
