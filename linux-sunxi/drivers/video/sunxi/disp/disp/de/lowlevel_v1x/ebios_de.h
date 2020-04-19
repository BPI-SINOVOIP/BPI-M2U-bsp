/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EBIOS_DE_H__
#define __EBIOS_DE_H__

#include "../bsp_display.h"
#include "iep.h"
/*write back end interrupt */

#define DE_WB_END_IE			(1<<7)
#define DE_FE_IE_REG_LOAD_FINISH    10
/*front-end all interrupt enable*/

#define DE_FE_INTEN_ALL             0x1ff
#define DE_IMG_REG_LOAD_FINISH  (1<<1)
/* sync reach flag when capture in process */

#define SCAL_WB_ERR_SYNC (1<<15)
/* lose data flag when capture in process */

#define SCAL_WB_ERR_LOSEDATA (1<<14)
#define SCAL_WB_ERR_STATUS (1<<12)	/* unvalid write back */

/*layer framebuffer format enum definition*/

enum de_fbfmt_e {
	DE_MONO_1BPP = 0,
	DE_MONO_2BPP,
	DE_MONO_4BPP,
	DE_MONO_8BPP,
	DE_COLOR_RGB655,
	DE_COLOR_RGB565,
	DE_COLOR_RGB556,
	DE_COLOR_ARGB1555,
	DE_COLOR_RGBA5551,
	DE_COLOR_RGB0888,
	DE_COLOR_ARGB8888,
	DE_COLOR_RGB888,
	DE_COLOR_ARGB4444,

};

/*internal layer framebuffer format enum definition*/

enum de_inter_fbfmt_e {
	DE_IF1BPP = 0,
	DE_IF2BPP,
	DE_IF4BPP,
	DE_IF8BPP
};

enum de_hwc_mode_e {
	DE_H32_V32_8BPP,
	DE_H64_V64_2BPP,
	DE_H64_V32_4BPP,
	DE_H32_V64_4BPP
};

enum de_pixels_num_t {
	DE_N32PIXELS = 0,
	DE_N64PIXELS
};

enum __SCAL_PS {
	DE_SCAL_BGRA = 0,
	DE_SCAL_ARGB = 1,
	DE_SCAL_AYUV = 0,
	DE_SCAL_VUYA = 1,
	DE_SCAL_UVUV = 0,
	DE_SCAL_VUVU = 1,
	DE_SCAL_UYVY = 0,
	DE_SCAL_YUYV = 1,
	DE_SCAL_VYUY = 2,
	DE_SCAL_YVYU = 3,
	DE_SCAL_RGB565 = 0,
	DE_SCAL_BGR565 = 1,
	DE_SCAL_ARGB4444 = 0,
	DE_SCAL_BGRA4444 = 1,
	DE_SCAL_ARGB1555 = 0,
	DE_SCAL_BGRA5551 = 1
};

enum __SCAL_INMODE {
	DE_SCAL_PLANNAR = 0,
	DE_SCAL_INTERLEAVED,
	DE_SCAL_UVCOMBINED,
	DE_SCAL_PLANNARMB = 4,
	DE_SCAL_UVCOMBINEDMB = 6
};

enum __SCAL_INFMT {
	DE_SCAL_INYUV444 = 0,
	DE_SCAL_INYUV422,
	DE_SCAL_INYUV420,
	DE_SCAL_INYUV411,
	DE_SCAL_INRGB565,	/* new */
	DE_SCAL_INRGB888,
	DE_SCAL_INRGB4444,	/* new */
	DE_SCAL_INRGB1555	/* new */
};

