#ifndef DEV_COMPOSER_C_C
#define DEV_COMPOSER_C_C

#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
#include <linux/slab.h>

#include "dev_disp.h"
#include <video/sunxi_display2.h>
static struct   mutex	gcommit_mutek;

#define DBG_TIME_TYPE 3
#define DBG_TIME_SIZE 100
#define DISP_NUMS_SCREEN 2

#define HWC_CMD_GET_FPS			(0x00000001)
#define HWC_CMD_GET_CLK_RATE	(0x00000002)
#define HWC_CMD_SET_SUNXI_STREAM (0x00000003)

#define HWCOMPOSER2_START (0x1)

typedef struct {
	unsigned long         time[DBG_TIME_TYPE][DBG_TIME_SIZE];
	unsigned int          time_index[DBG_TIME_TYPE];
	unsigned int          count[DBG_TIME_TYPE];
} composer_health_info;

typedef struct {
	int outaquirefencefd;
	int width;
	int Wstride;
	int height;
	enum disp_pixel_format format;
	unsigned int phys_addr;
} WriteBack_t;

typedef struct {
	int deviceid;
	int layer_num;
	struct disp_layer_config *hwclayer;
	int *fencefd;
	bool need_writeback;
	unsigned int ehancemode;
	unsigned int androidfrmnum;
	WriteBack_t *writeback_data;
} setup_dispc_data_t;

#ifdef CONFIG_COMPAT
typedef struct {
	int deviceid;
	int layer_num;
	compat_uptr_t hwclayer;
	compat_uptr_t fencefd;
	bool need_writeback;
	unsigned int ehancemode;
	unsigned int androidfrmnum;
	compat_uptr_t WriteBackdata;
} compat_setup_dispc_data_t;
#endif

typedef struct {
	setup_dispc_data_t hwc_data;
	struct disp_layer_config hwclayer[16];
	struct sync_fence *acquire_fence[16];
} disp_frame_t;

typedef struct {
	struct list_head list;
	unsigned int framenumber;
	unsigned int androidfrmnum;
	disp_frame_t frame[2];
} dispc_data_list_t;

typedef struct {
    struct list_head    list;
    void *vm_addr;
    void *p_addr;
    unsigned int        size;
    unsigned  int       androidfrmnum;
    bool                isphaddr;
    bool                update;
} dumplayer_t;

struct composer_private_data {
	struct work_struct    post2_cb_work;
	u32	                  Cur_Write_Cnt;
	u32                   Cur_Disp_Cnt[2];
	bool                  b_no_output;
    bool                  display_active[2];
    bool                  countrotate[2];
	struct                mutex	runtime_lock;
	struct list_head        update_regs_list;

	unsigned int            timeline_max;
	struct mutex            update_regs_list_lock;
	spinlock_t              update_reg_lock;
	struct work_struct      commit_work;
    struct workqueue_struct *Display_commit_work;
    struct sw_sync_timeline *relseastimeline;
    struct sw_sync_timeline *writebacktimeline;
    setup_dispc_data_t      tmptransfer[DISP_NUMS_SCREEN];
    disp_drv_info           *psg_disp_drv;

    struct list_head        dumplyr_list;
    spinlock_t              dumplyr_lock;
    unsigned int            listcnt;
    unsigned char           dumpCnt;
    unsigned char           display;
    unsigned char           layerNum;
    unsigned char           channelNum;
    bool                    pause;
    bool                    cancel;
    bool                    dumpstart;
    unsigned int            ehancemode[2];
	composer_health_info    health_info;
};
static struct composer_private_data composer_priv;
static int s_stop;
static int s_start;

extern int composer2_start_extern(disp_drv_info *p_disp_drv, int client);
int dispc_gralloc_queue(dispc_data_list_t *psDispcData, unsigned int framenuber);
s32 composer_stop(disp_drv_info *psg_disp_drv);

