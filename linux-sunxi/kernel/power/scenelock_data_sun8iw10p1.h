
#ifndef _LINUX_SCENELOCK_DATA_SUN8IW10P1_H
#define _LINUX_SCENELOCK_DATA_SUN8IW10P1_H

#include <linux/power/axp_depend.h>

scene_extended_standby_t extended_standby[] = {
	{
		.scene_type	= SCENE_TALKING_STANDBY,
		.name		= "talking_standby",
		.soc_pwr_dep.id             = TALKING_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know we do not need
		 * care about vcc_pm state.*/
		.soc_pwr_dep.soc_pwr_dm_state.state
					= BITMAP(VCC_DRAM_BIT) |
						BITMAP(VDD_CPUS_BIT) |
						BITMAP(VCC_LPDDR_BIT) |
						BITMAP(VCC_PLL_BIT) |
						BITMAP(VCC_PL_BIT),
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off.*/
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
					= BITMAP(PM_PLL_DRAM),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/* for pf port: set the io to disable state. */
		.soc_pwr_dep.soc_io_state.io_state[0]
					= {0x01c208b4,
						0x00f0f0ff,
						0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
					= {0x01c208b4,
						0x000f0f00,
						0x00070700},
#if 0
		/* for pb port */
		.soc_pwr_dep.soc_io_state.io_state[0]
					=	{0x01c20824,
							0x0000ffff,
							0x00007777},
		.soc_pwr_dep.soc_io_state.io_state[1]
					=	{0x01c20828,
							0x00000ff0,
							0x00000770},
#endif

	},

	{
		.scene_type	= SCENE_USB_STANDBY,
		.name		= "usb_standby",
		.soc_pwr_dep.id			= USB_STANDBY_FLAG,
		/* note: vcc_io for phy */
		.soc_pwr_dep.soc_pwr_dm_state.state
				= BITMAP(VCC_DRAM_BIT) |
					BITMAP(VDD_CPUS_BIT) |
					BITMAP(VCC_LPDDR_BIT) |
					BITMAP(VCC_PLL_BIT) |
					BITMAP(VCC_PL_BIT) |
					BITMAP(VDD_SYS_BIT) |
					BITMAP(VCC_IO_BIT),
		/* mean: donot need care about the voltage. */
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/* mean all osc is off. +losc, +hosc */
		.soc_pwr_dep.cpux_clk_state.osc_en
				= BITMAP(OSC_LOSC_BIT) |
					BITMAP(OSC_HOSC_BIT) |
					BITMAP(OSC_LDO1_BIT) |
					BITMAP(OSC_LDO0_BIT),
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
				= BITMAP(PM_PLL_DRAM) |
					BITMAP(PM_PLL_PERIPH),
		/* hsic pll can be disabled,
		 * cpus can change cci400 clk from hsic_pll.*/
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		/* PLL_PERIPH freq = 24*1*1/2= 12M */
		.soc_pwr_dep.cpux_clk_state.pll_change
				= BITMAP(PM_PLL_PERIPH),
		.soc_pwr_dep.cpux_clk_state.pll_factor[PM_PLL_PERIPH] = {
			/* N=1 */
		    .factor1 = 0,
			/* Div1 = 0 + 1 = 1 */
		    .factor2 = 0,
			/* Div2 = 0 + 1 = 1, only used in plltest debug */
			.factor3 = 0,
			},
		.soc_pwr_dep.cpux_clk_state.bus_change
			= BITMAP(BUS_AHB1) |
				BITMAP(BUS_AHB2),
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1]     = {
			/* need make sure losc is on. */
		    .src = CLK_SRC_LOSC,
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB2]     = {
			/* need make sure AHB1 is on. */
		    .src = CLK_SRC_AHB1,
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		/* for pf port: set the io to disable state. */
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},
	{
		.scene_type	= SCENE_USB_OHCI_STANDBY,
		.name		= "usb_ohci_standby",
		.soc_pwr_dep.id			= USB_OHCI_STANDBY_FLAG,
		/* note: vcc_io for phy; */
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_LPDDR_BIT) |
				BITMAP(VCC_PLL_BIT) |
				BITMAP(VCC_PL_BIT) |
				BITMAP(VDD_SYS_BIT) |
				BITMAP(VCC_IO_BIT),
		/* mean: donot need care about the voltage. */
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off. +losc, +hosc*/
		.soc_pwr_dep.cpux_clk_state.osc_en
			= BITMAP(OSC_LOSC_BIT) |
				BITMAP(OSC_HOSC_BIT) |
				BITMAP(OSC_LDO1_BIT) |
				BITMAP(OSC_LDO0_BIT),
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= BITMAP(PM_PLL_DRAM) |
				BITMAP(PM_PLL_PERIPH),
		/* hsic pll can be disabled,
		 * cpus can change cci400 clk from hsic_pll.*/
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change
			= BITMAP(PM_PLL_PERIPH),
		/* PLL_PERIPH freq = 24*1*1/2= 12M */
		.soc_pwr_dep.cpux_clk_state.pll_factor[PM_PLL_PERIPH] = {
			/* N=1 */
		    .factor1 = 0,
			/* Div1 = 0 + 1 = 1 */
		    .factor2 = 0,
			/* Div2 = 0 + 1 = 1, only used in plltest debug; */
		    .factor3 = 0,
		},
		.soc_pwr_dep.cpux_clk_state.bus_change
			= BITMAP(BUS_AHB1) |
			BITMAP(BUS_AHB2),
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1] = {
			/* need make sure losc is on. */
		    .src = CLK_SRC_LOSC,
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB2] = {
			/* need make sure AHB1 is on. */
		    .src = CLK_SRC_AHB1,
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		/* for pf port: set the io to disable state. */
		.soc_pwr_dep.soc_io_state.io_state[0]
				= {0x01c208b4,
					0x00f0f0ff,
					0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
				= {0x01c208b4,
					0x000f0f00,
					0x00070700},
	},

	{
		.scene_type	= SCENE_USB_EHCI_STANDBY,
		/* for 3G wakeup */
		.name		= "usb_ehci_standby",
		.soc_pwr_dep.id			= USB_EHCI_STANDBY_FLAG,
		/* note: vcc_io for phy; */
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VDD_SYS_BIT) |
				BITMAP(VCC_PLL_BIT) |
				BITMAP(VCC_IO_BIT),
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off. +losc, +hosc */
		.soc_pwr_dep.cpux_clk_state.osc_en
			= BITMAP(OSC_LOSC_BIT),
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= BITMAP(PM_PLL_DRAM),
		/* hsic pll can be disabled,
		 * cpus can change cci400 clk from hsic_pll.*/
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change
				= BITMAP(BUS_AHB1) |
					BITMAP(BUS_AHB2),
		/* need make sure losc is on.*/
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1] = {
		    .src = CLK_SRC_LOSC,
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		/* need make sure AHB1 is on.*/
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB2] = {
		    .src = CLK_SRC_AHB1,
		    .pre_div = 0,
		    .div_ratio = 0,
		},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},

	{
		.scene_type	= SCENE_MP3_STANDBY,
		.name		= "mp3_standby",
		.soc_pwr_dep.id			= MP3_STANDBY_FLAG,
	},
	{
		.scene_type	= SCENE_BOOT_FAST,
		.name		= "boot_fast",
		.soc_pwr_dep.id			= BOOT_FAST_STANDBY_FLAG,
	},
	{
		.scene_type		    = SCENE_SUPER_STANDBY,
		.name			    = "super_standby",
		.soc_pwr_dep.id             = SUPER_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help,
		 * we know we do not need care about vcc_pm state.*/
		.soc_pwr_dep.soc_pwr_dm_state.state
				= BITMAP(VCC_DRAM_BIT) |
					BITMAP(VDD_CPUS_BIT) |
					BITMAP(VCC_LPDDR_BIT) |
					BITMAP(VCC_PLL_BIT) |
					BITMAP(VCC_PL_BIT),
		/* mean care about cpua, dram, sys, cpus,
		 * dram_pll, vdd_adc, vcc_pl, vcc_io,
		 * vcc_cpvdd, vcc_ldoin, vcc_pll */
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off.*/
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= BITMAP(PM_PLL_DRAM),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},

	{
		.scene_type		    = SCENE_GPIO_HOLD_STANDBY,
		.name			    = "gpio_hold_standby",
		.soc_pwr_dep.id             = GPIO_HOLD_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know
		 * we do not need care about vcc_pm state.*/
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_LPDDR_BIT) |
				BITMAP(VCC_PLL_BIT) |
				BITMAP(VCC_PL_BIT) |
				BITMAP(VDD_SYS_BIT) |
				BITMAP(VCC_IO_BIT),
		/* mean care about cpua, dram, sys, cpus, dram_pll,
		 * vdd_adc, vcc_pl, vcc_io, vcc_cpvdd,
		 * vcc_ldoin, vcc_pll */
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off.*/
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= BITMAP(PM_PLL_DRAM),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},

	{
		.scene_type                 = SCENE_NORMAL_STANDBY,
		.name                       = "normal_standby",
		.soc_pwr_dep.id             = NORMAL_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know
		 * we do not need care about vcc_pm state. */
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_LPDDR_BIT) |
				BITMAP(VCC_PL_BIT) |
				BITMAP(VDD_SYS_BIT) |
				BITMAP(VDD_CPUA_BIT) |
				BITMAP(VCC_IO_BIT) |
				BITMAP(VCC_PLL_BIT),
		/* mean care about cpua, dram, sys, cpus, dram_pll,
		 * vdd_adc, vcc_pl, vcc_io, vcc_cpvdd,
		 * vcc_ldoin, vcc_pll*/
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[VDD_SYS_BIT]      = 1000000,
		.soc_pwr_dep.soc_pwr_dm_state.volt[VDD_CPUA_BIT]      = 1000000,
		/*  mean all osc is off.*/
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= BITMAP(PM_PLL_DRAM),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},

	{
		.scene_type		    = SCENE_GPIO_STANDBY,
		.name			    = "gpio_standby",
		.soc_pwr_dep.id             = GPIO_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io,
		 * vcc_ldoin is on. +vdd_sys
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know
		 * we do not need care about vcc_pm state.*/
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_LPDDR_BIT) |
				BITMAP(VCC_PLL_BIT) |
				BITMAP(VCC_PL_BIT) |
				BITMAP(VDD_SYS_BIT) |
				BITMAP(VCC_IO_BIT),
		/* mean care about cpua, dram, sys, cpus,
		 * dram_pll, vdd_adc, vcc_pl, vcc_io,
		 * vcc_cpvdd, vcc_ldoin, vcc_pll*/
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off. +losc */
		.soc_pwr_dep.cpux_clk_state.osc_en
			= BITMAP(OSC_LOSC_BIT),
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= BITMAP(PM_PLL_DRAM),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change
			= BITMAP(BUS_AHB1),
		.soc_pwr_dep.cpux_clk_state.bus_factor[BUS_AHB1]
			= {
				/* need make sure losc is on.*/
				.src = CLK_SRC_LOSC,
				.pre_div = 0,
				.div_ratio = 0,
			},
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x0,
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
#if 0
		/* for pb port */
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c20824,
				0x0000ffff,
				0x00007777},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c20828,
				0x00000ff0,
				0x00000770},
