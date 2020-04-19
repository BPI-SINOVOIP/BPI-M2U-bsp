#include "u708_default_panel.h"

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

static u8 const mipi_dcs_pixel_format[4] = { 0x77, 0x66, 0x66, 0x55 };

#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 1, val)
#define power_en(val)  sunxi_lcd_gpio_set_value(sel, 0, val)

static void LCD_cfg_panel_info(panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {

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

	u32 lcd_cmap_tbl[2][3][4] = {
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
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] -
			      lcd_gamma_tbl[i][1]) * j)
			    / num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16)
	    + (lcd_gamma_tbl[items - 1][1] << 8) + lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	/* open lcd power, and delay 50ms */
	LCD_OPEN_FUNC(sel, LCD_power_on, 100);
	/* open lcd power, than delay 200ms */
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);
	/* open lcd controller, and delay 100ms 300 */
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
	/* open lcd backlight, and delay 0ms */
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	/* close lcd backlight, and delay 0ms */
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 200);
	/* close lcd controller, and delay 0ms */
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 10);
	/* open lcd power, than delay 200ms */
	LCD_CLOSE_FUNC(sel, LCD_panel_exit, 200);
	/* close lcd power, and delay 500ms */
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);

	return 0;
}

static void LCD_power_on(u32 sel)
{
	/* config lcd_power pin to open lcd power */
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 1);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 2);
	sunxi_lcd_delay_ms(5);
	power_en(1);
	sunxi_lcd_delay_ms(20);
	panel_reset(1);
	sunxi_lcd_delay_ms(10);
	panel_reset(0);

	sunxi_lcd_delay_ms(10);
	panel_reset(1);
	sunxi_lcd_delay_ms(120);
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	power_en(0);
	sunxi_lcd_delay_ms(20);
	panel_reset(0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 2);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);
}

static void LCD_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_delay_ms(50);
	/* config lcd_bl_en pin to open lcd backlight */
	sunxi_lcd_backlight_enable(sel);
}

static void LCD_bl_close(u32 sel)
{
	/* config lcd_bl_en pin to close lcd backlight */
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_delay_ms(20);
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 sel)
{

	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(20);

	/* while(1){ */
	/* dsi_dcs_wr_0para(sel,0x11); */
	/* sunxi_lcd_delay_ms(120); */
	/* dsi_dcs_wr_0para(sel,0x29); */
	sunxi_lcd_delay_ms(80);
}

static void LCD_panel_exit(u32 sel)
{
	dsi_dcs_wr_0para(sel, DSI_DCS_SET_DISPLAY_OFF);
	sunxi_lcd_delay_ms(20);
	dsi_dcs_wr_0para(sel, DSI_DCS_ENTER_SLEEP_MODE);
	sunxi_lcd_delay_ms(80);
}

/* sel: 0:lcd0; 1:lcd1 */
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

/* sel: 0:lcd0; 1:lcd1 */
/*
 * static s32 LCD_set_bright(u32 sel, u32 bright)
 * {
 *	sunxi_lcd_dsi_dcs_write_1para(sel,0x51,bright);
 *	return 0;
 * }
 */

struct __lcd_panel_t u708_default_panel = {
	/*
	 * panel driver name,
	 * must mach the name of lcd_drv_name in sys_config.fex
	 */
	.name = "u708_default_panel",
	.func = {
		 .cfg_panel_info = LCD_cfg_panel_info,
		 .cfg_open_flow = LCD_open_flow,
		 .cfg_close_flow = LCD_close_flow,
		 .lcd_user_defined_func = LCD_user_defined_func,
		 /* .set_bright = LCD_set_bright, */
		 },
};
