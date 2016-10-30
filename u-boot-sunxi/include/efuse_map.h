#ifndef __KEY_H__
#define __KEY_H__

typedef struct EFUSE_KEY
{
	char name[32];
	int key_index;
	int store_max_bit;
	int show_bit_offset;
	int burned_bit_offset;
	int reserve[4];
}
efuse_key_map_t;

typedef struct
{
	char  name[64];
	u32 len;
	u32 res;
	u8  *key_data;
}
efuse_key_info_t;


#endif /*__KEY_H__*/

