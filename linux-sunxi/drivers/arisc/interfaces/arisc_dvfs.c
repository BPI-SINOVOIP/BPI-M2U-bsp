/*
 *  drivers/arisc/interfaces/arisc_dvfs.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../arisc_i.h"
#if defined CONFIG_SUNXI_ARISC_COM_DIRECTLY
#if defined CONFIG_ARCH_SUN8IW6P1

typedef struct arisc_freq_voltage {
	u32 freq;
	u32 voltage;
	u32 axi_div;
} arisc_freq_voltage_t;

static struct arisc_freq_voltage arisc_vf_table[2][ARISC_DVFS_VF_TABLE_MAX] = {
	/*
	 * cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	 */
	{
		/* freq         voltage     axi_div */
		{900000000,     1200,       3},
		{600000000,     1200,       3},
		{420000000,     1200,       3},
		{360000000,     1200,       3},
		{300000000,     1200,       3},
		{240000000,     1200,       3},
		{120000000,     1200,       3},
		{60000000,      1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
	},

	/*
	 * cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]
	 * cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]
	 */
	{
		/* freq         voltage     axi_div */
		{900000000,     1200,       3},
		{600000000,     1200,       3},
		{420000000,     1200,       3},
		{360000000,     1200,       3},
		{300000000,     1200,       3},
		{240000000,     1200,       3},
		{120000000,     1200,       3},
		{60000000,      1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
		{0,             1200,       3},
	},
};

int arisc_dvfs_cfg_vf_table(unsigned int cluster, unsigned int vf_num,
				void *vf_tbl)
{
	int cluster = 0;
	int index = 0;
	struct device_node *dvfs_main_np;
	struct device_node *dvfs_sub_np;
	unsigned int table_count;
	char cluster_name[2] = {'L', 'B'};
	int vf_table_type = 0;
	int vf_table_size[2] = {0, 0};
	char vf_table_main_key[64];
	char vf_table_sub_key[64];
	struct arisc_message *pmessage;

	dvfs_main_np = of_find_node_by_path("/dvfs_table");
	if (!dvfs_main_np) {
		ARISC_ERR("No dvfs table node found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dvfs_main_np, "vf_table_count",
				&table_count)) {
		ARISC_ERR("get vf_table_count failed\n");
		return -EINVAL;
	}

	if (table_count == 1) {
		ARISC_LOG("%s: support only one vf_table\n", __func__);
		vf_table_type = 0;
	} else
		vf_table_type = sunxi_get_soc_bin();

	sprintf(vf_table_main_key, "%s%d", "allwinner,vf_table",
			vf_table_type);

	ARISC_INF("%s: vf table type [%d=%s]\n", __func__, vf_table_type,
			vf_table_main_key);

	/* parse system config v-f table information */
	for (cluster = 0; cluster < 1; cluster++) {
		sprintf(vf_table_sub_key, "%c_LV_count",
				cluster_name[cluster]);
		dvfs_sub_np = of_find_compatible_node(dvfs_main_np, NULL,
				vf_table_main_key);
		if (!dvfs_sub_np) {
			ARISC_ERR("No %s node found\n", vf_table_main_key);
			return -ENODEV;
		}

		if (of_property_read_u32(dvfs_sub_np, vf_table_sub_key,
					&vf_table_size[cluster])) {
			ARISC_ERR("get %s failed\n", vf_table_sub_key);
			return -EINVAL;
		}

		for (index = 0; index < vf_table_size[cluster]; index++) {
			sprintf(vf_table_sub_key, "%c_LV%d_freq",
					cluster_name[cluster], index + 1);
			if (of_property_read_u32(dvfs_sub_np, vf_table_sub_key,
				&arisc_vf_table[cluster][index].freq)) {
				ARISC_ERR("get %s failed\n", vf_table_sub_key);
				return -EINVAL;
			}

			ARISC_INF("%s: freq [%s-%d=%d]\n", __func__,
				vf_table_sub_key, index,
				arisc_vf_table[cluster][index].freq);

			sprintf(vf_table_sub_key, "%c_LV%d_volt",
				cluster_name[cluster], index + 1);
			if (of_property_read_u32(dvfs_sub_np, vf_table_sub_key,
				&arisc_vf_table[cluster][index].voltage)) {
				ARISC_ERR("get %s failed\n", vf_table_sub_key);
				return -EINVAL;
			}

			ARISC_INF("%s: volt [%s-%d=%d]\n", __func__,
				vf_table_sub_key, index,
				arisc_vf_table[cluster][index].voltage);
		}
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	for (cluster = 0; cluster < 1; cluster++) {
		for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
			/* initialize message
			 *
			 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
			 * | index  |  freq  |voltage |axi_div |  pll   |
			 */
			pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
			pmessage->paras[0]   = index;
			pmessage->paras[1]   =
				arisc_vf_table[cluster][index].freq;
			pmessage->paras[2]   =
				arisc_vf_table[cluster][index].voltage;
			pmessage->paras[3]   =
				arisc_vf_table[cluster][index].axi_div;
			pmessage->paras[4]   = cluster;
			pmessage->state      = ARISC_MESSAGE_INITIALIZED;
			pmessage->cb.handler = NULL;
			pmessage->cb.arg     = NULL;

			ARISC_INF("v-f table: cluster %d index %d freq %d vol %d axi_div %d\n",
				pmessage->paras[4], pmessage->paras[0],
				pmessage->paras[1], pmessage->paras[2],
				pmessage->paras[3]);

			/* send request message */
			arisc_hwmsgbox_send_message(pmessage,
					ARISC_SEND_MSG_TIMEOUT);

			/* check config fail or not */
			if (pmessage->result) {
				ARISC_WRN("config dvfs v-f table [%d][%d] fail\n",
					cluster, index);
				return -EINVAL;
			}
		}
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return 0;
}
#elif defined CONFIG_ARCH_SUN8IW5P1

typedef struct arisc_freq_voltage {
	u32 freq;
	u32 voltage;
	u32 axi_div;
} arisc_freq_voltage_t;

static struct arisc_freq_voltage arisc_vf_table[ARISC_DVFS_VF_TABLE_MAX] = {
	/*freq          voltage   axi_div*/
	{900000000,     1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (600Mhz, 1008Mhz]*/
	{600000000,     1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (420Mhz, 600Mhz]*/
	{420000000,     1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (360Mhz, 420Mhz]*/
	{360000000,     1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (300Mhz, 360Mhz]*/
	{300000000,     1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (240Mhz, 300Mhz]*/
	{240000000,     1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (120Mhz, 240Mhz]*/
	{120000000,     1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (60Mhz,  120Mhz]*/
	{60000000,      1200,       3}, /*cpu0 vdd is 1.20v if cpu freq is (0Mhz,   60Mhz]*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
	{0,             1200,       3}, /*end of cpu dvfs table*/
};

int arisc_dvfs_cfg_vf_table(unsigned int cluster, unsigned vf_num, void *vf_tbl)
{
	int index = 0;
	struct device_node *dvfs_main_np;
	unsigned int table_count;
	int vf_table_size = 0;
	char vf_table_main_key[64];
	struct arisc_message *pmessage;

	dvfs_main_np = of_find_node_by_path("/dvfs_table");
	if (!dvfs_main_np) {
		ARISC_ERR("No dvfs table node found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dvfs_main_np, "vf_table_count", &table_count)) {
		ARISC_ERR("get vf_table_count failed\n");
		return -EINVAL;
	}

	/* parse system config v-f table information */

		if (of_property_read_u32(dvfs_main_np, "LV_count", &vf_table_size)) {
			ARISC_ERR("get LV_count failed\n");
			return -EINVAL;
		}
		for (index = 0; index < vf_table_size; index++) {
			sprintf(vf_table_main_key, "LV%d_freq", index + 1);
			if (of_property_read_u32(dvfs_main_np, vf_table_main_key, &arisc_vf_table[index].freq)) {
				ARISC_ERR("get %s failed\n", vf_table_main_key);
				return -EINVAL;
			}

			ARISC_INF("%s: freq [%s-%d=%d]\n", __func__, vf_table_main_key, index, arisc_vf_table[index].freq);

			sprintf(vf_table_main_key, "LV%d_volt", index + 1);
			if (of_property_read_u32(dvfs_main_np, vf_table_main_key, &arisc_vf_table[index].voltage)) {
				ARISC_ERR("get %s failed\n", vf_table_main_key);
				return -EINVAL;
			}

			ARISC_INF("%s: volt [%s-%d=%d]\n", __func__, vf_table_main_key, index, arisc_vf_table[index].voltage);
		}
	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

		for (index = 0; index < ARISC_DVFS_VF_TABLE_MAX; index++) {
			/* initialize message
			 *
			 * |paras[0]|paras[1]|paras[2]|paras[3]|paras[4]|
			 * | index  |  freq  |voltage |axi_div |  pll   |
			 */
			pmessage->type       = ARISC_CPUX_DVFS_CFG_VF_REQ;
			pmessage->paras[0]   = index;
			pmessage->paras[1]   = arisc_vf_table[index].freq;
			pmessage->paras[2]   = arisc_vf_table[index].voltage;
			pmessage->paras[3]   = arisc_vf_table[index].axi_div;
			pmessage->state      = ARISC_MESSAGE_INITIALIZED;
			pmessage->cb.handler = NULL;
			pmessage->cb.arg     = NULL;

			ARISC_INF("v-f table:  index %d freq %d vol %d axi_div %d\n",
				pmessage->paras[0], pmessage->paras[1], pmessage->paras[2],
				pmessage->paras[3]);

			/* send request message */
			arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

			/* check config fail or not */
			if (pmessage->result) {
				ARISC_WRN("config dvfs v-f table [%d] fail\n", index);
				return -EINVAL;
			}
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return 0;
}
#endif
/*
 * set specific pll target frequency.
 * @freq:    target frequency to be set, based on KHZ;
 * @pll:     which pll will be set
 * @mode:    the attribute of message, whether syn or asyn;
 * @cb:      callback handler;
 * @cb_arg:  callback handler arguments;
 *
 * return: result, 0 - set frequency successed,
 *                !0 - set frequency failed;
 */
int arisc_dvfs_set_cpufreq(unsigned int freq, unsigned int pll, unsigned int mode, arisc_cb_t cb, void *cb_arg)
{
	unsigned int          msg_attr = 0;
	struct arisc_message *pmessage;
	int                   result = 0;

	if (mode & ARISC_DVFS_SYN)
		msg_attr |= ARISC_MESSAGE_ATTR_HARDSYN;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(msg_attr);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message
	 *
	 * |paras[0]|paras[1]|
	 * |freq    |pll     |
	 */
	pmessage->type       = ARISC_CPUX_DVFS_REQ;
	pmessage->paras[0]   = freq;
	pmessage->paras[1]   = pll;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = cb;
	pmessage->cb.arg     = cb_arg;

	ARISC_INF("arisc dvfs request : %d\n", freq);
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* dvfs mode : syn or not */
	if (mode & ARISC_DVFS_SYN) {
		result = pmessage->result;
		arisc_message_free(pmessage);
	}

	return result;
}
EXPORT_SYMBOL(arisc_dvfs_set_cpufreq);
#else
/*
 * set specific pll target frequency.
 * @freq:    target frequency to be set, based on KHZ;
 * @pll:     which pll will be set
 * @mode:    the attribute of message, whether syn or asyn;
 * @cb:      callback handler;
 * @cb_arg:  callback handler arguments;
 *
 * return: result, 0 - set frequency successed,
 *                !0 - set frequency failed;
 */
int arisc_dvfs_set_cpufreq(unsigned int freq, unsigned int pll, unsigned int mode, arisc_cb_t cb, void *cb_arg)
{
	int                   ret = 0;

	ARISC_INF("arisc dvfs request : %d\n", freq);
	ret = invoke_scp_fn_smc(ARM_SVC_ARISC_CPUX_DVFS_REQ, (u64)freq, (u64)pll, (u64)mode);

	if (ret) {
		if (cb == NULL) {
			ARISC_WRN("callback not install\n");
		} else {
			/* call callback function */
			ARISC_WRN("call the callback function\n");
			(*(cb))(cb_arg);
		}
	}

	return ret;
}
EXPORT_SYMBOL(arisc_dvfs_set_cpufreq);

int arisc_dvfs_cfg_vf_table(unsigned int cluster, unsigned int vf_num,
				void *vf_tbl)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_ARISC_CPUX_DVFS_CFG_VF_REQ, cluster,
			vf_num, (u64)vf_tbl);

	return 0;
}
#endif
