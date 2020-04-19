/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "ebios_lcdc_tve.h"
#include "de_lcd.h"

static volatile struct __de_lcd_dev_t *lcd_dev[2];

__s32 hmdi_src_sel(__u32 sel)
{
	lcd_dev[0]->tcon_mul_ctl.bits.hdmi_src = sel;
	return 0;
}

__s32 dsi_src_sel(__u32 sel)
{
	lcd_dev[0]->tcon_mul_ctl.bits.dsi_src = sel;
	return 0;
}

__s32 lvds_open(__u32 sel, disp_panel_para *panel)
{
	lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_en = 1;
	if (panel->lcd_lvds_if == LCD_LVDS_IF_DUAL_LINK) {
		lcd_dev[sel]->tcon0_lvds_ana[0].bits.c = 2;
		lcd_dev[sel]->tcon0_lvds_ana[0].bits.v = 3;
		lcd_dev[sel]->tcon0_lvds_ana[0].bits.pd = 2;
		lcd_dev[sel]->tcon0_lvds_ana[1].bits.c = 2;
		lcd_dev[sel]->tcon0_lvds_ana[1].bits.v = 3;
		lcd_dev[sel]->tcon0_lvds_ana[1].bits.pd = 2;

		lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_ldo = 1;
		lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_ldo = 1;
		bsp_disp_lcd_delay_us(5);	/* 1200ns */
		lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_mb = 1;
		lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_mb = 1;
		bsp_disp_lcd_delay_us(5);	/* 1200ns */
		lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_drvc = 1;
		lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_drvc = 1;
		if (panel->lcd_lvds_colordepth == LCD_LVDS_6bit) {
			lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_drvd = 0x7;
			lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_drvd = 0x7;
		} else {
			lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_drvd = 0xf;
			lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_drvd = 0xf;
		}
	} else {
		lcd_dev[sel]->tcon0_lvds_ana[sel].bits.c = 2;
		lcd_dev[sel]->tcon0_lvds_ana[sel].bits.v = 3;
		lcd_dev[sel]->tcon0_lvds_ana[sel].bits.pd = 2;

		lcd_dev[sel]->tcon0_lvds_ana[sel].bits.en_ldo = 1;
		bsp_disp_lcd_delay_us(5);	/* 1200ns */
		lcd_dev[sel]->tcon0_lvds_ana[sel].bits.en_mb = 1;
		bsp_disp_lcd_delay_us(5);	/* 1200ns */
		lcd_dev[sel]->tcon0_lvds_ana[sel].bits.en_drvc = 1;
		if (panel->lcd_lvds_colordepth == LCD_LVDS_6bit)
			lcd_dev[sel]->tcon0_lvds_ana[sel].bits.en_drvd = 0x7;
		else
			lcd_dev[sel]->tcon0_lvds_ana[sel].bits.en_drvd = 0xf;

	}
	return 0;
}

__s32 lvds_close(__u32 sel)
{
	lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_drvd = 0;
	lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_drvd = 0;
	lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_drvc = 0;
	lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_drvc = 0;
	bsp_disp_lcd_delay_us(5);	/* 1200ns */
	lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_mb = 0;
	lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_mb = 0;
	bsp_disp_lcd_delay_us(5);	/* 1200ns */
	lcd_dev[sel]->tcon0_lvds_ana[0].bits.en_ldo = 0;
	lcd_dev[sel]->tcon0_lvds_ana[1].bits.en_ldo = 0;
	lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_en = 0;
	return 0;
}

__s32 tcon_get_timing(__u32 sel, __u32 index, disp_video_timing *tt)
{
	__u32 x, y, ht, hbp, vt, vbp, hspw, vspw;

	if (index == 0) {
		x = lcd_dev[sel]->tcon0_basic0.bits.x;
		y = lcd_dev[sel]->tcon0_basic0.bits.y;
		ht = lcd_dev[sel]->tcon0_basic1.bits.ht;
		hbp = lcd_dev[sel]->tcon0_basic1.bits.hbp;
		vt = lcd_dev[sel]->tcon0_basic2.bits.vt;
		vbp = lcd_dev[sel]->tcon0_basic2.bits.vbp;
		hspw = lcd_dev[sel]->tcon0_basic3.bits.hspw;
		vspw = lcd_dev[sel]->tcon0_basic3.bits.vspw;
	} else {
		x = lcd_dev[sel]->tcon1_basic0.bits.x;
		y = lcd_dev[sel]->tcon1_basic0.bits.y;
		ht = lcd_dev[sel]->tcon1_basic3.bits.ht;
		hbp = lcd_dev[sel]->tcon1_basic3.bits.hbp;
		vt = lcd_dev[sel]->tcon1_basic4.bits.vt;
		vbp = lcd_dev[sel]->tcon1_basic4.bits.vbp;
		hspw = lcd_dev[sel]->tcon1_basic5.bits.hspw;
		vspw = lcd_dev[sel]->tcon1_basic5.bits.vspw;
	}

	/* left_margin */
	tt->hor_back_porch = (hbp + 1) - (hspw + 1);
	/* right_margin */
	tt->hor_front_porch = (ht + 1) - (x + 1) - (hbp + 1);
	/* upper_margin */
	tt->ver_back_porch = (vbp + 1) - (vspw + 1);
	/* lower_margin */
	tt->ver_front_porch = (vt / 2) - (y + 1) - (vbp + 1);
	/* hsync_len */
	tt->hor_sync_time = (hspw + 1);
	/* vsync_len */
	tt->ver_sync_time = (vspw + 1);

	return 0;
}

__s32 tcon_set_reg_base(__u32 sel, uintptr_t base)
{
	lcd_dev[sel] = (struct __de_lcd_dev_t *)base;
	return 0;
}

__u32 tcon_get_reg_base(__u32 sel)
{
	return (__u32) lcd_dev[sel];
}

__s32 tcon_init(__u32 sel)
{
	/* tcon0_close(sel); */
	lcd_dev[sel]->tcon0_ctl.bits.tcon0_en = 0;
	lcd_dev[sel]->tcon1_ctl.bits.tcon1_en = 0;
	lcd_dev[sel]->tcon0_dclk.bits.tcon0_dclk_en = 0xf;
	lcd_dev[sel]->tcon_gctl.bits.tcon_en = 0;
	lcd_dev[sel]->tcon_gint0.bits.tcon_irq_en = 0;
	lcd_dev[sel]->tcon_gint0.bits.tcon_irq_flag = 0;
	lcd_dev[sel]->tcon_gctl.bits.tcon_en = 1;
	return 0;
}

