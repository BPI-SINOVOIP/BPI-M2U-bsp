#include <common.h>
#include <spare_head.h>
#include <sunxi_mbr.h>
#include <boot_type.h>
#include <sys_partition.h>
#include <sys_config.h>
#include <sunxi_board.h>
#include <fastboot.h>
#include <android_misc.h>
#include <power/sunxi/pmu.h>
#include "debug_mode.h"
#include "sunxi_string.h"
#include "sunxi_serial.h"
#include <fdt_support.h>
#include <sys_config_old.h>
#include <fdt_acc.h>
#include <arisc.h>
#include <sunxi_nand.h>
#include <mmc.h>
#include <asm/arch/dram.h>
#include <cputask.h>


DECLARE_GLOBAL_DATA_PTR;

#define PARTITION_SETS_MAX_SIZE	 1024




void sunxi_update_subsequent_processing(int next_work)
{
	printf("next work %d\n", next_work);
	switch(next_work)
	{
		case SUNXI_UPDATE_NEXT_ACTION_REBOOT:
		case SUNXI_UPDATA_NEXT_ACTION_SPRITE_TEST:
			printf("SUNXI_UPDATE_NEXT_ACTION_REBOOT\n");
			sunxi_board_restart(0);
			break;

		case SUNXI_UPDATE_NEXT_ACTION_SHUTDOWN:
			printf("SUNXI_UPDATE_NEXT_ACTION_SHUTDOWN\n");
			sunxi_board_shutdown();
			break;

		case SUNXI_UPDATE_NEXT_ACTION_REUPDATE:
			printf("SUNXI_UPDATE_NEXT_ACTION_REUPDATE\n");
			sunxi_board_run_fel();
			break;

		case SUNXI_UPDATE_NEXT_ACTION_BOOT:
		case SUNXI_UPDATE_NEXT_ACTION_NORMAL:
		default:
			printf("SUNXI_UPDATE_NEXT_ACTION_NULL\n");
			break;
	}

	return ;
}


void sunxi_fastboot_init(void)
{
	struct fastboot_ptentry fb_part;
	int index, part_total;
	char partition_sets[PARTITION_SETS_MAX_SIZE];
	char part_name[16];
	char *partition_index = partition_sets;
	int offset = 0;
	int temp_offset = 0;
	int storage_type = uboot_spare_head.boot_data.storage_type;

//	printf("--------fastboot partitions--------\n");
	part_total = sunxi_partition_get_total_num();
	if((part_total <= 0) || (part_total > SUNXI_MBR_MAX_PART_COUNT))
	{
		printf("mbr not exist\n");

		return ;
	}

//	printf("-total partitions:%d-\n", part_total);
//	printf("%-12s  %-12s  %-12s\n", "-name-", "-start-", "-size-");

	memset(partition_sets, 0, PARTITION_SETS_MAX_SIZE);

	for(index = 0; index < part_total && index < SUNXI_MBR_MAX_PART_COUNT; index++)
	{
		sunxi_partition_get_name(index, &fb_part.name[0]);
		fb_part.start = sunxi_partition_get_offset(index) * 512;
		fb_part.length = sunxi_partition_get_size(index) * 512;
		fb_part.flags = 0;
//		printf("%-12s: %-12x  %-12x\n", fb_part.name, fb_part.start, fb_part.length);

		memset(part_name, 0, 16);
		if(!storage_type)
		{
			sprintf(part_name, "nand%c", 'a' + index);
		}
		else
		{
			if(index == 0)
			{
				strcpy(part_name, "mmcblk0p2");
			}
			else if( (index+1)==part_total)
			{
				strcpy(part_name, "mmcblk0p1");
			}
			else
			{
				sprintf(part_name, "mmcblk0p%d", index + 4);
			}
		}

		temp_offset = strlen(fb_part.name) + strlen(part_name) + 2;
		if(temp_offset >= PARTITION_SETS_MAX_SIZE)
		{
			printf("partition_sets is too long, please reduces partition name\n");
			break;
		}
		//fastboot_flash_add_ptn(&fb_part);
		sprintf(partition_index, "%s@%s:", fb_part.name, part_name);
		offset += temp_offset;
		partition_index = partition_sets + offset;
	}

	partition_sets[offset-1] = '\0';
	partition_sets[PARTITION_SETS_MAX_SIZE - 1] = '\0';
//	printf("-----------------------------------\n");

	setenv("partitions", partition_sets);
}


