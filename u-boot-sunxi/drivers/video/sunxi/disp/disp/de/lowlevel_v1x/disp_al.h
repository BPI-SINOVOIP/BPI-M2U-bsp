/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _DISP_AL_H_
#define _DISP_AL_H_

#include "./ebios_lcdc_tve.h"
#include "./ebios_de.h"
#include "../bsp_display.h"
#include "../disp_private.h"

#define MAX_NUM_SCREEN 3

struct __disp_al_be_info_t {
	/* 0: dram; 1: fe0; 2:fe1; 3: fe2 ..(1-..: if fe exist) */
	u32 input_source;
	u32 out_select;		/* lcd, fe2 */
	u32 in_csc, out_csc;	/* 0:rgb; 1:yuv */
	u32 color_range;
	/* input size, eq the out_size of the pre_mod */
	u32 in_width, in_height;
	/* output size, eq the in_size of the next mod */
	u32 out_width, out_height;
	u32 b_out_interlace, b_in_interlace;
	u32 b_de_flicker;
	u32 enabled;
};

struct lcd_clk_info {
	disp_lcd_if lcd_if;
	int tcon_div;
	int lcd_div;
	int dsi_div;
	int dsi_rate;
};

struct __disp_al_fe_info_t {
	u32 out_select;		/* 0: be0; 1:be1; 2:be2; 0xff: wb */
	u32 in_csc, out_csc;	/* 0:rgb; 1:yuv */
	u32 color_range;
	/* input size, eq the out_size of the pre_mod */
	u32 in_width, in_height;
	/* output size, eq the in_size of the next mod */
	u32 out_width, out_height;
	u32 b_out_interlace, b_in_interlace;
	u32 b_de_flicker;
	u32 b_top_filed_first;
	u32 b_de_interlace;
	u32 enabled;
	u32 close_request;
	struct disp_scaler_info scaler_info;

	 /**/
/*	struct {*/
/*		u32 clk;*/
/*		u32 clk_src;*/
/*		u32 h_clk;*/
/*		u32 enabled;*/
/*	}clk;*/
#if 0
	struct disp_clk_info_t clk;
	struct disp_clk_info_t extra_clk;
#endif
	u32 clk_enabled;
	u32 de_freq;
	/* jolyxie */
	struct clk *clk;
	struct clk *clk_parent;
	struct clk *extra_clk;
};

struct __disp_al_lcd_info_t {
	/* input size, eq the out_size of the pre_mod */
	u32 in_width, in_height;
	/* display size, eq the in_size of the next mod */
	u32 out_width, out_height;
	u32 in_csc, out_csc;	/* 0:rgb; 1:yuv */
	u32 b_out_interlace, b_in_interlace;
	u32 backlight;
	u32 enabled;

	u32 lcd_if;
	u32 tcon_index;		/* 0:for panel; 1:for hdmi */
	u32 time_per_line;	/* unit:0.1us */
};

struct __disp_al_drc_info_t {
	/* input size, eq the out_size of the pre_mod */
	u32 in_width, in_height;
	/* display size, eq the in_size of the next mod */
	u32 out_width, out_height;
	u32 in_csc, out_csc;
	u32 b_de_flicker;
	u32 b_out_interlace, b_in_interlace;
	u32 backlight, backlight_dimming;
	u32 enabled;

	u32 mode;		/* 0:rgb(UI mode); 1:yuv(video mode) */
};

/* capture */
struct __disp_al_capture_info_t {
	u32 fps_counter;
	u32 switch_buffer_counter;
	disp_capture_para capture_para;
};

/* sat */
struct __disp_al_sat_info_t {
	u32 init_flag;
	disp_window window;
	disp_size size;
};

struct __disp_al_private_data {
	struct __disp_al_be_info_t be_info;
	struct __disp_al_fe_info_t fe_info;
	struct __disp_al_lcd_info_t lcd_info;
	struct __disp_al_drc_info_t drc_info;
	struct __disp_al_capture_info_t capture_info;
	struct __disp_al_sat_info_t sat_info;
	u32 output_type;
};

