/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __IEP_DRC_EBIOS_H__
#define __IEP_DRC_EBIOS_H__

#include "ebios_de.h"

#define IEP_DRC_SPA_TAB_LEN 9
#define IEP_DRC_INT_TAB_LEN 256
#define IEP_DRC_LGC_TAB_LEN 256
#define IEP_LH_INTERVAL_NUM 8
#define IEP_LH_THRES_NUM    7

union __imgehc_gnectl_reg_t {
	u32 dwval;
	struct {
		u32 en:1;	/* bit0 */
		u32 r0:7;	/* bit1~7 */
		u32 mod:2;	/* bit8~9 */
		u32 r1:6;	/* bit10~15 */
		u32 sync_edge:1;	/* bit16 */
		u32 field_parity:1;	/* bit17 */
		u32 r2:13;	/* bit18~30 */
		u32 bist_en:1;	/* bit31 */
	} bits;
};				/* 0x0 */

union __imgehc_drcsize_reg_t {
	u32 dwval;
	struct {
		u32 disp_w:12;	/* bit0~11 */
		u32 r0:4;	/* bit12~15 */
		u32 disp_h:12;	/* bit16~27 */
		u32 r1:4;	/* bit31~28 */
	} bits;
};

union __imgehc_drcctl_reg_t {
	u32 dwval;
	struct {
		u32 db_en:1;	/* bit0 */
		u32 dbrdy_ctl:1;	/* bit1 */
		u32 r0:6;	/* bit2~7 */
		u32 win_en:1;	/* bit8 */
		u32 hsv_mode_en:1;	/* bit9 */
		u32 r1:22;	/* bit10~31 */
	} bits;
};				/* 0x10 */

union __imgehc_drclgc_staadd_reg_t {
	u32 dwval;
	struct {
		u32 lgc_addr:32;	/* bit0~31 */
	} bits;
};				/* 0x14 */

union __imgehc_drc_set_reg_t {
	u32 dwval;
	struct {
		u32 lgc_abslumshf:1;	/* bit0 */
		u32 adjust_en:1;	/* bit1 */
		u32 r0:6;	/* bit2~7 */
		u32 lgc_abslumperval:8;	/* bit8~15 */
		u32 r1:8;	/* bit16~23 */
		u32 gain_autoload_dis:1;	/* bit24 */
		u32 r2:7;	/* bit25~31 */
	} bits;
};				/* 0x18 */

union __imgehc_drc_wp_reg0_t {
	u32 dwval;
	struct {
		u32 win_left:12;	/* bit0~11 */
		u32 r0:4;	/* bit12~15 */
		u32 win_top:12;	/* bit16~27 */
		u32 r1:4;	/* bit28~31 */
	} bits;
};				/* 0x1c */

union __imgehc_drc_wp_reg1_t {
	u32 dwval;
	struct {
		u32 win_right:12;	/* bit0~11 */
		u32 r0:4;	/* bit12~15 */
		u32 win_bottom:12;	/* bit16~27 */
		u32 r1:4;	/* bit28~31 */
	} bits;
};				/* 0x20 */

union __imgehc_wbctl_reg_t {
	u32 dwval;
	struct {
		u32 wb_en:1;	/* bit0 */
		u32 r0:7;	/* bit1~7 */
		u32 wb_mode:1;	/* bit8 */
		u32 r1:7;	/* bit9~15 */
		u32 wb_ps:1;	/* bit16 */
		u32 r2:7;	/* bit17~23 */
		u32 field:1;	/* bit24 */
		u32 r3:6;	/* bit25~30 */
		u32 wb_sts:1;	/* bit31 */
	} bits;
};				/* 0x24 */

union __imgehc_wbaddr_reg_t {
	u32 dwval;
	struct {
		u32 wb_addr:32;
	} bits;
};				/* 0x28 */

union __imgehc_wbline_reg_t {
	u32 dwval;
	struct {
		u32 linestride:32;
	} bits;
};				/* 0x2c */

union __imgehc_lhctl_reg_t {
	u32 dwval;
	struct {
		u32 lh_rec_clr:1;	/* bit0 */
		u32 lh_mod:1;	/* bit1 */
		u32 r0:30;	/* bit2~31 */
	} bits;
};				/* 0x30 */

union __imgehc_lhthr_reg0_t {
	u32 dwval;
	struct {
		u32 lh_thres_val1:8;	/* bit0~7 */
		u32 lh_thres_val2:8;	/* bit8~15 */
		u32 lh_thres_val3:8;	/* bit16~23 */
		u32 lh_thres_val4:8;	/* bit24~31 */
	} bits;
};				/* 0x34 */

