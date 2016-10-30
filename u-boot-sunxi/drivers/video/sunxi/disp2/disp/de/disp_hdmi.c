#include "disp_hdmi.h"

#if defined(SUPPORT_HDMI)
struct disp_device_private_data {
	u32 enabled;
	bool hpd;
	bool suspended;

	enum disp_output_type mode;

	struct disp_device_func hdmi_func;
	struct disp_video_timings *video_info;

	u32 irq_no;

	struct clk *clk;
};

static u32 hdmi_used = 0;

static spinlock_t hdmi_data_lock;
static struct mutex hdmi_mlock;

static struct disp_device *hdmis = NULL;
static struct disp_device_private_data *hdmi_private = NULL;
s32 disp_hdmi_set_mode(struct disp_device* hdmi, u32 mode);
s32 disp_hdmi_enable(struct disp_device* hdmi);

struct disp_device* disp_get_hdmi(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens || !bsp_disp_feat_is_supported_output_types(disp, DISP_OUTPUT_TYPE_HDMI)) {
		DE_WRN("disp %d not support HDMI output\n", disp);
		return NULL;
	}

	return &hdmis[disp];
}

static struct disp_device_private_data *disp_hdmi_get_priv(struct disp_device *hdmi)
{
	if (NULL == hdmi) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return (struct disp_device_private_data *)hdmi->priv_data;
}

static s32 hdmi_clk_init(struct disp_device *hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if (!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	}

	return 0;
}

static s32 hdmi_clk_exit(struct disp_device *hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if (!hdmi || !hdmip) {
		DE_WRN("hdmi clk init null hdl!\n");
		return DIS_FAIL;
	}

	return 0;
}

static s32 hdmi_clk_config(struct disp_device *hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	unsigned long rate = 0;

	if (!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	}
	rate = hdmip->video_info->pixel_clk * (hdmip->video_info->pixel_repeat + 1);
	if (hdmip->clk)
		clk_set_rate(hdmip->clk, rate);

	return 0;
}

static s32 hdmi_clk_enable(struct disp_device *hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	int ret = 0;

	if (!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	}

	hdmi_clk_config(hdmi);
	if (hdmip->clk) {
		ret = clk_prepare_enable(hdmip->clk);
		if (0 != ret)
			DE_WRN("fail enable hdmi's clock!\n");
	}

	return ret;
}

static s32 hdmi_clk_disable(struct disp_device *hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if (!hdmi || !hdmip) {
	    DE_WRN("hdmi clk init null hdl!\n");
	    return DIS_FAIL;
	}


	if (hdmip->clk)
		clk_disable(hdmip->clk);

	return 0;
}

//--------------------------------
//----hdmi interface functions----
//--------------------------------

static s32 disp_hdmi_set_func(struct disp_device*  hdmi, struct disp_device_func * func)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	memcpy(&hdmip->hdmi_func, func, sizeof(struct disp_device_func));

	return 0;
}

