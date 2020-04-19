/*************************************************************************/ /*!
@File           platform.h
@Title          Platform Header
@Copyright      Copyright (c) Allwinnertech Co., Ltd. All Rights Reserved
@Description    Define structs of GPU for Allwinnertech platform
@License        GPLv2

The contents of this file is used under the terms of the GNU General Public 
License Version 2 ("GPL") in which case the provisions of GPL are applicable
instead of those above.
*/ /**************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/export.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk/sunxi.h>

#ifndef CONFIG_OF
#include <linux/clk/sunxi_name.h>
#endif /* CONFIG_OF */

#include <linux/delay.h>
#include <linux/stat.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#else
#include <mach/sys_config.h>
#endif /* CONFIG_OF */
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sunxi-smc.h>

#ifdef CONFIG_CPU_BUDGET_THERMAL
#include <linux/cpu_budget_cooling.h>
#endif /* CONFIG_CPU_BUDGET_THERMAL */

#include "services_headers.h"
#include "sysinfo.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

struct aw_clk_data {
	const char *clk_name;    /* Clock name */

#ifndef CONFIG_OF
	/* Clock ID, which is in the system configuration head file */
	const char *clk_id;
#endif /* CONFIG_OF */

	/* Parent clock name */
	const char *clk_parent_name;

	/* Clock handle, we can set the frequency value via it */
	struct clk *clk_handle;

	/* If the value is 1, the reset API while handle it to reset corresponding module */
	bool       need_reset;

	/* The meaning of the values are as follows:
	*  0: don't need to set frequency.
	*  1: use the public frequency
	*  >1: use the private frequency, the value is for frequency.
	*/
	int        freq_type;

	int        freq; /* MHz */

	bool       clk_status;
};

struct vf_table_data {
	u32 freq;  /* MHz */
	u32 vol;   /* mV */
};

struct reg_data {
	u8   bit;
	phys_addr_t addr;
};

#ifdef CONFIG_CPU_BUDGET_THERMAL
/* The struct is for temperature-level table */
struct aw_tl_table
{
	u8 temp;
	u8 level;
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

struct aw_tempctrl_data
{
    bool temp_ctrl_status;

#ifdef CONFIG_CPU_BUDGET_THERMAL
	u8   count;     /* The data in tf_table to use */
	struct aw_tl_table tl_table[3];
#endif /* CONFIG_CPU_BUDGET_THERMAL */
};

struct aw_pm_data {
	struct regulator *regulator;
	char   *regulator_id;
	int    current_vol;
	struct aw_clk_data clk[4];
	struct mutex dvfs_lock;
	bool   dvfs_status;
	struct vf_table_data vf_table[9];
	int    cool_level;
	int    begin_level;
	int    current_level;
	int    max_available_level;
	int    max_level;

	/* 1 means GPU should provide the highest performance */
	int    scene_ctrl_cmd;

	bool   scene_ctrl_status;
};

struct aw_debug {
	bool enable;
	bool frequency;
	bool voltage;
	bool tempctrl;
	bool scenectrl;
	bool dvfs;
	bool level;
};

struct aw_private_data {
#ifdef CONFIG_CPU_BUDGET_THERMAL
	u8     sensor_num;
#endif /* CONFIG_CPU_BUDGET_THERMAL */

#ifdef CONFIG_OF
	struct device_node *np_gpu;
#endif /* CONFIG_OF */

	struct aw_tempctrl_data tempctrl;

	struct aw_pm_data pm;

	struct reg_data poweroff_gate;

	struct aw_debug debug;

	int irq_num;
};

#if defined(CONFIG_ARCH_SUN8IW6P1)
#include "sun8i/sun8iw6p1.h"
#else
#error "please select a platform\n"
#endif

#endif