__s32 tcon_exit(__u32 sel)
{
	lcd_dev[sel]->tcon_gctl.bits.tcon_en = 0;
	lcd_dev[sel]->tcon0_dclk.bits.tcon0_dclk_en = 0;
	return 0;
}

__s32 tcon_irq_enable(__u32 sel, enum __lcd_irq_id_t id)
{
	lcd_dev[sel]->tcon_gint0.bits.tcon_irq_en |= (1 << id);
	return 0;
}

__s32 tcon_irq_disable(__u32 sel, enum __lcd_irq_id_t id)
{
	lcd_dev[sel]->tcon_gint0.bits.tcon_irq_en &= ~(1 << id);
	return 0;
}

__u32 tcon_irq_query(__u32 sel, enum __lcd_irq_id_t id)
{
	__u32 en, fl;

	en = lcd_dev[sel]->tcon_gint0.bits.tcon_irq_en;
	fl = lcd_dev[sel]->tcon_gint0.bits.tcon_irq_flag;
	if (en & fl & (((__u32) 1) << id)) {
		lcd_dev[sel]->tcon_gint0.bits.tcon_irq_flag &=
		    ~(((__u32) 1) << id);
		return 1;
	} else {
		return 0;
	}
}

/*__s32 tcon_set_start_delay(__u32 sel, __u32 tcon_index, __u8 delay)*/
/*{*/
/*    if(tcon_index == 0) {*/
/*		lcd_dev[sel]->tcon0_ctl.bits.start_delay = delay;*/
/*	} else if(tcon_index == 1) {*/
/*		lcd_dev[sel]->tcon1_ctl.bits.start_delay = delay;*/
/*	}*/
/*    return 0;*/
/*}*/

__u32 tcon_get_start_delay(__u32 sel, __u32 tcon_index)
{
	if (tcon_index == 0)
		return lcd_dev[sel]->tcon0_ctl.bits.start_delay;
	else if (tcon_index == 1)
		return lcd_dev[sel]->tcon1_ctl.bits.start_delay;

	return 0;
}

__u32 tcon_get_cur_line(__u32 sel, __u32 tcon_index)
{
	if (tcon_index == 0)
		return lcd_dev[sel]->tcon_debug.bits.tcon0_current_line;
	else if (tcon_index == 1)
		return lcd_dev[sel]->tcon_debug.bits.tcon1_current_line;

	return 0;
}

__s32 tcon0_src_select(__u32 sel, enum __lcd_src_t src)
{
	lcd_dev[sel]->tcon0_ctl.bits.src_sel = src;
	return 0;
}

__s32 tcon0_src_get(__u32 sel)
{
	return lcd_dev[sel]->tcon0_ctl.bits.src_sel;
}

__s32 tcon0_open(__u32 sel, disp_panel_para *panel)
{
	lcd_dev[sel]->tcon_gint0.bits.tcon_irq_flag = 0;
	if ((panel->lcd_if == LCD_IF_HV) ||
	    (panel->lcd_if == LCD_IF_LVDS) ||
	    (panel->lcd_if == LCD_IF_EDP) ||
	    (panel->lcd_if == LCD_IF_EXT_DSI)) {
		lcd_dev[sel]->tcon0_ctl.bits.tcon0_en = 1;
		tcon_irq_enable(sel, LCD_IRQ_TCON0_VBLK);
	} else if (panel->lcd_if == LCD_IF_CPU ||
		   (panel->lcd_if == LCD_IF_DSI &&
		    panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE)) {
		lcd_dev[sel]->tcon0_ctl.bits.tcon0_en = 1;
		tcon_irq_enable(sel, LCD_IRQ_TCON0_CNTR);
		/* tcon_irq_enable(sel,LCD_IRQ_TCON0_TRIF); */
	} else if (panel->lcd_if == LCD_IF_DSI) {
		lcd_dev[sel]->tcon0_ctl.bits.tcon0_en = 1;
		/* tcon_irq_enable(sel,LCD_IRQ_TCON0_TRIF); */
	}
	return 0;
}

__s32 tcon0_close(__u32 sel)
{
	tcon_irq_disable(sel, LCD_IRQ_TCON0_CNTR);
	tcon_irq_disable(sel, LCD_IRQ_TCON0_VBLK);
	tcon_irq_disable(sel, LCD_IRQ_TCON0_TRIF);
	lcd_dev[sel]->tcon0_ctl.bits.tcon0_en = 0;
	/* while(lcd_dev[sel]->tcon0_cpu_ctl.bits.trigger_start); */
	bsp_disp_lcd_delay_ms(30);
	return 1;
}

static __s32 tcon0_cfg_mode_auto(__u32 sel, disp_panel_para *panel)
{
	__s32 start_delay;

	lcd_dev[sel]->tcon0_basic0.bits.x = panel->lcd_x - 1;
	lcd_dev[sel]->tcon0_basic0.bits.y = panel->lcd_y - 1;
	lcd_dev[sel]->tcon0_basic1.bits.ht = panel->lcd_ht - 1;
	lcd_dev[sel]->tcon0_basic1.bits.hbp =
	    (panel->lcd_hbp == 0) ? 0 : panel->lcd_hbp - 1;
	lcd_dev[sel]->tcon0_basic2.bits.vt = panel->lcd_vt * 2;
	lcd_dev[sel]->tcon0_basic2.bits.vbp =
	    (panel->lcd_vbp == 0) ? 0 : panel->lcd_vbp - 1;
	lcd_dev[sel]->tcon0_basic3.bits.hspw =
	    (panel->lcd_hspw == 0) ? 0 : panel->lcd_hspw - 1;
	lcd_dev[sel]->tcon0_basic3.bits.vspw =
	    (panel->lcd_vspw == 0) ? 0 : panel->lcd_vspw - 1;
	start_delay = panel->lcd_vt - panel->lcd_y - 10;
	if (panel->lcd_hv_if == LCD_HV_IF_CCIR656_2CYC) {
		lcd_dev[sel]->tcon0_basic0.bits.y = panel->lcd_y / 2 - 1;
		lcd_dev[sel]->tcon0_basic2.bits.vt =
		    (panel->lcd_hv_syuv_fdly ==
		     LCD_HV_SRGB_FDLY_2LINE) ? 525 : 625;
		lcd_dev[sel]->tcon0_basic1.bits.ht =
		    (panel->lcd_ht == 0) ? 0 : (panel->lcd_ht * 2 - 1);
		lcd_dev[sel]->tcon0_basic1.bits.hbp =
		    (panel->lcd_hbp == 0) ? 0 : (panel->lcd_hbp * 2 - 1);
		lcd_dev[sel]->tcon0_basic3.bits.hspw =
		    (panel->lcd_hspw == 0) ? 0 : (panel->lcd_hspw * 2 - 1);
		start_delay = panel->lcd_vt / 2 - panel->lcd_y / 2 - 10;
	}
	if (start_delay < 10)
		start_delay = 10;
	else if (start_delay > 31)
		start_delay = 31;

	lcd_dev[sel]->tcon0_ctl.bits.start_delay = start_delay;
	return 0;
}

