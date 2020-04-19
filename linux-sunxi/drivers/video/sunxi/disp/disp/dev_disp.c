/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dev_disp.h"
#include <linux/pm_runtime.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

struct fb_info_t g_fbi;
struct disp_drv_info g_disp_drv;
struct disp_fcount_data fcount_data;

struct disp_sw_init_para g_sw_init_para;

/* alloc based on 4K byte */
#define MY_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)

static u32 suspend_output_type[3] = { 0, 0, 0 };

/* 0:normal; */
/* suspend_status&1 != 0:in early_suspend; */
/* suspend_status&2 != 0:in suspend; */
static u32 suspend_status;
/* 0:after early suspend; */
/* 1:after suspend; */
/* 2:after resume; */
/* 3 :after late resume */
static u32 suspend_prestep = 3;

#if defined(CONFIG_ARCH_SUN9IW1P1)
static unsigned int gbuffer[4096];
#endif
static struct info_mm g_disp_mm[10];
static int g_disp_mm_sel;

static struct cdev *my_cdev;
static dev_t devid;
static struct class *disp_class;
static struct device *display_dev;

/*
 * when open the CONFIG_PM_RUNTIME,
 * this bool can also control if use the PM_RUNTIME.
 */
bool g_pm_runtime_enable;
static u32 power_status_init;
/* static u32 disp_print_cmd_level = 0; */
static u32 disp_cmd_print = 0xffff;	/* print cmd which eq disp_cmd_print */

#if 0
static struct resource disp_resource[] = {

};
#endif

#ifdef FB_RESERVED_MEM
void *disp_malloc(u32 num_bytes, u32 *phys_addr)
{
	u32 actual_bytes;
	void *address = NULL;

	if (num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);
		address = dma_alloc_coherent(g_fbi.dev,
					     actual_bytes,
					     (dma_addr_t *) phys_addr,
					     GFP_KERNEL);
		if (address) {
			__inf("disp_malloc ok, address=0x%p,size=0x%x\n",
			      (void *)(*(unsigned long *)phys_addr), num_bytes);
		} else {
			pr_warn("disp_malloc fail, size=0x%x\n", num_bytes);
			return NULL;
		}
		return address;
	}
#if 0
#if 1
	if (num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);
		address = dma_alloc_coherent(NULL,
					     actual_bytes,
					     (dma_addr_t *) phys_addr,
					     GFP_KERNEL);
		if (address) {
			__inf("dma_alloc_coherent ok,address=0x%x, size=0x%x\n",
			      *phys_addr, num_bytes);
			return address;
		}
		__wrn("dma_alloc_coherent fail,size=0x%x\n", num_bytes);
	} else {
		__wrn("disp_malloc size is zero\n");
	}
#else
	if (num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);
		*phys_addr = sunxi_mem_alloc(actual_bytes);
		if (*phys_addr) {
			__inf("sunxi_mem_alloc ok, address=0x%x,size=0x%x\n",
			      *phys_addr, num_bytes);
#if defined(CONFIG_ARCH_SUN6I)
			address = (void *)ioremap(*phys_addr, actual_bytes);
#else
			address = sunxi_map_kernel(*phys_addr, actual_bytes);
#endif
			if (address) {
				pr_info("sunxi_map_kernel ok, phys_addr=0x%x\n",
					(unsigned int)*phys_addr;
					pr_info("size=0x%x, virt_addr=0x%x",
						(unsigned int)num_bytes,
						(unsigned int)address));

			} else {
				__wrn("sunxi_map_kernel fail, phys_addr=0x%x\n",
				      (unsigned int)*phys_addr;
				      __wrn("size=0x%x, virt_addr=0x%x",
					    (unsigned int)num_bytes,
					    (unsigned int)address));
			}
			return address;
		}
		__wrn("sunxi_mem_alloc fail, size=0x%x\n", num_bytes);
	} else {
		__wrn("disp_malloc size is zero\n");
	}

#endif
#endif
	return NULL;
}

void disp_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
	u32 actual_bytes;

	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if (phys_addr && virt_addr)
		dma_free_coherent(g_fbi.dev,
				  actual_bytes, virt_addr,
				  (dma_addr_t) phys_addr);

#if 0
#if 1
	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if (phys_addr && virt_addr)
		dma_free_coherent(NULL,
				  actual_bytes, virt_addr,
				  (dma_addr_t) phys_addr);
#else
	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if (virt_addr) {
#if defined(CONFIG_ARCH_SUN6I)
		iounmap((void __iomem *)virt_addr);
#else
		sunxi_unmap_kernel((void *)virt_addr);
#endif
	}
	if (phys_addr) {
#if defined(CONFIG_ARCH_SUN6I)
		sunxi_mem_free((unsigned int)phys_addr);
#else
		sunxi_mem_free((unsigned int)phys_addr, actual_bytes);
#endif
	}
#endif
#endif

}
#endif

s32 drv_lcd_enable(u32 sel)
{
	u32 i = 0;
	disp_lcd_flow *flow;

	mutex_lock(&g_fbi.mlock);
	if (bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0)) {
		bsp_disp_lcd_pre_enable(sel);

		if (0 == (g_sw_init_para.sw_init_flag & (1 << sel))) {
			flow = bsp_disp_lcd_get_open_flow(sel);
			for (i = 0; i < flow->func_num; i++) {
				flow->func[i].func(sel);
				pr_info("[LCD]open, step %d finish\n", i);

				msleep_interruptible(flow->func[i].delay);
			}
		} else {
			bsp_disp_lcd_tcon_enable(sel);
			bsp_disp_lcd_pin_cfg(sel, 1);
		}
		bsp_disp_lcd_post_enable(sel);

		g_disp_drv.b_lcd_enabled[sel] = 1;
	}
	mutex_unlock(&g_fbi.mlock);

	return 0;
}

s32 drv_lcd_enable_except_backlight(u32 sel)
{
	u32 i = 0;
	disp_lcd_flow *flow;

	mutex_lock(&g_fbi.mlock);
	if (bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0)) {
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for (i = 0; i < flow->func_num - 1; i++) {
			flow->func[i].func(sel);
			pr_info("[LCD]open, step %d finish\n", i);

			msleep_interruptible(flow->func[i].delay);
		}
		bsp_disp_lcd_post_enable(sel);

		g_disp_drv.b_lcd_enabled[sel] = 1;
	}
	mutex_unlock(&g_fbi.mlock);

	return 0;
}

s32 drv_lcd_disable(u32 sel)
{
	u32 i = 0;
	disp_lcd_flow *flow;

	mutex_lock(&g_fbi.mlock);
	if (bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 1)) {
		bsp_disp_lcd_pre_disable(sel);

		flow = bsp_disp_lcd_get_close_flow(sel);
		for (i = 0; i < flow->func_num; i++) {
			flow->func[i].func(sel);
			pr_info("[LCD]close, step %d finish\n", i);

			msleep_interruptible(flow->func[i].delay);
		}
		bsp_disp_lcd_post_disable(sel);

		g_disp_drv.b_lcd_enabled[sel] = 0;
	}
	mutex_unlock(&g_fbi.mlock);

	return 0;
}

#if defined(CONFIG_ARCH_SUN9IW1P1)
s32 disp_set_hdmi_func(u32 screen_id, disp_hdmi_func *func)
{
	return bsp_disp_set_hdmi_func(screen_id, func);
}
EXPORT_SYMBOL(disp_set_hdmi_func);

