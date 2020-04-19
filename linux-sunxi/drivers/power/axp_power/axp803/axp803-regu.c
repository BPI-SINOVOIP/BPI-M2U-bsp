/*
 * drivers/power/axp/axp803/axp803-regu.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Pannan <pannan@allwinnertech.com>
 *
 * regulator driver of axp803
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/module.h>
#include <linux/power/axp_depend.h>
#include "../axp-core.h"
#include "../axp-regulator.h"
#include "axp803.h"
#include "axp803-regu.h"

enum AXP_REGLS {
	VCC_DCDC1,
	VCC_DCDC2,
	VCC_DCDC3,
	VCC_DCDC4,
	VCC_DCDC5,
	VCC_DCDC6,
	VCC_DCDC7,
	VCC_LDO1,
	VCC_LDO2,
	VCC_LDO3,
	VCC_LDO4,
	VCC_LDO5,
	VCC_LDO6,
	VCC_LDO7,
	VCC_LDO8,
	VCC_LDO9,
	VCC_LDO10,
	VCC_LDO11,
	VCC_LDO12,
	VCC_LDO13,
	VCC_LDOIO0,
	VCC_LDOIO1,
	VCC_DC1SW,
	VCC_803_MAX,
};

struct axp803_regulators {
	struct regulator_dev *regulators[VCC_803_MAX];
	struct axp_dev *chip;
};

#define AXP803_LDO(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_LDO(AXP803, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP803_DCDC(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_mask, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_DCDC(AXP803, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, mode_mask, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

#define AXP803_SW(_id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag) \
	AXP_SW(AXP803, _id, min, max, step1, vreg, shift, nbits,\
		ereg, emask, enval, disval, switch_vol, step2, new_level,\
		mode_addr, freq_addr, dvm_ereg, dvm_ebit, dvm_flag)

static struct axp_regulator_info axp803_regulator_info[] = {
	AXP803_DCDC(1,  1600, 3400, 100,  DCDC1, 0, 5,  DCDC1EN, 0x01, 0x01,
			0x00,    0,   0,   0,   0x80,  0x0, 0x3B,    0, 0, 0),
	AXP803_DCDC(2,   500, 1300,  10,  DCDC2, 0, 7,  DCDC2EN, 0x02, 0x02,
			0x00, 1200,  20,   0,   0x80,  0x2, 0x3B, 0x27, 2, 1),
	AXP803_DCDC(3,   500, 1300,  10,  DCDC3, 0, 7,  DCDC3EN, 0x04, 0x04,
			0x00, 1200,  20,   0,   0x80,  0x4, 0x3B, 0x27, 3, 1),
	AXP803_DCDC(4,   500, 1300,  10,  DCDC4, 0, 7,  DCDC4EN, 0x08, 0x08,
			0x00, 1200,  20,   0,   0x80,  0x8, 0x3B, 0x27, 4, 1),
	AXP803_DCDC(5,   800, 1840,  10,  DCDC5, 0, 7,  DCDC5EN, 0x10, 0x10,
			0x00, 1120,  20,   0,   0x80, 0x10, 0x3B, 0x27, 5, 1),
	AXP803_DCDC(6,   600, 1520,  10,  DCDC6, 0, 7,  DCDC6EN, 0x20, 0x20,
			0x00, 1120,  20,   0,   0x80, 0x20, 0x3B, 0x27, 6, 1),
	AXP803_DCDC(7,   600, 1520,  10,  DCDC7, 0, 7,  DCDC7EN, 0x40, 0x40,
			0x00, 1120,  20,   0,   0x80, 0x40, 0x3B, 0x27, 7, 1),
	AXP803_LDO(1,   3000, 3000,   0,   LDO1, 0, 0,   LDO1EN, 0x01, 0x01,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(2,    700, 3300, 100,   LDO2, 0, 5,   LDO2EN, 0x20, 0x20,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(3,    700, 3300, 100,   LDO3, 0, 5,   LDO3EN, 0x40, 0x40,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(4,    700, 3300, 100,   LDO4, 0, 5,   LDO4EN, 0x80, 0x80,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(5,    700, 3300, 100,   LDO5, 0, 5,   LDO5EN, 0x08, 0x08,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(6,    700, 4200, 100,   LDO6, 0, 5,   LDO6EN, 0x10, 0x10,
			0x00, 3400, 200,     0,      0, 0, 0, 0, 0),
	AXP803_LDO(7,    700, 3300, 100,   LDO7, 0, 5,   LDO7EN, 0x20, 0x20,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(8,    700, 3300, 100,   LDO8, 0, 5,   LDO8EN, 0x40, 0x40,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(9,    700, 1900,  50,   LDO9, 0, 5,   LDO9EN, 0x01, 0x01,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(10,   700, 1900,  50,  LDO10, 0, 5,  LDO10EN, 0x02, 0x02,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(11,   700, 1900,  50,  LDO11, 0, 5,  LDO11EN, 0x04, 0x04,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(12,   700, 1450,  50,  LDO12, 0, 4,  LDO12EN, 0x04, 0x04,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(13,   700, 1450,  50,  LDO13, 0, 4,  LDO13EN, 0x08, 0x08,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(IO0,  700, 3300, 100, LDOIO0, 0, 5, LDOIO0EN, 0x07, 0x03,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_LDO(IO1,  700, 3300, 100, LDOIO1, 0, 5, LDOIO1EN, 0x07, 0x03,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
	AXP803_SW(1,    1600, 3400, 100,  DC1SW, 0, 0,  DC1SWEN, 0x80, 0x80,
			0x00,     0,    0,   0,      0, 0, 0, 0, 0),
};

static struct regulator_init_data axp_regl_init_data[] = {
	[VCC_DCDC1] = {
		.constraints = {
			.name = "axp803_dcdc1",
			.min_uV = 1600000,
			.max_uV = 3400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC2] = {
		.constraints = {
			.name = "axp803_dcdc2",
			.min_uV = 500000,
			.max_uV = 1300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC3] = {
		.constraints = {
			.name = "axp803_dcdc3",
			.min_uV = 500000,
			.max_uV = 1300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC4] = {
		.constraints = {
			.name = "axp803_dcdc4",
			.min_uV = 500000,
			.max_uV = 1300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC5] = {
		.constraints = {
			.name = "axp803_dcdc5",
			.min_uV = 800000,
			.max_uV = 1840000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC6] = {
		.constraints = {
			.name = "axp803_dcdc6",
			.min_uV = 600000,
			.max_uV = 1520000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DCDC7] = {
		.constraints = {
			.name = "axp803_dcdc7",
			.min_uV = 600000,
			.max_uV = 1520000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO1] = {
		.constraints = {
			.name = "axp803_rtc",
			.min_uV = 3000000,
			.max_uV = 3000000,
		},
	},
	[VCC_LDO2] = {
		.constraints = {
			.name = "axp803_aldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO3] = {
		.constraints = {
			.name = "axp803_aldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO4] = {
		.constraints = {
			.name = "axp803_aldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO5] = {
		.constraints = {
			.name = "axp803_dldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO6] = {
		.constraints = {
			.name = "axp803_dldo2",
			.min_uV = 700000,
			.max_uV = 4200000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO7] = {
		.constraints = {
			.name = "axp803_dldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO8] = {
		.constraints = {
			.name = "axp803_dldo4",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO9] = {
		.constraints = {
			.name = "axp803_eldo1",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO10] = {
		.constraints = {
			.name = "axp803_eldo2",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO11] = {
		.constraints = {
			.name = "axp803_eldo3",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO12] = {
		.constraints = {
			.name = "axp803_fldo1",
			.min_uV = 700000,
			.max_uV = 1450000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDO13] = {
		.constraints = {
			.name = "axp803_fldo2",
			.min_uV = 700000,
			.max_uV = 1450000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDOIO0] = {
		.constraints = {
			.name = "axp803_ldoio0",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_LDOIO1] = {
		.constraints = {
			.name = "axp803_ldoio1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE
				| REGULATOR_CHANGE_STATUS,
		},
	},
	[VCC_DC1SW] = {
		.constraints = {
			.name = "axp803_dc1sw",
			.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		},
	},
};

static ssize_t workmode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = axp_regmap_read(regmap, info->mode_reg, &val);
	if (ret)
		return sprintf(buf, "IO ERROR\n");

	if ((val & info->mode_mask) == info->mode_mask)
		return sprintf(buf, "PWM\n");
	else
		return sprintf(buf, "AUTO\n");
}

static ssize_t workmode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;
	unsigned int mode;
	int ret;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = sscanf(buf, "%u", &mode);
	if (ret != 1)
		return -EINVAL;

	val = !!mode;
	if (val)
		axp_regmap_set_bits(regmap, info->mode_reg, info->mode_mask);
	else
		axp_regmap_clr_bits(regmap, info->mode_reg, info->mode_mask);

	return count;
}

static ssize_t frequency_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ret;
	uint8_t val;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	ret = axp_regmap_read(regmap, info->freq_reg, &val);
	if (ret)
		return ret;

	ret = val & 0x0F;

	return sprintf(buf, "%d\n", (ret * 5 + 50));
}

static ssize_t frequency_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	uint8_t val, tmp;
	int var, err;
	struct regulator_dev *rdev;
	struct axp_regulator_info *info;
	struct axp_regmap *regmap;

	rdev = container_of(dev, struct regulator_dev, dev);
	info = rdev_get_drvdata(rdev);
	regmap = info->regmap;

	err = kstrtoint(buf, 10, &var);
	if (err)
		return err;

	if (var < 50)
		var = 50;

	if (var > 100)
		var = 100;

	val = (var - 50) / 5;
	val &= 0x0F;

	axp_regmap_read(regmap, info->freq_reg, &tmp);
	tmp &= 0xF0;
	val |= tmp;
	axp_regmap_write(regmap, info->freq_reg, val);

	return count;
}

static struct device_attribute axp_regu_attrs[] = {
	AXP_REGU_ATTR(workmode),
	AXP_REGU_ATTR(frequency),
};

static int axp_regu_create_attrs(struct device *dev)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(axp_regu_attrs); j++) {
		ret = device_create_file(dev, &axp_regu_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}

	return 0;

sysfs_failed:
	while (j--)
		device_remove_file(dev, &axp_regu_attrs[j]);
	return ret;
}

static s32 axp803_regu_dependence(const char *ldo_name)
{
	s32 axp803_dependence = 0;

	if (strstr(ldo_name, "dcdc1") != NULL)
		axp803_dependence |= AXP803_813_DCDC1;
	else if (strstr(ldo_name, "dcdc2") != NULL)
		axp803_dependence |= AXP803_813_DCDC2;
	else if (strstr(ldo_name, "dcdc3") != NULL)
		axp803_dependence |= AXP803_813_DCDC3;
	else if (strstr(ldo_name, "dcdc4") != NULL)
		axp803_dependence |= AXP803_813_DCDC4;
	else if (strstr(ldo_name, "dcdc5") != NULL)
		axp803_dependence |= AXP803_813_DCDC5;
	else if (strstr(ldo_name, "dcdc6") != NULL)
		axp803_dependence |= AXP803_813_DCDC6;
	else if (strstr(ldo_name, "dcdc7") != NULL)
		axp803_dependence |= AXP803_813_DCDC7;
	else if (strstr(ldo_name, "aldo1") != NULL)
		axp803_dependence |= AXP803_813_ALDO1;
	else if (strstr(ldo_name, "aldo2") != NULL)
		axp803_dependence |= AXP803_813_ALDO2;
	else if (strstr(ldo_name, "aldo3") != NULL)
		axp803_dependence |= AXP803_813_ALDO3;
	else if (strstr(ldo_name, "dldo1") != NULL)
		axp803_dependence |= AXP803_813_DLDO1;
	else if (strstr(ldo_name, "dldo2") != NULL)
		axp803_dependence |= AXP803_813_DLDO2;
	else if (strstr(ldo_name, "dldo3") != NULL)
		axp803_dependence |= AXP803_813_DLDO3;
	else if (strstr(ldo_name, "dldo4") != NULL)
		axp803_dependence |= AXP803_813_DLDO4;
	else if (strstr(ldo_name, "eldo1") != NULL)
		axp803_dependence |= AXP803_813_ELDO1;
	else if (strstr(ldo_name, "eldo2") != NULL)
		axp803_dependence |= AXP803_813_ELDO2;
	else if (strstr(ldo_name, "eldo3") != NULL)
		axp803_dependence |= AXP803_813_ELDO3;
	else if (strstr(ldo_name, "fldo1") != NULL)
		axp803_dependence |= AXP803_813_FLDO1;
	else if (strstr(ldo_name, "fldo2") != NULL)
		axp803_dependence |= AXP803_813_FLDO2;
	else if (strstr(ldo_name, "ldoio0") != NULL)
		axp803_dependence |= AXP803_813_LDOIO0;
	else if (strstr(ldo_name, "ldoio1") != NULL)
		axp803_dependence |= AXP803_813_LDOIO1;
	else if (strstr(ldo_name, "dc1sw") != NULL)
		axp803_dependence |= AXP803_813_DC1SW;
	else if (strstr(ldo_name, "rtc") != NULL)
		axp803_dependence |= AXP803_813_RTC;
	else
		return -1;

	axp803_dependence |= (0 << 30);
	return axp803_dependence;
}

static int axp803_regulator_probe(struct platform_device *pdev)
{
	s32 i, ret = 0;
	struct axp_regulator_info *info;
	struct axp803_regulators *regu_data;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		ret = axp_regulator_dt_parse(pdev->dev.of_node,
					axp_regl_init_data,
					axp803_regu_dependence);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axp803 regulator device tree err!\n");
		return -EBUSY;
	}

	regu_data = devm_kzalloc(&pdev->dev, sizeof(*regu_data),
					GFP_KERNEL);
	if (!regu_data)
		return -ENOMEM;

	regu_data->chip = axp_dev;
	platform_set_drvdata(pdev, regu_data);

	for (i = 0; i < VCC_803_MAX; i++) {
		info = &axp803_regulator_info[i];
		info->pmu_num = axp_dev->pmu_num;
		regu_data->regulators[i] = axp_regulator_register(&pdev->dev,
				axp_dev->regmap, &axp_regl_init_data[i], info);

		if (IS_ERR(regu_data->regulators[i])) {
			dev_err(&pdev->dev, "failed to register regulator %s\n",
					info->desc.name);
			while (--i >= 0)
				axp_regulator_unregister(
					regu_data->regulators[i]);

			return -1;
		}

		if (info->desc.id >= AXP_DCDC_ID_START) {
			ret = axp_regu_create_attrs(
						&regu_data->regulators[i]->dev);
			if (ret)
				dev_err(&pdev->dev,
					"failed to register regulator attr %s\n",
					info->desc.name);
		}
	}

	/* voltage not to the OTP default when wakeup  */
	axp_regmap_set_bits(axp_dev->regmap, AXP803_HOTOVER_CTL, 0x02);

	return 0;
}

static int axp803_regulator_remove(struct platform_device *pdev)
{
	struct axp803_regulators *regu_data = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < VCC_803_MAX; i++)
		regulator_unregister(regu_data->regulators[i]);

	return 0;
}
static const struct of_device_id axp803_regu_dt_ids[] = {
	{ .compatible = "axp803-regulator", },
	{ .compatible = "axp288-regulator", },
	{},
};
MODULE_DEVICE_TABLE(of, axp803_regu_dt_ids);

static struct platform_driver axp803_regulator_driver = {
	.driver     = {
		.name   = "axp803-regulator",
		.of_match_table = axp803_regu_dt_ids,
	},
	.probe      = axp803_regulator_probe,
	.remove     = axp803_regulator_remove,
};

static int __init axp803_regulator_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp803_regulator_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
subsys_initcall(axp803_regulator_initcall);

MODULE_DESCRIPTION("Regulator Driver of AXP803");
MODULE_AUTHOR("pannan");
MODULE_LICENSE("GPL");
