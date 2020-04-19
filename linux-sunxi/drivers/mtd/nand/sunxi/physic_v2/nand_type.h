/*
****************************************************************************
* eNand
* Nand flash driver scan module
*
* Copyright(C), 2008-2009, SoftWinners Microelectronic Co., Ltd.
* All Rights Reserved
*
* File Name : nand_type.h
*
* Author :
*
* Version : v0.1
*
* Date : 2013-11-20
*
* Description :
*
* Others : None at present.
*
*
*
****************************************************************************
*/

#ifndef _NAND_TYPE_H_
#define _NAND_TYPE_H_

#include "nand_drv_cfg.h"


#define OOB_BUF_SIZE				32

#define FACTORY_BAD_BLOCK_ERROR			159
#define BYTES_PER_SECTOR			512
#define SHIFT_PER_SECTOR			9
#define BYTES_OF_USER_PER_PAGE			16
#define MIN_BYTES_OF_USER_PER_PAGE		16


#define ZONE_RESERVED_BLOCK_RATIO		16
#define SYS_ZONE_RESERVED_BLOCK_RATIO		4
#define MIN_FREE_BLOCK_NUM			20
#define MIN_FREE_BLOCK				2

#define SYS_RESERVED_BLOCK_RATIO		5
#define NORM_RESERVED_BLOCK_RATIO		10
#define MIN_FREE_BLOCK_NUM_RUNNING		15
#define MIN_FREE_BLOCK_NUM_V2			25
#define MAX_FREE_BLOCK_NUM			120
#define MIN_FREE_BLOCK_REMAIN			4

/*
 * extern __s32 printk(const char *format, ...);
 *
 * #define CHENEY_NDBG(args...) printk(KERN_ERR "[Cheney] " args);
 * #define CHENEY_NERR(args...) printk(KERN_ERR "[Cheney] " args);
*/

typedef struct {
	unsigned int		ChannelCnt;
	unsigned int		ChipCnt;
	unsigned int		ChipConnectInfo;
	unsigned int		RbCnt;
	unsigned int		RbConnectInfo;
	unsigned int		RbConnectMode;
	unsigned int		BankCntPerChip;
	unsigned int		DieCntPerChip;
	unsigned int		PlaneCntPerDie;
	unsigned int		SectorCntPerPage;
	unsigned int		PageCntPerPhyBlk;
	unsigned int		BlkCntPerDie;
	unsigned int		OperationOpt;
	unsigned int		FrequencePar;
	unsigned int		EccMode;
	unsigned char		NandChipId[8];
	unsigned int		ValidBlkRatio;
	unsigned int		good_block_ratio;
	unsigned int		ReadRetryType;
	unsigned int		DDRType;
	unsigned int		Reserved[32];
} boot_nand_para_t;

struct _optional_phy_op_par {
	unsigned char		 multi_plane_read_cmd[2];
	unsigned char		 multi_plane_write_cmd[2];
	unsigned char		 multi_plane_copy_read_cmd[3];
	unsigned char		 multi_plane_copy_write_cmd[3];
	unsigned char		 multi_plane_status_cmd;
	unsigned char		 inter_bnk0_status_cmd;
	unsigned char		 inter_bnk1_status_cmd;
	unsigned char		 bad_block_flag_position;
	unsigned short		 multi_plane_block_offset;
};

struct _nfc_init_ddr_info {
	unsigned int en_dqs_c;
	unsigned int en_re_c;
	unsigned int odt;
	unsigned int en_ext_verf;
	unsigned int dout_re_warmup_cycle;
	unsigned int din_dqs_warmup_cycle;
	unsigned int output_driver_strength;
	unsigned int rb_pull_down_strength;
};