union __imgehc_lhthr_reg1_t {
	u32 dwval;
	struct {
		u32 lh_thres_val5:8;	/* bit0~7 */
		u32 lh_thres_val6:8;	/* bit8~15 */
		u32 lh_thres_val7:8;	/* bit16~23 */
		u32 r0:8;	/* bit24~31 */
	} bits;
};				/* 0x38 */

union __imgehc_lhslum_reg_t {
	u32 dwval;
	struct {
		u32 lh_lum_data:32;	/* bit0~31 */
	} bits;
};				/* 0x0040 ~ 0x005c */

union __imgehc_lhscnt_reg_t {
	u32 dwval;
	struct {
		u32 lh_cnt_data:32;	/* bit0~31 */
	} bits;
};				/* 0x0060 ~ 0x007c */

union __imgehc_extern_drclgc_staadd_reg_t {
	u32 dwval;
	struct {
		u32 lgc_addr:32;	/* bit0~31 */
	} bits;
};				/* 0x84 */

union __imgehc_extern_high_drclgc_staadd_reg_t {
	u32 dwval;
	struct {
		u32 lgc_addr:2;	/* bit0~1 */
		u32 r0:30;	/* bit2~31 */
	} bits;
};				/* 0x88 */

union __imgehc_cscygcoff_reg_t {
	u32 dwval;
	struct {
		u32 csc_yg_coff:13;	/* bit0~12 */
		u32 r0:19;	/* bit13~31 */
	} bits;
};				/* 0xc0~0xc8 */

union __imgehc_cscygcon_reg_t {
	u32 dwval;
	struct {
		u32 csc_yg_con:14;	/* bit0~13 */
		u32 r0:18;	/* bit14~31 */
	} bits;
};				/* 0xcc */

union __imgehc_cscurcoff_reg_t {
	u32 dwval;
	struct {
		u32 csc_ur_coff:13;	/* bit0~12 */
		u32 r0:19;	/* bit13~31 */
	} bits;
};				/* 0xd0~0xd8 */

union __imgehc_cscurcon_reg_t {
	u32 dwval;
	struct {
		u32 csc_ur_con:14;	/* bit0~13 */
		u32 r0:18;	/* bit14~31 */
	} bits;
};				/* 0xdc */

union __imgehc_cscvbcoff_reg_t {
	u32 dwval;
	struct {
		u32 csc_vb_coff:13;	/* bit0~12 */
		u32 r0:19;	/* bit13~31 */
	} bits;
};				/* 0xe0~0xe8 */

union __imgehc_cscvbcon_reg_t {
	u32 dwval;
	struct {
		u32 csc_vb_con:14;	/* bit0~13 */
		u32 r0:18;	/* bit14~31 */
	} bits;
};				/* 0xec */

union __imgehc_drcspacoff_reg_t {
	u32 dwval;
	struct {
		u32 spa_coff0:8;	/* bit0~7 */
		u32 spa_coff1:8;	/* bit8~15 */
		u32 spa_coff2:8;	/* bit16~23 */
		u32 r0:8;	/* bit24~31 */
	} bits;
};				/* 0xf0~0xf8 */

union __imgehc_drcintcoff_reg_t {
	u32 dwval;
	struct {
		u32 inten_coff0:8;	/* bit0~7 */
		u32 inten_coff1:8;	/* bit8~15 */
		u32 inten_coff2:8;	/* bit16~23 */
		u32 inten_coff3:8;	/* bit24~31 */
	} bits;
};				/* 0x0100 ~ 0x01fc */

union __imgehc_drclgcoff_reg_t {
	u32 dwval;
	struct {
		u32 lumagain_coff0:16;	/* bit0~15 */
		u32 lumagain_coff1:16;	/* bit16~31 */
	} bits;
};				/* 0x0200 ~ 0x03fc */