enum __SCAL_OUTFMT {
	DE_SCAL_OUTPRGB888 = 0,
	DE_SCAL_OUTI0RGB888,
	DE_SCAL_OUTI1RGB888,
	DE_SCAL_OUTPYUV444 = 4,
	DE_SCAL_OUTPYUV420,
	DE_SCAL_OUTPYUV422,
	DE_SCAL_OUTPYUV411
};
/* for 3D inmod,  source mod must  */
/* be DE_SCAL_PLANNAR or DE_SCAL_UVCOMBINEDMB */
/* DE_SCAL_INTER_LEAVED and DE_SCAL_UVCOMBINED */
/* maybe supported in future==== */
enum __scal_3d_inmode_t {
	DE_SCAL_3DIN_TB = 0,
	DE_SCAL_3DIN_FP = 1,
	DE_SCAL_3DIN_SSF = 2,
	DE_SCAL_3DIN_SSH = 3,
	DE_SCAL_3DIN_LI = 4,
};

enum __scal_3d_outmode_t {
	DE_SCAL_3DOUT_CI_1 = 0,
	DE_SCAL_3DOUT_CI_2,
	DE_SCAL_3DOUT_CI_3,
	DE_SCAL_3DOUT_CI_4,
	DE_SCAL_3DOUT_LIRGB,
	DE_SCAL_3DOUT_HDMI_FPP,	/* for hdmi */
	DE_SCAL_3DOUT_HDMI_FPI,
	DE_SCAL_3DOUT_HDMI_TB,
	DE_SCAL_3DOUT_HDMI_FA,
	DE_SCAL_3DOUT_HDMI_SSH,
	DE_SCAL_3DOUT_HDMI_SSF,
	DE_SCAL_3DOUT_HDMI_LI,
};

struct layer_src_t {
	__u8 format;
	__u8 pixseq;
	__u8 br_swap;
	__u32 fb_width;
	__u32 fb_addr;
	__u32 offset_x;
	__u32 offset_y;

	bool yuv_ch;
	bool pre_multiply;
};
/*direct lcd pipe input source definition */

struct de_dlcdp_src_t {
	__u8 format;
	__u8 pixseq;
	__u32 fb_width;
	__u32 fb_addr;
	__u32 offset_x;
	__u32 offset_y;
};

struct de_hwc_src_t {
	__u8 mode;
	__u32 paddr;
};

struct de_yuv_ch_src_t {
	__u8 format;
	__u8 mode;
	__u8 pixseq;
	__u32 ch0_base;		/* in bits */
	__u32 ch1_base;		/* in bits */
	__u32 ch2_base;		/* in bits */
	__u32 line_width;	/* in bits */
	__u32 offset_x;
	__u32 offset_y;
	__u8 cs_mode;		/* 0:DISP_BT601; 1:DISP_BT709; */
	/*2:DISP_YCC; 3:DISP_VXYCC */
};

struct de_sprite_src_t {
	__u8 pixel_seq;		/* 0,1 */
	__u8 format;		/* 0:32bpp; 1:8bpp */
	__u32 offset_x;
	__u32 offset_y;
	__u32 fb_addr;
	__u32 fb_width;
};

struct __scal_src_type_t {
	/* for yuv420, 0: uv_hphase=-0.25, uv_vphase=-0.25; */
	/*other : uv_hphase = 0, uv_vphase = -0.25 */
	__u8 sample_method;
	/* 0:byte0,byte1, byte2, byte3; */
	/*1: byte3, byte2, byte1, byte0 */
	__u8 byte_seq;
	/* 0:plannar; 1: interleaved; 2: plannar */
	/*uv combined; 4: plannar mb; 6: uv combined mb */
	__u8 mod;
	/* 0:yuv444; 1: yuv422; 2: yuv420; */
	/*3:yuv411; 4: csi rgb; 5:rgb888 */
	__u8 fmt;
	__u8 ps;
	__u8 br_swap;
};

struct __scal_out_type_t {
	/* 0:byte0,byte1, byte2, byte3; */
	/*1: byte3, byte2, byte1, byte0 */
	__u8 byte_seq;
	/* 0:plannar rgb; 1: argb(byte0,byte1, byte2, byte3); */
	/*2:bgra; 4:yuv444; 5:yuv420; 6:yuv422; 7:yuv411 */
	__u8 fmt;
	/* output alpha channel enable, valid when rgb888fmt */

