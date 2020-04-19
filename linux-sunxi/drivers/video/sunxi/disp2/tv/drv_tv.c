/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "drv_tv.h"

#include <linux/regulator/consumer.h>
#include <linux/clk-private.h>

static int suspend;
struct tv_info_t g_tv_info;
static unsigned int cali[4] = {625, 625, 625, 625};
static int offset[4] = {0, 0, 0, 0};

static struct disp_video_timings video_timing[] = {
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_NTSC,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 60,
		.hor_front_porch = 16,
		.hor_sync_time = 62,
		.ver_total_time = 525,
		.ver_back_porch = 30,
		.ver_front_porch = 9,
		.ver_sync_time = 6,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,

	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_PAL,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 68,
		.hor_front_porch = 12,
		.hor_sync_time = 64,
		.ver_total_time = 625,
		.ver_back_porch = 39,
		.ver_front_porch = 5,
		.ver_sync_time = 5,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 57,
		.hor_front_porch = 62,
		.hor_sync_time = 19,
		.ver_total_time = 525,
		.ver_back_porch = 4,
		.ver_front_porch = 1,
		.ver_sync_time = 3,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576I,
		.pixel_clk = 216000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 69,
		.hor_front_porch = 63,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 2,
		.ver_front_porch = 44,
		.ver_sync_time = 3,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_480P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 480,
		.hor_total_time = 858,
		.hor_back_porch = 60,
		.hor_front_porch = 62,
		.hor_sync_time = 16,
		.ver_total_time = 525,
		.ver_back_porch = 9,
		.ver_front_porch = 30,
		.ver_sync_time = 6,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_576P,
		.pixel_clk = 54000000,
		.pixel_repeat = 0,
		.x_res = 720,
		.y_res = 576,
		.hor_total_time = 864,
		.hor_back_porch = 68,
		.hor_front_porch = 64,
		.hor_sync_time = 12,
		.ver_total_time = 625,
		.ver_back_porch = 5,
		.ver_front_porch = 39,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1650,
		.hor_back_porch = 220,
		.hor_front_porch = 40,
		.hor_sync_time = 110,
		.ver_total_time = 750,
		.ver_back_porch = 5,
		.ver_front_porch = 20,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_720P_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1980,
		.hor_back_porch = 220,
		.hor_front_porch = 40,
		.hor_sync_time = 440,
		.ver_total_time = 750,
		.ver_back_porch = 5,
		.ver_front_porch = 20,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_60HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 88,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080I_50HZ,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 2,
		.ver_front_porch = 38,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 1,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_60HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 88,
		.ver_total_time = 1125,
		.ver_back_porch = 4,
		.ver_front_porch = 36,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_TV_MOD_1080P_50HZ,
		.pixel_clk = 148500000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2640,
		.hor_back_porch = 148,
		.hor_front_porch = 44,
		.hor_sync_time = 528,
		.ver_total_time = 1125,
		.ver_back_porch = 4,
		.ver_front_porch = 36,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_800_600P_60,
		.pixel_clk = 40000000,
		.pixel_repeat = 0,
		.x_res = 800,
		.y_res = 600,
		.hor_total_time = 1056,
		.hor_back_porch = 88,
		.hor_front_porch = 40,
		.hor_sync_time = 128,
		.ver_total_time = 628,
		.ver_back_porch = 23,
		.ver_front_porch = 1,
		.ver_sync_time = 4,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1024_768P_60,
		.pixel_clk = 65000000,
		.pixel_repeat = 0,
		.x_res = 1024,
		.y_res = 768,
		.hor_total_time = 1344,
		.hor_back_porch = 160,
		.hor_front_porch = 24,
		.hor_sync_time = 136,
		.ver_total_time = 806,
		.ver_back_porch = 29,
		.ver_front_porch = 3,
		.ver_sync_time = 6,
		.hor_sync_polarity = 0,/* 0: negative, 1: positive */
		.ver_sync_polarity = 0,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1280_720P_60,
		.pixel_clk = 74250000,
		.pixel_repeat = 0,
		.x_res = 1280,
		.y_res = 720,
		.hor_total_time = 1650,
		.hor_back_porch = 220,
		.hor_front_porch = 110,
		.hor_sync_time = 40,
		.ver_total_time = 750,
		.ver_back_porch = 20,
		.ver_front_porch = 5,
		.ver_sync_time = 5,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
	{
		.vic = 0,
		.tv_mode = DISP_VGA_MOD_1920_1080P_60,
		.pixel_clk = 14850000,
		.pixel_repeat = 0,
		.x_res = 1920,
		.y_res = 1080,
		.hor_total_time = 2200,
		.hor_back_porch = 88,
		.hor_front_porch = 40,
		.hor_sync_time = 128,
		.ver_total_time = 628,
		.ver_back_porch = 23,
		.ver_front_porch = 1,
		.ver_sync_time = 4,
		.hor_sync_polarity = 1,/* 0: negative, 1: positive */
		.ver_sync_polarity = 1,/* 0: negative, 1: positive */
		.b_interlace = 0,
		.vactive_space = 0,
		.trd_mode = 0,
	},
};

