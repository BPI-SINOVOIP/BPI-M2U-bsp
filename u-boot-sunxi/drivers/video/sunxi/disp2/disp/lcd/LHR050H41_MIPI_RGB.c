#include "LHR050H41_MIPI_RGB.h"

extern s32 bsp_disp_get_panel_info(u32 screen_id, disp_panel_para *info);
static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 0, val)
#define power_en(val)  sunxi_lcd_gpio_set_value(sel, 1, val)

#define REGFLAG_END_OF_TABLE     0x102
#define REGFLAG_DELAY            0x101

struct lcd_setting_table {
    u16 cmd;
    u32 count;
    u8 para_list[64];
};

static struct lcd_setting_table lcm_initialization_setting[] = {
    //***ILI9881C_LG4.99-Gamma2.2-20150716****
    {0xFF,3,{0x98,0x81,0x03}},

    //GIP_1
    {0x01,1,{0x00}},
    {0x02,1,{0x00}},
    {0x03,1,{0x73}},
    {0x04,1,{0x03}},
    {0x05,1,{0x00}},
    {0x06,1,{0x06}},
    {0x07,1,{0x06}},
    {0x08,1,{0x00}},
    {0x09,1,{0x18}},
    {0x0a,1,{0x04}},
    {0x0b,1,{0x00}},
    {0x0c,1,{0x02}},
    {0x0d,1,{0x03}},
    {0x0e,1,{0x00}},
    {0x0f,1,{0x25}},
    {0x10,1,{0x25}},
    {0x11,1,{0x00}},
    {0x12,1,{0x00}},
    {0x13,1,{0x00}},
    {0x14,1,{0x00}},
    {0x15,1,{0x00}},
    {0x16,1,{0x0C}},
    {0x17,1,{0x00}},
    {0x18,1,{0x00}},
    {0x19,1,{0x00}},
    {0x1a,1,{0x00}},
    {0x1b,1,{0x00}},
    {0x1c,1,{0x00}},
    {0x1d,1,{0x00}},
    {0x1e,1,{0xC0}},
    {0x1f,1,{0x80}},
    {0x20,1,{0x04}},
    {0x21,1,{0x01}},
    {0x22,1,{0x00}},
    {0x23,1,{0x00}},
    {0x24,1,{0x00}},
    {0x25,1,{0x00}},
    {0x26,1,{0x00}},
    {0x27,1,{0x00}},
    {0x28,1,{0x33}},
    {0x29,1,{0x03}},
    {0x2a,1,{0x00}},
    {0x2b,1,{0x00}},
    {0x2c,1,{0x00}},
    {0x2d,1,{0x00}},
    {0x2e,1,{0x00}},
    {0x2f,1,{0x00}},
    {0x30,1,{0x00}},
    {0x31,1,{0x00}},
    {0x32,1,{0x00}},
    {0x33,1,{0x00}},
    {0x34,1,{0x04}},
    {0x35,1,{0x00}},
    {0x36,1,{0x00}},
    {0x37,1,{0x00}},
    {0x38,1,{0x3C}},
    {0x39,1,{0x00}},
    {0x3a,1,{0x00}},
    {0x3b,1,{0x00}},
    {0x3c,1,{0x00}},
    {0x3d,1,{0x00}},
    {0x3e,1,{0x00}},
    {0x3f,1,{0x00}},
    {0x40,1,{0x00}},
    {0x41,1,{0x00}},
    {0x42,1,{0x00}},
    {0x43,1,{0x00}},
    {0x44,1,{0x00}},

    //GIP_2
    {0x50,1,{0x01}},
    {0x51,1,{0x23}},
    {0x52,1,{0x45}},
    {0x53,1,{0x67}},
    {0x54,1,{0x89}},
    {0x55,1,{0xab}},
    {0x56,1,{0x01}},
    {0x57,1,{0x23}},
    {0x58,1,{0x45}},
    {0x59,1,{0x67}},
    {0x5a,1,{0x89}},
    {0x5b,1,{0xab}},
    {0x5c,1,{0xcd}},
    {0x5d,1,{0xef}},

