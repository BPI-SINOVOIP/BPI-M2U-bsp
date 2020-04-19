#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>

#include "dev_disp.h"
#include <video/sunxi_display2.h>
#include "dev_composer2.h"

#ifndef DISP_SCREEN_NUM
#define DISP_SCREEN_NUM 2
#endif

typedef struct disp_layer_config2 layer_config_priv;

typedef struct hwc_sync_timeline {
	unsigned int commit_frame_num;
	unsigned int posted_frame_num;
	struct sw_sync_timeline *release_timeline;
	struct mutex commit_mlock;
	unsigned char sync_offset;
	unsigned char disp;
	unsigned char id;
	unsigned char inited;
} sync_timeline_t;

typedef struct composer2_private {
	sync_timeline_t *timeline[HWC_SYNC_TIMELINE_NUM];
	struct work_struct posted_work;
	struct mutex posted_mlock;
	unsigned char power_mode; /* see HWC_K_POWER_MODE_OFF/ON */

#if defined(__aarch64__)
	char padding[128 - 96];
#endif
} composer_private_t;

static composer_private_t s_priv[DISP_SCREEN_NUM];
static disp_drv_info *s_disp_drv;
static int s_client;

static int composer2_ioctl(unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
static int composer2_compat_ioctl(unsigned int cmd, unsigned long arg);
#endif

static int disp_device_is_enable(unsigned int disp)
{
	if ((DISP_SCREEN_NUM > disp) && s_disp_drv) {
		struct disp_manager *disp_mgr = s_disp_drv->mgr[disp];
		if (disp_mgr && disp_mgr->device
			&& disp_mgr->device->is_enabled)
			return disp_mgr->device->is_enabled(disp_mgr->device);
	}
	return 1; /* default is enable */
}

static void update_sync_timeline_locked(sync_timeline_t *tl)
{
	u32 posted_frame_num = tl->posted_frame_num;
	u32 sync_value = tl->release_timeline->value + tl->sync_offset;
	if (sync_value < posted_frame_num)
		sw_sync_timeline_inc(tl->release_timeline,
			posted_frame_num - sync_value);
}

static void update_sync_timelines_locked(composer_private_t *priv)
{
	unsigned int id;
	for (id = 0; id < HWC_SYNC_TIMELINE_NUM; ++id) {
		sync_timeline_t *tl = priv->timeline[id];
		if (0 != tl->inited) {
			update_sync_timeline_locked(tl);
		}
	}
}

static void sync_posted_work0(struct work_struct *work)
{
	composer_private_t *priv = &s_priv[0];
	mutex_lock(&priv->posted_mlock);
	update_sync_timelines_locked(priv);
	mutex_unlock(&priv->posted_mlock);
}

static void sync_posted_work1(struct work_struct *work)
{
	composer_private_t *priv = &s_priv[1];
	mutex_lock(&priv->posted_mlock);
	update_sync_timelines_locked(priv);
	mutex_unlock(&priv->posted_mlock);
}

/*
* This will be called upon de sync finishing in irq,
* and make sure no long-time-opr in this function.
*/
static void sync_posted_hook(unsigned int disp)
{
	if (DISP_SCREEN_NUM > disp) {
		composer_private_t *priv = &(s_priv[disp]);
		for (disp = 0; disp < HWC_SYNC_TIMELINE_NUM; ++disp) {
			sync_timeline_t *tl = priv->timeline[disp];
			/* could ignore whether inited. fixme: should use spin lock ? */
			tl->posted_frame_num = tl->commit_frame_num;
		}
		/* fixme: need more priority ? */
		schedule_work(&(priv->posted_work));
	}
}

static int init_sync_timeline(sync_timeline_t *tl)
{
	char name[16];

	if (0 != tl->inited)
		return 0;

	snprintf(name, sizeof(name) / sizeof(name[0]),
		"DispTL%u_%u", tl->disp, tl->id);
	tl->release_timeline = sw_sync_timeline_create(name);
	mutex_init(&tl->commit_mlock);
	tl->inited = 1;
	return 0;
}

static int create_sync_timeline(unsigned int disp,
	unsigned int id)
{
	if ((DISP_SCREEN_NUM > disp)
		&& (HWC_SYNC_TIMELINE_NUM > id)) {
		sync_timeline_t *tl = s_priv[disp].timeline[id];
		return init_sync_timeline(tl);
	} else {
		printk("%s: disp=%d, id=%d\n", __func__, disp, id);
		return -1;
	}
}

static int destroy_sync_timeline(unsigned int disp, unsigned int id)
{
	if ((DISP_SCREEN_NUM > disp)
		&& (HWC_SYNC_TIMELINE_NUM > id)) {
		return 0;
	} else {
		printk("%s: disp=%d, id=%d\n", __func__, disp, id);
		return -1;
	}
}

static int sync_timeline_set_offset(unsigned int disp,
	unsigned int id, unsigned char offset)
{
	if ((DISP_SCREEN_NUM > disp)
		&& (HWC_SYNC_TIMELINE_NUM > id)) {
		sync_timeline_t *tl = s_priv[disp].timeline[id];
		tl->sync_offset = offset;
		return 0;
	} else {
		printk("%s: disp=%d, id=%d\n", __func__, disp, id);
		return -1;
	}
}

static int sync_timeline_commit_frame_count_inc(
	unsigned int disp, unsigned int id, unsigned int count)
{
	int release_fencefd = -1;
	int ret = -1;

	if ((DISP_SCREEN_NUM > disp) && (HWC_SYNC_TIMELINE_NUM > id)) {
		composer_private_t *priv = &(s_priv[disp]);
		sync_timeline_t *tl = priv->timeline[id];

		if (0 != tl->inited) {
			char fence_name[32];
			struct sync_pt *pt;
			struct sync_fence *release_fence;

			release_fencefd = get_unused_fd();
			if (0 > release_fencefd)
				return release_fencefd;

			tl->commit_frame_num += count; /* fixme: maybe should add lock */
			pt = sw_sync_pt_create(tl->release_timeline, tl->commit_frame_num);
			if (pt == NULL) {
				ret = -ENOMEM;
				goto err;
			}

			snprintf(fence_name, sizeof(fence_name) / sizeof(fence_name[0]),
					"TL%u_%u_Frame%x", disp, id, tl->commit_frame_num);
			release_fence = sync_fence_create(fence_name, pt);
			if (release_fence == NULL) {
				sync_pt_free(pt);
				ret = -ENOMEM;
				goto err;
			}
			sync_fence_install(release_fence, release_fencefd);

			if (!disp_device_is_enable(disp)) {
				printk("disp%d: commit_frame on device disabled\n", disp);
				tl->posted_frame_num = tl->commit_frame_num;
				mutex_lock(&(priv->posted_mlock));
				update_sync_timeline_locked(tl);
				mutex_unlock(&(priv->posted_mlock));
			}
		} else {
			printk("sync timeline(%d) is not inited."
				" commit frame count inc failed\n", tl->id);
		}

	} else {
		printk("[BAD_PARAS]sync_timeline_commit_frame_count_inc: "
			"disp=%d, id=%d\n", disp, id);
	}

	return release_fencefd;

err:
	put_unused_fd(release_fencefd);
	return ret;
}

static int set_power_mode(unsigned int disp, unsigned int mode)
{
	composer_private_t *priv = NULL;

	if (HWC_K_POWER_MODE_OFF != mode)
		return 0;
	if (DISP_SCREEN_NUM <= disp)
		return -1;
	priv = &(s_priv[disp]);

	mutex_lock(&priv->posted_mlock);
	update_sync_timelines_locked(priv);
	mutex_unlock(&priv->posted_mlock);

	return 0;
}

static int get_device_fps(unsigned int disp)
{
	if (DISP_SCREEN_NUM <= disp) {
		printk("%s: disp=%d\n", __func__, disp);
		return -1;
	}

	if (s_disp_drv) {
		struct disp_manager *disp_mgr = s_disp_drv->mgr[disp];
		if (disp_mgr && disp_mgr->device && disp_mgr->device->get_fps)
			return disp_mgr->device->get_fps(disp_mgr->device);
	}
	return -1;
}

static int get_de_clk_rate(unsigned int disp)
{
	if (DISP_SCREEN_NUM <= disp) {
		printk("%s: disp=%d\n", __func__, disp);
		return -1;
	}

	if (s_disp_drv) {
		struct disp_manager *disp_mgr = s_disp_drv->mgr[disp];
		if (disp_mgr && disp_mgr->get_clk_rate)
			return disp_mgr->get_clk_rate(disp_mgr);
	}
	return -1;
}

static int init_composer2_private(unsigned int disp,
	composer_private_t *priv, disp_drv_info *disp_drv)
{
	int i = 0;
	unsigned int timeline_num;
	sync_timeline_t *tl = NULL;

	if ((NULL == priv) || (NULL == disp_drv)) {
		printk("[ERR]disp%u: composer2_priv=%p, disp_drv=%p\n",
			disp, priv, disp_drv);
		return -1;
	}

	timeline_num = sizeof(priv->timeline) / sizeof(priv->timeline[0]);
	tl = kzalloc(sizeof(*tl) * timeline_num, GFP_ATOMIC);
	if (NULL == tl) {
		printk("kzalloc for %d sync_timeline_t failed !\n", timeline_num);
		return -1;
	}
	for (i = 0; i < timeline_num; ++i) {
		tl->disp = disp;
		tl->id = i;
		priv->timeline[i] = tl;
		tl++;
	}
	/* only init default timeline */
	tl = priv->timeline[0];
	init_sync_timeline(tl);

	switch (disp) {
	case 0:
		INIT_WORK(&(priv->posted_work), sync_posted_work0);
		break;
	case 1:
		INIT_WORK(&(priv->posted_work), sync_posted_work1);
		break;
	default:
		printk("[ERR]%s, not support for disp%u\n", __func__, disp);
		return -1;
	}
	mutex_init(&priv->posted_mlock);

	priv->power_mode = HWC_K_POWER_MODE_OFF;
	return 0;
}

static inline unsigned char valid_client(int client)
{
	return (START_CLIENT_HWC_2 == client)
		|| (START_CLIENT_HWC_1_X == client);
}

/* only support one thread to call this function */
static int composer2_start(disp_drv_info *p_disp_drv, int client)
{
	unsigned int disp;
	printk("composer2 start client(%d)\n", client);

	if (!valid_client(client)) {
		printk("invalid client(%d)\n", client);
		return -1;
	}
	if (client == s_client) {
		printk("already start same client(%d)\n", client);
		return 0;
	}
	if (START_CLIENT_HWC_INVALID != s_client) {
		printk("already has a client(%d)\n", s_client);
		return -1;
	}
	s_client = client;

	for (disp = 0; disp < sizeof(s_priv) / sizeof(s_priv[0]); ++disp)
		if (init_composer2_private(disp, &(s_priv[disp]), p_disp_drv)) {
			s_client = START_CLIENT_HWC_INVALID;
			return -1;
		}

	/* fixme: register ioctl func by calling composer2_init */
	if (NULL == s_disp_drv) {
		disp_register_ioctl_func(DISP_HWC_COMMIT, composer2_ioctl);
#ifdef CONFIG_COMPAT
		disp_register_compat_ioctl_func(DISP_HWC_COMMIT, composer2_compat_ioctl);
#endif
	}

	disp_register_sync_finish_proc(sync_posted_hook);

#if defined(__aarch64__)
	printk("sizeof:(s_priv)=%lu, (s_priv[0])=%lu\n",
		sizeof(s_priv), sizeof(s_priv[0]));
#endif

	return 0;
}

static int composer2_stop(disp_drv_info *p_disp_drv)
{
	/* todo: release all resources */
	printk("todo: stop composer2 of client(%d)\n", s_client);
	return 0;
}

static int composer2_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	if (DISP_HWC_COMMIT == cmd) {
		unsigned long *ubuffer = (unsigned long *)arg;
		switch (ubuffer[1]) {
		case HWC_START:
			if (NULL != s_disp_drv)
				ret = composer2_start(s_disp_drv, (int)ubuffer[2]);
			break;
		case HWC_STOP:
			if (NULL != s_disp_drv)
				ret = composer2_stop(s_disp_drv);
			break;
		case HWC_COMMIT_FRAME_NUM_INC:
			ret = sync_timeline_commit_frame_count_inc(
				(unsigned int)ubuffer[0], (unsigned int)ubuffer[2],
				(unsigned int)ubuffer[3]);
			break;
		case HWC_SYNC_TIMELINE_CREATE:
			ret = create_sync_timeline((unsigned int)ubuffer[0],
				(unsigned int)ubuffer[2]);
			break;
		case HWC_SYNC_TIMELINE_DESTROY:
			ret = destroy_sync_timeline((unsigned int)ubuffer[0],
				(unsigned int)ubuffer[2]);
			break;
		case HWC_SYNC_TIMELINE_SET_OFFSET:
			ret = sync_timeline_set_offset((unsigned int)ubuffer[0],
				(unsigned int)ubuffer[2], (unsigned char)ubuffer[3]);
			break;
		case HWC_SET_POWER_MODE:
			ret = set_power_mode((unsigned int)ubuffer[0],
				(unsigned int)ubuffer[2]);
			break;
		case HWC_GET_FPS:
			ret = get_device_fps((unsigned int)ubuffer[0]);
			break;
		case HWC_GET_CLK_RATE:
			ret = get_de_clk_rate((unsigned int)ubuffer[0]);
			break;
		default:
			printk("unknow cmd(%lx) of DISP_HWC_COMMIT\n", ubuffer[1]);
		}
	} else {
		printk("ERR: wrong cmd(%x) for composer2_ioctl\n", cmd);
	}
	return ret;
}