static __s32 tcon0_cfg_mode_tri(__u32 sel, disp_panel_para *panel)
{
	u32 start_delay = 0;

	lcd_dev[sel]->tcon0_basic0.bits.x = panel->lcd_x - 1;
	lcd_dev[sel]->tcon0_basic0.bits.y = panel->lcd_y - 1;
	lcd_dev[sel]->tcon0_cpu_tri0.bits.block_size = panel->lcd_x - 1;
	lcd_dev[sel]->tcon0_cpu_tri1.bits.block_num = panel->lcd_y - 1;
	lcd_dev[sel]->tcon0_cpu_tri2.bits.trans_start_mode = 0;
	lcd_dev[sel]->tcon0_cpu_tri2.bits.sync_mode = 0;
	start_delay = (panel->lcd_vt - panel->lcd_y - 10 - 1) *
	    panel->lcd_ht * 149 / panel->lcd_dclk_freq / 8;
	lcd_dev[sel]->tcon0_cpu_tri2.bits.start_delay = start_delay;

	/*if(panel->lcd_if==LCD_IF_DSI && */
	/*panel->lcd_dsi_if==LCD_DSI_IF_VIDEO_MODE) { */
	/*      __u32 tmp; */
	/*      tmp = panel->lcd_vspw+panel->lcd_vbp; */
	/*      if(tmp>23) { */
	/*              tmp -= 20; */
	/*              tmp *= panel->lcd_x; */
	/*              tmp /= 8; */
	/*      } else { */
	/*              tmp = 3; */
	/*      } */
	/*      lcd_dev[sel]->tcon0_cpu_tri2.bits.start_delay = tmp; */
	/*} */
	/*else { */
	/*      lcd_dev[sel]->tcon0_cpu_tri2.bits.start_delay = 3; */
	/*} */

	lcd_dev[sel]->tcon0_cpu_ctl.bits.trigger_fifo_en = 1;
	lcd_dev[sel]->tcon0_cpu_ctl.bits.trigger_en = 1;
	lcd_dev[sel]->tcon0_cpu_ctl.bits.flush = 1;
	lcd_dev[sel]->tcon0_ctl.bits.tcon0_en = 1;
	lcd_dev[sel]->tcon_gctl.bits.tcon_en = 1;

	/*if(panel->lcd_if==LCD_IF_CPU) { */
	/*      lcd_dev[sel]->tcon0_cpu_tri0.bits.block_space = 30; */
	/*} */
	/*else if((panel->lcd_if==LCD_IF_DSI) && */
	/*(panel->lcd_dsi_if==LCD_DSI_IF_COMMAND_MODE)) { */
	/*      lcd_dev[sel]->tcon0_cpu_tri0.bits.block_space = 200; */
	/*} */

	if ((panel->lcd_if == LCD_IF_CPU) ||
	    (panel->lcd_if == LCD_IF_DSI &&
	     panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE)) {
		lcd_dev[sel]->tcon0_cpu_tri0.bits.block_space =
		    panel->lcd_ht - panel->lcd_x - 1;
		lcd_dev[sel]->tcon0_cpu_tri2.bits.trans_start_set =
		    panel->lcd_x - 1;
	} else if ((panel->lcd_if == LCD_IF_DSI) &&
		   (panel->lcd_dsi_if == LCD_DSI_IF_VIDEO_MODE ||
		    panel->lcd_dsi_if == LCD_DSI_IF_BURST_MODE)) {
		lcd_dev[sel]->tcon0_cpu_tri0.bits.block_space =
		    panel->lcd_ht * dsi_pixel_bits[panel->lcd_dsi_format] /
		    (tcon_div * panel->lcd_dsi_lane) - panel->lcd_x - 40;
		lcd_dev[sel]->tcon0_cpu_tri2.bits.trans_start_set = 10;
	}
	if (panel->lcd_fresh_mode == 1) {
		__u32 lcd_te;

		lcd_te = (panel->lcd_if == LCD_IF_CPU) ?
		    panel->lcd_cpu_te : panel->lcd_dsi_te;
		if (lcd_te == 2) {	/* falling mode */
			lcd_dev[sel]->tcon0_cpu_tri3.bits.tri_int_mode = 3;
		} else if (lcd_te == 1) {	/* rising mode */
			lcd_dev[sel]->tcon0_cpu_tri3.bits.tri_int_mode = 2;
			goto exit;
		} else {
			__u32 cntr_set = panel->lcd_ht * panel->lcd_vt / 4;
			__u32 cntr_n, cntr_m;

			for (cntr_m = 1; cntr_m < 256; cntr_m++) {
				if ((cntr_set / cntr_m) < 65535) {
					cntr_n = cntr_set / cntr_m;
					lcd_dev[sel]->tcon0_cpu_tri3.bits.
					    counter_m = cntr_m - 1;
					lcd_dev[sel]->tcon0_cpu_tri3.bits.
					    counter_n = cntr_n - 1;
					lcd_dev[sel]->tcon0_cpu_tri3.bits.
					    tri_int_mode = 1;
					return 0;
				}
			}
			lcd_dev[sel]->tcon0_cpu_tri3.bits.tri_int_mode = 0;
			return -1;
		}
	}
exit:
	return 0;
}