/* type: 0:acquire, 1:release; 2:display */
static s32 composer_get_frame_fps(u32 type)
{
	__u32 pre_time_index, cur_time_index;
	__u32 pre_time, cur_time;
	__u32 fps = 0xff;

	pre_time_index = composer_priv.health_info.time_index[type];
	cur_time_index = (pre_time_index == 0) ? (DBG_TIME_SIZE - 1) : (pre_time_index - 1);

	pre_time = composer_priv.health_info.time[type][pre_time_index];
	cur_time = composer_priv.health_info.time[type][cur_time_index];

	if (pre_time != cur_time) {
		fps = 1000 * 100 / (cur_time - pre_time);
	}

	return fps;
}

/* type: 0:acquire, 1:release; 2:display */
static void composer_frame_checkin(u32 type)
{
	u32 index = composer_priv.health_info.time_index[type];
	composer_priv.health_info.time[type][index] = jiffies;
	index++;
	index = (index >= DBG_TIME_SIZE) ? 0 : index;
	composer_priv.health_info.time_index[type] = index;
	composer_priv.health_info.count[type]++;
}

unsigned int composer_dump(char *buf)
{
	u32 fps0, fps1, fps2;
	u32 cnt0, cnt1, cnt2;

	fps0 = composer_get_frame_fps(0);
	fps1 = composer_get_frame_fps(1);
	fps2 = composer_get_frame_fps(2);
	cnt0 = composer_priv.health_info.count[0];
	cnt1 = composer_priv.health_info.count[1];
	cnt2 = composer_priv.health_info.count[2];

	return sprintf(buf, "acquire: %d, %d.%d fps\nrelease: %d, %d.%d fps\ndisplay: %d, %d.%d fps\n",
		cnt0, fps0/10, fps0%10, cnt1, fps1/10, fps1%10, cnt2, fps2/10, fps2%10);
}

/* define the data cache of frame */
#define DATA_ALLOC 1
#define DATA_FREE 0
static int cache_num = 8;
static struct mutex cache_opr;
typedef struct data_cache{
	dispc_data_list_t *addr;
	int flag;
	struct data_cache *next;
} data_cache_t;
static data_cache_t *frame_data;
/* destroy data cache of frame */
static int mem_cache_destroy(void)
{
	data_cache_t *cur = frame_data;
	data_cache_t *next = NULL;
	while (cur != NULL) {
		if (cur->addr != NULL) {
			kfree(cur->addr);
		}
		next = cur->next;
		kfree(cur);
		cur = next;
	}
	mutex_destroy(&cache_opr);
	return 0;
}
/* create data cache of frame */
static int mem_cache_create(void)
{
	int i = 0;
	data_cache_t *cur = NULL;
	mutex_init(&cache_opr);
	frame_data = kzalloc(sizeof(data_cache_t), GFP_ATOMIC);
	if (frame_data == NULL) {
		printk("alloc frame data[0] fail\n");
		return -1;
	}
	frame_data->addr = kzalloc(sizeof(dispc_data_list_t), GFP_ATOMIC);
	if (frame_data->addr == NULL) {
		printk("alloc dispc data[0] fail\n");
		mem_cache_destroy();
		return -1;
	}
	frame_data->flag = DATA_FREE;
	cur = frame_data;
	for (i = 1; i < cache_num ; i++) {
		cur->next = kzalloc(sizeof(data_cache_t), GFP_ATOMIC);
		if (cur->next == NULL) {
			printk("alloc frame data[%d] fail\n", i);
			mem_cache_destroy();
			return -1;
		}
		cur->next->addr = kzalloc(sizeof(dispc_data_list_t), GFP_ATOMIC);
		if (cur->next->addr == NULL) {
			printk("alloc dispc data[%d] fail\n", i);
			mem_cache_destroy();
			return -1;
		}
		cur->next->flag = DATA_FREE;
		cur = cur->next;
	}
	return 0;
}
/* free data of a frame from cache*/
static int mem_cache_free(dispc_data_list_t *addr)
{
	int i = 0;
	data_cache_t *cur = NULL;
	mutex_lock(&cache_opr);
	cur = frame_data;
	for (i = 0; (cur != NULL) && (i < cache_num); i++) {
		if (addr != NULL && cur->addr == addr) {
			cur->flag = DATA_FREE;
			mutex_unlock(&cache_opr);
			return 0;
		}
		cur = cur->next;
	}
	mutex_unlock(&cache_opr);
	return -1;
}
/* alloc data of a frame from cache */
static dispc_data_list_t *mem_cache_alloc(void)
{
	int i = 0;
	data_cache_t *cur = NULL;
	mutex_lock(&cache_opr);
	cur = frame_data;
	for (i = 0; i < cache_num; i++) {
		if (cur != NULL && cur->flag == DATA_FREE) {
			if (cur->addr != NULL) {
				memset(cur->addr, 0, sizeof(dispc_data_list_t));
			}
			cur->flag = DATA_ALLOC;
			mutex_unlock(&cache_opr);
			return cur->addr;
		} else if (cur == NULL) {
			printk("alloc frame data fail, can not find avail buffer.\n");
			mutex_unlock(&cache_opr);
			return NULL;
		}
		if (i < cache_num - 1) {
			cur = cur->next;
		}
	}
	printk("All frame data are used, try adding new...\n");
	cur->next = kzalloc(sizeof(data_cache_t), GFP_ATOMIC);
	if (cur->next == NULL) {
		printk("alloc a frame data fail\n");
		mutex_unlock(&cache_opr);
		return NULL;
	}
	cur->next->addr = kzalloc(sizeof(dispc_data_list_t), GFP_ATOMIC);
	if (cur->next->addr == NULL) {
		printk("alloc a dispc data fail\n");
		kfree(cur->next);
		cur->next = NULL;
		mutex_unlock(&cache_opr);
		return NULL;
	}
	cur->next->flag = DATA_ALLOC;
	cache_num++;
	printk("create a new frame data success, cache num update to %d", cache_num);
	mutex_unlock(&cache_opr);
	return cur->next->addr;
}