/* FIXME */
s32 disp_set_hdmi_hpd(u32 hpd)
{
	/* bsp_disp_set_hdmi_hpd(hpd); */

	return 0;
}
EXPORT_SYMBOL(disp_set_hdmi_hpd);

#endif
static void resume_work_0(struct work_struct *work)
{
#if 0
	u32 i = 0;
	disp_lcd_flow *flow;
	u32 sel = 0;

	if (bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0)) {
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for (i = 0; i < flow->func_num - 1; i++) {
			flow->func[i].func(sel);

			msleep_interruptible(flow->func[i].delay);
		}
	}

	g_disp_drv.b_lcd_enabled[sel] = 1;
#else
	drv_lcd_enable_except_backlight(0);
#endif
}

static void resume_work_1(struct work_struct *work)
{
#if 0
	disp_lcd_flow *flow;
	u32 sel = 1;
	u32 i;

	if (bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0)) {
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for (i = 0; i < flow->func_num - 1; i++) {
			flow->func[i].func(sel);

			msleep_interruptible(flow->func[i].delay);
		}
	}

	g_disp_drv.b_lcd_enabled[sel] = 1;
#else
	drv_lcd_enable_except_backlight(1);
#endif
}

static void resume_work_2(struct work_struct *work)
{
#if 0
	disp_lcd_flow *flow;
	u32 sel = 1;
	u32 i;

	if (bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0)) {
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for (i = 0; i < flow->func_num - 1; i++) {
			flow->func[i].func(sel);

			msleep_interruptible(flow->func[i].delay);
		}
	}

	g_disp_drv.b_lcd_enabled[sel] = 1;
#else
	drv_lcd_enable_except_backlight(2);
#endif
}

static void start_work(struct work_struct *work)
{
	int num_screens;
	int screen_id;

	num_screens = bsp_disp_feat_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		__inf("sel=%d, output_type=%d,lcd_reg=%d, hdmi_reg=%d\n",
		      screen_id,
		      g_fbi.disp_init.output_type[screen_id],
		      bsp_disp_get_lcd_registered(screen_id),
		      bsp_disp_get_hdmi_registered());
		if (((g_fbi.disp_init.disp_mode ==
		      DISP_INIT_MODE_SCREEN0) && (screen_id == 0))
		    || ((g_fbi.disp_init.disp_mode ==
			 DISP_INIT_MODE_SCREEN1) && (screen_id == 1))) {
			if (g_fbi.disp_init.output_type[screen_id] ==
			    DISP_OUTPUT_TYPE_LCD) {
				if (bsp_disp_get_lcd_registered(screen_id)
				    && bsp_disp_get_output_type(screen_id)
				    != DISP_OUTPUT_TYPE_LCD)
					drv_lcd_enable(screen_id);
					suspend_output_type[screen_id] =
					    DISP_OUTPUT_TYPE_LCD;
			} else if (g_fbi.disp_init.output_type[screen_id]
				   == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
				if (bsp_disp_get_hdmi_registered() &&
				    bsp_disp_get_output_type(screen_id)
				    != DISP_OUTPUT_TYPE_HDMI) {
					__inf("hdmi register\n");
					if (g_sw_init_para.sw_init_flag &&
					    (g_sw_init_para.output_channel ==
					     screen_id)) {
						bsp_disp_shadow_protect
						    (screen_id, 1);
						bsp_disp_hdmi_enable(screen_id);
						g_sw_init_para.sw_init_flag
						    &= ~(1 << screen_id);
						bsp_disp_shadow_protect
						    (screen_id, 0);
						suspend_output_type[screen_id] =
						    DISP_OUTPUT_TYPE_HDMI;
					} else {
						bsp_disp_hdmi_set_mode
						    (screen_id,
						     g_fbi.disp_init.
						     output_mode[screen_id]);
						bsp_disp_hdmi_enable(screen_id);
						suspend_output_type[screen_id] =
						    DISP_OUTPUT_TYPE_HDMI;
					}
				}
#endif
			} else if (g_fbi.disp_init.output_type[screen_id] ==
				   DISP_OUTPUT_TYPE_TV) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
				if (bsp_disp_get_lcd_registered(screen_id) &&
				    (bsp_disp_get_output_type(screen_id)
				     != DISP_OUTPUT_TYPE_LCD) &&
				    (bsp_disp_get_lcd_output_type(screen_id)
				     == DISP_OUTPUT_TYPE_TV)) {
					if (g_sw_init_para.sw_init_flag &&
					    (g_sw_init_para.output_channel ==
					     screen_id)) {
						bsp_disp_shadow_protect
						    (screen_id, 1);
						bsp_disp_lcd_set_tv_mode
						    (screen_id,
						     g_fbi.disp_init.
						     output_mode[screen_id]);
						drv_lcd_enable(screen_id);
						g_sw_init_para.sw_init_flag &=
						    ~(1 << screen_id);
						bsp_disp_shadow_protect
						    (screen_id, 0);
						suspend_output_type[screen_id] =
						    DISP_OUTPUT_TYPE_LCD;
					} else {
						bsp_disp_lcd_set_tv_mode
						    (screen_id,
						     g_fbi.disp_init.
						     output_mode[screen_id]);
						drv_lcd_enable(screen_id);
						suspend_output_type[screen_id] =
						    DISP_OUTPUT_TYPE_LCD;
					}
				}
#endif
			}
		} else if ((g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN2)
			   && (screen_id == 2)
			   && (bsp_disp_get_output_type(screen_id) !=
			       DISP_OUTPUT_TYPE_LCD) &&
			   bsp_disp_get_lcd_registered(screen_id)) {
			drv_lcd_enable(screen_id);
			suspend_output_type[screen_id] =
			    DISP_OUTPUT_TYPE_LCD;
		}
	}
}

static s32 start_process(void)
{
	flush_work(&g_fbi.start_work);
	schedule_work(&g_fbi.start_work);

	return 0;
}

s32 get_para_from_cmdline(const char *cmdline, const char *name, char *value)
{
	char *p = (char *)cmdline;
	char *value_p = value;

	if (!cmdline || !name || !value)
		return -1;

	for (; *p != 0;) {
		if (*p++ == ' ') {
			if (strncmp(p, name, strlen(name)) == 0) {
				p += strlen(name);
				if (*p++ != '=')
					continue;

				while (*p != 0 && *p != ' ')
					*value_p++ = *p++;

				*value_p = '\0';
				return value_p - value;
			}
		}
	}

	return 0;
}

static u32 disp_parse_cmdline(struct __disp_bsp_init_para *para)
{
	char val[16] = { 0 };
	unsigned long value = 0;
	int ret;

	/* get the parameters of display_resolution */
	/* from cmdline passed by boot */
	if (get_para_from_cmdline(saved_command_line, "disp_rsl", val) > 0) {
		ret = kstrtoul(val, 16, &value);	/* simple_strtoul */
		if (ret != 0)
			value = 0;
	}
#if !defined(CONFIG_HOMLET_PLATFORM)
	value = 0x0;
#endif
	para->sw_init_para = &g_sw_init_para;
	g_sw_init_para.disp_rsl = value;
	g_sw_init_para.closed = (value >> 24) & 0xFF;
	g_sw_init_para.output_type = (value >> 16) & 0xFF;
	g_sw_init_para.output_mode = (value >> 8) & 0xFF;
	g_sw_init_para.output_channel = value & 0xFF;

	return 0;
}

