/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "sl698ph_720p.h"
#include "panels.h"

static void LCD_power_on(__u32 sel);
static void LCD_power_off(__u32 sel);
static void LCD_bl_open(__u32 sel);
static void LCD_bl_close(__u32 sel);

static void LCD_panel_init(__u32 sel);
static void LCD_panel_exit(__u32 sel);

static void LCD_cfg_panel_info(panel_extend_para *info)
{
	__u32 i = 0, j = 0;
	__u32 items;
	__u8 lcd_gamma_tbl[][2] = {

		/* {input value, corrected value} */
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	__u32 lcd_cmap_tbl[2][3][4] = {
		{
		 {LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		 {LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		 {LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		 },
		{
		 {LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		 {LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		 {LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		 },
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		__u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			__u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] -
			      lcd_gamma_tbl[i][1]) * j) / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
	    (lcd_gamma_tbl[items - 1][1] << 8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static __s32 LCD_open_flow(__u32 sel)
{
	/* open lcd power, and delay 50ms */

	LCD_OPEN_FUNC(sel, LCD_power_on, 30);
	/* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);
	/* open lcd controller, and delay 100ms */
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 100);
	/* open lcd backlight, and delay 0ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);

	return 0;
}

static __s32 LCD_close_flow(__u32 sel)
{
	/* close lcd backlight, and delay 0ms */

	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);
	/* close lcd controller, and delay 0ms */
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	/* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 20);
	/* close lcd power, and delay 500ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off, 50);

	return 0;
}

static void LCD_power_on(__u32 sel)
{
	/* config lcd_power pin to open lcd power0 */

	sunxi_lcd_power_enable(sel, 0);
	/* config lcd_power pin to open lcd power1 */
	sunxi_lcd_power_enable(sel, 1);
	/* config lcd_power pin to open lcd power2 */
	sunxi_lcd_power_enable(sel, 2);
}

static void LCD_power_off(__u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	/* config lcd_power pin to close lcd power0 */
	sunxi_lcd_power_disable(sel, 0);
	/* config lcd_power pin to close lcd power1 */
	sunxi_lcd_power_disable(sel, 1);
	/* config lcd_power pin to close lcd power2 */
	sunxi_lcd_power_disable(sel, 2);
}

static void LCD_bl_open(__u32 sel)
{
	sunxi_lcd_pwm_enable(sel);	/* open pwm module */
	/* config lcd_bl_en pin to open lcd backlight */
	sunxi_lcd_backlight_enable(sel);
}

static void LCD_bl_close(__u32 sel)
{
	/* config lcd_bl_en pin to close lcd backlight */

	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);	/* close pwm module */
}

#define REGFLAG_DELAY			0X01
/* END OF REGISTERS MARKER */

#define REGFLAG_END_OF_TABLE	0x02

struct LCM_setting_table {
	__u8 cmd;
	__u32 count;
	__u8 para_list[64];
};

#if 0
static struct LCM_setting_table otm1283_test_config_para[] = {
	{0x00, 1, 0X00},
	{0xff, 3, {0x12, 0x83, 0x01} },
	{0x00, 1, 0x80},
	{0xff, 2, {0x12, 0x83} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};
#endif

static struct LCM_setting_table otm1283_initialization_setting[] = {
	{0x00, 1, {0x00} },
	{0xFF, 3, {0x12, 0x83, 0x01} },
	{0x00, 1, {0x80} },
	{0xFF, 2, {0x12, 0x83} },
	{0x00, 1, {0x80} },
	{0xC0, 9, {0x00, 0x64, 0x00, 0x10, 0x10, 0x00,
		   0x64, 0x10, 0x10} },
	{0x00, 1, {0x90} },
	{0xC0, 6, {0x00, 0x56, 0x00, 0x01, 0x00, 0x04} },
	{0x00, 1, {0xA4} },
	{0xC0, 1, {0x16} },
	{0x00, 1, {0xB3} },
	{0xC0, 2, {0x00, 0x50} },
	{0x00, 1, {0x81} },
	{0xC1, 1, {0x66} },
	{0x00, 1, {0x90} },
	{0xC4, 1, {0x49} },
	{0x00, 1, {0xA0} },
	{0xC4, 14, {0x05, 0x10, 0x06, 0x02, 0x05, 0x15,
		    0x10, 0x05, 0x10, 0x07, 0x02, 0x05, 0x15, 0x10} },
	{0x00, 1, {0xB0} },
	{0xC4, 2, {0x00, 0x00} },
	{0x00, 1, {0x91} },
	{0xC5, 3, {0x46, 0x40, 0x00} },
	{0x00, 1, {0x00} },
	{0xD8, 2, {0xCF, 0xCF} },
	{0x00, 1, {0x00} },
	{0xd9, 1, {0x47} },
	{0x00, 1, {0x81} },
	{0xC4, 1, {0x82} },
	{0x00, 1, {0xB0} },
	{0xC5, 2, {0x04, 0xB8} },
	{0x00, 1, {0xBB} },
	{0xC5, 1, {0x80} },
	{0x00, 1, {0x82} },
	{0xC4, 1, {0x02} },
	{0x00, 1, {0xC6} },
	{0xB0, 1, {0x03} },
	{0x00, 1, {0x00} },
	{0xD0, 1, {0x40} },
	{0x00, 1, {0x00} },
	{0xD1, 2, {0x00, 0x00} },
	{0x00, 1, {0x80} },
	{0xCB, 11, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0x90} },
	{0xCB, 15, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00} },
	{0x00, 1, {0xA0} },
	{0xCB, 15, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00} },
	{0x00, 1, {0xB0} },
	{0xCB, 15, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00} },
	{0x00, 1, {0xC0} },
	{0xCB, 15, {0x05, 0x05, 0x05, 0x05, 0x05,
		    0x05, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x00,
		    0x05, 0x05} },
	{0x00, 1, {0xD0} },
	{0xCB, 15, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
		    0x00, 0x00} },
	{0x00, 1, {0xE0} },
	{0xCB, 14, {0x00, 0x00, 0x05, 0x05, 0x00,
		    0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0xF0} },
	{0xCB, 11, {0xff, 0xff, 0xff, 0xff, 0xff,
		    0xff, 0xff, 0xff, 0xff, 0xff, 0xff} },
	{0x00, 1, {0x80} },
	{0xCC, 15, {0x0E, 0x10, 0x0A, 0x0C, 0x02,
		    0x04, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x2D, 0x00,
		    0x29, 0x2A} },
	{0x00, 1, {0x90} },
	{0xCC, 15, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x0D, 0x0F, 0x09, 0x0B, 0x01, 0x03, 0x00,
		    0x00} },
	{0x00, 1, {0xA0} },
	{0xCC, 14, {0x00, 0x00, 0x2E, 0x2D, 0x00,
		    0x29, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0xB0} },
	{0xCC, 15, {0x0B, 0x09, 0x0F, 0x0D, 0x03,
		    0x01, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x2E, 0x00, 0x29,
		    0x2A} },
	{0x00, 1, {0xC0} },
	{0xCC, 15, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x0C, 0x0A, 0x10, 0x0E, 0x04, 0x02, 0x00,
		    0X00} },
	{0x00, 1, {0xD0} },
	{0xCC, 14, {0x00, 0x00, 0x2D, 0x2E, 0x00,
		    0x29, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0x80} },
	{0xCE, 12, {0x8B, 0x03, 0x18, 0x8A, 0x03,
		    0x18, 0x89, 0x03, 0x18, 0x88, 0x03, 0x18} },
	{0x00, 1, {0x90} },
	{0xCE, 14, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0xA0} },
	{0xCE, 14, {0x38, 0x07, 0x05, 0x00, 0x00,
		    0x18, 0x00, 0x38, 0x06, 0x05, 0x01, 0x00, 0x18, 0x00} },
	{0x00, 1, {0xB0} },
	{0xCE, 14, {0x38, 0x05, 0x05, 0x02, 0x00,
		    0x18, 0x00, 0x38, 0x04, 0x05, 0x03, 0x00, 0x18, 0x00} },
	{0x00, 1, {0xC0} },
	{0xCE, 14, {0x38, 0x03, 0x05, 0x04, 0x00,
		    0x18, 0x00, 0x38, 0x02, 0x05, 0x05, 0x00, 0x18, 0x00} },
	{0x00, 1, {0xD0} },
	{0xCE, 14, {0x38, 0x01, 0x05, 0x06, 0x00,
		    0x18, 0x00, 0x38, 0x00, 0x05, 0x07, 0x00, 0x18, 0x00} },
	{0x00, 1, {0x80} },
	{0xCF, 14, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0x90} },
	{0xCF, 14, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0xA0} },
	{0xCF, 14, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0xB0} },
	{0xCF, 14, {0x00, 0x00, 0x00, 0x00, 0x00,
		    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
	{0x00, 1, {0xC0} },
	{0xCF, 11, {0x3D, 0x02, 0x15, 0x20, 0x00,
		    0x00, 0x01, 0x81, 0x00, 0x03, 0x08} },
	{0x00, 1, {0xB5} },
	{0xC5, 6, {0x00, 0x6F, 0xFF, 0x00, 0x6F, 0xFF} },
	{0x00, 1, {0x90} },
	{0xF5, 4, {0x02, 0x11, 0x02, 0x11} },
	{0x00, 1, {0x90} },
	{0xC5, 1, {0x50} },
	{0x00, 1, {0x94} },
	{0xC5, 1, {0x66} },
	{0x00, 1, {0xB2} },
	{0xF5, 2, {0x00, 0x00} },
	{0x00, 1, {0xB4} },
	{0xF5, 2, {0x00, 0x00} },
	{0x00, 1, {0xB6} },
	{0xF5, 2, {0x00, 0x00} },
	{0x00, 1, {0xB8} },
	{0xF5, 2, {0x00, 0x00} },
	{0x00, 1, {0x94} },
	{0xF5, 1, {0x02} },
	{0x00, 1, {0xBA} },
	{0xF5, 1, {0x03} },
	{0x00, 1, {0xB4} },
	{0xC5, 1, {0xC0} },
	{0x00, 1, {0x00} },
	{0xE1, 16, {0x00, 0x0C, 0x12, 0x0F, 0x08,
		    0x10, 0x0b, 0x0A, 0x04, 0x07, 0x0D, 0x07, 0x0E,
		    0x15, 0x10, 0x03} },
	{0x00, 1, {0x00} },
	{0xE2, 16, {0x00, 0x0C, 0x12, 0x0E, 0x07,
		    0x10, 0x0b, 0x09, 0x04, 0x07, 0x0D, 0x07, 0x0E,
		    0x15, 0x10, 0x03} },
	{0x00, 1, {0xB4} },
	{0xC0, 1, {0x55} },

	/* necessary start */
	{0x00, 1, {0x80} },
	{0xC4, 1, {0x30} },
	{REGFLAG_DELAY, 10, {} },

	{0x00, 1, {0x8b} },
	{0xC4, 1, {0x00} },
	{REGFLAG_DELAY, 10, {} },

	{0x00, 1, {0x90} },
	{0xC4, 1, {0x52} },

	{0x00, 1, {0xb9} },
	{0xb0, 1, {0x51} },

	{0x00, 1, {0x82} },
	{0xC4, 1, {0x02} },

	{0x00, 1, {0xA2} },
	{0xC1, 1, {0x08} },
	{0x00, 1, {0xA4} },
	{0xC1, 1, {0xF0} },

	{0x00, 1, {0xC3} },
	{0xF5, 1, {0x81} },

	{0x00, 1, {0xB1} },
	{0xC6, 1, {0x05} },

	{0x00, 1, {0xA4} },
	{0xC0, 1, {0x00} },

	{0x00, 1, {0x87} },
	{0xC4, 1, {0x18} },

	{0x00, 1, {0xC2} },
	{0xF5, 1, {0xC0} },

	{0x00, 1, {0x80} },
	{0xC4, 1, {0x01} },

	{0x00, 1, {0x88} },
	{0xC4, 1, {0x80} },
	/* necessary end */

	/* {0x00,         1,  {0xA0}}, */
	/* {0xC1,         1,  {0x02}}, */
	/* {0x00,         1,  {0x81}}, */
	/* {0xC1,         1,  {0x07}}, */

	/* {0x00,         1,  {0xb5}},//modify @20140423 */
	/* {0xC0,         1,  {0x48}},//modify @20140423 */

	{0x00, 1, {0x00} },
	{0xff, 3, {0xff, 0xff, 0xff} },
	{REGFLAG_DELAY, 20, {} },
	{0x00, 1, {0x00} },
	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },
	{0x00, 1, {0x00} },
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 100, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table hx8394d_test_config_para[] = {
	{0xB9, 3, {0xFF, 0x83, 0x94} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table hx8394d_initialization_setting[] = {
#if 0
	{0xB9, 3, {0XFF, 0X83, 0X94} },
	{0xBA, 2, {0X33, 0X83} },
	{0xB0, 4, {0X00, 0X00, 0X7D, 0X0C} },
	{0xB1, 15, {0X6C, 0X15, 0X15, 0X24, 0X04, 0X11, 0XF1,
		    0X80, 0XE4, 0X97, 0X23, 0X80, 0XC0, 0XD2, 0X58} },
	{0xB2, 11, {0X00, 0X64, 0X10, 0X07, 0X22, 0X1C, 0X08,
		    0X08, 0X1C, 0X4D, 0X00} },
	{0xB4, 12, {0X00, 0XFF, 0X03, 0X70, 0X03, 0X70, 0X03,
		    0X70, 0X01, 0X74, 0X01, 0X74} },
	{0xBF, 3, {0X41, 0X0E, 0X01} },
	{0xD3, 37, {0X00, 0X06, 0X00, 0X40, 0X07, 0X08, 0X00,
		    0X32, 0X10, 0X07, 0X00, 0X07, 0X54, 0X15, 0X0F, 0X05,
		    0X04, 0X02, 0X12, 0X10, 0X05, 0X07, 0X33, 0X33, 0X0B,
		    0X0B, 0X37, 0X10, 0X07, 0X07, 0X08, 0X00, 0X00, 0X00,
		    0X0A, 0X00, 0X01} },
	{0xD5, 44, {0X04, 0X05, 0X06, 0X07, 0X00, 0X01, 0X02,
		    0X03, 0X20, 0X21, 0X22, 0X23, 0X18, 0X18, 0X18, 0X18,
		    0X18, 0X18, 0X18, 0X18, 0X19, 0X19, 0X18, 0X18, 0X18,
		    0X18, 0X1B, 0X1B, 0X1A, 0X1A, 0X18, 0X18, 0X18, 0X18,
		    0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18,
		    0X18} },
	{0xD6, 44, {0X03, 0X02, 0X01, 0X00, 0X07, 0X06, 0X05,
		    0X04, 0X23, 0X22, 0X21, 0X20, 0X18, 0X18, 0X18, 0X18,
		    0X18, 0X18, 0X58, 0X58, 0X18, 0X18, 0X19, 0X19, 0X18,
		    0X18, 0X1B, 0X1B, 0X1A, 0X1A, 0X18, 0X18, 0X18, 0X18,
		    0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18,
		    0X18} },
	{0xCC, 1, {0X09} },
	{0xB6, 2, {0X4C, 0X4C} },
	{0xE0, 42, {0X00, 0X10, 0X16, 0X2D, 0X33, 0X3F, 0X23,
		    0X3E, 0X07, 0X0B, 0X0D, 0X17, 0X0E, 0X12, 0X14, 0X12,
		    0X13, 0X06, 0X11, 0X13, 0X18, 0X00, 0X0F, 0X16, 0X2E,
		    0X33, 0X3F, 0X23, 0X3D, 0X07, 0X0B, 0X0D, 0X18, 0X0F,
		    0X12, 0X14, 0X12, 0X14, 0X07, 0X11, 0X12, 0X17} },
	{0xC7, 4, {0X00, 0XC0, 0X00, 0XC0} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 150, {} },
	{0x29, 0, {} },
	{REGFLAG_DELAY, 50, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
#else				/* debug only */
	{0xB9, 3, {0XFF, 0X83, 0X94} },
	{0xBA, 2, {0X33, 0X83} },
	{0xB0, 4, {0X00, 0X00, 0X7D, 0X0C} },
	{0xB1, 15, {0X6C, 0X15, 0X15, 0X24, 0X04, 0X11, 0XF1, 0X80,
		    0XE4, 0X97, 0X23, 0X80, 0XC0, 0XD2, 0X58} },
	{0xB2, 11, {0X00, 0X64, 0X10, 0X07, 0X22, 0X1C, 0X08, 0X08,
		    0X1C, 0X4D, 0X00} },
	{0xB4, 12, {0X00, 0XFF, 0X03, 0X5a, 0X03, 0X5a, 0X03, 0X5a,
		    0X01, 0X6a, 0X30, 0X6a} },
	{0xBc, 1, {0X07} },
	{0xBF, 3, {0X41, 0X0E, 0X01} },
	{0xB6, 2, {0X5C, 0X5C} },
	{0xCC, 1, {0X09} },
	{0xD3, 30, {0X00, 0X06, 0X00, 0X40, 0X07, 0X08, 0X00, 0X32,
		    0X10, 0X07, 0X00, 0X07, 0X54, 0X15, 0X0F, 0X05, 0X04, 0X02,
		    0X12, 0X10, 0X05, 0X07, 0X33, 0X33, 0X0B, 0X0B, 0X37, 0X10,
		    0X07, 0X07} },
	{0xD5, 44, {0X04, 0X05, 0X06, 0X07, 0X00, 0X01, 0X02, 0X03,
		    0X20, 0X21, 0X22, 0X23, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18,
		    0X18, 0X18, 0X19, 0X19, 0X18, 0X18, 0X18, 0X18, 0X1B, 0X1B,
		    0X1A, 0X1A, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18,
		    0X18, 0X18, 0X18, 0X18, 0X18, 0X18} },
	{0xD6, 44, {0X03, 0X02, 0X01, 0X00, 0X07, 0X06, 0X05, 0X04,
		    0X23, 0X22, 0X21, 0X20, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18,
		    0X58, 0X58, 0X18, 0X18, 0X19, 0X19, 0X18, 0X18, 0X1B, 0X1B,
		    0X1A, 0X1A, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18, 0X18,
		    0X18, 0X18, 0X18, 0X18, 0X18, 0X18} },
	{0xE0, 42, {0X00, 0X10, 0X16, 0X2D, 0X33, 0X3F, 0X23, 0X3E,
		    0X07, 0X0B, 0X0D, 0X17, 0X0E, 0X12, 0X14, 0X12, 0X13, 0X06,
		    0X11, 0X13, 0X18, 0X00, 0X0F, 0X16, 0X2E, 0X33, 0X3F, 0X23,
		    0X3D, 0X07, 0X0B, 0X0D, 0X18, 0X0F, 0X12, 0X14, 0X12, 0X14,
		    0X07, 0X11, 0X12, 0X17} },
	{0xc0, 2, {0X30, 0X14} },
	{0xC7, 4, {0X00, 0XC0, 0X00, 0XC0} },
	{0xdf, 1, {0X8e} },
	{0xd2, 1, {0X66} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 500, {} },
	{0x29, 0, {} },
	{REGFLAG_DELAY, 50, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
#endif
};

static void LCD_panel_init(__u32 sel)
{
	__u32 i;
	__u32 rx_num;
	__u8 rx_bf0 = 0, rx_bf1 = 0, rx_bf2 = 0;
	__u32 hx8394d_used = 0;

	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_delay_ms(10);

	panel_rst(1);		/* add by lyp@20140423 */
	sunxi_lcd_delay_ms(50);	/* add by lyp@20140423 */
	panel_rst(0);
	sunxi_lcd_delay_ms(20);
	panel_rst(1);
	sunxi_lcd_delay_ms(200);

	for (i = 0;; i++) {
		if (hx8394d_test_config_para[i].cmd == 0x02)
			break;
		else if (hx8394d_test_config_para[i].cmd == 0x01)
			sunxi_lcd_delay_ms(hx8394d_test_config_para[i].count);
		else
			dsi_dcs_wr(0, hx8394d_test_config_para[i].cmd,
				   hx8394d_test_config_para[i].para_list,
				   hx8394d_test_config_para[i].count);

	}

	dsi_dcs_rd(0, 0xDA, &rx_bf0, &rx_num);
	dsi_dcs_rd(0, 0xDB, &rx_bf1, &rx_num);
	dsi_dcs_rd(0, 0xDC, &rx_bf2, &rx_num);
	if ((rx_bf0 == 0x83) && (rx_bf1 == 0x94) && (rx_bf2 == 0xd))
		hx8394d_used = 1;

	if (hx8394d_used) {
		for (i = 0;; i++) {
			if (hx8394d_initialization_setting[i].cmd == 0x02)
				break;
			else if (hx8394d_initialization_setting[i].cmd == 0x01)
				sunxi_lcd_delay_ms
				    (hx8394d_initialization_setting[i].count);
			else
				dsi_dcs_wr(0,
					   hx8394d_initialization_setting[i].
					   cmd,
					   hx8394d_initialization_setting[i].
					   para_list,
					   hx8394d_initialization_setting[i].
					   count);

		}
	} else {
		for (i = 0;; i++) {
			if (otm1283_initialization_setting[i].cmd == 0x02)
				break;
			else if (otm1283_initialization_setting[i].cmd == 0x01)
				sunxi_lcd_delay_ms
				    (otm1283_initialization_setting[i].count);
			else
				dsi_dcs_wr(0,
					   otm1283_initialization_setting[i].
					   cmd,
					   otm1283_initialization_setting[i].
					   para_list,
					   otm1283_initialization_setting[i].
					   count);
		}
	}

	/* sunxi_lcd_dsi_write(sel,DSI_DCS_EXIT_SLEEP_MODE, 0, 0); */
	/* sunxi_lcd_delay_ms(200); */

	sunxi_lcd_dsi_clk_enable(sel);

	/* sunxi_lcd_dsi_write(sel,DSI_DCS_SET_DISPLAY_ON, 0, 0); */
	/* sunxi_lcd_delay_ms(200); */

}

static void LCD_panel_exit(__u32 sel)
{
	sunxi_lcd_dsi_clk_disable(sel);
	panel_rst(0);

}

/* sel: 0:lcd0; 1:lcd1 */
static __s32 LCD_user_defined_func(__u32 sel,
				   __u32 para1, __u32 para2, __u32 para3)
{
	return 0;
}

struct __lcd_panel_t sl698ph_720p_panel = {
	/* panel driver name, must mach the */
	/*name of lcd_drv_name in sys_config.fex */
	.name = "sl698ph_720p",
	.func = {
		 .cfg_panel_info = LCD_cfg_panel_info,
		 .cfg_open_flow = LCD_open_flow,
		 .cfg_close_flow = LCD_close_flow,
		 .lcd_user_defined_func = LCD_user_defined_func,
		 },
};
