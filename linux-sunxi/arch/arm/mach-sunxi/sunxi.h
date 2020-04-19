/*
 * Generic definitions for Allwinner SunXi SoCs
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_SUNXI_H
#define __MACH_SUNXI_H

#if defined(CONFIG_ARCH_SUN8IW10P1)
#define SUNXI_SRAM_A1_PBASE     (0x00000000)
#define SUNXI_SRAM_A1_SIZE      (0x4000)
#define SUNXI_SRAM_C_PBASE     (0x00004000)
#define SUNXI_SRAM_C_SIZE      (0x9000)

#elif defined(CONFIG_ARCH_SUN8IW11P1)
#define SUNXI_SRAM_A1_PBASE     (0x00000000)
#define SUNXI_SRAM_A1_SIZE      (0x4000)
#define SUNXI_SRAM_A_PBASE      (0x00000000)
#define SUNXI_SRAM_A_SIZE      (0xC000)
#elif defined(CONFIG_ARCH_SUN8IW5P1)
#define SUNXI_SRAM_A1_PBASE     (0x00000000)
#define SUNXI_SRAM_A1_SIZE      (0x8000)
#define SUNXI_SRAM_A2_PBASE       (0x00044000)
#define SUNXI_SRAM_A2_SIZE        (0x00010000)
#elif defined(CONFIG_ARCH_SUN8IW17P1)
#define SUNXI_SRAM_A1_PBASE     (0x00020000)
#define SUNXI_SRAM_A1_SIZE      (0x8000)
#define SUNXI_SRAM_C_PBASE      (0x00028000)
#define SUNXI_SRAM_C_SIZE       (0x00028000)
#define SUNXI_SRAM_A2_PBASE     (0x00100000)
#define SUNXI_SRAM_A2_SIZE      (0x00120000)
#elif defined(CONFIG_ARCH_SUN50IW1P1)
#define SUNXI_SRAM_BROM_PBASE     (0x00000000)
#define SUNXI_SRAM_BROM_SIZE      (0x00010000)
#define SUNXI_SRAM_A1_PBASE       (0x00010000)
#define SUNXI_SRAM_A1_SIZE        (0x00008000)
#define SUNXI_SRAM_A2_PBASE       (0x00040000)
#define SUNXI_SRAM_A2_SIZE        (0x00014000)
#define SUNXI_SRAM_C_PBASE        (0x00018000)
#define SUNXI_SRAM_C_SIZE         (0x00028000)
#elif defined(CONFIG_ARCH_SUN8IW8P1)
#define SUNXI_SRAM_A1_PBASE       (0x00000000)
#define SUNXI_SRAM_A1_SIZE        (0x4000)
#define SUNXI_SRAM_C_PBASE        (0x00004000)
#define SUNXI_SRAM_C_SIZE         (0x00014000)
#elif defined(CONFIG_ARCH_SUN3IW1P1)
#define SUNXI_SRAM_A1_PBASE       (0x00000000)
#define SUNXI_SRAM_A1_SIZE        (0x2000)
#define SUNXI_SRAM_C2_PBASE        (0x00002000)
#define SUNXI_SRAM_C2_SIZE         (0x2000)
#endif

#if defined(CONFIG_SMP) && !defined(CONFIG_MCPM)
extern struct smp_operations sunxi_smp_ops;
#endif

#ifdef CONFIG_SUNXI_TRUSTZONE
extern void __init sunxi_firmware_init(void);
#endif
#endif /* __MACH_SUNXI_H */