static u32 disp_para_check(struct __disp_bsp_init_para *para)
{
	u8 valid = 1;
	u8 type = DISP_OUTPUT_TYPE_HDMI | DISP_OUTPUT_TYPE_TV;

	valid &= ((type & para->sw_init_para->output_type) ? 1 : 0);
	if ((para->sw_init_para->output_type == DISP_OUTPUT_TYPE_HDMI)
	    || (para->sw_init_para->output_type == DISP_OUTPUT_TYPE_TV)) {
		valid &=
		    ((para->sw_init_para->output_mode <
		      DISP_TV_MODE_NUM) ? 1 : 0);
	} else if (para->sw_init_para->output_type == DISP_OUTPUT_TYPE_VGA) {
		/* todo */
	}
	if ((valid != 0) && (para->sw_init_para->closed == 0))
		para->sw_init_para->sw_init_flag |=
		    (((u8) 1) << para->sw_init_para->output_channel);
	else
		para->sw_init_para->sw_init_flag = 0;

	return 0;
}

u32 disp_get_disp_rsl(void)
{				/* export_symbol */
	return g_sw_init_para.disp_rsl;
}
EXPORT_SYMBOL(disp_get_disp_rsl);

s32 disp_register_sync_proc(void (*proc) (u32))
{
	struct proc_list *new_proc;

	new_proc =
	    (struct proc_list *)disp_sys_malloc(sizeof(struct proc_list));

	if (new_proc) {
		new_proc->proc = proc;
		list_add_tail(&(new_proc->list), &(g_fbi.sync_proc_list.list));
	} else
		pr_warn("malloc fail in %s\n", __func__);

	return 0;
}

s32 disp_unregister_sync_proc(void (*proc) (u32))
{
	struct proc_list *ptr;

	if (proc == NULL) {
		pr_warn("hdl is NULL in %s\n", __func__);
		return -1;
	}
	list_for_each_entry(ptr, &g_fbi.sync_proc_list.list, list) {
		if (ptr->proc == proc) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	return -1;
}

s32 disp_register_sync_finish_proc(void (*proc) (u32))
{
	struct proc_list *new_proc;

	new_proc =
	    (struct proc_list *)disp_sys_malloc(sizeof(struct proc_list));

	if (new_proc) {
		new_proc->proc = proc;
		list_add_tail(&(new_proc->list),
			      &(g_fbi.sync_finish_proc_list.list));
	} else
		pr_warn("malloc fail in %s\n", __func__);

	return 0;
}

s32 disp_unregister_sync_finish_proc(void (*proc) (u32))
{
	struct proc_list *ptr;

	if (proc == NULL) {
		pr_warn("hdl is NULL in %s\n", __func__);
		return -1;
	}
	list_for_each_entry(ptr, &g_fbi.sync_finish_proc_list.list, list) {
		if (ptr->proc == proc) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	return -1;
}

static s32 disp_sync_finish_process(u32 screen_id)
{
	struct proc_list *ptr;

	list_for_each_entry(ptr, &g_fbi.sync_finish_proc_list.list, list) {
		if (ptr->proc)
			ptr->proc(screen_id);
	}

	return 0;
}

s32 disp_register_ioctl_func(unsigned int cmd,
			     int (*proc)(unsigned int cmd, unsigned long arg))
{
	struct ioctl_list *new_proc;

	new_proc =
	    (struct ioctl_list *)disp_sys_malloc(sizeof(struct ioctl_list));

	if (new_proc) {
		new_proc->cmd = cmd;
		new_proc->func = proc;
		list_add_tail(&(new_proc->list),
			      &(g_fbi.ioctl_extend_list.list));
	} else
		pr_warn("malloc fail in %s\n", __func__);

	return 0;
}

s32 disp_unregister_ioctl_func(unsigned int cmd)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_fbi.ioctl_extend_list.list, list) {
		if (ptr->cmd == cmd) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	pr_warn("no ioctl found(cmd:0x%x) in %s\n", cmd, __func__);
	return -1;
}

static s32 disp_ioctl_extend(unsigned int cmd, unsigned long arg)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_fbi.ioctl_extend_list.list, list) {
		if (cmd == ptr->cmd)
			return ptr->func(cmd, arg);
	}

	return -1;
}

s32 disp_register_standby_func(int (*suspend) (void), int (*resume) (void))
{
	struct standby_cb_list *new_proc;

	new_proc = (struct standby_cb_list *)
	    disp_sys_malloc(sizeof(struct standby_cb_list));

	if (new_proc) {
		new_proc->suspend = suspend;
		new_proc->resume = resume;
		list_add_tail(&(new_proc->list), &(g_fbi.stb_cb_list.list));
	} else
		pr_warn("malloc fail in %s\n", __func__);

	return 0;
}

s32 disp_unregister_standby_func(int (*suspend) (void), int (*resume) (void))
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_fbi.stb_cb_list.list, list) {
		if ((ptr->suspend == suspend) && (ptr->resume == resume)) {
			list_del(&ptr->list);
			kfree((void *)ptr);
			return 0;
		}
	}

	return -1;
}

static s32 disp_suspend_cb(void)
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_fbi.stb_cb_list.list, list) {
		if (ptr->suspend)
			return ptr->suspend();
	}

	return -1;
}

static s32 disp_resume_cb(void)
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_fbi.stb_cb_list.list, list) {
		if (ptr->resume)
			return ptr->resume();
	}

	return -1;
}

s32 DRV_DISP_Init(void)
{
	struct __disp_bsp_init_para para;
	int i;

	__inf("DRV_DISP_Init !\n");

	init_waitqueue_head(&g_fbi.wait[0]);
	init_waitqueue_head(&g_fbi.wait[1]);
	init_waitqueue_head(&g_fbi.wait[2]);
	g_fbi.wait_count[0] = 0;
	g_fbi.wait_count[1] = 0;
	g_fbi.wait_count[2] = 0;
	INIT_WORK(&g_fbi.resume_work[0], resume_work_0);
	INIT_WORK(&g_fbi.resume_work[1], resume_work_1);
	INIT_WORK(&g_fbi.resume_work[2], resume_work_2);
	INIT_WORK(&g_fbi.start_work, start_work);
	INIT_LIST_HEAD(&g_fbi.sync_proc_list.list);
	INIT_LIST_HEAD(&g_fbi.sync_finish_proc_list.list);
	INIT_LIST_HEAD(&g_fbi.ioctl_extend_list.list);
	INIT_LIST_HEAD(&g_fbi.stb_cb_list.list);
	mutex_init(&g_fbi.mlock);

	memset(&para, 0, sizeof(struct __disp_bsp_init_para));

	for (i = 0; i < DISP_MOD_NUM; i++) {
		para.reg_base[i] = g_fbi.reg_base[i];
		/* para.reg_size[i]     = (u32)g_fbi.reg_size[i]; */
		para.irq_no[i] = g_fbi.irq_no[i];
	}

	for (i = 0; i < DISP_CLK_NUM; i++)
		para.mclk[i] = g_fbi.mclk[i];

	para.disp_int_process = disp_sync_finish_process;
	para.vsync_event = DRV_disp_vsync_event;
	para.start_process = start_process;
	para.capture_event = capture_event;

	disp_parse_cmdline(&para);
	disp_para_check(&para);

	memset(&g_disp_drv, 0, sizeof(struct disp_drv_info));

	bsp_disp_init(&para);
	bsp_disp_open();

	start_process();

	__inf("DRV_DISP_Init end\n");
	return 0;
}
EXPORT_SYMBOL(DRV_DISP_Init);

s32 DRV_DISP_Exit(void)
{
	Fb_Exit();
	bsp_disp_close();
	bsp_disp_exit(g_disp_drv.exit_mode);
	return 0;
}