__s32 tcon0_cfg(__u32 sel, disp_panel_para *panel)
{
	if ((panel->lcd_if == LCD_IF_HV) ||
	    (panel->lcd_if == LCD_IF_EDP) ||
	    (panel->lcd_if == LCD_IF_EXT_DSI)) {
		lcd_dev[sel]->tcon0_ctl.bits.tcon0_if = 0;
		lcd_dev[sel]->tcon0_hv_ctl.bits.hv_mode = panel->lcd_hv_if;
		lcd_dev[sel]->tcon0_hv_ctl.bits.srgb_seq =
		    panel->lcd_hv_srgb_seq;
		lcd_dev[sel]->tcon0_hv_ctl.bits.syuv_seq =
		    panel->lcd_hv_syuv_seq;
		lcd_dev[sel]->tcon0_hv_ctl.bits.syuv_fdly =
		    panel->lcd_hv_syuv_fdly;
		panel->lcd_fresh_mode = 0;
		tcon0_cfg_mode_auto(sel, panel);
	} else if (panel->lcd_if == LCD_IF_LVDS) {
		lcd_dev[sel]->tcon0_ctl.bits.tcon0_if = 0;
		lcd_dev[sel]->tcon0_hv_ctl.bits.hv_mode = 0;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_link =
		    panel->lcd_lvds_if;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_bitwidth =
		    panel->lcd_lvds_colordepth;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_mode =
		    panel->lcd_lvds_mode;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_debug_en = 0;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_correct_mode = 0;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_dir = 0;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_clk_sel = 1;
#if defined(CONFIG_FPGA_V4_PLATFORM)
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_data_revert = 0xf;
		lcd_dev[sel]->tcon0_lvds_ctl.bits.tcon0_lvds_clk_revert = 0x1;
#endif
		panel->lcd_fresh_mode = 0;
		tcon0_cfg_mode_auto(sel, panel);
	} else if (panel->lcd_if == LCD_IF_CPU) {
		lcd_dev[sel]->tcon0_ctl.bits.tcon0_if = 1;
		lcd_dev[sel]->tcon0_cpu_ctl.bits.cpu_mode = panel->lcd_cpu_if;
		lcd_dev[sel]->tcon0_cpu_ctl.bits.da = 1;
		lcd_dev[sel]->tcon_ecfifo_ctl.bits.ecc_fifo_setting = (1 << 3);
		panel->lcd_fresh_mode = 1;
		tcon0_cfg_mode_tri(sel, panel);
	} else if (panel->lcd_if == LCD_IF_DSI) {
		lcd_dev[sel]->tcon0_ctl.bits.tcon0_if = 1;
		lcd_dev[sel]->tcon0_cpu_ctl.bits.cpu_mode = 0x1;
		lcd_dev[sel]->tcon_ecfifo_ctl.bits.ecc_fifo_setting = (1 << 3);
		panel->lcd_fresh_mode =
		    (panel->lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE) ? 1 : 0;
		tcon0_cfg_mode_tri(sel, panel);
	}

	tcon0_frm(sel, panel->lcd_frm);
	tcon_gamma(sel, panel->lcd_gamma_en,
		   panel->lcd_extend_para.lcd_gamma_tbl);
	tcon_cmap(sel, panel->lcd_cmap_en, panel->lcd_extend_para.lcd_cmap_tbl);

	lcd_dev[sel]->tcon0_ctl.bits.rb_swap = panel->lcd_rb_swap;
	lcd_dev[sel]->tcon0_io_tri.bits.rgb_endian = panel->lcd_rgb_endian;
	lcd_dev[sel]->tcon_volume_ctl.bits.safe_period_mode = 3;
	/* panel->lcd_dclk_freq*15; */
	lcd_dev[sel]->tcon_volume_ctl.bits.safe_period_fifo_num = 3000;
	lcd_dev[sel]->tcon0_io_pol.bits.sync_inv = panel->lcd_hv_sync_polarity;
	switch (panel->lcd_hv_clk_phase) {
	case 0:
		lcd_dev[sel]->tcon0_io_pol.bits.clk_inv = 0;
		lcd_dev[sel]->tcon0_io_pol.bits.dclk_sel = 0;
		break;
	case 1:
		lcd_dev[sel]->tcon0_io_pol.bits.clk_inv = 0;
		lcd_dev[sel]->tcon0_io_pol.bits.dclk_sel = 2;
		break;
	case 2:
		lcd_dev[sel]->tcon0_io_pol.bits.clk_inv = 1;
		lcd_dev[sel]->tcon0_io_pol.bits.dclk_sel = 0;
		break;
	case 3:
		lcd_dev[sel]->tcon0_io_pol.bits.clk_inv = 1;
		lcd_dev[sel]->tcon0_io_pol.bits.dclk_sel = 2;
		break;
	default:
		lcd_dev[sel]->tcon0_io_pol.bits.clk_inv = 0;
		lcd_dev[sel]->tcon0_io_pol.bits.dclk_sel = 0;
		break;
	}

	if (panel->lcd_fresh_mode == 1) {
		__u32 lcd_te;

		lcd_te = (panel->lcd_if == LCD_IF_CPU) ?
		    panel->lcd_cpu_te : panel->lcd_dsi_te;
		lcd_dev[sel]->tcon0_io_tri.bits.io0_output_tri_en =
		    (lcd_te == 0) ? 0 : 1;
	} else {
		lcd_dev[sel]->tcon0_io_tri.bits.io0_output_tri_en = 0;
	}
	lcd_dev[sel]->tcon0_io_tri.bits.io1_output_tri_en = 0;
	lcd_dev[sel]->tcon0_io_tri.bits.io2_output_tri_en = 0;
	lcd_dev[sel]->tcon0_io_tri.bits.io3_output_tri_en = 0;
	lcd_dev[sel]->tcon0_io_tri.bits.data_output_tri_en = 0;
	return 0;
}

__s32 tcon0_tri_busy(__u32 sel)
{
	return lcd_dev[sel]->tcon0_cpu_ctl.bits.trigger_start;
}

__s32 tcon0_tri_start(__u32 sel)
{
	lcd_dev[sel]->tcon0_cpu_ctl.bits.trigger_start = 0;
	lcd_dev[sel]->tcon0_cpu_ctl.bits.trigger_start = 1;
	return 0;
}

__s32 tcon0_frm(__u32 sel, __u32 mode)
{
	if (mode == LCD_FRM_BYPASS) {
		lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_en = 0;
		return 0;
	}
	lcd_dev[sel]->tcon0_frm_seed_pr.bits.seed_value = 1;
	lcd_dev[sel]->tcon0_frm_seed_pg.bits.seed_value = 3;
	lcd_dev[sel]->tcon0_frm_seed_pb.bits.seed_value = 5;
	lcd_dev[sel]->tcon0_frm_seed_lr.bits.seed_value = 7;
	lcd_dev[sel]->tcon0_frm_seed_lg.bits.seed_value = 11;
	lcd_dev[sel]->tcon0_frm_seed_lb.bits.seed_value = 13;
	lcd_dev[sel]->tcon0_frm_tbl_0.bits.frm_table_value = 0x01010000;
	lcd_dev[sel]->tcon0_frm_tbl_1.bits.frm_table_value = 0x15151111;
	lcd_dev[sel]->tcon0_frm_tbl_2.bits.frm_table_value = 0x57575555;
	lcd_dev[sel]->tcon0_frm_tbl_3.bits.frm_table_value = 0x7f7f7777;
	if (mode == LCD_FRM_RGB666) {
		lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_mode_r = 0;
		lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_mode_g = 0;
		lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_mode_b = 0;
	} else if (mode == LCD_FRM_RGB565) {
		lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_mode_r = 1;
		lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_mode_g = 0;
		lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_mode_b = 1;
	}
	lcd_dev[sel]->tcon0_frm_ctl.bits.tcon0_frm_en = 1;
	return 0;
}

