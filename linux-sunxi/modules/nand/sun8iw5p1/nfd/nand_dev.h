/*
 * nand_dev.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __NAND_DEV_H__
#define __NAND_DEV_H__

struct nand_kobject {
	struct kobject kobj;
	struct _nftl_blk *nftl_blk;
	char name[32];
};

extern unsigned int do_static_wear_leveling(void *zone);
extern unsigned int static_wear_leveling(void *zone);
extern unsigned short nftl_get_zone_write_cache_nums(void *_zone);
extern unsigned int garbage_collect(void *zone);
extern unsigned int do_prio_gc(void *zone);


extern struct nand_blk_ops mytr;
extern struct kobj_type ktype;

extern struct _nand_partition *build_nand_partition(struct _nand_phy_partition
						    *phy_partition);
extern void add_nftl_blk_list(struct _nftl_blk *head,
			      struct _nftl_blk *nftl_blk);
extern struct _nftl_blk *del_last_nftl_blk(struct _nftl_blk *head);
extern int add_nand_blktrans_dev(struct nand_blk_dev *dev);
extern int add_nand_blktrans_dev_for_dragonboard(struct nand_blk_dev *dev);
extern int del_nand_blktrans_dev(struct nand_blk_dev *dev);
extern struct _nand_disk *get_disk_from_phy_partition(struct _nand_phy_partition
						      *phy_partition);
extern uint16 get_partitionNO(struct _nand_phy_partition *phy_partition);
extern int nftl_exit(struct _nftl_blk *nftl_blk);
extern struct _nftl_blk *get_nftl_need_read_claim(struct _nftl_blk *start_blk);
extern int read_reclaim(struct _nftl_blk *start_blk, struct _nftl_blk *nftl_blk,
			uchar *buf);

int _dev_nand_read(struct _nand_dev *nand_dev, __u32 start_sector, __u32 len,
		   unsigned char *buf);
int _dev_nand_write(struct _nand_dev *nand_dev, __u32 start_sector, __u32 len,
		    unsigned char *buf);
int _dev_nand_discard(struct _nand_dev *nand_dev, __u32 start_sector,
		      __u32 len);
int _dev_flush_write_cache(struct _nand_dev *nand_dev, __u32 num);
int _dev_flush_sector_write_cache(struct _nand_dev *nand_dev, __u32 num);
int dev_initialize(struct _nand_dev *nand_dev, struct _nftl_blk *nftl_blk,
		   __u32 offset, __u32 size);
int nand_flush(struct nand_blk_dev *dev);
int add_nand(struct nand_blk_ops *tr,
	     struct _nand_phy_partition *phy_partition);
int remove_nand(struct nand_blk_ops *tr);
unsigned int nand_read_reclaim(struct _nftl_blk *nftl_blk, unsigned char *buf);


extern uint32 gc_all(void *zone);
extern uint32 gc_one(void *zone);
extern uint32 prio_gc_one(void *zone, uint16 block, uint32 flag);
extern void print_nftl_zone(void *zone);
extern void print_free_list(void *zone);
extern void print_smart(void *zone);
extern void print_block_invalid_list(void *zone);
extern void print_logic_page_map(void *_zone, uint32 num);
extern uint32 nftl_set_zone_test(void *_zone, uint32 num);
extern int nand_dbg_phy_read(unsigned short nDieNum, unsigned short nBlkNum,
			     unsigned short nPage);
extern int nand_dbg_zone_phy_read(void *zone, uint16 block, uint16 page);
extern int nand_dbg_zone_erase(void *zone, uint16 block, uint16 erase_num);
extern int nand_dbg_zone_phy_write(void *zone, uint16 block, uint16 page);
extern int nand_dbg_phy_write(unsigned short nDieNum, unsigned short nBlkNum,
			      unsigned short nPage);
extern int nand_dbg_phy_erase(unsigned short nDieNum, unsigned short nBlkNum);
extern int nand_dbg_single_phy_erase(unsigned short nDieNum,
				     unsigned short nBlkNum);
extern int _dev_nand_read2(char *name, __u32 start_sector, __u32 len,
			   unsigned char *buf);

extern void nand_phy_test(void);
extern int nand_check_table(void *zone);

extern void udisk_test_start(struct _nftl_blk *nftl_blk);
extern void udisk_test_stop(void);
extern int debug_data;
extern int _dev_nand_read2(char *name, unsigned int start_sector,
			   unsigned int len, unsigned char *buf);
extern int _dev_nand_write2(char *name, unsigned int start_sector,
			    unsigned int len, unsigned char *buf);
void obj_test_release(struct kobject *kobject);
void udisk_test_speed(struct _nftl_blk *nftl_blk);


/*****************************************************************************/
extern int get_nand_secure_storage_max_item(void);
extern int nand_secure_storage_read(int item, unsigned char *buf,
				    unsigned int len);
extern int nand_secure_storage_write(int item, unsigned char *buf,
				     unsigned int len);

extern int NAND_ReadBoot0(unsigned int length, void *buf);
extern int NAND_ReadBoot1(unsigned int length, void *buf);
extern int NAND_BurnBoot0(unsigned int length, void *buf);
extern int NAND_BurnBoot1(unsigned int length, void *buf);

extern struct _nand_info *p_nand_info;

extern int add_nand(struct nand_blk_ops *tr,
		    struct _nand_phy_partition *phy_partition);
extern int add_nand_for_dragonboard_test(struct nand_blk_ops *tr);
extern int remove_nand(struct nand_blk_ops *tr);
extern int nand_flush(struct nand_blk_dev *dev);
extern struct _nand_phy_partition *get_head_phy_partition_from_nand_info(struct
								_nand_info
								 *nand_info);
extern struct _nand_phy_partition *get_next_phy_partition(struct
							  _nand_phy_partition
							  *phy_partition);

/*****************************************************************************/

#endif

