/*
 * The ioctl interface of sunxi key ladder.
 *
 * Copyright (C) 2017-2020 Allwinner.
 *
 * zhouhuacai <zhouhuacai@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef _SUNXI_SS_KL_IOCTRL_H
#define _SUNXI_SS_KL_IOCTRL_H

#include <linux/types.h>

#define KL_MAX_KEY_LEVEL (5)
#define KL_KEY_SIZE (16)	/*byte in unit:128bits*/

enum {
	CMD_SET_INFO = 0,
	CMD_SET_KEY_INFO,
	CMD_GET_INFO,
	CMD_GEN_ALL,
};

struct ss_kl_status {
	unsigned int kl_level:8;	/*0:no kl; 1: 3stage; 2: 5stage*/
	unsigned int kl_alg_type:8;	/*0:AES; 1:DES; 2:TDES*/
	unsigned int kl_encrypt:8;	/*0: decrypt; 1: encrypt*/
	unsigned int kl_stu:8;
} ss_kl_status_s;

enum {
	SS_NO_KL = 0,
	SS_KL_LEVEL3,
	SS_KL_LEVEL5,
};


struct ss_kl_s {
	struct ss_kl_status status;
	unsigned char vid[KL_KEY_SIZE];
	unsigned char mid[KL_KEY_SIZE];
	unsigned char ek[KL_MAX_KEY_LEVEL][KL_KEY_SIZE]; /*ecw,ek1,ek2...ekn*/
	unsigned char nonce[KL_KEY_SIZE];
	unsigned char da_nonce[KL_KEY_SIZE]; /*challenge response output */
};

struct sunxi_kl_paras {
	struct ss_kl_status status;
	u32 vid_phys;
	u32 mid_phys;
	u32 ek_pthys;
	u32 nonce_phys;
	u32 da_nonce_phys;
};

#define KL_IOCTL 0xfb

#define	KL_SET_INFO	_IOWR(KL_IOCTL, CMD_SET_INFO, struct ss_kl_s)
#define KL_SET_KEY_INFO	_IOWR(KL_IOCTL, CMD_SET_KEY_INFO, struct ss_kl_s)
#define KL_GET_INFO	_IOWR(KL_IOCTL, CMD_GET_INFO, struct ss_kl_s)
#define KL_GEN_ALL	_IOWR(KL_IOCTL, CMD_GEN_ALL, struct ss_kl_s)

#endif /* _SUNXI_SS_KL_IOCTRL_H */
