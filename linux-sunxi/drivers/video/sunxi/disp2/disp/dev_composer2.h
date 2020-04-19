#ifndef __DEV_COMPOSER2_H__
#define __DEV_COMPOSER2_H__

#define MAX_LAYER_NUM 16

enum {
	START_CLIENT_HWC_INVALID = 0,
	START_CLIENT_HWC_1_X,
	START_CLIENT_HWC_2,
};

/* id of timeline */
enum {
	HWC_SYNC_TIMELINE_DEFAULT = 0,
	HWC_SYNC_TIMELINE_EXTEND_1,
	HWC_SYNC_TIMELINE_NUM,
};

enum {
	HWC_K_POWER_MODE_OFF = 0,
	HWC_K_POWER_MODE_ON = 2,
};

enum {
	HWC_K_VSYNC_ENABLE = 1,
	HWC_K_VSYNC_DISABLE = 2,
};

/* cmds of DISP_HWC_COMMIT */
enum {

	HWC_CMD_BASE = 0x00000000,

	/*
	* HWC_START: start the dev_composer2 by client.
	* Only support one client.
	* usage:
	* arg[0] = 0;
	* arg[1] = HWC_START;
	* arg[2] = START_CLIENT_HWC_2 or START_CLIENT_HWC_1_X;
	* it will return 0 if success.
	*/
	HWC_START,
	HWC_STOP,

	/*
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_SET_POWER_MODE;
	* arg[2] = power_mode;
	* HWC_K_POWER_MODE_OFF: commit a blank frame on screen,
	*   then release fences of prev-frame by increasing timeline of sync.
	* HWC_K_POWER_MODE_ON: do nothing.
	*/
	HWC_SET_POWER_MODE,

	/*
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_GET_FPS;
	* it will return fps of device.
	*/
	HWC_GET_FPS,

	/*
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_GET_CLK_RATE;
	* it will return clk rate of de.
	*/
	HWC_GET_CLK_RATE,

	/*
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_VSYNC_ENABLE;
	* arg[2] = HWC_K_VSYNC_xxx;
	*/
	HWC_VSYNC_ENABLE,

	/*
	* HWC_SYNC_TIMELINE_CREATE
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_SYNC_TIMELINE_CREATE;
	* arg[2] = HWC_SYNC_TIMELINE_XXX; id of timeline.
	* it will return 0 if success, or return -1.
	*/
	HWC_SYNC_TIMELINE_CREATE,

	/*
	* HWC_SYNC_TIMELINE_DESTROY
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_SYNC_TIMELINE_DESTROY;
	* arg[2] = (unsigned long)(int timeline_id);
	* return 0 if success, or return -1.
	*/
	HWC_SYNC_TIMELINE_DESTROY,

	/*
	* HWC_SYNC_TIMELINE_SET_OFFSET
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_SYNC_TIMELINE_SET_OFFSET;
	* arg[2] = (unsigned long)(int timeline_id);
	* arg[3] = (unsigned long)(unsigned char sync_offset);
	* return 0 if success, or return -1.
	*/
	HWC_SYNC_TIMELINE_SET_OFFSET,

	/*
	* HWC_COMMIT_FRAME_NUM_INC
	* usage:
	* arg[0] = disp;
	* arg[1] = HWC_COMMIT_FRAME_NUM_INC;
	* arg[2] = (unsigned long)(int timeline_id);
	* arg[3] = (unsigned long)count;
	* return release fence_fd if success, or return -1.
	* fence_fd will not be signal until next vsync time plus sync_offset.
	*/
	HWC_COMMIT_FRAME_NUM_INC,

};

#endif /* #ifndef __DEV_COMPOSER2_H__ */
