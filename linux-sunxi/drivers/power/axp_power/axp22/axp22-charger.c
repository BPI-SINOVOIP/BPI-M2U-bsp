#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include "../axp-core.h"
#include "../axp-charger.h"
#include "axp22-charger.h"

static int axp22_get_ac_voltage(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_get_ac_current(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_set_ac_vhold(struct axp_charger_dev *cdev, int vol)
{
	return 0;
}

static int axp22_get_ac_vhold(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_set_ac_ihold(struct axp_charger_dev *cdev, int cur)
{
	return 0;
}

static int axp22_get_ac_ihold(struct axp_charger_dev *cdev)
{
	return 0;
}

static struct axp_ac_info axp22_ac_info = {
	.det_bit         = 7,
	.det_offset      = AXP22_CHARGE_STATUS,
	.valid_bit       = 6,
	.valid_offset    = AXP22_CHARGE_STATUS,
	.in_short_bit    = 1,
	.in_short_offset = AXP22_CHARGE_STATUS,
	.get_ac_voltage  = axp22_get_ac_voltage,
	.get_ac_current  = axp22_get_ac_current,
	.set_ac_vhold    = axp22_set_ac_vhold,
	.get_ac_vhold    = axp22_get_ac_vhold,
	.set_ac_ihold    = axp22_set_ac_ihold,
	.get_ac_ihold    = axp22_get_ac_ihold,
};

static int axp22_get_usb_voltage(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_get_usb_current(struct axp_charger_dev *cdev)
{
	return 0;
}

static int axp22_set_usb_vhold(struct axp_charger_dev *cdev, int vol)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	if (vol) {
		axp_regmap_set_bits(map, AXP22_CHARGE_VBUS, 0x40);
		if (vol >= 4000 && vol <= 4700) {
			tmp = (vol - 4000)/100;
			axp_regmap_update(map, AXP22_CHARGE_VBUS,
				tmp<<3, 0x7<<3);
		} else {
			pr_err("set usb limit voltage error, %d mV\n",
				axp22_config.pmu_usbpc_vol);
		}
	} else {
		axp_regmap_clr_bits(map, AXP22_CHARGE_VBUS, 0x40);
	}

	return 0;

}

static int axp22_get_usb_vhold(struct axp_charger_dev *cdev)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_CHARGE_VBUS, &tmp);
	tmp = (tmp >> 3) & 0x7;

	return 4000 + tmp * 100;
}

static int axp22_set_usb_ihold(struct axp_charger_dev *cdev, int cur)
{
	struct axp_regmap *map = cdev->chip->regmap;

	if (cur) {
		if (cur >= 900)
			axp_regmap_clr_bits(map, AXP22_CHARGE_VBUS, 0x3);
		else if (cur < 900)
			axp_regmap_update(map, AXP22_CHARGE_VBUS, 0x1, 0x3);
	} else {
		axp_regmap_set_bits(map, AXP22_CHARGE_VBUS, 0x3);
	}

	return 0;
}

static int axp22_get_usb_ihold(struct axp_charger_dev *cdev)
{
	u8 tmp;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_CHARGE_VBUS, &tmp);
	tmp = tmp & 0x3;
	if (tmp == 0x1)
		return 500;
	else if (tmp == 0)
		return 900;
	else
		return 0;
}

static struct axp_usb_info axp22_usb_info = {
	.det_bit         = 5,
	.det_offset      = AXP22_CHARGE_STATUS,
	.valid_bit       = 4,
	.valid_offset    = AXP22_CHARGE_STATUS,
	.get_usb_voltage = axp22_get_usb_voltage,
	.get_usb_current = axp22_get_usb_current,
	.set_usb_vhold   = axp22_set_usb_vhold,
	.get_usb_vhold   = axp22_get_usb_vhold,
	.set_usb_ihold   = axp22_set_usb_ihold,
	.get_usb_ihold   = axp22_get_usb_ihold,
};