static int disp_mem_request(int sel, u32 size)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = 0;
	struct page *page;

	if (g_disp_mm[sel].info_base != 0)
		return -EINVAL;

	g_disp_mm[sel].mem_len = size;
	map_size = PAGE_ALIGN(g_disp_mm[sel].mem_len);

	page = alloc_pages(GFP_KERNEL, get_order(map_size));
	if (page != NULL) {
		g_disp_mm[sel].info_base = page_address(page);
		if (g_disp_mm[sel].info_base == 0) {
			free_pages((unsigned long)(page), get_order(map_size));
			__wrn("page_address fail!\n");
			return -ENOMEM;
		}
		g_disp_mm[sel].mem_start =
		    virt_to_phys(g_disp_mm[sel].info_base);
		memset(g_disp_mm[sel].info_base, 0, size);

		__inf("pa=0x%08lx va=0x%p size:0x%x\n",
		      g_disp_mm[sel].mem_start, g_disp_mm[sel].info_base, size);

	} else {
		__wrn("alloc_pages fail!\n");
		return -ENOMEM;
	}
	return 0;
#else
	u32 ret = 0;
	u32 phy_addr;

	ret = (u32) disp_malloc(size, &phy_addr);
	if (ret != 0) {
		g_disp_mm[sel].info_base = (void *)ret;
		g_disp_mm[sel].mem_start = phy_addr;
		g_disp_mm[sel].mem_len = size;
		memset(g_disp_mm[sel].info_base, 0, size);
		__inf("pa=0x%08lx va=0x%p size:0x%x\n",
		      g_disp_mm[sel].mem_start, g_disp_mm[sel].info_base, size);
	} else {
		__wrn("disp_malloc fail!\n");
		return -ENOMEM;
	}
	return 0;
#endif
}

static int disp_mem_release(int sel)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(g_disp_mm[sel].mem_len);
	unsigned page_size = map_size;

	if (g_disp_mm[sel].info_base == 0)
		return -EINVAL;

	free_pages((unsigned long)(g_disp_mm[sel].info_base),
		   get_order(page_size));
	memset(&g_disp_mm[sel], 0, sizeof(struct info_mm));
#else
	if (g_disp_mm[sel].info_base == NULL)
		return -EINVAL;

	__inf("disp_mem_release, mem_id=%d, phy_addr=0x%x\n",
	      sel, (unsigned int)g_disp_mm[sel].mem_start);
	disp_free((void *)g_disp_mm[sel].info_base,
		  (void *)g_disp_mm[sel].mem_start, g_disp_mm[sel].mem_len);
	memset(&g_disp_mm[sel], 0, sizeof(struct info_mm));
#endif
	return 0;
}

int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops)
{
	src_ops->sunxi_lcd_delay_ms = bsp_disp_lcd_delay_ms;
	src_ops->sunxi_lcd_delay_us = bsp_disp_lcd_delay_us;
	src_ops->sunxi_lcd_tcon_enable = bsp_disp_lcd_tcon_enable;
	src_ops->sunxi_lcd_tcon_disable = bsp_disp_lcd_tcon_disable;
	src_ops->sunxi_lcd_pwm_enable = bsp_disp_lcd_pwm_enable;
	src_ops->sunxi_lcd_pwm_disable = bsp_disp_lcd_pwm_disable;
	src_ops->sunxi_lcd_backlight_enable = bsp_disp_lcd_backlight_enable;
	src_ops->sunxi_lcd_backlight_disable = bsp_disp_lcd_backlight_disable;
	src_ops->sunxi_lcd_power_enable = bsp_disp_lcd_power_enable;
	src_ops->sunxi_lcd_power_disable = bsp_disp_lcd_power_disable;
	src_ops->sunxi_lcd_set_panel_funs = bsp_disp_lcd_set_panel_funs;
	src_ops->sunxi_lcd_dsi_write = dsi_dcs_wr;
	src_ops->sunxi_lcd_dsi_clk_enable = dsi_clk_enable;
	src_ops->sunxi_lcd_pin_cfg = bsp_disp_lcd_pin_cfg;
	src_ops->sunxi_lcd_gpio_set_value = bsp_disp_lcd_gpio_set_value;
	src_ops->sunxi_lcd_gpio_set_direction = bsp_disp_lcd_gpio_set_direction;
	return 0;
}
EXPORT_SYMBOL(sunxi_disp_get_source_ops);

int disp_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long mypfn = vma->vm_pgoff;
	unsigned long vmsize = vma->vm_end - vma->vm_start;

	vma->vm_pgoff = 0;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start,
			    mypfn, vmsize, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

int disp_open(struct inode *inode, struct file *file)
{
	return 0;
}

int disp_release(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t disp_read(struct file *file,
		  char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

ssize_t disp_write(struct file *file,
		   const char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static int disp_probe(struct platform_device *pdev)
{
	struct fb_info_t *info = NULL;
	int i;
	int ret = 0;
	/* struct resource      *res; */
	int counter = 0;

	__inf("[DISP]disp_probe\n");

	info = &g_fbi;
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev, info);

	/* iomap */
	/* de - [device(tcon-top)] - lcd0/1/2.. - dsi */
	counter = 0;
	info->reg_base[DISP_MOD_DE] =
	    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
	if (!info->reg_base[DISP_MOD_DE]) {
		dev_err(&pdev->dev, "unable to map de registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter++;

#if defined(HAVE_DEVICE_COMMON_MODULE)
	info->reg_base[DISP_MOD_DEVICE] =
	    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
	if (!info->reg_base[DISP_MOD_DEVICE]) {
		dev_err(&pdev->dev,
			"unable to map device commonmodule registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
	counter++;
#endif

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		info->reg_base[DISP_MOD_LCD0 + i] =
		    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
		if (!info->reg_base[DISP_MOD_LCD0 + i]) {
			dev_err(&pdev->dev,
				"unable to map timingcontroller %d registers\n",
				i);
			ret = -EINVAL;
			goto err_iomap;
		}
		counter++;
	}

#if defined(SUPPORT_DSI)
	info->reg_base[DISP_MOD_DSI0] =
	    (uintptr_t __force) of_iomap(pdev->dev.of_node, counter);
	if (!info->reg_base[DISP_MOD_DSI0]) {
		dev_err(&pdev->dev, "unable to map de registers\n");
		ret = -EINVAL;
		goto err_iomap;
	}
#endif

	/* parse and map irq */
	/* lcd0/1/2.. - dsi */
	counter = 0;
	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		info->irq_no[DISP_MOD_LCD0 + i] =
		    irq_of_parse_and_map(pdev->dev.of_node, counter);
		if (!info->irq_no[DISP_MOD_LCD0 + i])
			dev_err(&pdev->dev,
				"irq_of_parse_and_map irq %d fail for timing controller%d\n",
				counter, i);

		counter++;
	}

#if defined(SUPPORT_DSI)
	info->irq_no[DISP_MOD_DSI0] =
	    irq_of_parse_and_map(pdev->dev.of_node, counter);
	if (!info->irq_no[DISP_MOD_DSI0])
		dev_err(&pdev->dev,
			"irq_of_parse_and_map irq %d fail for dsi\n", i);

	counter++;
#endif

	info->irq_no[DISP_MOD_BE0] =
	    irq_of_parse_and_map(pdev->dev.of_node, counter);
	if (!info->irq_no[DISP_MOD_BE0])
		dev_err(&pdev->dev,
			"irq_of_parse_and_map irq %d fail for dsi\n", i);

	counter++;

	/* get clk */
	/* de - [device(tcon-top)] - lcd0/1/2.. - dsi - lvds - other */
	counter = 0;
	info->mclk[MOD_CLK_DEBE0] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->mclk[MOD_CLK_DEBE0]))
		dev_err(&pdev->dev, "fail to get clk for de\n");

	counter++;

	info->mclk[MOD_CLK_DEFE0] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->mclk[MOD_CLK_DEFE0]))
		dev_err(&pdev->dev, "fail to get clk for de\n");

	counter++;

#if defined(HAVE_DEVICE_COMMON_MODULE)
	info->.mclk[DISP_MOD_DEVICE] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->.mclk[DISP_MOD_DEVICE]))
		dev_err(&pdev->dev,
			"fail to get clk for device common module\n");

	counter++;
