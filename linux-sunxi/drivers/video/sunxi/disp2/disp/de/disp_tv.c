#include "disp_tv.h"


#if defined(SUPPORT_TV)

#define TVE_COUNT 	1
static spinlock_t g_tv_data_lock;
static struct disp_device *g_ptv_devices = NULL;
static struct disp_device_private_data *g_ptv_private = NULL;
disp_bsp_init_para g_init_para;

static struct disp_device_private_data *disp_tv_get_priv(struct disp_device *ptv)
{
	if (NULL == ptv) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return (struct disp_device_private_data *)ptv->priv_data;
}

extern void sync_event_proc(u32 disp, bool timeout);

#if defined(__LINUX_PLAT__)
static s32 disp_tv_event_proc(int irq, void *parg)
#else
static s32 disp_tv_event_proc(void *parg)
#endif
{
	struct disp_device *ptv = (struct disp_device *)parg;
	struct disp_manager *mgr = NULL;
	u32 hwdev_index;

	if (ptv==NULL) {
		DE_WRN("ptv is null.\n");
		return DISP_IRQ_RETURN;
	}

	hwdev_index = ptv->hwdev_index;
	if (disp_al_device_query_irq(hwdev_index)) {
		int cur_line = disp_al_device_get_cur_line(hwdev_index);
		int start_delay = disp_al_device_get_start_delay(hwdev_index);

		mgr = ptv->manager;
		if (NULL == mgr)
			return DISP_IRQ_RETURN;

		if (cur_line <= (start_delay-4)) {
			sync_event_proc(mgr->disp, false);
		} else {
			sync_event_proc(mgr->disp, true);
		}
	}

	return DISP_IRQ_RETURN;
}

#if defined(CONFIG_ARCH_SUN8IW6)
static s32 tv_clk_init(struct disp_device*  ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
	    DE_WRN("tv init null hdl!\n");
	    return DIS_FAIL;
	}

	ptvp->clk_parent = clk_get_parent(ptvp->clk);

	return 0;
}

static s32 tv_clk_exit(struct disp_device*  ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
	    DE_WRN("tv init null hdl!\n");
	    return DIS_FAIL;
	}

	if (ptvp->clk_parent)
		clk_put(ptvp->clk_parent);

	return 0;
}

static s32 tv_clk_config(struct disp_device*  ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
		DE_WRN("tv init null hdl!\n");
		return DIS_FAIL;
	}

	return clk_set_rate(ptvp->clk, ptv->timings.pixel_clk);
}

#endif

static s32 tv_clk_enable(struct disp_device*  ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
	    DE_WRN("tv init null hdl!\n");
	    return DIS_FAIL;
	}

	return clk_prepare_enable(ptvp->clk);

}

static s32 tv_clk_disable(struct disp_device*  ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
	    DE_WRN("tv init null hdl!\n");
	    return DIS_FAIL;
	}

	clk_disable(ptvp->clk);

	return 0;
}

static s32 tv_calc_judge_line(struct disp_device *ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);
	int start_delay, usec_start_delay;
	int usec_judge_point;
	int pixel_clk;

	if (!ptv || !ptvp) {
		DE_WRN("tv init null hdl!\n");
		return DIS_FAIL;
	}

	pixel_clk = ptvp->video_info->pixel_clk;
#if defined(TV_UGLY_CLK_RATE)
	pixel_clk = (pixel_clk == TV_UGLY_CLK_RATE) ?
	    TV_COMPOSITE_CLK_RATE : pixel_clk;
