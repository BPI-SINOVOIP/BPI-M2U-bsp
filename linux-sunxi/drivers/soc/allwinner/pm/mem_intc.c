/*
********************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_int.c
* By      : gq.yang
* Version : v1.0
* Date    : 2012-11-3 20:13
* Descript: interrupt for platform mem
* Update  : date                auther      ver     notes
********************************************************************************
*/
#include "pm_i.h"

static void *int_reg_base;
static u32 gic_d_len;
static __u32	IntEnable0Reg, IntEnable1Reg, IntMask0Reg, IntMask1Reg;

/*
* STANDBY INTERRUPT INITIALISE
*
* Description: mem interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*/
__s32 mem_int_init(void)
{
	u32 *base = 0;

	pm_get_dev_info("intc", 0, &base, &gic_d_len);
	int_reg_base = base;

	return 0;
}

__s32 mem_int_save(void)
{
	/*backup intterupt controller registers */
	IntEnable0Reg = *(volatile __u32 *)(int_reg_base + SW_INT_ENABLE_REG0);
	IntEnable1Reg = *(volatile __u32 *)(int_reg_base + SW_INT_ENABLE_REG1);
	IntMask0Reg   = *(volatile __u32 *)(int_reg_base + SW_INT_MASK_REG0);
	IntMask1Reg   = *(volatile __u32 *)(int_reg_base + SW_INT_MASK_REG1);

	/* Disable & clear all interrupts */
	*(volatile __u32 *)(int_reg_base + SW_INT_ENABLE_REG0) = 0;
	*(volatile __u32 *)(int_reg_base + SW_INT_ENABLE_REG1) = 0;
	*(volatile __u32 *)(int_reg_base + SW_INT_MASK_REG0) = 0xffffffff;
	*(volatile __u32 *)(int_reg_base + SW_INT_MASK_REG1) = 0xffffffff;
	return 0;

}
/*
* mem_int_restore
*
* Description: mem interrupt exit.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*/
__s32 mem_int_restore(void)
{
	/* restore intterupt controller registers */
	*(volatile __u32 *)(int_reg_base + SW_INT_ENABLE_REG0) = IntEnable0Reg;
	*(volatile __u32 *)(int_reg_base + SW_INT_ENABLE_REG1) = IntEnable1Reg;
	*(volatile __u32 *)(int_reg_base + SW_INT_MASK_REG0)  = IntMask0Reg;
	*(volatile __u32 *)(int_reg_base + SW_INT_MASK_REG1) = IntMask1Reg;

	return 0;
}

/*
********************************************************************************
*                                       QUERY INTERRUPT
*
* Description: enable interrupt.
*
* Arguments  : src  interrupt source number.
*
* Returns    : 0/-1;
********************************************************************************
*/
__s32 mem_enable_int(enum interrupt_source_e src)
{
	__u32 tmpGrp = (__u32) src >> 5;
	__u32 tmpSrc = (__u32) src & 0x1f;

	if (0 == src)
		return -1;

	/* enable valid wakeup source intterupts */
	*(volatile __u32 *)(int_reg_base + SW_INT_ENABLE_REG0 + tmpGrp * 4) |=
								(1 << tmpSrc);

	*(volatile __u32 *)(int_reg_base + SW_INT_MASK_REG0 + tmpGrp * 4) &=
								~(1 << tmpSrc);

	return 0;
}

/*
********************************************************************************
*                                       QUERY INTERRUPT
*
* Description: query interrupt.
*
* Arguments  : src  interrupt source number.
*
* Returns    : 0/-1;
********************************************************************************
*/
__s32 mem_query_int(enum interrupt_source_e src)
{
	__s32 result = 0;
	__u32 tmpGrp = (__u32) src >> 5;
	__u32 tmpSrc = (__u32) src & 0x1f;

	if (0 == src)
		return -1;

	result = *(volatile __u32 *)(int_reg_base + SW_INT_PENDING_REG0 +
			tmpGrp * 4) & (1 << tmpSrc);

	/*printk("tmpSrc = 0x%x. result = 0x%x.\n", tmpSrc, result); */

	return result ? 0 : -1;
}
