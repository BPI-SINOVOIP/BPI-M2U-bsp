#ifndef __NAND_CLASS_H__
#define __NAND_CLASS_H__

#include "nand_blk.h"
#include "nand_dev.h"

extern int debug_data;

static ssize_t nand_test_store(struct kobject *kobject, struct attribute *attr,
			       const char *buf, size_t count);
static ssize_t nand_test_show(struct kobject *kobject, struct attribute *attr,
			      char *buf);
void obj_test_release(struct kobject *kobject);
void udisk_test_speed(struct _nftl_blk *nftl_blk);

int g_iShowVar = -1;

struct attribute prompt_attr = {
	.name = "nand_debug",
	.mode = S_IRUGO | S_IWUSR | S_IWGRP | S_IROTH
};

static struct attribute *def_attrs[] = {
	&prompt_attr,
	NULL
};

static const struct sysfs_ops obj_test_sysops = {
	.show = nand_test_show,
	.store = nand_test_store
};

struct kobj_type ktype = {
	.release = obj_test_release,
	.sysfs_ops = &obj_test_sysops,
	.default_attrs = def_attrs
};


extern int _dev_nand_read2(char *name, unsigned int start_sector,
				unsigned int len, unsigned char *buf);
extern int _dev_nand_write2(char *name, unsigned int start_sector,
				unsigned int len, unsigned char *buf);



#endif
