/*
 * drivers/soc/sunxi/pm/resume1/resume1_entry.S
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Yanggq <yanggq@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

	.text
	.arm
	.globl _start
_start:
	/* config soc para here if needed */

	/* to get cpu_resume entry */
	bl resume1_sram_entry
	b .                                 @ infinite loop

	.align 4
	.text
	.globl jump_to_cpu_resume
jump_to_cpu_resume:
	/* Set the return pointer */
	mov r12, r0
	mov lr, r12
	mov pc, lr
	.end
