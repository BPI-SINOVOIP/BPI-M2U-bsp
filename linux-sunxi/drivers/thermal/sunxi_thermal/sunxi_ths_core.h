/*
 * drivers/thermal/sunxi_ths_core.h
 *
 * Copyright (C) 2013-2024 allwinner.
 *	JiaRui Xiao<xiaojiarui@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SUNXI_THS_CORE_H__
#define __SUNXI_THS_CORE_H__

/* GET TMEP RETURN VALUE */
#define NO_DATA_INTTERUPT	(-41)
#define OUT_THE_THERMAL_LIMIT	(-42)
#define WRONG_SENSOR_ID		(-43)

#define MAX_SENSOR_NUM		(5)
#define REG_HAVE_SHUT_TEMP_NUM	(2)
#define THERMAL_ATTR_LENGTH	(25)
#define THERMAL_DATA_REG_MAX	0xfff

#define thsprintk(fmt, arg...) \
	pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)

#define to_thermal_sensor_device(x)  \
		container_of((x), struct sunxi_ths_data, pdev)

#define SETMASK(width, shift)  ((width ? ((-1U) >> (32-width)) : 0)  << (shift))
#define CLRMASK(width, shift)  (~(SETMASK(width, shift)))
#define GET_BITS(shift, width, reg)     \
		(((reg) & SETMASK(width, shift)) >> (shift))
#define SET_BITS(shift, width, reg, val) \
		(((reg) & CLRMASK(width, shift)) | (val << (shift)))

struct ths_info_attr {
	struct device_attribute attr;
	char name[THERMAL_ATTR_LENGTH];
};

struct sunxi_ths_data {
	void __iomem *base_addr;
	struct mutex ths_mutex_lock;
	struct platform_device *pdev;
	struct sunxi_ths_controller *ctrl;
	struct thermal_sensor_coefficent *ths_coefficent;
	struct clk *mclk;
	struct clk *pclk;
	u32 ths_driver_version;
	bool parent_clk;
	int irq;
	int shut_temp;
	int sensor_cnt;
	int combine_cnt;
	int alarm_low_temp;
	int alarm_high_temp;
	int alarm_temp_hysteresis;
	int cp_ft_flag; /* tell us what calibration they used. 00 cp, 01 ft */
	void *data; /* specific sensor data */
	struct ths_info_attr *ths_info_attrs;
	struct sunxi_ths_sensor **comb_sensor;
};

/**
 * to recored the single sensor data about name and temp
 */
struct thermal_sensor_info {
	int id;
	char *ths_name;
	int temp;
	int alarm_irq_type;
	bool irq_enabled;
};

/*
 *	@NO_INIT: this thermal reg doesn't need to init.
 *	@NORMAL_REG: this thermal reg just write the init value.
 *	@ENABLE_REG: this thermal reg used to enable the thermal sensor.
 *	@CDATA_OLD_REG: this thermal reg use old method to calibration thermal
 *	sensor.
 *	@CDATA_NEW_REG: this thermal reg use new method to calibration thermal
 *	sensor.
 *	@INT_STA_REG: this thermal reg show the thermal sensor status
 *	SHT_TMP_REG:this thermal reg will compara the TDATA_REG about temp, and
 *	if the TDATA_REG temp big than this reg will do interrput if you ennale.
 *	@TDATA_REG:this thermal reg show the thermal sensor calcular temp.
 *	};
 */
enum reg_type {
	NO_INIT = 0,
	NORMAL_REG,
	ENABLE_REG,
	CDATA_REG,
	INT_STA_REG,
	SHT_TMP_REG,
	TDATA_REG
};

struct thermal_reg {
	char name[32];
	u32 offset;
	u32 value;
	u32 init_type;
};

struct normal_temp_para {
	u32 MUL_PARA;
	u32 DIV_PARA;
	u32 MINU_PARA;
};

struct split_temp_point {
	long split_temp;
	u32 split_reg;
};

/**
 * temperature = ( MINUPA - reg * MULPA) / DIVPA
 */
struct temp_calculate_coefficent {
	struct split_temp_point down_limit;
	struct split_temp_point up_limit;
	struct normal_temp_para nt_para[MAX_SENSOR_NUM];
};

/**
 * this struct include all of the sensor driver init para
 */
struct thermal_sensor_coefficent {
	struct temp_calculate_coefficent *calcular_para;
	struct thermal_reg *reg_para;
};

extern int ths_driver_init_reg(struct sunxi_ths_data *,
			    struct thermal_sensor_coefficent *);
extern void ths_driver_reg_debug(struct sunxi_ths_data *,
				     struct thermal_sensor_coefficent *);
extern int ths_driver_get_temp(struct sunxi_ths_data *,
			     int);
extern int ths_driver_startup(struct sunxi_ths_data *,
			     struct device *);
extern void ths_driver_disable_reg(struct sunxi_ths_data *);
extern void ths_driver_clk_cfg(struct sunxi_ths_data *);
extern void ths_driver_clk_uncfg(struct sunxi_ths_data *);
extern void ths_drvier_remove_trip_attrs(struct sunxi_ths_data *);
extern int ths_driver_create_sensor_info_attrs(struct sunxi_ths_data *,
			     struct thermal_sensor_info *);
extern u32 ths_driver_temp_to_reg(int temp, u16 id,
			struct temp_calculate_coefficent *para);

#endif
