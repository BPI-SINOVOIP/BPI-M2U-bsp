/*
 * Copyright (C) 2016-2017 Allwinner Technology Limited. All rights reserved.
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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_
#include <linux/wait.h>

struct aw_vf_table {
	unsigned long freq; /* Hz */
	unsigned long vol;  /* uV */
};

enum power_status {
	POWER_MAN_STATUS = 0x01,
	POWER_GPU_STATUS = 0x02,
};

enum{
	TEMP_LEVEL_LOW = 0,
	TEMP_LEVEL_STABELED,
	TEMP_LEVEL_URGENT,
};

enum temp_status {
	TEMP_STATUS_NORNAL = 0,
	TEMP_STATUS_RUNNING,
	TEMP_STATUS_LOCKED,
};

enum scene_ctrl_cmd {
	SCENE_CTRL_NORMAL_MODE,
	SCENE_CTRL_PERFORMANCE_MODE
};

struct power_control {
	int  tempctrl_target;
	int  scenectrl_target;
	int  current_level;
	int  benckmark_mode;
	int  bk_threshold;
	int  normal_mode;
	int max_normal_level;
	bool man_ctrl;
	bool en_idle;
	bool en_tempctrl;
	bool en_scenectrl;
	bool en_dvfs;
	bool power_on;
	enum scene_ctrl_cmd scenectrl_cmd;
	enum power_status power_status;
};

enum power_ctl_level {
	/* 0 is the high level */
	POWER_ON_OFF_CTL = 0X01,
	POWER_MAN_CTL = 0x02,
	POWER_TEMP_CTL = 0x04,
	POWER_SCENE_CTL = 0x08,
	POWER_DVFS_CTL = 0x10,
};

struct sunxi_private {
	void *kbase_mali;
	void __iomem *power_gate_addr;
	void __iomem *smc_drm_addr;
	struct dentry *my_debug;
	struct power_control power_ctl;
	struct regulator *regulator;
	struct clk *gpu_pll_clk;
	int nr_of_vf;
	struct aw_vf_table *vf_table;
	struct aw_vf_table cur_vf;
	struct aw_vf_table target_vf;
	spinlock_t target_lock;
	struct mutex power_lock;
	struct mutex ctrl_lock;
#if IS_ENABLED(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND)
	struct devfreq_simple_ondemand_data sunxi_date;
#endif

#ifdef CONFIG_SUNXI_GPU_COOLING
	struct delayed_work temp_donwn_delay;
	int temp_current_level;
	int last_stable_target;
	int last_set_target;
	int stable_statistics;
	int stable_cnt;
	struct mutex temp_lock;
	enum temp_status temp_status;
#endif
	struct delayed_work power_off_delay;
	long delay_hz;
	wait_queue_head_t dvfs_done;
	unsigned long init_freq;
	unsigned long utilisition;
	bool do_dvfs;
	bool dvfs_ing;
	int down_vfs;
	int debug;
};

#ifdef CONFIG_SUNXI_GPU_COOLING
int thermal_cool_callback(int level);
extern int gpu_thermal_cool_register(int (*cool) (int));
extern int gpu_thermal_cool_unregister(void);
#endif /* CONFIG_SUNXI_GPU_COOLING */

#endif /* _PLATFORM_H_ */