struct __disp_al_mod_list_t {
	disp_mod_id mod_id;
	struct list_head list;
};

struct __disp_al_mod_init_data {
	disp_mod_id mod_id;
	disp_mod_id reg_id;
	unsigned int offset;
	char *name;
	u32 screen_id;
	 s32 (*set_reg_base)(u32 screen_id, uintptr_t reg_base);
	int (*notifier_call)(struct disp_notifier_block *,
			      u32 event, u32 sel, void *data);
};

struct __de_format_t {
	disp_pixel_format format;	/* input format */

	/* reg config */
	/* 0:plannar; 1: interleaved; */
	/*2: plannar uv combined; */
	/*4: plannar mb; */
	/*6: uv combined mb */
	u32 mod;
	/* 0:yuv444; 1: yuv422; */
	/* 2: yuv420; 3:yuv411; */
	/* 4: csi rgb; 5:rgb888 */
	u32 fmt;
	u32 ps;			/*  */
	u32 br_swap;		/* swap blue & red */
};

/***********************************************************
 *
 * global
 *
 ***********************************************************/
s32 disp_al_check_csc(u32 screen_id);
s32 disp_al_check_display_size(u32 screen_id);
s32 disp_al_format_to_bpp(disp_pixel_format fmt);
s32 disp_al_query_mod(disp_mod_id mod_id);
s32 disp_al_query_be_mod(u32 screen_id);
s32 disp_al_query_fe_mod(u32 screen_id);
s32 disp_al_query_lcd_mod(u32 screen_id);
s32 disp_al_query_smart_color_mod(u32 screen_id);
s32 disp_al_query_drc_mod(u32 screen_id);
s32 disp_al_query_dsi_mod(u32 screen_id);
s32 disp_al_query_edp_mod(u32 screen_id);

/************************************************************/
/*scaler*/
/*************************************************************/
s32 scaler_close(u32 scaler_id);
s32 scaler_sync_ex(u32 sel);
/***********************************************************/
 /**/
/* disp_al_lcd*/
     /**/
/***********************************************************/
s32 disp_al_lcd_cfg(u32 screen_id, disp_panel_para *panel);
s32 disp_al_lcd_init(u32 screen_id);
s32 disp_al_lcd_exit(u32 screen_id);
s32 disp_al_lcd_enable(u32 screen_id, u32 enable, disp_panel_para *panel);
s32 disp_al_lcd_set_src(u32 screen_id, disp_lcd_src src);
/* query lcd irq, clear it when the irq queried exist
 * take dsi irq s32o account, todo?
 */
s32 disp_al_lcd_query_irq(u32 screen_id, enum __lcd_irq_id_t irq_id);
/* take dsi irq s32o account, todo? */
s32 disp_al_lcd_tri_busy(u32 screen_id);
/* take dsi irq s32o account, todo? */
s32 disp_al_lcd_tri_start(u32 screen_id);
s32 disp_al_lcd_io_cfg(u32 screen_id, u32 enabled, disp_panel_para *panel);
s32 disp_al_lcd_get_cur_line(u32 screen_id);
s32 disp_al_lcd_get_start_delay(u32 screen_id);
s32 disp_al_lcd_set_clk_div(u32 screen_id, u32 clk_div);
u32 disp_al_lcd_get_clk_div(u32 screen_id);
s32 disp_al_edp_init(u32 screen_id, u32 edp_rate);
s32 disp_al_edp_cfg(u32 screen_id, disp_panel_para *panel);
s32 disp_al_edp_disable_cfg(u32 screen_id);
s32 disp_al_edp_int(enum __edp_irq_id_t edp_irq);
s32 disp_al_lcd_check_time(u32 screen_id, u32 us);
/***********************************************************
 *
 * disp_al_manager
 *
 ***********************************************************/
/* init reg */
s32 disp_al_manager_init(u32 screen_id);
s32 disp_al_mnanger_exit(u32 screen_id);
/*
 * take irq en/disable & reg_auto_load s32o account
 */