#define TVE_CHECK_PARAM(sel) \
	do { if (sel >= TVE_DEVICE_NUM) {\
		pr_warn("%s, sel(%d) is out of range\n", __func__, sel);\
		return -1;\
		} \
	} while (0)

/* #define TVDEBUG */
#if defined(TVDEBUG)
#define TV_DBG(fmt, arg...)   pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define TV_DBG(fmt, arg...)
#endif
#define TV_ERR(fmt, arg...)   pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
static struct task_struct *tve_task;
static u32 tv_hpd[SCREEN_COUNT];
static struct switch_dev switch_dev[SCREEN_COUNT];
static bool is_compatible_cvbs;

/* this switch is used for the purpose of compatible platform */
static struct switch_dev switch_cvbs = {
	.name = "cvbs",
};

static char switch_name[20];

void tv_report_hpd_work(u32 sel, u32 hpd)
{
	if (tv_hpd[sel] == hpd)
		return;

	switch (hpd) {
	case STATUE_CLOSE:
		switch_set_state(&switch_dev[sel], STATUE_CLOSE);
		if (is_compatible_cvbs)
			switch_set_state(&switch_cvbs, STATUE_CLOSE);
		break;

	case STATUE_OPEN:
		switch_set_state(&switch_dev[sel], STATUE_OPEN);
		if (is_compatible_cvbs)
			switch_set_state(&switch_cvbs, STATUE_OPEN);
		break;

	default:
		switch_set_state(&switch_dev[sel], STATUE_CLOSE);
		break;
	}
	tv_hpd[sel] = hpd;
}

s32 tv_detect_thread(void *parg)
{
	s32 hpd[SCREEN_COUNT];
	int i = 0;
	while(1) {
		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(20);

		if (!suspend) {
			for (i = 0; i < SCREEN_COUNT; i++) {
				hpd[i] = tve_low_get_dac_status(i);
				if (hpd[i] != tv_hpd[i]) {
					TV_DBG("hpd[%d] = %d\n", i, hpd[i]);
					tv_report_hpd_work(i, hpd[i]);
				}
			}
		}
	}
	return 0;
}

s32 tv_detect_enable(u32 sel)
{
	tve_low_dac_autocheck_enable(sel);
	/* only one thread to detect hot pluging */
	if (!tve_task) {
		tve_task = kthread_create(tv_detect_thread, (void *)0,
						"tve detect");
		if (IS_ERR(tve_task)) {
			s32 err = 0;
			err = PTR_ERR(tve_task);
			tve_task = NULL;
			return err;
		}
		else {
			TV_DBG("tve_task is ok!\n");
		}
		wake_up_process(tve_task);
	}
	return 0;
}

s32 tv_detect_disable(u32 sel)
{
	if (tve_task) {
		kthread_stop(tve_task);
		tve_task = NULL;
		tve_low_dac_autocheck_disable(sel);
	}
	return 0;
}
#else
void tv_report_hpd_work(u32 sel, u32 hpd)
{
	pr_debug("there is null report hpd work,you need support the switch class!");
}

s32 tv_detect_thread(void *parg)
{
	pr_debug("there is null tv_detect_thread,you need support the switch class!");
	return -1;
}