#endif

	for (i = 0; i < DISP_DEVICE_NUM; i++) {
		info->mclk[MOD_CLK_LCD0CH0 + i] =
		    of_clk_get(pdev->dev.of_node, counter);
		if (IS_ERR(info->mclk[MOD_CLK_LCD0CH0 + i]))
			dev_err(&pdev->dev,
				"fail to get clk for timing controller%d\n", i);

		counter++;
	}

#if defined(SUPPORT_DSI)
	info->mclk[MOD_CLK_MIPIDSIS] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->mclk[MOD_CLK_MIPIDSIS]))
		dev_err(&pdev->dev, "fail to get clk for dsi\n");

	counter++;

	info->mclk[MOD_CLK_MIPIDSIP] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->mclk[MOD_CLK_MIPIDSIP]))
		dev_err(&pdev->dev, "fail to get clk for lvds\n");

	counter++;
#endif

#if defined(SUPPORT_LVDS)
	info->mclk[MOD_CLK_LVDS] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->mclk[MOD_CLK_LVDS]))
		dev_err(&pdev->dev, "fail to get clk for lvds\n");

	counter++;
#endif

#ifndef CONFIG_ARCH_SUN3IW1P1
	info->mclk[MOD_CLK_IEPDRC0] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->mclk[MOD_CLK_IEPDRC0]))
		dev_err(&pdev->dev, "fail to get clk for lvds\n");

	counter++;

	info->mclk[MOD_CLK_SAT0] = of_clk_get(pdev->dev.of_node, counter);
	if (IS_ERR(info->mclk[MOD_CLK_SAT0]))
		dev_err(&pdev->dev, "fail to get clk for lvds\n");
	counter++;
#endif
	DRV_DISP_Init();
	Fb_Init(pdev);

	power_status_init = 1;
#if defined(CONFIG_PM_RUNTIME)
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 5000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#endif
	__inf("[DISP]disp_probe finish\n");

err_iomap:
	return ret;
}

static int disp_remove(struct platform_device *pdev)
{
	pr_info("disp_remove call\n");

	platform_set_drvdata(pdev, NULL);

	return 0;
}

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM_RUNTIME)
static void suspend(void)
{
	u32 screen_id = 0;
	int num_screens;
	u32 output_type;

	num_screens = bsp_disp_feat_get_num_screens();
#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) \
	|| defined(CONFIG_ARCH_SUN3IW1P1)
	disp_suspend_cb();
#endif
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		output_type = bsp_disp_get_output_type(screen_id);
		if (output_type == DISP_OUTPUT_TYPE_LCD)
			drv_lcd_disable(screen_id);
		else if (output_type == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			bsp_disp_hdmi_disable(screen_id);
#endif
		}
	}

	suspend_status |= DISPLAY_LIGHT_SLEEP;
	suspend_prestep = 0;

#if defined(CONFIG_ARCH_SUN9IW1P1)
	for (screen_id = 0; screen_id < num_screens; screen_id++)
		bsp_disp_hdmi_suspend(screen_id);

#endif
}

static void resume(void)
{
	u32 screen_id = 0;
	int num_screens;

#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || \
	defined(CONFIG_ARCH_SUN3IW1P1)
	disp_resume_cb();
#endif

	num_screens = bsp_disp_feat_get_num_screens();

#if defined(CONFIG_ARCH_SUN9IW1P1)
	if (suspend_prestep == 0) {
		/* FIXME */
		u32 temp;

		temp = readl((void __iomem *)(SUNXI_CCM_MOD_VBASE + 0x1a8));
		temp = temp | 0x4;
		writel(temp, (void __iomem *)(SUNXI_CCM_MOD_VBASE + 0x1a8));
	}

	for (screen_id = 0; screen_id < num_screens; screen_id++)
		bsp_disp_hdmi_resume(screen_id);

#endif
	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		if (suspend_output_type[screen_id] == DISP_OUTPUT_TYPE_LCD) {
			if (suspend_prestep == 0) {
				/* early_suspend -->  late_resume */
				drv_lcd_enable(screen_id);
			} else if (suspend_prestep != 3) {
				flush_work(&g_fbi.resume_work[screen_id]);
				bsp_disp_lcd_pwm_enable(screen_id);
				bsp_disp_lcd_backlight_enable(screen_id);
			}
		} else if (suspend_output_type[screen_id] ==
				DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			bsp_disp_hdmi_enable(screen_id);
#endif
		}
	}

	suspend_status &= (~DISPLAY_LIGHT_SLEEP);
	suspend_prestep = 3;

#if !defined(CONFIG_ARCH_SUN8IW3P1) && !defined(CONFIG_ARCH_SUN8IW5P1) \
	&& !defined(CONFIG_ARCH_SUN3IW1P1)
	disp_resume_cb();
#endif
}
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND)
void backlight_early_suspend(struct early_suspend *h)
{
	pr_info("early_suspend\n");
	suspend();
	pr_info("early_suspend finish\n");
}

void backlight_late_resume(struct early_suspend *h)
{
	pr_info("late_resume\n");
	resume();
	pr_info("late_resume finish\n");
}

static struct early_suspend backlight_early_suspend_handler = {

	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 200,
	.suspend = backlight_early_suspend,
	.resume = backlight_late_resume,
};
#endif

#if defined(CONFIG_PM_RUNTIME)
static int disp_runtime_suspend(struct device *dev)
{

	pr_info("%s\n", __func__);
	if (!g_pm_runtime_enable)
		return 0;

	suspend();
	pr_info("%s finish\n", __func__);

	return 0;
}

static int disp_runtime_resume(struct device *dev)
{
	pr_info("%s\n", __func__);
	if (!g_pm_runtime_enable)
		return 0;

	resume();
	pr_info("%s finish\n", __func__);

	return 0;
}

static int disp_runtime_idle(struct device *dev)
{
	pr_info("%s\n", __func__);

	if (g_fbi.dev) {
		pm_runtime_mark_last_busy(g_fbi.dev);
		pm_request_autosuspend(g_fbi.dev);
	} else {
		pr_warn("%s, display device is null\n", __func__);
	}

	/* return 0: for framework to request enter suspend.
		return non-zero: do susupend for myself;
	*/
	return -1;
}
#endif

