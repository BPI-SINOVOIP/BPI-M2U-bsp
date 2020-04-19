/* de_edp_core.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * core function of edp module
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _DE_EDP_CORE_H
#define _DE_EDP_CORE_H
#include "drv_edp_common.h"
#include "de_edp_param.h"

int dp_get_sink_info(struct sink_info *sink_inf);
int dp_sink_init(u32 sel);
int dp_video_set(u32 sel, struct disp_video_timings *tmg, u32 fps,
		 enum edp_video_src_t src);
int para_convert(int lanes, int color_bits, int ht_per_lane, int ht,
		 struct video_para *result);
int dp_tps1_test(u32 sel, u32 swing_lv, u32 pre_lv, u8 is_swing_max,
		 u8 is_pre_max);
s32 dp_tps2_test(u32 sel, u32 swing_lv, u32 pre_lv, u8 is_swing_max,
		 u8 is_pre_max);
s32 dp_quality_test(u32 sel, u32 swing_lv, u32 pre_lv, u8 is_swing_max,
		    u8 is_pre_max);
s32 dp_lane_par(u32 lane_cnt, u64 ls_clk, s32 *lane_cfg);
int cdiv(int a, int b);
void edp_irq_handler(void);

#endif /*End of file*/
