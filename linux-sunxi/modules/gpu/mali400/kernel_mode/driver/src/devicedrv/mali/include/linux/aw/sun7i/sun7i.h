/**
 * Copyright (C) 2015-2016 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */

#ifndef _MALI_SUN7I_H_
#define _MALI_SUN7I_H_

#define MALI_PP_CORES_NUM 2
#define AW_MALI_GPU_RESOURCES_MALI400_MPX MALI_GPU_RESOURCES_MALI400_MP2

#define GPU_PBASE           SW_PA_MALI_IO_BASE
#define IRQ_GPU_GP          AW_IRQ_GPU_GP
#define IRQ_GPU_GPMMU       AW_IRQ_GPU_GPMMU
#define IRQ_GPU_PP0         AW_IRQ_GPU_PP0
#define IRQ_GPU_PPMMU0      AW_IRQ_GPU_PPMMU0
#define IRQ_GPU_PP1         AW_IRQ_GPU_PP1
#define IRQ_GPU_PPMMU1      AW_IRQ_GPU_PPMMU1

aw_private_data aw_private = {
	.tempctrl      = {
		.temp_ctrl_status = 1,
	},
	.pm            = {
		.regulator      = NULL,
		.regulator_id   = "vdd-gpu",
		.clk[0]             = {
			.clk_name       = "pll",
			.clk_id         = CLK_SYS_PLL8,
			.clk_handle     = NULL,
			.parent_clk_num = -1,
			.need_reset     = 0,
			.need_set_freq  = 1
		},
		.clk[1]         = {
			.clk_name       = "mali",
			.clk_id         = CLK_MOD_MALI,
			.clk_handle     = NULL,
			.parent_clk_num = 0,
			.need_reset     = 1,
			.need_set_freq  = 1
		},
		.clk[2]             = {
			.clk_name       = "ahb",
			.clk_id         = CLK_AHB_MALI,
			.clk_handle     = NULL,
			.parent_clk_num = -1,
			.need_reset     = 0,
			.need_set_freq  = 0
		},
		.vf_table[0]   = {
			.vol  = 1200,
			.freq = 336,
		},
		.dvfs_status       = 0,
		.begin_level       = 0,
		.max_level         = 0,
		.scene_ctrl_cmd    = 0,
		.scene_ctrl_status = 0,
		.independent_pow   = 0,
		.dvm               = 0,
	},
	.debug           = {
		.enable      = 0,
		.frequency   = 0,
		.voltage     = 0,
		.tempctrl    = 0,
		.scenectrl   = 0,
		.dvfs        = 0,
		.level       = 0,
	}
};

#endif /* _MALI_SUN7I_H_ */