struct __NandStorageInfo_t {
	unsigned int		ChannelCnt;
	unsigned int		ChipCnt;
	unsigned int		ChipConnectInfo;
	unsigned int		RbCnt;
	unsigned int		RbConnectInfo;
	unsigned int		RbConnectMode;
	unsigned int		BankCntPerChip;
	unsigned int		DieCntPerChip;
	unsigned int		PlaneCntPerDie;
	unsigned int		SectorCntPerPage;
	unsigned int		PageCntPerPhyBlk;
	unsigned int		BlkCntPerDie;
	unsigned int		OperationOpt;
	unsigned int		FrequencePar;
	unsigned int		EccMode;
	unsigned char		NandChipId[8];
	unsigned int		ValidBlkRatio;
	unsigned int		ReadRetryType;
	unsigned int		DDRType;
	struct _optional_phy_op_par OptPhyOpPar;
};

struct _nand_phy_info_par {
	char		   nand_id[8];

	unsigned int	   die_cnt_per_chip;
	unsigned int	   sect_cnt_per_page;
	unsigned int	   page_cnt_per_blk;
	unsigned int	   blk_cnt_per_die;
	unsigned int	   operation_opt;
	unsigned int	   valid_blk_ratio;
	unsigned int	   access_freq;

/* the ecc mode for the nand flash chip
 * 0: bch-16, 1:bch-28, 2:bch_28 3:32 4:40 5:48 6:56 7:60 8:64 9:72
 */
	unsigned int	   ecc_mode;
	unsigned int	   read_retry_type;
	unsigned int	   ddr_type;
	unsigned int	   option_phyisc_op_no;
	unsigned int	   ddr_info_no;
	unsigned int	   id_number;
	unsigned int	   max_blk_erase_times;
	unsigned int	   driver_no;
	unsigned int	   access_high_freq;
};

struct _nand_physic_op_par {
	unsigned int		  chip;
	unsigned int		  block;
	unsigned int		  page;
	unsigned int		  sect_bitmap;
	unsigned char		  *mdata;
	unsigned char		  *sdata;
	unsigned int		  slen;
};

struct _nand_chip_info;

struct _nand_physic_function {
	int (*nand_physic_erase_block)(struct _nand_physic_op_par *npo);
	int (*nand_physic_read_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_write_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_bad_block_check)(struct _nand_physic_op_par *npo);
	int (*nand_physic_bad_block_mark)(struct _nand_physic_op_par *npo);

	int (*nand_physic_erase_super_block)(struct _nand_physic_op_par *npo);
	int (*nand_physic_read_super_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_write_super_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_super_bad_block_check)(
		struct _nand_physic_op_par *npo);

	int (*nand_physic_super_bad_block_mark)(
		struct _nand_physic_op_par *npo);

	int (*nand_read_boot0_page)(struct _nand_chip_info *nci,
		struct _nand_physic_op_par *npo);
	int (*nand_write_boot0_page)(struct _nand_chip_info *nci,
		struct _nand_physic_op_par *npo);

	int (*nand_read_boot0_one)(unsigned char *buf, unsigned int len,
		unsigned int counter);
	int (*nand_write_boot0_one)(unsigned char *buf, unsigned int len,
		unsigned int counter);

	int (*nand_physic_special_init)(void);
	int (*nand_physic_special_exit)(void);
	int (*is_lsb_page)(__u32 page_num);
};

struct _nand_chip_info {
	struct _nand_chip_info *nsi_next;
	struct _nand_chip_info *nctri_next;

	char		   id[8];
	unsigned int	   chip_no;
	unsigned int	   nctri_chip_no;

	unsigned int	   blk_cnt_per_chip;
	unsigned int	   sector_cnt_per_page;
	unsigned int	   page_cnt_per_blk;
	unsigned int	   page_offset_for_next_blk;

	unsigned int	   randomizer;
	unsigned int	   read_retry;
	unsigned char	   readretry_value[128];
	unsigned int	   retry_count;
	unsigned int	   lsb_page_type;

	unsigned int	   ecc_mode;
	unsigned int	   max_erase_times;
	unsigned int	   driver_no;