s32 tv_detect_enable(u32 sel)
{
	pr_debug("there is null tv_detect_enable,you need support the switch class!");
	return -1;
}

s32 tv_detect_disable(u32 sel)
{
	pr_debug("there is null tv_detect_disable,you need support the switch class!");
		return -1;
}
#endif

s32 tv_get_video_info(s32 mode)
{
	s32 i,count;
	count = sizeof(video_timing)/sizeof(struct disp_video_timings);
	for(i=0;i<count;i++) {
		if(mode == video_timing[i].tv_mode)
			return i;
	}
	return -1;
}

s32 tv_get_list_num(void)
{
	return sizeof(video_timing)/sizeof(struct disp_video_timings);
}

s32 tv_set_enhance_mode(u32 sel, u32 mode)
{
	return tve_low_enhance(sel, mode);
}

static void tve_clk_init(u32 sel)
{
	g_tv_info.clk_parent = clk_get_parent(g_tv_info.clk);
}

#if defined(TVE_TOP_SUPPORT)
static int tve_top_clk_enable(void)
{
	int ret;

	ret = clk_prepare_enable(g_tv_info.clk);
	if (0 != ret) {
		pr_warn("fail to enable tve's top clk!\n");
		return ret;
	}

	return 0;
}

static int tve_top_clk_disable(void)
{
	clk_disable(g_tv_info.clk);
	return 0;
}
#else
static int tve_top_clk_enable(void) {return 0; }
static int tve_top_clk_disable(void) {return 0; }
#endif

static int tve_clk_enable(u32 sel)
{
	int ret;

	ret = clk_prepare_enable(g_tv_info.screen[sel].clk);
	if (0 != ret) {
		pr_warn("fail to enable tve%d's clk!\n", sel);
		return ret;
	}

	return 0;
}

static int tve_clk_disable(u32 sel)
{
	clk_disable(g_tv_info.screen[sel].clk);
	return 0;
}

static void tve_clk_config(u32 sel, u32 tv_mode)
{
	struct disp_video_timings *info = video_timing;
	int i, list_num, rc;
	int ret = 0;
	bool find = false;
	unsigned long rate = 0, prate = 0;
	unsigned long parent_rate[] = {216000000, 297000000, 240000000};
	unsigned long round;

	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		if(info->tv_mode == tv_mode) {
			find = true;
			break;
		}
		info++;
	}
	if (!find) {
		pr_err("tv have no mode(%d)!\n", tv_mode);
		return;
	}

	rate = info->pixel_clk;
	rc = sizeof(parent_rate)/sizeof(unsigned long);
	for (i = 0; i < rc; i++) {
		if (!(parent_rate[i] % rate)) {
			prate = parent_rate[i];
			break;
		}
	}
	if (!prate) {
		prate = 984000000;
		pr_err("not find suitable parent rate, set max rate.\n");
	}

	pr_debug("parent count = %d, prate=%lu, rate=%lu, tv_mode=%d\n",
		rc, prate, rate, tv_mode);

	round = clk_round_rate(g_tv_info.screen[sel].clk, rate);
	if (round != rate) {
		ret = clk_set_rate(g_tv_info.screen[sel].clk->parent, prate);
		if (ret)
			pr_warn("fail to set rate(%ld) fo tve%d's pclk!\n",
			    prate, sel);
	}

	ret = clk_set_rate(g_tv_info.screen[sel].clk, rate);
	if (ret)
		pr_warn("fail to set rate(%ld) fo tve%d's clock!\n", rate, sel);
}

static s32 __get_offset(struct device_node *node, int i)
{
	char sub_key[20] = {0};
	s32 value = 0;
	int ret = 0;

	snprintf(sub_key, sizeof(sub_key), "dac_offset%d", i);
	ret = of_property_read_u32(node, sub_key, (u32 *)&value);
	if (ret < 0) {
		pr_warn("there is no tve dac(%d) offset value.\n", i);
	} else {
		/* Sysconfig can not use signed params, however,
		 * dac_offset as a signed param which ranges from
		 * -100 to 100, is maping sysconfig params from
		 * 0 to 200.
		*/
		if ((value > 200) || (value < 0))
			pr_err("dac offset is out of range.\n");
		else
			return value - 100;
	}

	return 0;
}