#define    ANDROID_NULL_MODE            0
#define    ANDROID_FASTBOOT_MODE		1
#define    ANDROID_RECOVERY_MODE		2
#define    USER_SELECT_MODE 			3
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int detect_other_boot_mode(void)
{
    int ret1, ret2;
	int key_high, key_low;
	int keyvalue;

	keyvalue = gd->key_pressd_value;
	printf("key %d\n", keyvalue);

    ret1 = script_parser_fetch("recovery_key", "key_max", &key_high, 1);
	ret2 = script_parser_fetch("recovery_key", "key_min", &key_low,  1);
	if((ret1) || (ret2))
	{
		printf("cant find rcvy value\n");
	}
	else
	{
		printf("recovery key high %d, low %d\n", key_high, key_low);
		if((keyvalue >= key_low) && (keyvalue <= key_high))
		{
			printf("key found, android recovery\n");

			return ANDROID_RECOVERY_MODE;
		}
	}
    ret1 = script_parser_fetch("fastboot_key", "key_max", &key_high, 1);
	ret2 = script_parser_fetch("fastboot_key", "key_min", &key_low, 1);
	if((ret1) || (ret2))
	{
		printf("cant find fstbt value\n");
	}
	else
	{
		printf("fastboot key high %d, low %d\n", key_high, key_low);
		if((keyvalue >= key_low) && (keyvalue <= key_high))
		{
			printf("key found, android fastboot\n");
			return ANDROID_FASTBOOT_MODE;
		}
	}

	return ANDROID_NULL_MODE;
}

int update_bootcmd(void)
{
	int   mode;
	int   pmu_value;
	u32   misc_offset = 0;
	char  misc_args[2048];
	char  misc_fill[2048];
	char  boot_commond[256];
	static struct bootloader_message *misc_message;

	if(gd->force_shell)
	{
		char delaytime[8];
		sprintf(delaytime, "%d", 3);
		setenv("bootdelay", delaytime);
	}

	memset(boot_commond, 0x0, 128);
	strcpy(boot_commond, getenv("bootcmd"));
	printf("base bootcmd=%s\n", boot_commond);

	if((uboot_spare_head.boot_data.storage_type == STORAGE_SD) ||
		(uboot_spare_head.boot_data.storage_type == STORAGE_EMMC)||
		uboot_spare_head.boot_data.storage_type == STORAGE_EMMC3)
	{
		sunxi_str_replace(boot_commond, "setargs_nand", "setargs_mmc");
		printf("bootcmd set setargs_mmc\n");
	}
	else if(uboot_spare_head.boot_data.storage_type == STORAGE_NOR)
	{
		sunxi_str_replace(boot_commond, "setargs_nand", "setargs_nor");
	}
	else if(uboot_spare_head.boot_data.storage_type == STORAGE_NAND)
	{
		printf("bootcmd set setargs_nand\n");
	}

	//user enter debug mode by plug usb up to 3 times
	if(debug_mode_get())
	{
		//if enter debug mode,set loglevel = 8
		debug_mode_update_info();
		return 0;
	}

	misc_message = (struct bootloader_message *)misc_args;
	memset(misc_args, 0x0, 2048);
	memset(misc_fill, 0xff, 2048);

	mode = detect_other_boot_mode();
	switch(mode)
	{
		case ANDROID_RECOVERY_MODE:
			strcpy(misc_message->command, "boot-recovery");
			break;
		case ANDROID_FASTBOOT_MODE:
			strcpy(misc_message->command, "bootloader");
			break;
		case ANDROID_NULL_MODE:
			{
				pmu_value = gd->pmu_saved_status;
				if(pmu_value == PMU_PRE_FASTBOOT_MODE)
				{
					puts("PMU : ready to enter fastboot mode\n");
					strcpy(misc_message->command, "bootloader");
				}
				else if(pmu_value == PMU_PRE_RECOVERY_MODE)
				{
					puts("PMU : ready to enter recovery mode\n");
					strcpy(misc_message->command, "boot-recovery");
				}
				else
				{
					misc_offset = sunxi_partition_get_offset_byname("misc");
					debug("misc_offset = %x\n",misc_offset);
					if(!misc_offset)
					{
						printf("no misc partition is found\n");
					}
					else
					{
						printf("misc partition found\n");
						//read misc partition data
						sunxi_flash_read(misc_offset, 2048/512, misc_args);
					}
				}
			}
			break;
	}

	if(!strcmp(misc_message->command, "efex"))
	{
		/* there is a recovery command */
		puts("find efex cmd\n");
		sunxi_flash_write(misc_offset, 2048/512, misc_fill);
		sunxi_board_run_fel();
		return 0;
	}
	else if(!strcmp(misc_message->command, "boot-resignature"))
	{
		puts("error: find boot-resignature cmd,but this cmd not implement\n");
		//sunxi_flash_write(misc_offset, 2048/512, misc_fill);
		//sunxi_oem_op_lock(SUNXI_LOCKING, NULL, 1);
	}
	else if(!strcmp(misc_message->command, "boot-recovery"))
	{
		if(!strcmp(misc_message->recovery, "sysrecovery"))
		{
			puts("recovery detected, will sprite recovery\n");
			strncpy(boot_commond, "sprite_recovery", sizeof("sprite_recovery"));
			sunxi_flash_write(misc_offset, 2048/512, misc_fill);
		}
		else
		{
			puts("Recovery detected, will boot recovery\n");
			sunxi_str_replace(boot_commond, "boot_normal", "boot_recovery");
		}
		/* android recovery will clean the misc */
	}
	else if(!strcmp(misc_message->command, "bootloader"))
	{
		puts("Fastboot detected, will boot fastboot\n");
		sunxi_str_replace(boot_commond, "boot_normal", "boot_fastboot");
		if(misc_offset)
			sunxi_flash_write(misc_offset, 2048/512, misc_fill);
	}
	else if(!strcmp(misc_message->command, "usb-recovery"))
	{
		puts("Recovery detected, will usb recovery\n");
		sunxi_str_replace(boot_commond, "boot_normal", "boot_recovery");
	}
	else if(!strcmp(misc_message->command ,"debug_mode"))
	{
		puts("debug_mode detected ,will enter debug_mode");
		if(0 == debug_mode_set())
		{
			debug_mode_update_info();
		}
		sunxi_flash_write(misc_offset,2048/512,misc_fill);
	}

	if (gd->need_shutdown) {
		sunxi_secondary_cpu_poweroff();
		sunxi_board_shutdown();
		for(;;);
	}

	setenv("bootcmd", boot_commond);
	tick_printf("to be run cmd=%s\n", boot_commond);

	return 0;
}