static void hwc_commit_work(struct work_struct *work)
{
    dispc_data_list_t *data, *next;
    disp_frame_t *frame;
    struct list_head saved_list;
    int err;
	int i, disp;
	struct sync_fence **acquire_fence;

    mutex_lock(&(gcommit_mutek));
    if (composer_priv.b_no_output)
	    goto _out;

    mutex_lock(&(composer_priv.update_regs_list_lock));
    list_replace_init(&composer_priv.update_regs_list, &saved_list);
    mutex_unlock(&(composer_priv.update_regs_list_lock));

	list_for_each_entry_safe(data, next, &saved_list, list) {
		list_del(&data->list);
		for (disp = 0; disp < DISP_NUMS_SCREEN; disp++) {
			frame = &data->frame[disp];
			acquire_fence = (struct sync_fence **)(&frame->acquire_fence[0]);
			for (i = 0; i < frame->hwc_data.layer_num; i++, acquire_fence++) {
				if (acquire_fence != NULL && *acquire_fence != NULL) {
					err = sync_fence_wait(*acquire_fence, 1000);
					sync_fence_put(*acquire_fence);
					if (err < 0) {
						printk("synce_fence_wait timeout AcquireFence:%p\n", *acquire_fence);
						sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
						goto free;
					}
				}
			}
		}

		if (composer_priv.pause == 0)
			dispc_gralloc_queue(data, data->framenumber);

free:
		mem_cache_free(data);
	}

_out:
	mutex_unlock(&(gcommit_mutek));
}

static void correct_layer_index(struct disp_layer_config *l, int lcnt)
{
	unsigned int mask = 0x0000ffff;
	int index;
	int empty_layer_idx;
	int t;

	for (index = 0; index < lcnt; index++) {
		t = (l[index].channel << 2) + (l[index].layer_id);
		mask &= (~(1 << t));
	}

	empty_layer_idx = lcnt;
	for (index = 0; index < 16; index++) {
		if ((mask & (1 << index)) == 0)
			continue;

		l[empty_layer_idx].channel  = (index >> 2) & 0x03;
		l[empty_layer_idx].layer_id = (index >> 0) & 0x03;
		empty_layer_idx++;
	}
}