struct __iep_drc_dev_t {
	union __imgehc_gnectl_reg_t gnectl;	/* 0x00 */
	union __imgehc_drcsize_reg_t drcsize;	/* 0x04 */
	u32 r0[2];		/* 0x08~0x0c */
	union __imgehc_drcctl_reg_t drcctl;	/* 0x10 */
	union __imgehc_drclgc_staadd_reg_t drclgc_addr;	/* 0x14 */
	union __imgehc_drc_set_reg_t drc_set;	/* 0x18 */
	union __imgehc_drc_wp_reg0_t drc_wp0;	/* 0x1c */
	union __imgehc_drc_wp_reg1_t drc_wp1;	/* 0x20 */
	union __imgehc_wbctl_reg_t wbctl;	/* 0x24 */
	union __imgehc_wbaddr_reg_t wbaddr;	/* 0x28 */
	union __imgehc_wbline_reg_t wbline;	/* 0x2c */
	union __imgehc_lhctl_reg_t lhctl;	/* 0x30 */
	union __imgehc_lhthr_reg0_t lhthr0;	/* 0x34 */
	union __imgehc_lhthr_reg1_t lhthr1;	/* 0x38 */
	u32 r2;			/* 0x3c */
	union __imgehc_lhslum_reg_t lhslum[8];	/* 0x40~0x5c */
	/* 0x0060 ~ 0x007c */
	union __imgehc_lhscnt_reg_t lhscnt[8];
	u32 r3;			/* 0x80 */
	/* 0x84 */
	union __imgehc_extern_drclgc_staadd_reg_t ext_drclgc_addr;
	/* 0x88 */
	union __imgehc_extern_high_drclgc_staadd_reg_t
	 exthigh_drclgc_addr;
	u32 r4[13];		/* 0x8c~0xbc */
	/* 0xc0~0xc8 */
	union __imgehc_cscygcoff_reg_t cscygcoff[3];
	union __imgehc_cscygcon_reg_t cscygcon;	/* 0xcc */
	/* 0xd0~0xd8 */
	union __imgehc_cscurcoff_reg_t cscurcoff[3];
	union __imgehc_cscurcon_reg_t cscurcon;	/* 0xdc */
	/* 0xe0~0xe8 */
	union __imgehc_cscvbcoff_reg_t cscvbcoff[3];
	union __imgehc_cscvbcon_reg_t cscvbcon;	/* 0xec */
	/* 0xf0~0xf8 */
	union __imgehc_drcspacoff_reg_t drcspacoff[3];
	u32 r5;			/* 0xff */
	/* 0x0100 ~ 0x01fc */
	union __imgehc_drcintcoff_reg_t drcintcoff[64];
	/* 0x0200 ~ 0x03fc */
	union __imgehc_drclgcoff_reg_t drclgcoff[128];
};

#define ____SEPARATOR_GLOBAL____
s32 DRC_EBIOS_Set_Reg_Base(u32 sel, uintptr_t base);
u32 DRC_EBIOS_Get_Reg_Base(u32 sel);
s32 DRC_EBIOS_Enable(u32 sel);
s32 DRC_EBIOS_Disable(u32 sel);
s32 DRC_EBIOS_Set_Mode(u32 sel, u32 mod);
s32 DRC_EBIOS_Bist_Enable(u32 sel, u32 en);
s32 DRC_EBIOS_Set_Reg_Refresh_Edge(u32 sel, u32 falling);
s32 DRC_EBIOS_Set_Csc_Coeff(u32 sel, u32 mod);
s32 DRC_EBIOS_Set_Display_Size(u32 sel, u32 width, u32 height);
s32 DRC_EBIOS_Win_Enable(u32 sel, u32 en);
u32 DRC_EBIOS_Set_Win_Para(u32 sel, u32 top, u32 bot, u32 left, u32 right);
#define ____SEPARATOR_DRC____
s32 DRC_EBIOS_Drc_Cfg_Rdy(u32 sel);
u32 DRC_EBIOS_Drc_Set_Lgc_Addr(u32 sel, u32 addr);
u32 DRC_EBIOS_Drc_Set_Lgc_Autoload_Disable(u32 sel, u32 disable);
u32 DRC_EBIOS_Drc_Adjust_Enable(u32 sel, u32 en);
u32 DRC_EBIOS_Drc_Set_Adjust_Para(u32 sel, u32 abslumperval, u32 abslumshf);
u32 DRC_EBIOS_Drc_Set_Spa_Coeff(u32 sel, u8 spatab[IEP_DRC_SPA_TAB_LEN]);
u32 DRC_EBIOS_Drc_Set_Int_Coeff(u32 sel, u8 inttab[IEP_DRC_INT_TAB_LEN]);
u32 DRC_EBIOS_Drc_Set_Mode(u32 sel, u32 mode);
#define ____SEPARATOR_LH____
u32 DRC_EBIOS_Lh_Set_Mode(u32 sel, u32 mod);
u32 DRC_EBIOS_Lh_Clr_Rec(u32 sel);
u32 DRC_EBIOS_Lh_Set_Thres(u32 sel, u8 thres[IEP_LH_THRES_NUM]);
u32 DRC_EBIOS_Lh_Get_Sum_Rec(u32 sel, u32 *sum);
u32 DRC_EBIOS_Lh_Get_Cnt_Rec(u32 sel, u32 *cnt);
#endif
