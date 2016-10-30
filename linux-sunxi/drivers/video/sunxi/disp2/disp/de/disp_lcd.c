#include "disp_lcd.h"

struct disp_lcd_private_data
{
	disp_lcd_flow             open_flow;
	disp_lcd_flow             close_flow;
	disp_panel_para           panel_info;
	panel_extend_para         panel_extend_info;
	disp_lcd_cfg              lcd_cfg;
	disp_lcd_panel_fun        lcd_panel_fun;
	bool                      enabling;
	bool                      disabling;
	bool                      bl_enabled;
	u32                       irq_no;
	u32                       irq_no_dsi;
	u32                       irq_no_edp;
	u32                       enabled;
	u32                       power_enabled;
	u32                       bl_need_enabled;
	u32                       frame_per_sec;
	u32                       usec_per_line;
	u32                       judge_line;
	u32                       tri_finish_fail;
	struct {
		uintptr_t               dev;
		u32                     channel;
		u32                     polarity;
		u32                     period_ns;
		u32                     duty_ns;
		u32                     enabled;
	}pwm_info;
	struct clk *clk;
	struct clk *lvds_clk;
	struct clk *dsi_clk0;
	struct clk *dsi_clk1;
	struct clk *edp_clk;
	struct clk *clk_parent;
};
#if defined(__LINUX_PLAT__)
static spinlock_t lcd_data_lock;
#else
static int lcd_data_lock;
#endif

static struct disp_device *lcds = NULL;
static struct disp_lcd_private_data *lcd_private;

s32 disp_lcd_set_bright(struct disp_device *lcd, u32 bright);
s32 disp_lcd_get_bright(struct disp_device *lcd);

struct disp_device* disp_get_lcd(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens || !bsp_disp_feat_is_supported_output_types(disp, DISP_OUTPUT_TYPE_LCD)) {
		DE_INF("disp %d not support lcd output\n", disp);
		return NULL;
	}

	return &lcds[disp];
}
static struct disp_lcd_private_data *disp_lcd_get_priv(struct disp_device *lcd)
{
	if (NULL == lcd) {
		DE_WRN("param is NULL!\n");
		return NULL;
	}

	return (struct disp_lcd_private_data *)lcd->priv_data;
}

static s32 disp_lcd_is_used(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	s32 ret = 0;

	if ((NULL == lcd) || (NULL == lcdp)) {
		ret = 0;
	} else {
		ret = (s32)lcdp->lcd_cfg.lcd_used;
	}
	return ret;
}

