/*
 * Copyright (C) 2015 Allwinnertech, z.q <zengqi@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "disp_format_convert.h"

#if defined SUPPORT_WB
#include "include.h"

#define FORMAT_MANAGER_NUM 1

static struct format_manager fmt_mgr[FORMAT_MANAGER_NUM];

struct format_manager *disp_get_format_manager(unsigned int id)
{
	return &fmt_mgr[id];
}

#if defined(__LINUX_PLAT__)
s32 disp_format_convert_finish_proc(int irq, void *parg)
#else
s32 disp_format_convert_finish_proc(void *parg)
#endif
{
	struct format_manager *mgr = (struct format_manager *)parg;

	if (NULL == mgr)
		return DISP_IRQ_RETURN;

	if (0 == disp_al_get_eink_wb_status(mgr->disp)) {
		mgr->write_back_finish = 1;
		wake_up_interruptible(&(mgr->write_back_queue));
	} else
		__wrn("covert err!\n");
	disp_al_clear_eink_wb_interrupt(mgr->disp);

	return DISP_IRQ_RETURN;
}

static s32 disp_format_convert_enable(unsigned int id)
{
	struct format_manager *mgr = &fmt_mgr[id];
	s32 ret = -1;
	static int first = 1;

	if (NULL == mgr) {
		__wrn("input param is null\n");
		return -1;
	}

	ret = disp_sys_register_irq(mgr->irq_num, 0, disp_format_convert_finish_proc,(void*)mgr, 0, 0);
	if (0 != ret) {
		__wrn("%s L%d: fail to format convert irq\n", __func__, __LINE__);
		return ret;
	}

	disp_sys_enable_irq(mgr->irq_num);
	if (first) {
		clk_disable(mgr->clk);
		first = 0;
	}
	ret = clk_prepare_enable(mgr->clk);

	if (0 != ret)
		DE_WRN("fail enable mgr's clock!\n");

	/* enable de clk, enable write back clk */
	disp_al_de_clk_enable(mgr->disp);
	disp_al_write_back_clk_init(mgr->disp);

	return 0;
}

static s32 disp_format_convert_disable(unsigned int id)
{
	struct format_manager *mgr = &fmt_mgr[id];
	s32 ret = -1;

	if (NULL == mgr) {
		__wrn("%s: input param is null\n", __func__);
		return -1;
	}

	/* disable write back clk, disable de clk */
	disp_al_write_back_clk_exit(mgr->disp);
	ret = disp_al_de_clk_disable(mgr->disp);
	if (0 != ret)
		return ret;

	clk_disable(mgr->clk);
	disp_sys_disable_irq(mgr->irq_num);
	disp_sys_unregister_irq(mgr->irq_num,disp_format_convert_finish_proc,(void*)mgr);

	return 0;
}

struct disp_manager_data mdata;
struct disp_layer_config_data ldata[16];

static s32 disp_format_convert_start(unsigned int id,
					struct disp_layer_config *config,
					unsigned int layer_num,
					struct image_format *dest)
{
	struct format_manager *mgr = &fmt_mgr[id];
	s32 ret = -1, k = 0;
	u32 lay_total_num = 0;

	long timerout = (100 * HZ)/1000;		/*100ms*/

	if ((NULL == dest) || (NULL == mgr)) {
		__wrn(KERN_WARNING"input param is null\n");
		return -1;
	}

	if ((DISP_FORMAT_8BIT_GRAY == dest->format)) {
		__debug("dest_addr = 0x%p\n", (void *)dest->addr1);

		lay_total_num = de_feat_get_num_layers(mgr->disp);
		memset((void *)&mdata, 0, sizeof(struct disp_manager_data));
		memset((void *)ldata, 0, 16 * sizeof(ldata[0]));

		mdata.flag = MANAGER_ALL_DIRTY;
		mdata.config.enable = 1;
		mdata.config.interlace = 0;
		mdata.config.blank = 0;
		mdata.config.size.x = 0;
		mdata.config.size.y = 0;
		mdata.config.size.width = dest->width;
		mdata.config.size.height = dest->height;

		for (k = 0; k < layer_num; k++) {
			ldata[k].flag = LAYER_ALL_DIRTY;
			__disp_config_transfer2inner(&ldata[k].config,
						   &config[k]);
		}
		disp_al_manager_apply(mgr->disp, &mdata);
		disp_al_layer_apply(mgr->disp, ldata, layer_num);
		disp_al_manager_sync(mgr->disp);
		disp_al_manager_update_regs(mgr->disp);
		disp_al_set_eink_wb_param(mgr->disp, dest->width, dest->height,
						(unsigned int)dest->addr1);

		/* enable inttrrupt */
		disp_al_enable_eink_wb_interrupt(mgr->disp);
		disp_al_enable_eink_wb(mgr->disp);

#ifdef __UBOOT_PLAT__
		/* wait write back complete */
		while (EWB_OK != disp_al_get_eink_wb_status(mgr->disp)) {
			msleep(1);
		}
#else
		timerout = wait_event_interruptible_timeout(mgr->write_back_queue, (1 == mgr->write_back_finish),timerout);
		mgr->write_back_finish = 0;
		if (0 == timerout) {
			__wrn("wait write back timeout\n");
			disp_al_disable_eink_wb_interrupt(mgr->disp);
			disp_al_disable_eink_wb(mgr->disp);
			ret = -1;
			return ret;
		}

		disp_al_disable_eink_wb_interrupt(mgr->disp);
#endif
		disp_al_disable_eink_wb(mgr->disp);
		ret = 0;
	} else {
		__wrn("src_format(), dest_format(0x%x) is not support\n",
			dest->format);
	}

	return ret;
}

s32 disp_init_format_convert_manager(disp_bsp_init_para * para)
{
	s32 ret = -1;
	unsigned int i = 0;
	struct format_manager *mgr;

	for (i = 0; i < FORMAT_MANAGER_NUM; i++) {
		mgr = &fmt_mgr[i];
		memset(mgr, 0, sizeof(struct format_manager));

		mgr->disp = i;
		init_waitqueue_head(&(mgr->write_back_queue));
		mgr->write_back_finish = 0;
		mgr->irq_num = para->irq_no[DISP_MOD_DE];
		mgr->enable = disp_format_convert_enable;
		mgr->disable = disp_format_convert_disable;
		mgr->start_convert = disp_format_convert_start;
		mgr->clk = para->mclk[DISP_MOD_DE];
		disp_al_set_eink_wb_base(i, para->reg_base[DISP_MOD_DE]);
	}

	return ret;
}

void disp_exit_format_convert_manager(void)
{
	unsigned int i = 0;
	struct format_manager *mgr = NULL;

	for (i = 0; i < FORMAT_MANAGER_NUM; i++) {
		if (mgr->disable) {
			mgr->disable(i);
		}
	}

	return;
}

#endif

