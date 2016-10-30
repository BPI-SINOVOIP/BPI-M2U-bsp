#include "disp_enhance.h"

struct disp_enhance_private_data
{
	u32                       reg_base;
	u32                       enabled;
	bool                      applied;

	struct disp_enhance_config config;

	s32 (*shadow_protect)(u32 sel, bool protect);
};
static spinlock_t enhance_data_lock;

static struct disp_enhance *enhances = NULL;
static struct disp_enhance_private_data *enhance_private;

struct disp_enhance* disp_get_enhance(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens) {
		DE_WRN("disp %d out of range\n", disp);
		return NULL;
	}

	return &enhances[disp];
}

static struct disp_enhance_private_data *disp_enhance_get_priv(struct disp_enhance *enhance)
{
	if (NULL == enhance) {
		DE_INF("NULL hdl!\n");
		return NULL;
	}

	return &enhance_private[enhance->disp];
}

static s32 disp_enhance_sync(struct disp_enhance* enhance)
{
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	enhance->update_regs(enhance);
	if (enhancep->enabled)
	{
		disp_al_enhance_sync(enhance->disp);
	}

	return 0;
}

static s32 disp_enhance_tasklet(struct disp_enhance* enhance)
{
	//unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	if (enhancep->enabled)
	{
		disp_al_enhance_tasklet(enhance->disp);
	}

	return 0;
}

static s32 disp_enhance_update_regs(struct disp_enhance* enhance)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	/* size likely change while it depend on layer size */
	//if (enhancep->applied) {
	{
		//DE_INF("enhance %d, enable=%d\n", enhance->disp, enhancep->config.info.enable);
		disp_al_enhance_update_regs(enhance->disp);
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhancep->applied = false;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	return 0;
}

static s32 disp_enhance_apply(struct disp_enhance* enhance)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);
	struct disp_enhance_config config;

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	DE_INF("disp_enhance_apply, screen %d\n", enhance->disp);
	memset(&config, 0, sizeof(struct disp_enhance_config));
	spin_lock_irqsave(&enhance_data_lock, flags);
	if (0 != enhancep->config.flags) {
		memcpy(&config, &enhancep->config, sizeof(struct disp_enhance_config));
		enhancep->applied = true;
	}
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	if (ENHANCE_NONE_DIRTY != config.flags) {
		disp_enhance_shadow_protect(enhance, 1);
		disp_al_enhance_apply(enhance->disp, &config);
		disp_enhance_shadow_protect(enhance, 0);
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	if (0 != config.flags)
		enhancep->applied = true;
	config.flags = ENHANCE_NONE_DIRTY;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	return 0;
}

static s32 disp_enhance_force_apply(struct disp_enhance* enhance)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);
	struct disp_device *dispdev = NULL;

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	if (enhance->manager)
		dispdev = enhance->manager->device;
	if (dispdev)
		dispdev->get_resolution(dispdev, &enhancep->config.info.size.width,
			&enhancep->config.info.size.height);

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhancep->config.flags |= ENHANCE_ALL_DIRTY;
	spin_unlock_irqrestore(&enhance_data_lock, flags);
	disp_enhance_apply(enhance);
	disp_enhance_update_regs(enhance);
	disp_enhance_sync(enhance);

	return 0;
}

/* seem no meaning */
static bool disp_enhance_is_enabled(struct disp_enhance* enhance)
{
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return false;
	}

	return enhancep->enabled;
}

static s32 disp_enhance_enable(struct disp_enhance* enhance)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);
	struct disp_device *dispdev = NULL;

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	if (enhance->manager)
		dispdev = enhance->manager->device;
	if (dispdev) {
		dispdev->get_resolution(dispdev, &enhancep->config.info.size.width,
			&enhancep->config.info.size.height);
		if (DISP_OUTPUT_TYPE_HDMI == dispdev->type)
			enhancep->config.info.mode = (enhancep->config.info.mode & 0xffff0000)  | (1); //hdmi
		else
			enhancep->config.info.mode = (enhancep->config.info.mode & 0xffff0000)  | (0); //lcd
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhancep->config.info.enable = 1;
	enhancep->config.flags |= ENHANCE_ENABLE_DIRTY | ENHANCE_SIZE_DIRTY;

	if ((0 == enhancep->config.info.window.width) || (0 == enhancep->config.info.window.height)) {
		enhancep->config.info.window.width = enhancep->config.info.size.width;
		enhancep->config.info.window.height = enhancep->config.info.size.height;
	}

	enhancep->enabled = 1;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	disp_enhance_apply(enhance);
	return 0;
}