static void imp_finish_cb(bool force_all);

static int hwc_commit(setup_dispc_data_t *disp_data)
{
	int disp, i;
	dispc_data_list_t *disp_data_list;
	setup_dispc_data_t *setup;
	disp_frame_t *frame;
	struct disp_layer_config *phwclayer;
	struct sync_fence **fence;
	int release_fencefd = -1;
	int acquire_fencefd[16];
	struct sync_fence *release_fence;
	struct sync_pt *pt;
	int needskip = 0;
	struct disp_device *dispdev;

	disp_data_list = mem_cache_alloc();
	if (!disp_data_list) {
		printk(KERN_ERR "%s: %d, alloc data for disp_data_list fail.\n", __func__, __LINE__);
		release_fencefd = -1;
		goto __out;
	}

	for (disp = 0; disp < DISP_NUMS_SCREEN; disp++) {
		setup = &disp_data[disp];
		frame = &disp_data_list->frame[disp];

		if (!setup->layer_num)
			continue;

		/*
		 * Released the last layer buffer when all devices is close,
		 * or it will lead to fence timeout issue.
		 */
		dispdev = composer_priv.psg_disp_drv->mgr[disp]->device;
		if (dispdev && dispdev->is_enabled(dispdev) == 0) {
			needskip++;
			continue;
		} else {
			needskip--;
		}

		/* backup display config from userspace */
		memcpy(&frame->hwc_data, setup, sizeof(*setup));
		fence = &frame->acquire_fence[0];
		phwclayer = &frame->hwclayer[0];
		frame->hwc_data.hwclayer = phwclayer;

		if (copy_from_user(acquire_fencefd, (void __user *)setup->fencefd, sizeof(int) * setup->layer_num)) {
			printk(KERN_ERR "%s,%d: copy_from_user fail, %p\n", __func__, __LINE__, setup->fencefd);
			mem_cache_free(disp_data_list);
			goto __out;
		}

		if (copy_from_user(phwclayer, (void __user *)setup->hwclayer, sizeof(struct disp_layer_config) * setup->layer_num)) {
			printk(KERN_ERR "%s,%d: copy_from_user fail, %p\n", __func__, __LINE__, setup->hwclayer);
			mem_cache_free(disp_data_list);
			goto __out;
		}
		correct_layer_index(phwclayer, setup->layer_num);

		/* get fence object by acquire fencefd */
		for (i = 0; i < setup->layer_num; i++) {
			if (acquire_fencefd[i] != -1)
				fence[i] = sync_fence_fdget(acquire_fencefd[i]);
		}
	}

	if (needskip > 0) {
		composer_priv.Cur_Write_Cnt = composer_priv.timeline_max;

		mutex_lock(&composer_priv.runtime_lock);
		imp_finish_cb(1);
		mutex_unlock(&composer_priv.runtime_lock);
		mem_cache_free(disp_data_list);
		goto __out;
	}

	if (!composer_priv.b_no_output) {
		release_fencefd = get_unused_fd();
		if (release_fencefd < 0) {
			mem_cache_free(disp_data_list);
			release_fencefd = -1;
			goto __out;
		}

		composer_priv.timeline_max++;
		pt = sw_sync_pt_create(composer_priv.relseastimeline, composer_priv.timeline_max);
		release_fence = sync_fence_create("sunxi_display", pt);
		sync_fence_install(release_fence, release_fencefd);

		disp_data_list->framenumber = composer_priv.timeline_max;
		mutex_lock(&(composer_priv.update_regs_list_lock));
		list_add_tail(&disp_data_list->list, &composer_priv.update_regs_list);
		mutex_unlock(&(composer_priv.update_regs_list_lock));

		if (!composer_priv.pause) {
			queue_work(composer_priv.Display_commit_work, &composer_priv.commit_work);
		}
	} else {
		mem_cache_free(disp_data_list);
		flush_workqueue(composer_priv.Display_commit_work);
	}

__out:
	if (copy_to_user((void __user *)(disp_data->fencefd),
			&release_fencefd, sizeof(int))) {
		printk(KERN_ERR "copy_to_user fail\n");
		return  -EFAULT;
	}
	return 0;
}

