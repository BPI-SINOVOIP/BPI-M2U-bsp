/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __DEV_LCD_H__
#define __DEV_LCD_H__

#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>	/* wake_up_process() */
#include <linux/kthread.h>	/* kthread_create()?°Èkthread_run() */
#include <linux/err.h>		/* IS_ERR()?°ÈPTR_ERR() */
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
/* #include <mach/platform.h> */

/*#include <mach/sys_config.h>*/
#include <video/drv_display.h>

extern int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops);
extern s32 DRV_DISP_Init(void);
extern s32 Fb_Init(u32 from);
extern void LCD_set_panel_funs(void);

#define PRINTF(msg...) { \
	pr_warn("[LCD] "); \
	printk(msg); \
	}

#define __inf(msg...)       {\
	pr_warn("[LCD] "); \
	printk(msg); \
	}

#define __msg(msg...)       {\
	pr_warn("[LCD] file:%s,line:%d:    ", \
	__FILE__, __LINE__); \
	printk(msg); \
	}

#define __wrn(msg...)       {\
	pr_warn("[LCD WRN] file:%s,line:%d:    ",\
	__FILE__, __LINE__); \
	printk(msg); \
	}

#define __here__            {\
	pr_warn) "[LCD] file:%s,line:%d\n", \
	__FILE__, __LINE__); \
	}

struct sunxi_lcd_drv {
	struct device *dev;
	struct cdev *lcd_cdev;
	dev_t devid;
	struct class *lcd_class;
	struct sunxi_disp_source_ops src_ops;
};

#include "panels/panels.h"

#endif