	bool alpha_en;
	__u8 alpha_coef_type;	/* 0:soft type;  1: sharp type */
};

struct __scal_src_size_t {
	__u32 src_width;
	__u32 src_height;
	__u32 x_off;
	__u32 y_off;
	__u32 scal_width;
	__u32 scal_height;
};

struct __scal_out_size_t {
	__u32 width;
	__u32 height;		/* when ouput interlace enable,  */
	/*the height is the 2x height of */
	/*scale, for example, */
	/*ouput is 480i, this value is 480 */
	__u32 x_off;
	__u32 y_off;
	__u32 fb_width;
	__u32 fb_height;
};

struct __scal_buf_addr_t {
	__u32 ch0_addr;		/*  */
	__u32 ch1_addr;
	__u32 ch2_addr;
};

struct __scal_scan_mod_t {
	__u8 field;		/* 0:frame scan; 1:field scan */
	__u8 bottom;		/* 0:top field; 1:bottom field */
};

__s32 DE_SCAL_Set_Reg_Base(__u32 sel, uintptr_t base);
__u32 DE_SCAL_Get_Reg_Base(__u8 sel);
__s32 DE_SCAL_Config_Src(__u8 sel,
			 struct __scal_buf_addr_t *addr,
			 struct __scal_src_size_t *size,
			 struct __scal_src_type_t *type, __u8 field, __u8 dien);
__s32 DE_SCAL_Set_Fb_Addr(__u8 sel, struct __scal_buf_addr_t *addr);
__s32 DE_SCAL_Set_Init_Phase(__u8 sel,
			     struct __scal_scan_mod_t *in_scan,
			     struct __scal_src_size_t *in_size,
			     struct __scal_src_type_t *in_type,
			     struct __scal_scan_mod_t *out_scan,
			     struct __scal_out_size_t *out_size,
			     struct __scal_out_type_t *out_type, __u8 dien);
__s32 DE_SCAL_Agth_Config(__u8 sel,
			  struct __scal_src_type_t *in_type,
			  struct __scal_src_size_t *in_size,
			  struct __scal_out_size_t *out_size,
			  __u8 dien, __u8 trden,
			  enum __scal_3d_outmode_t outmode);
__s32 DE_SCAL_Set_Scaling_Factor(__u8 sel,
				 struct __scal_scan_mod_t *in_scan,
				 struct __scal_src_size_t *in_size,
				 struct __scal_src_type_t *in_type,
				 struct __scal_scan_mod_t *out_scan,
				 struct __scal_out_size_t *out_size,
				 struct __scal_out_type_t *out_type);
__s32 DE_SCAL_Set_Scaling_Coef(__u8 sel,
			       struct __scal_scan_mod_t *in_scan,
			       struct __scal_src_size_t *in_size,
			       struct __scal_src_type_t *in_type,
			       struct __scal_scan_mod_t *out_scan,
			       struct __scal_out_size_t *out_size,
			       struct __scal_out_type_t *out_type,
			       __u8 smth_mode);
__s32 DE_SCAL_Set_Scaling_Coef_for_video(__u8 sel,
					 struct __scal_scan_mod_t *in_scan,
					 struct __scal_src_size_t *in_size,
					 struct __scal_src_type_t *in_type,
					 struct __scal_scan_mod_t *out_scan,
					 struct __scal_out_size_t *out_size,
					 struct __scal_out_type_t *out_type,
					 __u32 smth_mode);
__s32 DE_SCAL_Set_CSC_Coef(__u8 sel, __u8 in_csc_mode, __u8 out_csc_mode,
			   __u8 incs, __u8 outcs, __u32 in_br_swap,
			   __u32 out_br_swap);
__s32 DE_SCAL_Set_Out_Format(__u8 sel, struct __scal_out_type_t *out_type);
__s32 DE_SCAL_Set_Out_Size(__u8 sel,
			   struct __scal_scan_mod_t *out_scan,
			   struct __scal_out_type_t *out_type,
			   struct __scal_out_size_t *out_size);