	unsigned int	   interface_type;
	unsigned int	   frequency;
	unsigned int	   timing_mode;

	unsigned int	   support_change_onfi_timing_mode;
	unsigned int	   support_ddr2_specific_cfg;
	unsigned int	   support_io_driver_strength;
	unsigned int	   support_vendor_specific_cfg;
	unsigned int	   support_onfi_sync_reset;
	unsigned int	   support_toggle_only;

	unsigned int	   page_addr_bytes;

	unsigned int	   sdata_bytes_per_page;
	unsigned int	   ecc_sector;

	struct _nand_super_chip_info *nsci;
	struct _nand_controller_info *nctri;
	struct _nand_phy_info_par *npi;
	struct _optional_phy_op_par *opt_phy_op_par;
	struct _nfc_init_ddr_info *nfc_init_ddr_info;

	int (*nand_physic_erase_block)(struct _nand_physic_op_par *npo);
	int (*nand_physic_read_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_write_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_bad_block_check)(struct _nand_physic_op_par *npo);
	int (*nand_physic_bad_block_mark)(struct _nand_physic_op_par *npo);

	int (*nand_read_boot0_page)(struct _nand_chip_info *nci,
		struct _nand_physic_op_par *npo);
	int (*nand_write_boot0_page)(struct _nand_chip_info *nci,
		struct _nand_physic_op_par *npo);

	int (*nand_read_boot0_one)(unsigned char *buf, unsigned int len,
		unsigned int counter);
	int (*nand_write_boot0_one)(unsigned char *buf, unsigned int len,
		unsigned int counter);
	int (*is_lsb_page)(__u32 page_num);
};

struct _nand_super_chip_info {
	struct _nand_super_chip_info *nssi_next;

	unsigned int	   chip_no;
	unsigned int	   blk_cnt_per_super_chip;
	unsigned int	   sector_cnt_per_super_page;
	unsigned int	   page_cnt_per_super_blk;
	unsigned int	   page_offset_for_next_super_blk;
/* unsigned int		 multi_plane_block_offset; */

	unsigned int	   spare_bytes;
	unsigned int	   channel_num;

	unsigned int	   two_plane;
	unsigned int	   vertical_interleave;
	unsigned int	   dual_channel;

	unsigned int	   driver_no;

	struct _nand_chip_info *nci_first;
	struct _nand_chip_info *v_intl_nci_1;
	struct _nand_chip_info *v_intl_nci_2;
	struct _nand_chip_info *d_channel_nci_1;
	struct _nand_chip_info *d_channel_nci_2;

	int (*nand_physic_erase_super_block)(struct _nand_physic_op_par *npo);
	int (*nand_physic_read_super_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_write_super_page)(struct _nand_physic_op_par *npo);
	int (*nand_physic_super_bad_block_check)(
		struct _nand_physic_op_par *npo);

	int (*nand_physic_super_bad_block_mark)(
		struct _nand_physic_op_par *npo);
};

struct _nand_storage_info {
	unsigned int	   chip_cnt;
	unsigned int	   block_nums;
	struct _nand_chip_info *nci;
};

struct _nand_super_storage_info {
	unsigned int	super_chip_cnt;
	unsigned int	super_block_nums;
	unsigned int	support_two_plane;
	unsigned int	support_v_interleave;
	unsigned int	support_dual_channel;
	struct _nand_super_chip_info *nsci;
};

#define MAGIC_DATA_FOR_PERMANENT_DATA	  (0xa5a5a5a5)
struct _nand_permanent_data {
	unsigned int magic_data;
	unsigned int support_two_plane;
	unsigned int support_vertical_interleave;
	unsigned int support_dual_channel;
	unsigned int reserved[64 - 4];
};

struct _nand_temp_buf {

#define	 NUM_16K_BUF	2
#define	 NUM_32K_BUF	4
#define	 NUM_64K_BUF	1
#define	 NUM_NEW_BUF	4

