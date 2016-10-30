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
#include <power/sunxi/power.h>
#include <asm/arch/intc.h>
#include <sunxi_bmp.h>
#include <sunxi_board.h>
#include <sys_config_old.h>
#include <lzma/LzmaTools.h>
#include <smc.h>
#include <asm/arch/platsmp.h>
#include <cputask.h>

volatile unsigned int secondary_cpu_work_busy = 1;
volatile unsigned int bmp_decode_ready = 0;
static sunxi_bmp_store_t bmp_info;
static uint  gp_value;
static unsigned char *ready_to_decode_buf;

DECLARE_GLOBAL_DATA_PTR;

#ifdef  CONFIG_SUNXI_DISPLAY
static int initr_sunxi_display(void)
{
	int workmode = uboot_spare_head.boot_data.work_mode;
	if(!((workmode == WORK_MODE_BOOT) ||
		(workmode == WORK_MODE_CARD_PRODUCT) ||
		(workmode == WORK_MODE_SPRITE_RECOVERY)))
	{
		return 0;
	}

	tick_printf("start\n");
	drv_disp_init();

#ifdef CONFIG_SUNXI_HDCP_IN_SECURESTORAGE
{
	int hdcpkey_enable=0;
	int ret = 0;
	ret = script_parser_fetch("hdmi_para", "hdmi_hdcp_enable", &hdcpkey_enable, 1);
	if((ret) || (hdcpkey_enable != 1))
	{
		board_display_device_open();
		board_display_layer_request();
	}
	//here: write key to hardware
	if(hdcpkey_enable==1)
	{
		char buffer[4096];
		int data_len;
		int ret0;

		memset(buffer, 0, 4096);
		ret0 = sunxi_secure_storage_init();
		if(ret0)
		{
			printf("sunxi init secure storage failed\n");
		}
		else
		{
			ret0 = sunxi_secure_storage_read("hdcpkey", buffer, 4096, &data_len);
			if(ret0)
			{
				printf("probe hdcp key failed\n");
			}
			else
			{
				ret0 = smc_aes_bssk_decrypt_to_keysram(buffer, data_len);
				if(ret0)
				{
					printf("push hdcp key failed\n");
				}
				else
				{
					board_display_device_open();
					board_display_layer_request();
				}
			}
		}
	}
}
#else

#ifndef CONFIG_BOOT_GUI
	board_display_device_open();
	board_display_layer_request();
#else
	disp_devices_open();
	fb_init();
#endif

#endif
	tick_printf("end\n");
	return 0;
}
#endif


//check battery and voltage
static int __battery_ratio_calucate( void)
{
	int  Ratio ;

	Ratio = axp_probe_rest_battery_capacity();
	if(Ratio < 1)
	{
		//some board coulombmeter value is not precise whit low capacity, so open it again here
		//note :in this case ,you should wait at least 1s berfore you read battery ratio again
		axp_set_coulombmeter_onoff(0);
		axp_set_coulombmeter_onoff(1);
	}
	return Ratio;
}


typedef enum __BOOT_POWER_STATE
{
	SUNXI_STATE_SHUTDOWN_DIRECTLY = 0,
	SUNXI_STATE_SHUTDOWN_CHARGE,
	SUNXI_STATE_ANDROID_CHARGE,
	SUNXI_STATE_NORMAL_BOOT
}SUNXI_BOOT_POWER_STATE_E;

//function : PowerCheck
//para: null
//note:  Decide whether to boot

