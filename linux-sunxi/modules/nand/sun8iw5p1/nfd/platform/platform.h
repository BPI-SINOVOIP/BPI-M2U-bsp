/****************************************************************************
 * the file for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
****************************************************************************/
#ifndef __NAND_PLATFORM_H__
#define __NAND_PLATFORM_H__

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/scatterlist.h>
#include <linux/freezer.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/irqreturn.h>
#include <linux/device.h>
#include <linux/pagemap.h>
#include <linux/reboot.h>
#include <linux/kmod.h>
#include <linux/compat.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/time.h>
#include <linux/sys_config.h>
#include <linux/sunxi-sid.h>

#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>

#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#include <linux/regulator/consumer.h>

#ifdef CONFIG_DMA_ENGINE
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma/sunxi-dma.h>
#endif

/*CONFIG_ARCH_SUN8IW1P1
CONFIG_ARCH_SUN8IW3P1
CONFIG_ARCH_SUN9IW1P1
CONFIG_ARCH_SUN50I
CONFIG_ARCH_SUN8IW7P1*/

#if defined CONFIG_ARCH_SUN50IW2P1      /*h5*/
#define  PLATFORM_NO            0
#define  PLATFORM_STRINGS    "allwinner,sun50iw2-nand"
#define  PLATFORM_BOOT0_ACESS_PAGE_TABLE_MODE    1
#define  PLATFORM_CLASS         0

#elif defined CONFIG_ARCH_SUN8IW5P1    /*A33*/
#define  PLATFORM_NO            1
#define  PLATFORM_STRINGS    "allwinner,sun8iw5-nand"
#define  PLATFORM_BOOT0_ACESS_PAGE_TABLE_MODE     0
#define  PLATFORM_CLASS         1

#elif defined CONFIG_ARCH_SUN8IW6P1     /*H8vr*/
#define  PLATFORM_NO            2
#define  PLATFORM_STRINGS     "allwinner,sun8iw6-nand"
#define  PLATFORM_BOOT0_ACESS_PAGE_TABLE_MODE     1
#define  PLATFORM_CLASS         0

#elif defined CONFIG_ARCH_SUN8IW10P1     /*B100*/
#define  PLATFORM_NO            3
#define  PLATFORM_STRINGS    "allwinner,sun8iw10-nand"
#define  PLATFORM_BOOT0_ACESS_PAGE_TABLE_MODE     1
#define  PLATFORM_CLASS         1

#elif defined CONFIG_ARCH_SUN8IW11P1     /*v40*/
#define  PLATFORM_NO            4
#define  PLATFORM_STRINGS    "allwinner,sun8iw11-nand"
#define  PLATFORM_BOOT0_ACESS_PAGE_TABLE_MODE     1
#define  PLATFORM_CLASS         1


#else
#error "please select a platform\n"
#endif

extern int NAND_Print_DBG(const char *fmt, ...);
extern int NAND_Print(const char *fmt, ...);


int NAND_IS_Secure_sys(void);
int NAND_Get_Platform_NO(void);
int NAND_Get_Boot0_Acess_Pagetable_Mode(void);
int nand_print_platform(void);


#endif