static int axp22_get_rest_cap(struct axp_charger_dev *cdev)
{
	u8 val, temp_val[2], batt_max_cap_val[2];
	int batt_max_cap, coulumb_counter;
	int rest_vol;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_CAP, &val);
	rest_vol = (int) (val & 0x7F);

	axp_regmap_reads(map, 0xe2, 2, temp_val);
	coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	axp_regmap_reads(map, 0xe0, 2, temp_val);
	batt_max_cap = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	/* Avoid the power stay in 100% for a long time. */
	if (coulumb_counter > batt_max_cap) {
		batt_max_cap_val[0] = temp_val[0] | (0x1<<7);
		batt_max_cap_val[1] = temp_val[1];
		axp_regmap_writes(map, 0xe2, 2, batt_max_cap_val);
		AXP_DEBUG(AXP_SPLY, cdev->chip->pmu_num,
				"Axp22 coulumb_counter = %d\n", batt_max_cap);
	}

	return rest_vol;
}

static int axp22_get_bat_health(struct axp_charger_dev *cdev)
{
	u8 val;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, AXP22_FAULT_LOG1, &val);
	if (val & AXP22_FAULT_LOG_BATINACT)
		return POWER_SUPPLY_HEALTH_DEAD;
	else if (val & AXP22_FAULT_LOG_OVER_TEMP)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (val & AXP22_FAULT_LOG_COLD)
		return POWER_SUPPLY_HEALTH_COLD;
	else
		return POWER_SUPPLY_HEALTH_GOOD;
}

static inline int axp22_vbat_to_mV(u32 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1100 / 1000;
}

static int axp22_get_vbat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, AXP22_VBATH_RES, 2, tmp);
	res = (tmp[0] << 8) | tmp[1];

	return axp22_vbat_to_mV(res);
}

static inline int axp22_ibat_to_mA(u32 reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static inline int axp22_icharge_to_mA(u32 reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static int axp22_get_ibat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, AXP22_IBATH_REG, 2, tmp);
	res = (tmp[0] << 8) | tmp[1];

	return axp22_icharge_to_mA(res);
}

static int axp22_get_disibat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 dis_res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, AXP22_DISIBATH_REG, 2, tmp);
	dis_res = (tmp[0] << 8) | tmp[1];

	return axp22_ibat_to_mA(dis_res);
}

static int axp22_get_ic_temp(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, AXP22_INTTEMP, 2, tmp);
	res = (tmp[0] << 4) + (tmp[1] & 0x0F);
	res = res*1063/10000 - 2667/10;
	return res;
}