static int sunxi_probe_power_state(void)
{
	int __bat_exist, __power_source;
	int __bat_vol, __bat_ratio;
	int __safe_vol, power_start = 0;
	int ret = 0, __power_on_cause;
	SUNXI_BOOT_POWER_STATE_E __boot_state;

//power_start
//0:  not allow boot by insert dcin: press power key in long time & pre state is system state(Battery ratio shouled enough)
//0: 关机状态下，插入外部电源时，电池电量充足时，不允许开机，会进入充电模式；电池电量不足，则关机
//1: allow boot directly  by insert dcin:( Battery ratio shouled enough)
//1: 关机状态下，插入外部电源，电池电量充足时，直接开机进入系统；电池电量不足，则关机
//2: not allow boot by insert dcin:press power key in long time & pre state is system state(do not check battery ratio)
//2: 关机状态下，插入外部电源时，不允许开机，会进入充电模式；无视电池电量
//3: allow boot directly  by insert dcin:( do not check battery ratio)
//3: 关机状态下，插入外部电源，直接开机进入系统；无视电池电量。

	script_parser_fetch("target", "power_start", &power_start, 1);
	//check battery
	__bat_exist = axp_probe_battery_exist();
	//check power source，vbus or dcin, if both，treat it as vbus
	__power_source = axp_probe_power_source();
	if (__power_source == AXP_VBUS_EXIST)
		printf("vbus exist\n");
	else
		printf("dcin exist\n");

	if (__bat_exist <= 0)
	{
		printf("no battery exist\n");
		//EnterNormalBootMode();
		return SUNXI_STATE_NORMAL_BOOT;
	}

	if (PMU_PRE_CHARGE_MODE == gd->pmu_saved_status) {
		if (__power_source) {
			printf("SUNXI_STATE_ANDROID_CHARGE");
			return SUNXI_STATE_ANDROID_CHARGE;
		} else {
			printf("SUNXI_STATE_SHUTDOWN_DIRECTLY");
			return SUNXI_STATE_SHUTDOWN_DIRECTLY;
		}
	}

	//check battery ratio and vol
	__bat_ratio = __battery_ratio_calucate();
	__bat_vol = axp_probe_battery_vol();
	printf("Battery Voltage=%d, Ratio=%d\n", __bat_vol, __bat_ratio);

	//probe the vol threshold
	ret = script_parser_fetch(PMU_SCRIPT_NAME, "pmu_safe_vol", &__safe_vol, 1);
	if ((ret) || (__safe_vol < 3000)) {
		__safe_vol = 3500;
	}

	//judge if boot or charge
	__power_on_cause = axp_probe_startup_cause();

	switch (power_start) {
		case 0:
			if (__bat_ratio < 1) {
				if (!__power_source) {
					__boot_state = SUNXI_STATE_SHUTDOWN_DIRECTLY;
					break;
				}

				if (__bat_vol < __safe_vol) {
					//低电量，低电压情况下，进入关机充电模式
					__boot_state = SUNXI_STATE_SHUTDOWN_CHARGE;
					break;
				}
			}

			if (__power_on_cause == AXP_POWER_ON_BY_POWER_KEY)
				__boot_state = SUNXI_STATE_NORMAL_BOOT;
			else
				__boot_state = SUNXI_STATE_ANDROID_CHARGE;

			break;
		case 1:
			if (__bat_ratio < 1) {
				if (!__power_source) {
					__boot_state = SUNXI_STATE_SHUTDOWN_DIRECTLY;
					break;
				}

				if (__bat_vol < __safe_vol) {
					//低电量，低电压情况下，进入关机充电模式
					__boot_state = SUNXI_STATE_SHUTDOWN_CHARGE;
					break;
				}
			}
			__boot_state = SUNXI_STATE_NORMAL_BOOT;
			break;
		case 2:
			//判断，如果是按键开机
			if (__power_on_cause == AXP_POWER_ON_BY_POWER_KEY)
				__boot_state = SUNXI_STATE_NORMAL_BOOT;
			else
				__boot_state = SUNXI_STATE_ANDROID_CHARGE;

			break;
		case 3:
		default:
			//直接开机进入系统，不进充电模式
			__boot_state = SUNXI_STATE_NORMAL_BOOT;
			break;
	}


	switch(__boot_state)
	{
		case SUNXI_STATE_SHUTDOWN_DIRECTLY:
			printf("STATE_SHUTDOWN_DIRECTLY\n");
			break;
		case SUNXI_STATE_SHUTDOWN_CHARGE:
			printf("STATE_SHUTDOWN_CHARGE\n");
			break;
		case SUNXI_STATE_ANDROID_CHARGE:
			printf("STATE_ANDROID_CHARGE\n");
			break;
		case SUNXI_STATE_NORMAL_BOOT:
		default:
			printf("STATE_NORMAL_BOOT\n");
			break;
	}
	return __boot_state;

}


static int sunxi_pmu_treatment(void)
{
	if (uboot_spare_head.boot_ext[0].data[0] <= 0) {
		printf("no pmu exist\n");

		return -1;
	}
	//设置工厂模式
	axp_probe_factory_mode();
	//设置充电限制电流，为vbus
	axp_set_charge_vol_limit();
    axp_set_all_limit();
    //设置开机门限电压
    axp_set_hardware_poweron_vol();
    //设置各路输出电压
    axp_set_power_supply_output();

    return 0;
}

int sunxi_bmp_decode_from_compress(unsigned char *dst_buf, unsigned char *src_buf)
{
	unsigned long dst_len, src_len;

	src_len = *(uint *)(src_buf - 16);
	return lzmaBuffToBuffDecompress(dst_buf, (SizeT *)&dst_len,
			(void *)src_buf, (SizeT)src_len);
}

extern int sunxi_usb_dev_register(uint dev_name);
extern void sunxi_usb_main_loop(int mode);

