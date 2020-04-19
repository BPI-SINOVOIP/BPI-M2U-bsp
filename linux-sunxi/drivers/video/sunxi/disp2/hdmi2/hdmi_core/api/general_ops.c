/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
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
#endif
#include "general_ops.h"

static errorType_t errorCode = NO_ERROR;

u16 concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo)
{
	return (bit_field(bHi, oHi, nHi) << nLo) | bit_field(bLo, oLo, nLo);
}

u16 byte_to_word(const u8 hi, const u8 lo)
{
	return concat_bits(hi, 0, 8, lo, 0, 8);
}

u8 bit_field(const u16 data, u8 shift, u8 width)
{
	return (data >> shift) & ((((u16) 1) << width) - 1);
}

u32 byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0)
{
	u32 retval = 0;

	retval |= b0 << (0 * 8);
	retval |= b1 << (1 * 8);
	retval |= b2 << (2 * 8);
	retval |= b3 << (3 * 8);
	return retval;
}

void error_set(errorType_t err)
{
	if ((err > NO_ERROR) && (err < ERR_UNDEFINED_HTX))
		errorCode = err;
}

errorType_t error_Get(void)
{
	errorType_t tmpErr = errorCode;

	errorCode = NO_ERROR;
	return tmpErr;
}

