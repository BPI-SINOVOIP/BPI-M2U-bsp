/*
 * The driver of sunxi key ladder.
 *
 * Copyright (C) 2017-2020 Allwinner.
 *
 * zhouhuacai <zhouhuacai@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include "sunxi_ss.h"
#include "sunxi_ss_reg.h"
#include "sunxi_ss_proc.h"
#include "sunxi_ss_kl.h"
#include <linux/sunxi_ss_kl_ioctl.h>

static struct ss_kl_s *ss_kl;
static struct sunxi_kl_paras *kl_para;

static DEFINE_MUTEX(kl_lock);

void ss_kl_lock(void)
{
	mutex_lock(&kl_lock);
}

void ss_kl_unlock(void)
{
	mutex_unlock(&kl_lock);
}

static void
ss_kl_paras_init(struct sunxi_kl_paras *kl_para, struct ss_kl_s *ss_kl)
{
	kl_para->status = ss_kl->status;
	kl_para->vid_phys = virt_to_phys(ss_kl->vid);
	kl_para->mid_phys = virt_to_phys(ss_kl->mid);
	kl_para->ek_pthys = virt_to_phys(ss_kl->ek);
	kl_para->nonce_phys = virt_to_phys(ss_kl->nonce);
	kl_para->da_nonce_phys = virt_to_phys(ss_kl->da_nonce);
}

static int ss_kl_set_info(const void *param)
{
	if (copy_from_user((void *)ss_kl, (void __user *)param,
				sizeof(struct ss_kl_s)))
		return -EFAULT;

	ss_kl_paras_init(kl_para, ss_kl);

	return ss_kl_smc_call(TEESMC_KL_SET_INFO, virt_to_phys(kl_para));
}

static int ss_kl_set_ekey_info(const void *param)
{
	if (copy_from_user((void *)ss_kl->ek, (void __user *)param,
			sizeof(ss_kl->ek)))
		return -EFAULT;

	return ss_kl_smc_call(TEESMC_KL_SET_EKEY, 0);
}

static int ss_kl_get_info(void *param)
{
	if (ss_kl_smc_call(TEESMC_KL_GET_INFO, 0))
		return -ENXIO;

	if (copy_to_user((void __user *)param, ss_kl, sizeof(struct ss_kl_s)))
		return -EFAULT;

	return 0;
}

static int ss_kl_gen_all(void)
{
	return ss_kl_smc_call(TEESMC_KL_GEN_ALL, 0);
}

static int sunxi_ss_kl_open(struct inode *inode, struct file *file)
{
	int ret;

	ss_kl = kzalloc(sizeof(struct ss_kl_s), GFP_KERNEL);
	if (!ss_kl)
		return -ENOMEM;

	kl_para = kzalloc(sizeof(struct sunxi_kl_paras), GFP_KERNEL);
	if (!kl_para) {
		ret = -ENOMEM;
		goto err_out;
	}

	ret = ss_kl_smc_call(TEESMC_KL_DEV_OPEN, 0);
	if (!ret)
		return 0;

	kfree(kl_para);
err_out:
	kfree(ss_kl);
	return ret;
}

static int sunxi_ss_kl_release(struct inode *inode, struct file *file)
{
	kfree(ss_kl);
	kfree(kl_para);

	return ss_kl_smc_call(TEESMC_KL_DEV_RELEASE, 0);
}

static long sunxi_ss_kl_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	int ret = 0;

	ss_kl_lock();

	KL_DBG("sunxi_ss_kl_ioctl cmd: 0x%x\n", cmd);

	switch (cmd) {
	case KL_SET_INFO:
		ret = ss_kl_set_info((void *)arg);
		break;
	case KL_SET_KEY_INFO:
		ret = ss_kl_set_ekey_info((void *)arg);
		break;
	case KL_GET_INFO:
		ret = ss_kl_get_info((void *)arg);
		break;
	case KL_GEN_ALL:
		ret = ss_kl_gen_all();
		break;
	default:
		KL_ERR("Unsupported cmd:%d\n", cmd);
		ret = -EINVAL;
		break;
	}
	ss_kl_unlock();

	return ret;
}

static const struct file_operations sunxi_ss_kl_ops = {
	.owner   = THIS_MODULE,
	.open    = sunxi_ss_kl_open,
	.release = sunxi_ss_kl_release,
	.unlocked_ioctl = sunxi_ss_kl_ioctl,
};

static struct class kl_class = {
		.name           = "ss_key_ladder",
		.owner          = THIS_MODULE,
};

struct miscdevice ss_kl_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sunxi_key_ladder",
	.fops  = &sunxi_ss_kl_ops,
};

int __init sunxi_ss_kl_init(void)
{
	s32 ret = 0;

	ret = class_register(&kl_class);
	if (ret != 0)
		return ret;

	ret = misc_register(&ss_kl_device);
	if (ret != 0) {
		KL_ERR("misc_register() failed!(%d)\n", ret);
		class_unregister(&kl_class);
		return ret;
	}
	return ret;
}

void __exit sunxi_ss_kl_exit(void)
{
	if (misc_deregister(&ss_kl_device))
		KL_ERR("misc_deregister() failed!\n");
	class_unregister(&kl_class);
}

MODULE_DESCRIPTION("Sunxi keyladder driver");
MODULE_AUTHOR("zhouhuacai");
MODULE_LICENSE("GPL");

