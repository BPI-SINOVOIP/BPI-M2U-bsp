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
#include <asm/io.h>
#include <asm/arch/key.h>
#include <asm/arch/sys_proto.h>
#include <sys_config.h>
#ifdef BPI
#else
#include <power/sunxi/axp22_reg.h>
#include <power/sunxi/axp.h>
#endif
#include <power/sunxi/pmu.h>
#include <fdt_support.h>


__attribute__((section(".data")))
static uint32_t keyen_flag = 1;
#ifdef BPI
#else
static uint32_t version_flag = 0;
#endif

__weak int sunxi_key_clock_open(void)
{
	return 0;
}

__weak int sunxi_key_clock_close(void)
{
	return 0;
}

int sunxi_key_init(void)
{
	struct sunxi_lradc *sunxi_key_base = (struct sunxi_lradc *)SUNXI_KEYADC_BASE;
	uint reg_val = 0;
	int nodeoffset;

	sunxi_key_clock_open();

	reg_val = sunxi_key_base->ctrl;
	reg_val &= ~((7<<1) | (0xffU << 24));
	reg_val |=  LRADC_HOLD_EN;
	reg_val |=  LRADC_EN;
	sunxi_key_base->ctrl = reg_val;

	/* disable all key irq */
	sunxi_key_base->intc = 0;
	sunxi_key_base->ints = 0x1f1f;

	nodeoffset = fdt_path_offset(working_fdt,FDT_PATH_KEY_DETECT);
	if(nodeoffset > 0)
	{
		fdt_getprop_u32(working_fdt,nodeoffset,"keyen_flag",&keyen_flag);
#ifdef BPI
#else
		fdt_getprop_u32(working_fdt,nodeoffset,"version_flag",&version_flag);
#endif
	}

	return 0;
}

int sunxi_key_exit(void)
{
	struct sunxi_lradc *sunxi_key_base = (struct sunxi_lradc *)SUNXI_KEYADC_BASE;

	sunxi_key_base->ctrl = 0;
	/* disable all key irq */
	sunxi_key_base->intc = 0;
	sunxi_key_base->ints = 0x1f1f;

	sunxi_key_clock_close();

	return 0;
}


int sunxi_key_read(void)
{
	u32 ints;
	int key = -1;
	struct sunxi_lradc *sunxi_key_base = (struct sunxi_lradc *)SUNXI_KEYADC_BASE;

	if(!keyen_flag)
	{
		return -1;
	}
	ints = sunxi_key_base->ints;
	/* clear the pending data */
	sunxi_key_base->ints |= (ints & 0x1f);
	/* if there is already data pending,
	 read it */
	if( ints & ADC0_KEYDOWN_PENDING)
	{
		if(ints & ADC0_DATA_PENDING)
		{
			key = sunxi_key_base->data0 & 0x3f;
			if(!key)
			{
				key = -1;
			}
		}
	}
	else if(ints & ADC0_DATA_PENDING)
	{
		key = sunxi_key_base->data0 & 0x3f;
		if(!key)
		{
			key = -1;
		}
	}

	if(key > 0)
	{
#ifdef BPI
#else
		if(version_flag) {
			if(key>=(BPI_M2_BERRY_KEY - 2) && key<=(BPI_M2_BERRY_KEY + 2)) // BPI-M2 Berry 1.0 [0x2e]
				return -1;
		}
#endif
		printf("key pressed value=0x%x\n", key);
	}


	return key;
}

#ifdef BPI
#else
int do_axp_dump(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i;
	uchar reg_value;
	for(i=0;i<=0xe6;i++)
	{
		axp_i2c_read(AXP22_ADDR, i,&reg_value);
		printf("[AXP22X]:[%02X]=[%02X]\n", i , reg_value);
	}
	return 0;
}

int bpi_board_version(void)
{
	u32 ints;
	int key = 64;
	struct sunxi_lradc *sunxi_key_base = (struct sunxi_lradc *)SUNXI_KEYADC_BASE;
	ints = sunxi_key_base->ints;
	/* clear the pending data */
	sunxi_key_base->ints |= (ints & 0x1f);
	/* if there is already data pending,
	 read it */
	if( ints & ADC0_KEYDOWN_PENDING)
	{
		if(ints & ADC0_DATA_PENDING)
		{
			key = sunxi_key_base->data0 & 0x3f;
		}
	}
	else if(ints & ADC0_DATA_PENDING)
	{
		key = sunxi_key_base->data0 & 0x3f;
	}
	if(version_flag) {
		if(key>=(BPI_M2_BERRY_KEY - 2) && key<=(BPI_M2_BERRY_KEY + 2)) { // BPI-M2 Berry 1.0 [0x2e]
			printf("BPI: BPI-M2 Berry 1.0 \n");
			uboot_spare_head.boot_data.reserved[0] = BPI_M2_BERRY_ID;
		}
		else {
			printf("BPI: BPI-M2 Ultra\n");
			uboot_spare_head.boot_data.reserved[0] = BPI_M2_ULTRA_ID;
		}
	}
	return key;
}
#endif

int do_key_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct sunxi_lradc *sunxi_key_base = (struct sunxi_lradc *)SUNXI_KEYADC_BASE;
	u32 power_key = 0;
	int nodeoffset;

	nodeoffset = fdt_path_offset(working_fdt,FDT_PATH_KEY_DETECT);
	if(nodeoffset > 0)
	{
		fdt_getprop_u32(working_fdt,nodeoffset,"keyen_flag",&keyen_flag);
#ifdef BPI
#else
		fdt_getprop_u32(working_fdt,nodeoffset,"version_flag",&version_flag);
#endif
	}
	if(!keyen_flag)
	{
		puts("warnning: not support,please set keyen_flag=1 in sys_config.fex\n");
		return -1;
	}
	puts(" press a key:\n");
#ifdef BPI
#else
	printf("BPI: version_flag %d \n", version_flag);
#endif
	sunxi_key_base->ints = 0x1f1f;
	while(!ctrlc())
	{
		sunxi_key_read();
		power_key = axp_probe_key();
		if(power_key > 0) {
			break;
		}
	}

	return 0;

}

U_BOOT_CMD(
	key_test, 1, 0,	do_key_test,
	"Test the key value\n",
	""
);

#ifdef BPI
#else
U_BOOT_CMD(
	axp_dump, 1, 0,	do_axp_dump,
	"Dump the AXP value\n",
	""
);
#endif

