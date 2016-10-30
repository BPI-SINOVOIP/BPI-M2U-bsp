/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <power/sunxi/pmu.h>
#include <power/sunxi/axp.h>



extern int axpnull_set_supply_status(int vol_name, int vol_value, int onoff);
extern int axpnull_set_supply_status_byname(char *vol_name, int vol_value, int onoff);
extern int axpnull_probe_supply_status(int vol_name, int vol_value, int onoff);
extern int axpnull_probe_supply_status_byname(char *vol_name);
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_coulombmeter_onoff(int onoff)
{
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_charge_control(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_battery_ratio(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_power_status(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_battery_exist(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_battery_vol(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_key(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_pre_sys_mode(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_next_sys_mode(int data)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_this_poweron_cause(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_power_off(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_power_onoff_vol(int set_vol, int stage)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_charge_current(int current)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_charge_current(void)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_vbus_cur_limit(int current)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_vbus_vol_limit(int vol)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
    return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_int_pending(uchar *addr)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_probe_int_enable(uchar *addr)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axpnull_set_int_enable(uchar *addr)
{
	//printf("%s %d %s\n", __FILE__, __LINE__, __func__);
	return 0;
}

int axpnull_probe_vbus_cur_limit(void)
{
	return 0;	
}

sunxi_axp_module_init("axpnull", SUNXI_AXP_NULL);

#if 0
int __attribute__((weak))  axp_set_next_poweron_status(int value)
{
	return 0;
}
int __attribute__((weak)) axp_probe_pre_sys_mode(void)
{
	return 0;
}
int __attribute__((weak)) axp_set_supply_status_byname(char *pmu_type, char *vol_type, int vol_value, int onoff)
{
	return 0;
}
#endif

int __attribute__((weak)) PowerCheck(void)
{
	return 0;
}

int __attribute__((weak)) power_limit_init(void)
{
	return 0;
}