	unsigned int used_16k[NUM_16K_BUF];
	unsigned int used_16k_flag[NUM_16K_BUF];
	unsigned char *nand_temp_buf16k[NUM_16K_BUF];

	unsigned int used_32k[NUM_32K_BUF];
	unsigned int used_32k_flag[NUM_32K_BUF];
	unsigned char *nand_temp_buf32k[NUM_32K_BUF];

	unsigned int used_64k[NUM_64K_BUF];
	unsigned int used_64k_flag[NUM_64K_BUF];
	unsigned char *nand_temp_buf64k[NUM_64K_BUF];

	unsigned int used_new[NUM_NEW_BUF];
	unsigned int used_new_flag[NUM_NEW_BUF];
	unsigned char *nand_new_buf[NUM_NEW_BUF];
};

#ifndef NULL
#define NULL	(0)
#endif

/* define the mask for the nand flash optional operation */
#define NAND_CACHE_READ			(1 << 0)
#define NAND_CACHE_PROGRAM		(1 << 1)
#define NAND_MULTI_READ			(1 << 2)
#define NAND_MULTI_PROGRAM		(1 << 3)
#define NAND_PAGE_COPYBACK		(1 << 4)
#define NAND_INT_INTERLEAVE		(1 << 5)
#define NAND_EXT_INTERLEAVE		(1 << 6)
#define NAND_RANDOM			(1 << 7)
#define NAND_READ_RETRY			(1 << 8)
#define NAND_READ_UNIQUE_ID		(1 << 9)
#define NAND_PAGE_ADR_NO_SKIP		(1 << 10)
#define NAND_DIE_SKIP			(1 << 11)
/* #define NAND_LOG_BLOCK_MANAGE	(1 << 12) */
/* #define NAND_FORCE_WRITE_SYNC	(1 << 13) */
/* #define NAND_LOG_BLOCK_LSB_TYPE	(0xff << 16) */
#define NAND_LSB_PAGE_TYPE		(0xff << 12)
#define NAND_MAX_BLK_ERASE_CNT		(1 << 20)
#define NAND_READ_RECLAIM		(1 << 21)

#define NAND_TIMING_MODE		(1 << 24)
#define NAND_DDR2_CFG			(1 << 25)
#define NAND_IO_DRIVER_STRENGTH		(1 << 26)
#define NAND_VENDOR_SPECIFIC_CFG	(1 << 27)
#define NAND_ONFI_SYNC_RESET_OP		(1 << 28)

#define NAND_TOGGLE_ONLY		(1 << 29)

#define NAND_OP_TRUE			(0)
#define NAND_OP_FALSE			(-1)

#define ECC_LIMIT			10
#define ERR_ECC				(-2)
#define ERR_TIMEOUT			(-3)
#define ERR_PARA			(-4)

