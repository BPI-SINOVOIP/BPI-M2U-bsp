/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Char <yanjianbo@allwinnertech.com>
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
#include <config.h>
#include <common.h>
#include <sunxi_mbr.h>
#include <malloc.h>
#include <sys_config.h>
#include <sunxi_board.h>
#include "sprite_card.h"
#include "sprite_download.h"
#include "sprite_erase.h"
#include "./firmware/imgdecode.h"
#include "./firmware/imagefile_new.h"

extern uint img_file_start;
extern int sunxi_sprite_deal_part_from_sysrevoery(sunxi_download_info *dl_map);
extern int __imagehd(HIMAGE tmp_himage);

typedef struct tag_IMAGE_HANDLE
{
	ImageHead_t  ImageHead;
	ImageItem_t *ItemTable;
}IMAGE_HANDLE;


HIMAGE 	Img_Open_from_sysrecovery(__u32 start)
{
	IMAGE_HANDLE * pImage = NULL;
	uint ItemTableSize;

	img_file_start = start;
	if(!img_file_start)
	{
		printf("sunxi sprite error: unable to get firmware start position\n");

		return NULL;
	}
	debug("img start = 0x%x\n", img_file_start);
	pImage = (IMAGE_HANDLE *)malloc(sizeof(IMAGE_HANDLE));
	if (NULL == pImage)
	{
		printf("sunxi sprite error: fail to malloc memory for img head\n");

		return NULL;
	}
	memset(pImage, 0, sizeof(IMAGE_HANDLE));

	//debug("try to read mmc start %d\n", img_file_start);
	if(!sunxi_flash_read(img_file_start, IMAGE_HEAD_SIZE/512, &pImage->ImageHead))
	{
		printf("sunxi sprite error: read iamge head fail\n");

		goto _img_open_fail_;
	}
	debug("read mmc ok\n");

	if (memcmp(pImage->ImageHead.magic, IMAGE_MAGIC, 8) != 0)
	{
		printf("sunxi sprite error: iamge magic is bad\n");

		goto _img_open_fail_;
	}

	ItemTableSize = pImage->ImageHead.itemcount * sizeof(ImageItem_t);
	pImage->ItemTable = (ImageItem_t*)malloc(ItemTableSize);
	if (NULL == pImage->ItemTable)
	{
		printf("sunxi sprite error: fail to malloc memory for item table\n");

		goto _img_open_fail_;
	}

	if(!sunxi_flash_read(img_file_start + (IMAGE_HEAD_SIZE/512), ItemTableSize/512, pImage->ItemTable))
	{
		printf("sunxi sprite error: read iamge item table fail\n");

		goto _img_open_fail_;
	}

	return pImage;

_img_open_fail_:
	if(pImage->ItemTable)
	{
		free(pImage->ItemTable);
	}
	if(pImage)
	{
		free(pImage);
	}

	return NULL;
}


int  card_part_info(__u32 *part_start, __u32 *part_size, const char *str)
{
    char   buffer[SUNXI_MBR_SIZE];
    sunxi_mbr_t    *mbr;
    int    i;

    if(!sunxi_flash_read(0, SUNXI_MBR_SIZE/512, buffer))
    {
    	printf("read mbr failed\n");

    	return -1;
    }
    mbr = (sunxi_mbr_t *)buffer;

    for(i=0;i<mbr->PartCount;i++)
    {
    	printf("part name  = %s\n", mbr->array[i].name);
    	printf("part start = %d\n", mbr->array[i].addrlo);
    	printf("part size  = %d\n", mbr->array[i].lenlo);
    }

    for(i=0;i<mbr->PartCount;i++)
    {
        if(!strcmp(str, (char *)mbr->array[i].name))
        {
            *part_start = mbr->array[i].addrlo;
            *part_size  = mbr->array[i].lenlo;

            return 0;
        }
    }

    return -1;
}


int sprite_form_sysrecovery(void)
{
	HIMAGEITEM  imghd = 0;
	__u32       part_size;
	__u32		img_start;
	sunxi_download_info   *dl_info  = NULL;
	char        *src_buf = NULL;
	int         ret = -1;
	int production_media = uboot_spare_head.boot_data.storage_type;

	printf("sunxi sprite begin\n");

	sprite_cartoon_create();

	src_buf = (char *)malloc(1024 * 1024);
	if (!src_buf)
	{
		printf("sprite update error: fail to get memory for tmpdata\n");
		goto _update_error_;
	}

	ret = card_part_info(&img_start, &part_size, "sysrecovery");
	if (ret)
	{
		printf("sprite update error: read image start error\n");
    	goto _update_error_;
	}
	printf("part start = %d\n", img_start);
	imghd = Img_Open_from_sysrecovery(img_start);
	if (!imghd)
	{
		printf("sprite update error: fail to open img\n");
		goto _update_error_;
	}
	__imagehd(imghd);

	sprite_cartoon_upgrade(10);

	//erase the data partition.
	ret = card_part_info(&img_start, &part_size, "data");
	if (ret)
	{
		printf("sprite update error: no data part found\n");
	}
	else
	{
		__u32 tmp_size;
		__u32 tmp_start;

		tmp_start = img_start;
		tmp_size = part_size;
		printf("data part size=%d\n", tmp_size);
		printf("begin erase part data\n");
		memset(src_buf, 0xff, 1024 * 1024);
		sunxi_flash_write(tmp_start, 1024 * 1024/512, src_buf);
		printf("finish erase part data\n");
	}

	dl_info = (sunxi_download_info  *)malloc(sizeof(sunxi_download_info ));
	if (!dl_info) 
	{
		printf("sprite update error: fail to get memory for download map\n");
		goto _update_error_;
	}
	memset(dl_info, 0, sizeof(sunxi_download_info ));

	ret = sprite_card_fetch_download_map(dl_info);
	if (ret) {
		printf("sunxi sprite error: donn't download dl_map\n");
		goto _update_error_;
	}
	sprite_cartoon_upgrade(20);


	if (sunxi_sprite_deal_part_from_sysrevoery(dl_info))
	{
		printf("sunxi sprite error : download part error\n");
		return -1;
	}

	if(sunxi_sprite_deal_recorvery_boot(production_media))
	{
		printf("recovery error : download uboot or boot0 error!\n");
		return -1;
	}
	tick_printf("successed in downloading uboot and boot0\n");
	sprite_cartoon_upgrade(100);
	__msdelay(3000);

	Img_Close(imghd);
	if (dl_info)
	{
		free(dl_info);
	}
	if (src_buf)
	{
		free(src_buf);
	}

	sunxi_board_restart(0);
	return 0;
	
_update_error_:
	if (dl_info)
	{
		free(dl_info);
	}
	if (src_buf)
	{
		free(src_buf);
	}
	printf("sprite update error: current card sprite failed\n");
	printf("now hold the machine\n");
	return -1;
}