    //GIP_3
    {0x5e,1,{0x11}},
    {0x5f,1,{0x02}},
    {0x60,1,{0x02}},
    {0x61,1,{0x02}},
    {0x62,1,{0x02}},
    {0x63,1,{0x02}},
    {0x64,1,{0x02}},
    {0x65,1,{0x02}},
    {0x66,1,{0x02}},
    {0x67,1,{0x02}},
    {0x68,1,{0x02}},
    {0x69,1,{0x02}},
    {0x6a,1,{0x0C}},
    {0x6b,1,{0x02}},
    {0x6c,1,{0x0F}},
    {0x6d,1,{0x0E}},
    {0x6e,1,{0x0D}},
    {0x6f,1,{0x06}},
    {0x70,1,{0x07}},
    {0x71,1,{0x02}},
    {0x72,1,{0x02}},
    {0x73,1,{0x02}},
    {0x74,1,{0x02}},
    {0x75,1,{0x02}},
    {0x76,1,{0x02}},
    {0x77,1,{0x02}},
    {0x78,1,{0x02}},
    {0x79,1,{0x02}},
    {0x7a,1,{0x02}},
    {0x7b,1,{0x02}},
    {0x7c,1,{0x02}},
    {0x7d,1,{0x02}},
    {0x7e,1,{0x02}},
    {0x7f,1,{0x02}},
    {0x80,1,{0x0C}},
    {0x81,1,{0x02}},
    {0x82,1,{0x0F}},
    {0x83,1,{0x0E}},
    {0x84,1,{0x0D}},
    {0x85,1,{0x06}},
    {0x86,1,{0x07}},
    {0x87,1,{0x02}},
    {0x88,1,{0x02}},
    {0x89,1,{0x02}},
    {0x8A,1,{0x02}},

    //CMD_Page 4
    {0xFF,3,{0x98,0x81,0x04}},
    {0x6C,1,{0x15}},
    {0x6E,1,{0x22}},               //di_pwr_reg=0 VGH clamp 13.6V
    {0x6F,1,{0x33}},               // reg vcl + VGH pumping ratio 3x VGL=-2x
    {0x3A,1,{0xA4}},               //POWER SAVING
    {0x8D,1,{0x0D}},               //VGL clamp -8.6V
    {0x87,1,{0xBA}},               //ESD
    {0x26,1,{0x76}},
    {0xB2,1,{0xD1}},

    //CMD_Page 1
    {0xFF,3,{0x98,0x81,0x01}},
    {0x22,1,{0x0A}},	//BGR, SS
    {0x53,1,{0xDC}},	//VCOM1  D7
    {0x55,1,{0xA7}},	//VCOM2
    {0x50,1,{0x78}},      	//VREG1OUT=V
    {0x51,1,{0x78}},      	//VREG2OUT=V
    {0x31,1,{0x02}},	//column inversion
    {0x60,1,{0x14}},             //SDT

    {0xA0,1,{0x2A}},		//VP255	Gamma P    08  //3.79v
    {0xA1,1,{0x39}},               //VP251
    {0xA2,1,{0x46}},               //VP247
    {0xA3,1,{0x0e}},               //VP243
    {0xA4,1,{0x12}},               //VP239
    {0xA5,1,{0x25}},               //VP231
    {0xA6,1,{0x19}},               //VP219
    {0xA7,1,{0x1d}},               //VP203
    {0xA8,1,{0xa6}},               //VP175
    {0xA9,1,{0x1C}},                //VP144
    {0xAA,1,{0x29}},               //VP111
    {0xAB,1,{0x85}},               //VP80
    {0xAC,1,{0x1C}},               //VP52
    {0xAD,1,{0x1B}},               //VP36
    {0xAE,1,{0x51}},               //VP24
    {0xAF,1,{0x22}},               //VP16
    {0xB0,1,{0x2d}},               //VP12
    {0xB1,1,{0x4f}},               //VP8
    {0xB2,1,{0x59}},               //VP4
    {0xB3,1,{0x3F}},               //VP0