__s32 DE_SCAL_Set_Trig_Line(__u8 sel, __u32 line);
__s32 DE_SCAL_Set_Int_En(__u8 sel, __u32 int_num);
__s32 DE_SCAL_Set_Di_Ctrl(__u8 sel, __u8 en,
			  __u8 mode, __u8 diagintp_en, __u8 tempdiff_en);
__s32 DE_SCAL_Set_Di_PreFrame_Addr(__u8 sel,
				   __u32 luma_addr, __u32 chroma_addr);
__s32 DE_SCAL_Set_Di_MafFlag_Src(__u8 sel,
				 __u32 cur_addr, __u32 pre_addr, __u32 stride);
__s32 DE_SCAL_Set_Filtercoef_Ready(__u8 sel);
__s32 DE_SCAL_Output_Select(__u8 sel, __u8 out);
__s32 DE_SCAL_Writeback_Enable(__u8 sel);
__s32 DE_SCAL_Writeback_Disable(__u8 sel);
__s32 DE_SCAL_Writeback_Linestride_Enable(__u8 sel);
__s32 DE_SCAL_Writeback_Linestride_Disable(__u8 sel);
__s32 DE_SCAL_Set_Writeback_Addr_ex(__u32 sel,
				    struct __scal_buf_addr_t *addr,
				    struct __scal_out_size_t *size,
				    struct __scal_out_type_t *type);
__s32 DE_SCAL_Set_Writeback_Addr(__u8 sel, struct __scal_buf_addr_t *addr);
__s32 DE_SCAL_Set_CSC_Coef_Enhance(__u8 sel,
				   __u8 in_csc_mode, __u8 out_csc_mode,
				   __u8 incs, __u8 outcs, __s32 bright,
				   __s32 contrast, __s32 saturaion, __s32 hue,
				   __u32 in_br_swap, __u32 out_br_swap);
__s32 DE_SCAL_Get_3D_In_Single_Size(enum __scal_3d_inmode_t inmode,
				    struct __scal_src_size_t *fullsize,
				    struct __scal_src_size_t *singlesize);
__s32 DE_SCAL_Get_3D_Out_Single_Size(enum __scal_3d_outmode_t outmode,
				     struct __scal_out_size_t *singlesize,
				     struct __scal_out_size_t *fullsize);
__s32 DE_SCAL_Get_3D_Out_Full_Size(enum __scal_3d_outmode_t outmode,
				   struct __scal_out_size_t *singlesize,
				   struct __scal_out_size_t *fullsize);
__s32 DE_SCAL_Set_3D_Fb_Addr(__u8 sel, struct __scal_buf_addr_t *addr,
			     struct __scal_buf_addr_t *addrtrd);
__s32 DE_SCAL_Set_3D_Di_PreFrame_Addr(__u8 sel, struct __scal_buf_addr_t *addr,
				      struct __scal_buf_addr_t *addrtrd);
__s32 DE_SCAL_Set_3D_Ctrl(__u8 sel, __u8 trden, enum __scal_3d_inmode_t inmode,
			  enum __scal_3d_outmode_t outmode);
__s32 DE_SCAL_Config_3D_Src(__u8 sel, struct __scal_buf_addr_t *addr,
			    struct __scal_src_size_t *size,
			    struct __scal_src_type_t *type,
			    enum __scal_3d_inmode_t trdinmode,
			    struct __scal_buf_addr_t *addrtrd);
__s32 DE_SCAL_Input_Port_Select(__u8 sel, __u8 port);