int disp_suspend(struct device *dev)
{
	u32 screen_id = 0;
	int num_screens;
	u32 output_type;

	pr_info("disp_suspend\n");

	num_screens = bsp_disp_feat_get_num_screens();
#if defined(CONFIG_PM_RUNTIME)
	if (!pm_runtime_status_suspended(g_fbi.dev)) {
#else
	{
#endif
		if (!g_pm_runtime_enable)
			disp_suspend_cb();

		for (screen_id = 0; screen_id < num_screens; screen_id++) {
			output_type =
			    bsp_disp_get_output_type(screen_id);
			if (output_type == DISP_OUTPUT_TYPE_LCD) {
				if ((suspend_prestep == 2)
				     && g_pm_runtime_enable)
					flush_work(&g_fbi.resume_work[screen_id]);
				drv_lcd_disable(screen_id);
			} else if (output_type == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
				bsp_disp_hdmi_disable(screen_id);
#endif
			}
		}
	}

	suspend_status |= DISPLAY_DEEP_SLEEP;
	suspend_prestep = 1;
#if defined(CONFIG_ARCH_SUN9IW1P1)
	if (!g_pm_runtime_enable) {
		for (screen_id = 0; screen_id < num_screens; screen_id++)
			bsp_disp_hdmi_suspend(screen_id);
	}
#endif

#if defined(CONFIG_PM_RUNTIME)
	if (g_pm_runtime_enable) {
		pm_runtime_disable(g_fbi.dev);
		pm_runtime_set_suspended(g_fbi.dev);
		pm_runtime_enable(g_fbi.dev);
	}
#endif
	pr_info("disp_suspend finish\n");

	return 0;
}

int disp_resume(struct device *dev)
{
	u32 screen_id = 0;
	int num_screens;

	pr_info("disp_resume\n");

#if defined(CONFIG_ARCH_SUN9IW1P1)
	{
		/* FIXME */
		u32 temp;

		temp = readl((void __iomem *)(SUNXI_CCM_MOD_VBASE + 0x1a8));
		temp = temp | 0x4;
		writel(temp, (void __iomem *)(SUNXI_CCM_MOD_VBASE + 0x1a8));
	}
#endif
	num_screens = bsp_disp_feat_get_num_screens();
#if defined(CONFIG_ARCH_SUN9IW1P1)
	if (!g_pm_runtime_enable)
		for (screen_id = 0; screen_id < num_screens; screen_id++)
			bsp_disp_hdmi_resume(screen_id);
#endif

	for (screen_id = 0; screen_id < num_screens; screen_id++) {

		if (g_pm_runtime_enable) {
			if (suspend_output_type[screen_id] ==
			    DISP_OUTPUT_TYPE_LCD)
				schedule_work(&g_fbi.resume_work[screen_id]);
			continue;
		}

		if (suspend_output_type[screen_id] == DISP_OUTPUT_TYPE_LCD)
			drv_lcd_enable(screen_id);
		else if (suspend_output_type[screen_id] ==
			 DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			bsp_disp_hdmi_enable(screen_id);
#endif
		}
	}

	pr_info("disp_resume finish\n");
	suspend_status &= (~DISPLAY_DEEP_SLEEP);
	suspend_prestep = 2;

	if (g_pm_runtime_enable) {
		if (g_fbi.dev) {
#if defined(CONFIG_PM_RUNTIME)
			pm_runtime_disable(g_fbi.dev);
			pm_runtime_set_active(g_fbi.dev);
			pm_runtime_enable(g_fbi.dev);
#endif
		} else {
			pr_warn("%s, display device is null\n", __func__);
		}
	} else {
		disp_resume_cb();
	}

	return 0;
}

static const struct dev_pm_ops disp_runtime_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = disp_runtime_suspend,
	.runtime_resume  = disp_runtime_resume,
	.runtime_idle    = disp_runtime_idle,
#endif
	.suspend  = disp_suspend,
	.resume   = disp_resume,
};

static void disp_shutdown(struct platform_device *pdev)
{
	u32 type = 0, screen_id = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for (screen_id = 0; screen_id < num_screens; screen_id++) {
		type = bsp_disp_get_output_type(screen_id);
		if (type == DISP_OUTPUT_TYPE_LCD)
			drv_lcd_disable(screen_id);
		else if (type == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			bsp_disp_hdmi_disable(screen_id);
#endif
		}
	}

}

long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long karg[4];
	unsigned long ubuffer[4] = { 0 };
	int ret = 0;
	int num_screens = 2;

	num_screens = bsp_disp_feat_get_num_screens();

	if (copy_from_user((void *)karg,
			   (void __user *)arg, 4 * sizeof(unsigned long))) {
		__wrn("copy_from_user fail\n");
		return -EFAULT;
	}

	ubuffer[0] = *(unsigned long *)karg;
	ubuffer[1] = (*(unsigned long *)(karg + 1));
	ubuffer[2] = (*(unsigned long *)(karg + 2));
	ubuffer[3] = (*(unsigned long *)(karg + 3));

	if (cmd < DISP_CMD_FB_REQUEST) {
		if (ubuffer[0] >= num_screens) {
			__wrn("para err in disp_ioctl, cmd = 0x%x\n", cmd);
			__wrn("screen id = %d", (int)ubuffer[0]);
			return -1;
		}
	}
	if (DISPLAY_DEEP_SLEEP & suspend_status) {
		__wrn("ioctl:%x fail when in suspend!\n", cmd);
		return -1;
	}

	if (cmd == disp_cmd_print)
		PRINTF("cmd:0x%x,%ld,%ld\n", cmd, ubuffer[0], ubuffer[1]);

	switch (cmd) {
		/* ----disp global---- */
	case DISP_CMD_SET_BKCOLOR:
		{
			disp_color_info para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[1],
					   sizeof(disp_color_info))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = bsp_disp_set_back_color(ubuffer[0], &para);
			break;
		}

	case DISP_CMD_SET_COLORKEY:
		{
			disp_colorkey para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[1],
					   sizeof(disp_colorkey))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = bsp_disp_set_color_key(ubuffer[0], &para);
			break;
		}

	case DISP_CMD_GET_OUTPUT_TYPE:
		if (suspend_status == DISPLAY_NORMAL)
			ret = bsp_disp_get_output_type(ubuffer[0]);
		else
			ret = suspend_output_type[ubuffer[0]];
#if defined(CONFIG_HOMLET_PLATFORM)
		if (ret == DISP_OUTPUT_TYPE_LCD)
			ret = bsp_disp_get_lcd_output_type(ubuffer[0]);