#endif
	/*
	 * usec_per_line = 1 / fps / vt * 1000000
	 *               = 1 / (pixel_clk / vt / ht) / vt * 1000000
	 *               = ht / pixel_clk * 1000000
	 */
	ptvp->frame_per_sec = pixel_clk
	    / ptvp->video_info->hor_total_time
	    / ptvp->video_info->ver_total_time
	    * (ptvp->video_info->b_interlace + 1)
	    / (ptvp->video_info->trd_mode + 1);
	ptvp->usec_per_line = ptvp->video_info->hor_total_time
	    * 1000000 / pixel_clk;

	start_delay =
	    disp_al_device_get_start_delay(ptv->hwdev_index);
	usec_start_delay = start_delay * ptvp->usec_per_line;

	if (usec_start_delay <= 200)
		usec_judge_point = usec_start_delay * 3 / 7;
	else if (usec_start_delay <= 400)
		usec_judge_point = usec_start_delay / 2;
	else
		usec_judge_point = 200;
	ptvp->judge_line = usec_judge_point
	    / ptvp->usec_per_line;

	return 0;
}

s32 disp_tv_enable( struct disp_device* ptv)
{
	int ret;
	struct disp_manager *mgr = NULL;
	unsigned long flags;
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
		DE_WRN(" ptv | ptvp is null\n");
		return DIS_FAIL;
	}

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}
	mgr = ptv->manager;
	if (!mgr) {
		DE_WRN("tv%d's mgr is NULL\n", ptv->disp);
		return DIS_FAIL;
	}

	if (ptvp->enabled == 1) {
		DE_WRN("tv%d is already open\n", ptv->disp);
		return DIS_FAIL;
	}
	if (ptvp->tv_func.tv_get_video_timing_info == NULL) {
		DE_WRN("tv_get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	ptvp->tv_func.tv_get_video_timing_info(ptv->disp, &(ptvp->video_info));

	if (ptvp->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}
	memcpy(&ptv->timings, ptvp->video_info, sizeof(struct disp_video_timings));
	tv_calc_judge_line(ptv);

	if (mgr->enable)
		mgr->enable(mgr);
#if defined(CONFIG_ARCH_SUN8IW6)
	tv_clk_config(ptv);  			//no need tcon clk for 1680
	tv_clk_enable(ptv);
#endif
	if (NULL == ptvp->tv_func.tv_enable) {
		DE_WRN("tv_enable is NULL\n");
		return -1;
	}
	ret = tv_clk_enable(ptv);
	if (0 != ret) {
		DE_WRN("fail to enable tv's clock\n");
		goto exit;
	}

	ptvp->tv_func.tv_enable(ptv->disp);
	disp_al_tv_cfg(ptv->hwdev_index, ptvp->video_info);
	disp_al_tv_enable(ptv->hwdev_index);
	ret = disp_sys_register_irq(ptvp->irq_no,0,disp_tv_event_proc,(void*)ptv,0,0);
	if (ret!=0) {
		DE_WRN("tv request irq failed!\n");
	}
	disp_sys_enable_irq(ptvp->irq_no);
	spin_lock_irqsave(&g_tv_data_lock, flags);
	ptvp->enabled = 1;
	spin_unlock_irqrestore(&g_tv_data_lock, flags);

exit:
	return 0;
}

s32 disp_tv_sw_enable( struct disp_device* ptv)
{
	struct disp_manager *mgr = NULL;
	unsigned long flags;
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
		DE_WRN("tv init null hdl!\n");
		pr_debug("[DISP_TV] ptv | ptvp is wrong\n");
		return DIS_FAIL;
	}

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}
	mgr = ptv->manager;
	if (!mgr) {
		DE_WRN("tv%d's mgr is NULL\n", ptv->disp);
		return DIS_FAIL;
	}

	if (ptvp->enabled == 1) {
		DE_WRN("tv%d is already open\n", ptv->disp);
		return DIS_FAIL;
	}
	if (ptvp->tv_func.tv_get_video_timing_info == NULL) {
		DE_WRN("tv_get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	ptvp->tv_func.tv_get_video_timing_info(ptv->disp, &(ptvp->video_info));

	if (ptvp->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}
	memcpy(&ptv->timings, ptvp->video_info, sizeof(struct disp_video_timings));
	tv_calc_judge_line(ptv);

	if (mgr->sw_enable)
		mgr->sw_enable(mgr);
	if (NULL == ptvp->tv_func.tv_enable) {
		DE_WRN("tv_enable is NULL\n");
		return -1;
	}

	disp_sys_register_irq(ptvp->irq_no,0,disp_tv_event_proc,(void*)ptv,0,0);
	disp_sys_enable_irq(ptvp->irq_no);

#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (0 != tv_clk_enable(ptv))
		return -1;
#endif

	spin_lock_irqsave(&g_tv_data_lock, flags);
	ptvp->enabled = 1;
	spin_unlock_irqrestore(&g_tv_data_lock, flags);
	return 0;
}


s32 disp_tv_disable(struct disp_device* ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);
	unsigned long flags;
	struct disp_manager *mgr = NULL;

	if ((NULL == ptv) || (NULL == ptvp)) {
	    DE_WRN("tv set func null  hdl!\n");
	    return DIS_FAIL;
	}

	mgr = ptv->manager;
	if (!mgr) {
		DE_WRN("tv%d's mgr is NULL\n", ptv->disp);
		return DIS_FAIL;
	}

	if (ptvp->enabled == 0) {
		DE_WRN("tv%d is already closed\n", ptv->disp);
		return DIS_FAIL;
	}
	if (ptvp->tv_func.tv_disable== NULL) {
		DE_WRN("tv_func.tv_disable is NULL\n");
		return -1;
	}

	spin_lock_irqsave(&g_tv_data_lock, flags);
	ptvp->enabled = 0;
	spin_unlock_irqrestore(&g_tv_data_lock, flags);

	disp_tv_set_hpd(ptv, 0);
	ptvp->tv_func.tv_disable(ptv->disp);
	disp_al_tv_disable(ptv->hwdev_index);
	if (mgr->disable)
		mgr->disable(mgr);
	tv_clk_disable(ptv);
	ptvp->video_info = NULL;

	disp_sys_disable_irq(ptvp->irq_no);
	disp_sys_unregister_irq(ptvp->irq_no, disp_tv_event_proc,(void*)ptv);
	disp_delay_ms(1000);
	return 0;
}