//FIXME
extern void sync_event_proc(u32 disp, bool timeout);
#if defined(__LINUX_PLAT__)
static s32 disp_hdmi_event_proc(int irq, void *parg)
#else
static s32 disp_hdmi_event_proc(void *parg)
#endif
{
	struct disp_device *hdmi = (struct disp_device*)parg;
	struct disp_manager *mgr = NULL;
	u32 hwdev_index;

	if (NULL == hdmi)
		return DISP_IRQ_RETURN;

	hwdev_index = hdmi->hwdev_index;

	if (disp_al_device_query_irq(hwdev_index)) {
		int cur_line = disp_al_device_get_cur_line(hwdev_index);
		int start_delay = disp_al_device_get_start_delay(hwdev_index);

		mgr = hdmi->manager;
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

static s32 disp_hdmi_init(struct disp_device*  hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if (!hdmi || !hdmip) {
	    DE_WRN("hdmi init null hdl!\n");
	    return DIS_FAIL;
	}

	hdmi_clk_init(hdmi);
	return 0;
}

static s32 disp_hdmi_exit(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if (!hdmi || !hdmip) {
	    DE_WRN("hdmi init null hdl!\n");
	    return DIS_FAIL;
	}

	hdmi_clk_exit(hdmi);

  return 0;
}

s32 disp_hdmi_enable(struct disp_device* hdmi)
{
	unsigned long flags;
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	struct disp_manager *mgr = NULL;
	int ret;

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("%s, disp%d\n", __func__, hdmi->disp);

	if (hdmip->enabled == 1) {
		DE_WRN("hdmi%d is already enable\n", hdmi->disp);
		return DIS_FAIL;
	}

	mgr = hdmi->manager;
	if (!mgr) {
		DE_WRN("hdmi%d's mgr is NULL\n", hdmi->disp);
		return DIS_FAIL;
	}

	if (hdmip->hdmi_func.get_video_timing_info == NULL) {
		DE_WRN("hdmi_get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	hdmip->hdmi_func.get_video_timing_info(&(hdmip->video_info));

	if (hdmip->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}
	mutex_lock(&hdmi_mlock);
	if (hdmip->enabled == 1)
		goto exit;
	memcpy(&hdmi->timings, hdmip->video_info, sizeof(struct disp_video_timings));
	if (mgr->enable)
		mgr->enable(mgr);

	disp_sys_register_irq(hdmip->irq_no,0,disp_hdmi_event_proc,(void*)hdmi,0,0);
	disp_sys_enable_irq(hdmip->irq_no);

	ret = hdmi_clk_enable(hdmi);
	if (0 != ret)
		goto exit;
	disp_al_hdmi_cfg(hdmi->hwdev_index, hdmip->video_info);
	disp_al_hdmi_enable(hdmi->hwdev_index);

	if (NULL != hdmip->hdmi_func.enable)
		hdmip->hdmi_func.enable();
	else
		DE_WRN("hdmi_open is NULL\n");

	spin_lock_irqsave(&hdmi_data_lock, flags);
	hdmip->enabled = 1;
	spin_unlock_irqrestore(&hdmi_data_lock, flags);

exit:
	mutex_unlock(&hdmi_mlock);

	return 0;
}

static s32 disp_hdmi_sw_enable(struct disp_device* hdmi)
{
	unsigned long flags;
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	struct disp_manager *mgr = NULL;

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}
	mgr = hdmi->manager;
	if (!mgr) {
		DE_WRN("hdmi%d's mgr is NULL\n", hdmi->disp);
		return DIS_FAIL;
	}

	if (hdmip->hdmi_func.get_video_timing_info == NULL) {
		DE_WRN("hdmi_get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	hdmip->hdmi_func.get_video_timing_info(&(hdmip->video_info));

	if (hdmip->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}
	mutex_lock(&hdmi_mlock);
	memcpy(&hdmi->timings, hdmip->video_info, sizeof(struct disp_video_timings));
	if (mgr->sw_enable)
		mgr->sw_enable(mgr);

	disp_sys_register_irq(hdmip->irq_no,0,disp_hdmi_event_proc,(void*)hdmi,0,0);
	disp_sys_enable_irq(hdmip->irq_no);

	spin_lock_irqsave(&hdmi_data_lock, flags);
	hdmip->enabled = 1;
	spin_unlock_irqrestore(&hdmi_data_lock, flags);
	mutex_unlock(&hdmi_mlock);

	return 0;
}

static s32 disp_hdmi_disable(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	unsigned long flags;
	struct disp_manager *mgr = NULL;

	if ((NULL == hdmi) || (NULL == hdmip)) {
	    DE_WRN("hdmi set func null  hdl!\n");
	    return DIS_FAIL;
	}

	mgr = hdmi->manager;
	if (!mgr) {
		DE_WRN("hdmi%d's mgr is NULL\n", hdmi->disp);
		return DIS_FAIL;
	}

	if (hdmip->enabled == 0) {
		DE_WRN("hdmi%d is already disable\n", hdmi->disp);
		return DIS_FAIL;
	}

	if (hdmip->hdmi_func.disable == NULL)
	    return -1;

	mutex_lock(&hdmi_mlock);
	if (hdmip->enabled == 0)
		goto exit;

	spin_lock_irqsave(&hdmi_data_lock, flags);
	hdmip->enabled = 0;
	spin_unlock_irqrestore(&hdmi_data_lock, flags);

	hdmip->hdmi_func.disable();

	disp_al_hdmi_disable(hdmi->hwdev_index);
	hdmi_clk_disable(hdmi);

	if (mgr->disable)
		mgr->disable(mgr);

	disp_sys_disable_irq(hdmip->irq_no);
	disp_sys_unregister_irq(hdmip->irq_no, disp_hdmi_event_proc,(void*)hdmi);

exit:
	mutex_unlock(&hdmi_mlock);

	return 0;
}

static s32 disp_hdmi_is_enabled(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	return hdmip->enabled;
}


s32 disp_hdmi_set_mode(struct disp_device* hdmi, u32 mode)
{
	s32 ret = 0;
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (hdmip->hdmi_func.set_mode == NULL) {
		DE_WRN("hdmi_set_mode is null!\n");
		return -1;
	}

	ret = hdmip->hdmi_func.set_mode((enum disp_tv_mode)mode);

	if (ret == 0)
		hdmip->mode = mode;

	return ret;
}

static s32 disp_hdmi_get_mode(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	return hdmip->mode;
}

static s32 disp_hdmi_detect(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}
	if (hdmip->hdmi_func.get_HPD_status)
		return hdmip->hdmi_func.get_HPD_status();
	return DIS_FAIL;
}

static s32 disp_hdmi_set_detect(struct disp_device* hdmi, bool hpd)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	struct disp_manager *mgr = NULL;

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}
	mgr = hdmi->manager;
	if (!mgr) {
		return DIS_FAIL;
	}
	mutex_lock(&hdmi_mlock);
	if ((1 == hdmip->enabled) && (false == hdmip->hpd) && (true == hpd)) {
		if (hdmip->hdmi_func.get_video_timing_info) {
			hdmip->hdmi_func.get_video_timing_info(&(hdmip->video_info));
			if (hdmip->video_info == NULL) {
				DE_WRN("video info is null\n");
				hdmip->hpd = hpd;
				mutex_unlock(&hdmi_mlock);
				return DIS_FAIL;
			}
		}

		if (mgr->update_color_space)
			mgr->update_color_space(mgr);
	}
	hdmip->hpd = hpd;
	mutex_unlock(&hdmi_mlock);

	return 0;
}

static s32 disp_hdmi_check_support_mode(struct disp_device* hdmi, u32 mode)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (hdmip->hdmi_func.mode_support == NULL)
		return -1;

	return hdmip->hdmi_func.mode_support(mode);
}

static s32 disp_hdmi_get_input_csc(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return 0;
	}

	if (hdmip->hdmi_func.get_input_csc == NULL)
		return 0;

	return hdmip->hdmi_func.get_input_csc();
}