#endif

	},

	{
		.scene_type	= SCENE_MISC_STANDBY,
		.name		= "misc_standby",
		.soc_pwr_dep.id = MISC_STANDBY_FLAG,
		/*
		 * mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know
		 * we do not need care about vcc_pm state.*/
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_PL_BIT) |
				BITMAP(VCC_PLL_BIT) |
				BITMAP(VCC_IO_BIT),
		   0x0644, /* -vcc_io; -dram, ldoin/ adc/ cpvdd */
		/* mean care about cpua, dram, sys, cpus, dram_pll,
		 * vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll */
		/* mean: donot need care about the voltage. */
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off. */
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,
		/* mean pll5 is shutdowned & open by dram driver. */
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (~(0x20)),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},
	{
		.scene_type	= SCENE_MISC1_STANDBY,
		.name		= "misc1_standby",
		.soc_pwr_dep.id = MISC1_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know
		 * we do not need care about vcc_pm state. */
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_PLL_BIT) |
				BITMAP(VCC_PL_BIT),
		/* mean care about cpua, dram, sys, cpus, dram_pll,
		 * vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll */
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off.*/
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis   = (~(0x20)),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/* hold gpio */
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},

	{
		/* for parse sysconfig config.
		 * default config according dram enter selfresh.
		 * when not enable enter selfresh, need open vdd_sys.*/
		.scene_type		    = SCENE_DYNAMIC_STANDBY,
		.name			    = "dynamic_standby",
		.soc_pwr_dep.id             = DYNAMIC_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know
		 * we do not need care about vcc_pm state. */
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_LPDDR_BIT) |
				BITMAP(VCC_PLL_BIT) |
				BITMAP(VCC_PL_BIT),
		/* mean care about cpua, dram, sys, cpus, dram_pll,
		 * vdd_adc, vcc_pl, vcc_io, vcc_cpvdd, vcc_ldoin, vcc_pll*/
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[0]      = 0x0,
		/*  mean all osc is off. */
		.soc_pwr_dep.cpux_clk_state.osc_en         = 0x0,
		/*  mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= (BITMAP(PM_PLL_DRAM)),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,

		/* enter selfresh, for compatible reason.*/
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/*  for pf port: set the io to disable state. */
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},

	{
		.scene_type                 = SCENE_WLAN_STANDBY,
		.name                       = "wlan_standby",
		.soc_pwr_dep.id             = WLAN_STANDBY_FLAG,
		/* mean dram, cpus,dram_pll,vcc_pl, vcc_io, vcc_ldoin is on.
		 * note: vcc_pm is marked on, just for cross-platform reason.
		 * at a83: with the sys_mask's help, we know
		 * we do not need care about vcc_pm state. */
		.soc_pwr_dep.soc_pwr_dm_state.state
			= BITMAP(VCC_DRAM_BIT) |
				BITMAP(VDD_CPUS_BIT) |
				BITMAP(VCC_LPDDR_BIT) |
				BITMAP(VCC_PL_BIT) |
				BITMAP(VDD_SYS_BIT) |
				BITMAP(VDD_CPUA_BIT) |
				BITMAP(VCC_IO_BIT) |
				BITMAP(VCC_PLL_BIT),
		/* mean care about cpua, dram, sys, cpus, dram_pll,
		 * vdd_adc, vcc_pl, vcc_io, vcc_cpvdd,
		 * vcc_ldoin, vcc_pll*/
		/* mean: donot need care about the voltage.*/
		.soc_pwr_dep.soc_pwr_dm_state.volt[VDD_SYS_BIT]      = 1000000,
		.soc_pwr_dep.soc_pwr_dm_state.volt[VDD_CPUA_BIT]      = 1000000,
		/*  mean all osc is off.*/
		.soc_pwr_dep.cpux_clk_state.osc_en
				= BITMAP(OSC_LOSC_BIT) |
					BITMAP(OSC_HOSC_BIT) |
					BITMAP(OSC_LDO1_BIT) |
					BITMAP(OSC_LDO0_BIT),
		/* mean pll5 is shutdowned & open by dram driver.*/
		.soc_pwr_dep.cpux_clk_state.init_pll_dis
			= BITMAP(PM_PLL_DRAM),
		.soc_pwr_dep.cpux_clk_state.exit_pll_en    = 0x0,
		.soc_pwr_dep.cpux_clk_state.pll_change     = 0x0,
		.soc_pwr_dep.cpux_clk_state.bus_change     = 0x0,
		.soc_pwr_dep.soc_dram_state.selfresh_flag     = 0x1,
		.soc_pwr_dep.soc_io_state.hold_flag     = 0x1,
		/* for pf port: set the io to disable state.;*/
		.soc_pwr_dep.soc_io_state.io_state[0]
			= {0x01c208b4,
				0x00f0f0ff,
				0x00707077},
		.soc_pwr_dep.soc_io_state.io_state[1]
			= {0x01c208b4,
				0x000f0f00,
				0x00070700},
	},

};

#endif