int disable_node(char* name)
{

	int nodeoffset = 0;
	int ret = 0;

	if(strcmp(name,"mmc3") == 0)
	{
#ifndef CONFIG_MMC3_SUPPORT
		return 0;
#endif
	}
	//nodeoffset = fdt_path_offset(working_fdt, name);
	//ret = fdt_set_node_status(working_fdt,nodeoffset,FDT_STATUS_DISABLED,0);
	nodeoffset = fdtfast_path_offset(working_fdt, name);
	if (!nodeoffset) {
		printf("can not find %s offset in fdtfast table\n", name);

		return -1;
	}
	ret = fdtfast_set_node_status(working_fdt,nodeoffset,FDT_STATUS_DISABLED,0);
	if(ret < 0)
	{
		printf("disable nand error: %s\n", fdt_strerror(ret));
	}
	return ret;
}

int  dragonboard_handler(void* dtb_base)
{
#ifdef CONFIG_SUNXI_DRAGONBOARD_SUPPORT
	int card_num = 0;

	printf("%s call\n",__func__);
	if(!nand_uboot_probe())  //nand_uboot_init(1)
	{
		printf("try nand successed \n");
		disable_node("mmc2");
		disable_node("mmc3");
	}
	else
	{
		struct mmc *mmc_tmp = NULL;
#ifdef CONFIG_MMC3_SUPPORT
		card_num = 3;
#else
		card_num = 2;
#endif
		printf("try to find card%d \n", card_num);
		board_mmc_set_num(card_num);
		board_mmc_pre_init(card_num);
		mmc_tmp = find_mmc_device(card_num);
		if(!mmc_tmp)
		{
			return -1;
		}
		if (0 == mmc_init(mmc_tmp))
		{
			printf("try mmc successed \n");
			disable_node("nand0");
			if(card_num == 2)
				disable_node("mmc3");
			else if(card_num == 3)
				disable_node("mmc2");
		}
		else
		{
			printf("try card%d successed \n", card_num);
		}
		mmc_exit();
	}
#endif
	return 0;

}