static int hwc_commit_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	setup_dispc_data_t tmp[DISP_NUMS_SCREEN];
	unsigned long *ubuffer = (unsigned long *)arg;
	int total = ubuffer[2];

	if (DISP_HWC_COMMIT == cmd) {

		if (HWCOMPOSER2_START == ubuffer[1]) {
			if (0x2 == ubuffer[2]) {
				/*START_CLIENT_HWC_2*/
				const int client = 0x2;
				disp_drv_info *drv_info = composer_priv.psg_disp_drv;
				composer_stop(NULL);
				return composer2_start_extern(drv_info, client);
			} else if (0x1 == ubuffer[2]) {
				/*START_CLIENT_HWC_1_X*/
				const int client = 0x1;
				disp_drv_info *drv_info = composer_priv.psg_disp_drv;
				composer_stop(NULL);
				return composer2_start_extern(drv_info, client);
			} else {
				printk("start composer2 by client(%ld) failed\n", ubuffer[2]);
				return -1;
			}
		}

		if (total > DISP_NUMS_SCREEN) {
			printk("%s: support only %d screen\n", __func__, DISP_NUMS_SCREEN);
			total = DISP_NUMS_SCREEN;
		}

		memset(tmp, 0, sizeof(setup_dispc_data_t) * DISP_NUMS_SCREEN);
		if (copy_from_user(tmp, (void __user *)ubuffer[1], sizeof(setup_dispc_data_t) * DISP_NUMS_SCREEN)) {
			printk("%s,%d: copy_from_user fail\n", __func__, __LINE__);
			return  -EFAULT;
		}
		ret = hwc_commit(tmp);
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static int hwc_compat_commit_ioctl(unsigned int cmd, unsigned long arg)
{
	int i;
	int ret = -1;
	compat_setup_dispc_data_t compat_setup_data[DISP_NUMS_SCREEN];
	setup_dispc_data_t tmp[DISP_NUMS_SCREEN];
	unsigned long *ubuffer = (unsigned long *)arg;
	int total = ubuffer[2];

	if (DISP_HWC_COMMIT == cmd) {

		if (HWCOMPOSER2_START == ubuffer[1]) {
			if (0x2 == ubuffer[2]) {
				/*START_CLIENT_HWC_2*/
				const int client = 0x2;
				disp_drv_info *drv_info = composer_priv.psg_disp_drv;
				composer_stop(NULL);
				return composer2_start_extern(drv_info, client);
			} else if (0x1 == ubuffer[2]) {
				/*START_CLIENT_HWC_1_X*/
				const int client = 0x1;
				disp_drv_info *drv_info = composer_priv.psg_disp_drv;
				composer_stop(NULL);
				return composer2_start_extern(drv_info, client);
			} else {
				printk("start composer2 by client(%ld) failed\n", ubuffer[2]);
				return -1;
			}
		}

		if (total > DISP_NUMS_SCREEN) {
			printk("%s: support only %d screen\n", __func__, DISP_NUMS_SCREEN);
			total = DISP_NUMS_SCREEN;
		}

		memset(tmp, 0, sizeof(setup_dispc_data_t) * DISP_NUMS_SCREEN);
		memset(compat_setup_data, 0, sizeof(compat_setup_dispc_data_t) * DISP_NUMS_SCREEN);
		if (copy_from_user(compat_setup_data, (void __user *)ubuffer[1], sizeof(compat_setup_dispc_data_t) * DISP_NUMS_SCREEN)) {
			printk("%s,%d: copy_from_user fail\n", __func__, __LINE__);
			return  -EFAULT;
		}

		for (i = 0; i < DISP_NUMS_SCREEN; i++) {
			tmp[i].deviceid       = compat_setup_data[i].deviceid;
			tmp[i].layer_num      = compat_setup_data[i].layer_num;
			tmp[i].need_writeback = compat_setup_data[i].need_writeback;
			tmp[i].ehancemode     = compat_setup_data[i].ehancemode;
			tmp[i].androidfrmnum  = compat_setup_data[i].androidfrmnum;
			tmp[i].hwclayer       = (void *)(unsigned long)compat_setup_data[i].hwclayer;
			tmp[i].fencefd        = (void *)(unsigned long)compat_setup_data[i].fencefd;
		}
		ret = hwc_commit(tmp);
	}
	return ret;
}
#endif

static int hwc_custom_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	if (DISP_HWC_CUSTOM == cmd) {
		int disp;
		int flag;
		unsigned long *ubuffer = (unsigned long *)arg;
		struct disp_manager *disp_mgr = NULL;

		disp = ubuffer[0];
		flag = ubuffer[1];
		disp = ubuffer[0];
		if (disp < 0 || disp > DISP_NUMS_SCREEN) {
			printk(KERN_ERR "%s: invalid input params\n", __func__);
			return -EINVAL;
		}

		disp_mgr = composer_priv.psg_disp_drv->mgr[disp];
		if (!disp_mgr)
			return -EINVAL;

		switch (flag) {
		case HWC_CMD_GET_FPS:
			if (disp_mgr->device && disp_mgr->device->get_fps)
				ret = disp_mgr->device->get_fps(disp_mgr->device);
			break;
		case HWC_CMD_GET_CLK_RATE:
			if (disp_mgr->get_clk_rate)
				ret = disp_mgr->get_clk_rate(disp_mgr);
			break;
		default:
			printk(KERN_ERR "%s: unknown ioctl\n", __func__);
			break;
		}
	}

	return ret;
}



