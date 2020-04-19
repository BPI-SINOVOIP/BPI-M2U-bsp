/*
 * drivers/soc/sunxi/pm/resume1/resume1_sram.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Yanggq <yanggq@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/power/aw_pm.h>

#define SUPER_STANDBY_PA (0x43000000)

struct super_standby_para ssp;

void resume1_memcpy(void *dest, void *src, int n)
{
	char *tmp_src = (char *)NULL;
	char *tmp_dst = (char *)NULL;
	u32 *src_p = (u32 *)(src);
	u32 *dst_p = (u32 *)dest;

	if (!dest || !src)
		return;

	for (; n > 4; n -= 4)
		*dst_p++ = *src_p++;

	tmp_src = (char *)(src_p);
	tmp_dst = (char *)(dst_p);

	for (; n > 0; n--)
		*tmp_dst++ = *tmp_src++;
}


void resume1_sram_entry(void)
{
	resume1_memcpy((void *)&ssp, (void *)(SUPER_STANDBY_PA),
				sizeof(struct super_standby_para));
	jump_to_cpu_resume((void *)ssp.cpu_resume_entry);
}
