/*
 * Copyright (C) Freescale Semiconductor, Inc. 2006.
 * Author: Jason Jin<Jason.jin@freescale.com>
 *         Zhang Wei<wei.zhang@freescale.com>
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
 *
 */
#ifndef __SECURE_STORAGE_H__
#define __SECURE_STORAGE_H__

extern int sunxi_secure_storage_init(void);
extern int sunxi_secure_storage_exit(void);

extern int sunxi_secure_storage_list(void);
extern int sunxi_secure_storage_probe(const char *item_name);
extern int sunxi_secure_storage_read(const char *item_name, char *buffer, int length, int *data_len);

extern int sunxi_secure_storage_write(const char *item_name, char *buffer, int length);
extern int sunxi_secure_storage_erase(const char *item_name);
extern int sunxi_secure_storage_erase_all(void);
extern int sunxi_secure_storage_erase_data_only(const char *item_name);

extern int sunxi_secure_object_down( const char *name , char *buf, int len, int encrypt, int write_protect);
extern int sunxi_secure_object_up(const char *name,char *buf,int len);

extern int sunxi_secure_object_set(const char *item_name, int encyrpt,int replace, int, int, int);
extern int sunxi_secure_object_write(const char *item_name, char *buffer, int length);
extern int sunxi_secure_object_read(const char *item_name, char *buffer, int buffer_len, int *data_len);


extern int smc_load_sst_encrypt(
		char *name,
		char *in, unsigned int len,
		char *out, unsigned int *outLen);

#define SUNXI_SECURE_STORTAGE_INFO_HEAD_LEN (64 + 4 + 4 + 4)
#define SUNXI_HDCP_KEY_LEN (288)
typedef struct
{
	char     name[64];      //key name
	uint32_t len;           //the len fo key_data
	uint32_t encrypted;
	uint32_t write_protect;
	char    key_data[4096 - SUNXI_SECURE_STORTAGE_INFO_HEAD_LEN];//the raw data of key
}
sunxi_secure_storage_info_t;

#endif
