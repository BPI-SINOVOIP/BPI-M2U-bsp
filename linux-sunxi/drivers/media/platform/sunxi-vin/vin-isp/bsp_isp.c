
/*
 ******************************************************************************
 *
 * bsp_isp.c
 *
 * Hawkview ISP - bsp_isp.c module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/11/07	    First Version
 *
 ******************************************************************************
 */

#include <linux/string.h>
#include <linux/kernel.h>

#include "bsp_isp.h"
#include "isp_platform_drv.h"
#include "bsp_isp_comm.h"

static int isp_platform_id;
struct isp_bsp_fun_array *fun_array_curr;

void bsp_isp_enable(unsigned long id)
{
	fun_array_curr->isp_enable(id, 1);
}

void bsp_isp_disable(unsigned long id)
{
	fun_array_curr->isp_enable(id, 0);
}

void bsp_isp_channel_enable(unsigned long id, enum isp_channel ch)
{
	fun_array_curr->isp_ch_enable(id, ch, 1);
}

void bsp_isp_channel_disable(unsigned long id, enum isp_channel ch)
{
	fun_array_curr->isp_ch_enable(id, ch, 0);
}

void bsp_isp_capture_start(unsigned long id)
{
	fun_array_curr->isp_capture_start(id);
}

void bsp_isp_capture_stop(unsigned long id)
{
	fun_array_curr->isp_capture_stop(id);
}

unsigned int bsp_isp_get_para_ready(unsigned long id)
{
	return fun_array_curr->isp_get_para_ready(id);
}

void bsp_isp_set_para_ready(unsigned long id)
{
	fun_array_curr->isp_set_para_ready(id, PARA_READY);
}
void bsp_isp_clr_para_ready(unsigned long id)
{
	fun_array_curr->isp_set_para_ready(id, PARA_NOT_READY);
}

void bsp_isp_irq_enable(unsigned long id, unsigned int irq_flag)
{
	fun_array_curr->isp_irq_enable(id, irq_flag);
}
void bsp_isp_irq_disable(unsigned long id, unsigned int irq_flag)
{
	fun_array_curr->isp_irq_disable(id, irq_flag);
}

unsigned int bsp_isp_get_irq_status(unsigned long id, unsigned int irq)
{
	return fun_array_curr->isp_get_irq_status(id, irq);
}


void bsp_isp_clr_irq_status(unsigned long id, unsigned int irq)
{
	fun_array_curr->isp_clr_irq_status(id, irq);
}

int bsp_isp_int_get_enable(unsigned long id)
{
	return fun_array_curr->isp_int_get_enable(id);
}


void bsp_isp_set_statistics_addr(unsigned long id, dma_addr_t addr)
{
	fun_array_curr->isp_set_statistics_addr(id, addr);
}

void bsp_isp_set_base_addr(unsigned long id, unsigned long vaddr)
{
	fun_array_curr->map_reg_addr(id, vaddr);
}

void bsp_isp_set_dma_load_addr(unsigned long id, unsigned long dma_addr)
{
	fun_array_curr->isp_set_load_addr(id, dma_addr);
}

void bsp_isp_set_dma_saved_addr(unsigned long id, unsigned long dma_addr)
{
	fun_array_curr->isp_set_saved_addr(id, dma_addr);
}

void bsp_isp_set_map_load_addr(unsigned long id, unsigned long vaddr)
{
	fun_array_curr->map_load_dram_addr(id, vaddr);
}

void bsp_isp_set_map_saved_addr(unsigned long id, unsigned long vaddr)
{
	fun_array_curr->map_saved_dram_addr(id, vaddr);
}

void bsp_isp_update_lens_gamma_table(unsigned long id, struct isp_table_addr *tbl_addr)
{
	fun_array_curr->isp_set_table_addr(id, LENS_GAMMA_TABLE,
		(unsigned long)(tbl_addr->isp_lsc_tbl_dma_addr));
}

void bsp_isp_update_drc_table(unsigned long id, struct isp_table_addr *tbl_addr)
{
	fun_array_curr->isp_set_table_addr(id, DRC_TABLE,
		(unsigned long)(tbl_addr->isp_drc_tbl_dma_addr));
}

void bsp_isp_update_table(unsigned long id, unsigned short flag)
{
	fun_array_curr->isp_update_table(id, flag);
}

void bsp_isp_set_speed_mode(unsigned long id, unsigned int speed_mode)
{
	fun_array_curr->isp_set_speed_mode(id, speed_mode);
}

void bsp_isp_src0_enable(unsigned long id)
{
	fun_array_curr->isp_src0_en(id, 1);
}

void bsp_isp_src0_disable(unsigned long id)
{
	fun_array_curr->isp_src0_en(id, 0);
}
void bsp_isp_set_input_fmt(unsigned long id, enum isp_input_seq fmt)
{
	fun_array_curr->isp_set_input_fmt(id, (unsigned int)fmt);
}

void bsp_isp_set_ob_zone(unsigned long id, struct isp_size_settings *ss)
{
	fun_array_curr->isp_set_size(id, &ss->ob_black, &ss->ob_valid, &ss->ob_start);
}

void bsp_isp_print_reg_saved(unsigned long id)
{
	fun_array_curr->isp_print_reg_saved(id);
}

void bsp_isp_init_platform(unsigned int platform_id)
{
	struct isp_platform_drv *isp_platform;


	isp_platform_id = platform_id;
	isp_platform_init(platform_id);
	isp_platform = isp_get_driver();
	fun_array_curr = isp_platform->fun_array;
}
