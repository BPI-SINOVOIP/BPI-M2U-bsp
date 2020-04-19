/* g2d_driver_i.h
 *
 * Copyright (c)	2011 Allwinnertech Co., Ltd.
 *					2011 Yupu Tang
 *
 * @ G2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

#ifndef __G2D_DRIVER_I_H
#define __G2D_DRIVER_I_H

#include "g2d_bsp.h"
/* #include "g2d_bsp_v2.h" */

#define G2D_DEBUG	1
#ifdef	G2D_DEBUG
#define	DBG(format, args...) pr_debug("%s: " format, "G2D", ## args)
#else
#define	DBG(format, args...)
#endif
#define ERR(format, args...) pr_err("%s: " format, "G2D", ## args)
#define WARNING(format, args...) \
	pr_warn("%s: " format, "G2D", ## args)
#define INFO(format, args...) pr_info("%s: " format, "G2D", ## args)

#define MAX_G2D_MEM_INDEX	1000
#define	INTC_IRQNO_DE_MIX	SUNXI_IRQ_MP

#if ((defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1))
#define G2D_V2X_SUPPORT
#endif

struct info_mem {
	unsigned long phy_addr;
	void *virt_addr;
	__u32 b_used;
	__u32 mem_len;
};

typedef struct {
	struct device *dev;
	struct resource *mem;
	void __iomem *io;
	__u32 irq;
	struct mutex mutex;
	struct clk *clk;
	bool opened;
} __g2d_info_t;

typedef struct {
	__u32 mid;
	__u32 used;
	__u32 status;
	struct semaphore *g2d_finished_sem;
	struct semaphore *event_sem;
	wait_queue_head_t queue;
	__u32 finish_flag;
} __g2d_drv_t;

struct g2d_alloc_struct {
	__u32 address;
	__u32 size;
	__u32 u_size;
	struct g2d_alloc_struct *next;
};

typedef enum {
	G2D_CMD_BITBLT = 0x50,
	G2D_CMD_FILLRECT = 0x51,
	G2D_CMD_STRETCHBLT = 0x52,
	G2D_CMD_PALETTE_TBL = 0x53,
	G2D_CMD_QUEUE = 0x54,
	G2D_CMD_BITBLT_H = 0x55,
	G2D_CMD_FILLRECT_H = 0x56,
	G2D_CMD_BLD_H = 0x57,
	G2D_CMD_MASK_H = 0x58,

	G2D_CMD_MEM_REQUEST = 0x59,
	G2D_CMD_MEM_RELEASE = 0x5A,
	G2D_CMD_MEM_GETADR = 0x5B,
	G2D_CMD_MEM_SELIDX = 0x5C,
	G2D_CMD_MEM_FLUSH_CACHE = 0x5D,
	G2D_CMD_INVERTED_ORDER = 0x5E,
} g2d_cmd;

irqreturn_t g2d_handle_irq(int irq, void *dev_id);
int g2d_init(g2d_init_para *para);
int g2d_blit(g2d_blt *para);
int g2d_fill(g2d_fillrect *para);
int g2d_stretchblit(g2d_stretchblt *para);
/* int g2d_set_palette_table(g2d_palette *para); */
int g2d_wait_cmd_finish(void);
int g2d_cmdq(unsigned int para);

#endif /* __G2D_DRIVER_I_H */
