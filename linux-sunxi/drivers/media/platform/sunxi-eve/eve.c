/*
 * drivers\media\platform\eve.c
 * (C) Copyright 2016-2017
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * xiongyi<xiongyi@allwinnertech.com>
 *
 * Driver for embedded vision engine(EVE).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include "eve.h"
#include <linux/signal.h>
/* volatile deleted here*/
static unsigned char *eve_regs_init, *eve_time_gating;
static struct fasync_struct *async_queue;
static unsigned long eve_end_flag = 0x05, eve_int_flag;
static unsigned long eve_sig_sel;
static unsigned char reserror;
static unsigned long eve_irq_id;

static int my_delay(int ms)
{
	mdelay(ms);
	return 0;
}

static void readAndWrite(unsigned char *addr, unsigned long mask)
{
	unsigned long rd;
	rd	=	readl(addr);
	rd	&=	mask;
	writel(rd, addr);
	return;
}

static void readOrWrite(unsigned char *addr, unsigned long mask)
{
	unsigned long rd;
	rd	=	readl(addr);
	rd	|=	mask;
	writel(rd, addr);
	return;
}

static irqreturn_t eve_interrupt(unsigned long data)
{
	unsigned long now_int, int_mask;

	int_mask = readl(eve_regs_init + 0x04);
	int_mask = ((int_mask&0x03)<<3) | ((int_mask&0x1C00)>>10);
	now_int = readl(eve_regs_init);
	writel(now_int, eve_regs_init);
	eve_int_flag |= (now_int & int_mask);
	if (eve_int_flag > 0x07)
		reserror = 2;
	else
		reserror = 0;
	if ((eve_int_flag >= eve_end_flag) && async_queue) {
		eve_int_flag = 0x00;
		eve_sig_sel = 0x01;
		kill_fasync(&async_queue, SIGIO, POLL_IN);
	}
	return IRQ_HANDLED;
}

static int eve_fasync(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &async_queue);
}

static int evedev_open(struct inode *node, struct file *filp)
{
	return 0;
}

static int evedev_close(struct inode *node, struct file *filp)
{
	return eve_fasync(-1, filp, 0);
}

static ssize_t eve_write(struct file *filp,
	const char __user *buf,
	size_t count,
	loff_t *ppos)
{
	if (copy_from_user(&eve_end_flag,
	(void __user *)buf,
	sizeof(unsigned long)))
		return -EFAULT;
	return 0;
}

static ssize_t eve_read(struct file *filp,
	char __user *buf,
	size_t count,
	loff_t *ppos)
{
	if (copy_to_user((char *)buf, &eve_sig_sel, sizeof(unsigned long)))
		return -EFAULT;
	eve_sig_sel = 0x00;
	return 0;
}


