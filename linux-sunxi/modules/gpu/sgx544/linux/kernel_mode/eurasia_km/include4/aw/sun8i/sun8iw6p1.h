/*************************************************************************/ /*!
@File           sun8iw6p1.h
@Title          SUN8IW6P1 Header
@Copyright      Copyright (c) Allwinnertech Co., Ltd. All Rights Reserved
@Description    Define Detailed data about GPU for Allwinnertech platforms
@License        GPLv2

The contents of this file is used under the terms of the GNU General Public 
License Version 2 ("GPL") in which case the provisions of GPL are applicable
instead of those above.
*/ /**************************************************************************/

#ifndef _SUN8IW6P1_H_
#define _SUN8IW6P1_H_

struct aw_private_data aw_private = {
#ifdef CONFIG_MALI_DT
	.np_gpu        = NULL,
#endif /* CONFIG_MALI_DT */
	.tempctrl      = {
		.temp_ctrl_status = 1,
#ifdef CONFIG_CPU_BUDGET_THERMAL
		.count            = 3,
		.tl_table[0] = {
			.temp  = 80,
			.level = 3,
		},
		.tl_table[1] = {
			.temp  = 90,
			.level = 2,
		},
		.tl_table[2] = {
			.temp  = 100,
			.level = 0,
		}
#endif /* CONFIG_CPU_BUDGET_THERMAL */
	},
	.pm            = {
		.regulator      = NULL,
		.regulator_id   = "vdd-gpu",
		.clk[0]         = {
			.clk_name        = "pll",
			.clk_parent_name = NULL,
			.clk_handle      = NULL,
			.need_reset      = 0,
			.freq_type       = 1,
			.freq            = 0,
			.clk_status      = 0
		},
		.clk[1]         = {
			.clk_name        = "core",
			.clk_parent_name = "pll",
			.clk_handle      = NULL,
			.need_reset      = 0,
			.freq_type       = 1,
			.freq            = 0,
			.clk_status      = 0
		},
		.clk[2]         = {
			.clk_name        = "mem",
			.clk_parent_name = "pll",
			.clk_handle      = NULL,
			.need_reset      = 0,
			.freq_type       = 1,
			.freq            = 0,
			.clk_status      = 0
		},
		.clk[3]         = {
			.clk_name        = "hyd",
			.clk_parent_name = "pll",
			.clk_handle      = NULL,
			.need_reset      = 1,
			.freq_type       = 1,
			.freq            = 0,
			.clk_status      = 0
		},
		.vf_table[0]   = {
			.freq = 216,
			.vol  = 700
		},
		.vf_table[1]   = {
			.freq = 252,
			.vol  = 740
		},
		.vf_table[2]   = {
			.freq = 288,
			.vol  = 800
		},
		.vf_table[3]   = {
			.freq = 456,
			.vol  = 840
		},
		.vf_table[4]   = {
			.freq = 552,
			.vol  = 900
		},
		.vf_table[5]   = {
			.freq = 624,
			.vol  = 940
		},
		.vf_table[6]   = {
			.freq = 648,
			.vol  = 1000
		},
		.vf_table[7]   = {
			.freq = 672,
			.vol  = 1040
		},
		.vf_table[8]   = {
			.freq = 720,
			.vol  = 1100
		},
		.dvfs_status       = 0,
		.begin_level       = 4,
		.max_level         = 8,
		.scene_ctrl_cmd    = 0,
		.scene_ctrl_status = 1,
	},
	.poweroff_gate   = {
		.bit  = 0,
		.addr = (phys_addr_t)(SUNXI_R_PRCM_VBASE + 0x118),
	},
	.debug           = {
		.enable      = 0,
		.frequency   = 0,
		.voltage     = 0,
		.tempctrl    = 0,
		.scenectrl   = 0,
		.dvfs        = 0,
		.level       = 0,
	},
	.irq_num         = 129
};

#endif /* _SUN8IW6P1_H_ */