s32 tv_init(struct platform_device *pdev)
{
	s32 i = 0, ret = 0;
	u32 cali_value = 0, sel = pdev->id;
	char sub_key[20] = {0};
	unsigned int value, output_type, output_mode;
	unsigned int interface = 0;

	ret = of_property_read_u32(pdev->dev.of_node, "interface",
					&interface);
	if (ret < 0)
		pr_err("get tv interface failed!\n");
#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)


		if (interface == DISP_TV_CVBS) {
			snprintf(switch_name, sizeof(switch_name),
				"tve%d_cvbs", sel);
			switch_dev[sel].name = switch_name;
			/* if tve0 is the CVBS interface,
			 * than add creating another cvbs switch,
			 * which is compatible with old hardware of
			 * TVE module.
			 */
			if (sel == 0) {
				switch_dev_register(&switch_cvbs);
				is_compatible_cvbs = true;
			}
		} else if (interface == DISP_TV_YPBPR) {
			snprintf(switch_name, sizeof(switch_name),
				"tve%d_ypbpr", sel);
			switch_dev[sel].name = switch_name;
		} else if (interface == DISP_TV_SVIDEO) {
			snprintf(switch_name, sizeof(switch_name),
				"tve%d_svideo", sel);
			switch_dev[sel].name = switch_name;
		} else if (interface == DISP_VGA) {
			snprintf(switch_name, sizeof(switch_name),
				"tve%d_vga", sel);
			switch_dev[sel].name = switch_name;
		}
		switch_dev_register(&switch_dev[sel]);

#endif
		tve_top_clk_enable();
		/* get mapping dac */
		for (i = 0; i < DAC_COUNT; i++) {
			u32 value;
			u32 dac_no;

			snprintf(sub_key, sizeof(sub_key), "dac_src%d", i);
			ret = of_property_read_u32(pdev->dev.of_node,
							sub_key, &value);
			if (ret < 0) {
				pr_warn("tve%d have no dac %d\n", sel, i);
			} else {
				dac_no = value;
				g_tv_info.screen[sel].dac_no[i] = value;
				g_tv_info.screen[sel].dac_num++;
				cali_value = tve_low_get_sid(dac_no);

				pr_debug("cali_temp = %u\n", cali_value);
				/* VGA mode: 16~31 bits
				 * CVBS & YPBPR mode: 0~15 bits
				 */
				if (interface == DISP_VGA)
					cali[dac_no] = (cali_value >> 16)
							& 0xffff;
				else
					cali[dac_no] = cali_value & 0xffff;

				offset[dac_no] = __get_offset(pdev->dev.of_node,
								i);
				pr_debug("cali[%u] = %u, offset[%u] = %u\n",
					dac_no, cali[dac_no], dac_no,
					offset[dac_no]);
			}

			snprintf(sub_key, sizeof(sub_key), "dac_type%d", i);
			ret = of_property_read_u32(pdev->dev.of_node,
							sub_key, &value);
			if (ret < 0) {
				pr_warn("tve%d have no type%d\n", sel, i);
				/* if do'not config type, set disabled status */
				g_tv_info.screen[sel].dac_type[i] = 7;
			} else {
				g_tv_info.screen[sel].dac_type[i] = value;
			}
		}

		/* parse boot params */
		value = disp_boot_para_parse("boot_disp");
		output_type = (value >> (sel * 16) >>  8) & 0xff;
		output_mode = (value) >> (sel * 16) & 0xff;

#if TVE_DEVICE_NUM == 1
		/*
		 * On the platform that only support 1 channel tvout,
		 * We need to check if tvout be enabled at bootloader
		 * through all paths. So here we check the disp1 part.
		 */
		if ((0 == sel) && (output_type != DISP_OUTPUT_TYPE_TV)) {
			output_type = (value >> (1 * 16) >>  8) & 0xff;
			output_mode = (value) >> (1 * 16) & 0xff;
		}
#endif

		mutex_init(&g_tv_info.screen[sel].mlock);
		pr_debug("[TV]:value = %d, type = %d, mode = %d\n",
				value, output_type, output_mode);
		if ((output_type == DISP_OUTPUT_TYPE_TV)
			|| (output_type == DISP_OUTPUT_TYPE_VGA)) {
			g_tv_info.screen[sel].enable = 1;
			g_tv_info.screen[sel].tv_mode = output_mode;
			pr_debug("[TV]:g_tv_info.screen[0].tv_mode = %d",
				g_tv_info.screen[sel].tv_mode);
		} else {
			g_tv_info.screen[sel].tv_mode = DISP_TV_MOD_PAL;
		}

		tve_low_set_reg_base(sel, g_tv_info.screen[sel].base_addr);
		tve_clk_init(sel);
		tve_clk_config(sel, g_tv_info.screen[sel].tv_mode);
#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
		tve_clk_enable(sel);
#endif

		if ((output_type != DISP_OUTPUT_TYPE_TV)
			&& (output_type != DISP_OUTPUT_TYPE_VGA))
			tve_low_init(sel, &g_tv_info.screen[sel].dac_no[0],
					cali, offset,
					g_tv_info.screen[sel].dac_type,
					g_tv_info.screen[sel].dac_num);

		tv_detect_enable(sel);
	return 0;
}

