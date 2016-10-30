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
#include <mmc.h>
#include <boot_type.h>
#include <sunxi_board.h>
#include <sunxi_flash.h>
#include <sys_config.h>
#include <sys_partition.h>
#include <malloc.h>
#include "flash_interface.h"
#include <private_toc.h>
#include <private_boot0.h>
#include <private_uboot.h>
#include <sunxi_nand.h>

int sunxi_flash_init_uboot(int verbose);
extern int sunxi_card_fill_boot0_magic(void);

extern int card_read_boot0(void *buffer,uint length);



/*
************************************************************************************************************
*
*                                             function
*
*
*
*
*
*
*
*
************************************************************************************************************
*/

static int
sunxi_null_op(unsigned int start_block, unsigned int nblock, void *buffer){
	return 0;
}

static int
sunxi_null_erase(int erase, void *mbr_buffer)
{
	return 0;
}

static uint
sunxi_null_size(void){
	return 0;
}

static int
sunxi_null_init(int flag){
	return -1;
}

static int
sunxi_null_exit(int force){
	return -1;
}

static int
sunxi_null_flush(void){
	return 0;
}

static int
sunxi_null_force_erase(void){
    return 0;
}

#ifdef CONFIG_SUNXI_SPINOR
static int
sunxi_null_datafinish(void){
	return 0;
}
#endif


/************************************************************************************************************
 *
 *                                            weak function
 *
 ************************************************************************************************************
 */

int  __attribute__((weak)) nand_init_for_boot(int workmode)
{
	return -1;
}

int  __attribute__((weak)) sdmmc_init_for_boot(int workmode, int card_no)
{
	return -1;
}
int __attribute__((weak))  spinor_init_for_boot(int workmode, int spino)
{
	return -1;
}

int __attribute__((weak)) nand_init_for_sprite(int workmode)
{
	return -1;
}
int __attribute__((weak)) sdmmc_init_for_sprite(int workmode)
{
	return -1;
}
int __attribute__((weak)) sdmmc_init_card0_for_sprite(void)
{
	return -1;
}
int __attribute__((weak)) spinor_init_for_sprite(int workmode)
{
	return -1;
}

int __attribute__((weak)) nand_read_uboot_data(unsigned char *buf,unsigned int len)
{
	return -1;
}