static s32 lcd_parse_panel_para(u32 disp, disp_panel_para * info)
{
    s32 ret = 0;
    char primary_key[25];
    s32 value = 0;

    sprintf(primary_key, "lcd%d", disp);
    memset(info, 0, sizeof(disp_panel_para));

    ret = disp_sys_script_get_item(primary_key, "lcd_x", &value, 1);
    if (ret == 1)
    {
        info->lcd_x = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_y", &value, 1);
    if (ret == 1)
    {
        info->lcd_y = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_width", &value, 1);
    if (ret == 1)
    {
        info->lcd_width = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_height", &value, 1);
    if (ret == 1)
    {
        info->lcd_height = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_dclk_freq", &value, 1);
    if (ret == 1)
    {
        info->lcd_dclk_freq = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_pwm_used", &value, 1);
    if (ret == 1)
    {
        info->lcd_pwm_used = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_pwm_ch", &value, 1);
    if (ret == 1)
    {
        info->lcd_pwm_ch = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_pwm_freq", &value, 1);
    if (ret == 1)
    {
        info->lcd_pwm_freq = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_pwm_pol", &value, 1);
    if (ret == 1)
    {
        info->lcd_pwm_pol = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_if", &value, 1);
    if (ret == 1)
    {
        info->lcd_if = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_hbp", &value, 1);
    if (ret == 1)
    {
        info->lcd_hbp = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_ht", &value, 1);
    if (ret == 1)
    {
        info->lcd_ht = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_vbp", &value, 1);
    if (ret == 1)
    {
        info->lcd_vbp = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_vt", &value, 1);
    if (ret == 1)
    {
        info->lcd_vt = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_hv_if", &value, 1);
    if (ret == 1)
    {
        info->lcd_hv_if = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_vspw", &value, 1);
    if (ret == 1)
    {
        info->lcd_vspw = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_hspw", &value, 1);
    if (ret == 1)
    {
        info->lcd_hspw = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_lvds_if", &value, 1);
    if (ret == 1)
    {
        info->lcd_lvds_if = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_lvds_mode", &value, 1);
    if (ret == 1)
    {
        info->lcd_lvds_mode = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_lvds_colordepth", &value, 1);
    if (ret == 1)
    {
        info->lcd_lvds_colordepth= value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_lvds_io_polarity", &value, 1);
    if (ret == 1)
    {
        info->lcd_lvds_io_polarity = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_cpu_if", &value, 1);
    if (ret == 1)
    {
        info->lcd_cpu_if = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_cpu_te", &value, 1);
    if (ret == 1)
    {
        info->lcd_cpu_te = value;
    }

	ret = disp_sys_script_get_item(primary_key, "lcd_cpu_mode", &value, 1);
	if (ret == 1)
		info->lcd_cpu_mode = value;

    ret = disp_sys_script_get_item(primary_key, "lcd_frm", &value, 1);
    if (ret == 1)
    {
        info->lcd_frm = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_dsi_if", &value, 1);
    if (ret == 1)
    {
        info->lcd_dsi_if = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_dsi_lane", &value, 1);
    if (ret == 1)
    {
        info->lcd_dsi_lane = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_dsi_format", &value, 1);
    if (ret == 1)
    {
        info->lcd_dsi_format = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_dsi_eotp", &value, 1);
    if (ret == 1)
    {
        info->lcd_dsi_eotp = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_dsi_te", &value, 1);
    if (ret == 1)
    {
        info->lcd_dsi_te = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_edp_rate", &value, 1);
    if (ret == 1)
    {
        info->lcd_edp_rate = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_edp_lane", &value, 1);
    if (ret == 1)
    {
        info->lcd_edp_lane= value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_edp_colordepth", &value, 1);
    if (ret == 1)
    {
        info->lcd_edp_colordepth = value;
    }

	ret = disp_sys_script_get_item(primary_key, "lcd_edp_fps", &value, 1);
    if (ret == 1)
    {
        info->lcd_edp_fps = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_hv_clk_phase", &value, 1);
    if (ret == 1)
    {
        info->lcd_hv_clk_phase = value;
    }

	ret = disp_sys_script_get_item(primary_key, "lcd_hv_sync_polarity", &value, 1);
    if (ret == 1)
    {
        info->lcd_hv_sync_polarity = value;
    }

	ret = disp_sys_script_get_item(primary_key, "lcd_hv_srgb_seq", &value, 1);
    if (ret == 1)
    {
        info->lcd_hv_srgb_seq = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_rb_swap", &value, 1);
    if (ret == 1)
    {
        info->lcd_rb_swap = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_hv_syuv_seq", &value, 1);
    if (ret == 1)
    {
        info->lcd_hv_syuv_seq = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_hv_syuv_fdly", &value, 1);
    if (ret == 1)
    {
        info->lcd_hv_syuv_fdly = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_gamma_en", &value, 1);
    if (ret == 1)
    {
        info->lcd_gamma_en = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_cmap_en", &value, 1);
    if (ret == 1)
    {
        info->lcd_cmap_en = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_xtal_freq", &value, 1);
    if (ret == 1)
    {
        info->lcd_xtal_freq = value;
    }

    ret = disp_sys_script_get_item(primary_key, "lcd_size", (int*)info->lcd_size, 2);
    ret = disp_sys_script_get_item(primary_key, "lcd_model_name", (int*)info->lcd_model_name, 2);

    return 0;
}

#if 0
static void lcd_panel_parameter_check(u32 disp, struct disp_device* lcd)
{
	disp_panel_para* info;
	u32 cycle_num = 1;
	u32 Lcd_Panel_Err_Flag = 0;
	u32 Lcd_Panel_Wrn_Flag = 0;
	u32 Disp_Driver_Bug_Flag = 0;

	u32 lcd_fclk_frq;
	u32 lcd_clk_div;
	s32 ret = 0;

	char primary_key[20];
	s32 value = 0;

	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return ;
	}

	if (!disp_al_query_lcd_mod(lcd->hwdev_index))
		return;

	sprintf(primary_key, "lcd%d", lcd->disp);
	ret = disp_sys_script_get_item(primary_key, "lcd_used", &value, 1);

	if (ret != 0 ) {
		DE_WRN("get lcd%dpara lcd_used fail\n", lcd->disp);
		return;
	} else {
		if (value != 1) {
			DE_WRN("lcd%dpara is not used\n", lcd->disp);
			return;
		}
	}

	info = &(lcdp->panel_info);
	if (NULL == info) {
		DE_WRN("NULL hdl!\n");
		return;
	}

	if (info->lcd_if == 0 && info->lcd_hv_if == 8)
		cycle_num = 3;
	else if (info->lcd_if == 0 && info->lcd_hv_if == 10)
		cycle_num = 3;
	else if (info->lcd_if == 0 && info->lcd_hv_if == 11)
		cycle_num = 4;
	else if (info->lcd_if == 0 && info->lcd_hv_if == 12)
		cycle_num = 4;
	else if (info->lcd_if == 1 && info->lcd_cpu_if == 2)
		cycle_num = 3;
	else if (info->lcd_if == 1 && info->lcd_cpu_if == 4)
		cycle_num = 2;
	else if (info->lcd_if == 1 && info->lcd_cpu_if == 6)
		cycle_num = 2;
	else if (info->lcd_if == 1 && info->lcd_cpu_if == 10)
		cycle_num = 2;
	else if (info->lcd_if == 1 && info->lcd_cpu_if == 12)
		cycle_num = 3;
	else if (info->lcd_if == 1 && info->lcd_cpu_if == 14)
		cycle_num = 2;
	else
		cycle_num = 1;

	if (info->lcd_hbp > info->lcd_hspw)
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT0;
	}

	if (info->lcd_vbp > info->lcd_vspw)
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT1;
	}

	if (info->lcd_ht >= (info->lcd_hbp+info->lcd_x*cycle_num+4))
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT2;
	}

	if ((info->lcd_vt) >= (info->lcd_vbp+info->lcd_y + 2))
	{
		;
	}
	else
	{
		Lcd_Panel_Err_Flag |= BIT3;
	}

	lcd_clk_div = disp_al_lcd_get_clk_div(disp);

	if (lcd_clk_div >= 6)
	{
		;
	}
	else if (lcd_clk_div >=2)
	{
		if ((info->lcd_hv_clk_phase == 1) && (info->lcd_hv_clk_phase == 3))
		{
			Lcd_Panel_Err_Flag |= BIT10;
		}
	}
	else
	{
		Disp_Driver_Bug_Flag |= 1;
	}

	if ((info->lcd_if == 1 && info->lcd_cpu_if == 0) ||	(info->lcd_if == 1 && info->lcd_cpu_if == 10)
		|| (info->lcd_if == 1 && info->lcd_cpu_if == 12) ||(info->lcd_if == 3 && info->lcd_lvds_colordepth == 1))
	{
		if (info->lcd_frm != 1)
			Lcd_Panel_Wrn_Flag |= BIT0;
	}
	else if (info->lcd_if == 1 && ((info->lcd_cpu_if == 2) || (info->lcd_cpu_if == 4) || (info->lcd_cpu_if == 6)
		|| (info->lcd_cpu_if == 8) || (info->lcd_cpu_if == 14)))
	{
		if (info->lcd_frm != 2)
			Lcd_Panel_Wrn_Flag |= BIT1;
	}

	lcd_fclk_frq = (info->lcd_dclk_freq * 1000 * 1000) / ((info->lcd_vt) * info->lcd_ht);
	if (lcd_fclk_frq < 50 || lcd_fclk_frq > 70)
	{
		Lcd_Panel_Wrn_Flag |= BIT2;
	}

	if (Lcd_Panel_Err_Flag != 0 || Lcd_Panel_Wrn_Flag != 0)
	{
		if (Lcd_Panel_Err_Flag != 0)
		{
			u32 i;
			for (i = 0; i < 200; i++)
			{
				//OSAL_PRINTF("*** Lcd in danger...\n");
			}
		}

		OSAL_PRINTF("*****************************************************************\n");
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("*** LCD Panel Parameter Check\n");
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("***             by guozhenjie\n");
		OSAL_PRINTF("***\n");
		OSAL_PRINTF("*****************************************************************\n");

		OSAL_PRINTF("*** \n");
		OSAL_PRINTF("*** Interface:");
		if (info->lcd_if == 0 && info->lcd_hv_if == 0)
			{OSAL_PRINTF("*** Parallel HV Panel\n");}
		else if (info->lcd_if == 0 && info->lcd_hv_if == 8)
			{OSAL_PRINTF("*** Serial HV Panel\n");}
		else if (info->lcd_if == 0 && info->lcd_hv_if == 10)
			{OSAL_PRINTF("*** Dummy RGB HV Panel\n");}
		else if (info->lcd_if == 0 && info->lcd_hv_if == 11)
			{OSAL_PRINTF("*** RGB Dummy HV Panel\n");}
		else if (info->lcd_if == 0 && info->lcd_hv_if == 12)
			{OSAL_PRINTF("*** Serial YUV Panel\n");}
		else if (info->lcd_if == 3 && info->lcd_lvds_colordepth== 0)
			{OSAL_PRINTF("*** 24Bit LVDS Panel\n");}
		else if (info->lcd_if == 3 && info->lcd_lvds_colordepth== 1)
			{OSAL_PRINTF("*** 18Bit LVDS Panel\n");}
		else if ((info->lcd_if == 1) && (info->lcd_cpu_if == 0 || info->lcd_cpu_if == 10 || info->lcd_cpu_if == 12))
			{OSAL_PRINTF("*** 18Bit CPU Panel\n");}
		else if ((info->lcd_if == 1) && (info->lcd_cpu_if == 2 || info->lcd_cpu_if == 4 ||
				info->lcd_cpu_if == 6 || info->lcd_cpu_if == 8 || info->lcd_cpu_if == 14))
			{OSAL_PRINTF("*** 16Bit CPU Panel\n");}
		else
		{
			OSAL_PRINTF("\n");
			OSAL_PRINTF("*** lcd_if:     %d\n",info->lcd_if);
			OSAL_PRINTF("*** lcd_hv_if:  %d\n",info->lcd_hv_if);
			OSAL_PRINTF("*** lcd_cpu_if: %d\n",info->lcd_cpu_if);
		}
		if (info->lcd_frm == 0)
			{OSAL_PRINTF("*** Lcd Frm Disable\n");}
		else if (info->lcd_frm == 1)
			{OSAL_PRINTF("*** Lcd Frm to RGB666\n");}
		else if (info->lcd_frm == 2)
			{OSAL_PRINTF("*** Lcd Frm to RGB565\n");}

		OSAL_PRINTF("*** \n");
		OSAL_PRINTF("*** Timing:\n");
		OSAL_PRINTF("*** lcd_x:      %d\n", info->lcd_x);
		OSAL_PRINTF("*** lcd_y:      %d\n", info->lcd_y);
		OSAL_PRINTF("*** lcd_ht:     %d\n", info->lcd_ht);
		OSAL_PRINTF("*** lcd_hbp:    %d\n", info->lcd_hbp);
		OSAL_PRINTF("*** lcd_vt:     %d\n", info->lcd_vt);
		OSAL_PRINTF("*** lcd_vbp:    %d\n", info->lcd_vbp);
		OSAL_PRINTF("*** lcd_hspw:   %d\n", info->lcd_hspw);
		OSAL_PRINTF("*** lcd_vspw:   %d\n", info->lcd_vspw);
		OSAL_PRINTF("*** lcd_frame_frq:  %dHz\n", lcd_fclk_frq);

		OSAL_PRINTF("*** \n");
		if (Lcd_Panel_Err_Flag & BIT0)
			{OSAL_PRINTF("*** Err01: Violate \"lcd_hbp > lcd_hspw\"\n");}
		if (Lcd_Panel_Err_Flag & BIT1)
			{OSAL_PRINTF("*** Err02: Violate \"lcd_vbp > lcd_vspw\"\n");}
		if (Lcd_Panel_Err_Flag & BIT2)
			{OSAL_PRINTF("*** Err03: Violate \"lcd_ht >= (lcd_hbp+lcd_x*%d+4)\"\n", cycle_num);}
		if (Lcd_Panel_Err_Flag & BIT3)
			{OSAL_PRINTF("*** Err04: Violate \"(lcd_vt) >= (lcd_vbp+lcd_y+2)\"\n");}
		if (Lcd_Panel_Err_Flag & BIT10)
			{OSAL_PRINTF("*** Err10: Violate \"lcd_hv_clk_phase\",use \"0\" or \"2\"");}
		if (Lcd_Panel_Wrn_Flag & BIT0)
			{OSAL_PRINTF("*** WRN01: Recommend \"lcd_frm = 1\"\n");}
		if (Lcd_Panel_Wrn_Flag & BIT1)
			{OSAL_PRINTF("*** WRN02: Recommend \"lcd_frm = 2\"\n");}
		if (Lcd_Panel_Wrn_Flag & BIT2)
			{OSAL_PRINTF("*** WRN03: Recommend \"lcd_dclk_frq = %d\"\n",
				((info->lcd_vt) * info->lcd_ht) * 60 / (1000 * 1000));}
		OSAL_PRINTF("*** \n");

		if (Lcd_Panel_Err_Flag != 0)
		{
			u32 image_base_addr;
			u32 reg_value = 0;

			image_base_addr = DE_Get_Reg_Base(disp);

			sys_put_wvalue(image_base_addr+0x804, 0xffff00ff);//set background color

			reg_value = sys_get_wvalue(image_base_addr + 0x800);
			sys_put_wvalue(image_base_addr+0x800, reg_value & 0xfffff0ff);//close all layer

			mdelay(2000);
			sys_put_wvalue(image_base_addr + 0x804, 0x00000000);//set background color
			sys_put_wvalue(image_base_addr + 0x800, reg_value);//open layer

			OSAL_PRINTF("*** Try new parameters,you can make it pass!\n");
		}
		OSAL_PRINTF("*** LCD Panel Parameter Check End\n");
		OSAL_PRINTF("*****************************************************************\n");
	}
}
#endif

static void lcd_get_sys_config(u32 disp, disp_lcd_cfg *lcd_cfg)
{
    disp_gpio_set_t  *gpio_info;
    int  value = 1;
    char primary_key[20], sub_name[25];
    int i = 0;
    int  ret;

    sprintf(primary_key, "lcd%d", disp);
//lcd_used
    ret = disp_sys_script_get_item(primary_key, "lcd_used", &value, 1);
    if (ret == 1)
    {
        lcd_cfg->lcd_used = value;
    }

    if (lcd_cfg->lcd_used == 0) //no need to get lcd config if lcd_used eq 0
        return ;

//lcd_bl_en
    lcd_cfg->lcd_bl_en_used = 0;
    gpio_info = &(lcd_cfg->lcd_bl_en);
    ret = disp_sys_script_get_item(primary_key,"lcd_bl_en", (int *)gpio_info, 3);
    if (ret == 3)
    {
        lcd_cfg->lcd_bl_en_used = 1;
    }

	sprintf(sub_name, "lcd_bl_en_power");
	ret = disp_sys_script_get_item(primary_key, sub_name, (int *)lcd_cfg->lcd_bl_en_power, 2);

//lcd fix power
	for (i=0; i<LCD_POWER_NUM; i++)
	{
		if (i==0)
			sprintf(sub_name, "lcd_fix_power");
		else
			sprintf(sub_name, "lcd_fix_power%d", i);
		lcd_cfg->lcd_power_used[i] = 0;
		ret = disp_sys_script_get_item(primary_key,sub_name, (int *)(lcd_cfg->lcd_fix_power[i]), 2);
		if (ret == 2) {
			/* str */
			lcd_cfg->lcd_fix_power_used[i] = 1;
		}
	}

//lcd_power
	for (i=0; i<LCD_POWER_NUM; i++)
	{
		if (i==0)
			sprintf(sub_name, "lcd_power");
		else
			sprintf(sub_name, "lcd_power%d", i);
		lcd_cfg->lcd_power_used[i] = 0;
		ret = disp_sys_script_get_item(primary_key,sub_name, (int *)(lcd_cfg->lcd_power[i]), 2);
		if (ret == 2) {
			/* str */
			lcd_cfg->lcd_power_used[i] = 1;
		}
	}

//lcd_gpio
	for (i = 0; i < 6; i++)
    {
        sprintf(sub_name, "lcd_gpio_%d", i);

        gpio_info = &(lcd_cfg->lcd_gpio[i]);
        ret = disp_sys_script_get_item(primary_key,sub_name, (int *)gpio_info, 3);
        if (ret == 3)
        {
            lcd_cfg->lcd_gpio_used[i]= 1;
        }
    }

//lcd_gpio_scl,lcd_gpio_sda
    gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SCL]);
    ret = disp_sys_script_get_item(primary_key,"lcd_gpio_scl", (int *)gpio_info, 3);
    if (ret == 3)
    {
        lcd_cfg->lcd_gpio_used[LCD_GPIO_SCL]= 1;
    }
    gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SDA]);
    ret = disp_sys_script_get_item(primary_key,"lcd_gpio_sda", (int *)gpio_info, 3);
    if (ret == 3)
    {
        lcd_cfg->lcd_gpio_used[LCD_GPIO_SDA]= 1;
    }

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++)
	{
		sprintf(sub_name, "lcd_gpio_power%d", i);

		ret = disp_sys_script_get_item(primary_key, sub_name, (int *)lcd_cfg->lcd_gpio_power[i], 2);
	}

	for (i=0; i<LCD_GPIO_REGU_NUM; i++) {
		if (0==i)
			sprintf(sub_name, "lcd_pin_power");
		else
			sprintf(sub_name, "lcd_pin_power%d", i);
		ret = disp_sys_script_get_item(primary_key, sub_name, (int *)lcd_cfg->lcd_pin_power[i], 2);
	}

//backlight adjust
	for (i = 0; i < 101; i++) {
		sprintf(sub_name, "lcd_bl_%d_percent", i);
		lcd_cfg->backlight_curve_adjust[i] = 0;

		if (i == 100)
			lcd_cfg->backlight_curve_adjust[i] = 255;

		ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
		if (ret == 1) {
			value = (value > 100)? 100:value;
			value = value * 255 / 100;
			lcd_cfg->backlight_curve_adjust[i] = value;
		}
	}

	sprintf(sub_name, "lcd_backlight");
	ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
	if (ret == 1) {
		value = (value > 256)? 256:value;
		lcd_cfg->backlight_bright = value;
	} else {
		lcd_cfg->backlight_bright = 197;
	}

}

static s32 lcd_clk_init(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	DE_INF("lcd %d clk init\n", lcd->disp);

	lcdp->clk_parent = clk_get_parent(lcdp->clk);

	return DIS_SUCCESS;
}

static s32 lcd_clk_exit(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (lcdp->clk_parent)
		clk_put(lcdp->clk_parent);

	return DIS_SUCCESS;
}

static s32 lcd_clk_config(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct lcd_clk_info clk_info;
	unsigned long pll_rate = 297000000, lcd_rate = 33000000, dclk_rate = 33000000, dsi_rate = 0;//hz
	unsigned long pll_rate_set = 297000000, lcd_rate_set = 33000000, dclk_rate_set = 33000000, dsi_rate_set = 0;//hz

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	memset(&clk_info, 0, sizeof(struct lcd_clk_info));
	disp_al_lcd_get_clk_info(lcd->hwdev_index, &clk_info, &lcdp->panel_info);
	dclk_rate = lcdp->panel_info.lcd_dclk_freq * 1000000;//Mhz -> hz
	if (LCD_IF_DSI == lcdp->panel_info.lcd_if) {
		lcd_rate = dclk_rate * clk_info.dsi_div;
		pll_rate = lcd_rate * clk_info.lcd_div;
	} else {
		lcd_rate = dclk_rate * clk_info.tcon_div;
		pll_rate = lcd_rate * clk_info.lcd_div;
	}
	dsi_rate = pll_rate / clk_info.dsi_div;

	if (lcdp->clk_parent) {
		clk_set_rate(lcdp->clk_parent, pll_rate);
		pll_rate_set = clk_get_rate(lcdp->clk_parent);
	}

	if (clk_info.lcd_div)
		lcd_rate_set = pll_rate_set / clk_info.lcd_div;
	else
		lcd_rate_set = pll_rate_set;

	clk_set_rate(lcdp->clk, lcd_rate_set);
	lcd_rate_set = clk_get_rate(lcdp->clk);
	if (LCD_IF_DSI == lcdp->panel_info.lcd_if) {
		dsi_rate_set = pll_rate_set / clk_info.dsi_div;
		dsi_rate_set = (0 == clk_info.dsi_rate)? dsi_rate_set:clk_info.dsi_rate;
		clk_set_rate(lcdp->dsi_clk0, dsi_rate_set);
		//disp_sys_clk_set_rate(lcdp->dsi_clk1, dsi_rate_set);//FIXME, dsi clk0 = dsi clk1(rate)
	}
	dclk_rate_set = lcd_rate_set / clk_info.tcon_div;
	if ((pll_rate_set != pll_rate) || (lcd_rate_set != lcd_rate)
		|| (dclk_rate_set != dclk_rate)) {
			DE_WRN("disp %d, clk: pll(%ld),clk(%ld),dclk(%ld) dsi_rate(%ld)\n     clk real:pll(%ld),clk(%ld),dclk(%ld) dsi_rate(%ld)\n",
				lcd->disp, pll_rate, lcd_rate, dclk_rate, dsi_rate, pll_rate_set, lcd_rate_set, dclk_rate_set, dsi_rate_set);
	}

	return 0;
}

static s32 lcd_clk_enable(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int ret = 0;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	lcd_clk_config(lcd);

	/*
	 * Because clk is lvds_clk's parent, so when we use lvds interface,
	 * we just need to enable lvds_clk.
	 */
	if (LCD_IF_LVDS != lcdp->panel_info.lcd_if) {
		ret = clk_prepare_enable(lcdp->clk);
		if (0 != ret) {
			DE_WRN("fail enable lcd's clock!\n");
			goto exit;
		}
	}

	if (LCD_IF_LVDS == lcdp->panel_info.lcd_if) {
		ret = clk_prepare_enable(lcdp->lvds_clk);
		if (0 != ret) {
			DE_WRN("fail enable lvds's clock!\n");
			goto exit;
		}
	} else if (LCD_IF_DSI == lcdp->panel_info.lcd_if) {
		if (lcdp->dsi_clk0) {
			ret = clk_prepare_enable(lcdp->dsi_clk0);
			if (0 != ret) {
				DE_WRN("fail enable dsi's clock0!\n");
				goto exit;
			}
		} else {
			DE_WRN("dsi's clock0 is NULL!\n");
			goto exit;
		}
		if (lcdp->dsi_clk1) {
			ret = clk_prepare_enable(lcdp->dsi_clk1);
			if (0 != ret) {
				DE_WRN("fail enable dsi's clock1!\n");
				goto exit;
			}
		}
	} else if (LCD_IF_EDP == lcdp->panel_info.lcd_if) {
		ret = clk_prepare_enable(lcdp->edp_clk);
		if (0 != ret) {
			DE_WRN("fail enable edp's clock!\n");
			goto exit;
		}
	}

exit:
	return	ret;
}

static s32 lcd_clk_disable(struct disp_device* lcd)
{	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (LCD_IF_LVDS == lcdp->panel_info.lcd_if) {
		clk_disable(lcdp->lvds_clk);
	} else if (LCD_IF_DSI == lcdp->panel_info.lcd_if) {
		if (lcdp->dsi_clk1)
			clk_disable(lcdp->dsi_clk1);
		if (lcdp->dsi_clk0)
			clk_disable(lcdp->dsi_clk0);
	} else if (LCD_IF_EDP == lcdp->panel_info.lcd_if) {
		clk_disable(lcdp->edp_clk);
	}
	/*
	 * Because clk is lvds_clk's parent, so when we use lvds interface,
	 * we just need to disable lvds_clk.
	 */
	if (LCD_IF_LVDS != lcdp->panel_info.lcd_if)
		clk_disable(lcdp->clk);

	return	DIS_SUCCESS;
}

static int lcd_calc_judge_line(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (lcdp->usec_per_line == 0) {
		disp_panel_para *panel_info = &lcdp->panel_info;
		/*
		 * usec_per_line = 1 / fps / vt * 1000000
		 *               = 1 / (dclk * 1000000 / vt / ht) / vt * 1000000
		 *               = ht / dclk(Mhz)
		 */
		lcdp->frame_per_sec = panel_info->lcd_dclk_freq * 1000000
		    / panel_info->lcd_ht / panel_info->lcd_vt
		    * (panel_info->lcd_interlace + 1);
		lcdp->usec_per_line = panel_info->lcd_ht
		    / panel_info->lcd_dclk_freq;
	}

	if (lcdp->judge_line == 0) {
		int start_delay = disp_al_lcd_get_start_delay(lcd->hwdev_index,
		    &lcdp->panel_info);
		int usec_start_delay = start_delay * lcdp->usec_per_line;
		int usec_judge_point;

		if (usec_start_delay <= 200)
			usec_judge_point = usec_start_delay * 3 / 7;
		else if (usec_start_delay <= 400)
			usec_judge_point = usec_start_delay / 2;
		else
			usec_judge_point = 200;
		lcdp->judge_line = usec_judge_point / lcdp->usec_per_line;
	}

	return 0;
}

#ifdef EINK_FLUSH_TIME_TEST
struct timeval lcd_start,lcd_mid, lcd_mid1, lcd_mid2,lcd_end,t5_b,t5_e,pin_b,pin_e,po_b,po_e,tocn_b,tcon_e;
unsigned int lcd_t1=0,lcd_t2=0,lcd_t3=0,lcd_t4=0,lcd_t5=0,lcd_pin,lcd_po,lcd_tcon;
#endif
static s32 disp_lcd_tcon_enable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return disp_al_lcd_enable(lcd->hwdev_index, &lcdp->panel_info);
}

static s32 disp_lcd_tcon_disable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	return disp_al_lcd_disable(lcd->hwdev_index, &lcdp->panel_info);
}

static s32 disp_lcd_pin_cfg(struct disp_device *lcd, u32 bon)
{
	int  i;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char dev_name[25];

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("lcd %d pin config, state %s, %d\n", lcd->disp, (bon)? "on":"off", bon);

	//io-pad
	if (bon == 1) {
		for (i=0; i<LCD_GPIO_REGU_NUM; i++) {
			if (!((!strcmp(lcdp->lcd_cfg.lcd_pin_power[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_pin_power[i], "none"))))
				disp_sys_power_enable(lcdp->lcd_cfg.lcd_pin_power[i]);
		}
	}

	sprintf(dev_name, "lcd%d", lcd->disp);
	disp_sys_pin_set_state(dev_name, (1==bon)? DISP_PIN_STATE_ACTIVE:DISP_PIN_STATE_SLEEP);

	disp_al_lcd_io_cfg(lcd->hwdev_index, bon, &lcdp->panel_info);

	if (bon == 0) {
		for (i=LCD_GPIO_REGU_NUM-1; i>=0; i--) {
			if (!((!strcmp(lcdp->lcd_cfg.lcd_pin_power[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_pin_power[i], "none"))))
				disp_sys_power_disable(lcdp->lcd_cfg.lcd_pin_power[i]);
		}
	}

	return DIS_SUCCESS;
}

#if defined (CONFIG_FPGA_V4_PLATFORM) && defined (SUPPORT_EINK)
static s32 disp_lcd_pwm_enable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	volatile unsigned long *reg = 0;
	unsigned long val = 0;
	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		reg = ((volatile unsigned long *)0xf1c20878);
		val = readl(reg);
		val = val & 0xfff0ffff;
		val = val | 0x10000;
		writel(val, reg);
		reg = ((volatile unsigned long *)0xf1c2087C);
		val = readl(reg);
		val = val & 0xefffffff;
		val = val | 0x10000000;
		writel(val, reg);
		return 0;
	}
	DE_WRN("pwm device hdl is NULL\n");
	return DIS_FAIL;
}
static s32 disp_lcd_pwm_disable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	volatile unsigned long  *reg = 0;
	unsigned long val = 0;
	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		reg = ((volatile unsigned long  *)0xf1c20878);
		val = readl(reg);
		val = val & 0xfff0ffff;
		val = val | 0x10000;
		writel(val, reg);
		reg = ((volatile unsigned long  *)0xf1c2087C);
		val = readl(reg);
		val = val & 0xefffffff;
		val = val | 0x00000000;
		writel(val, reg);
		return 0;
	}
	DE_WRN("pwm device hdl is NULL\n");
	return DIS_FAIL;
}
#else
static s32 disp_lcd_pwm_enable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		return disp_sys_pwm_enable(lcdp->pwm_info.dev);
	}
	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}

static s32 disp_lcd_pwm_disable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		return disp_sys_pwm_disable(lcdp->pwm_info.dev);
	}
	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}
#endif

static s32 disp_lcd_backlight_enable(struct disp_device *lcd)
{
	disp_gpio_set_t  gpio_info[1];
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	if (lcdp->bl_enabled) {
		spin_unlock_irqrestore(&lcd_data_lock, flags);
		return -EBUSY;
	}

	lcdp->bl_need_enabled = 1;
	lcdp->bl_enabled = true;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	if (disp_lcd_is_used(lcd)) {
		unsigned bl;
		if (lcdp->lcd_cfg.lcd_bl_en_used) {
			//io-pad
			if (!((!strcmp(lcdp->lcd_cfg.lcd_bl_en_power, "")) || (!strcmp(lcdp->lcd_cfg.lcd_bl_en_power, "none"))))
				disp_sys_power_enable(lcdp->lcd_cfg.lcd_bl_en_power);

			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_bl_en), sizeof(disp_gpio_set_t));

			lcdp->lcd_cfg.lcd_bl_gpio_hdl =
			    disp_sys_gpio_request(gpio_info, 1);
		}
		bl = disp_lcd_get_bright(lcd);
		disp_lcd_set_bright(lcd, bl);
	}

	return 0;
}

static s32 disp_lcd_backlight_disable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	if (!lcdp->bl_enabled) {
		spin_unlock_irqrestore(&lcd_data_lock, flags);
		return -EBUSY;
	}

	lcdp->bl_enabled = false;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	if (disp_lcd_is_used(lcd)) {
		if (lcdp->lcd_cfg.lcd_bl_en_used) {
			disp_sys_gpio_release(lcdp->lcd_cfg.lcd_bl_gpio_hdl, 2);

			//io-pad
			if (!((!strcmp(lcdp->lcd_cfg.lcd_bl_en_power, "")) || (!strcmp(lcdp->lcd_cfg.lcd_bl_en_power, "none"))))
				disp_sys_power_disable(lcdp->lcd_cfg.lcd_bl_en_power);
		}
	}

	return 0;
}

#if defined (CONFIG_FPGA_V4_PLATFORM) && defined (SUPPORT_EINK)
static s32 disp_lcd_power_enable(struct disp_device *lcd, u32 power_id)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	volatile unsigned long  *reg = 0;
	unsigned long val = 0;
	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd)) {
		reg = ((volatile unsigned long  *)0xf1c20878);
		val = readl(reg);
		val = val & 0x000fffff;
		val = val | 0x11100000;
		writel(val, reg);
		reg = ((volatile unsigned long  *)0xf1c2087C);
		val = readl(reg);
		val = val & 0x1fffffff;
		val = val | 0xe0000000;
		writel(val, reg);
		return 0;
	}
	return DIS_FAIL;
}
static s32 disp_lcd_power_disable(struct disp_device *lcd, u32 power_id)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	volatile unsigned long  *reg = 0;
	unsigned long val = 0;
	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd)) {
		reg = ((volatile unsigned long  *)0xf1c20878);
		val = readl(reg);
		val = val & 0x000fffff;
		val = val | 0x77700000;
		writel(val, reg);
		return 0;
	}
	return DIS_FAIL;
}
#else
static s32 disp_lcd_power_enable(struct disp_device *lcd, u32 power_id)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (disp_lcd_is_used(lcd)) {
		if (1 == lcdp->lcd_cfg.lcd_power_used[power_id]) {
			/* regulator type */
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_power[power_id]);
		}
	}

	return 0;
}

static s32 disp_lcd_power_disable(struct disp_device *lcd, u32 power_id)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (disp_lcd_is_used(lcd)) {
		if (1 == lcdp->lcd_cfg.lcd_power_used[power_id]) {
			/* regulator type */
			disp_sys_power_disable(lcdp->lcd_cfg.lcd_power[power_id]);
		}
	}

	return 0;
}
#endif

static s32 disp_lcd_bright_get_adjust_value(struct disp_device *lcd, u32 bright)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	bright = (bright > 255)? 255:bright;
	return lcdp->panel_extend_info.lcd_bright_curve_tbl[bright];
}

static s32 disp_lcd_bright_curve_init(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 i = 0, j=0;
	u32 items = 0;
	u32 lcd_bright_curve_tbl[101][2];

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	for (i = 0; i < 101; i++) {
		if (lcdp->lcd_cfg.backlight_curve_adjust[i] == 0) {
			if (i == 0) {
				lcd_bright_curve_tbl[items][0] = 0;
				lcd_bright_curve_tbl[items][1] = 0;
				items++;
			}
		}	else {
			lcd_bright_curve_tbl[items][0] = 255 * i / 100;
			lcd_bright_curve_tbl[items][1] = lcdp->lcd_cfg.backlight_curve_adjust[i];
			items++;
		}
	}

	for (i=0; i<items-1; i++) {
		u32 num = lcd_bright_curve_tbl[i+1][0] - lcd_bright_curve_tbl[i][0];

		for (j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] + ((lcd_bright_curve_tbl[i+1][1] - lcd_bright_curve_tbl[i][1]) * j)/num;
			lcdp->panel_extend_info.lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j] = value;
		}
	}
	lcdp->panel_extend_info.lcd_bright_curve_tbl[255] = lcd_bright_curve_tbl[items-1][1];

	return 0;
}

s32 disp_lcd_set_bright(struct disp_device *lcd, u32 bright)
{
	u32 duty_ns;
	__u64 backlight_bright = bright;
	__u64 backlight_dimming;
	__u64 period_ns;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;
	bool bright_update = false;
	struct disp_manager *mgr = NULL;
	struct disp_smbl *smbl = NULL;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	mgr = lcd->manager;
	if (NULL == mgr) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	smbl = mgr->smbl;

	spin_lock_irqsave(&lcd_data_lock, flags);
	backlight_bright = (backlight_bright > 255)? 255:backlight_bright;
	if (lcdp->lcd_cfg.backlight_bright != backlight_bright) {
		bright_update = true;
		lcdp->lcd_cfg.backlight_bright = backlight_bright;
	}
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	if (bright_update && smbl)
		smbl->update_backlight(smbl, backlight_bright);

	if (lcdp->pwm_info.dev) {
		if (backlight_bright != 0)	{
			backlight_bright += 1;
		}
		backlight_bright = disp_lcd_bright_get_adjust_value(lcd, backlight_bright);

		lcdp->lcd_cfg.backlight_dimming = (0 == lcdp->lcd_cfg.backlight_dimming)? 256:lcdp->lcd_cfg.backlight_dimming;
		backlight_dimming = lcdp->lcd_cfg.backlight_dimming;
		period_ns = lcdp->pwm_info.period_ns;
		duty_ns = (backlight_bright * backlight_dimming *  period_ns/256 + 128) / 256;
		lcdp->pwm_info.duty_ns = duty_ns;
		disp_sys_pwm_config(lcdp->pwm_info.dev, duty_ns, period_ns);
	}

	if (lcdp->lcd_panel_fun.set_bright && lcdp->enabled) {
		lcdp->lcd_panel_fun.set_bright(lcd->disp,disp_lcd_bright_get_adjust_value(lcd,bright));
	}

	return DIS_SUCCESS;
}

s32 disp_lcd_get_bright(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	return lcdp->lcd_cfg.backlight_bright;
}

static s32 disp_lcd_set_bright_dimming(struct disp_device *lcd, u32 dimming)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 bl = 0;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	dimming = dimming > 256?256:dimming;
	lcdp->lcd_cfg.backlight_dimming = dimming;
	bl = disp_lcd_get_bright(lcd);
	disp_lcd_set_bright(lcd, bl);

	return DIS_SUCCESS;
}

static s32 disp_lcd_get_panel_info(struct disp_device *lcd, disp_panel_para* info)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	memcpy(info, (disp_panel_para*)(&(lcdp->panel_info)), sizeof(disp_panel_para));
	return 0;
}

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
extern int eink_display_one_frame(struct disp_eink_manager* manager);
#endif
extern void sync_event_proc(u32 disp, bool timeout);
#if defined(__LINUX_PLAT__)
static s32 disp_lcd_event_proc(int irq, void *parg)
#else
static s32 disp_lcd_event_proc(void *parg)
#endif
{
	struct disp_device *lcd = (struct disp_device*)parg;
	struct disp_lcd_private_data *lcdp = NULL;
#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	struct disp_eink_manager* eink_manager = NULL;
#else
	struct disp_manager *mgr = NULL;
#endif
	u32 hwdev_index;
	u32 irq_flag = 0;

	if (NULL == lcd)
		return DISP_IRQ_RETURN;

	hwdev_index = lcd->hwdev_index;
	lcdp = disp_lcd_get_priv(lcd);

	if (NULL == lcdp)
		return DISP_IRQ_RETURN;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	eink_manager = disp_get_eink_manager(0);
	if (NULL == eink_manager)
		return DISP_IRQ_RETURN;
#endif

	if (disp_al_lcd_query_irq(hwdev_index, LCD_IRQ_TCON0_VBLK, &lcdp->panel_info)) {
#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)

		eink_display_one_frame(eink_manager);
#else
		int cur_line = disp_al_lcd_get_cur_line(hwdev_index, &lcdp->panel_info);
		int start_delay = disp_al_lcd_get_start_delay(hwdev_index, &lcdp->panel_info);

		mgr = lcd->manager;
		if (NULL == mgr)
			return DISP_IRQ_RETURN;

		if (cur_line <= (start_delay - lcdp->judge_line)) {
			sync_event_proc(mgr->disp, false);
		} else {
			sync_event_proc(mgr->disp, true);
		}
#endif
	} else {
		irq_flag = disp_al_lcd_query_irq(hwdev_index,
		    LCD_IRQ_TCON0_CNTR, &lcdp->panel_info);
		irq_flag |= disp_al_lcd_query_irq(hwdev_index,
		    LCD_IRQ_TCON0_TRIF, &lcdp->panel_info);

		if (0 == irq_flag)
			goto exit;

		if (disp_al_lcd_tri_busy(hwdev_index, &lcdp->panel_info)) {
			/* if lcd is still busy when tri/cnt irq coming,
			 * take it as failture, record failture times,
			 * when it reach 2 times, clear counter
			 */
			lcdp->tri_finish_fail++;
			lcdp->tri_finish_fail = (2 == lcdp->tri_finish_fail) ?
			    0 : lcdp->tri_finish_fail;
		} else
			lcdp->tri_finish_fail = 0;

		mgr = lcd->manager;
		if (NULL == mgr)
			return DISP_IRQ_RETURN;

		if (0 == lcdp->tri_finish_fail) {
			sync_event_proc(mgr->disp, false);
			disp_al_lcd_tri_start(hwdev_index, &lcdp->panel_info);
		} else
			sync_event_proc(mgr->disp, true);
	}

exit:
	return DISP_IRQ_RETURN;
}

/* lcd enable except for backlight */
static s32 disp_lcd_fake_enable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i;
	struct disp_manager *mgr = NULL;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if ((NULL == mgr)) {
		DE_WRN("mgr is NULL!\n");
		return DIS_FAIL;
	}
	if (1 == disp_lcd_is_enabled(lcd))
		return 0;

#if !defined (SUPPORT_EINK) && !defined (EINK_PANEL_USED)
	if (mgr->enable)
		mgr->enable(mgr);
#endif

	/* init fix power */
	for (i=0; i<LCD_POWER_NUM; i++) {
		if (1 == lcdp->lcd_cfg.lcd_fix_power_used[i]) {
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_fix_power[i]);
		}
	}

	if ((LCD_IF_DSI == lcdp->panel_info.lcd_if) && (0 != lcdp->irq_no_dsi)) {
		disp_sys_register_irq(lcdp->irq_no_dsi,0,disp_lcd_event_proc,(void*)lcd,0,0);
		disp_sys_enable_irq(lcdp->irq_no_dsi);
	} else {
		disp_sys_register_irq(lcdp->irq_no,0,disp_lcd_event_proc,(void*)lcd,0,0);
		disp_sys_enable_irq(lcdp->irq_no);
	}
	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabling = 1;
	lcdp->bl_need_enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	disp_lcd_gpio_init(lcd);
	lcd_clk_enable(lcd);
	disp_al_lcd_cfg(lcd->hwdev_index, &lcdp->panel_info, &lcdp->panel_extend_info);
	lcdp->open_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_open_flow)	{
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->disp);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->disp);
	}

	for (i = 0; i < lcdp->open_flow.func_num - 1; i++) {
		if (lcdp->open_flow.func[i].func) {
			lcdp->open_flow.func[i].func(lcd->disp);
			DE_INF("open flow:step %d finish, to delay %d\n", i, lcdp->open_flow.func[i].delay);
			if (0 != lcdp->open_flow.func[i].delay)
				disp_delay_ms(lcdp->open_flow.func[i].delay);
		}
	}
	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return 0;
}
#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
static s32 disp_lcd_enable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_manager *mgr = NULL;
	int ret;
	int i;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	flush_work(&lcd->close_eink_panel_work);
	DE_INF("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if ((NULL == mgr)) {
		DE_WRN("mgr is NULL!\n");
		return DIS_FAIL;
	}
	if (1 == disp_lcd_is_enabled(lcd))
		return 0;

	disp_sys_register_irq(lcdp->irq_no, 0, disp_lcd_event_proc, (void *)lcd, 0, 0);
	disp_sys_enable_irq(lcdp->irq_no);

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabling = 1;
	lcdp->bl_need_enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	if (lcdp->lcd_panel_fun.cfg_panel_info)
		lcdp->lcd_panel_fun.cfg_panel_info(&lcdp->panel_extend_info);
	else
		DE_WRN("lcd_panel_fun[%d].cfg_panel_info is NULL\n", lcd->disp);

	disp_lcd_gpio_init(lcd);
	ret = lcd_clk_enable(lcd);
	if (0 != ret)
		return DIS_FAIL;

	disp_al_lcd_cfg(lcd->hwdev_index, &lcdp->panel_info, &lcdp->panel_extend_info);
	lcdp->open_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_open_flow)	{
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->disp);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->disp);
	}

	for (i=0; i<lcdp->open_flow.func_num; i++) {
		if (lcdp->open_flow.func[i].func) {
			lcdp->open_flow.func[i].func(lcd->disp);
			DE_INF("open flow:step %d finish, to delay %d\n", i, lcdp->open_flow.func[i].delay);
			if (0 != lcdp->open_flow.func[i].delay)
				disp_delay_ms(lcdp->open_flow.func[i].delay);
		}
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return 0;
}