static s32 disp_enhance_disable(struct disp_enhance* enhance)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhancep->config.info.enable = 0;
	enhancep->config.flags |= ENHANCE_ENABLE_DIRTY;

	enhancep->enabled = 0;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	disp_enhance_apply(enhance);

	return 0;
}

static s32 disp_enhance_demo_enable(struct disp_enhance* enhance)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhancep->config.info.demo_enable = 1;
	enhancep->config.flags |= ENHANCE_DEMO_DIRTY;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	disp_enhance_apply(enhance);
	return 0;
}

static s32 disp_enhance_demo_disable(struct disp_enhance* enhance)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhancep->config.info.demo_enable = 0;
	enhancep->config.flags |= ENHANCE_DEMO_DIRTY;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	disp_enhance_apply(enhance);
	return 0;
}

static s32 disp_enhance_set_mode(struct disp_enhance* enhance, u32 mode)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhancep->config.info.mode = (enhancep->config.info.mode & 0xffff)  | (mode << 16);
	enhancep->config.flags |= ENHANCE_MODE_DIRTY;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	disp_enhance_apply(enhance);
	return 0;
}

static s32 disp_enhance_get_mode(struct disp_enhance* enhance)
{
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return 0;
	}

	return enhancep->config.info.mode >> 16;
}

static s32 disp_enhance_set_manager(struct disp_enhance* enhance, struct disp_manager *mgr)
{
	unsigned long flags;
	if ((NULL == enhance) || (NULL == mgr)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	enhance->manager = mgr;
	mgr->enhance = enhance;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	return 0;
}

static s32 disp_enhance_unset_manager(struct disp_enhance* enhance)
{
	unsigned long flags;
	if ((NULL == enhance)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	spin_lock_irqsave(&enhance_data_lock, flags);
	if (enhance->manager)
		enhance->manager->enhance = NULL;
	enhance->manager = NULL;
	spin_unlock_irqrestore(&enhance_data_lock, flags);

	return 0;
}


static s32 disp_enhance_shadow_protect(struct disp_enhance *enhance, bool protect)
{
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	if (enhancep->shadow_protect)
		return enhancep->shadow_protect(enhance->disp, protect);

	return -1;
}

static s32 disp_enhance_dump(struct disp_enhance *enhance, char* buf)
{
	struct disp_enhance_config config;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);
	u32 count = 0;

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	memcpy(&config, &enhancep->config, sizeof(struct disp_enhance_config));

	count += sprintf(buf + count, "enhance %d: %s, %s\n",
		enhance->disp, (config.info.enable==1)?"enable":"disable",
		(config.info.demo_enable==1)?"demo":"normal");

	return count;
}

static s32 disp_enhance_init(struct disp_enhance *enhance)
{
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	return 0;
}

static s32 disp_enhance_exit(struct disp_enhance *enhance)
{
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}

	return 0;
}

