#ifndef __DISP_MANAGER_H__
#define __DISP_MANAGER_H__

#include "disp_private.h"

s32 disp_init_mgr(disp_bsp_init_para * para);
s32 __disp_config_transfer2inner(struct disp_layer_config_inner *cfg_inner,
				      struct disp_layer_config *config);
s32 __disp_config2_transfer2inner(struct disp_layer_config_inner *cfg_inner,
				      struct disp_layer_config2 *config2);

#if defined(CONFIG_SUNXI_IOMMU)
#define DE_MASTOR_ID 0
extern void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag);
#endif

#endif