static s32 disp_lcd_disable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_manager *mgr = NULL;
	int i;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if ((NULL == mgr)) {
		DE_WRN("mgr is NULL!\n");
		return DIS_FAIL;
	}
	if (0 == disp_lcd_is_enabled(lcd))
		return 0;

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	lcdp->bl_need_enabled = 0;
	lcdp->close_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_close_flow)	{
		lcdp->lcd_panel_fun.cfg_close_flow(lcd->disp);
	} else {
		DE_WRN("lcd_panel_fun[%d].cfg_close_flow is NULL\n", lcd->disp);
	}

	for (i = 0; i < lcdp->close_flow.func_num; i++) {
		if (lcdp->close_flow.func[i].func) {
			lcdp->close_flow.func[i].func(lcd->disp);
			DE_INF("close flow:step %d finish, to delay %d\n", i, lcdp->close_flow.func[i].delay);
			if (0 != lcdp->close_flow.func[i].delay)
				disp_delay_ms(lcdp->close_flow.func[i].delay);
		}
	}

	lcd_clk_disable(lcd);
	disp_lcd_gpio_exit(lcd);

	disp_sys_disable_irq(lcdp->irq_no);
	disp_sys_unregister_irq(lcdp->irq_no, disp_lcd_event_proc, (void *)lcd);

	return 0;
}