static inline int axp22_vts_to_temp(int data)
{
	int temp;
	if (data < 80)
		return 30;
	else if (data < axp22_config.pmu_bat_temp_para16)
		return 80;
	else if (data <= axp22_config.pmu_bat_temp_para15)
		temp = 70 + (axp22_config.pmu_bat_temp_para15-data)*10/
			(axp22_config.pmu_bat_temp_para15-
			axp22_config.pmu_bat_temp_para16);
	else if (data <= axp22_config.pmu_bat_temp_para14)
		temp = 60 + (axp22_config.pmu_bat_temp_para14-data)*10/
			(axp22_config.pmu_bat_temp_para14-
			axp22_config.pmu_bat_temp_para15);
	else if (data <= axp22_config.pmu_bat_temp_para13)
		temp = 55 + (axp22_config.pmu_bat_temp_para13-data)*5/
			(axp22_config.pmu_bat_temp_para13-
			axp22_config.pmu_bat_temp_para14);
	else if (data <= axp22_config.pmu_bat_temp_para12)
		temp = 50 + (axp22_config.pmu_bat_temp_para12-data)*5/
			(axp22_config.pmu_bat_temp_para12-
			axp22_config.pmu_bat_temp_para13);
	else if (data <= axp22_config.pmu_bat_temp_para11)
		temp = 45 + (axp22_config.pmu_bat_temp_para11-data)*5/
			(axp22_config.pmu_bat_temp_para11-
			axp22_config.pmu_bat_temp_para12);
	else if (data <= axp22_config.pmu_bat_temp_para10)
		temp = 40 + (axp22_config.pmu_bat_temp_para10-data)*5/
			(axp22_config.pmu_bat_temp_para10-
			axp22_config.pmu_bat_temp_para11);
	else if (data <= axp22_config.pmu_bat_temp_para9)
		temp = 30 + (axp22_config.pmu_bat_temp_para9-data)*10/
			(axp22_config.pmu_bat_temp_para9-
			axp22_config.pmu_bat_temp_para10);
	else if (data <= axp22_config.pmu_bat_temp_para8)
		temp = 20 + (axp22_config.pmu_bat_temp_para8-data)*10/
			(axp22_config.pmu_bat_temp_para8-
			axp22_config.pmu_bat_temp_para9);
	else if (data <= axp22_config.pmu_bat_temp_para7)
		temp = 10 + (axp22_config.pmu_bat_temp_para7-data)*10/
			(axp22_config.pmu_bat_temp_para7-
			axp22_config.pmu_bat_temp_para8);
	else if (data <= axp22_config.pmu_bat_temp_para6)
		temp = 5 + (axp22_config.pmu_bat_temp_para6-data)*5/
			(axp22_config.pmu_bat_temp_para6-
			axp22_config.pmu_bat_temp_para7);
	else if (data <= axp22_config.pmu_bat_temp_para5)
		temp = 0 + (axp22_config.pmu_bat_temp_para5-data)*5/
			(axp22_config.pmu_bat_temp_para5-
			axp22_config.pmu_bat_temp_para6);
	else if (data <= axp22_config.pmu_bat_temp_para4)
		temp = -5 + (axp22_config.pmu_bat_temp_para4-data)*5/
			(axp22_config.pmu_bat_temp_para4-
			axp22_config.pmu_bat_temp_para5);
	else if (data <= axp22_config.pmu_bat_temp_para3)
		temp = -10 + (axp22_config.pmu_bat_temp_para3-data)*5/
			(axp22_config.pmu_bat_temp_para3-
			axp22_config.pmu_bat_temp_para4);
	else if (data <= axp22_config.pmu_bat_temp_para2)
		temp = -15 + (axp22_config.pmu_bat_temp_para2-data)*5/
			(axp22_config.pmu_bat_temp_para2-
			axp22_config.pmu_bat_temp_para3);
	else if (data <= axp22_config.pmu_bat_temp_para1)
		temp = -25 + (axp22_config.pmu_bat_temp_para1-data)*10/
			(axp22_config.pmu_bat_temp_para1-
			axp22_config.pmu_bat_temp_para2);
	else
		temp = -25;
	return temp;
}

static int axp22_bat_temp_set(int bat_temp)
{
	static int count;
	static int bat_val = -7;

	if (-7 >= bat_temp) {
		if (10 == count) {
			if (-13 >= bat_val)
				bat_val = -13;
			else {
				bat_val--;
				bat_temp = bat_val;
			}
			count = 0;
		} else
			count++;
	} else if (-7 > bat_val) {
		if (10 == count) {
			bat_val++;
			bat_temp = bat_val;
			count = 0;
		} else
			count++;
	}
	return bat_temp;
}

static int axp22_get_bat_temp(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, AXP22_VTS_RES, 2, tmp);
	res = (tmp[0] << 4) + (tmp[1] & 0x0F);
	res = res*800/1000;
	return axp22_bat_temp_set(axp22_vts_to_temp(res));
}


static int axp22_set_chg_cur(struct axp_charger_dev *cdev, int cur)
{
	uint8_t tmp = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (cur == 0)
		axp_regmap_clr_bits(map, AXP22_CHARGE_CONTROL1, 0x80);
	else
		axp_regmap_set_bits(map, AXP22_CHARGE_CONTROL1, 0x80);

	if (cur >= 300 && cur <= 2550) {
		tmp = (cur - 300) / 150;
		axp_regmap_update(map, AXP22_CHARGE_CONTROL1, tmp, 0x0F);
	} else if (cur < 300) {
		axp_regmap_clr_bits(map, AXP22_CHARGE_CONTROL1, 0x0F);
	} else {
		axp_regmap_set_bits(map, AXP22_CHARGE_CONTROL1, 0x0F);
	}

	return 0;
}

static int axp22_set_chg_vol(struct axp_charger_dev *cdev, int vol)
{
	uint8_t tmp = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (vol < 4200) {
		tmp &= ~(3 << 5);
	} else if (vol < 4220) {
		tmp &= ~(3 << 5);
		tmp |= 1 << 6;
	} else if (vol < 4240) {
		tmp &= ~(3 << 5);
		tmp |= 1 << 5;
	} else {
		tmp |= 3 << 5;
	}

	axp_regmap_update(map, AXP22_CHARGE_CONTROL1, tmp, 0x3<<5);

	return 0;
}

