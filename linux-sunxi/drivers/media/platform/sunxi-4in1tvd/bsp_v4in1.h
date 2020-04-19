/*
 * bsp_tvd.h
 *
 *  Created on: 2015-9-6
 *      Author: dlp
 */

#ifndef __BSP_V4IN1_H__
#define __BSP_V4IN1_H__

#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#define FLITER_NUM 1
#define TVD_3D_COMP_BUFFER_SIZE (0x400000)

/*
* detail information of registers
*/

typedef union {
	u32 dwval;
	struct {
		u32 tvd_adc_map:2;
		u32 res0:30;
	} bits;
} tvd_top_map_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 tvd_en_3d_dma:1;
		u32 comb_3d_en:1;
		u32 res0:2;
		u32 comb_3d_sel:2;
		u32 res1:26;
	} bits;
} tvd_3d_ctl1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 dram_trig;
	} bits;
} tvd_3d_ctl2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 comb_3d_addr0;
	} bits;
} tvd_3d_ctl3_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 comb_3d_addr1;
	} bits;
} tvd_3d_ctl4_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 comb_3d_size;
	} bits;
} tvd_3d_ctl5_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 res0:4;
		u32 lpf_dig_en:1;
		u32 res1:19;
		u32 lpf_dig_sel:1;
		u32 res2:7;
	} bits;
} tvd_adc_dig_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 adc_en:1;
		u32 afe_en:1;
		u32 lpf_en:1;
		u32 lpf_sel:2;
		u32 res0:27;
	} bits;
} tvd_adc_ctl_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 stage1_ibias:2;
		u32 stage2_ibias:2;
		u32 stage3_ibias:2;
		u32 stage4_ibias:2;
		u32 stage5_ibias:2;
		u32 stage6_ibias:2;
		u32 stage7_ibias:2;
		u32 stage8_ibias:2;
		u32 clp_step:3;
		u32 res0:9;
		u32 data_dly:1;
		u32 res1:2;
		u32 adc_test:1;
	} bits;
} tvd_adc_cfg_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 adc_wb_length:23;
		u32 res0:1;
		u32 adc_wb_start:1;
		u32 res1:3;
		u32 adc_wb_buffer_reset:1;
		u32 adc_dump_mode:1;
		u32 adc_test_mode:1;
		u32 fifo_err:1;
	} bits;
} tvd_adc_dump_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 tvd_en_ch:1;
		u32 res0:14;
		u32 clr_rsmp_fifo:1;
		u32 res1:9;
		u32 en_lock_disable_write_back1only_start_wb_when_locked:1;
		u32 en_lock_disable_write_back2when_unlocked:1;
		u32 res2:5;
	} bits;
} tvd_en_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 ypbpr_mode:1;
		u32 svideo_mode:1;
		u32 progressive_mode:1;
		u32 res0:1;
		u32 blue_display_mode:2;
		u32 res1:2;
		u32 blue_color:1;
		u32 res2:23;
	} bits;
} tvd_mode_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 agc_en:1;
		u32 agc_frequence:1;
		u32 res0:6;
		u32 agc_target:8;
		u32 cagc_en:1;
		u32 res1:7;
		u32 cagc_target:8;
	} bits;
} tvd_clamp_agc1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 agc_gate_width:7;
		u32 res0:1;
		u32 agc_backporch_delay:8;
		u32 agc_gate_begin:13;
		u32 res1:2;
		u32 black_level_clamp:1;
	} bits;
} tvd_clamp_agc2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 h_sample_step;
	} bits;
} tvd_hlock1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 htol:4;
		u32 res0:12;
		u32 hsync_filter_gate_start_time:8;
		u32 hsync_filter_gate_end_time:8;
	} bits;
} tvd_hlock2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 hsync_rising_detect_window_start_time:8;
		u32 hsync_rising_detect_window_end_time:8;
		u32 hsync_tip_detect_window_start_time:8;
		u32 hsync_tip_detect_window_end_time:8;
	} bits;
} tvd_hlock3_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 hsync_detect_window_start_time_for_coarse_detection:8;
		u32 hsync_detect_window_end_time_for_corase_detect:8;
		u32 hsync_rising_time_for_fine_detect:8;
		u32 hsync_fine_to_coarse_offset:8;
	} bits;
} tvd_hlock4_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 hactive_start:8;
		u32 hactive_width:8;
		u32 backporch_detect_window_start_time:8;
		u32 backporch_detect_window_end_time:8;
	} bits;
} tvd_hlock5_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vtol:3;
		u32 res0:1;
		u32 vactive_start:11;
		u32 res1:1;
		u32 vactive_height:11;
		u32 res2:5;
	} bits;
} tvd_vlock1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 hsync_dectector_disable_start_line:7;
		u32 res0:9;
		u32 hsync_detector_disable_end_line:5;
		u32 res1:11;
	} bits;
} tvd_vlock2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 color_kill_en:1;
		u32 color_std:3;
		u32 res0:4;
		u32 burst_gate_start_time:8;
		u32 burst_gate_end_time:8;
		u32 wide_burst_gate:1;
		u32 res1:1;
		u32 chroma_lpf:2;
		u32 color_std_ntsc:1;
		u32 res2:3;
	} bits;
} tvd_clock1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 c_sample_step;
	} bits;
} tvd_clock2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 _3d_comb_filter_mode:3;
		u32 _3d_comb_filter_dis:1;
		u32 _2d_comb_filter_mode:4;
		u32 secam_notch_wide:1;
		u32 chroma_bandpass_filter_en:1;
		u32 pal_chroma_level:6;
		u32 comb_filter_buffer_clear:1;
		u32 res0:3;
		u32 notch_factor:3;
		u32 _2d_comb_factor:3;
		u32 _3d_comb_factor:3;
		u32 chroma_coring_enable:1;
		u32 res1:2;
	} bits;
} tvd_yc_sep1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 horizontal_luma_filter_gain:2;
		u32 horizontal_chroma_filter_gain:2;
		u32 luma_vertical_filter_gain:2;
		u32 chroma_vertical_filter_gain:2;
		u32 motion_detect_noise_detect_en:1;
		u32 motion_detect_noise_threshold:7;
		u32 noise_detect_en:1;
		u32 noise_threshold:7;
		u32 luma_noise_factor:2;
		u32 chroma_noise_factor:2;
		u32 burst_noise_factor:2;
		u32 vertical_noise_factor:2;
	} bits;
} tvd_yc_sep2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 yc_delay:4;
		u32 res0:4;
		u32 contrast_gain:8;
		u32 bright_offset:8;
		u32 sharp_en:1;
		u32 sharp_coef1:3;
		u32 sharp_coef2:2;
		u32 res1:2;
	} bits;
} tvd_enhance1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 saturation_gain:8;
		u32 chroma_enhance_en:1;
		u32 chroma_enhance_strength:2;
		u32 res0:21;
	} bits;
} tvd_enhance2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 cb_gain:12;
		u32 res0:4;
		u32 cr_gain:12;
		u32 cbcr_gain_en:1;
		u32 res1:3;
	} bits;
} tvd_enhance3_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 wb_en:1;
		u32 wb_format:1;
		u32 field_sel:1;
		u32 hyscale_en:1;
		u32 wb_mb_mode:1;
		u32 wb_frame_mode:1;
		u32 flip_field:1;
		u32 res0:1;
		u32 wb_addr_valid:1;
		u32 res1:7;
		u32 hactive_stride:12;
		u32 res2:3;
		/* 0x0->nv12, 0x1->nv21 */
		u32 wb_uv_swap:1;
	} bits;
} tvd_wb1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 hactive_num:12;
		u32 res0:4;
		u32 vactive_num:11;
		u32 res1:5;
	} bits;
} tvd_wb2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 ch1_y_addr;
	} bits;
} tvd_wb3_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 ch1_c_addr;
	} bits;
} tvd_wb4_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 lock:1;
		u32 unlock:1;
		u32 res0:2;
		u32 fifo_c_o:1;
		u32 fifo_y_o:1;
		u32 res1:1;
		u32 fifo_c_u:1;
		u32 fifo_y_u:1;
		u32 res2:7;
		u32 wb_addr_change_err:1;
		u32 res3:7;
		u32 frame_end:1;
		u32 res4:3;
		u32 fifo_3d_rx_u:1;
		u32 fifo_3d_rx_o:1;
		u32 fifo_3d_tx_u:1;
		u32 fifo_3d_tx_o:1;
	} bits;
} tvd_irq_ctl_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 lock:1;
		u32 unlock:1;
		u32 res0:2;
		u32 fifo_c_o:1;
		u32 fifo_y_o:1;
		u32 res1:1;
		u32 fifo_c_u:1;
		u32 fifo_y_u:1;
		u32 res2:7;
		u32 wb_addr_change_err:1;
		u32 res3:7;
		u32 frame_end:1;
		u32 res4:3;
		u32 fifo_3d_rx_u:1;
		u32 fifo_3d_rx_o:1;
		u32 fifo_3d_tx_u:1;
		u32 fifo_3d_tx_o:1;
	} bits;
} tvd_irq_status_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 res0:8;
		u32 afe_gain_value:8;
		u32 tvin_lock_debug:1;
		u32 tvin_lock_high:1;
		u32 truncation2_reset_gain_enable:1;
		u32 truncation_reset_gain_enable:1;
		u32 unlock_reset_gain_enable:1;
		u32 afe_gain_mode:1;
		u32 clamp_mode:1;
		u32 clamp_up_start:1;
		u32 clamp_dn_start:1;
		u32 clamp_updn_cycles:7;
	} bits;
} tvd_debug1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 res0:3;
		u32 agc_gate_thresh:5;
		u32 ccir656_en:1;
		u32 adc_cbcr_pump_swap:1;
		u32 adc_updn_swap:1;
		u32 adc_input_swap:1;
		u32 hv_dely:1;
		u32 cv_inv:1;
		u32 tvd_src:1;
		u32 res1:3;
		u32 adc_wb_mode:2;
		u32 hue:8;
		u32 res2:4;
	} bits;
} tvd_debug2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 res0:20;
		u32 noise_thresh:8;
		u32 ccir656_cbcr_write_back_sequence:1;
		u32 cbcr_swap:1;
		u32 nstd_hysis:2;
	} bits;
} tvd_debug3_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 fixed_burstgate:1;
		u32 cautopos:5;
		u32 vnon_std_threshold:4;
		u32 hnon_std_threshold:4;
		u32 user_ckill_mode:2;
		u32 hlock_ckill:1;
		u32 vbi_ckill:1;
		u32 chroma_kill:4;
		u32 agc_gate_vsync_stip:1;
		u32 agc_gate_vsync_coarse:1;
		u32 agc_gate_kill_mode:2;
		u32 agc_peak_en:1;
		u32 hstate_unlocked:1;
		u32 disable_hfine:1;
		u32 hstate_fixed:1;
		u32 hlock_vsync_mode:2;
	} bits;
} tvd_debug4_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vsync_clamp_mode:2;
		u32 vsync_vbi_lockout_start:7;
		u32 vsync_vbi_max:7;
		u32 vlock_wide_range:1;
		u32 locked_count_clean_max:4;
		u32 locked_count_noisy_max:4;
		u32 hstate_max:3;
		u32 fixed_cstate:1;
		u32 vodd_delayed:1;
		u32 veven_delayed:1;
		u32 field_polarity:1;
	} bits;
} tvd_debug5_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 cstate:3;
		u32 lose_chromalock_level:3;
		u32 lose_chromalock_count:4;
		u32 palsw_level:2;
		u32 vsync_thresh:6;
		u32 vsync_cntl:2;
		u32 vloop_tc:2;
		u32 field_detect_mode:2;
		u32 cpump_delay:8;
	} bits;
} tvd_debug6_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 hresampler_2up:1;
		u32 cpump_adjust_polarity:1;
		u32 cpump_adjust_delay:6;
		u32 cpump_adjust:8;
		u32 cpump_delay_en:1;
		u32 vf_nstd_en:1;
		u32 vcr_auto_switch_en:1;
		u32 mv_hagc:1;
		u32 dagc_en:1;
		u32 agc_half_en:1;
		u32 dc_clamp_mode:2;
		u32 ldpause_threshold:4;
		u32 res0:4;
	} bits;
} tvd_debug7_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 chroma_step_ntsc;
	} bits;
} tvd_debug8_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 chroma_step_paln;
	} bits;
} tvd_debug9_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 chroma_step_palm;
	} bits;
} tvd_debug10_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 chroma_step_pal;
	} bits;
} tvd_debug11_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 y_wb_protect;
	} bits;
} tvd_debug12_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 uv_wb_protect;
	} bits;
} tvd_debug13_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 agc_peak_nominal:7;
		u32 agc_peak_cntl:3;
		u32 vsync_agc_lockout_start:7;
		u32 vsync_agc_max:6;
		u32 noise_line:5;
		u32 res0:4;
	} bits;
} tvd_debug14_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 agc_analog_gain_status:8;
		u32 agc_digital_gain_status:8;
		u32 chroma_magnitude_status:8;
		u32 res0:8;
	} bits;
} tvd_status1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 chroma_sync_dto_increment_status;
	} bits;
} tvd_status2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 horizontal_sync_dto_increment_status:30;
		u32 res0:2;
	} bits;
} tvd_status3_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 no_signal_detected:1;
		u32 h_locked:1;
		u32 v_locked:1;
		u32 chroma_pll_locked_to_colour_burst:1;
		u32 macrovision_vbi_pseudosync_pulses_detection:1;
		u32 macrovision_colour_stripes_detected:3;
		u32 proscan_detected:1;
		u32 hnon_standard:1;
		u32 vnon_standard:1;
		u32 res0:5;
		u32 pal_detected:1;
		u32 secam_detected:1;
		u32 _625lines_detected:1;
		u32 noisy:1;
		u32 vcr:1;
		u32 vcr_trick:1;
		u32 vcr_ff:1;
		u32 vcr_rew:1;
		u32 res1:8;
	} bits;
} tvd_status4_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 adc_data_value:10;
		u32 res0:1;
		u32 adc_data_show:1;
		u32 sync_level:11;
		u32 blank_level:11;
	} bits;
} tvd_status5_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 auto_detect_finish:1;
		u32 tv_standard:3;
		u32 auto_detect_en:1;
		u32 mask_palm:1;
		u32 mask_palcn:1;
		u32 mask_pal60:1;
		u32 mask_ntsc443:1;
		u32 mask_secam:1;
		u32 mask_unknown:1;
		u32 res0:21;
	} bits;
} tvd_status6_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 teletext_vbi_frame_code_register1:8;
		u32 teletext_vbi_frame_code_register2:8;
		u32 data_high_level_register:8;
		u32 vbi_data_type_configuration_register_for_line7:8;
	} bits;
} tvd_vbi1_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_data_type_configuration_register_for_line8:8;
		u32 vbi_data_type_configuration_register_for_line9:8;
		u32 vbi_data_type_configuration_register_for_line10:8;
		u32 vbi_data_type_configuration_register_for_line11:8;
	} bits;
} tvd_vbi2_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_data_type_configuration_register_for_line12:8;
		u32 vbi_data_type_configuration_register_for_line13:8;
		u32 vbi_data_type_configuration_register_for_line14:8;
		u32 vbi_data_type_configuration_register_for_line15:8;
	} bits;
} tvd_vbi3_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_data_type_configuration_register_for_line16:8;
		u32 vbi_data_type_configuration_register_for_line17:8;
		u32 vbi_data_type_configuration_register_for_line18:8;
		u32 vbi_data_type_configuration_register_for_line19:8;
	} bits;
} tvd_vbi4_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_data_type_configuration_register_for_line20:8;
		u32 vbi_data_type_configuration_register_for_line21:8;
		u32 vbi_data_type_configuration_register_for_line22:8;
		u32 vbi_data_type_configuration_register_for_line23:8;
	} bits;
} tvd_vbi5_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_data_type_configuration:8;
		u32 vbi_loop_filter_gain:8;
		u32 vbi_loop_filter_i_gain:8;
		u32 vbi_loop_filter_g_gain:8;
	} bits;
} tvd_vbi6_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 upper_byte_vbi_close_caption_dto:8;
		u32 lower_byte_vbi_close_caption_dto:8;
		u32 upper_byte_vbi_teletext_dto:8;
		u32 lower_byte_vbi_teletext_dto:8;
	} bits;
} tvd_vbi7_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 upper_byte_vbi_wss625_dto:8;
		u32 lower_byte_vbi_wss625_dto:8;
		u32 vbi_close_caption_data_1_register1:8;
		u32 vbi_close_caption_data_1_register2:8;
	} bits;
} tvd_vbi8_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_close_caption_data_1_register3:8;
		u32 vbi_close_caption_data_1_register4:8;
		u32 vbi_close_caption_data_2_register:8;
		u32 vbi_wss_data_1_register:8;
	} bits;
} tvd_vbi9_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_wss_data_2_register:8;
		u32 vbi_data_status_register:8;
		u32 vbi_caption_start_register:8;
		u32 vbi_wss625_start_register:8;
	} bits;
} tvd_vbi10_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 vbi_teletext_start_register:8;
		u32 res0:8;
		u32 res1:8;
		u32 res2:8;
	} bits;
} tvd_vbi11_reg_t;

