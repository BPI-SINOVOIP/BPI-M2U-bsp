/*
 * di_ebios.c DE-Interlace driver
 *
 * Copyright (C) 2017-2020 allwinner.
 * xlf<xielinfei@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include "di_ebios_sun3i.h"
#include <linux/slab.h>
#include <asm/io.h>

extern volatile __di_dev_t *di_dev;

__u32 DI_VAtoPA(__u32 va)
{
	if ((va) > 0x40000000)
		return (va) - 0x40000000;
	return va;
}

/* should initial some registers for M2M de-interlace used */
__s32 DI_Init(void)
{
	DI_Set_Di_Ctrl(1, 3, 1, 1);

	di_dev->para.dwval = 0x050a0609; /* default parameters */
	di_dev->intr.dwval = 0;	/* disable interrupt */

	/* reset di module */
	di_dev->ctrl.bits.reset = 1;
	while (di_dev->ctrl.bits.reset)
		;

	/* enable de-interlacing finish interrupt */
	di_dev->intr.bits.int_en = 1;

	return 0;
}

__s32 DI_Config_Src(__di_buf_addr_t *addr, __di_src_size_t *size,
		__di_src_type_t *type)
{
	__u32 src_width_lmt, src_height_lmt;

	src_width_lmt = size->src_width & 0xfffffffe;
	src_height_lmt = size->src_height & 0xfffffffc;
	di_dev->size.bits.width = src_width_lmt - 1;
	di_dev->size.bits.height = src_height_lmt - 1;

	di_dev->fmt.bits.fmt = ((type->mod == DI_UVCOMBINED) ? 0 : 1);
	/* default input ps equal to output ps */
	di_dev->fmt.bits.ps_rev = 0;

	if (type->mod == DI_UVCOMBINED) {
		/* (word) 4 byte */
		di_dev->inls0.bits.ls = (size->src_width+0x3)>>2;
		di_dev->inls1.bits.ls = ((size->src_width>>1)+0x1)>>1;
	} else if (type->mod == DI_UVCOMBINEDMB) {
		di_dev->inls0.bits.ls = ((((size->src_width+0x1f)>>5)
					<<5)-0x1f)<<0x3; /* 32 byte */
		di_dev->inls1.bits.ls = (((((size->src_width>>1)+0xf)>>4)<<5)
				-0x1f)<<0x3; /* 32 byte */
	}

	/* set output linestirde */
	di_dev->outls0.bits.ls = (size->src_width+0x3)>>2;
	di_dev->outls1.bits.ls = ((size->src_width>>1)+0x1)>>1;

	di_dev->curadd0.dwval = addr->ch0_addr;
	di_dev->curadd1.dwval = addr->ch1_addr;
	return 0;
}

__s32 DI_Set_Scaling_Factor(__di_src_size_t *in_size, __di_out_size_t *out_size)
{

	return 0;
}

__s32 DI_Set_Scaling_Coef(__di_src_size_t *in_size, __di_out_size_t *out_size,
		__di_src_type_t *in_type,  __di_out_type_t *out_type)
{

	return 0;
}

__s32 DI_Set_Out_Format(__di_out_type_t *out_type)
{

	return 0;
}

__s32 DI_Set_Out_Size(__di_out_size_t *out_size)
{

	return 0;
}

__s32 DI_Set_Writeback_Addr(__di_buf_addr_t *addr)
{
	di_dev->outadd0.dwval = addr->ch0_addr;
	di_dev->outadd1.dwval = addr->ch1_addr;

	return 0;
}

__s32 DI_Set_Writeback_Addr_ex(__di_buf_addr_t *addr,
		__di_out_size_t *size, __di_out_type_t *type)
{
	DI_Set_Writeback_Addr(addr);

	return 0;
}

__s32 DI_Set_Di_Ctrl(__u8 en, __u8 mode, __u8 diagintp_en, __u8 tempdiff_en)
{

	return 0;
}

__s32 DI_Set_Di_PreFrame_Addr(__u32 luma_addr, __u32 chroma_addr)
{
	di_dev->preadd0.dwval = luma_addr;
	di_dev->preadd1.dwval = chroma_addr;

	return 0;
}

__s32 DI_Set_Di_MafFlag_Src(__u32 cur_addr, __u32 pre_addr, __u32 stride)
{
	di_dev->flagadd.dwval = cur_addr;
	di_dev->flagls.bits.ls = MAX_SIZE_IN_HORIZONAL/PIXELS_PER_WORLD;

	return 0;
}

__s32 DI_Set_Di_Field(u32 field)
{

	return 0;
}
__s32 DI_Set_Reg_Rdy(void)
{

	return 0;
}

__s32 DI_Enable(void)
{

	return 0;
}

__s32 DI_Module_Enable(void)
{

	return 0;
}

__s32 DI_Set_Reset(void)
{
	/* reset di module */
	di_dev->ctrl.bits.reset = 1;
	while (di_dev->ctrl.bits.reset)
		;

	return 0;
}

__s32 DI_Set_Irq_Enable(__u32 enable)
{
	/* enable de-interlacing finish interrupt */
	di_dev->intr.bits.int_en = (enable & 0x1);

	return 0;
}

__s32 DI_Clear_irq(void)
{
	di_dev->status.bits.finish_sts = 0x1;

	return 0;
}

__s32 DI_Get_Irq_Status(void)
{
	__u32 wb_finish;
	__u32 wb_processing;

	wb_finish = di_dev->status.bits.finish_sts;
	wb_processing = di_dev->status.bits.busy;

	if (wb_processing)
		return 2;
	else if (wb_finish == 0 && wb_processing == 0)
		return 1;
	else if (wb_finish)
		return 0;
	else
		return 3;
}

__s32 DI_Set_Writeback_Start(void)
{
	/* start one frame de-interlace,self clear when finished */
	di_dev->ctrl.bits.start = 1;

	return 0;
}

__s32 DI_Internal_Set_Clk(__u32 enable)
{
	return 0;
}