/*static void disp_composer_proc(u32 sel)
{
	if (0 == s_start)
		return;
    if (sel < 2) {
	    if (composer_priv.Cur_Write_Cnt < composer_priv.Cur_Disp_Cnt[sel]) {
		    composer_priv.countrotate[sel] = 1;
	    }
	    composer_priv.Cur_Disp_Cnt[sel] = composer_priv.Cur_Write_Cnt;
    }
    schedule_work(&composer_priv.post2_cb_work);
    return ;
}
*/
static void imp_finish_cb(bool force_all)
{
    u32 little = 1;
	u32 flag = 0;

    if (composer_priv.pause) {
	    return;
    }
    if (composer_priv.display_active[0]) {
	    little = composer_priv.Cur_Disp_Cnt[0];
    }
    if (composer_priv.display_active[0] && composer_priv.countrotate[0]
		    && composer_priv.display_active[1] && composer_priv.countrotate[1]) {
	    composer_priv.countrotate[0] = 0;
	    composer_priv.countrotate[1] = 0;
    }
    if (composer_priv.display_active[1]) {
	    if (composer_priv.display_active[0]) {
		    if (composer_priv.countrotate[0] != composer_priv.countrotate[1]) {
			    if (composer_priv.countrotate[0] && composer_priv.display_active[0]) {
				    little = composer_priv.Cur_Disp_Cnt[1];
			    } else{
				    little = composer_priv.Cur_Disp_Cnt[0];
			    }
		    } else{
			    if (composer_priv.Cur_Disp_Cnt[1] > composer_priv.Cur_Disp_Cnt[0]) {
				    little = composer_priv.Cur_Disp_Cnt[0];
			    } else{
				    little = composer_priv.Cur_Disp_Cnt[1];
			    }
		    }
	    } else{
		    little = composer_priv.Cur_Disp_Cnt[1];
	    }
    }
    while (composer_priv.relseastimeline->value != composer_priv.Cur_Write_Cnt) {
	    if (!force_all && (composer_priv.relseastimeline->value >= little - 1)) {
		    break;
	    }
	    sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
	    composer_frame_checkin(1);/* release */
	    flag = 1;
    }
    if (flag)
	    composer_frame_checkin(2);/* display */
}

