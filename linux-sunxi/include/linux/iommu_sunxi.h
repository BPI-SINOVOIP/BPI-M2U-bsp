/*
 * include/linux/iommu_sunxi.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: fanqinghua <fanqinghua@allwinnertech.com>
 *
 * sunxi iommu header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __INCLUDE_IOMMU_SUNXI_H
#define __INCLUDE_IOMMU_SUNXI_H
/*
 * @mastor_id:
 *  DE_MASTOR_ID
 *  DI_MASTOR_ID
 *  VE_MASTOR_ID
 *  CSI_MASTOR_ID
 *  VP9_MASTOR_ID
 * @flag:
 *  true:enable the device iommu
 *  false:disable the mastor iommu
 * void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag);
 */

#define DE_MASTOR_ID 0
#define DI_MASTOR_ID 2
#define VE_MASTOR_ID 3
#define CSI_MASTOR_ID 4
#define VP9_MASTOR_ID 5
void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag);

#endif
