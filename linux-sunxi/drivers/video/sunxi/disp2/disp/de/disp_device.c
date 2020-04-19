#include <linux/kernel.h>
#include "disp_device.h"

static LIST_HEAD(device_list);

s32 disp_device_set_manager(struct disp_device* dispdev, struct disp_manager *mgr)
{
	if ((NULL == dispdev) || (NULL == mgr)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("device %d, mgr %d\n", dispdev->disp, mgr->disp);

	dispdev->manager = mgr;
	mgr->device = dispdev;

	return DIS_SUCCESS;
}

s32 disp_device_unset_manager(struct disp_device* dispdev)
{
	if ((NULL == dispdev)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (dispdev->manager)
		dispdev->manager->device = NULL;
	dispdev->manager = NULL;

	return DIS_SUCCESS;
}

s32 disp_device_get_resolution(struct disp_device* dispdev, u32 *xres, u32 *yres)
{
	if ((NULL == dispdev)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	*xres = dispdev->timings.x_res;
	*yres = dispdev->timings.y_res;

	return 0;
}

s32 disp_device_get_timings(struct disp_device* dispdev, struct disp_video_timings *timings)
{
	if ((NULL == dispdev)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (timings)
		memcpy(timings, &dispdev->timings, sizeof(struct disp_video_timings));

	return 0;
}

s32 disp_device_is_interlace(struct disp_device *dispdev)
{
	if ((NULL == dispdev)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	return dispdev->timings.b_interlace;
}

s32 disp_device_get_status(struct disp_device *dispdev)
{
	if (NULL == dispdev) {
		DE_WRN("NULL hdl!\n");
		return 0;
	}

	return disp_al_device_get_status(dispdev->hwdev_index);
}

bool disp_device_is_in_safe_period(struct disp_device *dispdev)
{
	int cur_line;
	int start_delay;
	bool ret = true;

	if (NULL == dispdev) {
		DE_WRN("NULL hdl!\n");
		goto exit;
	}

	start_delay =
	    disp_al_device_get_start_delay(dispdev->hwdev_index);
	cur_line = disp_al_device_get_cur_line(dispdev->hwdev_index);
	if (cur_line >= start_delay)
		ret = false;

exit:
	return ret;
}

u32 disp_device_usec_before_vblank(struct disp_device *dispdev)
{
	int cur_line;
	int start_delay;
	u32 usec = 0;
	struct disp_video_timings *timings;
	u32 usec_per_line;
	unsigned long long n_temp, base_temp;
	u32 mod;

	if (NULL == dispdev) {
		DE_WRN("NULL hdl!\n");
		goto exit;
	}

	start_delay =
	    disp_al_device_get_start_delay(dispdev->hwdev_index);
	cur_line = disp_al_device_get_cur_line(dispdev->hwdev_index);
	if (cur_line > (start_delay - 4)) {
		timings = &dispdev->timings;
		/*usec_per_line = (u32)((uint64_t)timings->hor_total_time *
					(uint64_t)1000000 /
					(uint64_t)timings->pixel_clk);*/
		n_temp = (unsigned long long)timings->hor_total_time *
				(unsigned long long)(1000000);
		base_temp = (unsigned long long)timings->pixel_clk;
		mod = (u32)do_div(n_temp, base_temp);
		usec_per_line = (u32)n_temp;
		usec = (timings->ver_total_time - cur_line + 1) * usec_per_line;
	}

exit:
	return usec;
}

/* get free device */
struct disp_device* disp_device_get(int disp, enum disp_output_type output_type)
{
	struct disp_device* dispdev = NULL;

	list_for_each_entry(dispdev, &device_list, list) {
		if ((dispdev->type == output_type) && (dispdev->disp == disp)
			&& (NULL == dispdev->manager)) {
			return dispdev;
		}
	}

	return NULL;
}

struct disp_device* disp_device_find(int disp, enum disp_output_type output_type)
{
	struct disp_device* dispdev = NULL;

	list_for_each_entry(dispdev, &device_list, list) {
		if ((dispdev->type == output_type) && (dispdev->disp == disp)) {
			return dispdev;
		}
	}

	return NULL;
}

struct list_head* disp_device_get_list_head(void)
{
	return (&device_list);
}


s32 disp_device_register(struct disp_device *dispdev)
{
	list_add_tail(&dispdev->list, &device_list);
	return 0;
}

s32 disp_device_unregister(struct disp_device *dispdev)
{
	list_del(&dispdev->list);
	return 0;
}