#define ERR_NO_10			(-10)
#define ERR_NO_11			(-11)
#define ERR_NO_12			(-12)
#define ERR_NO_13			(-13)
#define ERR_NO_14			(-14)
#define ERR_NO_15			(-15)
#define ERR_NO_16			(-16)
#define ERR_NO_17			(-17)
#define ERR_NO_18			(-18)
#define ERR_NO_19			(-19)
#define ERR_NO_20			(-20)
#define ERR_NO_21			(-21)
#define ERR_NO_22			(-22)
#define ERR_NO_23			(-23)
#define ERR_NO_24			(-24)
#define ERR_NO_25			(-25)
#define ERR_NO_26			(-26)
#define ERR_NO_27			(-27)
#define ERR_NO_28			(-28)
#define ERR_NO_29			(-29)
#define ERR_NO_30			(-30)
#define ERR_NO_31			(-31)
#define ERR_NO_32			(-32)
#define ERR_NO_33			(-33)
#define ERR_NO_34			(-34)
#define ERR_NO_35			(-35)
#define ERR_NO_36			(-36)
#define ERR_NO_37			(-37)
#define ERR_NO_38			(-38)
#define ERR_NO_39			(-39)
#define ERR_NO_40			(-40)
#define ERR_NO_41			(-41)
#define ERR_NO_42			(-42)
#define ERR_NO_43			(-43)
#define ERR_NO_44			(-44)
#define ERR_NO_45			(-45)
#define ERR_NO_46			(-46)
#define ERR_NO_47			(-47)
#define ERR_NO_48			(-48)
#define ERR_NO_49			(-49)
#define ERR_NO_50			(-50)
#define ERR_NO_51			(-51)
#define ERR_NO_52			(-52)
#define ERR_NO_53			(-53)
#define ERR_NO_54			(-54)
#define ERR_NO_55			(-55)
#define ERR_NO_56			(-56)
#define ERR_NO_57			(-57)
#define ERR_NO_58			(-58)
#define ERR_NO_59			(-59)
#define ERR_NO_60			(-60)
#define ERR_NO_61			(-61)
#define ERR_NO_62			(-62)
#define ERR_NO_63			(-63)
#define ERR_NO_64			(-64)
#define ERR_NO_65			(-65)
#define ERR_NO_66			(-66)
#define ERR_NO_67			(-67)
#define ERR_NO_68			(-68)
#define ERR_NO_69			(-69)
#define ERR_NO_70			(-70)
#define ERR_NO_71			(-71)
#define ERR_NO_72			(-72)
#define ERR_NO_73			(-73)
#define ERR_NO_74			(-74)
#define ERR_NO_75			(-75)
#define ERR_NO_76			(-76)
#define ERR_NO_77			(-77)
#define ERR_NO_78			(-78)
#define ERR_NO_79			(-79)
#define ERR_NO_80			(-80)
#define ERR_NO_81			(-81)
#define ERR_NO_82			(-82)
#define ERR_NO_83			(-83)
#define ERR_NO_84			(-84)
#define ERR_NO_85			(-85)
#define ERR_NO_86			(-86)
#define ERR_NO_87			(-87)
#define ERR_NO_88			(-88)
#define ERR_NO_89			(-89)
#define ERR_NO_90			(-90)
#define ERR_NO_91			(-91)
#define ERR_NO_92			(-92)
#define ERR_NO_93			(-93)
#define ERR_NO_94			(-94)
#define ERR_NO_95			(-95)
#define ERR_NO_96			(-96)
#define ERR_NO_97			(-97)
#define ERR_NO_98			(-98)
#define ERR_NO_99			(-99)
#define ERR_NO_100			(-100)
#define ERR_NO_101			(-101)
#define ERR_NO_102			(-102)
#define ERR_NO_103			(-103)
#define ERR_NO_104			(-104)
#define ERR_NO_105			(-105)
#define ERR_NO_106			(-106)
#define ERR_NO_107			(-107)
#define ERR_NO_108			(-108)
#define ERR_NO_109			(-109)
#define ERR_NO_110			(-110)
#define ERR_NO_111			(-111)
#define ERR_NO_112			(-112)
#define ERR_NO_113			(-113)
#define ERR_NO_114			(-114)
#define ERR_NO_115			(-115)
#define ERR_NO_116			(-116)
#define ERR_NO_117			(-117)
#define ERR_NO_118			(-118)
#define ERR_NO_119			(-119)
#define ERR_NO_120			(-120)
#define ERR_NO_121			(-121)
#define ERR_NO_122			(-122)
#define ERR_NO_123			(-123)
#define ERR_NO_124			(-124)
#define ERR_NO_125			(-125)
#define ERR_NO_126			(-126)
#define ERR_NO_127			(-127)
#define ERR_NO_128			(-128)
#define ERR_NO_129			(-129)
#define ERR_NO_130			(-130)
#define ERR_NO_131			(-131)
#define ERR_NO_132			(-132)
#define ERR_NO_133			(-133)

#endif
