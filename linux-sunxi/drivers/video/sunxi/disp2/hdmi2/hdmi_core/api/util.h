/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef SNPS_API_UTIL_H_
#define SNPS_API_UTIL_H_

#include "../../config.h"

#if defined(__LINUX_PLAT__)
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm.h>
#include <asm/div64.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/compat.h>
#else
#include "../../hdmi_boot.h"
#endif

/* *************************************************************************
 * Data Manipulation and Access
 * *************************************************************************/
/**
 * Find first (least significant) bit set
 * @param[in] data word to search
 * @return bit position or 32 if none is set
 */
static inline unsigned first_bit_set(uint32_t data)
{
	unsigned n = 0;

	if (data != 0) {
		for (n = 0; (data & 1) == 0; n++)
			data >>= 1;
	}
	return n;
}


/**
 * Set bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @param[in] value new value
 * @return new raw data
 */
static inline uint32_t set(uint32_t data, uint32_t mask, uint32_t value)
{
	return ((value << first_bit_set(mask)) & mask) | (data & ~mask);
}

#endif