#endif
		break;

	case DISP_CMD_GET_SCN_WIDTH:
		ret = bsp_disp_get_screen_width(ubuffer[0]);
		break;

	case DISP_CMD_GET_SCN_HEIGHT:
		ret = bsp_disp_get_screen_height(ubuffer[0]);
		break;

	case DISP_CMD_SHADOW_PROTECT:
		ret = bsp_disp_shadow_protect(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_VSYNC_EVENT_EN:
		ret = bsp_disp_vsync_event_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_BLANK:
	{
		/* only response main device' blank request */

		if (!g_pm_runtime_enable)
			break;

		if (0 != ubuffer[0])
			break;

		if (ubuffer[1]) {
#if defined(CONFIG_PM_RUNTIME)
			if (g_fbi.dev)
				pm_runtime_put(g_fbi.dev);
			else
				pr_warn("%s, disp device is null\n", __func__);
#endif
		} else {
			if (power_status_init) {
				/*
				 * avoid first unblank, because device
				 * is ready when driver init
				 */
				power_status_init = 0;
				break;
			}

#if defined(CONFIG_PM_RUNTIME)
			if (g_fbi.dev) {
				/* recover the pm_runtime status */
				pm_runtime_disable(g_fbi.dev);
				pm_runtime_set_suspended(g_fbi.dev);
				pm_runtime_enable(g_fbi.dev);
				pm_runtime_get_sync(g_fbi.dev);
			} else {
				pr_warn("%s, disp device is null\n", __func__);
			}
#endif
		}
		break;
	}

		/* ----layer---- */
	case DISP_CMD_LAYER_ENABLE:
		ret = bsp_disp_layer_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_DISABLE:
		ret = bsp_disp_layer_disable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_INFO:
		{
			disp_layer_info para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[2],
					   sizeof(disp_layer_info))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = bsp_disp_layer_set_info(ubuffer[0],
						      ubuffer[1], &para);
			break;
		}

	case DISP_CMD_LAYER_GET_INFO:
		{
			disp_layer_info para;

			ret = bsp_disp_layer_get_info(ubuffer[0],
						      ubuffer[1], &para);
			if (copy_to_user((void __user *)ubuffer[2],
					 &para, sizeof(disp_layer_info))) {
				__wrn("copy_to_user fail\n");
				return -EFAULT;
			}
			break;
		}

	case DISP_CMD_LAYER_GET_FRAME_ID:
		ret = bsp_disp_layer_get_frame_id(ubuffer[0], ubuffer[1]);
		break;

		/* ----lcd---- */
	case DISP_CMD_LCD_ENABLE:
		ret = drv_lcd_enable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_LCD;

		break;

	case DISP_CMD_LCD_DISABLE:
		ret = drv_lcd_disable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		break;

	case DISP_CMD_LCD_SET_BRIGHTNESS:
		ret = bsp_disp_lcd_set_bright(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LCD_GET_BRIGHTNESS:
		ret = bsp_disp_lcd_get_bright(ubuffer[0]);
		break;

	case DISP_CMD_LCD_BACKLIGHT_ENABLE:
		if (suspend_status == DISPLAY_NORMAL)
			ret = bsp_disp_lcd_backlight_enable(ubuffer[0]);
		break;

	case DISP_CMD_LCD_BACKLIGHT_DISABLE:
		if (suspend_status == DISPLAY_NORMAL)
			ret = bsp_disp_lcd_backlight_disable(ubuffer[0]);
		break;
#if defined(CONFIG_ARCH_SUN9IW1P1)
		/* ----hdmi---- */
	case DISP_CMD_HDMI_ENABLE:
		ret = bsp_disp_hdmi_enable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_HDMI;
		break;

	case DISP_CMD_HDMI_DISABLE:
		ret = bsp_disp_hdmi_disable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		break;

	case DISP_CMD_HDMI_SET_MODE:
		ret = bsp_disp_hdmi_set_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_HDMI_GET_MODE:
		ret = bsp_disp_hdmi_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_HDMI_SUPPORT_MODE:
		ret = bsp_disp_hdmi_check_support_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_HDMI_GET_HPD_STATUS:
		if (suspend_status == DISPLAY_NORMAL)
			ret = bsp_disp_hdmi_get_hpd_status(ubuffer[0]);
		else
			ret = 0;
		break;
	case DISP_CMD_HDMI_GET_VENDOR_ID:
		{
			__u8 vendor_id[2] = { 0 };

			bsp_disp_hdmi_get_vendor_id(ubuffer[0], vendor_id);
			ret = vendor_id[1] << 8 | vendor_id[0];
			__inf("vendor id [1]=%x, [0]=%x. --> %x",
			      vendor_id[1], vendor_id[0], ret);
		}
		break;

	case DISP_CMD_HDMI_GET_EDID:
		{
			u8 *buf;
			u32 bytes = 1024;

			ret = 0;
			buf = (u8 *) bsp_disp_hdmi_get_edid(ubuffer[0]);
			if (buf) {
				bytes =
				    (ubuffer[2] > bytes) ? bytes : ubuffer[2];
				if (copy_to_user
				    ((void __user *)ubuffer[1], buf, bytes))
					__wrn("copy_to_user fail\n");
				else
					ret = bytes;
			}

			break;
		}
#endif
#if 0
	case DISP_CMD_HDMI_SET_SRC:
		ret = bsp_disp_hdmi_set_src(ubuffer[0],
					    (disp_lcd_src) ubuffer[1]);
		break;
#endif
		/* ----framebuffer---- */
	case DISP_CMD_FB_REQUEST:
		{
			disp_fb_create_info para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[1],
					   sizeof(disp_fb_create_info))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = Display_Fb_Request(ubuffer[0], &para);
			break;
		}

	case DISP_CMD_FB_RELEASE:
		ret = Display_Fb_Release(ubuffer[0]);
		break;
#if 0
	case DISP_CMD_FB_GET_PARA:
		{
			disp_fb_create_info para;

			ret = Display_Fb_get_para(ubuffer[0], &para);
			if (copy_to_user((void __user *)ubuffer[1],
					 &para, sizeof(disp_fb_create_info))) {
				__wrn("copy_to_user fail\n");
				return -EFAULT;
			}
			break;
		}

	case DISP_CMD_GET_DISP_INIT_PARA:
		{
			disp_init_para para;

			ret = Display_get_disp_init_para(&para);
			if (copy_to_user((void __user *)ubuffer[0],
					 &para, sizeof(disp_init_para))) {
				__wrn("copy_to_user fail\n");
				return -EFAULT;
			}
			break;
		}

