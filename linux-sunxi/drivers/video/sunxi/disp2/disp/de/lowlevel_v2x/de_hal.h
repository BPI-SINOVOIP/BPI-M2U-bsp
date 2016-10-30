#ifndef __DE_HAL_H__
#define __DE_HAL_H__
#include "de_rtmx.h"
#include "de_scaler.h"
#include "de_csc.h"

extern int de_al_mgr_apply(unsigned int screen_id,
			   struct disp_manager_data *data);
extern int de_al_mgr_apply_color(unsigned int screen_id,
			   struct disp_csc_config *cfg);
extern int de_al_init(disp_bsp_init_para *para);
extern int de_al_lyr_apply(unsigned int screen_id,
			   struct disp_layer_config_data *data,
			   unsigned int layer_num, bool direct_show);
extern int de_al_mgr_sync(unsigned int screen_id);
extern int de_al_mgr_update_regs(unsigned int screen_id);
extern int de_al_query_irq(unsigned int screen_id);
extern int de_al_enable_irq(unsigned int screen_id, unsigned en);
int de_update_device_fps(unsigned int sel, u32 fps);
int de_update_clk_rate(u32 rate);
int de_get_clk_rate(void);

extern int de_enhance_set_size(unsigned int screen_id, struct disp_rect *size);
extern int de_enhance_set_format(unsigned int screen_id, unsigned int format);

#endif