#else
static s32 disp_lcd_enable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i;
	struct disp_manager *mgr = NULL;
	unsigned bl;
	int ret;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	__inf("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if ((NULL == mgr)) {
		DE_WRN("mgr is NULL!\n");
		return DIS_FAIL;
	}
	if (1 == disp_lcd_is_enabled(lcd))
		return 0;

	if (mgr->enable)
		mgr->enable(mgr);

	/* init fix power */
	for (i=0; i<LCD_POWER_NUM; i++) {
		if (1 == lcdp->lcd_cfg.lcd_fix_power_used[i]) {
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_fix_power[i]);
		}
	}

	if ((LCD_IF_DSI == lcdp->panel_info.lcd_if) && (0 != lcdp->irq_no_dsi)) {
		disp_sys_register_irq(lcdp->irq_no_dsi,0,disp_lcd_event_proc,(void*)lcd,0,0);
		disp_sys_enable_irq(lcdp->irq_no_dsi);
	} else {
		disp_sys_register_irq(lcdp->irq_no,0,disp_lcd_event_proc,(void*)lcd,0,0);
		disp_sys_enable_irq(lcdp->irq_no);
	}
	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabling = 1;
	lcdp->bl_need_enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	if (lcdp->lcd_panel_fun.cfg_panel_info)
		lcdp->lcd_panel_fun.cfg_panel_info(&lcdp->panel_extend_info);
	else
		DE_WRN("lcd_panel_fun[%d].cfg_panel_info is NULL\n", lcd->disp);
	lcdp->panel_extend_info.lcd_gamma_en = lcdp->panel_info.lcd_gamma_en;
	disp_lcd_gpio_init(lcd);
	ret = lcd_clk_enable(lcd);
	if (0 != ret)
		return DIS_FAIL;

	disp_sys_pwm_set_polarity(lcdp->pwm_info.dev, lcdp->pwm_info.polarity);
	disp_al_lcd_cfg(lcd->hwdev_index, &lcdp->panel_info, &lcdp->panel_extend_info);
	lcd_calc_judge_line(lcd);
	lcdp->open_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_open_flow)	{
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->disp);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->disp);
	}

	for (i = 0; i < lcdp->open_flow.func_num; i++) {
		if (lcdp->open_flow.func[i].func) {
			lcdp->open_flow.func[i].func(lcd->disp);
			DE_INF("open flow:step %d finish, to delay %d\n", i, lcdp->open_flow.func[i].delay);
			if (0 != lcdp->open_flow.func[i].delay)
				disp_delay_ms(lcdp->open_flow.func[i].delay);
		}
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	bl = disp_lcd_get_bright(lcd);
	disp_lcd_set_bright(lcd, bl);

	return 0;
}