static struct axp_battery_info axp22_batt_info = {
	.chgstat_bit          = 6,
	.chgstat_offset       = AXP22_MODE_CHGSTATUS,
	.det_bit              = 5,
	.det_valid_bit        = 4,
	.det_valid            = 1,
	.det_offset           = AXP22_MODE_CHGSTATUS,
	.cur_direction_bit    = 2,
	.cur_direction_offset = AXP22_CHARGE_STATUS,
	.get_rest_cap         = axp22_get_rest_cap,
	.get_bat_health       = axp22_get_bat_health,
	.get_vbat             = axp22_get_vbat,
	.get_ibat             = axp22_get_ibat,
	.get_disibat          = axp22_get_disibat,
	.get_bat_temp         = axp22_get_bat_temp,
	.get_ic_temp          = axp22_get_ic_temp,
	.set_chg_cur          = axp22_set_chg_cur,
	.set_chg_vol          = axp22_set_chg_vol,
};

static struct power_supply_info battery_data = {
	.name = "PTI PL336078",
	.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
	.voltage_max_design = 4200000,
	.voltage_min_design = 3500000,
	.use_for_apm = 1,
};

static struct axp_supply_info axp22_spy_info = {
	.ac   = &axp22_ac_info,
	.usb  = &axp22_usb_info,
	.batt = &axp22_batt_info,
};

