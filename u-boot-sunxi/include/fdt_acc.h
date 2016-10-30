#ifndef __DTBFAST_H_
#define __DTBFAST_H_

#include <linux/types.h>
#include <sys_config.h>

#define  DTBFAST_HEAD_MAX_DEPTH       (16)


struct dtbfast_header {
	uint32_t magic;			 /* magic word FDT_MAGIC */
	uint32_t totalsize;		 /* total file size */
	uint32_t level0_count;	 /* the count of level0 head */
	uint32_t off_head;    	 /* offset to head */
	uint32_t head_count;		 /* total head */
	uint32_t off_prop;		 /* offset to prop */
	uint32_t prop_count;		 /* total prop */
	uint32_t reserved[9];

};


struct head_node {
	uint32_t  name_sum;		//�������Ƶ�ÿ���ַ����ۼӺ�
	uint32_t  name_sum_short; //������@֮���ַ����ۼӺ�
	uint32_t  name_offset;	//�������Ƶ�ƫ��������dtb��Ѱ��
	uint32_t  name_bytes;		//�������Ƶĳ���
	uint32_t  name_bytes_short;//@֮ǰ���Ƶĳ���
	uint32_t  repeate_count;
	uint32_t  head_offset;	//ָ���һ��head��offset
	uint32_t  head_count;		//head�ܵĸ���
	uint32_t  data_offset;	//ָ���һ��prop��offset
	uint32_t  data_count;		//prop�ܵĸ���
	uint32_t  reserved[2];
};

struct prop_node {
	uint32_t  name_sum;		//���Ƶ�ÿ���ַ����ۼӺ�
	uint32_t  name_offset;	//�������Ƶ�ƫ������dtb��Ѱ��
	uint32_t  name_bytes;		//�������Ƶĳ���
	uint32_t  offset;			//����prop��ƫ������dtb��Ѱ��
};


int fdtfast_path_offset(const void *fdt, const char *path);

int fdtfast_set_node_status(void *fdt, int nodeoffset, enum fdt_status status, unsigned int error_code);

int fdtfast_setprop_u32(void *fdt, int nodeoffset, const char *name,
				  uint32_t val);

int fdtfast_getprop_u32(const void *fdt, int nodeoffset,
			const char *name, uint32_t *val);

int fdtfast_getprop_string(const void *fdt, int nodeoffset,
			const char *name, char **val);

int fdtfast_getprop_gpio(const void *fdt, int nodeoffset,
		const char* prop_name,	user_gpio_set_t* gpio_list);

#endif /* __DTBFAST_H_ */