s32 tv_exit(void)
{
	s32 i;

	for (i=0; i<g_tv_info.tv_number; i++)
		tv_detect_disable(i);

	for (i=0; i<g_tv_info.tv_number; i++)
		tv_disable(i);

	return 0;
}

s32 tv_get_mode(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	return g_tv_info.screen[sel].tv_mode;
}

s32 tv_set_mode(u32 sel, enum disp_tv_mode tv_mode)
{
	if(tv_mode >= DISP_TV_MODE_NUM) {
		return -1;
	}

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	mutex_lock(&g_tv_info.screen[sel].mlock);
	g_tv_info.screen[sel].tv_mode = tv_mode;
	mutex_unlock(&g_tv_info.screen[sel].mlock);
	return  0;
}

s32 tv_get_input_csc(u32 sel)
{
	/* vga interface is rgb mode. */
	if (is_vga_mode(g_tv_info.screen[sel].tv_mode))
		return 0;
	else
		return 1;
}

s32 tv_get_video_timing_info(u32 sel, struct disp_video_timings **video_info)
{
	struct disp_video_timings *info;
	int ret = -1;
	int i, list_num;
	info = video_timing;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		mutex_lock(&g_tv_info.screen[sel].mlock);
		if(info->tv_mode == g_tv_info.screen[sel].tv_mode){
			*video_info = info;
			ret = 0;
			mutex_unlock(&g_tv_info.screen[sel].mlock);
			break;
		}
		mutex_unlock(&g_tv_info.screen[sel].mlock);
		info ++;
	}
	return ret;
}

static int __pin_config(int sel, char *name)
{
	int ret = 0;
	char type_name[10] = {0};
	struct device_node *node;
	struct platform_device *pdev;
	struct pinctrl *pctl;
	struct pinctrl_state *state;

	snprintf(type_name, sizeof(type_name), "tv%d", sel);

	node = of_find_compatible_node(NULL, type_name, "allwinner,sunxi-tv");
	if (!node) {
		TV_ERR("of_find_tv_node %s fail\n", type_name);
		ret = -EINVAL;
		goto exit;
	}

	pdev = of_find_device_by_node(node);
	if (!node) {
		TV_ERR("of_find_device_by_node for %s fail\n", type_name);
		ret = -EINVAL;
		goto exit;
	}

	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		TV_ERR("pinctrl_get for %s fail\n", type_name);
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		TV_ERR("pinctrl_lookup_state for %s fail\n", type_name);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		TV_ERR("pinctrl_select_state(%s)fail\n", type_name);
		goto exit;
	}

exit:
	return ret;
}

