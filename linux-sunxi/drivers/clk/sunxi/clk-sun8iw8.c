/*
 * Copyright (C) 2016 Allwinnertech, czy <chenzunyin@allwinnertech.com>
 *
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk-private.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/clk/sunxi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include "clk-sunxi.h"
#include "clk-factors.h"
#include "clk-periph.h"
#include "clk-sun8iw8.h"
#include "clk-sun8iw8_tbl.c"

#ifndef CONFIG_EVB_PLATFORM
#define LOCKBIT(x) 31
#else
#define LOCKBIT(x) x
#endif



static DEFINE_SPINLOCK(clk_lock);
void __iomem *sunxi_clk_base;
void __iomem *sunxi_clk_cpus_base;
int sunxi_clk_maxreg = SUNXI_CLK_MAX_REG;
int cpus_clk_maxreg;

#ifdef CONFIG_SUNXI_CLK_DUMMY_DEBUG
unsigned int dummy_reg[1024];

unsigned int
dummy_readl(unsigned int *regaddr)
{
	unsigned int val;
	val = *regaddr;

	pr_info(KERN_INFO"--%s-- dummy_readl to read reg 0x%x with val 0x%x\n",
		__func__,
		((unsigned int) regaddr - (unsigned int) &dummy_reg[0]), val);

	return val;
}

void
dummy_writel(unsigned int val, unsigned int *regaddr)
{
	*regaddr = val;
	pr_info(KERN_WARN"--%s-- dummy_writel to write reg 0x%x with val 0x%x\n",
		__func__,
		((unsigned int) regaddr - (unsigned int) &dummy_reg[0]), val);
}

void
dummy_reg_init(void)
{
	memset(dummy_reg, 0x0, sizeof(dummy_reg));
	dummy_reg[PLL_CPU / 4] = 0x00001000;
	dummy_reg[PLL_AUDIO / 4] = 0x00035514;
	dummy_reg[PLL_VIDEO0 / 4] = 0x03006207;
	dummy_reg[PLL_DDR0 / 4] = 0x00001800;
	dummy_reg[PLL_PERIPH0 / 4] = 0x00041811;
	dummy_reg[PLL_VIDEO1 / 4] = 0x03006207;
	dummy_reg[PLL_24M / 4] = 0xa00c1801;
	dummy_reg[PLL_PERIPH1 / 4] = 0x00041811;
	dummy_reg[PLL_DE / 4] = 0x03006207;
	dummy_reg[PLL_DDR1 / 4] = 0x00001800;

	dummy_reg[CPU_CFG / 4] = 0x00010300;
	dummy_reg[AHB1_CFG / 4] = 0x00001010;
	dummy_reg[APB2_CFG / 4] = 0x01000000;
	dummy_reg[PLL_LOCK / 4] = 0x000000FF;
	dummy_reg[CPU_LOCK / 4] = 0x000000FF;
}
#endif /* of CONFIG_SUNXI_CLK_DUMMY_DEBUG */

/*
 * ns  nw  ks  kw  ms  mw  ps  pw
 * d1s d1w d2s d2w {frac   out mode} en-s sdmss
 * sdmsw sdmpat sdmval
 */
SUNXI_CLK_FACTORS(pll_cpu, 8, 5, 4, 2, 0, 2, 16, 2, 0, 0, 0, 0, 0, 0, 0,
		   31, 24, 0, PLL_CPUPAT, 0xd1303333);
SUNXI_CLK_FACTORS(pll_audio, 8, 7, 0, 0, 0, 5, 16, 4, 0, 0, 0, 0, 0, 0, 0,
		   31, 24, 1, PLL_AUDIOPAT, 0xc0010d84);
SUNXI_CLK_FACTORS(pll_video, 8, 7, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 1, 25, 24,
		   31, 20, 0, PLL_VIDEOPAT, 0xd1303333);
SUNXI_CLK_FACTORS(pll_ve, 8, 7, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 1, 25, 24,
		   31, 20, 0, PLL_VEPAT, 0xd1303333);
SUNXI_CLK_FACTORS_UPDATE(pll_ddr0, 8, 5, 4, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 31, 24, 0, PLL_DDR0PAT, 0xd1303333, 20);
SUNXI_CLK_FACTORS(pll_periph0, 8, 5, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		   31, 0, 0, 0, 0);
SUNXI_CLK_FACTORS(pll_isp, 8, 7, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 1, 25, 24,
		   31, 20, 0, PLL_ISPPAT, 0xd1303333);
SUNXI_CLK_FACTORS(pll_periph1, 8, 5, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		   31, 20, 0, PLL_PERI1PAT, 0xd1303333);
SUNXI_CLK_FACTORS_UPDATE(pll_ddr1, 8, 6, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 31, 24, 0, PLL_DDR1PAT, 0xf1303333, 30);

static int
get_factors_pll_cpu(u32 rate, u32 parent_rate,
		     struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllcpu_max ? pllcpu_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	return sunxi_clk_com_ftr_sr(&sunxi_clk_factor_pll_cpu, factor,
				     factor_pllcpu_tbl, index,
				     sizeof(factor_pllcpu_tbl)
				     / sizeof(struct sunxi_clk_factor_freq));
}