#endif

		/* capture */
	case DISP_CMD_CAPTURE_SCREEN:
		ret = bsp_disp_capture_screen(ubuffer[0],
					      (disp_capture_para *) ubuffer[1]);
		break;

	case DISP_CMD_CAPTURE_SCREEN_STOP:
		ret = bsp_disp_capture_screen_stop(ubuffer[0]);
		break;

		/* ----enhance---- */
	case DISP_CMD_SET_BRIGHT:
		ret = bsp_disp_smcl_set_bright(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_BRIGHT:
		ret = bsp_disp_smcl_get_bright(ubuffer[0]);
		break;

	case DISP_CMD_SET_CONTRAST:
		ret = bsp_disp_smcl_set_contrast(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_CONTRAST:
		ret = bsp_disp_smcl_get_contrast(ubuffer[0]);
		break;

	case DISP_CMD_SET_SATURATION:
		ret = bsp_disp_smcl_set_saturation(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_SATURATION:
		ret = bsp_disp_smcl_get_saturation(ubuffer[0]);
		break;

	case DISP_CMD_SET_HUE:
		ret = bsp_disp_smcl_set_hue(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_HUE:
		ret = bsp_disp_smcl_get_hue(ubuffer[0]);
		break;

	case DISP_CMD_ENHANCE_ENABLE:
		ret = bsp_disp_smcl_enable(ubuffer[0]);
		break;

	case DISP_CMD_ENHANCE_DISABLE:
		ret = bsp_disp_smcl_disable(ubuffer[0]);
		break;

	case DISP_CMD_GET_ENHANCE_EN:
		ret = bsp_disp_smcl_is_enabled(ubuffer[0]);
		break;

	case DISP_CMD_SET_ENHANCE_MODE:
		ret = bsp_disp_smcl_set_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_ENHANCE_MODE:
		ret = bsp_disp_smcl_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_SET_ENHANCE_WINDOW:
		{
			disp_window para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[1],
					   sizeof(disp_window))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = bsp_disp_smcl_set_window(ubuffer[0], &para);
			break;
		}

	case DISP_CMD_GET_ENHANCE_WINDOW:
		{
			disp_window para;

			ret = bsp_disp_smcl_get_window(ubuffer[0], &para);
			if (copy_to_user((void __user *)ubuffer[1],
					 &para, sizeof(disp_window))) {
				__wrn("copy_to_user fail\n");
				return -EFAULT;
			}
			break;
		}

	case DISP_CMD_DRC_ENABLE:
		ret = bsp_disp_smbl_enable(ubuffer[0]);
		break;

	case DISP_CMD_DRC_DISABLE:
		ret = bsp_disp_smbl_disable(ubuffer[0]);
		break;

	case DISP_CMD_GET_DRC_EN:
		ret = bsp_disp_smbl_is_enabled(ubuffer[0]);
		break;

	case DISP_CMD_DRC_SET_WINDOW:
		{
			disp_window para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[1],
					   sizeof(disp_window))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = bsp_disp_smbl_set_window(ubuffer[0], &para);
			break;
		}

	case DISP_CMD_DRC_GET_WINDOW:
		{
			disp_window para;

			ret = bsp_disp_smbl_get_window(ubuffer[0], &para);
			if (copy_to_user((void __user *)ubuffer[1],
					 &para, sizeof(disp_window))) {
				__wrn("copy_to_user fail\n");
				return -EFAULT;
			}
			break;
		}

#if defined(CONFIG_ARCH_SUN9IW1P1)
		/* ---- cursor ---- */
	case DISP_CMD_CURSOR_ENABLE:
		ret = bsp_disp_cursor_enable(ubuffer[0]);
		break;

	case DISP_CMD_CURSOR_DISABLE:
		ret = bsp_disp_cursor_disable(ubuffer[0]);
		break;

	case DISP_CMD_CURSOR_SET_POS:
		{
			disp_position para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[1],
					   sizeof(disp_position))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = bsp_disp_cursor_set_pos(ubuffer[0], &para);
			break;
		}

	case DISP_CMD_CURSOR_GET_POS:
		{
			disp_position para;

			ret = bsp_disp_cursor_get_pos(ubuffer[0], &para);
			if (copy_to_user((void __user *)ubuffer[1],
					 &para, sizeof(disp_position))) {
				__wrn("copy_to_user fail\n");
				return -EFAULT;
			}
			break;
		}

	case DISP_CMD_CURSOR_SET_FB:
		{
			disp_cursor_fb para;

			if (copy_from_user(&para,
					   (void __user *)ubuffer[1],
					   sizeof(disp_cursor_fb))) {
				__wrn("copy_from_user fail\n");
				return -EFAULT;
			}
			ret = bsp_disp_cursor_set_fb(ubuffer[0], &para);
			break;
		}

	case DISP_CMD_CURSOR_SET_PALETTE:
		if ((ubuffer[1] == 0) || ((int)ubuffer[3] <= 0)) {
			__wrn("para invalid ,buffer:0x%x,size:0x%x\n",
			      (unsigned int)ubuffer[1],
			      (unsigned int)ubuffer[3]);
			return -1;
		}
		if (copy_from_user(gbuffer, (void __user *)ubuffer[1],
				   ubuffer[3])) {
			__wrn("copy_from_user fail\n");
			return -EFAULT;
		}
		ret = bsp_disp_cursor_set_palette(ubuffer[0], (void *)gbuffer,
						  ubuffer[2], ubuffer[3]);
		break;
#endif
		/* ----for test---- */
	case DISP_CMD_MEM_REQUEST:
		ret = disp_mem_request(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_MEM_RELEASE:
		ret = disp_mem_release(ubuffer[0]);
		break;

	case DISP_CMD_MEM_SELIDX:
		g_disp_mm_sel = ubuffer[0];
		break;

	case DISP_CMD_MEM_GETADR:
		ret = g_disp_mm[ubuffer[0]].mem_start;
		break;

/* case DISP_CMD_PRINT_REG: */
/* ret = bsp_disp_print_reg(1, ubuffer[0], 0); */
/* break; */

		/* ----for tv ---- */
	case DISP_CMD_TV_ON:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = drv_lcd_enable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_LCD;
#endif
		break;
	case DISP_CMD_TV_OFF:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = drv_lcd_disable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
#endif
		break;
	case DISP_CMD_TV_GET_MODE:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = bsp_disp_lcd_get_tv_mode(ubuffer[0]);
#endif
		break;
	case DISP_CMD_TV_SET_MODE:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = bsp_disp_lcd_set_tv_mode(ubuffer[0], ubuffer[1]);
#endif
		break;

	default:
		ret = disp_ioctl_extend(cmd, (unsigned long)ubuffer);
		break;
	}

	return ret;
}

static const struct file_operations disp_fops = {
	.owner = THIS_MODULE,
	.open = disp_open,
	.release = disp_release,
	.write = disp_write,
	.read = disp_read,
	.unlocked_ioctl = disp_ioctl,
	.mmap = disp_mmap,
};

#if !defined(CONFIG_OF)
static struct platform_device disp_device = {
	.name = "disp",
	.id = -1,
	.num_resources = ARRAY_SIZE(disp_resource),
	.resource = disp_resource,
	.dev = {

		.power = {

			  .async_suspend = 1,
			  }
		}
};
#else
static const struct of_device_id sunxi_disp_match[] = {
	{.compatible = "allwinner,sunxi-disp",},
	{},
};
#endif

static struct platform_driver disp_driver = {
	.probe = disp_probe,
	.remove = disp_remove,
	.shutdown = disp_shutdown,
	.driver = {
		   .name = "disp",
		   .owner = THIS_MODULE,
		   .pm = &disp_runtime_pm_ops,
		   .of_match_table = sunxi_disp_match,
		   },
};

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC
static struct ddrfreq_vb_time_ops ddrfreq_ops = {

	.get_vb_time = bsp_disp_get_vb_time,
	.get_next_vb_time = bsp_disp_get_next_vb_time,
	.is_in_vb = bsp_disp_is_in_vb,
};
#endif

static int __init disp_module_init(void)
{
	int ret = 0, err;

	pr_info("[DISP]disp_module_init\n");

	alloc_chrdev_region(&devid, 0, 1, "disp");
	my_cdev = cdev_alloc();
	cdev_init(my_cdev, &disp_fops);
	my_cdev->owner = THIS_MODULE;
	err = cdev_add(my_cdev, devid, 1);
	if (err) {
		__wrn("cdev_add fail\n");
		return -1;
	}

	disp_class = class_create(THIS_MODULE, "disp");
	if (IS_ERR(disp_class)) {
		__wrn("class_create fail\n");
		return -1;
	}

	display_dev = device_create(disp_class, NULL, devid, NULL, "disp");
#if !defined(CONFIG_OF)
	ret = platform_device_register(&disp_device);
#endif

	if (ret == 0)
		ret = platform_driver_register(&disp_driver);

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&backlight_early_suspend_handler);
#endif
	disp_attr_node_init(display_dev);
	capture_module_init();

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC
	ddrfreq_set_vb_time_ops(&ddrfreq_ops);
#endif

	pr_info("[DISP]disp_module_init finish\n");

	return ret;
}

static void __exit disp_module_exit(void)
{
	__inf("disp_module_exit\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&backlight_early_suspend_handler);
#endif
	DRV_DISP_Exit();
	capture_module_exit();

	platform_driver_unregister(&disp_driver);
#if !defined(CONFIG_OF)
	platform_device_unregister(&disp_device);
#endif

	device_destroy(disp_class, devid);
	class_destroy(disp_class);

	cdev_del(my_cdev);
}

module_init(disp_module_init);
module_exit(disp_module_exit);

MODULE_AUTHOR("tyle");
MODULE_DESCRIPTION("display driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:disp");