static int axp22_charger_init(struct axp_dev *axp_dev)
{
	u8 ocv_cap[32];
	u8 val;
	int cur_coulomb_counter, rdc;
	struct axp_regmap *map = axp_dev->regmap;

	if (axp22_config.pmu_init_chgend_rate == 10)
		val &= ~(1 << 4);
	else
		val |= 1 << 4;

	val &= 0x7F;
	axp_regmap_write(map, AXP22_CHARGE_CONTROL1, val);

	if (axp22_config.pmu_init_chg_pretime < 30)
		axp22_config.pmu_init_chg_pretime = 30;

	if (axp22_config.pmu_init_chg_csttime < 360)
		axp22_config.pmu_init_chg_csttime = 360;

	val = ((((axp22_config.pmu_init_chg_pretime - 40) / 10) << 6)
			| ((axp22_config.pmu_init_chg_csttime - 360) / 120));
	axp_regmap_update(map, AXP22_CHARGE_CONTROL2, val, 0xC2);

	/* adc set */
	val = AXP22_ADC_BATVOL_ENABLE | AXP22_ADC_BATCUR_ENABLE;
	if (0 != axp22_config.pmu_bat_temp_enable)
		val = val | AXP22_ADC_TSVOL_ENABLE;
	axp_regmap_update(map, AXP22_ADC_CONTROL, val,
						AXP22_ADC_BATVOL_ENABLE
						| AXP22_ADC_BATCUR_ENABLE
						| AXP22_ADC_TSVOL_ENABLE);

	axp_regmap_read(map, AXP22_ADC_CONTROL3, &val);
	switch (axp22_config.pmu_init_adc_freq / 100) {
	case 1:
		val &= ~(3 << 6);
		break;
	case 2:
		val &= ~(3 << 6);
		val |= 1 << 6;
		break;
	case 4:
		val &= ~(3 << 6);
		val |= 2 << 6;
		break;
	case 8:
		val |= 3 << 6;
		break;
	default:
		break;
	}

	if (0 != axp22_config.pmu_bat_temp_enable)
		val &= (~(1 << 2));
	axp_regmap_write(map, AXP22_ADC_CONTROL3, val);

	if (0 != axp22_config.pmu_bat_temp_enable) {
		axp_regmap_write(map, AXP22_VLTF_CHGSET,
			axp22_config.pmu_bat_charge_ltf*10/128);
		axp_regmap_write(map, AXP22_VHTF_CHGSET,
			axp22_config.pmu_bat_charge_htf*10/128);
		axp_regmap_write(map, AXP22_VLTF_DISCHGSET,
			axp22_config.pmu_bat_shutdown_ltf*10/128);
		axp_regmap_write(map, AXP22_VHTF_DISCHGSET,
			axp22_config.pmu_bat_shutdown_htf*10/128);
	}

	/* bat para */
	axp_regmap_write(map, AXP22_WARNING_LEVEL,
		((axp22_config.pmu_battery_warning_level1 - 5) << 4)
		+ axp22_config.pmu_battery_warning_level2);
	ocv_cap[0]  = axp22_config.pmu_bat_para1;
	ocv_cap[1]  = axp22_config.pmu_bat_para2;
	ocv_cap[2]  = axp22_config.pmu_bat_para3;
	ocv_cap[3]  = axp22_config.pmu_bat_para4;
	ocv_cap[4]  = axp22_config.pmu_bat_para5;
	ocv_cap[5]  = axp22_config.pmu_bat_para6;
	ocv_cap[6]  = axp22_config.pmu_bat_para7;
	ocv_cap[7]  = axp22_config.pmu_bat_para8;
	ocv_cap[8]  = axp22_config.pmu_bat_para9;
	ocv_cap[9]  = axp22_config.pmu_bat_para10;
	ocv_cap[10] = axp22_config.pmu_bat_para11;
	ocv_cap[11] = axp22_config.pmu_bat_para12;
	ocv_cap[12] = axp22_config.pmu_bat_para13;
	ocv_cap[13] = axp22_config.pmu_bat_para14;
	ocv_cap[14] = axp22_config.pmu_bat_para15;
	ocv_cap[15] = axp22_config.pmu_bat_para16;
	ocv_cap[16] = axp22_config.pmu_bat_para17;
	ocv_cap[17] = axp22_config.pmu_bat_para18;
	ocv_cap[18] = axp22_config.pmu_bat_para19;
	ocv_cap[19] = axp22_config.pmu_bat_para20;
	ocv_cap[20] = axp22_config.pmu_bat_para21;
	ocv_cap[21] = axp22_config.pmu_bat_para22;
	ocv_cap[22] = axp22_config.pmu_bat_para23;
	ocv_cap[23] = axp22_config.pmu_bat_para24;
	ocv_cap[24] = axp22_config.pmu_bat_para25;
	ocv_cap[25] = axp22_config.pmu_bat_para26;
	ocv_cap[26] = axp22_config.pmu_bat_para27;
	ocv_cap[27] = axp22_config.pmu_bat_para28;
	ocv_cap[28] = axp22_config.pmu_bat_para29;
	ocv_cap[29] = axp22_config.pmu_bat_para30;
	ocv_cap[30] = axp22_config.pmu_bat_para31;
	ocv_cap[31] = axp22_config.pmu_bat_para32;
	axp_regmap_writes(map, 0xC0, 32, ocv_cap);

	/*Init 16's Reset PMU en */
	if (axp22_config.pmu_reset)
		axp_regmap_set_bits(map, 0x8F, 0x08); /* enable */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x08); /* disable */

	/*Init IRQ wakeup en*/
	if (axp22_config.pmu_irq_wakeup)
		axp_regmap_set_bits(map, 0x8F, 0x80); /* enable */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x80); /* disable */

	/*Init N_VBUSEN status*/
	if (axp22_config.pmu_vbusen_func)
		axp_regmap_set_bits(map, 0x8F, 0x10); /* output */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x10); /* input */

	/*Init InShort status*/
	if (axp22_config.pmu_inshort)
		axp_regmap_set_bits(map, 0x8F, 0x60); /* InShort */
	else
		axp_regmap_clr_bits(map, 0x8F, 0x60); /* auto detect */

	/*Init CHGLED function*/
	if (axp22_config.pmu_chgled_func)
		axp_regmap_set_bits(map, 0x32, 0x08); /* control by charger */
	else
		axp_regmap_clr_bits(map, 0x32, 0x08); /* drive MOTO */

	/*set CHGLED Indication Type*/
	if (axp22_config.pmu_chgled_type)
		axp_regmap_set_bits(map, 0x34, 0x10); /* Type B */
	else
		axp_regmap_clr_bits(map, 0x34, 0x10); /* Type A */

	/*Init PMU Over Temperature protection*/
	if (axp22_config.pmu_hot_shutdown)
		axp_regmap_set_bits(map, 0x8f, 0x04); /* enable */
	else
		axp_regmap_clr_bits(map, 0x8f, 0x04); /* disable */

	/*Init battery capacity correct function*/
	if (axp22_config.pmu_batt_cap_correct)
		axp_regmap_set_bits(map, 0xb8, 0x20); /* enable */
	else
		axp_regmap_clr_bits(map, 0xb8, 0x20); /* disable */

	/* Init battery regulator enable or not when charge finish*/
	if (axp22_config.pmu_chg_end_on_en)
		axp_regmap_set_bits(map, 0x34, 0x20); /* enable */
	else
		axp_regmap_clr_bits(map, 0x34, 0x20); /* disable */

	if (!axp22_config.pmu_batdeten)
		axp_regmap_clr_bits(map, AXP22_PDBC, 0x40);
	else
		axp_regmap_set_bits(map, AXP22_PDBC, 0x40);

	/* RDC initial */
	axp_regmap_read(map, AXP22_RDC0, &val);
	if ((axp22_config.pmu_battery_rdc) && (!(val & 0x40))) {
		rdc = (axp22_config.pmu_battery_rdc * 10000 + 5371) / 10742;
		axp_regmap_write(map, AXP22_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		axp_regmap_write(map, AXP22_RDC1, rdc & 0x00FF);
	}

	axp_regmap_read(map, AXP22_BATCAP0, &val);
	if ((axp22_config.pmu_battery_cap) && (!(val & 0x80))) {
		cur_coulomb_counter = axp22_config.pmu_battery_cap
					* 1000 / 1456;
		axp_regmap_write(map, AXP22_BATCAP0,
					((cur_coulomb_counter >> 8) | 0x80));
		axp_regmap_write(map, AXP22_BATCAP1,
					cur_coulomb_counter & 0x00FF);
	} else if (!axp22_config.pmu_battery_cap) {
		axp_regmap_write(map, AXP22_BATCAP0, 0x00);
		axp_regmap_write(map, AXP22_BATCAP1, 0x00);
	}

	if (axp22_config.pmu_bat_unused == 1)
		axp22_spy_info.batt->det_unused = 1;
	else
		axp22_spy_info.batt->det_unused = 0;

	return 0;
}