static void post2_cb(struct work_struct *work)
{
	mutex_lock(&composer_priv.runtime_lock);
	imp_finish_cb(composer_priv.b_no_output);
	mutex_unlock(&composer_priv.runtime_lock);
}
extern s32  bsp_disp_shadow_protect(u32 disp, bool protect);
int dispc_gralloc_queue(dispc_data_list_t *dispc_data, unsigned int framenuber)
{
    int disp;
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *psmgr = NULL;
    struct disp_enhance *psenhance = NULL;
    struct disp_layer_config *psconfig;
    disp_frame_t *frame;
    disp = 0;
    while (disp < DISP_NUMS_SCREEN) {
	    frame = &dispc_data->frame[disp];
	    if (!frame->hwc_data.layer_num) {
		    composer_priv.display_active[disp] = 0;
		    disp++;
		    continue;
	    }
	    psmgr = psg_disp_drv->mgr[disp];
	    if (psmgr != NULL) {
		    psenhance = psmgr->enhance;
		    bsp_disp_shadow_protect(disp, true);
		    if (frame->hwc_data.ehancemode) {
			    if (frame->hwc_data.ehancemode != composer_priv.ehancemode[disp]) {
				    switch (frame->hwc_data.ehancemode) {
				    case 1:
					    psenhance->demo_disable(psenhance);
					    psenhance->enable(psenhance);
					    break;
				    case 2:
					    psenhance->enable(psenhance);
					    psenhance->demo_enable(psenhance);
					    break;
				    default:
					    psenhance->disable(psenhance);
					    printk("translat a err info\n");
				    }
			    }
			    composer_priv.ehancemode[disp] = frame->hwc_data.ehancemode;
		    } else {
			    if (composer_priv.ehancemode[disp]) {
				    psenhance->disable(psenhance);
				    composer_priv.ehancemode[disp] = 0;
			    }
		    }
		    psconfig = frame->hwc_data.hwclayer;
		    psmgr->set_layer_config(psmgr, psconfig, disp ? 8 : 16);
		    bsp_disp_shadow_protect(disp, false);
		    if (composer_priv.display_active[disp] == 0) {
			    composer_priv.display_active[disp] = 1;
			    composer_priv.Cur_Disp_Cnt[disp] = framenuber;
		    }
	    }
	    disp++;
    }
    composer_priv.Cur_Write_Cnt = framenuber;
    composer_frame_checkin(0);/* acquire */
    if (composer_priv.b_no_output) {
	    mutex_lock(&composer_priv.runtime_lock);
	    imp_finish_cb(1);
	    mutex_unlock(&composer_priv.runtime_lock);
    }
    return 0;
}

extern int composer2_shutdown(void);
/*
 * hwc_suspend_prepare:
 * 1. skip all buffers when shutdown.
 * 2. close existed layer and clear de composition.
 */
#define HWC_SUSPEND_PREPARE
#ifdef HWC_SUSPEND_PREPARE
void hwc_suspend_prepare(void)
{
	int disp;
	dispc_data_list_t *disp_data = NULL;
	struct disp_manager *disp_mgr;

	if (s_stop) {
		composer2_shutdown();
		return;
	}

	composer_priv.b_no_output = 1;
	disp_data = mem_cache_alloc();
	if (!disp_data)
		return;

	memset(disp_data, 0, sizeof(*disp_data));
	correct_layer_index(&disp_data->frame[0].hwclayer[0], 0);

	for (disp = 0; disp < DISP_NUMS_SCREEN; disp++) {
		disp_mgr = composer_priv.psg_disp_drv->mgr[disp];
	}
}
#else
void hwc_suspend_prepare(void) { return; }
#endif