//for inner debug
s32 disp_enhance_set_para(struct disp_enhance* enhance, disp_enhance_para *para)
{
	unsigned long flags;
	struct disp_enhance_private_data *enhancep = disp_enhance_get_priv(enhance);
	struct disp_device *dispdev = NULL;

	if ((NULL == enhance) || (NULL == enhancep)) {
		DE_INF("NULL hdl!\n");
		return -1;
	}
	if (enhance->manager)
		dispdev = enhance->manager->device;
	if (dispdev)
		dispdev->get_resolution(dispdev, &enhancep->config.info.size.width,
			&enhancep->config.info.size.height);

	spin_lock_irqsave(&enhance_data_lock, flags);
	memcpy(&enhancep->config.info.window, &para->window, sizeof(struct disp_rect));
	enhancep->config.info.enable = para->enable;
	enhancep->config.flags |= ENHANCE_ENABLE_DIRTY | ENHANCE_SIZE_DIRTY;

	if ((0 == enhancep->config.info.window.width) || (0 == enhancep->config.info.window.height)) {
		enhancep->config.info.window.width = enhancep->config.info.size.width;
		enhancep->config.info.window.height = enhancep->config.info.size.height;
	}


	enhancep->config.info.bright = para->bright;
	enhancep->config.info.contrast= para->contrast;
	enhancep->config.info.saturation= para->saturation;
	enhancep->config.info.hue = para->hue;
	enhancep->config.info.sharp = para->sharp;
	enhancep->config.flags |= ENHANCE_SHARP_DIRTY;
	enhancep->config.info.auto_contrast = para->auto_contrast;

	enhancep->config.info.auto_color = para->auto_color;
	enhancep->config.flags |= ENHANCE_SHARP_DIRTY;
	enhancep->config.info.fancycolor_red = para->fancycolor_red;
	enhancep->config.info.fancycolor_blue = para->fancycolor_blue;
	enhancep->config.info.fancycolor_green = para->fancycolor_green;

	enhancep->enabled = para->enable;
	spin_unlock_irqrestore(&enhance_data_lock, flags);
	DE_INF("en=%d, para=<%d,%d,%d,%d>, sharp=%d, auto_para=<%d,%d>, fancycolor=<%d,%d,%d>\n", enhancep->config.info.enable,
	enhancep->config.info.bright,enhancep->config.info.contrast,
	enhancep->config.info.saturation, enhancep->config.info.hue,
	enhancep->config.info.sharp, enhancep->config.info.auto_color,
	enhancep->config.info.auto_contrast, enhancep->config.info.fancycolor_red,
	enhancep->config.info.fancycolor_green, enhancep->config.info.fancycolor_blue);

	disp_enhance_apply(enhance);
	return 0;
}

s32 disp_init_enhance(disp_bsp_init_para * para)
{
	u32 num_enhances;
	u32 disp;
	struct disp_enhance *enhance;
	struct disp_enhance_private_data *enhancep;

	DE_INF("disp_init_enhance\n");

	spin_lock_init(&enhance_data_lock);
	num_enhances = bsp_disp_feat_get_num_screens();//bsp_disp_feat_get_num_smart_backlights();
	enhances = (struct disp_enhance *)kmalloc(sizeof(struct disp_enhance) * num_enhances, GFP_KERNEL | __GFP_ZERO);
	if (NULL == enhances) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}
	enhance_private = (struct disp_enhance_private_data *)kmalloc(sizeof(struct disp_enhance_private_data)\
		* num_enhances, GFP_KERNEL | __GFP_ZERO);
	if (NULL == enhance_private) {
		DE_WRN("malloc memory fail!\n");
		return DIS_FAIL;
	}

	for (disp=0; disp<num_enhances; disp++) {
		enhance = &enhances[disp];
		enhancep = &enhance_private[disp];

		switch(disp) {
		case 0:
			enhance->name = "enhance0";
			enhance->disp = 0;
			break;
		case 1:
			enhance->name = "enhance1";
			enhance->disp = 1;
			break;
		case 2:
			enhance->name = "enhance2";
			enhance->disp = 2;

			break;
		}
		enhancep->shadow_protect = para->shadow_protect;

		enhance->enable = disp_enhance_enable;
		enhance->disable = disp_enhance_disable;
		enhance->is_enabled = disp_enhance_is_enabled;
		enhance->init = disp_enhance_init;
		enhance->exit = disp_enhance_exit;
		enhance->apply = disp_enhance_apply;
		enhance->update_regs = disp_enhance_update_regs;
		enhance->force_apply = disp_enhance_force_apply;
		enhance->sync = disp_enhance_sync;
		enhance->tasklet = disp_enhance_tasklet;
		enhance->set_manager = disp_enhance_set_manager;
		enhance->unset_manager = disp_enhance_unset_manager;
		enhance->set_para = disp_enhance_set_para;
		enhance->dump = disp_enhance_dump;
		enhance->demo_enable = disp_enhance_demo_enable;
		enhance->demo_disable = disp_enhance_demo_disable;
		enhance->set_mode = disp_enhance_set_mode;
		enhance->get_mode = disp_enhance_get_mode;
#if 0
		enhance->set_bright = disp_enhance_set_bright;
		enhance->set_saturation = disp_enhance_set_saturation;
		enhance->set_hue = disp_enhance_set_hue;
		//enhance->set_contrast = disp_enhance_set_contrast;
		enhance->set_mode = disp_enhance_set_mode;
		enhance->set_window = disp_enhance_set_window;
#endif

		enhance->init(enhance);
	}

	return 0;
}
