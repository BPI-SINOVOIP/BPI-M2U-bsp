/*
 * Allwinner SoCs timer handling.
 *
 * Copyright (C) 2012 Maxime Ripard
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * Copyright (C) 2014 Superm Wu
 * Superm Wu <superm@allwinnertech.com>
 *
 * Based on code from
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Benn Huang <benn@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk/sunxi.h>
#include <linux/delay.h>
#ifdef CONFIG_ARCH_SUN3I
#include <asm/sched_clock.h>
#include <linux/clocksource.h>
#endif

#define TIMER_IRQ_EN_REG	0x00
#define TIMER_IRQ_EN(val)	(1 << val)
#define TIMER_IRQ_ST_REG	0x04
#define TIMER_IRQ_ST(val)	(1 << val)
#define TIMER_CTL_REG(val)	(0x10 * val + 0x10)
#define TIMER_CTL_ENABLE	(1 << 0)
#define TIMER_CTL_AUTORELOAD	(1 << 1)
#define TIMER_CTL_MODE_MASK	(1 << 7)
#define TIMER_CTL_PERIODIC	(0 << 7)
#define TIMER_CTL_ONESHOT	(1 << 7)
#define TIMER_INTVAL_REG(val)	(0x10 * val + 0x14)
#define TIMER_CNTVAL_REG(val)	(0x10 * val + 0x18)

static int timer_nr;
static void __iomem *timer_base;
static spinlock_t timer_spin_lock;

#ifdef CONFIG_ARCH_SUN3I
static DEFINE_SPINLOCK(clksrc_lock);
#define SUN3I_HRES_CLK_RATE		24000000
#define SUN3I_CLKSRC_ID		1

static cycle_t sun3i_clksrc_read(struct clocksource *cs)
{
	unsigned long flags;
	u32 cnt = 0;

	spin_lock_irqsave(&clksrc_lock, flags);
	cnt = readl(timer_base + TIMER_CNTVAL_REG(SUN3I_CLKSRC_ID));
	cnt = 0xffffffff - cnt;
	spin_unlock_irqrestore(&clksrc_lock, flags);

	return cnt;
}

static struct clocksource sun3i_clocksrc = {
	.name = "sun3i high-res couter",
	.list = {NULL, NULL},
	.rating = 300, /* perfect clock source */
	.read = sun3i_clksrc_read, /* read clock counter */
	.enable = 0, /* not define */
	.disable = 0, /* not define */
	.mask = CLOCKSOURCE_MASK(32), /* 32bits mask */
	.mult = 0, /* calculated by shift */
	.shift = 10, /* 32bit shift for */
	.max_idle_ns = 1000000000000ULL,
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
};

u32 sun3i_sched_clock_read(void)
{
	u32   cnt = 0;
	unsigned long flags;

	spin_lock_irqsave(&clksrc_lock, flags);
	cnt = readl(timer_base + TIMER_CNTVAL_REG(SUN3I_CLKSRC_ID));
	cnt = 0xffffffff - cnt;
	spin_unlock_irqrestore(&clksrc_lock, flags);

	return cnt;
}

void __init sun3i_clocksource_init(void)
{
	u32 data = 0;

	/*set interval value to max(0xffffffff)*/
	writel(0xffffffff, timer_base + TIMER_INTVAL_REG(SUN3I_CLKSRC_ID));

	/*set counter value to 0*/
	writel(0, timer_base + TIMER_CNTVAL_REG(SUN3I_CLKSRC_ID));

	data = readl(timer_base + TIMER_CTL_REG(SUN3I_CLKSRC_ID));
	/*continuse mode, no pre-scale, 24M clock src, auto reload, enabl*/
	data |= (0x01 << 2) | (0x01 << 1) | (0x01 << 0);
	writel(data, timer_base + TIMER_CTL_REG(SUN3I_CLKSRC_ID));

	/* calculate the mult by shift	*/
	sun3i_clocksrc.mult = clocksource_hz2mult(SUN3I_HRES_CLK_RATE,
			sun3i_clocksrc.shift);

	/* register clock source */
	clocksource_register(&sun3i_clocksrc);
	/* set sched clock */
	setup_sched_clock(sun3i_sched_clock_read, 32, SUN3I_HRES_CLK_RATE);
}
#endif

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
extern void tick_broadcast(const struct cpumask *mask);
#else
#define tick_broadcast	NULL
#endif

static void sunxi_clkevt_mode(enum clock_event_mode mode,
			      struct clock_event_device *clk)
{
	u32 u = readl(timer_base + TIMER_CTL_REG(timer_nr));

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		u &= ~(TIMER_CTL_MODE_MASK);
		writel(u | TIMER_CTL_ENABLE | TIMER_CTL_PERIODIC, timer_base + TIMER_CTL_REG(timer_nr));
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		u &= ~(TIMER_CTL_MODE_MASK);
		writel(u | TIMER_CTL_ENABLE | TIMER_CTL_ONESHOT, timer_base + TIMER_CTL_REG(timer_nr));
		break;

	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		writel(u & ~(TIMER_CTL_ENABLE), timer_base + TIMER_CTL_REG(timer_nr));
		udelay(2);
		break;
	}
}