s32 tv_enable(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	if(!g_tv_info.screen[sel].enable) {
		if (is_vga_mode(g_tv_info.screen[sel].tv_mode))
			__pin_config(sel, "active");

		tve_clk_config(sel, g_tv_info.screen[sel].tv_mode);
		tve_low_set_tv_mode(sel, g_tv_info.screen[sel].tv_mode,
					cali);
		tve_low_dac_enable(sel);
		tve_low_open(sel);
		mutex_lock(&g_tv_info.screen[sel].mlock);
		g_tv_info.screen[sel].enable = 1;
		mutex_unlock(&g_tv_info.screen[sel].mlock);
	}
	return 0;
}

s32 tv_disable(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	mutex_lock(&g_tv_info.screen[sel].mlock);
	if(g_tv_info.screen[sel].enable) {
		tve_low_close(sel);
		g_tv_info.screen[sel].enable = 0;
	}
	mutex_unlock(&g_tv_info.screen[sel].mlock);
	if (is_vga_mode(g_tv_info.screen[sel].tv_mode))
		__pin_config(sel, "sleep");
	return 0;
}

s32 tv_suspend(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	mutex_lock(&g_tv_info.screen[sel].mlock);
	if (!g_tv_info.screen[sel].suspend) {
		g_tv_info.screen[sel].suspend = true;
		tv_detect_disable(sel);
		tve_clk_disable(sel);
		tve_top_clk_disable();
	}
	suspend = 1;
	mutex_unlock(&g_tv_info.screen[sel].mlock);

	return 0;
}

s32 tv_resume(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);
	mutex_lock(&g_tv_info.screen[sel].mlock);
	suspend = 0;
	if (g_tv_info.screen[sel].suspend) {
		g_tv_info.screen[sel].suspend = false;
		tve_top_clk_enable();
		tve_clk_enable(sel);
		tve_low_init(sel, &g_tv_info.screen[sel].dac_no[0],
				cali, offset, g_tv_info.screen[sel].dac_type,
				g_tv_info.screen[sel].dac_num);
		tv_detect_enable(sel);
	}
	mutex_unlock(&g_tv_info.screen[sel].mlock);

	return  0;
}

s32 tv_mode_support(u32 sel, enum disp_tv_mode mode)
{
	u32 i, list_num;
	struct disp_video_timings *info;

	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

	info = video_timing;
	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		if(info->tv_mode == mode) {
			return 1;
		}
		info ++;
	}
	return 0;
}

s32 tv_hot_plugging_detect (u32 state)
{
	int i = 0;
	for(i=0; i<SCREEN_COUNT;i++) {
		if(state == STATUE_OPEN) {
			return tve_low_dac_autocheck_enable(i);
		}
		else if(state == STATUE_CLOSE){
			return tve_low_dac_autocheck_disable(i);
		}
	}
	return 0;
}

#if !defined(CONFIG_OF)
static struct resource tv_resource[1] =
{
	[0] = {
		.start = 0x01c16000,  //modify
		.end   = 0x01c165ff,
		.flags = IORESOURCE_MEM,
	},
};
#endif

#if !defined(CONFIG_OF)
struct platform_device tv_device =
{
	.name		   = "tv",
	.id				= -1,
	.num_resources  = ARRAY_SIZE(tv_resource),
	.resource		= tv_resource,
	.dev			= {}
};
#else
static const struct of_device_id sunxi_tv_match[] = {
	{ .compatible = "allwinner,sunxi-tv", },
	{},
};
#endif