static long eve_ioctl(struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	struct eve_register reg;
	unsigned long rddata = 0x5a5a5a5a;

	if (unlikely(copy_from_user(&reg, (void __user *)arg,
			sizeof(struct eve_register)))) {
		return -EFAULT;
	}
BACK_POINT:
	if (likely(cmd == EVE_WRITE_REGISTER)) {
		writel(reg.value, eve_regs_init + reg.addr);
		return 0;
	} else if (cmd == EVE_READ_RESNUM) {
		if (reserror > 0) {
			if (reserror == 2)
				goto RESETTH;
			return 0;
		}
		return readl(eve_regs_init + reg.addr);
	}
	switch (cmd) {
	case EVE_READ_REGISTER:
		rddata = readl(eve_regs_init + reg.addr);
		return rddata;
	case EVE_OPEN_CLK:
		reg.value	=	reg.value | 0x80000000;
		writel(reg.value, eve_time_gating + PLL_EVE_CONTROL_REGISTER);
		readOrWrite(eve_time_gating + REG_MCLK_SYS, 0x00200000);
		readOrWrite(eve_time_gating + REG_AHB_EVE, 0x00000001);
		readOrWrite(eve_time_gating + EVE_CLOCK_REGISTER, 0x80000000);
		readAndWrite(eve_time_gating + REG_AHB_EVE, 0xfffeffff);
		my_delay(5);
		readOrWrite(eve_time_gating + REG_AHB_EVE, 0x00010000);
		break;
	case EVE_SYS_RESET:
		readAndWrite(eve_time_gating + REG_AHB_EVE, 0xfffeffff);
		my_delay(5);
		readOrWrite(eve_time_gating + REG_AHB_EVE, 0x00010000);
		break;
RESETTH:
	case EVE_MOD_RESET:
		reserror = 1;
		writel(0x0000001f, eve_regs_init);
		writel(0x0000001c, eve_regs_init + 4);
		writel(0x00000000, eve_regs_init + 4);
		my_delay(5);
		writel(0x00000020, eve_regs_init + 4);
		writel(0x00000000, eve_regs_init + 4);
		goto BACK_POINT;
		/* break here is  left out intentionally */
	case EVE_CLOSE_CLK:
		readAndWrite(eve_time_gating + EVE_CLOCK_REGISTER, 0x7fffffff);
		readAndWrite(eve_time_gating + REG_AHB_EVE, 0xfffffffe);
		readAndWrite(eve_time_gating + REG_MCLK_SYS, 0xffdfffff);
		readAndWrite(eve_time_gating + PLL_EVE_CONTROL_REGISTER,
		0x7fffffff);
		readAndWrite(eve_time_gating + REG_AHB_EVE, 0xfffeffff);
		break;
	case EVE_PLL_SET:
		reg.value	=	reg.value | 0x80000000;
		writel(reg.value, eve_time_gating +
		EVE_CLOCK_REGISTER);
		break;
	default:
		break;
	}
	return 0;
}

static const struct file_operations eve_fops = {
	.owner	=	THIS_MODULE,
	.unlocked_ioctl	=	eve_ioctl,
	.fasync	=	eve_fasync,
	.read	=	eve_read,
	.write	=	eve_write,
	.open	=	evedev_open,
	.release	=	evedev_close
};

static struct miscdevice eve_dev = {
	.minor	=	MISC_DYNAMIC_MINOR,
	.name	=	DEVICE_NAME,
	.fops	=	&eve_fops
};

static int __init eve_init(void)
{
	struct device_node *node;
	int r = misc_register(&eve_dev);

	if (r >= 0)
		dev_info((const struct device *)&eve_dev,
		"EVE device had been registered! %d\n", r);
	else
		dev_info((const struct device *)&eve_dev,
		"EVE device register failed! %d\n", r);

	node = of_find_compatible_node(NULL, NULL,
	"allwinner,sunxi-aie-eve");
	eve_irq_id = irq_of_parse_and_map(node, 0);
	r = request_irq(eve_irq_id, (irq_handler_t)eve_interrupt, 0,
	DEVICE_NAME, NULL);
	if (r < 0) {
		dev_info((const struct device *)&eve_dev,
		"Request EVE Irq error! return %d\n", r);
		return -EFAULT;
	}
	dev_info((const struct device *)&eve_dev,
	"Request EVE Irq success! return %d, irq_id = %ld\n",
	r,  eve_irq_id);
	eve_regs_init = (unsigned char *)of_iomap(node, 0);
	dev_info((const struct device *)&eve_dev,
	"[EVE] remap from %#010x to %#010lx\n", 0x01500000,
	(unsigned long)eve_regs_init);
	eve_time_gating = (unsigned char *)of_iomap(node, 1);
	dev_info((const struct device *)&eve_dev,
	"[EVE] remap from %#010x to %#010lx\n",
	0x03001000, (unsigned long)eve_time_gating);
	return 0;
}

static void __exit eve_exit(void)
{
	free_irq(eve_irq_id, NULL);
	iounmap(eve_regs_init);
	iounmap(eve_time_gating);
	misc_deregister(&eve_dev);
	dev_info((const struct device *)&eve_dev,
						"EVE device has been removed!\n");
};

module_init(eve_init);
module_exit(eve_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("XiongYi<xiongyi@allwinnertech.com>");
MODULE_DESCRIPTION("EVE device driver");
MODULE_VERSION("0.21");
MODULE_ALIAS("platform:EVE(AW1721)");