static s32 disp_tv_init(struct disp_device*  ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
	    DE_WRN("tv init null hdl!\n");
	    return DIS_FAIL;
	}
#if defined(CONFIG_ARCH_SUN8IW6)
	tv_clk_init(ptv);
#endif
	return 0;
}


s32 disp_tv_exit(struct disp_device* ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if (!ptv || !ptvp) {
	    DE_WRN("tv init null hdl!\n");
	    return DIS_FAIL;
	}
	disp_tv_disable(ptv);
#if defined(CONFIG_ARCH_SUN8IW6)
	tv_clk_exit(ptv);
#endif
	kfree(ptv);
	kfree(ptvp);
	ptv = NULL;
	ptvp = NULL;
	return 0;
}

s32 disp_tv_is_enabled(struct disp_device* ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	return ptvp->enabled;

}


s32 disp_tv_suspend(struct disp_device* ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (false == ptvp->suspended) {
		ptvp->suspended = true;
		if (ptvp->tv_func.tv_suspend != NULL) {
			ptvp->tv_func.tv_suspend(ptv->disp);
		}
	}
	return 0;
}

s32 disp_tv_resume(struct disp_device* ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (true == ptvp->suspended) {
		if (ptvp->tv_func.tv_resume != NULL) {
			ptvp->tv_func.tv_resume(ptv->disp);
		}
		ptvp->suspended = false;
	}

	return 0;
}

s32 disp_tv_set_mode(struct disp_device *ptv, enum disp_tv_mode tv_mode)
{
	s32 ret = 0;
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (ptvp->tv_func.tv_set_mode == NULL) {
		DE_WRN("tv_set_mode is null!\n");
		return DIS_FAIL;
	}

	ret = ptvp->tv_func.tv_set_mode(ptv->disp, tv_mode);
	if (ret == 0)
		ptvp->tv_mode = tv_mode;

	return ret;
}


