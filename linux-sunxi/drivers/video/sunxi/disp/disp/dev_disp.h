/* display driver
 *
 * Copyright (c) 2017 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __DEV_DISP_H__
#define __DEV_DISP_H__

#include "drv_disp_i.h"
#include "disp_sys_intf.h"

#define FB_RESERVED_MEM

#define DISPLAY_NORMAL 0
#define DISPLAY_LIGHT_SLEEP 1
#define DISPLAY_DEEP_SLEEP 2

struct info_mm {
	void *info_base;	/* Virtual address */
	unsigned long mem_start;	/* Start of frame buffer mem */
	/* (physical address) */
	u32 mem_len;		/* Length of frame buffer mem */
};

struct proc_list {
	void (*proc)(u32 screen_id);
	struct list_head list;
};

struct ioctl_list {
	unsigned int cmd;
	int (*func)(unsigned int cmd, unsigned long arg);
	struct list_head list;
};

struct standby_cb_list {
	int (*suspend)(void);
	int (*resume)(void);
	struct list_head list;
};

struct fb_info_t {
	struct device *dev;
	uintptr_t reg_base[DISP_MOD_NUM];
	u32 reg_size[DISP_MOD_NUM];
	u32 irq_no[DISP_MOD_NUM];

	struct clk *mclk[DISP_CLK_NUM];

	disp_init_para disp_init;

	bool fb_enable[FB_MAX];
	disp_fb_mode fb_mode[FB_MAX];

	/* [fb_id][0]:screen0 layer handle; */
	/* [fb_id][1]:screen1 layer handle */
	u32 layer_hdl[FB_MAX][3];
	struct fb_info *fbinfo[FB_MAX];
	disp_fb_create_info fb_para[FB_MAX];
	u32 pseudo_palette[FB_MAX][16];
	wait_queue_head_t wait[3];
	unsigned long wait_count[3];
	struct work_struct vsync_work[3];
	struct work_struct resume_work[3];
	struct work_struct start_work;
	ktime_t vsync_timestamp[3];

	int blank[3];

	struct proc_list sync_proc_list;
	struct proc_list sync_finish_proc_list;
	struct ioctl_list ioctl_extend_list;
	struct standby_cb_list stb_cb_list;
	struct mutex mlock;
};

struct disp_drv_info {
	u32 exit_mode;		/* 0:clean all  1:disable interrupt */
	bool b_lcd_enabled[3];
};

struct sunxi_disp_mod {
	disp_mod_id id;
	char name[32];
};

struct __fb_addr_para {
	int fb_paddr;
	int fb_size;
};

#define DISP_RESOURCE(res_name, res_start, res_end, res_flags) \
{\
	.start = (int __force)res_start, \
	.end = (int __force)res_end, \
	.flags = res_flags, \
	.name = #res_name \
},

int disp_open(struct inode *inode, struct file *file);
int disp_release(struct inode *inode, struct file *file);
ssize_t disp_read(struct file *file,
		  char __user *buf, size_t count, loff_t *ppos);
ssize_t disp_write(struct file *file,
		   const char __user *buf, size_t count, loff_t *ppos);
int disp_mmap(struct file *file, struct vm_area_struct *vma);
long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

s32 disp_create_heap(u32 pHeapHead, u32 pHeapHeadPhy, u32 nHeapSize);
void *disp_malloc(u32 num_bytes, u32 *phy_addr);
void disp_free(void *virt_addr, void *phy_addr, u32 num_bytes);

extern s32 Display_Fb_Request(u32 fb_id, disp_fb_create_info *fb_para);
extern s32 Display_Fb_Release(u32 fb_id);
extern s32 Display_Fb_get_para(u32 fb_id, disp_fb_create_info *fb_para);
extern s32 Display_get_disp_init_para(disp_init_para *init_para);

