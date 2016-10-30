/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "drv_tv.h"

static int suspend;
struct tv_info_t g_tv_info;
static unsigned int cali[4] = {625, 625, 625, 625};
static unsigned int offset[4] = {0, 0, 0, 0};
extern s32 disp_set_tv_func(struct disp_tv_func * func);
extern uintptr_t disp_getprop_regbase(char *main_name,
					char *sub_name, u32 index);

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
		printf("%s, sel(%d) is out of range\n", __func__, sel);\
		return -1;\
		} \
	} while (0)

/* #define TVDEBUG */
#if defined(TVDEBUG)
#define TV_DBG(fmt, arg...)   printf("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define TV_DBG(fmt, arg...)
#endif
#define TV_ERR(fmt, arg...)   printf("%s()%d - "fmt, __func__, __LINE__, ##arg)

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
static struct task_struct *tve_task;
static u32 tv_hpd[SCREEN_COUNT];
static struct switch_dev switch_dev[SCREEN_COUNT];
static char switch_name[20];

void tv_report_hpd_work(u32 sel, u32 hpd)
{
	if (tv_hpd[sel] == hpd)
		return;

	switch (hpd) {
	case STATUE_CLOSE:
		switch_set_state(&switch_dev[sel], STATUE_CLOSE);
		break;

	case STATUE_OPEN:
		switch_set_state(&switch_dev[sel], STATUE_OPEN);
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

		if (!suspend) {
			for (i = 0; i < SCREEN_COUNT; i++) {
				hpd[i] = tve_low_get_dac_status(i);
				if (hpd[i] != tv_hpd[i]) {
					TV_DBG("hpd[%d] = %d\n", i, hpd[i]);
					tv_report_hpd_work(i, hpd[i]);
				}
			}
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(20);
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
	printf("no report hpd work,you need support the switch class!\n");
}

s32 tv_detect_thread(void *parg)
{
	printf("no report hpd work,you need support the switch class!\n");
	return -1;
}

s32 tv_detect_enable(u32 sel)
{
	tve_low_dac_autocheck_enable(sel);
	printf("no report hpd work,you need support the switch class!\n");
	return -1;
}

s32 tv_detect_disable(u32 sel)
{
	tve_low_dac_autocheck_disable(sel);
	printf("no report hpd work,you need support the switch class!\n");
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
	g_tv_info.clk_parent = clk_get_parent(g_tv_info.screen[sel].clk);
}

static int tve_top_clk_enable(void)
{
	int ret;

	ret = clk_prepare_enable(g_tv_info.clk);
	if (0 != ret) {
		printf("fail to enable tve's top clk!\n");
		return ret;
	}

	return 0;
}

static int tve_top_clk_disable(void)
{
	clk_disable(g_tv_info.clk);
	return 0;
}

static int tve_clk_enable(u32 sel)
{
	int ret;

	ret = clk_prepare_enable(g_tv_info.screen[sel].clk);
	if (0 != ret) {
		printf("fail to enable tve%d's clk!\n", sel);
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
	bool rate_exact = false;
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
		printf("tv have no mode(%d)!\n", tv_mode);
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
		printf("not find suitable parent rate, set max rate.\n");
	}

	TV_DBG("parent count = %d, prate=%lu, rate=%lu, tv_mode=%d\n",
		rc, prate, rate, tv_mode);

	round = clk_round_rate(g_tv_info.screen[sel].clk, rate);
	if (round == rate)
		rate_exact = true;

	if (!rate_exact) {
		ret = clk_set_rate(g_tv_info.screen[sel].clk->parent, prate);
		if (ret)
			printf("fail to set rate(%ld) fo tve%d's pclk!\n",
			    prate, sel);
	}
	ret = clk_set_rate(g_tv_info.screen[sel].clk, rate);
	if (ret)
		printf("fail to set rate(%ld) fo tve%d's clock!\n", rate, sel);
}

static s32 tv_inside_init(int sel)
{
	s32 i = 0, ret = 0;
	u32 sid = 0;
	char main_key[20];
	char sub_key[20];

	sprintf(main_key, "tv%d", sel);

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)
		unsigned int interface = 0;

		ret = of_property_read_u32(pdev->dev.of_node, "interface",
						&interface);
		if (ret < 0)
			pr_err("get tv interface failed!\n");

		if (interface == DISP_TV_CVBS) {
			snprintf(switch_name, sizeof(switch_name),
				"tve%d_cvbs", sel);
			switch_dev[sel].name = switch_name;
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
#if defined(TVE_TOP_SUPPORT)
		tve_top_clk_enable();
#endif
		/* get mapping dac */
		for (i = 0; i < DAC_COUNT; i++) {
			int value;
			sprintf(sub_key, "dac_src%d", i);
			ret = disp_sys_script_get_item(main_key, sub_key, &value, 1);
			if (ret != 1) {
			} else {
				g_tv_info.screen[sel].dac_no[i] = value;
				g_tv_info.screen[sel].dac_num++;
			}

			sprintf(sub_key, "dac_type%d", i);
			ret = disp_sys_script_get_item(main_key,
							sub_key, &value, 1);
			if (ret != 1) {
				printf("tv%d have no type%d\n", sel, i);
				/* if do'not config type, set disabled status */
				g_tv_info.screen[sel].dac_type[i] = 7;
			} else {
				g_tv_info.screen[sel].dac_type[i] = value;
			}
		}

		sid = tve_low_get_sid(0x10);
		if (0 == sid)
			g_tv_info.screen[sel].sid = 0x200;
		else
			g_tv_info.screen[sel].sid = sid;

		tve_low_set_reg_base(sel, g_tv_info.screen[sel].base_addr);
		tve_clk_init(sel);
		tve_clk_config(sel, g_tv_info.screen[sel].tv_mode);
		tve_clk_enable(sel);
		tve_low_init(sel, &g_tv_info.screen[sel].dac_no[0],
				cali, offset, g_tv_info.screen[sel].dac_type,
				g_tv_info.screen[sel].dac_num);

		tv_detect_enable(sel);
	return 0;
}

#if defined(TVE_TOP_SUPPORT)
static int tv_top_init(int sel)
{
	int ret;
	char main_key[20];
	int node_offset = 0;

	sprintf(main_key, "tv%d", sel);

	if (g_tv_info.tv_number)
		return 0;

	if (sel < 0) {
		TV_DBG("failed to get alias id\n");
		return -EINVAL;
	}

	g_tv_info.base_addr = (void __iomem *)disp_getprop_regbase(main_key,
								"reg", 0);

	if (!g_tv_info.base_addr) {
		printf("unable to map tve common registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}

	node_offset = disp_fdt_nodeoffset(main_key);
	g_tv_info.clk = of_clk_get(node_offset, 0);

	if (IS_ERR(g_tv_info.clk)) {
		printf("fail to get clk for tve common module!\n");
		goto err_iomap;
	}

	tve_low_set_top_reg_base(g_tv_info.base_addr);

	return 0;

err_iomap:
	return ret;
}
#endif

static int tv_probe(int sel)
{
	struct disp_tv_func disp_func;
	char main_key[20];
	int index = 0;
	int node_offset = 0;
	int ret = 0;
	int value = 0;
	printf("%s:000\n", __func__);

	sprintf(main_key, "tv%d", sel);

	ret = disp_sys_script_get_item(main_key, "boot_mask", &value, 1);
	if (ret == 1 && value == 1) {
		printf("skip tv%d in boot.\n", sel);
		return -1;
	}

	if (!g_tv_info.tv_number)
		memset(&g_tv_info, 0, sizeof(struct tv_info_t));

#if defined(TVE_TOP_SUPPORT)
	tv_top_init(sel);
	index = 1;
#endif

	if (sel < 0) {
		TV_DBG("failed to get alias id\n");
		return -EINVAL;
	}

	g_tv_info.screen[sel].base_addr = (void __iomem *)disp_getprop_regbase(
								main_key,
								"reg", index);
	if (!g_tv_info.screen[sel].base_addr) {
		printf("fail to get addr for tv%d!\n", sel);
		goto err_iomap;
	}

	node_offset = disp_fdt_nodeoffset(main_key);
	of_periph_clk_config_setup(node_offset);
	g_tv_info.screen[sel].clk = of_clk_get(node_offset, index);
	if (!g_tv_info.screen[sel].clk) {
		printf("fail to get clk for tve%d's!\n", sel);
		goto err_iomap;
	}

	g_tv_info.screen[sel].tv_mode = DISP_TV_MOD_PAL;
	tv_inside_init(sel);

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
		disp_func.tv_get_hpd_status = tv_get_hpd_status;

		//disp_set_tv_func(&disp_func);
		disp_tv_register(&disp_func);
	}
	g_tv_info.tv_number++;

	return 0;
err_iomap:
	/*
	if (g_tv_info.base_addr)
		iounmap((char __iomem *)g_tv_info.base_addr);

	if (g_tv_info.screen[sel].base_addr)
		iounmap((char __iomem *)g_tv_info.screen[sel].base_addr);
	*/
	return -EINVAL;
}

s32 tv_init(void)
{
	s32 i = 0, ret = 0;
	char main_key[20];
	char str[10];

	for (i = 0; i < 2; i++) {
		printf("%s:\n", __func__);
		sprintf(main_key, "tv%d", i);

		ret = disp_sys_script_get_item(main_key, "status", (int*)str, 2);
		if (ret != 2) {
			printf("fetch tv%d err.\n", i);
			continue;
		}
		if (0 != strcmp(str, "okay"))
			continue;
		tv_probe(i);
	}

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

	g_tv_info.screen[sel].tv_mode = tv_mode;
	return  0;
}

s32 tv_get_input_csc(u32 sel)
{
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
 		if(info->tv_mode == g_tv_info.screen[sel].tv_mode){
			*video_info = info;
			ret = 0;
 			break;
		}
 		info ++;
	}
	return ret;
}

s32 tv_get_hpd_status(u32 sel)
{
	return tve_low_get_dac_status(sel);
}

static int __pin_config(int sel, char *name)
{
	int ret = 0;
	char compat[32] = {0};
	int state = 0;

	snprintf(compat, sizeof(compat), "tv%d", sel);
	if (!strcmp(name, "active"))
		state = 1;
	else
		state = 0;

	ret = fdt_set_all_pin(compat, (1 == state)?"pinctrl-0":"pinctrl-1");

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
					g_tv_info.screen[sel].sid);
		tve_low_dac_enable(sel);
		tve_low_open(sel);
 		g_tv_info.screen[sel].enable = 1;
 	}
	return 0;
}

s32 tv_disable(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

 	if(g_tv_info.screen[sel].enable) {
		tve_low_close(sel);
		g_tv_info.screen[sel].enable = 0;
	}

	if (is_vga_mode(g_tv_info.screen[sel].tv_mode))
		__pin_config(sel, "sleep");
 	return 0;
}

s32 tv_suspend(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

 	if (!g_tv_info.screen[sel].suspend) {
		g_tv_info.screen[sel].suspend = true;
		tv_detect_disable(sel);
		tve_clk_disable(sel);
		tve_top_clk_disable();
	}
	suspend = 1;

	return 0;
}

s32 tv_resume(u32 sel)
{
	TVE_CHECK_PARAM(sel);
	TV_DBG("tv %d\n", sel);

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

