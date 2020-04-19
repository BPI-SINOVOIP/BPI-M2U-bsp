#ifndef _LINUX_FIVM_H
#define _LINUX_FIVM_H

#include <linux/fs.h>
#define CONFIG_FILE_INTEGRITY
struct fivm_operation {
	int (*mmap_verify)(struct file * , unsigned long);
	int (*open_verify)(struct file *, const char *, int);
};

extern int fivm_mmap_verify(struct file *, unsigned long);
extern int fivm_open_verify(struct file *, const char *, int);

#endif /*_LINUX_FIVM_H*/