static int
get_factors_pll_isp(u32 rate, u32 parent_rate,
		     struct clk_factors_value *factor)
{
	u64 tmp_rate;
	int index;
	if (!factor)
		return -1;

	tmp_rate = rate > pllisp_max ? pllisp_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;
	if (sunxi_clk_com_ftr_sr
	    (&sunxi_clk_factor_pll_isp, factor, factor_pllisp_tbl, index,
	     sizeof(factor_pllisp_tbl) /
	     sizeof(struct sunxi_clk_factor_freq)))
		return -1;
	if (rate == 297000000) {

		factor->frac_mode = 0;
		factor->frac_freq = 1;
		factor->factorm = 0;
	  }
	else if (rate == 270000000) {

		factor->frac_mode = 0;
		factor->frac_freq = 0;
		factor->factorm = 0;
	  }
	else {

		factor->frac_mode = 1;
		factor->frac_freq = 0;
	  }

	return 0;
}

static int
get_factors_pll_ve(u32 rate, u32 parent_rate,
		    struct clk_factors_value *factor)
{
	u64 tmp_rate;
	int index;
	if (!factor)
		return -1;

	tmp_rate = rate > pllve_max ? pllve_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;
	if (sunxi_clk_com_ftr_sr
	    (&sunxi_clk_factor_pll_ve, factor, factor_pllve_tbl, index,
	     sizeof(factor_pllve_tbl) /
	     sizeof(struct sunxi_clk_factor_freq)))
		return -1;
	if (rate == 297000000) {

		factor->frac_mode = 0;
		factor->frac_freq = 1;
		factor->factorm = 0;
	  }
	else if (rate == 270000000) {

		factor->frac_mode = 0;
		factor->frac_freq = 0;
		factor->factorm = 0;
	  }
	else {

		factor->frac_mode = 1;
		factor->frac_freq = 0;
	  }

	return 0;
}

static int
get_factors_pll_audio(u32 rate, u32 parent_rate,
		       struct clk_factors_value *factor)
{
	if (rate == 22579200) {

		factor->factorn = 6;
		factor->factorm = 0;
		factor->factorp = 7;
		sunxi_clk_factor_pll_audio.sdmval = 0xc0010d84;
	  }
	else if (rate == 24576000) {

		factor->factorn = 13;
		factor->factorm = 0;
		factor->factorp = 13;
		sunxi_clk_factor_pll_audio.sdmval = 0xc000ac02;
	  }
	else
		return -1;

	return 0;
}

static int
get_factors_pll_video(u32 rate, u32 parent_rate,
		       struct clk_factors_value *factor)
{
	u64 tmp_rate;
	int index;

	if (!factor)
		return -1;

	tmp_rate = rate > pllvideo_max ? pllvideo_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	if (sunxi_clk_com_ftr_sr
	    (&sunxi_clk_factor_pll_video, factor, factor_pllvideo_tbl, index,
	     sizeof(factor_pllvideo_tbl) /
	     sizeof(struct sunxi_clk_factor_freq)))
		return -1;

	if (rate == 297000000) {

		factor->frac_mode = 0;
		factor->frac_freq = 1;
		factor->factorm = 0;
	  }
	else if (rate == 270000000) {

		factor->frac_mode = 0;
		factor->frac_freq = 0;
		factor->factorm = 0;
	  }
	else {

		factor->frac_mode = 1;
		factor->frac_freq = 0;
	  }

	return 0;
}

static int
get_factors_pll_ddr0(u32 rate, u32 parent_rate,
		      struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllddr0_max ? pllddr0_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	return sunxi_clk_com_ftr_sr(&sunxi_clk_factor_pll_ddr0, factor,
				     factor_pllddr0_tbl, index,
				     sizeof(factor_pllddr0_tbl) /
				     sizeof(struct sunxi_clk_factor_freq));
}

static int
get_factors_pll_periph0(u32 rate, u32 parent_rate,
			 struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllperiph0_max ? pllperiph0_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	return sunxi_clk_com_ftr_sr(&sunxi_clk_factor_pll_periph0, factor,
				     factor_pllperiph0_tbl, index,
				     sizeof(factor_pllperiph0_tbl) /
				     sizeof(struct sunxi_clk_factor_freq));
}

static int
get_factors_pll_periph1(u32 rate, u32 parent_rate,
			 struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = (rate > pllperiph1_max) ? pllperiph1_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	return sunxi_clk_com_ftr_sr(&sunxi_clk_factor_pll_periph1, factor,
				     factor_pllperiph1_tbl, index,
				     sizeof(factor_pllperiph1_tbl) /
				     sizeof(struct sunxi_clk_factor_freq));
}


static int
get_factors_pll_ddr1(u32 rate, u32 parent_rate,
		      struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;

	if (!factor)
		return -1;

	tmp_rate = rate > pllddr1_max ? pllddr1_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;

	return sunxi_clk_com_ftr_sr(&sunxi_clk_factor_pll_ddr1, factor,
				     factor_pllddr1_tbl, index,
				     sizeof(factor_pllddr1_tbl) /
				     sizeof(struct sunxi_clk_factor_freq));
}

