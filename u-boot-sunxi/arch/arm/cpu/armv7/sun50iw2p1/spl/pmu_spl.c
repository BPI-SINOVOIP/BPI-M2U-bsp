/*
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* This code should work for both the S3C2400 and the S3C2410
 * as they seem to have the same I2C controller inside.
 * The different address mapping is handled by the s3c24xx.h files below.
 */

#include <common.h>
#include <asm/arch/base_pmu.h>
#include <power/sunxi/axp806_reg.h>

int pmu_type;

#define dbg(format,arg...)	printf(format,##arg)


static int axp_probe(void)
{
	u8  pmu_type;

	if(axp_i2c_read(AXP806_ADDR, 0x3, &pmu_type))
	{
		printf("axp read error\n");

		return -1;
	}
	pmu_type &= 0xCF;
	if(pmu_type == 0x41)
	{
		/* pmu type AXP806 */
		printf("PMU: AXP806\n");
		return AXP806_ADDR;
	}
	else
	{
		printf("unknow PMU\n");
		return -1;
	}

}

static int axp806_set_dcdca(int vol)
{
	return 0;
}

static int axp806_set_dcdcd(int vol)
{
	int ret = -1;
	unsigned char  vol_set;

	if(vol>=1600)
	{
		vol_set = (vol - 1500) / 100;
		vol_set +=45 ;
	}
	else
	{
		vol_set=(vol - 600)/20;
	}

	ret = axp_i2c_write(AXP806_ADDR, BOOT_POWER806_DCDOUT_VOL,vol_set);
	printf("PMU:Set DDR Vol %dV %s.\n", vol,ret==0?"OK":"Fail");

	return ret;
}


int set_ddr_voltage(int set_vol)
{
	if(pmu_type == AXP806_ADDR)
	{
		return axp806_set_dcdcd(set_vol);
	}

	return -1;
}

int set_pll_voltage(int set_vol)
{
	if(pmu_type == AXP806_ADDR)
	{
		return axp806_set_dcdca(set_vol);
	}

	return -1;
}

int pmu_init(void)
{
	i2c_init_cpus(CONFIG_SYS_I2C_SPEED, AXP806_ADDR);

	pmu_type = axp_probe();

	return pmu_type;
}