typedef union {
	u32 dwval;
	struct {
		u32 res0;
	} bits;
} tvd_reservd_reg_t;

typedef struct {
	tvd_top_map_reg_t tvd_top_map;
	tvd_reservd_reg_t tvd_top_reg004;
	tvd_3d_ctl1_reg_t tvd_3d_ctl1;
	tvd_3d_ctl2_reg_t tvd_3d_ctl2;
	tvd_3d_ctl3_reg_t tvd_3d_ctl3;
	tvd_3d_ctl4_reg_t tvd_3d_ctl4;
	tvd_3d_ctl5_reg_t tvd_3d_ctl5;
	tvd_reservd_reg_t tvd_top_reg01c[2];
	tvd_adc_dig_reg_t tvd_adc0_dig;
	tvd_adc_ctl_reg_t tvd_adc0_ctl;
	tvd_adc_cfg_reg_t tvd_adc0_cfg;
	tvd_reservd_reg_t tvd_top_reg030[5];
	tvd_adc_dig_reg_t tvd_adc1_dig;
	tvd_adc_ctl_reg_t tvd_adc1_ctl;
	tvd_adc_cfg_reg_t tvd_adc1_cfg;
	tvd_reservd_reg_t tvd_top_reg050[5];
	tvd_adc_dig_reg_t tvd_adc2_dig;
	tvd_adc_ctl_reg_t tvd_adc2_ctl;
	tvd_adc_cfg_reg_t tvd_adc2_cfg;
	tvd_reservd_reg_t tvd_top_reg060[5];
	tvd_adc_dig_reg_t tvd_adc3_dig;
	tvd_adc_ctl_reg_t tvd_adc3_ctl;
	tvd_adc_cfg_reg_t tvd_adc3_cfg;
	tvd_reservd_reg_t tvd_top_reg090[24];
	tvd_adc_dump_reg_t tvd_adc_dump;
} __tvd_top_dev_t;

