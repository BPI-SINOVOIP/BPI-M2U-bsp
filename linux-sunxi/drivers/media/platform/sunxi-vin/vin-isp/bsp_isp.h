
/*
 *****************************************************************************
 *
 * bsp_isp.h
 *
 * Hawkview ISP - bsp_isp.h module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/12/25	    First Version
 *
 ******************************************************************************
 */
#ifndef __BSP__ISP__H
#define __BSP__ISP__H

#include "../utility/bsp_common.h"
#include "bsp_isp_comm.h"

struct isp_table_addr {
	void *isp_lsc_tbl_vaddr;
	void *isp_lsc_tbl_paddr;
	void *isp_lsc_tbl_dma_addr;
	void *isp_gamma_tbl_vaddr;
	void *isp_gamma_tbl_paddr;
	void *isp_gamma_tbl_dma_addr;
	void *isp_linear_tbl_vaddr;
	void *isp_linear_tbl_paddr;
	void *isp_linear_tbl_dma_addr;

	void *isp_drc_tbl_vaddr;
	void *isp_drc_tbl_paddr;
	void *isp_drc_tbl_dma_addr;
	void *isp_saturation_tbl_vaddr;
	void *isp_saturation_tbl_paddr;
	void *isp_saturation_tbl_dma_addr;
};

void bsp_isp_enable(unsigned long id);
void bsp_isp_disable(unsigned long id);

void bsp_isp_channel_enable(unsigned long id, enum isp_channel ch);
void bsp_isp_channel_disable(unsigned long id, enum isp_channel ch);

void bsp_isp_capture_start(unsigned long id);
void bsp_isp_capture_stop(unsigned long id);
unsigned int bsp_isp_get_para_ready(unsigned long id);
void bsp_isp_set_para_ready(unsigned long id);
void bsp_isp_clr_para_ready(unsigned long id);

void bsp_isp_irq_enable(unsigned long id, unsigned int irq_flag);
void bsp_isp_irq_disable(unsigned long id, unsigned int irq_flag);
unsigned int bsp_isp_get_irq_status(unsigned long id, unsigned int irq);
int bsp_isp_int_get_enable(unsigned long id);

void bsp_isp_clr_irq_status(unsigned long id, unsigned int irq);
void bsp_isp_set_statistics_addr(unsigned long id, dma_addr_t addr);

void bsp_isp_set_speed_mode(unsigned long id, unsigned int speed_mode);

void bsp_isp_src0_enable(unsigned long id);
void bsp_isp_src0_disable(unsigned long id);
void bsp_isp_set_input_fmt(unsigned long id, enum isp_input_seq fmt);
void bsp_isp_set_ob_zone(unsigned long id, struct isp_size_settings *ss);

void bsp_isp_set_base_addr(unsigned long id, unsigned long vaddr);
void bsp_isp_set_dma_load_addr(unsigned long id, unsigned long dma_addr);
void bsp_isp_set_dma_saved_addr(unsigned long id, unsigned long dma_addr);
void bsp_isp_set_map_load_addr(unsigned long id, unsigned long vaddr);
void bsp_isp_set_map_saved_addr(unsigned long id, unsigned long vaddr);
void bsp_isp_update_lens_gamma_table(unsigned long id, struct isp_table_addr *tbl_addr);
void bsp_isp_update_drc_table(unsigned long id, struct isp_table_addr *tbl_addr);
void bsp_isp_update_table(unsigned long id, unsigned short flag);

void bsp_isp_init_platform(unsigned int platform_id);

#endif