/*
 * pll_cpux: 24*N*K/(M*P)
 */
static unsigned long
calc_rate_pll_cpu(u32 parent_rate, struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);

	tmp_rate = tmp_rate * (factor->factorn + 1) * (factor->factork + 1);
	do_div(tmp_rate, (factor->factorm + 1) * (1 << factor->factorp));

	return (unsigned long) tmp_rate;
}

/*
 * pll_audio:24*N/(M*P)
 */
static unsigned long
calc_rate_pll_audio(u32 parent_rate, struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);

	if ((factor->factorn == 6)
	    && (factor->factorm == 0) && (factor->factorp == 7))
		return 22579200;
	else if ((factor->factorn == 13)
		 && (factor->factorm == 0) && (factor->factorp == 13))
		return 24576000;
	else {
		  tmp_rate = tmp_rate * (factor->factorn + 1);
		  do_div(tmp_rate,
			  (factor->factorm + 1) * (factor->factorp + 1));
		  return (unsigned long) tmp_rate;
	  }
}

/*
 * pll_video0:24*N/M
 */
static unsigned long
calc_rate_media(u32 parent_rate, struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);

	if (factor->frac_mode == 0) {
		if (factor->frac_freq == 1)
			return 297000000;
		else
			return 270000000;
	  }
	else {
		  tmp_rate = tmp_rate * (factor->factorn + 1);
		  do_div(tmp_rate, factor->factorm + 1);
		  return (unsigned long) tmp_rate;
	  }
}

/*
 * pll_ddr0:24*N/M
 */
static unsigned long
calc_rate_pll_ddr0(u32 parent_rate, struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);

	tmp_rate = tmp_rate * (factor->factorn + 1) * (factor->factork + 1);
	do_div(tmp_rate, factor->factorm + 1);

	return (unsigned long) tmp_rate;
}

/*
 *pll_ddr1
*/
static unsigned long
calc_rate_pll_ddr1(u32 parent_rate, struct clk_factors_value *factor)
{
	u64 tmp_rate = (parent_rate ? parent_rate : 24000000);
	tmp_rate = tmp_rate * (factor->factorn + 1);
	do_div(tmp_rate, factor->factorm + 1);

	return (unsigned long) tmp_rate;
}


/*
 * pll_periph0:24*N*K/2
 */
static unsigned long
calc_rate_pll_periph(u32 parent_rate, struct clk_factors_value *factor)
{
	return (unsigned long) (parent_rate ? (parent_rate / 2) : 12000000)
		* (factor->factorn + 1) * (factor->factork + 1);
}



static const char * const hosc_parents[] = { "hosc" };
static const char * const cpu_parents[] = {
		"losc", "hosc", "pll_cpu", "pll_cpu" };
static const char * const axi_parents[] = { "cpu" };
static const char * const pll_periphahb0_parents[] = { "pll_periph0" };
static const char * const ahb0_parents[] = {
		"losc", "hosc", "axi", "pll_periphahb0" };
static const char * const apb0_parents[] = { "ahb0" };
static const char * const apb1_parents[] = {
		"losc", "hosc", "pll_periph0", "pll_periph0" };
static const char * const periph_parents[] = {
		"hosc", "pll_periph0", "pll_periph1", "" };
static const char * const ahb0mod_parents[] = { "ahb0" };
static const char * const apb0mod_parents[] = { "apb0" };
static const char * const apb1mod_parents[] = { "apb1" };
static const char * const ahb1_parents[] = { "ahb0", "pll_periph0d2", "", ""};
static const char * const losc_parents[] = { "losc" };
static const char * const ahb1mod_parents[] = { "ahb1" };
static const char * const i2s_parents[] = {"pll_audiox8", "pll_audiox4", "pll_audiox2", "pll_audio"};
static const char * const ss_parents[] = {"hosc", "pll_periph0", "", ""};
static const char * const de_parents[] = {"pll_video", "", "pll_periph0", "", "", "", "", ""};
static const char * const csi_m_parents[] = {"hosc", "pll_video", "pll_periph1", "pll_periph0",
			 "", "", "", ""};
static const char * const csi_s_parents[] = {"pll_video", "pll_isp", "", "", "", "", "", ""};
static const char * const ve_parents[] = {"pll_ve"};
static const char * const adda_parents[] = {"pll_audio"};
static const char * const usbohci12m_parents[] = {"hoscx2", "hosc", "losc", ""};
static const char * const usbohci_parents[]    = {"usbohci_16"};
static const char * const sdmmc2_parents[] = {"sdmmc2_mode"};
static const char * const mbus_parents[] = {"hosc", "pll_periph0x2", "pll_ddr0", ""};
static const char * const mipicsi_parents[] = {"pll_video", "pll_periph0", "pll_isp", ""};
static const char * const lvds_parents[] = {"hosc"};

