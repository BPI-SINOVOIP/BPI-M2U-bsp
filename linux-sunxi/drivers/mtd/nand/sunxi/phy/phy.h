/*
 * Phy.h
 *
 * Copyright (C) 2017 AllWinnertech Ltd.
 * Author: zhongguizhao <zhongguizhao@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _PHY_H
#define _PHY_H

#include "mbr.h"
#include "../physic_v2/nand_type.h"
#include "../physic_v2/uboot_head.h"
#include <linux/mtd/nand.h>

#define PHY_RESERVED_BLOCK_RATIO	256
#define MAX_PHY_RESERVED_BLOCK		64
#define MIN_PHY_RESERVED_BLOCK		8
#define MIN_PHY_RESERVED_BLOCK_V2	0

#define FACTORY_BAD_BLOCK_SIZE		2048
#define PHY_PARTITION_BAD_BLOCK_SIZE	4096
#define PARTITION_BAD_BLOCK_SIZE	4096

#define MBR_DATA_SIZE

struct _nand_lib_cfg {
	unsigned int	phy_interface_cfg;

	unsigned int	phy_support_two_plane;
	unsigned int	phy_nand_support_vertical_interleave;
	unsigned int	phy_support_dual_channel;

	unsigned int	phy_wait_rb_before;
	unsigned int	phy_wait_rb_mode;
	unsigned int	phy_wait_dma_mode;
};

struct _nand_phy_partition;

struct _nand_super_block {
	unsigned short	Block_NO;
	unsigned short	Chip_NO;
};

struct _nand_disk {
	unsigned int	size;
	/* unsigned int offset; */
	unsigned int	type;
	unsigned  char	name[PARTITION_NAME_SIZE];
};

struct _partition {
	struct _nand_disk nand_disk[MAX_PART_COUNT_PER_FTL];
	unsigned int size;
	unsigned int cross_talk;
	unsigned int attribute;
	struct _nand_super_block start;
	struct _nand_super_block end;
	/* unsigned int offset; */
};

struct _bad_block_list {
	struct _nand_super_block super_block;
	struct _nand_super_block *next;
};

typedef union {
	unsigned char ndata[4096];
	PARTITION_MBR data;
} _MBR;

typedef union {
	unsigned char ndata[2048 + 512];
	struct _partition data[MAX_PARTITION];
} _PARTITION;

typedef union {
	unsigned char ndata[512];
	struct _nand_super_storage_info data;
} _NAND_STORAGE_INFO;

typedef union {
	unsigned char ndata[2048];
	struct _nand_super_block data[512];
} _FACTORY_BLOCK;

typedef union {
	unsigned char ndata[256];
	struct _uboot_info data;
} _UBOOT_INFO;

struct _boot_info {
	unsigned int magic;
	unsigned int len;
	unsigned int sum;

	unsigned int no_use_block;
	unsigned int uboot_start_block;
	unsigned int uboot_next_block;
	unsigned int logic_start_block;
	unsigned int nouse[128 - 7];

	_MBR mbr;				/* 1k */
	_PARTITION partition;			/* 0.5k */
	_NAND_STORAGE_INFO storage_info;	/* 0.5k */
	_FACTORY_BLOCK factory_block;		/* 1k */
	/* _UBOOT_INFO uboot_info; */		/* 0.25K*/
};


struct _nand_info {
	unsigned short			type;
	unsigned short			SectorNumsPerPage;
	unsigned short			BytesUserData;
	unsigned short			PageNumsPerBlk;
	unsigned short			BlkPerChip;
	unsigned short			ChipNum;
	unsigned short			FirstBuild;
	unsigned short			new_bad_page_addr;
	unsigned long long		FullBitmap;
	struct _nand_super_block	mbr_block_addr;
	struct _nand_super_block	bad_block_addr;
	struct _nand_super_block	new_bad_block_addr;
	struct _nand_super_block	no_used_block_addr;
	/* struct _bad_block_list	*new_bad_block_addr; */
	struct _nand_super_block	*factory_bad_block;
	struct _nand_super_block	*new_bad_block;
	unsigned char			*temp_page_buf;
	unsigned char			*mbr_data;
	struct _nand_phy_partition	*phy_partition_head;
	struct _partition		partition[MAX_PARTITION];
	struct _nand_lib_cfg		nand_lib_cfg;
	unsigned short			partition_nums;
	unsigned short			cache_level;
	unsigned short			capacity_level;

	unsigned short			mini_free_block_first_reserved;
	unsigned short			mini_free_block_reserved;

	unsigned int			MaxBlkEraseTimes;
	unsigned int			EnableReadReclaim;

	unsigned int			read_claim_interval;

	struct _boot_info		*boot;
};

struct _nand_phy_partition {
	unsigned short			PartitionNO;
	unsigned short			CrossTalk;
	unsigned short			SectorNumsPerPage;
	unsigned short			BytesUserData;
	unsigned short			PageNumsPerBlk;
	unsigned short			TotalBlkNum;
	unsigned short			GoodBlockNum;
	unsigned short			FullBitmapPerPage;
	unsigned short			FreeBlock;
	unsigned int			Attribute;
	unsigned int			TotalSectors;
	struct _nand_super_block	StartBlock;
	struct _nand_super_block	EndBlock;
	struct _nand_info		*nand_info;
	struct _nand_super_block	*factory_bad_block;
	struct _nand_super_block	*new_bad_block;
	struct _nand_phy_partition	*next_phy_partition;
	struct _nand_disk		*disk;

	int (*page_read)(unsigned short nDieNum, unsigned short nBlkNum,
	unsigned short nPage, unsigned short SectBitmap, void *pBuf,
		void *pSpare);
	int (*page_write)(unsigned short nDieNum, unsigned short nBlkNum,
	unsigned short nPage, unsigned short SectBitmap, void *pBuf,
		void *pSpare);
	int (*block_erase)(unsigned short nDieNum, unsigned short nBlkNum);
};

struct _nand_partition_page {
	unsigned short	Page_NO;
	unsigned short	Block_NO;
};

struct _physic_par {
	struct _nand_partition_page	phy_page;
	unsigned short			page_bitmap;
	unsigned char			*main_data_addr;
	unsigned char			*spare_data_addr;
};

struct _nand_partition {
	char				name[32];
	unsigned short			sectors_per_page;
	unsigned short			spare_bytes;
	unsigned short			pages_per_block;
	unsigned short			total_blocks;
	unsigned short			bytes_per_page;
	unsigned int			bytes_per_block;
	unsigned short			full_bitmap;
	unsigned long long		cap_by_sectors;
	unsigned long long		cap_by_bytes;
	unsigned long long		total_by_bytes;
	struct _nand_phy_partition	*phy_partition;

	int (*nand_erase_superblk)(struct _nand_partition *nand,
		struct _physic_par *p);
	int (*nand_read_page)(struct _nand_partition *nand,
		struct _physic_par *p);
	int (*nand_write_page)(struct _nand_partition *nand,
		struct _physic_par *p);
	int (*nand_is_blk_good)(struct _nand_partition *nand,
		struct _physic_par *p);
	int (*nand_mark_bad_blk)(struct _nand_partition *nand,
		struct _physic_par *p);
};

#endif