int sunxi_secendary_cpu_task(void)
{
	int next_mode;

	printf("secondary entry\n");

	sunxi_pmu_treatment();

	next_mode = sunxi_probe_power_state();

	if (next_mode == SUNXI_STATE_NORMAL_BOOT) {
		ready_to_decode_buf = (unsigned char *)(SUNXI_LOGO_COMPRESSED_LOGO_BUFF);
	} else if (next_mode == SUNXI_STATE_SHUTDOWN_CHARGE) {
		gd->need_shutdown = 1;
		ready_to_decode_buf = (unsigned char *)(SUNXI_SHUTDOWN_CHARGE_COMPRESSED_LOGO_BUFF);
	} else if (next_mode == SUNXI_STATE_ANDROID_CHARGE) {
		gd->chargemode = 1;
		ready_to_decode_buf = (unsigned char *)(SUNXI_ANDROID_CHARGE_COMPRESSED_LOGO_BUFF);
	} else if (next_mode == SUNXI_STATE_SHUTDOWN_DIRECTLY) {
		gd->need_shutdown = 1;
		goto __secondary_cpu_end;
	}
printf("on cpu2\n");
	arm_svc_set_cpu_on(2, (uint)third_cpu_start);
printf("on end\n");
	enable_interrupts();
	sunxi_gic_cpu_interface_init(get_core_pos());

	initr_sunxi_display();
	while(!bmp_decode_ready);

	if (bmp_decode_ready == 1) {
		sunxi_bmp_dipslay_screen(bmp_info);
		printf("boot logo display ok\n");
	}
	board_display_wait_lcd_open();		//add by jerry
	board_display_set_exit_mode(1);

	secondary_cpu_work_busy = 0;
__secondary_cpu_end:
	printf("kill cpu %d\n", get_core_pos());

	disable_interrupts();
	sunxi_gic_cpu_interface_exit();

	cleanup_before_powerdown();

	arm_svc_set_cpu_wfi();

	return 0;
}

int sunxi_third_cpu_task(void)
{
	int ret;
	unsigned char *buf;

	printf("third entry\n");

	if (ready_to_decode_buf == NULL) {
		bmp_decode_ready = -1;
		goto __third_cpu_end;
	}

	buf = (unsigned char *)(SUNXI_DISPLAY_FRAME_BUFFER_ADDR - SUNXI_DISPLAY_FRAME_BUFFER_SIZE);

	ret = sunxi_bmp_decode_from_compress(buf, ready_to_decode_buf);
	if(ret) {
		printf("bmp lzma decode err\n");
		bmp_decode_ready = -2;

		goto __third_cpu_end;
	}

	ret = sunxi_bmp_display_mem(buf, &bmp_info);
	if (ret) {
		printf("bmp decode err\n");
		bmp_decode_ready = -3;

		goto __third_cpu_end;
	}

	bmp_decode_ready = 1;
__third_cpu_end:
	printf("kill cpu %d\n", get_core_pos());

	disable_interrupts();
	sunxi_gic_cpu_interface_exit();

	cleanup_before_powerdown();

	arm_svc_set_cpu_wfi();

	return 0;
}

int sunxi_secondary_cpu_poweroff(void)
{
	int count = 0;

	count = 0;
	while (sunxi_probe_wfi_mode(2)) {
		__msdelay(1);
		count ++;
		printf("wait cpu2...delay=%d\n", count);
		if(count > 1000) {
			printf("cpu2 run bad\n");

			break;
		}
	}
printf("ready to disbale cpu2\n");
	sunxi_disable_cpu(2);
	while(sunxi_probe_cpu_power_status(2));
printf("disbale cpu2\n");
	count = 0;
	while (sunxi_probe_wfi_mode(1)) {
		__msdelay(1);
		count ++;
		printf("wait cpu1...delay=%d\n", count);
		if(count > 1000) {
			printf("cpu1 run bad\n");

			break;
		}
	}
printf("ready to disbale cpu1\n");
	sunxi_disable_cpu(1);
	while(sunxi_probe_cpu_power_status(1));
printf("disbale cpu1\n");
	sunxi_restore_gp_status();

	printf("smp=0x%x\n", readl(0x01700000 + 0x30));
	printf("cpu1 power=0x%x\n", readl(0x01f01400 + 0x144));
	printf("cpu2 power=0x%x\n", readl(0x01f01400 + 0x148));

	return 0;
}

void sunxi_store_gp_status(void)
{
	volatile uint flag;

	gp_value = readl(SUNXI_RTC_BASE + 0x108);
	do
	{
		writel(0, SUNXI_RTC_BASE + 0x108);
		__usdelay(10);
		asm volatile("ISB SY");
		asm volatile("DMB SY");
		flag  = readl(SUNXI_RTC_BASE + 0x108);
	}
	while(flag != 0);
}

void sunxi_set_gp_status(void)
{
	volatile uint flag;

	do
	{
		writel(0x55, SUNXI_RTC_BASE + 0x108);
		__usdelay(10);
		asm volatile("ISB SY");
		asm volatile("DMB SY");
		flag  = readl(SUNXI_RTC_BASE + 0x108);
	}
	while(flag != 0x55);
}

void sunxi_restore_gp_status(void)
{
	volatile uint flag;

	do
	{
		writel(gp_value, SUNXI_RTC_BASE + 0x108);
		__usdelay(10);
		asm volatile("ISB SY");
		asm volatile("DMB SY");
		flag  = readl(SUNXI_RTC_BASE + 0x108);
	}
	while(flag != gp_value);
}