int update_fdt_para_for_kernel(void* dtb_base)
{
	int ret;
	int nodeoffset = 0;
	uint storage_type = 0;


	storage_type = uboot_spare_head.boot_data.storage_type;
	//fix nand&sdmmc
	switch(storage_type)
	{
		case STORAGE_NAND:
			disable_node("mmc2");
			disable_node("mmc3");
			break;
		case STORAGE_EMMC:
			disable_node("nand0");
			disable_node("mmc3");
			break;
		case STORAGE_EMMC3:
			disable_node("nand0");
			disable_node("mmc2");
			break;
		case STORAGE_SD:
		{
			uint32_t dragonboard_test = 0;
			nodeoffset= fdt_path_offset(dtb_base, "/soc/target");
			ret = fdt_getprop_u32(dtb_base, nodeoffset, "dragonboard_test",
				(uint32_t *) &dragonboard_test);
			if(dragonboard_test == 1)
			{
				dragonboard_handler(dtb_base);
			}
			else
			{
				disable_node("nand0");
				disable_node("mmc2");
				disable_node("mmc3");
			}
		}
			break;
		default:
			break;

	}
	//fix memory
	ret = fdt_fixup_memory(dtb_base, gd->bd->bi_dram[0].start ,gd->bd->bi_dram[0].size);
	if(ret < 0)
	{
		return -1;
	}

	//fix dram para
	uint32_t *dram_para = NULL;
	dram_para = (uint32_t *)uboot_spare_head.boot_data.dram_para;
	tick_printf("update dtb dram start\n");
	//nodeoffset = fdt_path_offset(dtb_base, "/dram");
	nodeoffset = fdtfast_path_offset(working_fdt, "/dram");
	if(nodeoffset<0)
	{
		printf("## error: %s : %s\n", __func__,fdt_strerror(ret));
		return -1;
	}
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_clk", dram_para[0]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_type", dram_para[1]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_zq", dram_para[2]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_odt_en", dram_para[3]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_para1", dram_para[4]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_para2", dram_para[5]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_mr0", dram_para[6]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_mr1", dram_para[7]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_mr2", dram_para[8]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_mr3", dram_para[9]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr0", dram_para[10]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr1", dram_para[11]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr2", dram_para[12]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr3", dram_para[13]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr4", dram_para[14]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr5", dram_para[15]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr6", dram_para[16]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr7", dram_para[17]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr8", dram_para[18]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr9", dram_para[19]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr10", dram_para[20]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr11", dram_para[21]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr12", dram_para[22]);
	fdtfast_setprop_u32(dtb_base,nodeoffset, "dram_tpr13", dram_para[23]);
	tick_printf("update dtb dram  end\n");

	return 0;
}

int get_boot_work_mode(void)
{
	return uboot_spare_head.boot_data.work_mode;
}

int get_boot_storage_type(void)
{
	return uboot_spare_head.boot_data.storage_type;
}

u32 get_boot_dram_para_addr(void)
{
	return (u32)uboot_spare_head.boot_data.dram_para;
}

u32 get_boot_dram_para_size(void)
{
	return 32*4;
}

u32 get_boot_dram_update_flag(void)
{
	u32 flag = 0;
	//boot pass dram para to uboot
	//dram_tpr13:bit31
	//0:uboot update boot0  1: not
	__dram_para_t *pdram = (__dram_para_t *)uboot_spare_head.boot_data.dram_para;
	flag = (pdram->dram_tpr13>>31)&0x1;
	return flag == 0 ? 1:0;
}

void set_boot_dram_update_flag(u32 *dram_para)
{
	//dram_tpr13:bit31
	//0:uboot update boot0  1: not
	u32 flag = 0;
	__dram_para_t *pdram = (__dram_para_t *)dram_para;
	flag = pdram->dram_tpr13;
	flag |= (1<<31);
	pdram->dram_tpr13 = flag;
}



/*
************************************************************************************************************
*
*                                             function
*
*    name          :	get_debugmode_flag
*
*    parmeters     :
*
*    return        :
*
*    note          :	guoyingyang@allwinnertech.com
*
*
************************************************************************************************************
*/
int get_debugmode_flag(void)
{
	int debug_mode = 0;
	int nodeoffset;
	if(get_boot_work_mode() != WORK_MODE_BOOT)
	{
		gd->debug_mode = 1;
		return 0;
	}
	nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_PLATFORM);
	if(fdt_getprop_u32(working_fdt,nodeoffset,"debug_mode",(uint32_t*)&debug_mode) > 0)
	{
		gd->debug_mode = debug_mode;
	}
	else
	{
		gd->debug_mode = 1;
	}
	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :	update_dram_para_for_ota
*
*    parmeters     :
*
*    return        :
*
*    note          :	after ota, should restore dram para to flash.
*
*
************************************************************************************************************
*/

int update_dram_para_for_ota(void)
{
	extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

#ifdef CONFIG_ARCH_SUN50IW1P1
	return 0;
#endif

#ifdef FPGA_PLATFORM
	return 0;
#else
	int ret = 0;
	if( (get_boot_work_mode() == WORK_MODE_BOOT) &&
		(STORAGE_SD != get_boot_storage_type()))
	{
		if(get_boot_dram_update_flag() || mmc_request_update_boot0(2))
		{
			printf("begin to update boot0 atfer ota\n");
			ret = sunxi_flash_update_boot0();
			printf("update boot0 %s\n", ret==0?"success":"fail");
			//if update fail, should reset the system
			if(ret)
			{
				do_reset(NULL, 0, 0,NULL);
			}
		}
	}
	return ret;
#endif
}

int board_late_init(void)
{
	int ret  = 0;
	if(get_boot_work_mode() == WORK_MODE_BOOT)
	{
		sunxi_fastboot_init();
		update_dram_para_for_ota();
		update_bootcmd();
		ret = update_fdt_para_for_kernel(working_fdt);
#ifdef CONFIG_SUNXI_SERIAL
                sunxi_set_serial_num();
#endif
		return 0;
	}

	return ret;
}