struct factor_init_data sunxi_factos[] = {
	{
	 .name = "pll_cpu",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags =
	 CLK_GET_RATE_NOCACHE | CLK_RATE_FLAT_FACTORS | CLK_RATE_FLAT_DELAY,
	 .reg = PLL_CPU,
	 .lock_reg = PLL_CPU,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 0,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_cpu,
	 .get_factors = &get_factors_pll_cpu,
	 .calc_rate = &calc_rate_pll_cpu,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_audio",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = 0,
	 .reg = PLL_AUDIO,
	 .lock_reg = PLL_AUDIO,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 1,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_audio,
	 .get_factors = &get_factors_pll_audio,
	 .calc_rate = &calc_rate_pll_audio,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_video",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = 0,
	 .reg = PLL_VIDEO,
	 .lock_reg = PLL_VIDEO,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 2,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_video,
	 .get_factors = &get_factors_pll_video,
	 .calc_rate = &calc_rate_media,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_ddr0",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = CLK_GET_RATE_NOCACHE,
	 .reg = PLL_DDR0,
	 .lock_reg = PLL_DDR0,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 3,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_ddr0,
	 .get_factors = &get_factors_pll_ddr0,
	 .calc_rate = &calc_rate_pll_ddr0,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_periph0",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = 0,
	 .reg = PLL_PERIPH0,
	 .lock_reg = PLL_PERIPH0,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 4,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_periph0,
	 .get_factors = &get_factors_pll_periph0,
	 .calc_rate = &calc_rate_pll_periph,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_periph1",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = 0,
	 .reg = PLL_PERIPH1,
	 .lock_reg = PLL_PERIPH1,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 5,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_periph1,
	 .get_factors = &get_factors_pll_periph1,
	 .calc_rate = &calc_rate_pll_periph,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_ve",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = 0,
	 .reg = PLL_VE,
	 .lock_reg = PLL_VE,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 6,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_ve,
	 .get_factors = &get_factors_pll_ve,
	 .calc_rate = &calc_rate_media,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_isp",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = 0,
	 .reg = PLL_ISP,
	 .lock_reg = PLL_ISP,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 7,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_isp,
	 .get_factors = &get_factors_pll_isp,
	 .calc_rate = &calc_rate_media,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
	{
	 .name = "pll_ddr1",
	 .parent_names = hosc_parents,
	 .num_parents = 1,
	 .flags = CLK_GET_RATE_NOCACHE,
	 .reg = PLL_DDR1,
	 .lock_reg = PLL_DDR1,
	 .lock_bit = LOCKBIT(28),
	 .pll_lock_ctrl_reg = 0,
	 .lock_en_bit = 8,
	 .lock_mode = PLL_LOCK_NONE_MODE,
	 .config = &sunxi_clk_factor_pll_ddr1,
	 .get_factors = &get_factors_pll_ddr1,
	 .calc_rate = &calc_rate_pll_ddr1,
	 .priv_ops = (struct clk_ops *) NULL,
	 },
};

struct sunxi_clk_comgate com_gates[] = {
	{"csi", 0, 0x3, BUS_GATE_SHARE | RST_GATE_SHARE | MBUS_GATE_SHARE, 0},
};

/*
 * SUNXI_CLK_PERIPH(name, mux_reg, mux_sft, mux_wid, div_reg, div_msft,
 *			div_mwid, div_nsft, div_nwid, gate_flag, en_reg,
 *			rst_reg, bus_gate_reg, drm_gate_reg, en_sft, rst_sft,
 *			bus_gate_sft, dram_gate_sft, lock,
			com_gate, com_gate_off)
 */

/*
 * Bus Module Clock
 */
SUNXI_CLK_PERIPH(cpu, CPU_CFG, 16, 2, 0, 0, 0, 0, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(axi, 0, 0, 0, CPU_CFG, 0, 2, 0, 0, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(pll_periphahb0, 0, 0, 0, AHB1_CFG, 6, 2, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(ahb0, AHB1_CFG, 12, 2, AHB1_CFG, 0, 0, 4, 2, 0,
		  0, 0, 0, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(apb0, 0, 0, 0, AHB1_CFG, 0, 0, 8, 2, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(ahb1, AHB2_CFG, 0, 2, 0, 0, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(apb1, APB2_CFG, 24, 2, APB2_CFG, 0, 5, 16, 2, 0, 0, 0, 0,
		  0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(dma, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0, BUS_GATE0,
		  0, 0, 6, 6, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc0_mod, SD0_CFG, 24, 2, SD0_CFG, 0, 4, 16, 2, 0,
		SD0_CFG, 0, 0, 0, 31, 8, 8, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc0_bus, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_GATE0,
		0, 0, 0, 8, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc0_rst, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0, 0,
		0, 0, 8, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc1_mod, SD1_CFG, 24, 2, SD1_CFG, 0, 4, 16, 2, 0,
		SD1_CFG, 0, 0, 0, 31, 9, 9, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc1_bus, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_GATE0,
		0, 0, 0, 9, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc1_rst, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0, 0,
		0, 0, 9, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc2_mode, SD2_CFG, 24, 2, SD2_CFG, 30, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc2_module, 0, 0, 0, SD2_CFG, 0, 4, 16, 2, 0,
		SD2_CFG, 0, 0, 0, 31, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc2_bus, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_GATE0,
		0, 0, 0, 10, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(sdmmc2_rst, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0, 0,
		0, 0, 10, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(spi0, SPI0_CFG, 24, 2, SPI0_CFG, 0, 4, 16, 2, 0, SPI0_CFG,
		BUS_RST0, BUS_GATE0, 0, 31, 20, 20, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(i2s0, I2S0_CFG, 16, 2, 0, 0, 0, 0, 0, 0, I2S0_CFG, BUS_RST3,
		BUS_GATE2, 0, 31, 12, 12, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(ss, SS_CFG, 24, 2, SS_CFG, 0, 4, 16, 2, 0, SS_CFG, BUS_RST0,
		BUS_GATE0, 0, 31, 5, 5, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(usbphy0, 0, 0, 0, 0, 0, 0, 0, 0, 0, USB_CFG, USB_CFG,
		0, 0, 8, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(usbohci012m, USB_CFG, 20, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(usbohci_16, 0, 0, 0, 0, 0, 0, 0, 0, 0, USB_CFG, 0, 0,
		0, 16, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(usbohci0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0,
		BUS_GATE0, DRAM_GATE, 0, 29, 29, 18, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(usbehci0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0,
		BUS_GATE0, DRAM_GATE, 0, 26, 26, 17, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(usbotg, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0,
		BUS_GATE0, 0, 0, 24, 24, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(de, DE_CFG, 24, 3, DE_CFG, 0, 4, 0, 0, 0, DE_CFG,
		BUS_RST1, BUS_GATE1, 0, 31, 12, 12, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(tcon, TCON_CFG, 24, 3, TCON_CFG, 0, 4, 0, 0, 0, TCON_CFG,
		BUS_RST1, BUS_GATE1, 0, 31, 4, 4, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(csi0_s, 0, 0, 0, 0, 0, 0, 0, 0, 0, CSI0_CFG, BUS_RST1,
		BUS_GATE1, DRAM_GATE, 31, 8, 8, 1, &clk_lock, &com_gates[0], 0);

SUNXI_CLK_PERIPH(csi0_m, CSI0_CFG, 8, 3, CSI0_CFG, 0, 5, 0, 0, 0, CSI0_CFG,
		BUS_RST1, BUS_GATE1, DRAM_GATE, 15, 8, 8, 1, &clk_lock, &com_gates[0], 1);

SUNXI_CLK_PERIPH(csi1_s, CSI1_CFG, 24, 3, CSI1_CFG, 16, 4, 0, 0, 0, CSI1_CFG, BUS_RST1,
		BUS_GATE1, DRAM_GATE, 31, 8, 8, 1, &clk_lock, &com_gates[0], 2);

SUNXI_CLK_PERIPH(csi1_m, CSI1_CFG, 8, 3, CSI1_CFG, 0, 5, 0, 0, 0, CSI1_CFG,
		BUS_RST1, BUS_GATE1, DRAM_GATE, 15, 8, 8, 1, &clk_lock, &com_gates[0], 3);

SUNXI_CLK_PERIPH(ve, 0, 0, 0, VE_CFG, 0, 0, 16, 3, 0, VE_CFG, BUS_RST1,
		BUS_GATE1, DRAM_GATE, 31, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(adda, 0, 0, 0, 0, 0, 0, 0, 0, 0, ADDA_CFG, BUS_RST3,
		BUS_GATE2, 0, 31, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(avs, 0, 0, 0, 0, 0, 0, 0, 0, 0, AVS_CFG, 0, 0, 0, 31, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(mbus, MBUS_CFG, 24, 2, MBUS_CFG, 0, 3, 0, 0, 0,
		MBUS_CFG, 0, 0, 0, 31, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(mipicsi, MIPI_CSI, 24, 2, MIPI_CSI, 0, 4, 0, 0, 0,
		MIPI_CSI, 0, 0, 0, 31, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(gmac, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST0, BUS_GATE0, 0,
		0, 17, 17, 0, &clk_lock, NULL, 0);

/*
 * Uart Module Clock
 */
SUNXI_CLK_PERIPH(uart0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST4,
		  BUS_GATE3, 0, 0, 16, 16, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(uart1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST4,
		  BUS_GATE3, 0, 0, 17, 17, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(uart2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST4,
		BUS_GATE3, 0, 0, 18, 18, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(twi0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST4,
		BUS_GATE3, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(twi1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST4,
		BUS_GATE3, 0, 0, 1, 1, 0, &clk_lock, NULL, 0);

/*
 * Other small modules
 */
SUNXI_CLK_PERIPH(pio, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_GATE2,
		  0, 0, 0, 5, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(losc_out, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		  LOSC_OUT_GATE, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(ephy, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST2,
		BUS_GATE4, 0, 0, 2, 0, 0, &clk_lock, NULL, 0);

SUNXI_CLK_PERIPH(lvds, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, BUS_RST2,
		0, 0, 0, 0, 0, 0, &clk_lock, NULL, 0);


struct periph_init_data sunxi_periphs_init[] = {
	{"cpu", CLK_GET_RATE_NOCACHE, cpu_parents, ARRAY_SIZE(cpu_parents),
	 &sunxi_clk_periph_cpu},
	{"axi", 0, axi_parents, ARRAY_SIZE(axi_parents),
	 &sunxi_clk_periph_axi},
	{"pll_periphahb0", CLK_IGNORE_SYNCBOOT, pll_periphahb0_parents,
	 ARRAY_SIZE(pll_periphahb0_parents),
	 &sunxi_clk_periph_pll_periphahb0},

	{"ahb0", 0, ahb0_parents, ARRAY_SIZE(ahb0_parents),
	 &sunxi_clk_periph_ahb0},
	{"apb0", 0, apb0_parents, ARRAY_SIZE(apb0_parents),
	 &sunxi_clk_periph_apb0},
	{"apb1", 0, apb1_parents, ARRAY_SIZE(apb1_parents),
	 &sunxi_clk_periph_apb1},
	{"ahb1", 0, ahb1_parents, ARRAY_SIZE(ahb1_parents),
	 &sunxi_clk_periph_ahb1},

	{"sdmmc0_mod", 0, periph_parents, ARRAY_SIZE(periph_parents),
	&sunxi_clk_periph_sdmmc0_mod},

	{"sdmmc0_bus", 0, periph_parents, ARRAY_SIZE(periph_parents),
	&sunxi_clk_periph_sdmmc0_bus},

	{"sdmmc0_rst", 0, periph_parents, ARRAY_SIZE(periph_parents),
	&sunxi_clk_periph_sdmmc0_rst},

	{"sdmmc1_mod", 0, periph_parents, ARRAY_SIZE(periph_parents),
	&sunxi_clk_periph_sdmmc1_mod},

	{"sdmmc1_bus", 0, periph_parents, ARRAY_SIZE(periph_parents),
	&sunxi_clk_periph_sdmmc1_bus},

	{"sdmmc1_rst", 0, periph_parents, ARRAY_SIZE(periph_parents),
	&sunxi_clk_periph_sdmmc1_rst},

	{"sdmmc2_mode", 0, periph_parents, ARRAY_SIZE(periph_parents),
	&sunxi_clk_periph_sdmmc2_mode},

	{"sdmmc2_module", 0, sdmmc2_parents, ARRAY_SIZE(sdmmc2_parents),
	&sunxi_clk_periph_sdmmc2_module},

	{"sdmmc2_bus", 0, sdmmc2_parents, ARRAY_SIZE(sdmmc2_parents),
	&sunxi_clk_periph_sdmmc2_bus},

	{"sdmmc2_rst", 0, sdmmc2_parents, ARRAY_SIZE(sdmmc2_parents),
	&sunxi_clk_periph_sdmmc2_rst},

	{"spi0", 0, periph_parents, ARRAY_SIZE(periph_parents), &sunxi_clk_periph_spi0},

	{"i2s0", 0, i2s_parents, ARRAY_SIZE(i2s_parents), &sunxi_clk_periph_i2s0},

	{"ss", 0, ss_parents, ARRAY_SIZE(ss_parents), &sunxi_clk_periph_ss},

	{"usbphy0", 0, hosc_parents, ARRAY_SIZE(hosc_parents), &sunxi_clk_periph_usbphy0},

	{"usbohci_16", 0, ahb1mod_parents, ARRAY_SIZE(ahb1mod_parents), &sunxi_clk_periph_usbohci_16},

	{"usbohci0", 0, usbohci_parents, ARRAY_SIZE(usbohci_parents), &sunxi_clk_periph_usbohci0},

	{"usbohci012m", 0, usbohci12m_parents, ARRAY_SIZE(usbohci12m_parents),
	&sunxi_clk_periph_usbohci012m},

	{"usbehci0", 0, ahb1mod_parents, ARRAY_SIZE(ahb1mod_parents), &sunxi_clk_periph_usbehci0},

	{"usbotg", 0, ahb1mod_parents, ARRAY_SIZE(ahb1mod_parents), &sunxi_clk_periph_usbotg},

	{"de", 0, de_parents, ARRAY_SIZE(de_parents), &sunxi_clk_periph_de},

	{"tcon", 0, de_parents, ARRAY_SIZE(de_parents), &sunxi_clk_periph_tcon},

	{"csi0_s", 0, hosc_parents, ARRAY_SIZE(hosc_parents), &sunxi_clk_periph_csi0_s},

	{"csi0_m", 0, csi_m_parents, ARRAY_SIZE(csi_m_parents), &sunxi_clk_periph_csi0_m},

	{"csi1_s", 0, csi_s_parents, ARRAY_SIZE(csi_s_parents), &sunxi_clk_periph_csi1_s},

	{"csi1_m", 0, csi_m_parents, ARRAY_SIZE(csi_m_parents), &sunxi_clk_periph_csi1_m},

	{"ve", 0, ve_parents, ARRAY_SIZE(ve_parents), &sunxi_clk_periph_ve},

	{"adda", 0, adda_parents, ARRAY_SIZE(adda_parents), &sunxi_clk_periph_adda},

	{"avs", 0, hosc_parents, ARRAY_SIZE(hosc_parents), &sunxi_clk_periph_avs},

	{"mbus", 0, mbus_parents, ARRAY_SIZE(mbus_parents), &sunxi_clk_periph_mbus},

	{"mipicsi", 0, mipicsi_parents, ARRAY_SIZE(mipicsi_parents), &sunxi_clk_periph_mipicsi},

	{"gmac", 0, ahb1mod_parents, ARRAY_SIZE(ahb1mod_parents), &sunxi_clk_periph_gmac},

	{"ephy", 0, ahb0mod_parents, ARRAY_SIZE(ahb0mod_parents), &sunxi_clk_periph_ephy},

	{"lvds", 0, lvds_parents, ARRAY_SIZE(lvds_parents), &sunxi_clk_periph_lvds},

	{"twi0", 0, apb1mod_parents, ARRAY_SIZE(apb1mod_parents),
	&sunxi_clk_periph_twi0},

	{"twi1", 0, apb1mod_parents, ARRAY_SIZE(apb1mod_parents),
	&sunxi_clk_periph_twi1},

	{"dma", 0, ahb0mod_parents, ARRAY_SIZE(ahb0mod_parents),
	 &sunxi_clk_periph_dma},

	{"pio", 0, apb0mod_parents, ARRAY_SIZE(apb0mod_parents),
	 &sunxi_clk_periph_pio},

	{"uart0", 0, apb1mod_parents, ARRAY_SIZE(apb1mod_parents),
	 &sunxi_clk_periph_uart0},

	{"uart1", 0, apb1mod_parents, ARRAY_SIZE(apb1mod_parents),
	 &sunxi_clk_periph_uart1},

	{"uart2", 0, apb1mod_parents, ARRAY_SIZE(apb1mod_parents),
	&sunxi_clk_periph_uart2},

	{"losc_out", 0, losc_parents, ARRAY_SIZE(losc_parents),
	 &sunxi_clk_periph_losc_out}
};

void __init
sunxi_init_clocks(void)
{
}

#ifdef CONFIG_OF
/*
 * set default rate for clk
 */
static int
__set_clk_rates(struct device_node *node, struct clk *clk)
{
	u32 assigned_clock_rates = 0;
	int res = -1;

	/*set pll default rate here ,
	and make you know it is setted succeed or not */
	if (!of_property_read_u32 (node, "assigned-clock-rates",
				&assigned_clock_rates)) {
		u32 real_clock_rate = 0;
		clk_set_rate(clk, assigned_clock_rates);
		real_clock_rate = clk_get_rate(clk);

		if (real_clock_rate == assigned_clock_rates)
			res = 0;

		pr_info("%s-set_default_rate=%u %s!\n",
			__clk_get_name(clk), assigned_clock_rates,
			res ? "failed" : "success");
	  }

	return res;
}

/*
 * set default clk source for clk
 */
static int
__set_clk_parents(struct device_node *node, struct clk *clk)
{
	int index = 0, rc;
	struct of_phandle_args clkspec;
	struct clk *pclk;

	rc = of_parse_phandle_with_args(node, "assigned-clock-parents",
					"#clock-cells", index, &clkspec);
	if (rc < 0)
		/* skip empty (null) phandles */
		return rc;

	pclk = of_clk_get_from_provider(&clkspec);
	if (IS_ERR(pclk)) {
		pr_warn("clk: couldn't get parent clock %d for %s\n",
			index, node->full_name);
		  return PTR_ERR(pclk);
	  }

	rc = clk_set_parent(clk, pclk);
	if (rc < 0)
		pr_err("%s-set_default_source=%s failed at: %d\n",
			__clk_get_name(clk), __clk_get_name(pclk), rc);

	return rc;
}

/*
 * of_sunxi_clocks_init() - Clocks initialize
 */
void
of_sunxi_clocks_init(struct device_node *node)
{
#ifdef CONFIG_SUNXI_CLK_DUMMY_DEBUG
	sunxi_clk_base = &dummy_reg[0];
	dummy_reg_init();
#else
	sunxi_clk_base = of_iomap(node, 0);
#endif
	sunxi_clk_periph_losc_out.gate.bus = of_iomap(node, 2);
	/*do some initialize arguments here */
	sunxi_clk_factor_initlimits();

	/*sunxi_clk_get_factors_ops(&pll_mipi_ops);
	   pll_mipi_ops.get_parent = get_parent_pll_mipi;
	   pll_mipi_ops.set_parent = set_parent_pll_mipi;
	   pll_mipi_ops.enable = clk_enable_pll_mipi;
	   pll_mipi_ops.disable = clk_disable_pll_mipi; */
	pr_info("%s : sunxi_clk_base[0x%lx]\n", __func__,
		 (unsigned long) sunxi_clk_base);

	clk_add_alias("pll1", NULL, "pll_cpu", NULL);
	clk_add_alias("pll2", NULL, "pll_audio", NULL);
	clk_add_alias("pll3", NULL, "pll_video", NULL);
	clk_add_alias("pll4", NULL, "pll_ddr0", NULL);
	clk_add_alias("pll5", NULL, "pll_periph0", NULL);
	clk_add_alias("pll6", NULL, "pll_periph1", NULL);
	clk_add_alias("pll7", NULL, "pll_ve", NULL);
	clk_add_alias("pll8", NULL, "pll_isp", NULL);
	clk_add_alias("pll9", NULL, "pll_ddr1", NULL);
}

void
of_sunxi_fixed_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	u32 rate;

	if (of_property_read_u32(node, "clock-frequency", &rate))
		return;

	of_property_read_string(node, "clock-output-names", &clk_name);

	clk = clk_register_fixed_rate(NULL, clk_name, NULL, CLK_IS_ROOT,
				       rate);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	  }
}

void
of_sunxi_fixed_factor_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *parent_name;
	u32 div, mult;

	if (of_property_read_u32(node, "clock-div", &div)) {
	pr_err("%s Fixed factor clock <%s> must have a clock-div property\n",
	__func__, node->name);
		return;
	  }

	if (of_property_read_u32(node, "clock-mult", &mult)) {
	pr_err("%s Fixed factor clock <%s> must have a clokc-mult property\n",
	__func__, node->name);
		return;
	  }

	of_property_read_string(node, "clock-output-names", &clk_name);
	parent_name = of_clk_get_parent_name(node, 0);

	clk = clk_register_fixed_factor(NULL, clk_name, parent_name, 0, mult,
					div);
	if (!IS_ERR(clk)) {
		clk_register_clkdev(clk, clk_name, NULL);
		of_clk_add_provider(node, of_clk_src_simple_get, clk);
	  }
}

/*
 * of_pll_clk_setup() - Setup function for pll factors clk
 */
void
of_pll_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	const char *lock_mode = NULL;
	struct factor_init_data *factor;
	int i;
	int ret;

	of_property_read_string(node, "clock-output-names", &clk_name);
	ret = of_property_read_string(node, "lock-mode", &lock_mode);

	/*get pll clk init config */
	for (i = 0; i < ARRAY_SIZE(sunxi_factos); i++) {

		factor = &sunxi_factos[i];
		if (strcmp(clk_name, factor->name))
			continue;

		if (!strcmp(lock_mode, "new"))
			factor->lock_mode = PLL_LOCK_NEW_MODE;
		else if (!strcmp(lock_mode, "old"))
			factor->lock_mode = PLL_LOCK_OLD_MODE;
		else
			factor->lock_mode = PLL_LOCK_NONE_MODE;

		  /*register clk */
		clk = sunxi_clk_register_factors(NULL, sunxi_clk_base,
						&clk_lock, factor);
		  /*add to of */
		  if (!IS_ERR(clk)) {
			clk_register_clkdev(clk, clk_name, NULL);
			of_clk_add_provider(node, of_clk_src_simple_get,
							clk);
			__set_clk_parents(node, clk);
			__set_clk_rates(node, clk);
			return;
		    }
	  }
	pr_err("clk %s not found in %s\n", clk_name, __func__);
}

/*
 * of_periph_clk_setup() - Setup function for periph clk
 */
void
of_periph_clk_setup(struct device_node *node)
{
	struct clk *clk;
	const char *clk_name = node->name;
	struct periph_init_data *pd;
	unsigned int i;


	of_property_read_string(node, "clock-output-names", &clk_name);

	/*get periph clk init config */
	for (i = 0; i < ARRAY_SIZE(sunxi_periphs_init); i++) {

		pd = &sunxi_periphs_init[i];
		if (strcmp(clk_name, pd->name))
			continue;

		  /*register clk */
		  clk = sunxi_clk_register_periph(pd, sunxi_clk_base);
		  /*add to of */
		  if (!IS_ERR(clk)) {
			clk_register_clkdev(clk, clk_name, NULL);
			of_clk_add_provider(node, of_clk_src_simple_get,
							clk);
			__set_clk_parents(node, clk);
			__set_clk_rates(node, clk);
			return;
		}
	}
	pr_err("clk %s not found in %s\n", clk_name, __func__);
}

CLK_OF_DECLARE(sunxi_clocks_init, "allwinner,sunxi-clk-init",
		of_sunxi_clocks_init);
CLK_OF_DECLARE(sunxi_fixed_clk, "allwinner,fixed-clock",
		of_sunxi_fixed_clk_setup);
CLK_OF_DECLARE(pll_clk, "allwinner,sunxi-pll-clock", of_pll_clk_setup);
CLK_OF_DECLARE(sunxi_fixed_factor_clk, "allwinner,fixed-factor-clock",
		of_sunxi_fixed_factor_clk_setup);
CLK_OF_DECLARE(periph_clk, "allwinner,sunxi-periph-clock",
		of_periph_clk_setup);

#endif