#if defined(TVE_TOP_SUPPORT)
static int tv_top_init(struct platform_device *pdev)
{
	int ret;

	if (g_tv_info.tv_number)
		return 0;

	pdev->id = of_alias_get_id(pdev->dev.of_node, "tv");
	if (pdev->id < 0) {
		TV_DBG("failed to get alias id\n");
		return -EINVAL;
	}

	g_tv_info.base_addr = of_iomap(pdev->dev.of_node, 0);
	if (!g_tv_info.base_addr) {
		dev_err(&pdev->dev, "unable to map tve common registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	g_tv_info.clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(g_tv_info.clk)) {
		dev_err(&pdev->dev, "fail to get clk for tve common module!\n");
		goto err_iomap;
	}

	tve_low_set_top_reg_base(g_tv_info.base_addr);

	return 0;

err_iomap:
	if (g_tv_info.base_addr)
		iounmap((char __iomem *)g_tv_info.base_addr);
	return ret;
}
#endif

static int tv_probe(struct platform_device *pdev)
{
	struct disp_tv_func disp_func;
	int index = 0;

	if (!g_tv_info.tv_number)
		memset(&g_tv_info, 0, sizeof(struct tv_info_t));

#if defined(TVE_TOP_SUPPORT)
	tv_top_init(pdev);
	index = 1;
#endif

	pdev->id = of_alias_get_id(pdev->dev.of_node, "tv");
	if (pdev->id < 0) {
		TV_DBG("failed to get alias id\n");
		return -EINVAL;
	}

	g_tv_info.screen[pdev->id].base_addr =
	    of_iomap(pdev->dev.of_node, index);
	if (IS_ERR_OR_NULL(g_tv_info.screen[pdev->id].base_addr)) {
		dev_err(&pdev->dev, "fail to get addr for tve%d!\n", pdev->id);
		goto err_iomap;
	}

	g_tv_info.screen[pdev->id].clk = of_clk_get(pdev->dev.of_node, index);
	if (IS_ERR_OR_NULL(g_tv_info.screen[pdev->id].clk)) {
		dev_err(&pdev->dev, "fail to get clk for tve%d's!\n", pdev->id);
		goto err_iomap;
	}

	tv_init(pdev);

	/* register device only once */
	if (!g_tv_info.tv_number) {
		memset(&disp_func, 0, sizeof(struct disp_tv_func));
		disp_func.tv_enable = tv_enable;
		disp_func.tv_disable = tv_disable;
		disp_func.tv_resume = tv_resume;
		disp_func.tv_suspend = tv_suspend;
		disp_func.tv_get_mode = tv_get_mode;
		disp_func.tv_set_mode = tv_set_mode;
		disp_func.tv_get_video_timing_info = tv_get_video_timing_info;
		disp_func.tv_get_input_csc = tv_get_input_csc;
		disp_func.tv_mode_support = tv_mode_support;
		disp_func.tv_hot_plugging_detect = tv_hot_plugging_detect;
		disp_func.tv_set_enhance_mode = tv_set_enhance_mode;
		disp_tv_register(&disp_func);
	}
	g_tv_info.tv_number++;

	return 0;
err_iomap:
	if (g_tv_info.base_addr)
		iounmap((char __iomem *)g_tv_info.base_addr);

	if (g_tv_info.screen[pdev->id].base_addr)
		iounmap((char __iomem *)g_tv_info.screen[pdev->id].base_addr);

	return -EINVAL;
}

static int tv_remove(struct platform_device *pdev)
{
	tv_exit();
	return 0;
}

/*int tv_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}*/

/*int tv_resume(struct platform_device *pdev)
{
	return 0;
}*/
static struct platform_driver tv_driver =
{
	.probe	  = tv_probe,
	.remove	 = tv_remove,
 //   .suspend	= tv_suspend,
  //  .resume	 = tv_resume,
	.driver	 =
	{
		.name   = "tv",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_tv_match,
	},
};

int tv_open(struct inode *inode, struct file *file)
{
	return 0;
}

int tv_release(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t tv_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

ssize_t tv_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

int tv_mmap(struct file *file, struct vm_area_struct * vma)
{
	return 0;
}

long tv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations tv_fops =
{
	.owner		= THIS_MODULE,
	.open		= tv_open,
	.release	= tv_release,
	.write	  = tv_write,
	.read		= tv_read,
	.unlocked_ioctl	= tv_ioctl,
	.mmap	   = tv_mmap,
};

int __init tv_module_init(void)
{
	int ret = 0;

#if !defined(CONFIG_OF)
	ret = platform_device_register(&tv_device);
#endif
	if (ret == 0) {
		ret = platform_driver_register(&tv_driver);
	}
	return ret;
}

static void __exit tv_module_exit(void)
{
	platform_driver_unregister(&tv_driver);
#if !defined(CONFIG_OF)
	platform_device_unregister(&tv_device);
#endif
}

late_initcall(tv_module_init);
module_exit(tv_module_exit);

MODULE_AUTHOR("zengqi");
MODULE_DESCRIPTION("tv driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:tv");
