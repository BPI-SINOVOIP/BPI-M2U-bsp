/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include "standby.h"
#include "../pm_config.h"

void mem_status_init(char *name)
{
	return;
}

void mem_status_init_nommu(void)
{
	return;
}

void mem_status_clear(void)
{
	int i = 0;

	while (i < STANDBY_STATUS_REG_NUM) {
		*(volatile int *)(STANDBY_STATUS_REG + i * 4) = 0x0;
		i++;
	}
	return;

}

void mem_status_exit(void)
{
	return;
}

void save_mem_status(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_STATUS_REG + 0x0c) = val;
	asm volatile ("dsb");
	asm volatile ("isb");
	return;
}

__u32 get_mem_status(void)
{
	return *(volatile __u32 *)(STANDBY_STATUS_REG + 0x0c);
}

void show_mem_status(void)
{
	int i = 0;

	while (i < STANDBY_STATUS_REG_NUM) {
		printk("addr %x, value = %x. \n",
		       (STANDBY_STATUS_REG + i * 4),
		       *(volatile int *)(STANDBY_STATUS_REG + i * 4));
		i++;
	}
}

void save_mem_status_nommu(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_STATUS_REG_PA + 0x0c) = val;
	return;
}

void save_cpux_mem_status_nommu(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_STATUS_REG_PA + 0x04) = val;
	return;
}

void save_super_flags(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_SUPER_FLAG_REG) = val;
	asm volatile ("dsb");
	asm volatile ("isb");
	return;
}

void save_super_addr(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_SUPER_ADDR_REG) = val;
	asm volatile ("dsb");
	asm volatile ("isb");
	return;
}