static s32 disp_lcd_disable(struct disp_device* lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_manager *mgr = NULL;
	int i;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if ((NULL == mgr)) {
		DE_WRN("mgr is NULL!\n");
		return DIS_FAIL;
	}
	if (0 == disp_lcd_is_enabled(lcd))
		return 0;

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	lcdp->bl_need_enabled = 0;
	lcdp->close_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_close_flow)	{
		lcdp->lcd_panel_fun.cfg_close_flow(lcd->disp);
	}	else {
		DE_WRN("lcd_panel_fun[%d].cfg_close_flow is NULL\n", lcd->disp);
	}

	for (i=0; i<lcdp->close_flow.func_num; i++) {
		if (lcdp->close_flow.func[i].func) {
			lcdp->close_flow.func[i].func(lcd->disp);
			DE_INF("close flow:step %d finish, to delay %d\n", i, lcdp->close_flow.func[i].delay);
			if (0 != lcdp->close_flow.func[i].delay)
				disp_delay_ms(lcdp->close_flow.func[i].delay);
		}
	}

	disp_lcd_gpio_exit(lcd);

	lcd_clk_disable(lcd);
	if ((LCD_IF_DSI == lcdp->panel_info.lcd_if) && (0 != lcdp->irq_no_dsi)) {
		disp_sys_disable_irq(lcdp->irq_no_dsi);
		disp_sys_unregister_irq(lcdp->irq_no_dsi, disp_lcd_event_proc,(void*)lcd);
	} else {
		disp_sys_disable_irq(lcdp->irq_no);
		disp_sys_unregister_irq(lcdp->irq_no, disp_lcd_event_proc,(void*)lcd);
	}

	/* disable fix power */
	for (i=LCD_POWER_NUM -1; i>=0; i--) {
		if (1 == lcdp->lcd_cfg.lcd_fix_power_used[i]) {
			disp_sys_power_disable(lcdp->lcd_cfg.lcd_fix_power[i]);
		}
	}