static int hwc_suspend(void)
{
	composer_priv.b_no_output = 1;
	mutex_lock(&composer_priv.runtime_lock);
	printk("%s after lock\n", __func__);
	imp_finish_cb(1);
	mutex_unlock(&composer_priv.runtime_lock);
	printk("%s release lock\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	printk("%s\n", __func__);
	return 0;
}

s32 composer_stop(disp_drv_info *psg_disp_drv)
{
	if (s_stop) {
		return 0;
	}
	s_stop = 1;
	mem_cache_destroy();
	if (NULL != composer_priv.tmptransfer[0].hwclayer)
		kfree(composer_priv.tmptransfer[0].hwclayer);
	if (NULL != composer_priv.tmptransfer[1].hwclayer)
		kfree(composer_priv.tmptransfer[1].hwclayer);

	/* disp_unregister_sync_finish_proc(disp_composer_proc); */
	disp_unregister_standby_func(hwc_suspend, hwc_resume);
#ifdef CONFIG_COMPAT
	disp_unregister_compat_ioctl_func(DISP_HWC_COMMIT);
	disp_unregister_compat_ioctl_func(DISP_HWC_CUSTOM);

#endif
	disp_unregister_ioctl_func(DISP_HWC_COMMIT);
	/* disp_unregister_ioctl_func(DISP_HWC_CUSTOM); */

	/*
	* spin_lock_init(&(composer_priv.update_reg_lock));
	* spin_lock_init(&(composer_priv.dumplyr_lock));
	* mutex_init(&composer_priv.update_regs_list_lock);
	* mutex_init(&gcommit_mutek);
	*/

	sync_timeline_destroy(&composer_priv.relseastimeline->obj);

	/*
	*list_del(&composer_priv.update_regs_list);
	*list_del(&composer_priv.dumplyr_list);
	*INIT_WORK(&composer_priv.commit_work, hwc_commit_work);
	*/
	destroy_workqueue(composer_priv.Display_commit_work);
	/*
	* mutex_init(&composer_priv.runtime_lock);
	* INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
	*/
	/* memset(&composer_priv, 0x0, sizeof(struct composer_private_data)); */
	printk("###finished composer_stop\n");
	return 0;
}

s32 composer_init(disp_drv_info *psg_disp_drv)
{
	int ret = 0;
	/* fixme: make s_start true if hwc client want to use dev_composer1*/
	s_start = 0;
	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
	mutex_init(&composer_priv.runtime_lock);
	composer_priv.Display_commit_work = create_freezable_workqueue("SunxiDisCommit");
	INIT_WORK(&composer_priv.commit_work, hwc_commit_work);
	INIT_LIST_HEAD(&composer_priv.update_regs_list);
	INIT_LIST_HEAD(&composer_priv.dumplyr_list);
	composer_priv.relseastimeline = sw_sync_timeline_create("sunxi-display");
	composer_priv.timeline_max = 0;
	composer_priv.b_no_output = 0;
	composer_priv.Cur_Write_Cnt = 0;
	composer_priv.display_active[0] = 0;
	composer_priv.display_active[1] = 0;
	composer_priv.Cur_Disp_Cnt[0] = 0;
	composer_priv.Cur_Disp_Cnt[1] = 0;
	composer_priv.countrotate[0] = 0;
	composer_priv.countrotate[1] = 0;
	composer_priv.pause = 0;
	composer_priv.listcnt = 0;
	mutex_init(&composer_priv.update_regs_list_lock);
	mutex_init(&gcommit_mutek);
	spin_lock_init(&(composer_priv.update_reg_lock));
	spin_lock_init(&(composer_priv.dumplyr_lock));
	disp_register_ioctl_func(DISP_HWC_COMMIT, hwc_commit_ioctl);
	disp_register_ioctl_func(DISP_HWC_CUSTOM, hwc_custom_ioctl);
#ifdef CONFIG_COMPAT
	disp_register_compat_ioctl_func(DISP_HWC_COMMIT, hwc_compat_commit_ioctl);
#endif
	/* disp_register_sync_finish_proc(disp_composer_proc); */
	disp_register_standby_func(hwc_suspend, hwc_resume);
	composer_priv.tmptransfer[0].hwclayer = kzalloc(sizeof(struct disp_layer_config) * 16, GFP_ATOMIC);
	composer_priv.tmptransfer[1].hwclayer = kzalloc(sizeof(struct disp_layer_config) * 16, GFP_ATOMIC);
	composer_priv.psg_disp_drv = psg_disp_drv;
    /* alloc mem_des struct pool, 48k */
	ret = mem_cache_create();
	if (ret != 0)
		printk("%s(%d) alloc frame buffer err!\n", __func__, __LINE__);
	return 0;
}

#endif