static irqreturn_t axp_ac_usb_in_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	axp_usbac_in(chg_dev);
	return IRQ_HANDLED;
}

extern int axp_usbcur(enum AW_CHARGE_TYPE type);
static irqreturn_t axp_ac_out_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_usbcur(CHARGE_USB_20);
	axp_change(chg_dev);
	axp_usbac_out(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_usb_out_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	axp_usbac_out(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_capchange_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_capchange(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_change_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_low_warning1_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	return IRQ_HANDLED;
}

static irqreturn_t axp_low_warning2_isr(int irq, void *data)
{
	struct axp_charger_dev *chg_dev = data;
	axp_change(chg_dev);
	return IRQ_HANDLED;
}

static struct axp_interrupts axp_charger_irq[] = {
	{"usb in",        axp_ac_usb_in_isr},
	{"usb out",       axp_usb_out_isr},
	{"ac in",         axp_ac_usb_in_isr},
	{"ac out",        axp_ac_out_isr},
	{"bat in",        axp_capchange_isr},
	{"bat out",       axp_capchange_isr},
	{"bat temp low",  axp_change_isr},
	{"bat temp over", axp_change_isr},
	{"charging",      axp_change_isr},
	{"charge over",   axp_change_isr},
	{"low warning1",  axp_low_warning1_isr},
	{"low warning2",  axp_low_warning2_isr},
	{"ic temp over",  axp_change_isr},
};

static int axp22_charger_probe(struct platform_device *pdev)
{
	int ret, i, irq;
	struct axp_charger_dev *chg_dev;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		/* get dt and sysconfig */
		ret = axp_charger_dt_parse(pdev->dev.of_node, &axp22_config);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axp22 charger device tree err!\n");
		return -EBUSY;
	}

	axp22_ac_info.ac_vol = axp22_config.pmu_ac_vol;
	axp22_ac_info.ac_cur = axp22_config.pmu_ac_cur;
	axp22_usb_info.usb_pc_vol = axp22_config.pmu_usbpc_vol;
	axp22_usb_info.usb_pc_cur = axp22_config.pmu_usbpc_cur;
	axp22_usb_info.usb_ad_vol = axp22_config.pmu_ac_vol;
	axp22_usb_info.usb_ad_cur = axp22_config.pmu_ac_cur;
	axp22_batt_info.runtime_chgcur = axp22_config.pmu_runtime_chgcur;
	axp22_batt_info.suspend_chgcur = axp22_config.pmu_suspend_chgcur;
	axp22_batt_info.shutdown_chgcur = axp22_config.pmu_shutdown_chgcur;
	axp22_batt_info.bat_warning_level1 =
		axp22_config.pmu_battery_warning_level1;
	axp22_batt_info.bat_warning_level2 =
		axp22_config.pmu_battery_warning_level2;
	battery_data.voltage_max_design = axp22_config.pmu_init_chgvol
								* 1000;
	battery_data.voltage_min_design = axp22_config.pmu_pwroff_vol
								* 1000;
	battery_data.energy_full_design = axp22_config.pmu_battery_cap;

	axp22_charger_init(axp_dev);

	chg_dev = axp_power_supply_register(&pdev->dev, axp_dev,
					&battery_data, &axp22_spy_info);
	if (IS_ERR_OR_NULL(chg_dev))
		goto fail;

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;

		ret = axp_request_irq(axp_dev, irq,
				axp_charger_irq[i].isr, chg_dev);
		if (ret != 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
					axp_charger_irq[i].name, irq, ret);
			goto out_irq;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_charger_irq[i].name, irq, ret);

	}

	platform_set_drvdata(pdev, chg_dev);

	return 0;