s32 disp_al_manager_enable(u32 screen_id, u32 enabled);
s32 disp_al_manager_set_backcolor(u32 screen_id, disp_color_info *bk_color);
s32 disp_al_manager_set_color_key(u32 screen_id, disp_colorkey *ck_mode);
s32 disp_al_manager_sync(u32 screen_id);
s32 disp_al_manager_query_irq(u32 screen_id, u32 irq_id);
s32 disp_al_manager_set_display_size(u32 screen_id, u32 width, u32 height);
s32 disp_al_manager_get_display_size(u32 screen_id, u32 *width, u32 *height);

/***********************************************************
 *
 * disp_al_capture
 *
 ***********************************************************/
s32 disp_al_capture_init(u32 screen_id);
s32 disp_al_capture_screen_proc(u32 screen_id);
s32 disp_al_capture_screen_switch_buff(u32 screen_id);
s32 disp_al_capture_screen(u32 screen_id, disp_capture_para *para);
s32 disp_al_capture_screen_stop(u32 screen_id);
s32 disp_al_capture_screen_get_buffer_id(u32 screen_id);
s32 disp_al_capture_sync(u32 screen_id);
s32 disp_al_caputure_screen_finished(u32 screen_id);

/***********************************************************
 *
 * disp_al_layer
 *
 ***********************************************************/
s32 disp_al_layer_set_pipe(u32 screen_id, u32 layer_id, u32 pipe);
s32 disp_al_layer_set_zorder(u32 screen_id, u32 layer_id, u32 zorder);
s32 disp_al_layer_set_alpha_mode(u32 screen_id,
				 u32 layer_id, u32 alpha_mode, u32 alpha_value);
s32 disp_al_layer_color_key_enable(u32 screen_id, u32 layer_id, u32 enabled);
s32 disp_al_layer_set_screen_window(u32 screen_id,
				    u32 layer_id, disp_window *window);
s32 disp_al_layer_set_framebuffer(u32 screen_id,
				  u32 layer_id, disp_fb_info *fb);
s32 disp_al_layer_enable(u32 screen_id, u32 layer_id, u32 enabled);
s32 disp_al_layer_set_extra_info(u32 screen_id,
				 u32 layer_id, disp_layer_info *info,
				 struct __disp_layer_extra_info_t *extra_info);
s32 disp_al_layer_sync(u32 screen_id, u32 layer_id,
		       struct __disp_layer_extra_info_t *extra_info);

/* scale */
s32 scaler_init(struct __disp_bsp_init_para *para);
s32 scaler_exit(void);

/***********************************************************
 *
 * disp_al_smcl(smart color)
 *
 ***********************************************************/
s32 disp_al_smcl_clk_init(u32 clk_id);
s32 disp_al_smcl_clk_exit(u32 clk_id);
s32 disp_al_smcl_clk_enable(u32 clk_id, u32 enable);
s32 disp_al_smcl_set_para(u32 screen_id,
			  u32 brightness, u32 saturaion, u32 hue, u32 mode);
s32 disp_al_smcl_set_window(u32 screen_id, disp_window *window);
s32 disp_al_smcl_enable(u32 screen_id, u32 enable);
s32 disp_al_smcl_sync(u32 screen_id);
/***********************************************************
 *
 * disp_al_smbl(smart backlight)
 *
 ***********************************************************/
s32 disp_al_smbl_enable(u32 screen_id, u32 enable);
s32 disp_al_smbl_set_window(u32 screen_id, disp_window *window);
s32 disp_al_smbl_sync(u32 screen_id);
s32 disp_al_smbl_update_backlight(u32 screen_id, u32 bl);
s32 disp_al_smbl_get_backlight_dimming(u32 screen_id);

s32 disp_al_smbl_tasklet(u32 screen_id);

/* private */
s32 disp_al_notifier_register(struct disp_notifier_block *nb);
s32 disp_al_notifier_unregister(struct disp_notifier_block *nb);

s32 disp_init_al(struct __disp_bsp_init_para *para);
s32 scaler_sync(u32 scaler_id);
s32 scaler_close(u32 scaler_id);

#endif