#if !defined (EINK_PANEL_USED) && !defined (SUPPORT_EINK)
	if (mgr->disable)
		mgr->disable(mgr);
#endif

	return 0;
}
#endif
static s32 disp_lcd_sw_enable(struct disp_device* lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i;
	struct disp_manager *mgr = NULL;
	disp_gpio_set_t  gpio_info[1];

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	mgr = lcd->manager;
	if ((NULL == mgr)) {
		DE_WRN("mgr is NULL!\n");
		return DIS_FAIL;
	}
	if (mgr->sw_enable)
		mgr->sw_enable(mgr);

#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (0 != lcd_clk_enable(lcd))
		return DIS_FAIL;
#endif

	/* init fix power */
	for (i=0; i<LCD_POWER_NUM; i++) {
		if (1 == lcdp->lcd_cfg.lcd_fix_power_used[i]) {
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_fix_power[i]);
		}
	}

	/* init lcd power */
	for (i=0; i<LCD_POWER_NUM; i++) {
		if (1 == lcdp->lcd_cfg.lcd_power_used[i]) {
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_power[i]);
		}
	}

	/* init gpio */
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++)
	{
		if (!((!strcmp(lcdp->lcd_cfg.lcd_gpio_power[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_gpio_power[i], "none"))))
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_gpio_power[i]);
	}

	for (i=0; i<LCD_GPIO_NUM; i++) {
		lcdp->lcd_cfg.gpio_hdl[i] = 0;

		if (lcdp->lcd_cfg.lcd_gpio_used[i]) {
			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_gpio[i]), sizeof(disp_gpio_set_t));
			lcdp->lcd_cfg.gpio_hdl[i] =
					disp_sys_gpio_request(gpio_info, 1);
		}
	}

	/* init lcd pin */
	for (i=0; i<LCD_GPIO_REGU_NUM; i++) {
		if (!((!strcmp(lcdp->lcd_cfg.lcd_pin_power[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_pin_power[i], "none"))))
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_pin_power[i]);
	}

	/* init bl */
	if (lcdp->lcd_cfg.lcd_bl_en_used) {
		//io-pad
		if (!((!strcmp(lcdp->lcd_cfg.lcd_bl_en_power, "")) || (!strcmp(lcdp->lcd_cfg.lcd_bl_en_power, "none"))))
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_bl_en_power);
		lcdp->lcd_cfg.lcd_bl_gpio_hdl =
		    disp_sys_gpio_request(&lcdp->lcd_cfg.lcd_bl_en, 1);
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	lcdp->bl_need_enabled = 1;
	lcdp->bl_enabled = true;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	lcd_calc_judge_line(lcd);
	disp_al_lcd_disable_irq(lcd->hwdev_index, LCD_IRQ_TCON0_VBLK, &lcdp->panel_info);
	if ((LCD_IF_DSI == lcdp->panel_info.lcd_if) && (0 != lcdp->irq_no_dsi)) {
		disp_sys_register_irq(lcdp->irq_no_dsi,0,disp_lcd_event_proc,(void*)lcd,0,0);
		disp_sys_enable_irq(lcdp->irq_no_dsi);
	} else {
		disp_sys_register_irq(lcdp->irq_no,0,disp_lcd_event_proc,(void*)lcd,0,0);
		disp_sys_enable_irq(lcdp->irq_no);
	}
	disp_al_lcd_enable_irq(lcd->hwdev_index, LCD_IRQ_TCON0_VBLK, &lcdp->panel_info);

	return 0;
}

