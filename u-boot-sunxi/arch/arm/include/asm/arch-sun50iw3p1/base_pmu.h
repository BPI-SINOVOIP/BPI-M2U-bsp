/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __BASE_PMU_H_
#define __BASE_PMU_H_


int pmu_init(u8 power_mode);

s32 pmu_bus_init(void);
s32 pmu_bus_read(u32 rtaddr, u32 daddr, u8 *data);
s32 pmu_bus_write(u32 rtaddr, u32 daddr, u8 data);

int set_ddr_voltage(int set_vol);
int set_pll_voltage(int set_vol);
int probe_power_key(void);

#endif  /* __BASE_PMU_H_ */