    {0xC0,1,{0x2A}},		//VN255 GAMMA N     08 //-4.11
    {0xC1,1,{0x3a}},               //VN251
    {0xC2,1,{0x45}},               //VN247
    {0xC3,1,{0x0e}},               //VN243
    {0xC4,1,{0x11}},               //VN239
    {0xC5,1,{0x24}},               //VN231
    {0xC6,1,{0x1a}},               //VN219
    {0xC7,1,{0x1c}},               //VN203
    {0xC8,1,{0xaa}},               //VN175
    {0xC9,1,{0x1C}},               //VN144
    {0xCA,1,{0x29}},               //VN111
    {0xCB,1,{0x96}},               //VN80
    {0xCC,1,{0x1C}},               //VN52
    {0xCD,1,{0x1B}},               //VN36
    {0xCE,1,{0x51}},               //VN24
    {0xCF,1,{0x22}},               //VN16
    {0xD0,1,{0x2b}},               //VN12
    {0xD1,1,{0x4b}},              //VN8
    {0xD2,1,{0x59}},               //VN4
    {0xD3,1,{0x3F}},               //VN0

    //CMD_Page 0
    {0xFF,3,{0x98,0x81,0x00}},
    {0x35,1,{0x00}},
    {0x11,1,{0x00}},
    {REGFLAG_DELAY, 120, {}},
    {0x29,1,{0x00}},
    {REGFLAG_DELAY, 20, {}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void LCD_cfg_panel_info(panel_extend_para * info)
{
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
	{
		//{input value, corrected value}
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
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl)/2;
	for (i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{
	printf("[BPI]LCD_open_flow\n");
	
	LCD_OPEN_FUNC(sel, LCD_power_on, 20);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 10);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 20);     //open lcd controller, and delay 100ms
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	printf("[BPI]LCD_close_flow\n");
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	200);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 500);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	printf("[BPI]LCD_power_on\n");
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 1);//config lcd_power pin to open lcd power0
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_enable(sel, 2);//config lcd_power pin to open lcd power2
	sunxi_lcd_delay_ms(5);
	/* jdi, power on */
	power_en(1);
	sunxi_lcd_delay_ms(5);
	panel_reset(1);
	sunxi_lcd_delay_ms(10);
	panel_reset(0);
	sunxi_lcd_delay_ms(10);
	panel_reset(1);
	sunxi_lcd_delay_ms(20);
	
	sunxi_lcd_pin_cfg(sel, 1);
}

static void LCD_power_off(u32 sel)
{
	printf("[BPI]LCD_power_off\n");
	sunxi_lcd_pin_cfg(sel, 0);
	power_en(0);
	sunxi_lcd_delay_ms(20);
	panel_reset(0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 2);//config lcd_power pin to close lcd power2
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);//config lcd_power pin to close lcd power1
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(u32 sel)
{
	printf("[BPI]LCD_bl_open\n");
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	printf("[BPI]LCD_bl_close\n");
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 sel)
{
	u32 i;

	printf("[BPI]LCD_panel_init\n");
	
	for (i = 0; ; i++) {
        	if(lcm_initialization_setting[i].cmd == REGFLAG_END_OF_TABLE) {
            		break;
        	} 
		else if (lcm_initialization_setting[i].cmd == REGFLAG_DELAY) {
            		sunxi_lcd_delay_ms(lcm_initialization_setting[i].count);
        	} else {
            		dsi_dcs_wr(sel, (u8)lcm_initialization_setting[i].cmd, lcm_initialization_setting[i].para_list, lcm_initialization_setting[i].count);
        	}
    	}

	sunxi_lcd_dsi_clk_enable(sel);

	return;
}

static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_clk_disable(sel);

	return ;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

__lcd_panel_t LHR050H41_MIPI_RGB_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "LHR050H41_MIPI_RGB",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};