typedef struct {
	tvd_en_reg_t tvd_en;
	tvd_mode_reg_t tvd_mode;
	tvd_clamp_agc1_reg_t tvd_clamp_agc1;
	tvd_clamp_agc2_reg_t tvd_clamp_agc2;
	tvd_hlock1_reg_t tvd_hlock1;
	tvd_hlock2_reg_t tvd_hlock2;
	tvd_hlock3_reg_t tvd_hlock3;
	tvd_hlock4_reg_t tvd_hlock4;
	tvd_hlock5_reg_t tvd_hlock5;
	tvd_vlock1_reg_t tvd_vlock1;
	tvd_vlock2_reg_t tvd_vlock2;
	tvd_reservd_reg_t tvd_reg02c;
	tvd_clock1_reg_t tvd_clock1;
	tvd_clock2_reg_t tvd_clock2;
	tvd_reservd_reg_t tvd_reg038[2];
	tvd_yc_sep1_reg_t tvd_yc_sep1;
	tvd_yc_sep2_reg_t tvd_yc_sep2;
	tvd_reservd_reg_t tvd_reg048[2];
	tvd_enhance1_reg_t tvd_enhance1;
	tvd_enhance2_reg_t tvd_enhance2;
	tvd_enhance3_reg_t tvd_enhance3;
	tvd_reservd_reg_t tvd_reg05c;
	tvd_wb1_reg_t tvd_wb1;
	tvd_wb2_reg_t tvd_wb2;
	tvd_wb3_reg_t tvd_wb3;
	tvd_wb4_reg_t tvd_wb4;
	tvd_reservd_reg_t tvd_reg070[4];
	tvd_irq_ctl_reg_t tvd_irq_ctl;
	tvd_reservd_reg_t tvd_reg084[3];
	tvd_irq_status_reg_t tvd_irq_status;
	tvd_reservd_reg_t tvd_reg094[27];
	tvd_debug1_reg_t tvd_debug1;
	tvd_debug2_reg_t tvd_debug2;
	tvd_debug3_reg_t tvd_debug3;
	tvd_debug4_reg_t tvd_debug4;
	tvd_debug5_reg_t tvd_debug5;
	tvd_debug6_reg_t tvd_debug6;
	tvd_debug7_reg_t tvd_debug7;
	tvd_debug8_reg_t tvd_debug8;
	tvd_debug9_reg_t tvd_debug9;
	tvd_debug10_reg_t tvd_debug10;
	tvd_debug11_reg_t tvd_debug11;
	tvd_debug12_reg_t tvd_debug12;
	tvd_debug13_reg_t tvd_debug13;
	tvd_debug14_reg_t tvd_debug14;
	tvd_reservd_reg_t tvd_reg138[18];
	tvd_status1_reg_t tvd_status1;
	tvd_status2_reg_t tvd_status2;
	tvd_status3_reg_t tvd_status3;
	tvd_status4_reg_t tvd_status4;
	tvd_status5_reg_t tvd_status5;
	tvd_status6_reg_t tvd_status6;
	tvd_reservd_reg_t tvd_reg198[906];
	tvd_vbi1_reg_t tvd_vbi1;
	tvd_vbi2_reg_t tvd_vbi2;
	tvd_vbi3_reg_t tvd_vbi3;
	tvd_vbi4_reg_t tvd_vbi4;
	tvd_vbi5_reg_t tvd_vbi5;
	tvd_vbi6_reg_t tvd_vbi6;
	tvd_vbi7_reg_t tvd_vbi7;
	tvd_vbi8_reg_t tvd_vbi8;
	tvd_vbi9_reg_t tvd_vbi9;
	tvd_vbi10_reg_t tvd_vbi10;
	tvd_vbi11_reg_t tvd_vbi11;
} __tvd_dev_t;