#ifdef CONFIG_COMPAT
/* Notice: args was compated by disp_compat_ioctl. */
static int composer2_compat_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	unsigned long *ubuffer = (unsigned long *)arg;

	switch (ubuffer[1]) {
	case HWC_COMMIT_FRAME_NUM_INC:
	case HWC_SYNC_TIMELINE_CREATE:
	case HWC_SYNC_TIMELINE_DESTROY:
	case HWC_SYNC_TIMELINE_SET_OFFSET:
	case HWC_SET_POWER_MODE:
	case HWC_GET_FPS:
	case HWC_GET_CLK_RATE:
	case HWC_START:
	case HWC_STOP:
		ret = composer2_ioctl(cmd, arg);
		break;
	default:
		printk("unknow cmd(%lx) of composer2_compat_ioctl\n", ubuffer[1]);
	}
	return ret;
}
#endif

int composer2_shutdown(void)
{
	if (START_CLIENT_HWC_1_X == s_client) {
		unsigned int i;
		for (i = 0; i < sizeof(s_priv) / sizeof(s_priv[0]); ++i) {
			composer_private_t *priv = &(s_priv[i]);
			unsigned int id;

			mutex_lock(&priv->posted_mlock);
			for (id = 0; id < HWC_SYNC_TIMELINE_NUM; ++id) {
				sync_timeline_t *tl = priv->timeline[id];
				tl->sync_offset = 0;
				tl->commit_frame_num += 3;
				tl->posted_frame_num += 3;
			}
			update_sync_timelines_locked(priv);
			mutex_unlock(&priv->posted_mlock);
		}
	}
	return 0;
}

int composer2_init(disp_drv_info *p_disp_drv)
{
	s_disp_drv = p_disp_drv;
	disp_register_ioctl_func(DISP_HWC_COMMIT, composer2_ioctl);
#ifdef CONFIG_COMPAT
	disp_register_compat_ioctl_func(DISP_HWC_COMMIT, composer2_compat_ioctl);
#endif

	return 0;
}

int composer2_exit(void)
{
	disp_unregister_ioctl_func(DISP_HWC_COMMIT);
#ifdef CONFIG_COMPAT
	disp_unregister_compat_ioctl_func(DISP_HWC_COMMIT);
#endif
	s_disp_drv = NULL;
	return 0;
}

/* this will be extern to be called by dev_composer */
int composer2_start_extern(disp_drv_info *p_disp_drv, int client)
{
	int ret = composer2_start(p_disp_drv, client);
	s_disp_drv = p_disp_drv;
	return ret;
}