static s32 disp_hdmi_get_input_color_range(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);
	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (1 == disp_hdmi_get_input_csc(hdmi))
		return DISP_COLOR_RANGE_16_235;
	else
		return DISP_COLOR_RANGE_0_255;
}

static s32 disp_hdmi_suspend(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	mutex_lock(&hdmi_mlock);
	if (false == hdmip->suspended) {
		if (hdmip->hdmi_func.suspend != NULL) {
			hdmip->hdmi_func.suspend();
		}
		hdmip->suspended = true;
	}
	mutex_unlock(&hdmi_mlock);

	return 0;
}

static s32 disp_hdmi_resume(struct disp_device* hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	mutex_lock(&hdmi_mlock);
	if (true == hdmip->suspended) {
		if (hdmip->hdmi_func.resume != NULL) {
			hdmip->hdmi_func.resume();
		}
		hdmip->suspended = false;
	}
	mutex_unlock(&hdmi_mlock);

	return 0;
}

static s32 disp_hdmi_get_status(struct disp_device *hdmi)
{
	struct disp_device_private_data *hdmip = disp_hdmi_get_priv(hdmi);

	if ((NULL == hdmi) || (NULL == hdmip)) {
		DE_WRN("hdmi set func null  hdl!\n");
		return DIS_FAIL;
	}

	return disp_al_device_get_status(hdmi->hwdev_index);
}