out_irq:
	for (i = i - 1; i >= 0; i--) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}
fail:
	return -1;
}

static int axp22_charger_remove(struct platform_device *pdev)
{
	int i, irq;
	struct axp_charger_dev *chg_dev = platform_get_drvdata(pdev);
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	for (i = 0; i < ARRAY_SIZE(axp_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}

	axp_power_supply_unregister(chg_dev);

	return 0;
}

static int axp22_charger_suspend(struct platform_device *dev,
				pm_message_t state)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);

	axp_suspend_flag = AXP_WAS_SUSPEND;
	axp_charger_suspend(chg_dev);

	return 0;
}

static int axp22_charger_resume(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);
	struct axp_regmap *map = chg_dev->chip->regmap;
	int pre_rest_vol;

	if (axp_suspend_flag == AXP_SUSPEND_WITH_IRQ) {
		axp_suspend_flag = AXP_NOT_SUSPEND;
#ifdef CONFIG_AXP_NMI_USED
		enable_nmi();
#endif
	} else {
		axp_suspend_flag = AXP_NOT_SUSPEND;
	}

	pre_rest_vol = chg_dev->rest_vol;
	axp_charger_resume(chg_dev);

	if (chg_dev->rest_vol - pre_rest_vol) {
		pr_info("battery vol change: %d->%d\n",
				pre_rest_vol, chg_dev->rest_vol);
		axp_regmap_write(map, AXP22_DATA_BUFFER1,
				chg_dev->rest_vol | 0x80);
	}

	return 0;
}

static void axp22_charger_shutdown(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);
	axp_charger_shutdown(chg_dev);
}

static const struct of_device_id axp22_charger_dt_ids[] = {
	{ .compatible = "axp221s-charger", },
	{ .compatible = "axp227-charger", },
	{},
};
MODULE_DEVICE_TABLE(of, axp22_charger_dt_ids);

static struct platform_driver axp22_charger_driver = {
	.driver     = {
		.name   = "axp22-charger",
		.of_match_table = axp22_charger_dt_ids,
	},
	.probe    = axp22_charger_probe,
	.remove   = axp22_charger_remove,
	.suspend  = axp22_charger_suspend,
	.resume   = axp22_charger_resume,
	.shutdown = axp22_charger_shutdown,
};

static int __init axp22_charger_initcall(void)
{
	int ret;

	ret = platform_driver_register(&axp22_charger_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
fs_initcall_sync(axp22_charger_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_DESCRIPTION("Charger Driver for axp22x PMIC");
MODULE_ALIAS("platform:axp22-charger");
