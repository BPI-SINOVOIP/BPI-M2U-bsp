/*
 * arch/arm/mach-sunxi/platsmp.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi smp ops header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PLAT_SMP_H
#define __PLAT_SMP_H

#ifdef CONFIG_CPU_IDLE_SUNXI
#include <linux/sunxi-cpuidle.h>
#endif

#define get_nr_cores()					\
	({						\
		unsigned int __val;			\
		asm("mrc	p15, 1, %0, c9, c0, 2"	\
		    : "=r" (__val)			\
		    :					\
		    : "cc");				\
		((__val>>24) & 0x03) + 1;		\
	})

#if defined(CONFIG_ARCH_SUN8IW17)
#define SUNXI_CCI400_PBASE		(0x030D0000)
#define SUNXI_CLUSTER0_CPUX_CFG_PBASE	(0x09010000)
#define SUNXI_CLUSTER1_CPUX_CFG_PBASE	(0x09810000)
#define SUNXI_CPUS_CFG_PBASE		(0x07000400)
#define CLUSTER_CPU_STATUS		(0x80)
#define CPU_RST_CTRL			(0x00)
#define CLUSTER_CTRL0			(0x10)
#define CLUSTER_CTRL1			(0x14)
#define CLUSTER_PWRON_RST(cluster)	(0x40 + (cluster) * 0x40)
#define CPU_PWR_CLAMP(cluster, cpu)	(0x50 + (cluster*16 + cpu)*0x4)
#define CLUSTER_PWROFF_GATING(cluster)	(0x44 + (cluster) * 0x40)
#else
#ifdef CONFIG_ARCH_SUN8IW5
#define SUNXI_CPUCFG_PBASE		(0x01F01C00)
#define SUNXI_R_PRCM_PBASE		(0x01F01400)
#define SUNXI_CPU_PWROFF_REG		(0x100)
#define SUNXI_CPUX_PWR_CLAMP(x)		(0x140 + (x)*0x04)
#else
#define SUNXI_CPUCFG_PBASE		(0x01C25C00)
#endif
#define CPUCFG_CPUX_RST_CTRL(x)		(0x40 + (x)*0x40)
#define CPUCFG_CPUX_STATUS_REG(x)	(0x48 + (x)*0x40)
#define CPUCFG_GENER_CTRL_REG		(0x184)
#define CPUCFG_DEBUG_REG1		(0x1e4)
#if !(defined CONFIG_ARCH_SUN8IW10) && !(defined CONFIG_ARCH_SUN8IW5)
#define CPUCFG_PWROFF_GATING_REG	(0x110)
#define CPUCFG_PWR_SWITCH_REG(x)	(0x120 + (x)*0x04)
#endif
#endif

#if defined(CONFIG_ARCH_SUN8IW10)
#define SUNXI_RTC_PBASE				(0x01C20400)
#define CPU_SOFT_ENTRY_REG0			(0x1e4)
#elif defined(CONFIG_ARCH_SUN8IW11)
#define SUNXI_SYSCTL_PBASE			(0x01C00000)
#define CPU_SOFT_ENTRY_REG0			(0xbc)
#elif defined(CONFIG_ARCH_SUN8IW17)
#define SUN8IW17_RTC_PBASE			(0x07000000)
#define CPU_SOFT_ENTRY_REG0			(0x1bc)
#elif defined(CONFIG_ARCH_SUN8IW5)
#define CPU_SOFT_ENTRY_REG0			(0x1a4)
#endif

extern void __iomem *sunxi_rtc_base;
extern void __iomem *sunxi_sysctl_base;
extern void __iomem *sunxi_cpucfg_base;

static inline void sunxi_set_secondary_entry(void *entry)
{
#if (defined CONFIG_ARCH_SUN8IW10) || (defined CONFIG_ARCH_SUN8IW17)
	writel((u32)entry, sunxi_rtc_base + CPU_SOFT_ENTRY_REG0);
#elif (defined CONFIG_ARCH_SUN8IW5)
	writel((u32)entry, sunxi_cpucfg_base + CPU_SOFT_ENTRY_REG0);
#else
	writel((u32)entry, sunxi_sysctl_base + CPU_SOFT_ENTRY_REG0);
#endif
}

#if defined(CONFIG_MCPM) && !defined(CONFIG_SUNXI_TRUSTZONE)
extern void sun8i_power_up_setup(unsigned int affinity_level);
#endif

#ifndef CONFIG_MCPM
extern void sunxi_cpu_die(unsigned int cpu);
extern int  sunxi_cpu_kill(unsigned int cpu);
extern int  sunxi_cpu_disable(unsigned int cpu);
extern void __iomem *sunxi_r_prcm_base;

static inline int sunxi_is_wfi_mode(int cpu)
{
#ifdef CONFIG_EVB_PLATFORM
	return readl(sunxi_cpucfg_base + CPUCFG_CPUX_STATUS_REG(cpu)) & (1<<2);
#else
	return 1;
#endif
}

static inline void sunxi_enable_cpu(int cpu)
{
	unsigned int value;
#if (defined CONFIG_ARCH_SUN8IW5)
	u32 pwr_reg;
#endif

	/*  Assert nCOREPORESET LOW and hold L1RSTDISABLE LOW.
	    Ensure DBGPWRDUP is held LOW to prevent any external
	    debug access to the processor.
	*/
	/* assert cpu core reset */
	writel(0, sunxi_cpucfg_base + CPUCFG_CPUX_RST_CTRL(cpu));

	/* L1RSTDISABLE hold low */
	value = readl(sunxi_cpucfg_base + CPUCFG_GENER_CTRL_REG);
	value &= ~(1<<cpu);
	writel(value, sunxi_cpucfg_base + CPUCFG_GENER_CTRL_REG);
	udelay(10);

	/* DBGPWRDUP hold low */
	value = readl(sunxi_cpucfg_base + CPUCFG_DEBUG_REG1);
	value &= ~(1<<cpu);
	writel(value, sunxi_cpucfg_base + CPUCFG_DEBUG_REG1);