static int sunxi_clkevt_next_event(unsigned long evt,
				   struct clock_event_device *unused)
{
	unsigned long flags;
	volatile u32 ctrl = 0;
	if(unused){
		spin_lock_irqsave(&timer_spin_lock, flags);

		/*disable timer*/
		ctrl = readl(timer_base + TIMER_CTL_REG(timer_nr));
		ctrl &= ~(TIMER_CTL_ENABLE);
		writel(ctrl, (timer_base + TIMER_CTL_REG(timer_nr)));
		udelay(1);

		/* set timer intervalue */
		writel(evt, (timer_base + TIMER_INTVAL_REG(timer_nr)));
		udelay(1);

		/*reload the timer intervalue*/
		ctrl = readl(timer_base + TIMER_CTL_REG(timer_nr));
		ctrl |= TIMER_CTL_AUTORELOAD;
		writel(ctrl, (timer_base + TIMER_CTL_REG(timer_nr)));
		while (readl(timer_base + TIMER_CTL_REG(timer_nr)) & TIMER_CTL_AUTORELOAD)
			;

		/*enable timer*/
		ctrl = readl(timer_base + TIMER_CTL_REG(timer_nr));
		ctrl |= TIMER_CTL_ENABLE;
		writel(ctrl, (timer_base + TIMER_CTL_REG(timer_nr)));
		spin_unlock_irqrestore(&timer_spin_lock, flags);

	}else{
		pr_warn("[%s][line-%d]set next event error!\n",__func__,__LINE__);
		BUG();
		return -EINVAL;
	}

	return 0;
}

static struct clock_event_device sunxi_clockevent = {
	.name = "sunxi_tick",
	.shift = 32,
	.rating = 300,
	.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode = sunxi_clkevt_mode,
	.set_next_event = sunxi_clkevt_next_event,
	.broadcast = tick_broadcast,
};

static irqreturn_t sunxi_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = (struct clock_event_device *)dev_id;

	writel(TIMER_IRQ_ST(timer_nr), timer_base + TIMER_IRQ_ST_REG);
	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction sunxi_timer_irq = {
	.name = "sunxi_timer",
	.flags = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler = sunxi_timer_interrupt,
	.dev_id = &sunxi_clockevent,
};

static void __init sunxi_timer_init(struct device_node *node)
{
	u32 rate = 0;
	u32 prescale = 0;
	int ret, irq;
	u32 val;

	timer_nr = of_alias_get_id(node, "global_timer");
	if (timer_nr < 0) {
		pr_err("[%s][line-%d] get soc_timer number error! timer_nr:%d\n", __func__, __LINE__, timer_nr);
		return;
	}

	if (of_property_read_u32(node, "clock-frequency", &rate)) {
		pr_err("[%s][line-%d] <%s> must have a clock-frequency property\n",
			__func__, __LINE__, node->name);
		return;
	}

	if (of_property_read_u32(node, "timer-prescale", &prescale)) {
		pr_err("[%s][line-%d] <%s> must have a timer-prescale property\n",
			__func__, __LINE__, node->name);
		return;
	}

	timer_base = of_iomap(node, 0);
	if (!timer_base)
		panic("Can't map registers");

	irq = irq_of_parse_and_map(node, 0);
	if (irq <= 0)
		panic("Can't parse IRQ");

	/* it is proved by test that clk-src=32000, prescale=1 on fpga */
#ifndef CONFIG_EVB_PLATFORM
	rate = 32000;
	prescale  = 1;
#endif

	writel(rate / (prescale * HZ),
	       timer_base + TIMER_INTVAL_REG(timer_nr));

	/* set clock source to HOSC, 16 pre-division */
	val = readl(timer_base + TIMER_CTL_REG(timer_nr));
	val &= ~(0x07 << 4);
	val &= ~(0x03 << 2);
	val |= (4 << 4) | (1 << 2);
	writel(val, timer_base + TIMER_CTL_REG(timer_nr));

	/* set mode to auto reload */
	val = readl(timer_base + TIMER_CTL_REG(timer_nr));
	writel(val | TIMER_CTL_AUTORELOAD, timer_base + TIMER_CTL_REG(timer_nr));

	ret = setup_irq(irq, &sunxi_timer_irq);
	if (ret)
		pr_warn("failed to setup irq %d\n", irq);

	/* Enable timer interrupt */
	val = readl(timer_base + TIMER_IRQ_EN_REG);
	writel(val | TIMER_IRQ_EN(timer_nr), timer_base + TIMER_IRQ_EN_REG);

	sunxi_clockevent.mult = div_sc(rate / prescale,
				NSEC_PER_SEC,
				sunxi_clockevent.shift);
	sunxi_clockevent.max_delta_ns = clockevent_delta2ns(0x7fffffff,
							    &sunxi_clockevent);
	sunxi_clockevent.min_delta_ns = clockevent_delta2ns(0x10,
							    &sunxi_clockevent);
	sunxi_clockevent.cpumask = cpumask_of(0);

	clockevents_register_device(&sunxi_clockevent);

#ifdef CONFIG_ARCH_SUN3I
	sun3i_clocksource_init();
#endif
}
CLOCKSOURCE_OF_DECLARE(sunxi, "allwinner,sunxi-timer",
		       sunxi_timer_init);