extern s32 DRV_disp_vsync_event(u32 sel);
extern s32 capture_event(u32 sel);
extern s32 DRV_disp_take_effect_event(u32 sel);
extern s32 disp_register_sync_proc(void (*proc) (u32));
extern s32 disp_unregister_sync_proc(void (*proc) (u32));
extern s32 disp_register_sync_finish_proc(void (*proc) (u32));
extern s32 disp_unregister_sync_finish_proc(void (*proc) (u32));
extern s32 disp_register_ioctl_func(unsigned int cmd,
				    int (*proc)(unsigned int cmd,
						 unsigned long arg));
extern s32 disp_unregister_ioctl_func(unsigned int cmd);
extern s32 disp_register_standby_func(int (*suspend) (void),
				      int (*resume)(void));
extern s32 disp_unregister_standby_func(int (*suspend) (void),
					int (*resume)(void));

extern s32 DRV_DISP_Init(void);
extern s32 DRV_DISP_Exit(void);
extern int disp_suspend(struct device *dev);
extern int disp_resume(struct device *dev);

s32 disp_set_hdmi_func(u32 screen_id, disp_hdmi_func *func);
s32 disp_set_hdmi_hpd(u32 hpd);
int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops);
__s32 capture_event(__u32 sel);
int capture_module_init(void);
void capture_module_exit(void);
__s32 sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para);
__s32 fb_draw_colorbar(__u32 base,
		       __u32 width, __u32 height,
		       struct fb_var_screeninfo *var);
__s32 fb_draw_gray_pictures(__u32 base, __u32 width, __u32 height,
			    struct fb_var_screeninfo *var);
int disp_attr_node_init(struct device *display_dev);
int disp_attr_node_exit(void);
u32 get_fastboot_mode(void);
void DRV_disp_int_process(u32 sel);

extern struct disp_drv_info g_disp_drv;
extern u32 fastboot;
extern struct fb_info_t g_fbi;
extern u32 disp_get_disp_rsl(void);

extern s32 DRV_lcd_open(u32 sel);
extern s32 DRV_lcd_close(u32 sel);
extern s32 Fb_Init(struct platform_device *pdev);
extern s32 Fb_Exit(void);
extern int disp_attr_node_init(struct device *display_dev);
extern int capture_module_init(void);
extern void capture_module_exit(void);
#ifdef CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC
struct ddrfreq_vb_time_ops {
	int (*get_vb_time)(void);
	int (*get_next_vb_time)(void);
	int (*is_in_vb)(void);
};
extern int ddrfreq_set_vb_time_ops(struct ddrfreq_vb_time_ops *ops);
#endif
extern s32 dsi_clk_enable(u32 sel, u32 en);
extern s32 dsi_dcs_wr(u32 sel, u8 cmd, u8 *para_p, u32 para_num);
extern int Fb_wait_for_vsync(struct fb_info *info);
extern __s32 fb_draw_colorbar(__u32 base,
			      __u32 width, __u32 height,
			      struct fb_var_screeninfo *var);
extern s32 drv_lcd_enable(u32 sel);
extern s32 drv_lcd_disable(u32 sel);

extern s32 disp_video_set_dit_mode(u32 scaler_index, u32 mode);
extern s32 disp_video_get_dit_mode(u32 scaler_index);
extern s32 disp_get_frame_count(u32 screen_id, char *buf);

extern int dispc_blank(int disp, int blank);
s32 disp_get_frame_count(u32 screen_id, char *buf);
s32 drv_lcd_enable(u32 sel);
s32 drv_lcd_disable(u32 sel);
int Fb_wait_for_vsync(struct fb_info *info);
extern bool g_pm_runtime_enable;

/* composer */
#if defined(CONFIG_ARCH_SUN6I) || \
	defined(CONFIG_ARCH_SUN8IW3P1) || \
	defined(CONFIG_ARCH_SUN8IW5P1) || \
	defined(CONFIG_ARCH_SUN3IW1P1) || \
	defined(CONFIG_ARCH_SUN9IW1P1)
extern s32 composer_init(void);
#endif

#endif
