/*
 * Allwinner SoCs de-interlace driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _SUNXI_DI_H
#define _SUNXI_DI_H

#include <linux/types.h>
#include <linux/wait.h>
#include <linux/pm.h>
#include "di.h"

#define DI_RESERVED_MEM

#define DI_MODULE_NAME "deinterlace"
#define DI_TIMEOUT                      30                    /* DI-Interlace 30ms timeout */
#define DI_MODULE_TIMEOUT               0x1055

#ifdef CONFIG_ARCH_SUN3IW1
#define FLAG_WIDTH                      (736)
#define FLAG_HIGH                       (288)
#else
#define FLAG_WIDTH                      (2048)
#define FLAG_HIGH                       (1100)
#endif

#if ((defined CONFIG_ARCH_SUN8IW12P1) ||\
	(defined CONFIG_ARCH_SUN8IW17P1) ||\
	(defined CONFIG_ARCH_SUN50IW6P1))
#define DI_V2X_SUPPORT
#endif

#if (defined CONFIG_ARCH_SUN50IW6P1)
#define IOMMU_SUPPORT
#endif

#if ((defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1))
#define DI_V22
#define SUPPORT_DNS
#endif

#ifdef CONFIG_ARCH_SUN50IW6P1
#define DI_V23
#endif

typedef struct {
	void __iomem *base_addr;
	struct __di_mem_t mem_in_params;
	struct __di_mem_t mem_out_params;
	atomic_t     di_complete;
	atomic_t     enable;
	wait_queue_head_t wait;
	void *in_flag_phy;
	void *out_flag_phy;
	size_t  flag_size;
	u32  irq_number;
	u32  time_value;
#ifdef CONFIG_PM
	struct dev_pm_domain di_pm_domain;
#endif
	unsigned int users;
}di_struct, *pdi_struct;

#define	DI_IOC_MAGIC		'D'
#define	DI_IOCSTART		_IOWR(DI_IOC_MAGIC, 0, struct __di_rectsz_t)
#define DI_IOCSTART2	_IOWR(DI_IOC_MAGIC, 1, struct __di_rectsz_t)
#define DI_IOCSETMODE	_IOWR(DI_IOC_MAGIC, 2, struct __di_mode_t)

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_TEST = 1U << 4,
};

#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	 printk(KERN_DEBUG fmt , ## arg)

#endif

