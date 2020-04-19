/*
 * arch/arm/mach-sunxi/include/mach/irqs.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi irq defination header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_MACH_IRQS_H
#define __SUNXI_MACH_IRQS_H


/* irq total number = gic irq max + gpio irq max(reserve 256) */

#undef  NR_IRQS
#define NR_IRQS                 (256)

#ifndef NR_IRQS
#error "NR_IRQS not defined by the board-specific files"
#endif

#define SUNXI_GIC_START          32

#if defined CONFIG_ARCH_SUN8IW5P1
#include "sun8i/irqs-sun8iw5p1.h"
#elif defined CONFIG_ARCH_SUN8IW6P1
#include "sun8i/irqs-sun8iw6p1.h"
#endif

#endif