s32 disp_lcd_is_enabled(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	return (s32)lcdp->enabled;
}

static s32 disp_lcd_set_open_func(struct disp_device* lcd, LCD_FUNC func, u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (func) {
		lcdp->open_flow.func[lcdp->open_flow.func_num].func = func;
		lcdp->open_flow.func[lcdp->open_flow.func_num].delay = delay;
		lcdp->open_flow.func_num ++;
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_set_close_func(struct disp_device* lcd, LCD_FUNC func, u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (func) {
		lcdp->close_flow.func[lcdp->close_flow.func_num].func = func;
		lcdp->close_flow.func[lcdp->close_flow.func_num].delay = delay;
		lcdp->close_flow.func_num ++;
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_set_panel_funs(struct disp_device* lcd, char *name, disp_lcd_panel_fun * lcd_cfg)
{
	char primary_key[20], drv_name[32];
	s32 ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	sprintf(primary_key, "lcd%d", lcd->disp);

	ret = disp_sys_script_get_item(primary_key, "lcd_driver_name",  (int*)drv_name, 2);
	DE_INF("lcd %d, driver_name %s,  panel_name %s\n", lcd->disp, drv_name, name);
	if ((2==ret) && !strcmp(drv_name, name)) {
		memset(&lcdp->lcd_panel_fun, 0, sizeof(disp_lcd_panel_fun));
		lcdp->lcd_panel_fun.cfg_panel_info= lcd_cfg->cfg_panel_info;
		lcdp->lcd_panel_fun.cfg_open_flow = lcd_cfg->cfg_open_flow;
		lcdp->lcd_panel_fun.cfg_close_flow = lcd_cfg->cfg_close_flow;
		lcdp->lcd_panel_fun.lcd_user_defined_func = lcd_cfg->lcd_user_defined_func;
		lcdp->lcd_panel_fun.set_bright = lcd_cfg->set_bright;
		return 0;
	}

	return -1;
}

s32 disp_lcd_gpio_init(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i = 0;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	//io-pad
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++)
	{
		if (!((!strcmp(lcdp->lcd_cfg.lcd_gpio_power[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_gpio_power[i], "none"))))
			disp_sys_power_enable(lcdp->lcd_cfg.lcd_gpio_power[i]);
	}

	for (i=0; i<LCD_GPIO_NUM; i++) {
		lcdp->lcd_cfg.gpio_hdl[i] = 0;

		if (lcdp->lcd_cfg.lcd_gpio_used[i]) {
			disp_gpio_set_t  gpio_info[1];

			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_gpio[i]), sizeof(disp_gpio_set_t));
			lcdp->lcd_cfg.gpio_hdl[i] = disp_sys_gpio_request(gpio_info, 1);
		}
	}

	return 0;
}

s32 disp_lcd_gpio_exit(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i = 0;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	for (i=0; i<LCD_GPIO_NUM; i++) {
		if (lcdp->lcd_cfg.gpio_hdl[i]) {
			disp_gpio_set_t  gpio_info[1];

			disp_sys_gpio_release(lcdp->lcd_cfg.gpio_hdl[i], 2);

			memcpy(gpio_info, &(lcdp->lcd_cfg.lcd_gpio[i]), sizeof(disp_gpio_set_t));
			gpio_info->mul_sel = 7;
			lcdp->lcd_cfg.gpio_hdl[i] = disp_sys_gpio_request(gpio_info, 1);
			disp_sys_gpio_release(lcdp->lcd_cfg.gpio_hdl[i], 2);
			lcdp->lcd_cfg.gpio_hdl[i] = 0;
		}
	}

	//io-pad
	for (i = LCD_GPIO_REGU_NUM-1; i>=0; i--)
	{
		if (!((!strcmp(lcdp->lcd_cfg.lcd_gpio_power[i], "")) || (!strcmp(lcdp->lcd_cfg.lcd_gpio_power[i], "none"))))
			disp_sys_power_disable(lcdp->lcd_cfg.lcd_gpio_power[i]);
	}

	return 0;
}

//direction: input(0), output(1)
s32 disp_lcd_gpio_set_direction(struct disp_device* lcd, u32 io_index, u32 direction)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	return  disp_sys_gpio_set_direction(lcdp->lcd_cfg.gpio_hdl[io_index], direction, gpio_name);
}

s32 disp_lcd_gpio_get_value(struct disp_device* lcd,u32 io_index)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	return disp_sys_gpio_get_value(lcdp->lcd_cfg.gpio_hdl[io_index], gpio_name);
}

s32 disp_lcd_gpio_set_value(struct disp_device* lcd, u32 io_index, u32 data)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	return disp_sys_gpio_set_value(lcdp->lcd_cfg.gpio_hdl[io_index], data, gpio_name);
}