int __attribute__((weak))  nand_read_boot0(void *buffer,uint length)
{
	return -1;
}
int __attribute__((weak)) sunxi_download_boot0_atfter_ota(void *buffer, int production_media)
{
	return -1;
}
int __attribute__((weak))  sunxi_sprite_download_uboot(void *buffer, int production_media, int generate_checksum)
{
	return -1;
}
int __attribute__((weak))  card_erase(int erase, void *mbr_buffer)
{
	return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*
*
*
*
*
*
*
************************************************************************************************************
*/

int (* sunxi_flash_init_pt)(int stage) = sunxi_null_init;
int (* sunxi_flash_read_pt) (uint start_block, uint nblock, void *buffer) = sunxi_null_op;
//int (* sunxi_flash_read_sequence) (uint start_block, uint nblock, void *buffer) = sunxi_null_op;
int (* sunxi_flash_write_pt)(uint start_block, uint nblock, void *buffer) = sunxi_null_op;
uint (* sunxi_flash_size_pt)(void) = sunxi_null_size;
int (* sunxi_flash_exit_pt) (int force) = sunxi_null_exit;
int (* sunxi_flash_flush_pt) (void) = sunxi_null_flush;
int (* sunxi_flash_phyread_pt) (unsigned int start_block, unsigned int nblock, void *buffer) = sunxi_null_op;
int (* sunxi_flash_phywrite_pt)(unsigned int start_block, unsigned int nblock, void *buffer) = sunxi_null_op;

int (* sunxi_sprite_init_pt)(int stage) = sunxi_null_init;
int (* sunxi_sprite_read_pt) (uint start_block, uint nblock, void *buffer) = sunxi_null_op;
int (* sunxi_sprite_write_pt)(uint start_block, uint nblock, void *buffer) = sunxi_null_op;
int (* sunxi_sprite_erase_pt)(int erase, void *mbr_buffer) = sunxi_null_erase;
uint (* sunxi_sprite_size_pt)(void) = sunxi_null_size;
int (* sunxi_sprite_exit_pt) (int force) = sunxi_null_exit;
int (* sunxi_sprite_flush_pt)(void) = sunxi_null_flush;
int (* sunxi_sprite_force_erase_pt)(void)  = sunxi_null_force_erase;
int (* sunxi_sprite_phyread_pt) (unsigned int start_block, unsigned int nblock, void *buffer) = sunxi_null_op;
int (* sunxi_sprite_phywrite_pt)(unsigned int start_block, unsigned int nblock, void *buffer) = sunxi_null_op;
#ifdef CONFIG_SUNXI_SPINOR
int (* sunxi_sprite_datafinish_pt) (void) = sunxi_null_datafinish;
#endif



//-------------------------------------noraml interface start--------------------------------------------
int sunxi_flash_read (uint start_block, uint nblock, void *buffer)
{
	debug("sunxi flash read : start %d, sector %d\n", start_block, nblock);
	return sunxi_flash_read_pt(start_block, nblock, buffer);
}

int sunxi_flash_write(uint start_block, uint nblock, void *buffer)
{
	debug("sunxi flash write : start %d, sector %d\n", start_block, nblock);
	return sunxi_flash_write_pt(start_block, nblock, buffer);
}

uint sunxi_flash_size(void)
{
	return sunxi_flash_size_pt();
}

int sunxi_flash_exit(int force)
{
    return sunxi_flash_exit_pt(force);
}

int sunxi_flash_flush(void)
{
	return sunxi_flash_flush_pt();
}

int sunxi_flash_phyread (uint start_block, uint nblock, void *buffer)
{
	return sunxi_flash_phyread_pt(start_block, nblock, buffer);
}

int sunxi_flash_phywrite(uint start_block, uint nblock, void *buffer)
{
	return sunxi_flash_phywrite_pt(start_block, nblock, buffer);
}
//-----------------------------------noraml interface end---------------------------------------------------


//-------------------------------------sprite interface start-----------------------------------------------
int sunxi_sprite_init(int stage)
{
	return sunxi_sprite_init_pt(stage);
}

int sunxi_sprite_read (uint start_block, uint nblock, void *buffer)
{
	return sunxi_sprite_read_pt(start_block, nblock, buffer);
}

int sunxi_sprite_write(uint start_block, uint nblock, void *buffer)
{
	return sunxi_sprite_write_pt(start_block, nblock, buffer);
}

int sunxi_sprite_erase(int erase, void *mbr_buffer)
{
	return sunxi_sprite_erase_pt(erase, mbr_buffer);
}

uint sunxi_sprite_size(void)
{
	return sunxi_sprite_size_pt();
}

int sunxi_sprite_exit(int force)
{
    return sunxi_sprite_exit_pt(force);
}

int sunxi_sprite_flush(void)
{
	return sunxi_sprite_flush_pt();
}

int sunxi_sprite_phyread (uint start_block, uint nblock, void *buffer)
{
	return sunxi_sprite_phyread_pt(start_block, nblock, buffer);
}

int sunxi_sprite_phywrite(uint start_block, uint nblock, void *buffer)
{
	return sunxi_sprite_phywrite_pt(start_block, nblock, buffer);
}

int sunxi_sprite_force_erase(void)
{
    return sunxi_sprite_force_erase_pt();
}
//-------------------------------------sprite interface end-----------------------------------------------

//sunxi flash boot interface init
int sunxi_flash_boot_handle(int storage_type,int workmode )
{
	int card_no;
	int state;
	switch(storage_type)
	{
		case STORAGE_NAND:
		{
			//nand handle init
			state = nand_init_for_boot(workmode);
		}
		break;

		case STORAGE_SD:
		case STORAGE_EMMC:
                case STORAGE_EMMC3:
		{
			//sdmmc handle init
		//	card_no = (storage_type == 1)?0:2;
		        if(storage_type == STORAGE_SD)
                                card_no = 0;
                        else if(storage_type == STORAGE_EMMC)
                                card_no = 2;
                        else
                                card_no = 3;
                        state = sdmmc_init_for_boot(workmode,card_no);
		}
		break;

		case STORAGE_NOR:
		{
			state = spinor_init_for_boot(workmode,0);
		}
		break;

		default:
		{
			printf("not support\n");
			state = -1;
		}
		break;

	}

	if(state != 0)
	{
		return -1;
	}

	//script_parser_patch("target", "storage_type", &storage_type, 1);
	tick_printf("sunxi flash init ok\n");
	return  0;
}

int sunxi_flash_sprite_handle(int storage_type,int workmode)
{
	int state = 0;
	//try emmc, nand, spi-nor
	state = sdmmc_init_for_sprite(workmode);

	if(state !=0 )
	{
		printf("try emmc fail\n");
		state = nand_init_for_sprite(workmode);
		if(state != 0)
		{
			printf("try nand fail\n");
		}
	}

	if(state !=0 )
	{
		state = spinor_init_for_sprite(workmode);
		if(state != 0)
		{
			printf("try spinor fail\n");
		}
	}

	if(state !=0 )
	{
		return -1;
	}

	//sdcard burn mode
	if((workmode == WORK_MODE_CARD_PRODUCT) || (workmode == 0x30))
	{
		state = sdmmc_init_card0_for_sprite();
		if(state != 0)
		{
			return -1;
		}
	}
	return 0;
}

int sunxi_flash_handle_init(void)
{

	int workmode = 0;
	int storage_type = 0;
	int state = 0;

	workmode     = uboot_spare_head.boot_data.work_mode;
	storage_type = uboot_spare_head.boot_data.storage_type;

	printf("workmode = %d,storage type = %d\n", workmode,storage_type);

	if (workmode == WORK_MODE_BOOT || workmode == WORK_MODE_SPRITE_RECOVERY)
	{
		state = sunxi_flash_boot_handle(storage_type,workmode);
	}
	else if((workmode & WORK_MODE_PRODUCT) || (workmode == 0x30))
	{
		state = sunxi_flash_sprite_handle(storage_type,workmode);
	}
	else if(workmode & WORK_MODE_UPDATE)
	{
	}
	else
	{
	}
	//init blk dev for fat
	sunxi_flash_init_uboot(0);

	return state;
}


//-------------------------------------------------------------------------------------------
//sunxi flash blk device for fat
//-------------------------------------------------------------------------------------------
static block_dev_desc_t 	sunxi_flash_blk_dev;

block_dev_desc_t *sunxi_flash_get_dev(int dev)
{
	sunxi_flash_blk_dev.dev = dev;
	sunxi_flash_blk_dev.lba = sunxi_partition_get_size(dev);

	return ((block_dev_desc_t *) & sunxi_flash_blk_dev);
}

unsigned long  sunxi_flash_part_read(int dev_num, unsigned long start, lbaint_t blkcnt, void *dst)
{
	uint offset;

	offset = sunxi_partition_get_offset(dev_num);
	if(!offset)
	{
		printf("sunxi flash error: cant get part %d offset\n", dev_num);

		return 0;
	}
	start += offset;
	debug("nand try to read from %x, length %x block\n", (int )start, (int )blkcnt);
	return sunxi_flash_read((uint)start, (uint )blkcnt, dst);

}

unsigned long  sunxi_flash_part_write(int dev_num, unsigned long start, lbaint_t blkcnt, const void *dst)
{
	uint offset;

	offset = sunxi_partition_get_offset(dev_num);
	if(!offset)
	{
		printf("sunxi flash error: cant get part %d offset\n", dev_num);

		return 0;
	}
	start += offset;
	debug("nand try to write from %x, length %x block\n", (int )start, (int )blkcnt);

	return sunxi_flash_write((uint)start, (uint )blkcnt, (void *)dst);

}


int sunxi_flash_init_uboot(int verbose)
{

	debug("sunxi flash init uboot\n");
	sunxi_flash_blk_dev.if_type = IF_TYPE_SUNXI_FLASH;
	sunxi_flash_blk_dev.part_type = PART_TYPE_DOS;
	sunxi_flash_blk_dev.dev = 0;
	sunxi_flash_blk_dev.lun = 0;
	sunxi_flash_blk_dev.type = 0;

	/* FIXME fill in the correct size (is set to 32MByte) */
	sunxi_flash_blk_dev.blksz = 512;
	sunxi_flash_blk_dev.lba = 0;
	sunxi_flash_blk_dev.removable = 0;
	sunxi_flash_blk_dev.block_read = sunxi_flash_part_read;
	sunxi_flash_blk_dev.block_write = sunxi_flash_part_write;

	return 0;
}


extern int sunxi_sprite_download_uboot(void *buffer, int production_media, int mode);
extern  int nand_read_uboot_data(unsigned char *buf,unsigned int len);

#ifdef CONFIG_SUNXI_SPINOR
int sunxi_nor_flash_update_fdt(void* fdt_buf, size_t fdt_size)
{
	printf("Warnning:spinor platform not support update fdt to flash\n");
	return -1;
}
int sunxi_nor_flash_update_boot0(void)
{
	printf("Warnning:spinor platform not support update boot0 to flash\n");
	return -1;
}
#endif

#if defined(CONFIG_SUNXI_MODULE_NAND) || defined(CONFIG_SUNXI_MODULE_SDMMC)
int read_boot_package(int storage_type, void *package_buf)
{

	int total_length = 0;
	int read_len = uboot_spare_head.boot_data.boot_package_size;
	int ret = 0;
	sbrom_toc1_head_info_t *toc1_head=NULL;

	printf("boot package size: 0x%x\n",read_len);
	if(storage_type == STORAGE_NAND)
	{
		ret = nand_read_uboot_data(package_buf,read_len);
	}
	else if(storage_type == STORAGE_EMMC || storage_type == STORAGE_SD || storage_type == STORAGE_EMMC3)
	{
		ret = sunxi_sprite_phyread(UBOOT_START_SECTOR_IN_SDMMC, read_len/512, package_buf);
	}
	else if(storage_type == STORAGE_NOR)
	{
	}

	toc1_head = (struct sbrom_toc1_head_info *)package_buf;
	if(toc1_head->magic != TOC_MAIN_INFO_MAGIC)
	{
		printf("toc1 magic error\n");
		return -1;
	}
	total_length = toc1_head->valid_len;

	printf("read uboot from flash: ret %d\n",ret);
	return total_length;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sunxi_flash_upload_boot0
*
*    parmeters     :
*
*    return        :
*
*    note          :	size should page align for nand flash
*
*
************************************************************************************************************
*/
int sunxi_flash_upload_boot0(char * buffer, int size)
{
	int storage_type = 0;
	int ret = 0;
	storage_type = get_boot_storage_type();
	if(STORAGE_NAND == storage_type)
	{
		ret = nand_read_boot0(buffer, size);
	}
	else if(STORAGE_EMMC == storage_type || STORAGE_EMMC3 == storage_type)
	{
		ret = card_read_boot0(buffer, size);
	}
	else
	{
		printf("%s:not support storage type %d\n", __func__,storage_type);
		ret = -1;
	}
	return ret;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sunxi_flash_get_boot0_size
*
*    parmeters     :
*
*    return        :
*
*    note          :	get boot0|sboot size from boot head
*
*
************************************************************************************************************
*/

int sunxi_flash_get_boot0_size(void)
{
	//note:page align for nand
	char boot_buffer[32*1024];
	int size;
	int ret = 0;

	//read boot head
	ret = sunxi_flash_upload_boot0(boot_buffer,sizeof(boot_buffer));
	if(ret)
	{
		printf("%s: get boot0 head fail\n",__func__);
		return -1;
	}
	//get boot size
	if(SUNXI_NORMAL_MODE == sunxi_get_securemode())
	{
		boot0_file_head_t    *boot0  = (boot0_file_head_t *)boot_buffer;
		size = boot0->boot_head.length;
	}
	else
	{
		toc0_private_head_t  *toc0   = (toc0_private_head_t *)boot_buffer;
		size = toc0->length;
	}
	return size;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :	sunxi_flash_update_boot0
*
*    parmeters     :
*
*    return        :
*
*    note          :	only for dram para update after ota
*
*
************************************************************************************************************
*/
int sunxi_nand_emmc_flash_update_boot0(void)
{
	int storage_type = 0;
	char* boot_buffer = NULL;
	int ret = 0;
	int size = 0;

	storage_type = get_boot_storage_type();
	printf("storage type = %d\n", storage_type);

	size = sunxi_flash_get_boot0_size();
	if(size <= 0)
	{
		printf("%s: get boot size error\n", __func__);
		return -1;
	}
	printf("boot0 size %d\n", size);
	boot_buffer = malloc(size);
	if(!boot_buffer)
	{
		printf("%s:alloc memory fail\n", __func__);
		return -1;
	}
	//read total boot data
	ret = sunxi_flash_upload_boot0(boot_buffer,size);
	if(ret)
	{
		printf("%s:upload boot0 fail\n", __func__ );
		goto _UPDATE_ERROR_;
	}
	ret = sunxi_download_boot0_atfter_ota(boot_buffer,storage_type);
	if(ret)
	{
		printf("%s:update boot0 for ota fail\n", __func__);
	}
	free(boot_buffer);
	//sunxi_flash_flush();

	return ret;
_UPDATE_ERROR_:
	if(boot_buffer) free(boot_buffer);
	return -1;
}

int sunxi_nand_emmc_flash_update_fdt(void* fdt_buf, size_t fdt_size)
{
	int package_size;
	int i = 0;
	char *package_buf = NULL;
	int   package_buf_size = 10<<20;
	int   find_flag = 0;
	u32   dtb_base = 0;
	int   storage_type = 0;
	int   ret = -1;

	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;

	struct sbrom_toc1_item_info  *toc1_item = NULL;
	struct spare_boot_head_t *uboot_head = NULL;

	storage_type = get_boot_storage_type();

	//10M buffer
	package_buf = (char*)malloc(package_buf_size);
	if(package_buf == NULL)
	{
		printf("%s: malloc fail\n", __func__);
		return -1;
	}

	memset(package_buf,0,package_buf_size);
	package_size = read_boot_package(storage_type,package_buf);
	if(package_size <= 0)
	{
		goto _UPDATE_END;
	}

	toc1_head = (struct sbrom_toc1_head_info *)package_buf;
	item_head = (struct sbrom_toc1_item_info *)(package_buf + sizeof(struct sbrom_toc1_head_info));
	toc1_item = item_head;
	for(i=0;i<toc1_head->items_nr;i++,toc1_item++)
	{
		printf("Entry_name        = %s\n",   toc1_item->name);
		if(strncmp(toc1_item->name, ITEM_UBOOT_NAME, sizeof(ITEM_UBOOT_NAME)) == 0)
		{
			find_flag = 1;
			break;
		}
	}
	if(!find_flag)
	{
		printf("error:can't find uboot\n");
		goto _UPDATE_END;
	}

	uboot_head =  (struct spare_boot_head_t *)(package_buf+toc1_item->data_offset);
	if(strncmp((const char *)uboot_head->boot_head.magic, UBOOT_MAGIC, MAGIC_SIZE))
	{
		printf("%s: uboot magic is error\n",__func__);
		goto _UPDATE_END;
	}

	dtb_base = (u32)(package_buf + toc1_item->data_offset) + uboot_head->boot_data.dtb_offset;
	memcpy((void*)dtb_base, fdt_buf, fdt_size);

	ret = sunxi_sprite_download_uboot(package_buf,storage_type,1);

_UPDATE_END:
	if(package_buf)
	{
		free(package_buf);
	}
	return ret;
}
#endif

int sunxi_flash_update_boot0(void)
{
	int storage_type = 0;
	int ret = -1;
	storage_type = get_boot_storage_type();
	printf("storage type = %d\n", storage_type);

	if (storage_type == STORAGE_NOR)
	{
#ifdef CONFIG_SUNXI_SPINOR
		ret = sunxi_nor_flash_update_boot0();
#endif
	}
	else
	{
#if defined(CONFIG_SUNXI_MODULE_NAND) || defined(CONFIG_SUNXI_MODULE_SDMMC)
		ret = sunxi_nand_emmc_flash_update_boot0();
#endif
	}

	return ret;
}

int sunxi_flash_update_fdt(void* fdt_buf, size_t fdt_size)
{
	int storage_type = 0;
	int ret = -1;
	storage_type = get_boot_storage_type();
	printf("storage type = %d\n", storage_type);

	if (storage_type == STORAGE_NOR)
	{
#ifdef CONFIG_SUNXI_SPINOR
		ret = sunxi_nor_flash_update_fdt(fdt_buf, fdt_size);
#endif
	}
	else
	{
#if defined(CONFIG_SUNXI_MODULE_NAND) || defined(CONFIG_SUNXI_MODULE_SDMMC)
		ret = sunxi_nand_emmc_flash_update_fdt(fdt_buf, fdt_size);
#endif
	}

	return ret;
}