__u32 tcon0_cpu_16b_to_24b(__u32 value)
{
	return ((value & 0xfc00) << 8)
	    | ((value & 0x0300) << 6)
	    | ((value & 0x00e0) << 5)
	    | ((value & 0x001f) << 3);
}

__u32 tcon0_cpu_24b_to_16b(__u32 value)
{
	return ((value & 0xfc0000) >> 8)
	    | ((value & 0x00c000) >> 6)
	    | ((value & 0x001c00) >> 5)
	    | ((value & 0x0000f8) >> 3);
}

__u32 tcon0_cpu_busy(__u32 sel)
{
	if (lcd_dev[sel]->tcon0_cpu_ctl.bits.wr_flag
	    || lcd_dev[sel]->tcon0_cpu_ctl.bits.rd_flag)
		return 1;
	else
		return 0;
}

__s32 tcon0_cpu_wr_24b_index(__u32 sel, __u32 index)
{
	__u32 count = 0;

	while ((tcon0_cpu_busy(sel)) && (count < 50)) {
		count++;
		bsp_disp_lcd_delay_us(100);
	}
	lcd_dev[sel]->tcon0_cpu_ctl.bits.ca = 0;
	lcd_dev[sel]->tcon0_cpu_wr.bits.data_wr = index;
	return 0;
}

__s32 tcon0_cpu_wr_24b_data(__u32 sel, __u32 data)
{
	__u32 count = 0;

	while ((tcon0_cpu_busy(sel)) && (count < 50)) {
		count++;
		bsp_disp_lcd_delay_us(100);
	}
	lcd_dev[sel]->tcon0_cpu_ctl.bits.ca = 1;
	lcd_dev[sel]->tcon0_cpu_wr.bits.data_wr = data;
	return 0;
}

__s32 tcon0_cpu_wr_24b(__u32 sel, __u32 index, __u32 data)
{
	tcon0_cpu_wr_24b_index(sel, index);
	tcon0_cpu_wr_24b_data(sel, data);
	return 0;
}

__s32 tcon0_cpu_rd_24b(__u32 sel, __u32 index, __u32 *data)
{
	return -1;
}

__s32 tcon0_cpu_wr_16b(__u32 sel, __u32 index, __u32 data)
{
	tcon0_cpu_wr_24b(sel, tcon0_cpu_16b_to_24b(index),
			 tcon0_cpu_16b_to_24b(data));
	return 0;
}

__s32 tcon0_cpu_wr_16b_index(__u32 sel, __u32 index)
{
	tcon0_cpu_wr_24b_index(sel, tcon0_cpu_16b_to_24b(index));
	return 0;
}

__s32 tcon0_cpu_wr_16b_data(__u32 sel, __u32 data)
{
	tcon0_cpu_wr_24b_data(sel, tcon0_cpu_16b_to_24b(data));
	return 0;
}

__s32 tcon0_cpu_rd_16b(__u32 sel, __u32 index, __u32 *data)
{
	return -1;
}

__s32 tcon0_set_dclk_div(__u32 sel, __u8 div)
{
	lcd_dev[sel]->tcon0_dclk.bits.tcon0_dclk_div = div;

	return 0;
}

__u32 tcon0_get_dclk_div(__u32 sel)
{
	return lcd_dev[sel]->tcon0_dclk.bits.tcon0_dclk_div;
}

__s32 tcon1_open(__u32 sel)
{
	lcd_dev[sel]->tcon1_ctl.bits.tcon1_en = 1;
	tcon_irq_enable(sel, LCD_IRQ_TCON1_VBLK);
	return 0;
}

__s32 tcon1_close(__u32 sel)
{
	lcd_dev[sel]->tcon1_ctl.bits.tcon1_en = 0;
	tcon_irq_disable(sel, LCD_IRQ_TCON0_VBLK);
	return 0;
}