s32 disp_tv_get_mode(struct disp_device* ptv)
{

	enum disp_output_type  tv_mode;
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (ptvp->tv_func.tv_get_mode == NULL) {
		DE_WRN("tv_set_mode is null!\n");
		return -1;
	}

	tv_mode = ptvp->tv_func.tv_get_mode(ptv->disp);

	if (tv_mode != ptvp->tv_mode)
		ptvp->tv_mode = tv_mode;

	return ptvp->tv_mode;

}

s32 disp_tv_get_input_csc(struct disp_device* ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (ptvp->tv_func.tv_get_input_csc == NULL)
		return DIS_FAIL;

	return ptvp->tv_func.tv_get_input_csc(ptv->disp);			//0 or 1.
}


s32 disp_tv_set_func(struct disp_device*  ptv, struct disp_tv_func * func)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp) || (NULL == func)) {
		DE_WRN("tv set func null  hdl!\n");
		DE_WRN("in  disp_set_tv_func,point  ptv = %p, point  ptvp = %p\n", ptv, ptvp);
		return DIS_FAIL;
	}
	ptvp->tv_func.tv_enable = func->tv_enable;
	ptvp->tv_func.tv_disable = func->tv_disable;
	ptvp->tv_func.tv_suspend = func->tv_suspend;
	ptvp->tv_func.tv_resume = func->tv_resume;
	ptvp->tv_func.tv_get_mode = func->tv_get_mode;
	ptvp->tv_func.tv_set_mode = func->tv_set_mode;
	ptvp->tv_func.tv_get_input_csc = func->tv_get_input_csc;
	ptvp->tv_func.tv_get_video_timing_info = func->tv_get_video_timing_info;
	ptvp->tv_func.tv_mode_support = func->tv_mode_support;
	ptvp->tv_func.tv_hot_plugging_detect = func->tv_hot_plugging_detect;
	ptvp->tv_func.tv_set_enhance_mode = func->tv_set_enhance_mode;

	return 0;
}

s32 disp_tv_check_support_mode(struct disp_device*  ptv, enum disp_output_type tv_mode)
{

	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return DIS_FAIL;
	}
	if (!ptvp->tv_func.tv_mode_support)
		return 0;

	return ptvp->tv_func.tv_mode_support(ptv->disp, tv_mode);
}


static s32 disp_tv_get_fps(struct disp_device *ptv)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set func null  hdl!\n");
		return 0;
	}

	return ptvp->frame_per_sec;
}

s32 disp_init_tv_para(disp_bsp_init_para * para)
{
	if (para == NULL) {
		DE_WRN("disp_init_tv_para is NULL");
		return DIS_FAIL;
	}
	memcpy(&g_init_para, para, sizeof(disp_bsp_init_para));
	return 0;
}

s32 disp_tv_set_hpd(struct disp_device*  ptv, u32 state)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv set phd  null!\n");
		return DIS_FAIL;
	}

	if (ptvp->tv_func.tv_hot_plugging_detect== NULL)
		return DIS_FAIL;

	return ptvp->tv_func.tv_hot_plugging_detect(state);
}

s32 disp_set_enhance_mode(struct disp_device *ptv, u32 mode)
{
	struct disp_device_private_data *ptvp = disp_tv_get_priv(ptv);

	if ((NULL == ptv) || (NULL == ptvp)) {
		DE_WRN("tv private is null!\n");
		return DIS_FAIL;
	}

	if (!ptvp->tv_func.tv_set_enhance_mode) {
		DE_WRN("tv set_enhance_mode is null!\n");
		return DIS_FAIL;
	}

	return ptvp->tv_func.tv_set_enhance_mode(ptv->disp, mode);
}


