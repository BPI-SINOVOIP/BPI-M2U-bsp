/*
 * combo rx module
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "combo_rx_reg_i.h"
#include "combo_rx_reg.h"
#include "../../utility/vin_io.h"

volatile void *cmb_rx_base_addr[MAX_MAP_NUM_PHY];

int cmb_rx_set_base_addr(unsigned int sel, unsigned long addr)
{
	return 0;
}

void cmb_rx_enable(unsigned int sel)
{
}

void cmb_rx_disable(unsigned int sel)
{
}

void cmb_rx_mode_sel(unsigned int sel, enum combo_rx_mode_sel mode)
{
}

void cmb_rx_app_pixel_out(unsigned int sel, enum combo_rx_pix_num pix_num)
{
}

void cmb_rx_phya_a_d0_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_b_d0_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_c_d0_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_a_d1_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_b_d1_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_c_d1_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_a_d2_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_b_d2_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_c_d2_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_a_d3_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_b_d3_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_c_d3_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_a_ck_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_b_ck_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_c_ck_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_config(unsigned int sel)
{
}

void cmb_rx_phya_ck_mode(unsigned int sel, unsigned int mode)
{
}

void cmb_rx_phya_ck_pol(unsigned int sel, unsigned int pol)
{
}

void cmb_rx_phya_offset(unsigned int sel, unsigned int offset)
{
}

void cmb_rx_phya_singal_dly_en(unsigned int sel, unsigned int en)
{
}

void cmb_rx_phya_singal_dly_ctr(unsigned int sel, struct phya_signal_dly_ctr *phya_signal_dly)
{
}

void cmb_rx_mipi_ctr(unsigned int sel, struct mipi_ctr *mipi_ctr)
{
}


void cmb_rx_mipi_dphy_mapping(unsigned int sel, struct mipi_lane_map *mipi_map)
{
}

void cmb_rx_mipi_csi2_status(unsigned int sel, unsigned int *mipi_csi2_status)
{
}

void cmb_rx_mipi_csi2_data_id(unsigned int sel, unsigned int *mipi_csi2_data_id)
{
}

void cmb_rx_mipi_csi2_word_cnt(unsigned int sel, unsigned int *mipi_csi2_word_cnt)
{
}

void cmb_rx_mipi_csi2_ecc_val(unsigned int sel, unsigned int *mipi_csi2_ecc_val)
{
}

void cmb_rx_mipi_csi2_line_lentgh(unsigned int sel, unsigned int *mipi_csi2_line_lentgh)
{
}

void cmb_rx_mipi_csi2_rcv_cnt(unsigned int sel, unsigned int *mipi_csi2_rcv_cnt)
{
}

void cmb_rx_mipi_csi2_ecc_err_cnt(unsigned int sel, unsigned int *mipi_csi2_ecc_err_cnt)
{
}

void cmb_rx_mipi_csi2_check_sum_err_cnt(unsigned int sel, unsigned int *mipi_csi2_check_sum_err_cnt)
{
}

void cmb_rx_lvds_ctr(unsigned int sel, struct lvds_ctr *lvds_ctr)
{
}

void cmb_rx_hispi_ctr(unsigned int sel, struct hispi_ctr *hispi_ctr)
{
}

void cmb_rx_lvds_mapping(unsigned int sel, struct combo_lane_map *lvds_map)
{
}


void cmb_rx_lvds_sync_code(unsigned int sel, struct combo_sync_code *lvds_sync_code)
{
}