static struct __disp_timing_t tv_timing_tbl[30] = {
	/* clk,        x,       y,       ht,    hfp,   hsa,  hbp, */
	/*vt,  vfp,  vsa,  vbp,   hsp,  vsp,  int, act_space, trd */
	{13500000, 720, 480, 858, 19, 62, 57, 525, 4,
	 3, 15, 0, 0, 1, 0, 0},	/* DISP_TV_MOD_480I */
	{13500000, 720, 576, 864, 12, 63, 69, 625, 2,
	 3, 19, 0, 0, 1, 0, 0},	/* DISP_TV_MOD_576I */
	{27000000, 720, 480, 858, 16, 62, 60, 525, 9,
	 6, 30, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_480P */
	{27000000, 720, 576, 864, 12, 64, 68, 625, 5,
	 5, 39, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_576P */
	{74250000, 1280, 720, 1980, 440, 40, 220, 750, 5,
	 5, 20, 1, 1, 0, 0, 0},	/* DISP_TV_MOD_720P_50HZ */
	{74250000, 1280, 720, 1650, 110, 40, 220, 750, 5,
	 5, 20, 1, 1, 0, 0, 0},	/* DISP_TV_MOD_720P_60HZ */
	{74250000, 1920, 1080, 2640, 528, 44, 148, 1125, 2,
	 5, 15, 1, 1, 1, 0, 0},	/* DISP_TV_MOD_1080I_50HZ */
	{74250000, 1920, 1080, 2200, 88, 44, 148, 1125, 2,
	 5, 15, 1, 1, 1, 0, 0},	/* DISP_TV_MOD_1080I_60HZ */
	{74250000, 1920, 1080, 2750, 638, 44, 148, 1125, 4,
	 5, 36, 1, 1, 0, 0, 0},	/* DISP_TV_MOD_1080P_24HZ */
	{148500000, 1920, 1080, 2640, 528, 44, 148, 1125, 4,
	 5, 36, 1, 1, 0, 0, 0},	/* DISP_TV_MOD_1080P_50HZ */
	{148500000, 1920, 1080, 2200, 88, 44, 148, 1125, 4,
	 5, 36, 1, 1, 0, 0, 0},	/* DISP_TV_MOD_1080P_60HZ */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_PAL */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_PAL_SVIDEO */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* reserve */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_NTSC */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_NTSC_SVIDEO */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* reserve */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_PAL_M */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_PAL_M_SVIDEO */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* reserve */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_PAL_NC */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* DISP_TV_MOD_PAL_NC_SVIDEO */
	{000000000, 0000, 0000, 0000, 00, 00, 000, 0000, 0,
	 0, 00, 0, 0, 0, 0, 0},	/* reserve */
	{148500000, 1920, 2160, 2750, 638, 44, 148, 1125, 4,
	 5, 36, 1, 1, 0, 45, 1},	/* DISP_TV_MOD_1080P_24HZ_3D_FP */
	{148500000, 1280, 1440, 1980, 440, 40, 220, 750, 5,
	 5, 20, 1, 1, 0, 30, 1},	/* DISP_TV_MOD_720P_50HZ_3D_FP */
	{148500000, 1280, 1440, 1650, 110, 40, 220, 750, 5,
	 5, 20, 1, 1, 0, 30, 1},
};

__s32 tcon1_cfg(__u32 sel, struct __disp_timing_t *timing)
{
	__u32 start_delay;

	lcd_dev[sel]->tcon1_basic0.bits.x = timing->hor_pixels - 1;
	lcd_dev[sel]->tcon1_basic0.bits.y =
	    timing->ver_pixels / (timing->interlace + 1) - 1;
	lcd_dev[sel]->tcon1_basic1.bits.ls_xo = timing->hor_pixels - 1;
	lcd_dev[sel]->tcon1_basic1.bits.ls_yo =
	    timing->ver_pixels / (timing->interlace + 1) +
	    (timing->trd_mode ? timing->vactive_space : 0) - 1;
	lcd_dev[sel]->tcon1_basic2.bits.xo = timing->hor_pixels - 1;
	lcd_dev[sel]->tcon1_basic2.bits.yo =
	    timing->ver_pixels / (timing->interlace + 1) +
	    (timing->trd_mode ? timing->vactive_space : 0) - 1;
	lcd_dev[sel]->tcon1_basic3.bits.ht = timing->hor_total_time - 1;
	lcd_dev[sel]->tcon1_basic3.bits.hbp =
	    timing->hor_sync_time + timing->hor_back_time - 1;
	lcd_dev[sel]->tcon1_basic4.bits.vt =
	    timing->ver_total_time * (2 -
				      timing->interlace) *
	    (timing->trd_mode ? 2 : 1);
	lcd_dev[sel]->tcon1_basic4.bits.vbp =
	    timing->ver_sync_time + timing->ver_back_porch - 1;
	lcd_dev[sel]->tcon1_basic5.bits.hspw = timing->hor_sync_time - 1;
	lcd_dev[sel]->tcon1_basic5.bits.vspw = timing->ver_sync_time - 1;
	lcd_dev[sel]->tcon1_io_pol.bits.io0_inv = timing->ver_sync_polarity;
	lcd_dev[sel]->tcon1_io_pol.bits.io1_inv = timing->hor_sync_polarity;
	lcd_dev[sel]->tcon1_ctl.bits.interlace_en = timing->interlace;
	lcd_dev[sel]->tcon_fill_start0.bits.fill_begin =
	    (timing->ver_total_time + 1) << 12;
	lcd_dev[sel]->tcon_fill_end0.bits.fill_end =
	    (timing->ver_total_time + timing->vactive_space) << 12;
	lcd_dev[sel]->tcon_fill_data0.bits.fill_value = 0;
	lcd_dev[sel]->tcon_fill_ctl.bits.tcon1_fill_en =
	    timing->trd_mode ? 1 : 0;
	start_delay =
	    (timing->ver_total_time -
	     timing->ver_pixels) / (timing->interlace + 1) - 5;
	start_delay = (start_delay > 31) ? 31 : start_delay;
	lcd_dev[sel]->tcon1_ctl.bits.start_delay = start_delay;
	return 0;
}

__s32 tcon1_cfg_ex(__u32 sel, disp_panel_para *panel)
{
	return -1;
}

__s32 tcon1_set_hdmi_mode(__u32 sel, disp_tv_mode mode)
{
	tcon1_cfg(sel, tv_timing_tbl + mode);
	lcd_dev[sel]->tcon1_io_pol.bits.io2_inv = 1;
	lcd_dev[sel]->tcon1_io_tri.bits.io0_output_tri_en = 1;
	lcd_dev[sel]->tcon1_io_tri.bits.io1_output_tri_en = 1;
	lcd_dev[sel]->tcon1_io_tri.bits.io2_output_tri_en = 1;
	lcd_dev[sel]->tcon1_io_tri.bits.io3_output_tri_en = 1;
	lcd_dev[sel]->tcon1_io_tri.bits.data_output_tri_en = 0xffffff;

	hmdi_src_sel(sel);
	return 0;
}

__s32 tcon1_set_tv_mode(__u32 sel, disp_tv_mode mode)
{
	return -1;
}

#if 0
__s32 tcon1_set_vga_mode(__u32 sel, __disp_vga_mode_t mode)
{
	return -1;
}
#endif
__s32 tcon1_src_select(__u32 sel, enum __lcd_src_t src)
{
	if (src == LCD_SRC_BLUE)
		lcd_dev[sel]->tcon0_ctl.bits.src_sel = 2;
	else
		lcd_dev[sel]->tcon0_ctl.bits.src_sel = src;

	return 0;
}

__s32 tcon1_src_get(__u32 sel)
{

	return lcd_dev[sel]->tcon0_ctl.bits.src_sel;
}

static __u32 tcon_ceu_range_cut(__s32 *x_value, __s32 x_min, __s32 x_max)
{
	if (*x_value > x_max) {
		*x_value = x_max;
		return 1;
	} else if (*x_value < x_min) {
		*x_value = x_min;
		return 1;
	} else
		return 0;
}

static __s32 tcon_ceu_reg_corr(__s32 val, __u32 bit)
{
	if (val >= 0)
		return val;
	else
		return (bit) | (__u32) (-val);
}

static __s32 tcon_ceu_rect_multi(__s32 *dest, __s32 *src1, __s32 *src2)
{
	__u32 x, y, z;
	__s64 val_int64;

	for (x = 0; x < 4; x++)
		for (y = 0; y < 4; y++) {
			val_int64 = 0;
			for (z = 0; z < 4; z++)
				val_int64 +=
				    (__s64) src1[x * 4 + z] * src2[z * 4 + y];
			val_int64 = (val_int64 + 512) >> 10;
			dest[x * 4 + y] = val_int64;
		}
	return 0;
}

static __s32 tcon_ceu_rect_calc(__s32 *p_rect,
				__s32 b, __s32 c, __s32 s, __s32 h)
{
	__u8 const table_sin[91] = {

		0, 2, 4, 7, 9, 11,
		13, 16, 18, 20,
		22, 24, 27, 29, 31, 33,
		35, 37, 40, 42,
		44, 46, 48, 50, 52, 54,
		56, 58, 60, 62,
		64, 66, 68, 70, 72, 73,
		75, 77, 79, 81,
		82, 84, 86, 87, 89, 91,
		92, 94, 95, 97,
		98, 99, 101, 102, 104, 105,
		106, 107, 109, 110,
		111, 112, 113, 114, 115, 116,
		117, 118, 119, 119,
		120, 121, 122, 122, 123, 124,
		124, 125, 125, 126,
		126, 126, 127, 127, 127, 128,
		128, 128, 128, 128,
		128
	};

	__s32 const f_csh = 1024;
	__s32 const f_sh = 8;
	__s32 h1 = 0, h2 = 0, h3 = 0, h4 = 0;

	if (h >= 0 && h < 90) {
		h1 = table_sin[90 - h];
		h2 = table_sin[h];
		h3 = -table_sin[h];
		h4 = table_sin[90 - h];
	} else if (h >= 90 && h < 180) {
		h1 = -table_sin[h - 90];
		h2 = table_sin[180 - h];
		h3 = -table_sin[180 - h];
		h4 = -table_sin[h - 90];
	} else if (h >= 180 && h < 270) {
		h1 = -table_sin[270 - h];
		h2 = -table_sin[h - 180];
		h3 = table_sin[h - 180];
		h4 = -table_sin[270 - h];
	} else if (h >= 270 && h <= 360) {
		h1 = table_sin[h - 270];
		h2 = -table_sin[360 - h];
		h3 = table_sin[360 - h];
		h4 = table_sin[h - 270];
	}

	p_rect[0] = c * f_sh;
	p_rect[1] = 0;
	p_rect[2] = 0;
	p_rect[3] = -16 * c * f_sh + (b + 16) * f_csh;
	p_rect[4] = 0;
	p_rect[5] = (c * s * h1) >> 11;
	p_rect[6] = (c * s * h2) >> 11;
	p_rect[7] = 128 * (1 * f_csh - p_rect[5] - p_rect[6]);
	p_rect[8] = 0;
	p_rect[9] = (c * s * h3) >> 11;
	p_rect[10] = (c * s * h4) >> 11;
	p_rect[11] = 128 * (1 * f_csh - p_rect[9] - p_rect[10]);
	p_rect[12] = 0;
	p_rect[13] = 0;
	p_rect[14] = 0;
	p_rect[15] = 1024;
	return 0;
}

static __s32 tcon_ceu_calc(__u32 r2y_type, __u32 cen_type,
			   __u32 y2r_type, __s32 b, __s32 c, __s32 s, __s32 h,
			   __s32 *p_coff)
{
	const __s32 rect_1[16] = {

		1024, 0, 0, 0,
		0, 1024, 0, 0,
		0, 0, 1024, 0,
		0, 0, 0, 1024
	};

	const __s32 rect_r2y_sd[16] = {

		263, 516, 100, 16384,
		-152, -298, 450, 131072,
		450, -377, -73, 131072,
		0, 0, 0, 1024
	};

	const __s32 rect_r2y_hd[16] = {

		187, 629, 63, 16384,
		-103, -346, 450, 131072,
		450, -409, -41, 131072,
		0, 0, 0, 1024
	};

	const __s32 rect_y2r_sd[16] = {

		1192, 0, 1634, -228262,
		1192, -400, -833, 138740,
		1192, 2066, 0, -283574,
		0, 0, 0, 1024
	};

	const __s32 rect_y2r_hd[16] = {

		1192, 0, 1836, -254083,
		1192, -218, -547, 78840,
		1192, 2166, 0, -296288,
		0, 0, 0, 1024
	};

	__s32 rect_tmp0[16];
	__s32 rect_tmp1[16];

	__s32 *p_rect = NULL;
	__s32 *p_r2y = NULL;
	__s32 *p_y2r = NULL;
	__s32 *p_ceu = NULL;
	__u32 i = 0;

	if (r2y_type) {
		if (r2y_type == 1)
			p_r2y = (__s32 *) rect_r2y_sd;
		else if (r2y_type == 2)
			p_r2y = (__s32 *) rect_r2y_hd;
		p_rect = p_r2y;
	} else
		p_rect = (__s32 *) rect_1;

	if (cen_type) {
		tcon_ceu_range_cut(&b, -600, 600);
		tcon_ceu_range_cut(&c, 0, 300);
		tcon_ceu_range_cut(&s, 0, 300);
		tcon_ceu_range_cut(&h, 0, 360);
		p_ceu = rect_tmp1;
		tcon_ceu_rect_calc(p_ceu, b, c, s, h);
		tcon_ceu_rect_multi(rect_tmp0, p_ceu, p_rect);
		p_rect = rect_tmp0;
	}

	if (y2r_type) {
		if (y2r_type == 1)
			p_y2r = (__s32 *) rect_y2r_sd;
		else if (y2r_type == 2)
			p_y2r = (__s32 *) rect_y2r_hd;
		tcon_ceu_rect_multi(rect_tmp1, p_y2r, p_rect);
		p_rect = rect_tmp1;
	}

/*	const __s32 rect_srgb_warm[16]=*/
/*	{*/
/*		 1280,	   0,	   0,	   0,*/
/*			0,	1024,	   0,	   0,*/
/*			0,	   0,	 819,	   0,*/
/*			0,	   0,      0,	1024*/
/*	};*/

/*	const __s32 rect_srgb_cool[16]=*/
/*	{*/
/*		 819,	   0,	   0,	   0,*/
/*			0,	1024,	   0,	   0,*/
/*			0,	   0,	1280,	   0,*/
/*			0,	   0,      0,	1024*/
/*	};*/

/*	if(srgb_type) {*/
/*		if(srgb_type==1)*/
/*			p_srgb == (__s32*)rect_srgb_warm;*/
/*		else if(srgb_type==2)*/
/*			p_srgb == (__s32*)rect_srgb_cool;*/
/*		tcon_ceu_rect_multi(rect_tmp0,p_srgb,p_rect);*/
/*		p_rect = rect_tmp0;*/
/*	}*/

	for (i = 0; i < 12; i++)
		*(p_coff + i) = *(p_rect + i);

	return 0;
}

__s32 tcon_ceu(__u32 sel, __u32 mode, __s32 b, __s32 c, __s32 s, __s32 h)
{
	__s32 ceu_coff[12];
	__u32 error;

	if (mode == 1) {
		tcon_ceu_calc(1, 1, 1, b, c, s, h, ceu_coff);
	} else if (mode == 2) {
		tcon_ceu_calc(0, 1, 0, b, c, s, h, ceu_coff);
	} else {
		lcd_dev[sel]->tcon_ceu_ctl.bits.ceu_en = 0;
		return 0;
	}

	ceu_coff[0] = (ceu_coff[0] + 2) >> 2;
	ceu_coff[1] = (ceu_coff[1] + 2) >> 2;
	ceu_coff[2] = (ceu_coff[2] + 2) >> 2;
	ceu_coff[3] = (ceu_coff[3] + 32) >> 6;
	ceu_coff[4] = (ceu_coff[4] + 2) >> 2;
	ceu_coff[5] = (ceu_coff[5] + 2) >> 2;
	ceu_coff[6] = (ceu_coff[6] + 2) >> 2;
	ceu_coff[7] = (ceu_coff[7] + 32) >> 6;
	ceu_coff[8] = (ceu_coff[8] + 2) >> 2;
	ceu_coff[9] = (ceu_coff[9] + 2) >> 2;
	ceu_coff[10] = (ceu_coff[10] + 2) >> 2;
	ceu_coff[11] = (ceu_coff[11] + 32) >> 6;

	error = 0;
	error |= tcon_ceu_range_cut(ceu_coff + 0, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 1, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 2, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 3, -262143, 262143);
	error |= tcon_ceu_range_cut(ceu_coff + 4, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 5, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 6, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 7, -262143, 262143);
	error |= tcon_ceu_range_cut(ceu_coff + 8, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 9, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 10, -4095, 4095);
	error |= tcon_ceu_range_cut(ceu_coff + 11, -262143, 262143);

	if (error) {
		lcd_dev[sel]->tcon_ceu_ctl.bits.ceu_en = 0;
		return -1;
	}

	if (mode == 1) {
		lcd_dev[sel]->tcon_ceu_coef_rv.bits.max = 255;
		lcd_dev[sel]->tcon_ceu_coef_rv.bits.min = 0;
		lcd_dev[sel]->tcon_ceu_coef_gv.bits.max = 255;
		lcd_dev[sel]->tcon_ceu_coef_gv.bits.min = 0;
		lcd_dev[sel]->tcon_ceu_coef_bv.bits.max = 255;
		lcd_dev[sel]->tcon_ceu_coef_bv.bits.min = 0;
	} else if (mode == 2) {
		lcd_dev[sel]->tcon_ceu_coef_rv.bits.max = 235;
		lcd_dev[sel]->tcon_ceu_coef_rv.bits.min = 16;
		lcd_dev[sel]->tcon_ceu_coef_gv.bits.max = 240;
		lcd_dev[sel]->tcon_ceu_coef_gv.bits.min = 16;
		lcd_dev[sel]->tcon_ceu_coef_bv.bits.max = 240;
		lcd_dev[sel]->tcon_ceu_coef_bv.bits.min = 16;
	}
	lcd_dev[sel]->tcon_ceu_coef_rr.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[0], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_rg.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[1], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_rb.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[2], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_rc.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[3], 1 << 18);
	lcd_dev[sel]->tcon_ceu_coef_gr.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[0], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_gg.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[1], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_gb.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[2], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_gc.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[3], 1 << 18);
	lcd_dev[sel]->tcon_ceu_coef_br.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[0], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_bg.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[1], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_bb.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[2], 1 << 12);
	lcd_dev[sel]->tcon_ceu_coef_bc.bits.value =
	    tcon_ceu_reg_corr(ceu_coff[3], 1 << 18);
	lcd_dev[sel]->tcon_ceu_ctl.bits.ceu_en = 1;

	return 0;
}

