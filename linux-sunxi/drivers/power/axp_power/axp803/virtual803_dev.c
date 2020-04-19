/*
 * drivers/power/axp/axp803/virtualaxp803_dev.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Pannan <pannan@allwinnertech.com>
 *
 * virtual regulator driver of axp803
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/module.h>

static struct platform_device virt[] = {
	{
		.name = "reg-803-cs-rtc",
		.id = -1,
		.dev = {
			.platform_data = "axp803_rtc",
		}
	},
	{
		.name = "reg-803-cs-aldo1",
		.id = -1,
		.dev = {
			.platform_data = "axp803_aldo1",
		}
	},
	{
		.name = "reg-803-cs-aldo2",
		.id = -1,
		.dev = {
			.platform_data = "axp803_aldo2",
		}
	},
	{
		.name = "reg-803-cs-aldo3",
		.id = -1,
		.dev = {
			.platform_data = "axp803_aldo3",
		}
	},
	{
		.name = "reg-803-cs-dldo1",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dldo1",
		}
	},
	{
		.name = "reg-803-cs-dldo2",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dldo2",
		}
	},
	{
		.name = "reg-803-cs-dldo3",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dldo3",
		}
	},
	{
		.name = "reg-803-cs-dldo4",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dldo4",
		}
	},
	{
		.name = "reg-803-cs-eldo1",
		.id = -1,
		.dev = {
			.platform_data = "axp803_eldo1",
		}
	},
	{
		.name = "reg-803-cs-eldo2",
		.id = -1,
		.dev = {
			.platform_data = "axp803_eldo2",
		}
	},
	{
		.name = "reg-803-cs-eldo3",
		.id = -1,
		.dev = {
			.platform_data = "axp803_eldo3",
		}
	},
	{
		.name = "reg-803-cs-fldo1",
		.id = -1,
		.dev = {
			.platform_data = "axp803_fldo1",
		}
	},
	{
		.name = "reg-803-cs-fldo2",
		.id = -1,
		.dev = {
			.platform_data = "axp803_fldo2",
		}
	},
	{
		.name = "reg-803-cs-dcdc1",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dcdc1",
		}
	},
	{
		.name = "reg-803-cs-dcdc2",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dcdc2",
		}
	},
	{
		.name = "reg-803-cs-dcdc3",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dcdc3",
		}
	},
	{
		.name = "reg-803-cs-dcdc4",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dcdc4",
		}
	},
	{
		.name = "reg-803-cs-dcdc5",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dcdc5",
		}
	},
	{
		.name = "reg-803-cs-dcdc6",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dcdc6",
		}
	},
	{
		.name = "reg-803-cs-dcdc7",
		.id = -1,
		.dev = {
			.platform_data = "axp803_dcdc7",
		}
	},
	{
		.name = "reg-803-cs-ldoio0",
		.id = -1,
		.dev = {
			.platform_data = "axp803_ldoio0",
		}
	},
	{
		.name = "reg-803-cs-ldoio1",
		.id = -1,
		.dev = {
			.platform_data = "axp803_ldoio1",
		}
	},
};

static int __init virtual_init(void)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(virt); j++) {
		ret = platform_device_register(&virt[j]);
		if (ret)
			goto creat_devices_failed;
	}

	return 0;

creat_devices_failed:
	while (j--)
		platform_device_register(&virt[j]);
	return ret;
}
module_init(virtual_init);

static void __exit virtual_exit(void)
{
	int j;

	for (j = ARRAY_SIZE(virt) - 1; j >= 0; j--)
		platform_device_unregister(&virt[j]);
}
module_exit(virtual_exit);

MODULE_DESCRIPTION("X-POWERS axp regulator test");
MODULE_AUTHOR("Weijin Zhong");
MODULE_LICENSE("GPL");