s32 disp_init_hdmi(disp_bsp_init_para * para)
{
	int ret = 0;
	int value = 0;
	hdmi_used = 1;

	ret = disp_sys_script_get_item("hdmi", "boot_mask", &value, 1);
	if (ret == 1 && value == 1) {
		printf("skip hdmi in boot.\n");
		return -1;
	}

	if (hdmi_used) {
		u32 num_devices;
		u32 disp = 0;
		struct disp_device* hdmi;
		struct disp_device_private_data* hdmip;
		u32 hwdev_index = 0;
		u32 num_devices_support_hdmi = 0;

		DE_INF("disp_init_hdmi\n");
		spin_lock_init(&hdmi_data_lock);
		mutex_init(&hdmi_mlock);

		num_devices = bsp_disp_feat_get_num_devices();
		for (hwdev_index=0; hwdev_index<num_devices; hwdev_index++) {
			if (bsp_disp_feat_is_supported_output_types(hwdev_index, DISP_OUTPUT_TYPE_HDMI))
				num_devices_support_hdmi ++;
		}
		hdmis = (struct disp_device *)kmalloc(sizeof(struct disp_device) * num_devices_support_hdmi, GFP_KERNEL | __GFP_ZERO);
		if (NULL == hdmis) {
			DE_WRN("malloc memory fail!\n");
			return DIS_FAIL;
		}

		hdmi_private = (struct disp_device_private_data *)kmalloc(sizeof(struct disp_device_private_data)\
		* num_devices_support_hdmi, GFP_KERNEL | __GFP_ZERO);
		if (NULL == hdmi_private) {
			DE_WRN("malloc memory fail!\n");
			return DIS_FAIL;
		}

		disp = 0;
		for (hwdev_index=0; hwdev_index<num_devices; hwdev_index++) {
			if (!bsp_disp_feat_is_supported_output_types(hwdev_index, DISP_OUTPUT_TYPE_HDMI)) {
			    continue;
			}

			hdmi = &hdmis[disp];
			hdmip = &hdmi_private[disp];
			hdmi->priv_data = (void*)hdmip;

			hdmi->disp = disp;
			hdmi->hwdev_index = hwdev_index;
			sprintf(hdmi->name, "hdmi%d", disp);
			hdmi->type = DISP_OUTPUT_TYPE_HDMI;
			hdmip->mode = DISP_TV_MOD_720P_50HZ;
			hdmip->irq_no = para->irq_no[DISP_MOD_LCD0 + hwdev_index];
			hdmip->clk = para->mclk[DISP_MOD_LCD0 + hwdev_index];

			hdmi->set_manager = disp_device_set_manager;
			hdmi->unset_manager = disp_device_unset_manager;
			hdmi->get_resolution = disp_device_get_resolution;
			hdmi->get_timings = disp_device_get_timings;
			hdmi->is_interlace = disp_device_is_interlace;

			hdmi->init = disp_hdmi_init;
			hdmi->exit = disp_hdmi_exit;

			hdmi->set_func = disp_hdmi_set_func;
			hdmi->enable = disp_hdmi_enable;
			hdmi->sw_enable = disp_hdmi_sw_enable;
			hdmi->disable = disp_hdmi_disable;
			hdmi->is_enabled = disp_hdmi_is_enabled;
			hdmi->set_mode = disp_hdmi_set_mode;
			hdmi->get_mode = disp_hdmi_get_mode;
			hdmi->check_support_mode = disp_hdmi_check_support_mode;
			hdmi->get_input_csc = disp_hdmi_get_input_csc;
			hdmi->get_input_color_range = disp_hdmi_get_input_color_range;
			hdmi->suspend = disp_hdmi_suspend;
			hdmi->resume = disp_hdmi_resume;
			hdmi->detect = disp_hdmi_detect;
			hdmi->set_detect = disp_hdmi_set_detect;
			hdmi->get_status = disp_hdmi_get_status;

			hdmi->init(hdmi);
			disp_device_register(hdmi);
			disp ++;
		}
	}
	return 0;
}
#endif