typedef enum {
	TVD_IRQ_LOCK = 0,
	TVD_IRQ_UNLOCK = 1,
	TVD_IRQ_FIFO_C_O = 4,
	TVD_IRQ_FIFO_Y_O = 5,
	TVD_IRQ_FIFO_C_U = 7,
	TVD_IRQ_FIFO_Y_U = 8,
	TVD_IRQ_WB_ADDR_CHANGE_ERR = 16,
	TVD_IRQ_FRAME_END = 24,
	TVD_IRQ_FIFO_3D_RX_U = 28,
	TVD_IRQ_FIFO_3D_RX_O = 29,
	TVD_IRQ_FIFO_3D_TX_U = 30,
	TVD_IRQ_FIFO_3D_TX_O = 31,
} TVD_IRQ_T;

typedef enum {
	TVD_PL_YUV420 = 0,
	TVD_MB_YUV420 = 1,
	TVD_PL_YUV422 = 2,
} TVD_FMT_T;

s32 tvd_top_set_reg_base(unsigned long base);
s32 tvd_set_reg_base(u32 sel, unsigned long base);
s32 tvd_init(u32 sel, u32 interface);
s32 tvd_get_status(u32 sel, u32 *locked, u32 *system);
s32 tvd_config(u32 sel, u32 interface, u32 mode);
s32 tvd_set_wb_width(u32 sel, u32 width);
s32 tvd_set_wb_width_and_jump(u32 sel, u32 width);
s32 tvd_set_wb_width_jump(u32 sel, u32 width_jump);
s32 tvd_set_wb_height(u32 sel, u32 height);
s32 tvd_set_wb_addr(u32 sel, u32 addr_y, u32 width, u32 height);
s32 tvd_set_wb_fmt(u32 sel, TVD_FMT_T fmt);
s32 tvd_set_wb_uv_swap(u32 sel, u32 swap);
s32 tvd_set_wb_field(u32 sel, u32 is_field_mode, u32 is_field_even);
s32 tvd_capture_on(u32 sel);
s32 tvd_capture_off(u32 sel);
s32 tvd_irq_enable(u32 sel, TVD_IRQ_T irq_id);
s32 tvd_irq_disable(u32 sel, TVD_IRQ_T irq_id);
s32 tvd_irq_status_get(u32 sel, TVD_IRQ_T irq_id, u32 *irq_status);
s32 tvd_irq_status_clear(u32 sel, TVD_IRQ_T irq_id);
s32 tvd_dma_irq_status_get(u32 sel, u32 *irq_status);
s32 tvd_dma_irq_status_clear_err_flag(u32 sel, u32 irq_status);

s32 tvd_adc_config(u32 adc);
s32 tvd_set_saturation(u32 sel, u32 saturation);
s32 tvd_set_luma(u32 sel, u32 luma);
s32 tvd_set_contrast(u32 sel, u32 contrast);
u32 tvd_get_saturation(u32 sel);
u32 tvd_get_luma(u32 sel);
u32 tvd_get_contrast(u32 sel);
void tvd_3d_mode(u32 _3d_sel, u32 _3d_en, u32 _3d_addr);
u32 tvd_dbgmode_dump_data(u32 chan_sel, u32 mode, uintptr_t dump_dst_addr,
			  u32 data_length);
void tvd_agc_auto_config(u32 sel);
void tvd_agc_manual_config(u32 sel, u32 agc_manual_val);
void tvd_cagc_config(u32 sel, u32 enable);
#endif