static s32 disp_lcd_get_dimensions(struct disp_device *lcd, u32 *width, u32 *height)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*width = lcdp->panel_info.lcd_width;
	*height = lcdp->panel_info.lcd_height;
	return 0;
}

static s32 disp_lcd_get_status(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	return disp_al_device_get_status(lcd->hwdev_index);
}

static s32 disp_lcd_get_fps(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	return lcdp->frame_per_sec;
}

static s32 disp_lcd_init(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("lcd %d\n", lcd->disp);

	lcd_get_sys_config(lcd->disp, &lcdp->lcd_cfg);
	if (disp_lcd_is_used(lcd)) {
		struct disp_video_timings *timmings;
		disp_panel_para *panel_info;
		lcd_parse_panel_para(lcd->disp, &lcdp->panel_info);
		timmings = &lcd->timings;
		panel_info = &lcdp->panel_info;
		timmings->pixel_clk = panel_info->lcd_dclk_freq * 1000;
		timmings->x_res = panel_info->lcd_x;
		timmings->y_res = panel_info->lcd_y;
		timmings->hor_total_time= panel_info->lcd_ht;
		timmings->hor_sync_time= panel_info->lcd_hspw;
		timmings->hor_back_porch= panel_info->lcd_hbp-panel_info->lcd_hspw;
		timmings->hor_front_porch= panel_info->lcd_ht-panel_info->lcd_hbp - panel_info->lcd_x;
		timmings->ver_total_time= panel_info->lcd_vt;
		timmings->ver_sync_time= panel_info->lcd_vspw;
		timmings->ver_back_porch= panel_info->lcd_vbp-panel_info->lcd_vspw;
		timmings->ver_front_porch= panel_info->lcd_vt-panel_info->lcd_vbp -panel_info->lcd_y;
	}
	disp_lcd_bright_curve_init(lcd);

	if (disp_lcd_is_used(lcd)) {
		__u64 backlight_bright;
		__u64 period_ns, duty_ns;
		if (lcdp->panel_info.lcd_pwm_used) {
			lcdp->pwm_info.channel = lcdp->panel_info.lcd_pwm_ch;
			lcdp->pwm_info.polarity = lcdp->panel_info.lcd_pwm_pol;
			lcdp->pwm_info.dev = disp_sys_pwm_request(lcdp->panel_info.lcd_pwm_ch);

			if (lcdp->panel_info.lcd_pwm_freq != 0) {
				period_ns = 1000*1000*1000 / lcdp->panel_info.lcd_pwm_freq;
			} else {
				DE_WRN("lcd%d.lcd_pwm_freq is ZERO\n", lcd->disp);
				period_ns = 1000*1000*1000 / 1000;  //default 1khz
			}

			backlight_bright = lcdp->lcd_cfg.backlight_bright;

			duty_ns = (backlight_bright * period_ns) / 256;
			//DE_DBG("[PWM]backlight_bright=%d,period_ns=%d,duty_ns=%d\n",(u32)backlight_bright,(u32)period_ns, (u32)duty_ns);
			disp_sys_pwm_set_polarity(lcdp->pwm_info.dev, lcdp->pwm_info.polarity);
			lcdp->pwm_info.duty_ns = duty_ns;
			lcdp->pwm_info.period_ns = period_ns;
		}
		lcd_clk_init(lcd);
	}
#if defined (SUPPORT_EINK) && defined (EINK_PANEL_USED)
	disp_lcd_pin_cfg(lcd, 1);
#endif
	//lcd_panel_parameter_check(lcd->disp, lcd);
	return 0;
}
#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
static void disp_close_eink_panel_task(struct work_struct *work)//(unsigned long parg)
{
	struct disp_device*  plcd = NULL;
	plcd = disp_device_find(0, DISP_OUTPUT_TYPE_LCD);
	plcd->disable(plcd);
	diplay_finish_flag = 1;
	return;
}
#endif

static s32 disp_lcd_exit(struct disp_device* lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	lcd_clk_exit(lcd);

	return 0;
}

s32 disp_init_lcd(disp_bsp_init_para * para)
{
	u32 num_devices;
	u32 disp = 0;
	struct disp_device *lcd;
	struct disp_lcd_private_data *lcdp;
	u32 hwdev_index = 0;
	u32 num_devices_support_lcd = 0;

	DE_INF("disp_init_lcd\n");

#if defined(__LINUX_PLAT__)
	spin_lock_init(&lcd_data_lock);
#endif
	num_devices = bsp_disp_feat_get_num_devices();
	for (hwdev_index=0; hwdev_index<num_devices; hwdev_index++) {
		if (bsp_disp_feat_is_supported_output_types(hwdev_index, DISP_OUTPUT_TYPE_LCD))
			num_devices_support_lcd ++;
	}
	lcds = (struct disp_device *)kmalloc(sizeof(struct disp_device) * num_devices_support_lcd,
		GFP_KERNEL | __GFP_ZERO);
	if (NULL == lcds) {
		DE_WRN("malloc memory(%d bytes) fail!\n",
			(unsigned int)sizeof(struct disp_device) * num_devices_support_lcd);
		return DIS_FAIL;
	}
	lcd_private = (struct disp_lcd_private_data *)kmalloc(sizeof(struct disp_lcd_private_data)\
		* num_devices_support_lcd, GFP_KERNEL | __GFP_ZERO);
	if (NULL == lcd_private) {
		DE_WRN("malloc memory(%d bytes) fail!\n",
			(unsigned int)sizeof(struct disp_lcd_private_data) * num_devices_support_lcd);
		return DIS_FAIL;
	}

	disp = 0;
	for (hwdev_index=0; hwdev_index<num_devices; hwdev_index++) {
		if (!bsp_disp_feat_is_supported_output_types(hwdev_index, DISP_OUTPUT_TYPE_LCD)) {
			continue;
		}
		lcd = &lcds[disp];
		lcdp = &lcd_private[disp];
		lcd->priv_data = (void*)lcdp;

		sprintf(lcd->name, "lcd%d", disp);
		lcd->disp = disp;
		lcd->hwdev_index = hwdev_index;
		lcd->type = DISP_OUTPUT_TYPE_LCD;
		lcdp->irq_no = para->irq_no[DISP_MOD_LCD0 + hwdev_index];
		lcdp->clk = para->mclk[DISP_MOD_LCD0 + hwdev_index];
		lcdp->lvds_clk = para->mclk[DISP_MOD_LVDS];
#if defined(SUPPORT_DSI)
		lcdp->irq_no_dsi = para->irq_no[DISP_MOD_DSI0];
		lcdp->dsi_clk0 = para->mclk[DISP_MOD_DSI0];
		lcdp->dsi_clk1 = para->mclk[DISP_MOD_DSI1];
#endif
		DE_INF("lcd %d, irq_no=%d, irq_no_dsi=%d\n", disp, lcdp->irq_no, lcdp->irq_no_dsi);

		lcd->set_manager = disp_device_set_manager;
		lcd->unset_manager = disp_device_unset_manager;
		lcd->get_resolution = disp_device_get_resolution;
		lcd->get_timings = disp_device_get_timings;
		lcd->enable = disp_lcd_enable;
		lcd->sw_enable = disp_lcd_sw_enable;
		lcd->fake_enable = disp_lcd_fake_enable;
		lcd->disable = disp_lcd_disable;
		lcd->is_enabled = disp_lcd_is_enabled;
		lcd->set_bright = disp_lcd_set_bright;
		lcd->get_bright = disp_lcd_get_bright;
		lcd->set_bright_dimming = disp_lcd_set_bright_dimming;
		lcd->get_panel_info = disp_lcd_get_panel_info;

		lcd->set_panel_func = disp_lcd_set_panel_funs;
		lcd->set_open_func = disp_lcd_set_open_func;
		lcd->set_close_func = disp_lcd_set_close_func;
		lcd->backlight_enable = disp_lcd_backlight_enable;
		lcd->backlight_disable = disp_lcd_backlight_disable;
		lcd->pwm_enable = disp_lcd_pwm_enable;
		lcd->pwm_disable = disp_lcd_pwm_disable;
		lcd->power_enable = disp_lcd_power_enable;
		lcd->power_disable = disp_lcd_power_disable;
		lcd->pin_cfg = disp_lcd_pin_cfg;
		lcd->tcon_enable = disp_lcd_tcon_enable;
		lcd->tcon_disable = disp_lcd_tcon_disable;
		lcd->gpio_set_value = disp_lcd_gpio_set_value;
		lcd->gpio_set_direction = disp_lcd_gpio_set_direction;
		lcd->get_dimensions = disp_lcd_get_dimensions;
		lcd->get_status = disp_lcd_get_status;
		lcd->get_fps = disp_lcd_get_fps;

		lcd->init = disp_lcd_init;
		lcd->exit = disp_lcd_exit;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
		INIT_WORK(&lcd->close_eink_panel_work, disp_close_eink_panel_task);
#endif
		lcd->init(lcd);
		disp_device_register(lcd);
		disp ++;
	}

	return 0;
}