__s32 tcon_gamma(__u32 sel, __u32 en, __u32 *gamma_tbl)
{
	__u32 i;

	lcd_dev[sel]->tcon_gctl.bits.tcon_gamma_en = 0;
	if (en == 0)
		return 0;

	for (i = 0; i < 256; i++) {
		lcd_dev[sel]->tcon_gamma_tlb[i].bits.pixel = *gamma_tbl;
		gamma_tbl++;
	}

	lcd_dev[sel]->tcon_gctl.bits.tcon_gamma_en = en;
	return 0;
}

__s32 tcon_cmap(__u32 sel, __u32 mode, unsigned int lcd_cmap_tbl[2][3][4])
{
	if (!(mode == 1)) {
		lcd_dev[sel]->tcon_cmap_ctl.bits.cmap_en = 0;
	} else {
		lcd_dev[sel]->tcon_cmap_odd0.bits.out0 =
		    (lcd_cmap_tbl[0][2][0] << 8) |
		    (lcd_cmap_tbl[0][1][0] << 4) | (lcd_cmap_tbl[0][0][0]);
		lcd_dev[sel]->tcon_cmap_odd0.bits.out1 =
		    (lcd_cmap_tbl[0][2][1] << 8) |
		    (lcd_cmap_tbl[0][1][1] << 4) | (lcd_cmap_tbl[0][0][1]);
		lcd_dev[sel]->tcon_cmap_odd1.bits.out2 =
		    (lcd_cmap_tbl[0][2][2] << 8) |
		    (lcd_cmap_tbl[0][1][2] << 4) | (lcd_cmap_tbl[0][0][2]);
		lcd_dev[sel]->tcon_cmap_odd1.bits.out3 =
		    (lcd_cmap_tbl[0][2][3] << 8) |
		    (lcd_cmap_tbl[0][1][3] << 4) | (lcd_cmap_tbl[0][0][3]);
		lcd_dev[sel]->tcon_cmap_even0.bits.out0 =
		    (lcd_cmap_tbl[1][2][0] << 8) |
		    (lcd_cmap_tbl[1][1][0] << 4) | (lcd_cmap_tbl[1][0][0]);
		lcd_dev[sel]->tcon_cmap_even0.bits.out1 =
		    (lcd_cmap_tbl[1][2][1] << 8) |
		    (lcd_cmap_tbl[1][1][1] << 4) | (lcd_cmap_tbl[1][0][1]);
		lcd_dev[sel]->tcon_cmap_even1.bits.out2 =
		    (lcd_cmap_tbl[1][2][2] << 8) |
		    (lcd_cmap_tbl[1][1][2] << 4) | (lcd_cmap_tbl[1][0][2]);
		lcd_dev[sel]->tcon_cmap_even1.bits.out3 =
		    (lcd_cmap_tbl[1][2][3] << 8) |
		    (lcd_cmap_tbl[1][1][3] << 4) | (lcd_cmap_tbl[1][0][3]);
		lcd_dev[sel]->tcon_cmap_ctl.bits.cmap_en = 1;
	}
	return 0;
}