s32 disp_init_tv(void)
{
	u32 num_devices;
	u32 disp = 0;
	struct disp_device *p_tv;
	struct disp_device_private_data *p_tvp;
	u32 hwdev_index = 0;
	u32 num_devices_support_tv = 0;
	u32 value = 0;
	char type_name[32] = {0};
	char compat[32] = {0};
	char status[10] = {0};
	const char *str;
	int ret = 0;
	struct device_node *node;
#if defined(__LINUX_PLAT__)
	spin_lock_init(&g_tv_data_lock);
#endif

	snprintf(compat, sizeof(compat), "allwinner,sunxi-tv");
	num_devices = bsp_disp_feat_get_num_devices();
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (bsp_disp_feat_is_supported_output_types(hwdev_index,
							DISP_OUTPUT_TYPE_TV))
				num_devices_support_tv ++;
	}
	g_ptv_devices = kmalloc(sizeof(struct disp_device)
			* num_devices_support_tv, GFP_KERNEL | __GFP_ZERO);
	if (NULL == g_ptv_devices) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	g_ptv_private = kmalloc(sizeof(*p_tvp) * num_devices_support_tv,
				GFP_KERNEL | __GFP_ZERO);
	if (NULL == g_ptv_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (!bsp_disp_feat_is_supported_output_types(hwdev_index,
							DISP_OUTPUT_TYPE_TV)) {
			DE_WRN("screen %d don't support TV!\n", hwdev_index);
			continue;
		}

		snprintf(type_name, sizeof(type_name), "tv%d", disp);
		node = of_find_compatible_node(NULL, type_name, compat);

		ret = of_property_read_string(node, "status", &str);
		memcpy((void *)status, str, strlen(str)+1);
		if (ret || strcmp(status, "okay")) {
			DE_WRN("disp%d not support tv\n", disp);
			disp++;
			continue;
		}

		ret = of_property_read_u32_array(node, "interface", &value, 1);
		if (ret || (value != DISP_TV_CVBS && value != DISP_TV_YPBPR)) {
			DE_WRN("disp%d not support tv\n", disp);
			disp++;
			continue;
		}

		p_tv = &g_ptv_devices[disp];
		p_tvp = &g_ptv_private[disp];
		p_tv->priv_data = (void *)p_tvp;
		p_tv->disp = disp;
		p_tv->hwdev_index = hwdev_index;
		snprintf(p_tv->name, sizeof(p_tv->name), "tv%d", disp);
		p_tv->type = DISP_OUTPUT_TYPE_TV;
		p_tvp->tv_mode = DISP_TV_MOD_PAL;
		p_tvp->irq_no = g_init_para.irq_no[DISP_MOD_LCD0 + hwdev_index];
		p_tvp->clk = g_init_para.mclk[DISP_MOD_LCD0 + hwdev_index];

		p_tv->set_manager = disp_device_set_manager;
		p_tv->unset_manager = disp_device_unset_manager;
		p_tv->get_resolution = disp_device_get_resolution;
		p_tv->get_timings = disp_device_get_timings;
		p_tv->is_interlace = disp_device_is_interlace;
		p_tv->init = disp_tv_init;
		p_tv->exit = disp_tv_exit;
		p_tv->set_tv_func = disp_tv_set_func;
		p_tv->enable = disp_tv_enable;
		p_tv->sw_enable = disp_tv_sw_enable;
		p_tv->disable = disp_tv_disable;
		p_tv->is_enabled = disp_tv_is_enabled;
		p_tv->set_mode = disp_tv_set_mode;
		p_tv->get_mode = disp_tv_get_mode;
		p_tv->check_support_mode = disp_tv_check_support_mode;
		p_tv->get_input_csc = disp_tv_get_input_csc;
		p_tv->suspend = disp_tv_suspend;
		p_tv->resume = disp_tv_resume;
		p_tv->set_enhance_mode = disp_set_enhance_mode;
		p_tv->get_fps = disp_tv_get_fps;
		p_tv->init(p_tv);

		disp_device_register(p_tv);
		disp++;
	}
	return 0;
}

#endif