#if !(defined CONFIG_ARCH_SUN8IW10) && !(defined CONFIG_ARCH_SUN8IW5)
	/* Release power switch */
	writel(0xFE, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu));
	udelay(20);
	writel(0xF8, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu));
	udelay(10);
	writel(0xE0, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu));
	udelay(10);
	writel(0x80, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu));
	udelay(10);
	writel(0x00, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu));
	udelay(20);
	while (0x00 != readl(sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu))) {
		;
	}

	/* Clear power-off gating */
	value = readl((void *)(sunxi_cpucfg_base + CPUCFG_PWROFF_GATING_REG));
	value &= ~(1<<cpu);
	writel(value, (void *)(sunxi_cpucfg_base + CPUCFG_PWROFF_GATING_REG));
	udelay(20);
#endif

#if (defined CONFIG_ARCH_SUN8IW5)

	/* step2: release power clamp */
	writel(0xFE, (void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)));
	udelay(20);
	writel(0xF8, (void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)));
	udelay(10);
	writel(0xE0, (void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)));
	udelay(10);
	writel(0x80, (void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)));
	udelay(10);
	writel(0x00, (void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)));
	udelay(20);
	while (0x00 != readl((void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)))) {
		;
	}
	/* step3: clear power-off gating */
	pwr_reg = readl((void *)(sunxi_r_prcm_base + SUNXI_CPU_PWROFF_REG));
	pwr_reg &= ~(0x00000001<<cpu);
	writel(pwr_reg, (void *)(sunxi_r_prcm_base + SUNXI_CPU_PWROFF_REG));
	udelay(20);

#endif

	/* de-assert core reset */
	writel(3, sunxi_cpucfg_base + CPUCFG_CPUX_RST_CTRL(cpu));

	/* assert DBGPWRDUP signal */
	value = readl(sunxi_cpucfg_base + CPUCFG_DEBUG_REG1);
	value |= (1<<cpu);
	writel(value, sunxi_cpucfg_base + CPUCFG_DEBUG_REG1);
}

#if !(defined CONFIG_ARCH_SUN8IW10) && !(defined CONFIG_ARCH_SUN8IW5)
static inline void sunxi_disable_cpu(int cpu)
{
	unsigned int value;

	/* assert cpu core reset */
	writel(0, sunxi_cpucfg_base + CPUCFG_CPUX_RST_CTRL(cpu));

	/* DBGPWRDUP hold low */
	value = readl(sunxi_cpucfg_base + CPUCFG_DEBUG_REG1);
	value &= ~(1<<cpu);
	writel(value, sunxi_cpucfg_base + CPUCFG_DEBUG_REG1);

	/* power gating off */
	value = readl(sunxi_cpucfg_base + CPUCFG_PWROFF_GATING_REG);
	value |= (1<<cpu);
	writel(value, sunxi_cpucfg_base + CPUCFG_PWROFF_GATING_REG);
	udelay(20);

	/* power switch off */
	writel(0xff, sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu));
	udelay(30);
	while (0xff != readl(sunxi_cpucfg_base + CPUCFG_PWR_SWITCH_REG(cpu))) {
		;
	}
}
#endif

#if (defined CONFIG_ARCH_SUN8IW5)
static inline void sunxi_disable_cpu(int cpu)
{
	u32 pwr_reg;

	/* step1: set up power-off signal */
	pwr_reg = readl(sunxi_r_prcm_base + SUNXI_CPU_PWROFF_REG);
	pwr_reg |= (1<<cpu);
	writel(pwr_reg, sunxi_r_prcm_base + SUNXI_CPU_PWROFF_REG);
	udelay(20);

#if defined(CONFIG_ARCH_SUN8IW5)
	/* step2: active the power output clamp */
	writel(0xff, (void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)));
	udelay(30);
	while (0xff != readl((void *)(sunxi_r_prcm_base + SUNXI_CPUX_PWR_CLAMP(cpu)))) {
		;
	}
#endif
}

#endif
#endif

#endif /* __PLAT_SMP_H */