__s32 DE_SCAL_Vpp_Enable(__u8 sel, __u32 enable);
__s32 DE_SCAL_Vpp_Set_Luma_Sharpness_Level(__u8 sel, __u32 level);
__s32 DE_SCAL_Vpp_Set_Chroma_Sharpness_Level(__u8 sel, __u32 level);
__s32 DE_SCAL_Vpp_Set_White_Level_Extension(__u8 sel, __u32 level);
__s32 DE_SCAL_Vpp_Set_Black_Level_Extension(__u8 sel, __u32 level);
__s32 DE_SCAL_Reset(__u8 sel);
__s32 DE_SCAL_Start(__u8 sel);
__s32 DE_SCAL_Set_Reg_Rdy(__u8 sel);
__s32 DE_SCAL_Enable(__u8 sel);
__s32 DE_SCAL_Disable(__u8 sel);
__s32 DE_SCAL_Get_Field_Status(__u8 sel);
__s32 DE_SCAL_EnableINT(__u8 sel, __u32 irqsrc);
__s32 DE_SCAL_DisableINT(__u8 sel, __u32 irqsrc);
__u32 DE_SCAL_QueryINT(__u8 sel);
__u32 DE_SCAL_ClearINT(__u8 sel, __u32 irqsrc);
__s32 DE_SCAL_Input_Select(__u8 sel, __u32 source);
__u8 DE_SCAL_Get_Input_Format(__u8 sel);
__u8 DE_SCAL_Get_Input_Mode(__u8 sel);
__u8 DE_SCAL_Get_Output_Format(__u8 sel);
__u16 DE_SCAL_Get_Input_Width(__u8 sel);
__u16 DE_SCAL_Get_Input_Height(__u8 sel);
__u16 DE_SCAL_Get_Output_Width(__u8 sel);
__u16 DE_SCAL_Get_Output_Height(__u8 sel);
__s32 DE_SCAL_Get_Start_Status(__u8 sel);

__s32 DE_Set_Reg_Base(__u32 sel, uintptr_t address);
__u32 DE_Get_Reg_Base(__u32 sel);
__u32 DE_BE_Reg_Init(__u32 sel);
__s32 DE_BE_Enable(__u32 sel);
__s32 DE_BE_Disable(__u32 sel);
__s32 DE_BE_Output_Select(__u32 sel, __u32 out_sel);
__s32 DE_BE_Set_BkColor(__u32 sel, disp_color_info bkcolor);
__s32 DE_BE_Set_ColorKey(__u32 sel, disp_color_info ck_max,
			 disp_color_info ck_min, __u32 ck_red_match,
			 __u32 ck_green_match, __u32 ck_blue_match);
__s32 DE_BE_Set_SystemPalette(__u32 sel, __u32 *pbuffer,
			      __u32 offset, __u32 size);
__s32 DE_BE_Get_SystemPalette(__u32 sel, __u32 *pbuffer,
			      __u32 offset, __u32 size);
__s32 DE_BE_Cfg_Ready(__u32 sel);
__s32 DE_BE_EnableINT(__u8 sel, __u32 irqsrc);
__s32 DE_BE_DisableINT(__u8 sel, __u32 irqsrc);
__u32 DE_BE_QueryINT(__u8 sel);
__u32 DE_BE_ClearINT(__u8 sel, __u32 irqsrc);
__s32 DE_BE_reg_auto_load_en(__u32 sel, __u32 en);

__s32 DE_BE_Layer_Enable(__u32 sel, __u8 layidx, bool enable);
__s32 DE_BE_Layer_Set_Format(__u32 sel,
			     __u8 layidx, __u8 format, bool br_swap, __u8 order,
			     bool pre_multiply);
__s32 DE_BE_Layer_Set_Framebuffer(__u32 sel,
				  __u8 layidx, struct layer_src_t *layer_fb);
