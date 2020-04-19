/*
 * Copyright (C) 2012 Samsung Electronics.
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Tomasz Figa <t.figa@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/suspend.h>

#include <asm/firmware.h>

static const struct firmware_ops default_firmware_ops;

const struct firmware_ops *firmware_ops = &default_firmware_ops;

static const struct firmware_secure_ops default_firmware_secure_ops;

const struct firmware_secure_ops *firmware_secure_ops =
	&default_firmware_secure_ops;
EXPORT_SYMBOL(firmware_secure_ops);
