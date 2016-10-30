/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _SUNXI_TIMER_H_
#define _SUNXI_TIMER_H_

#include <asm/arch/platform.h>

#define  SUNXI_WATCHDOG_CFG    (SUNXI_TIMER_BASE + 0x90)
#define  SUNXI_WATCHDOG_MODE   (SUNXI_TIMER_BASE + 0x94)

/* General purpose timer */
struct sunxi_timer {
	volatile u32 ctl;
	volatile u32 inter;
	volatile u32 val;
	uint  	 res[1];
};

/* Audio video sync*/
struct sunxi_avs {
	volatile u32 ctl;		/* 0x80 */
	volatile u32 cnt0;		/* 0x84 */
	volatile u32 cnt1;		/* 0x88 */
	volatile u32 div;		/* 0x8c */
};

/* 64 bit counter */
struct sunxi_64cnt {
	volatile u32 ctl;		/* 0xa0 */
	volatile u32 lo;			/* 0xa4 */
	volatile u32 hi;			/* 0xa8 */
};

/* Watchdog */
struct sunxi_wdog {
	volatile u32 ctrl;
	volatile u32 mode;

};

/* Alarm */
struct sunxi_alarm {
	volatile u32 ddhhmmss;	/* 0x10c */
	volatile u32 hhmmss;		/* 0x110 */
	volatile u32 en;			/* 0x114 */
	volatile u32 irqen;		/* 0x118 */
	volatile u32 irqsta;		/* 0x11c */
};


struct sunxi_timer_reg {
	volatile u32 tirqen;		/* 0x00 */
	volatile u32 tirqsta;	/* 0x04 */
	uint     res1[2];
	struct sunxi_timer timer[6];	/* We have 6 timers */
	uint  	 res2[4];
	struct sunxi_avs avs;
	struct sunxi_wdog wdog[1];
};

struct timer_list
{
	unsigned int expires;
	void (*function)(void *data);
	unsigned long data;
	int   timer_num;
};

static __inline void watchdog_disable(void)
{
	/* disable watchdog */
	writel(0, SUNXI_WATCHDOG_MODE);

	return ;
}

static __inline void watchdog_enable(void)
{
	/* enable watchdog */
	writel(3, SUNXI_WATCHDOG_MODE);

	return ;
}

extern int  timer_init(void);

extern void timer_exit(void);

extern void watchdog_disable(void);

extern void watchdog_enable(void);

extern void init_timer(struct timer_list *timer);

extern void add_timer(struct timer_list *timer);

extern void del_timer(struct timer_list *timer);

extern void __usdelay(unsigned long usec);

extern void __msdelay(unsigned long msec);

#define TMRC_INT_EN             (TIMER_BASE + 0x00)
#define TMRC_INT_ST             (TIMER_BASE + 0x04)

#define TMRC_CTRL(n)            (TIMER_BASE + 0x10 + 0x10 * (n) + 0x00)
#define TMRC_INTV(n)            (TIMER_BASE + 0x10 + 0x10 * (n) + 0x04)
#define TMRC_CURNT(n)           (TIMER_BASE + 0x10 + 0x10 * (n) + 0x08)

#endif