__s32 DE_BE_Layer_Set_Screen_Win(__u32 sel, __u8 layidx, disp_window *win);
__s32 DE_BE_Layer_Video_Enable(__u32 sel, __u8 layidx, bool video_en);
__s32 DE_BE_Layer_Video_Ch_Sel(__u32 sel, __u8 layidx, bool scaler_index);
__s32 DE_BE_Layer_Yuv_Ch_Enable(__u32 sel, __u8 layidx, bool yuv_en);
__s32 DE_BE_Layer_Set_Prio(__u32 sel, __u8 layidx, __u8 prio);
__s32 DE_BE_Layer_Set_Pipe(__u32 sel, __u8 layidx, __u8 pipe);
__s32 DE_BE_Layer_Alpha_Enable(__u32 sel, __u8 layidx, bool enable);
__s32 DE_BE_Layer_Set_Alpha_Value(__u32 sel, __u8 layidx, __u8 alpha_val);
__s32 DE_BE_Layer_ColorKey_Enable(__u32 sel, __u8 layidx, bool enable);
__s32 DE_BE_Layer_Set_Work_Mode(__u32 sel, __u8 layidx, __u8 mode);

__s32 DE_BE_YUV_CH_Enable(__u32 sel, bool enable);
__s32 DE_BE_YUV_CH_Set_Src(__u32 sel, struct de_yuv_ch_src_t *in_src);

__s32 DE_BE_HWC_Enable(__u32 sel, bool enable);
__s32 DE_BE_HWC_Set_Pos(__u32 sel, disp_position *pos);
__s32 DE_BE_HWC_Get_Pos(__u32 sel, disp_position *pos);
__s32 DE_BE_HWC_Set_Palette(__u32 sel, __u32 address, __u32 offset, __u32 size);
__s32 DE_BE_HWC_Get_Format(void);
__s32 DE_BE_HWC_Set_Src(__u32 sel, struct de_hwc_src_t *hwc_pat);

__s32 DE_BE_Sprite_Enable(__u32 sel, bool enable);
__s32 DE_BE_Sprite_Disable(__u32 sel);
__s32 DE_BE_Sprite_Set_Format(__u32 sel, __u8 pixel_seq, __u8 format);
__s32 DE_BE_Sprite_Global_Alpha_Enable(__u32 sel, bool enable);
__s32 DE_BE_Sprite_Set_Global_Alpha(__u32 sel, __u8 alpha_val);
__s32 DE_BE_Sprite_Block_Set_Pos(__u32 sel, __u8 blk_idx, __s16 x, __s16 y);
__s32 DE_BE_Sprite_Block_Set_Size(__u32 sel,
				  __u8 blk_idx, __u32 xsize, __u32 ysize);
__s32 DE_BE_Sprite_Block_Set_fb(__u32 sel,
				__u8 blk_idx, __u32 addr, __u32 line_width);
__s32 DE_BE_Sprite_Block_Set_Next_Id(__u32 sel, __u8 blk_idx, __u8 next_blk_id);
__s32 DE_BE_Sprite_Set_Palette_Table(__u32 sel,
				     __u32 address, __u32 offset, __u32 size);
__s32 DE_BE_Set_Enhance(__u8 sel, __u32 brightness,
			__u32 contrast, __u32 saturaion, __u32 hue);
__s32 DE_BE_Set_Enhance_ex(__u8 sel, __u32 out_csc,
			   __u32 out_color_range, __u32 enhance_en,
			   __u32 brightness, __u32 contrast, __u32 saturaion,
			   __u32 hue);
__s32 DE_BE_enhance_enable(__u32 sel, bool enable);
__s32 DE_BE_set_display_size(__u32 sel, __u32 width, __u32 height);
__s32 DE_BE_get_display_width(__u32 sel);
__s32 DE_BE_get_display_height(__u32 sel);
__s32 DE_BE_deflicker_enable(__u32 sel, bool enable);
__s32 DE_BE_Set_Outitl_enable(__u32 sel, bool enable);
__s32 DE_BE_Format_To_Bpp(__u32 sel, __u8 format);
__u32 DE_BE_Offset_To_Addr(__u32 src_addr, __u32 width,
			   __u32 x, __u32 y, __u32 bpp);
__u32 DE_BE_Addr_To_Offset(__u32 src_addr, __u32 off_addr,
			   __u32 width, __u32 bpp, disp_position *pos);

#endif				/* __EBIOS_DE_H__ */
