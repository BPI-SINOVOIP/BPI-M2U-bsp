/*
 * Nand_physic_inc.h
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
#define __NAND_PHYSIC_INCLUDE_H__

#ifndef _NAND_PHYSIC_INC_H
#define _NAND_PHYSIC_INC_H

#include "nand_type.h"
#include "./controller/nand_controller.h"
#include "nand_drv_cfg.h"
#include "../nand_osal.h"
#include "../phy/phy.h"
#include "uboot_head.h"

#ifndef GLOBAL
#define GLOBAL extern
#endif

GLOBAL int ndfc_change_page_size(struct _nand_controller_info *nctri);
GLOBAL int ndfc_get_rb_sta(struct _nand_controller_info *nctri, u32 rb);
GLOBAL void ndfc_enable_ecc(struct _nand_controller_info *nctri,
	u32 pipline, u32 randomize);
GLOBAL void ndfc_disable_ecc(struct _nand_controller_info *nctri);
GLOBAL void ndfc_repeat_mode_enable(struct _nand_controller_info *nctri);
GLOBAL void ndfc_repeat_mode_disable(struct _nand_controller_info *nctri);
GLOBAL void ndfc_disable_randomize(struct _nand_controller_info *nctri);
GLOBAL void ndfc_set_rand_seed(struct _nand_controller_info *nctri,
	u32 page_no);
GLOBAL void ndfc_set_default_rand_seed(struct _nand_controller_info *nctri);
GLOBAL void ndfc_enable_randomize(struct _nand_controller_info *nctri);
GLOBAL void ndfc_set_ecc_mode(struct _nand_controller_info *nctri, u8 ecc_mode);
GLOBAL void ndfc_select_chip(struct _nand_controller_info *nctri, u32 chip);
GLOBAL void ndfc_select_rb(struct _nand_controller_info *nctri, u32 rb);
GLOBAL int _normal_cmd_io_send(struct _nand_controller_info *nctri,
	struct _nctri_cmd *icmd);
GLOBAL int _normal_cmd_io_wait(struct _nand_controller_info *nctri,
	struct _nctri_cmd *icmd);
GLOBAL int _batch_cmd_io_send(struct _nand_controller_info *nctri,
	struct _nctri_cmd_seq *cmd_list);
GLOBAL int _batch_cmd_io_wait(struct _nand_controller_info *nctri,
	struct _nctri_cmd_seq *cmd_list);
GLOBAL void ndfc_clean_cmd_seq(struct _nctri_cmd_seq *cmd_list);
GLOBAL void print_cmd_seq(struct _nctri_cmd_seq *cmd_list);
GLOBAL int ndfc_execute_cmd(struct _nand_controller_info *nctri,
	struct _nctri_cmd_seq *cmd_list);
GLOBAL int ndfc_wait_all_rb_ready(struct _nand_controller_info *nctri);
GLOBAL int ndfc_get_spare_data(struct _nand_controller_info *nctri,
	u32 *sbuf, u32 ecc_blk_cnt);
GLOBAL int ndfc_set_spare_data(struct _nand_controller_info *nctri,
	u32 *sbuf, u32 ecc_blk_cnt);
GLOBAL int init_nctri(struct _nand_controller_info *nctri);
GLOBAL int ndfc_check_ecc(struct _nand_controller_info *nctri,
	u32 eblock_cnt);
GLOBAL int ndfc_update_ecc_sta_and_spare_data(struct _nand_physic_op_par *npo,
	int ecc_sta, unsigned char *sbuf);
GLOBAL int wait_reg_status(volatile u32 *reg, u32 mark, u32 value, u32 us);
GLOBAL int ndfc_change_nand_interface(struct _nand_controller_info *nctri,
	u32 ddr_type, u32 sdr_edo, u32 ddr_edo, u32 ddr_delay);
GLOBAL void ndfc_get_nand_interface(struct _nand_controller_info *nctri,
	u32 *pddr_type, u32 *psdr_edo, u32 *pddr_edo, u32 *pddr_delay);
GLOBAL void ndfc_print_reg(struct _nand_controller_info *nctri);
GLOBAL void ndfc_print_save_reg(struct _nand_controller_info *nctri);
GLOBAL void _setup_ndfc_ddr2_para(struct _nand_controller_info *nctri);
GLOBAL void _set_ndfc_def_timing_param(struct _nand_chip_info *nci);
GLOBAL int ndfc_is_toogle_interface(struct _nand_controller_info *nctri);
GLOBAL int ndfc_set_legacy_interface(struct _nand_controller_info *nctri);
GLOBAL int ndfc_set_toogle_interface(struct _nand_controller_info *nctri);
GLOBAL u32 ndfc_get_selected_rb_no(struct _nand_controller_info *nctri);
GLOBAL u32 ndfc_check_rb_b2r_int_occur(struct _nand_controller_info *nctri);
GLOBAL void ndfc_clear_rb_b2r_int(struct _nand_controller_info *nctri);
GLOBAL void ndfc_disable_rb_b2r_int(struct _nand_controller_info *nctri);
GLOBAL u32 ndfc_check_dma_int_occur(struct _nand_controller_info *nctri);
GLOBAL void ndfc_clear_dma_int(struct _nand_controller_info *nctri);
GLOBAL void ndfc_disable_dma_int(struct _nand_controller_info *nctri);
GLOBAL void ndfc_enable_dma_int(struct _nand_controller_info *nctri);
GLOBAL int ndfc_get_ecc_mode(struct _nand_controller_info *nctri);
GLOBAL int save_nctri(struct _nand_controller_info *nctri);
GLOBAL int recover_nctri(struct _nand_controller_info *nctri);
GLOBAL u32 ndfc_get_page_size(struct _nand_controller_info *nctri);
GLOBAL int ndfc_set_page_size(
	struct _nand_controller_info *nctri, u32 page_size);
GLOBAL int ndfc_soft_reset(struct _nand_controller_info *nctri);

GLOBAL int nand_physic_init(void);
GLOBAL int init_parameter(void);
GLOBAL int nand_physic_exit(void);
GLOBAL int nand_init_temp_buf(struct _nand_temp_buf *nand_temp_buf);
GLOBAL u8 *nand_get_temp_buf(unsigned int size);
GLOBAL int nand_free_temp_buf(unsigned char *buf, unsigned int size);

GLOBAL struct _nand_info *NandHwInit(void);
GLOBAL int NandHwExit(void);
GLOBAL int NandHwSuperStandby(void);
GLOBAL int NandHwSuperResume(void);
GLOBAL int NandHwNormalStandby(void);
GLOBAL int NandHwNormalResume(void);
GLOBAL int NandHwShutDown(void);
GLOBAL void *NAND_GetIOBaseAddr(u32 no);

GLOBAL void do_nand_interrupt(u32 no);

GLOBAL int nand_write_nboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_read_nboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_write_uboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_read_uboot_data(unsigned char *buf, unsigned int len);
GLOBAL int nand_check_nboot(unsigned char *buf, unsigned int len);
GLOBAL int nand_check_uboot(unsigned char *buf, unsigned int len);
GLOBAL int nand_get_param(boot_nand_para_t *nand_param);
GLOBAL int NAND_UpdatePhyArch(void);
GLOBAL int nand_uboot_erase_all_chip(unsigned int force_flag);
GLOBAL int nand_erase_chip(unsigned int chip, unsigned int start_block,
	unsigned int end_block, unsigned int force_flag);
GLOBAL int nand_dragonborad_test_one(unsigned char *buf,
	unsigned char *oob, unsigned int blk_num);
GLOBAL void print_uboot_info(struct _uboot_info *uboot);
GLOBAL int uboot_info_init(struct _uboot_info *uboot);

GLOBAL int nand_get_uboot_total_len(void);
GLOBAL int nand_get_nboot_total_len(void);
GLOBAL int init_phyinfo_buf(void);
GLOBAL int get_uboot_offset(void *buf);
GLOBAL int is_uboot_magic(void *buf);
GLOBAL int check_magic_physic_info(unsigned int *mem_base, char *magic);
GLOBAL int check_sum_physic_info(unsigned int *mem_base, unsigned int size);
GLOBAL int physic_info_get_offset(unsigned int *pages_offset);
GLOBAL int physic_info_get_one_copy(unsigned int start_block,
	unsigned int pages_offset, unsigned int *block_per_copy,
	unsigned int *buf);
GLOBAL int clean_physic_info(void);
GLOBAL int physic_info_read(void);
GLOBAL int physic_info_add_to_uboot_tail(unsigned int *buf_dst,
	unsigned int uboot_size);
GLOBAL unsigned int  is_uboot_block(unsigned int  block, char *uboot_buf);
GLOBAL int is_phyinfo_empty(struct _boot_info *info);
GLOBAL int add_len_to_uboot_tail(unsigned int uboot_size);
GLOBAL int print_physic_info(struct _boot_info *info);
GLOBAL unsigned int  cal_sum_physic_info(struct _boot_info *mem_base,
	unsigned int  size);
GLOBAL int change_uboot_start_block(struct _boot_info *info,
	unsigned int start_block);

GLOBAL struct _nand_lib_cfg *g_phy_cfg;
GLOBAL struct _nand_storage_info *g_nsi;
GLOBAL struct _nand_super_storage_info *g_nssi;
GLOBAL struct _nand_controller_info *g_nctri;
GLOBAL struct __NandStorageInfo_t *g_nand_storage_info;
GLOBAL struct _nand_physic_function nand_physic_function[];
GLOBAL struct _nand_permanent_data nand_permanent_data;
GLOBAL int (*nand_physic_special_init)(void);
GLOBAL int (*nand_physic_special_exit)(void);

GLOBAL struct _nand_storage_info g_nsi_data;
GLOBAL struct _nand_super_storage_info g_nssi_data;
GLOBAL struct _nand_controller_info g_nctri_data[2];
GLOBAL struct __NandStorageInfo_t g_nand_storage_info_data;
GLOBAL struct _nand_chip_info nci_data[MAX_CHIP_PER_CHANNEL * 2];
GLOBAL struct _nand_super_chip_info nsci_data[MAX_CHIP_PER_CHANNEL * 2];
GLOBAL struct _nand_temp_buf ntf;
GLOBAL struct _uboot_info uboot_info;
GLOBAL struct _nand_info aw_nand_info;
GLOBAL struct _boot_info *phyinfo_buf;

GLOBAL int nand_physic_temp1;
GLOBAL int nand_physic_temp2;
GLOBAL int nand_physic_temp3;
GLOBAL int nand_physic_temp4;
GLOBAL int nand_physic_temp5;
GLOBAL int nand_physic_temp6;
GLOBAL int nand_physic_temp7;
GLOBAL int nand_physic_temp8;

GLOBAL int add_to_nctri(struct _nand_controller_info *node);
GLOBAL struct _nand_controller_info *nctri_get(
	struct _nand_controller_info *head, unsigned int num);
GLOBAL int update_nctri(struct _nand_controller_info *nctri);
GLOBAL int set_nand_script_frequence(void);
GLOBAL int check_nctri(struct _nand_controller_info *nctri);
GLOBAL int _check_scan_data(u32 first_check, u32 chip,
	u32 *scan_good_blk_no, u8 *main_buf);


GLOBAL int nci_add_to_nsi(struct _nand_storage_info *nsi,
	struct _nand_chip_info *node);
GLOBAL int nci_add_to_nctri(struct _nand_controller_info *nctri,
	struct _nand_chip_info *node);
GLOBAL struct _nand_chip_info *nci_get_from_nsi(
	struct _nand_storage_info *nsi, unsigned int num);
GLOBAL struct _nand_chip_info *nci_get_from_nctri(
	struct _nand_controller_info *nctri, unsigned int num);
GLOBAL struct _nand_phy_info_par *search_id(char *id);
GLOBAL int init_nci_from_id(struct _nand_chip_info *nci,
	struct _nand_phy_info_par *npi);
GLOBAL int nand_build_nsi(struct _nand_storage_info *nsi,
	struct _nand_controller_info *nctri);
GLOBAL int read_nand_structure(void *phy_arch, unsigned int *good_blk_no);
GLOBAL int write_nand_structure(void *phy_arch, unsigned int good_blk_no,
	unsigned int blk_cnt);
GLOBAL int set_nand_structure(void *phy_arch);
GLOBAL unsigned int NAND_GetLsbblksize(void);
GLOBAL unsigned int NAND_GetPhyblksize(void);

GLOBAL int nsci_add_to_nssi(struct _nand_super_storage_info *nssi,
	struct _nand_super_chip_info *node);
GLOBAL struct _nand_super_chip_info *nsci_get_from_nssi(
	struct _nand_super_storage_info *nssi, unsigned int num);
GLOBAL int init_nsci_from_nctri(struct _nand_super_storage_info *nssi,
	struct _nand_super_chip_info *nsci, struct _nand_controller_info *nctri,
	unsigned int channel_num, unsigned int chip_no,
	unsigned int nsci_num_in_nctri);
GLOBAL int nand_build_nssi(struct _nand_super_storage_info *nssi,
	struct _nand_controller_info *nctri);
GLOBAL int nand_build_storage_info(void);

GLOBAL int nand_physic_erase_super_block(unsigned int chip,
	unsigned int block);
GLOBAL int nand_physic_read_super_page(unsigned int chip, unsigned int block,
	unsigned int page, unsigned int bitmap,
	unsigned char *mbuf, unsigned char *sbuf);
GLOBAL int nand_physic_write_super_page(unsigned int chip, unsigned int block,
	unsigned int page, unsigned int bitmap,
	unsigned char *mbuf, unsigned char *sbuf);
GLOBAL int nand_physic_super_bad_block_check(unsigned int chip,
	unsigned int block);
GLOBAL int nand_physic_super_bad_block_mark(unsigned int chip,
	unsigned int block);

GLOBAL int nand_secure_storage_block;
GLOBAL int nand_secure_storage_block_bak;

GLOBAL int nand_secure_storage_read(int item, unsigned char *buf,
	unsigned int len);
GLOBAL int nand_secure_storage_write(int item, unsigned char *buf,
	unsigned int len);

GLOBAL int nand_secure_storage_init(int flag);
GLOBAL int nand_secure_storage_first_build(unsigned int start_block);
GLOBAL int get_nand_secure_storage_max_item(void);
GLOBAL int is_nand_secure_storage_build(void);

GLOBAL int nand_secure_storage_write_init(unsigned int block);
GLOBAL unsigned int nand_secure_check_sum(unsigned char *mbuf,
	unsigned int len);
GLOBAL int nand_secure_storage_read_one(unsigned int block, int item,
	unsigned char *mbuf, unsigned int len);
GLOBAL int nand_secure_storage_check(void);
GLOBAL int nand_secure_storage_update(void);
GLOBAL int nand_secure_storage_repair(int flag);

GLOBAL int nand_physic_bad_block_mark(unsigned int chip, unsigned int block);
GLOBAL int nand_physic_bad_block_check(unsigned int chip, unsigned int block);
GLOBAL int nand_physic_write_page(unsigned int chip, unsigned int block,
	unsigned int page, unsigned int bitmap,
	unsigned char *mbuf, unsigned char *sbuf);
GLOBAL int nand_physic_erase_block(unsigned int chip, unsigned int block);
GLOBAL int nand_physic_read_page(unsigned int chip, unsigned int block,
	unsigned int page, unsigned int bitmap,
	unsigned char *mbuf, unsigned char *sbuf);
GLOBAL int nand_write_data_in_whole_block(unsigned int chip,
	unsigned int block, unsigned char *mbuf, unsigned int mlen,
	unsigned char *sbuf, unsigned int slen);
GLOBAL int nand_read_data_in_whole_block(unsigned int chip,
	unsigned int block, unsigned char *mbuf,
	unsigned int mlen, unsigned char *sbuf, unsigned int slen);
GLOBAL int nand_physic_block_copy(unsigned int chip_s,
	unsigned int block_s, unsigned int chip_d, unsigned int block_d);

GLOBAL int set_ce_rb_for_chip(struct _nand_chip_info *nci,
	unsigned int channel, unsigned int chips);
GLOBAL void nand_enable_chip(struct _nand_chip_info *nci);
GLOBAL void nand_disable_chip(struct _nand_chip_info *nci);
GLOBAL int nand_reset_chip(struct _nand_chip_info *nci);
GLOBAL int nand_first_reset_chip(struct _nand_chip_info *nci,
	unsigned int chip_no);
GLOBAL int nand_read_chip_status(struct _nand_chip_info *nci,
	u8 *pstatus);
GLOBAL int nand_read_chip_status_ready(struct _nand_chip_info *nci);
GLOBAL int is_chip_rb_ready(struct _nand_chip_info *nci);
GLOBAL int nand_read_id(struct _nand_chip_info *nci, unsigned char *id);
GLOBAL int nand_first_read_id(struct _nand_chip_info *nci,
	unsigned int chip_no, char *id);
GLOBAL unsigned int get_row_addr(unsigned int page_offset_for_next_blk,
	unsigned int block, unsigned int page);
GLOBAL int fill_cmd_addr(u32 col_addr, u32 col_cycle, u32 row_addr,
	u32 row_cycle, u8 *abuf);
GLOBAL void set_default_batch_read_cmd_seq(struct _nctri_cmd_seq *cmd_seq);
GLOBAL void set_default_batch_write_cmd_seq(struct _nctri_cmd_seq *cmd_seq,
	u32 write_cmd1, u32 write_cmd2);
GLOBAL int _change_all_nand_parameter(struct _nand_controller_info *nctri,
	u32 ddr_type, u32 pre_ddr_type, u32 dclk);
GLOBAL int _get_right_timing_para(struct _nand_controller_info *nctri,
	u32 ddr_type, u32 *good_sdr_edo, u32 *good_ddr_edo,
	u32 *good_ddr_delay);
GLOBAL int set_cmd_with_nand_bus(struct _nand_chip_info *nci, u8 *cmd,
	u32 wait_rb, u8 *addr, u8 *dat, u32 dat_len, u32 counter);
GLOBAL int get_data_with_nand_bus_one_cmd(struct _nand_chip_info *nci,
	u8 *cmd, u8 *addr, u8 *dat, u32 dat_len);
GLOBAL int set_one_cmd(struct _nand_chip_info *nci, u8 cmd, u32 wait_rb);
GLOBAL int set_one_addr(struct _nand_chip_info *nci, u8 addr);

GLOBAL int nand_reset_super_chip(struct _nand_super_chip_info *nsci,
	unsigned int super_chip_no);
GLOBAL int nand_read_super_chip_status(struct _nand_super_chip_info *nsci,
	unsigned int super_chip_no);
GLOBAL int is_super_chip_rb_ready(struct _nand_super_chip_info *nsci,
	unsigned int super_chip_no);
GLOBAL int nand_wait_all_rb_ready(void);

GLOBAL int nand_code_info(void);
GLOBAL void show_nctri(struct _nand_controller_info *nctri_head);
GLOBAL void show_nci(struct _nand_chip_info *nci_head);
GLOBAL void show_nsi(void);
GLOBAL void show_nssi(void);
GLOBAL void show_static_info(void);
GLOBAL void show_spare(int flag);
GLOBAL int nand_get_version(__u8 *nand_version);
GLOBAL int nand_update_end(void);
GLOBAL void nand_show_data(uchar *buf, u32 len);

GLOBAL int m0_read_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m0_write_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m0_read_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);
GLOBAL int m0_write_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);
GLOBAL int _generate_page_map_tab(__u32 nand_page_size, __u32 copy_cnt,
	__u32 page_cnt, __u32 *page_addr,
	__u32 *page_map_tab_addr, __u32 *tab_size);

GLOBAL int m0_use_chip_function(struct _nand_physic_op_par *npo,
	unsigned int function);
GLOBAL int m0_rw_use_chip_function(struct _nand_physic_op_par *npo,
	unsigned int function);
GLOBAL int m0_erase_super_block(struct _nand_physic_op_par *npo);
GLOBAL int m0_read_super_page(struct _nand_physic_op_par *npo);
GLOBAL int m0_write_super_page(struct _nand_physic_op_par *npo);
GLOBAL int m0_super_bad_block_check(struct _nand_physic_op_par *npo);
GLOBAL int m0_super_bad_block_mark(struct _nand_physic_op_par *npo);

GLOBAL int (*function_read_page_end)(struct _nand_physic_op_par *npo);

GLOBAL int m0_special_init(void);
GLOBAL int m0_special_exit(void);
GLOBAL int m0_erase_block(struct _nand_physic_op_par *npo);
GLOBAL int m0_erase_block_start(struct _nand_physic_op_par *npo);
GLOBAL int m0_read_page_start(struct _nand_physic_op_par *npo);
GLOBAL int m0_read_page_end_not_retry(struct _nand_physic_op_par *npo);
GLOBAL int m0_read_page_end(struct _nand_physic_op_par *npo);
GLOBAL int m0_read_page(struct _nand_physic_op_par *npo);
GLOBAL int m0_read_two_plane_page(struct _nand_physic_op_par *npo);
GLOBAL int m0_write_page_start(struct _nand_physic_op_par *npo, int plane_no);
GLOBAL int m0_write_page_end(struct _nand_physic_op_par *npo);
GLOBAL int m0_write_page(struct _nand_physic_op_par *npo);
GLOBAL int m0_write_two_plane_page(struct _nand_physic_op_par *npo);
GLOBAL int m0_bad_block_check(struct _nand_physic_op_par *npo);
GLOBAL int m0_bad_block_mark(struct _nand_physic_op_par *npo);
GLOBAL int m0_is_lsb_page(__u32 page_num);

GLOBAL int m1_read_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m1_write_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m1_read_boot0_one(unsigned char *buf,
	unsigned int len, unsigned int counter);
GLOBAL int m1_write_boot0_one(unsigned char *buf,
	unsigned int len, unsigned int counter);

GLOBAL int m8_read_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m8_write_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m8_read_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);
GLOBAL int m8_write_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);

GLOBAL u8 m1_read_retry_mode;
GLOBAL u8 m1_read_retry_cycle;
GLOBAL u8 m1_read_retry_reg_num;
GLOBAL u8 m1_read_retry_reg_adr[4];


GLOBAL int m1_set_readretry(struct _nand_chip_info *nci);
GLOBAL int m1_readretry_init(struct _nand_chip_info *nci);
GLOBAL int m1_readretry_exit(struct _nand_chip_info *nci);
GLOBAL int m1_special_init(void);
GLOBAL int m1_special_exit(void);
GLOBAL int m1_write_page_FF(struct _nand_physic_op_par *npo, u32 page_size_k);
GLOBAL int m1_read_page_end(struct _nand_physic_op_par *npo);
GLOBAL int m1_setdefaultparam(struct _nand_chip_info *nci);
GLOBAL int m1_is_lsb_page(__u32 page_num);

GLOBAL int m2_read_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m2_write_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m2_read_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);
GLOBAL int m2_write_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);
GLOBAL int m2_is_lsb_page(__u32 page_num);

GLOBAL u8 m2_read_retry_mode;
GLOBAL u8 m2_read_retry_cycle;
GLOBAL u8 m2_read_retry_reg_num;
GLOBAL u8 m2_read_retry_reg_adr[8];

GLOBAL u8 m2_lsb_mode_reg_adr[4];
GLOBAL u8 m2_lsb_mode_default_val[4];
GLOBAL u8 m2_lsb_mode_val[4];
GLOBAL u8 m2_lsb_mode_reg_num;

GLOBAL int m2_set_readretry(struct _nand_chip_info *nci);
GLOBAL int m2_readretry_init(struct _nand_chip_info *nci);
GLOBAL int m2_readretry_exit(struct _nand_chip_info *nci);
GLOBAL int m2_special_init(void);
GLOBAL int m2_special_exit(void);
GLOBAL int m2_read_page_end(struct _nand_physic_op_par *npo);
GLOBAL int m2_setdefaultparam(struct _nand_chip_info *nci);
GLOBAL int m2_lsb_init(struct _nand_chip_info *nci);
GLOBAL int m2_lsb_enable(struct _nand_chip_info *nci);
GLOBAL int m2_lsb_disable(struct _nand_chip_info *nci);
GLOBAL int m2_lsb_exit(struct _nand_chip_info *nci);

GLOBAL int m3_read_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m3_write_boot0_page(struct _nand_chip_info *nci,
	struct _nand_physic_op_par *npo);
GLOBAL int m3_read_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);
GLOBAL int m3_write_boot0_one(unsigned char *buf, unsigned int len,
	unsigned int counter);

GLOBAL u8 m3_read_retry_mode;
GLOBAL u8 m3_read_retry_cycle;
GLOBAL u8 m3_read_retry_reg_num;
GLOBAL u8 m3_read_retry_reg_adr[4];
GLOBAL s16 m3_read_retry_val[7][4];

GLOBAL u8 m3_lsb_mode_reg_adr[5];
GLOBAL u8 m3_lsb_mode_default_val[5];
GLOBAL u8 m3_lsb_mode_val[5];
GLOBAL u8 m3_lsb_mode_reg_num;

GLOBAL int m3_set_readretry(struct _nand_chip_info *nci);
GLOBAL int m3_readretry_init(struct _nand_chip_info *nci);
GLOBAL int m3_readretry_exit(struct _nand_chip_info *nci);
GLOBAL int m3_special_init(void);
GLOBAL int m3_special_exit(void);
GLOBAL int m3_lsb_init(struct _nand_chip_info *nci);
GLOBAL int m3_lsb_enable(struct _nand_chip_info *nci);
GLOBAL int m3_lsb_disable(struct _nand_chip_info *nci);
GLOBAL int m3_lsb_exit(struct _nand_chip_info *nci);
GLOBAL int m3_is_lsb_page(__u32 page_num);

GLOBAL u8 m4_read_retry_mode;
GLOBAL u8 m4_read_retry_cycle;

GLOBAL int m4_set_readretry(struct _nand_chip_info *nci);
GLOBAL int m4_readretry_init(struct _nand_chip_info *nci);
GLOBAL int m4_readretry_exit(struct _nand_chip_info *nci);
GLOBAL int m4_special_init(void);
GLOBAL int m4_special_exit(void);
GLOBAL int m4_0x40_is_lsb_page(__u32 page_num);
GLOBAL int m4_0x41_is_lsb_page(__u32 page_num);
GLOBAL int m4_0x42_is_lsb_page(__u32 page_num);

GLOBAL u8 m5_read_retry_mode;
GLOBAL u8 m5_read_retry_cycle;

GLOBAL int m5_set_readretry(struct _nand_chip_info *nci);
GLOBAL int m5_readretry_init(struct _nand_chip_info *nci);
GLOBAL int m5_readretry_exit(struct _nand_chip_info *nci);
GLOBAL int m5_special_init(void);
GLOBAL int m5_special_exit(void);
GLOBAL int m5_is_lsb_page(__u32 page_num);

GLOBAL u8 m6_read_retry_cycle;

GLOBAL int m6_set_readretry(struct _nand_chip_info *nci);
GLOBAL int m6_readretry_init(struct _nand_chip_info *nci);
GLOBAL int m6_readretry_exit(struct _nand_chip_info *nci);
GLOBAL int m6_special_init(void);
GLOBAL int m6_special_exit(void);
GLOBAL int m6_is_lsb_page(__u32 page_num);

GLOBAL u8 m7_read_retry_mode;
GLOBAL u8 m7_read_retry_cycle;

GLOBAL int m7_set_readretry(struct _nand_chip_info *nci, u16 page);
GLOBAL int m7_readretry_init(struct _nand_chip_info *nci);
GLOBAL int m7_readretry_exit(struct _nand_chip_info *nci);
GLOBAL int m7_special_init(void);
GLOBAL int m7_special_exit(void);
GLOBAL int m7_is_lsb_page(__u32 page_num);

GLOBAL int m9_special_init(void);
GLOBAL int m9_special_exit(void);

#endif /*__NAND_PHYSIC_INCLUDE_H__*/
