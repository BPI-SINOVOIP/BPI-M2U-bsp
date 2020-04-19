/*
 * sunxi-di.c DE-Interlace driver
 *
 * Copyright (C) 2013-2015 allwinner.
 *	Ming Li<liming@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/major.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <asm/irq.h>
#include <linux/dma-buf.h>
#include "sunxi-di.h"

#define DI_MAX_USERS 1

static di_struct *di_data;
static s32 sunxi_di_major = -1;
static struct class *di_dev_class;
static struct workqueue_struct *di_wq;
static struct work_struct  di_work;
static struct timer_list *s_timer;
static struct device *di_device;
static struct clk *di_clk;
static struct clk *di_clk_source;
static struct device *iommu_dev;

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long long id;
};

static u32 debug_mask;

static ssize_t di_timeout_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = kstrtoul(buf, 10, &data);

	if (error) {
		pr_err("%s kstrtoul error\n", __func__);
		goto exit;
	}

	dprintk(DEBUG_DATA_INFO, "%s data = %ld\n", __func__, data);

	if (data)
		di_data->time_value = data;
	else
		di_data->time_value = DI_TIMEOUT;

	return count;

exit:
	return error;
}

static DEVICE_ATTR(timeout, 0664,
		NULL, di_timeout_store);

static struct attribute *di_attributes[] = {
	&dev_attr_timeout.attr,
	NULL
};

static struct attribute_group di_attribute_group = {
	.attrs = di_attributes
};

#ifdef DI_RESERVED_MEM
/* alloc based on 4K byte */
#define MY_BYTE_ALIGN(x) (((x + (4*1024-1)) >> 12) << 12)
void *sunxi_di_alloc(u32 num_bytes, unsigned long *phys_addr)
{
	u32 actual_bytes;
	void *address = NULL;

	if (num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);

		address = dma_alloc_coherent(iommu_dev, actual_bytes,
			    (dma_addr_t *)phys_addr, GFP_KERNEL);
		if (address) {
			dprintk(DEBUG_DATA_INFO,
			"dma_alloc_coherent ok, address=0x%p, size=0x%x\n",
			(void *)(*(unsigned long *)phys_addr), num_bytes);
			return address;
		} else {
			pr_err("dma_alloc_coherent fail, size=0x%x\n",
				num_bytes);
			return NULL;
		}
	} else {
		pr_err("%s size is zero\n", __func__);
	}

	return NULL;
}

void sunxi_di_free(void *virt_addr, unsigned long phys_addr, u32 num_bytes)
{
	u32 actual_bytes;

	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if (phys_addr && virt_addr)
		dma_free_coherent(iommu_dev, actual_bytes,
				virt_addr, (dma_addr_t)phys_addr);

	return;
}
#endif

static int di_mem_request(struct __di_mem_t *di_mem)
{
#ifndef DI_RESERVED_MEM
	unsigned map_size = 0;
	struct page *page;

	map_size = PAGE_ALIGN(di_mem->size);

	page = alloc_pages(GFP_KERNEL, get_order(map_size));
	if (page != NULL) {
		di_mem->v_addr = page_address(page);
		if (di_mem->v_addr == NULL)	{
			free_pages((unsigned long)(page), get_order(map_size));
			printk(KERN_ERR "page_address fail!\n");
			return -ENOMEM;
		}
		di_mem->p_addr = virt_to_phys(di_mem->v_addr);
		memset(di_mem->v_addr, 0, di_mem->size);

		dprintk(DEBUG_DATA_INFO, "pa=0x%p va=0x%p size:0x%x\n",
			(void *)di_mem->p_addr, di_mem->v_addr, di_mem->size);
		return 0;
	}	else {
		printk(KERN_ERR "alloc_pages fail!\n");
		return -ENOMEM;
	}
#else
	di_mem->v_addr = sunxi_di_alloc(di_mem->size,
			(unsigned long *)&di_mem->p_addr);
	if (di_mem->v_addr == NULL) {
		printk(KERN_ERR "%s: failed!\n", __func__);
		return -ENOMEM;
	}
	memset(di_mem->v_addr, 0, di_mem->size);
#endif
	return 0;
}

static int di_mem_release(struct __di_mem_t *di_mem)
{
#ifndef DI_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(di_mem->size);
	unsigned page_size = map_size;

	if (di_mem->v_addr == NULL) {
		printk(KERN_ERR "%s: failed!\n", __func__);
		return -1;
	}

	free_pages((unsigned long)(di_mem->v_addr), get_order(page_size));
	di_mem->v_addr = NULL;
#else
	if (di_mem->v_addr == NULL) {
		printk(KERN_ERR "%s: failed!\n", __func__);
		return -1;
	}
	sunxi_di_free(di_mem->v_addr, di_mem->p_addr, di_mem->size);
	di_mem->v_addr = NULL;
#endif
	return 0;
}

static void di_complete_check_set(s32 data)
{
	atomic_set(&di_data->di_complete, data);

	return;
}

static s32 di_complete_check_get(void)
{
	s32 data_temp = 0;

	data_temp = atomic_read(&di_data->di_complete);

	return data_temp;
}

static void di_timer_handle(unsigned long arg)
{
	u32 flag_size = 0;

	di_complete_check_set(DI_MODULE_TIMEOUT);
	wake_up_interruptible(&di_data->wait);
	flag_size = (FLAG_WIDTH*FLAG_HIGH)/4;
	di_irq_enable(0);
	di_irq_clear();
	di_reset();
	if (di_data->mem_in_params.v_addr != NULL)
		memset(di_data->mem_in_params.v_addr, 0, flag_size);
	if (di_data->mem_out_params.v_addr != NULL)
		memset(di_data->mem_out_params.v_addr, 0, flag_size);
	pr_err("di_timer_handle: timeout\n");
}

static void di_work_func(struct work_struct *work)
{
	del_timer_sync(s_timer);
	return;
}

static irqreturn_t di_irq_service(int irqno, void *dev_id)
{
	s32 ret;

	dprintk(DEBUG_INT, "%s: enter\n", __func__);
	di_irq_enable(0);
	ret = di_get_status();
	if (ret == 0) {
		di_complete_check_set(0);
		wake_up_interruptible(&di_data->wait);
		queue_work(di_wq, &di_work);
	} else {
		di_complete_check_set(-ret);
		wake_up_interruptible(&di_data->wait);
	}
	di_irq_clear();
	di_reset();
	return IRQ_HANDLED;
}

static s32 di_clk_cfg(struct device_node *node)
{
	unsigned long rate = 0;

	di_clk = of_clk_get(node, 0);
	if (!di_clk || IS_ERR(di_clk)) {
		printk(KERN_ERR "try to get di clock failed!\n");
		return -1;
	}

	di_clk_source = of_clk_get(node, 1);
	if (!di_clk_source || IS_ERR(di_clk_source)) {
		printk(KERN_ERR "try to get di_clk_source clock failed!\n");
		return -1;
	}

	rate = clk_get_rate(di_clk_source);
	dprintk(DEBUG_INIT, "%s: get di_clk_source rate %luHZ\n", __func__,
		rate);

	if (clk_set_parent(di_clk, di_clk_source)) {
		pr_err(
		"%s: set di_clk parent to di_clk_source failed!\n", __func__);
		return -1;
	}

#if defined CONFIG_ARCH_SUN9IW1P1
#else
	rate = rate/2;
	if (clk_set_rate(di_clk, rate)) {
		pr_err("set di clock freq to PLL_PERIPH0/2 failed!\n");
		return -1;
	}
	rate = clk_get_rate(di_clk);
	dprintk(DEBUG_INIT,
			"%s: get di_clk rate %dHZ\n", __func__, (__u32)rate);
#endif
	return 0;
}

static void di_clk_uncfg(void)
{
	if (di_clk == NULL || IS_ERR(di_clk)) {
		pr_err("di_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(di_clk);
		clk_put(di_clk);
		di_clk = NULL;
	}
	if (NULL == di_clk_source || IS_ERR(di_clk_source)) {
		pr_err("di_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(di_clk_source);
		di_clk_source = NULL;
	}
	return;
}

static s32 di_clk_enable(void)
{
	if (di_clk == NULL || IS_ERR(di_clk)) {
		pr_err("di_clk handle is invalid, just return!\n");
		return -1;
	} else {
		if (clk_prepare_enable(di_clk)) {
			pr_err("try to enable di_clk failed!\n");
			return -1;
		}
	}
	return 0;
}

static void di_clk_disable(void)
{
	if (di_clk == NULL || IS_ERR(di_clk)) {
		pr_err("di_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(di_clk);
	}
	return;
}

static s32 sunxi_di_params_init(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct resource *mem_res = NULL;
	s32 ret = 0;
	ret = di_clk_cfg(node);
	if (ret) {
		pr_err("%s: clk cfg failed.\n", __func__);
		goto clk_cfg_fail;
	}

	di_data->irq_number = platform_get_irq(pdev, 0);
	if (di_data->irq_number < 0) {
		pr_err("%s:get irq number failed!\n",  __func__);
		return -EINVAL;
	}
	if (request_irq(di_data->irq_number, di_irq_service, 0, "DE-Interlace",
			di_device)) {
		ret = -EBUSY;
		pr_err("%s: request irq failed.\n", __func__);
		goto request_irq_err;
	}

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		pr_err("%s: failed to get MEM res\n", __func__);
		ret = -ENXIO;
		goto mem_io_err;
	}

	if (!request_mem_region(mem_res->start, resource_size(mem_res),
			mem_res->name)) {
		pr_err("%s: failed to request mem region\n", __func__);
		ret = -EINVAL;
		goto mem_io_err;
	}

	di_data->base_addr = ioremap(mem_res->start, resource_size(mem_res));
	if (!di_data->base_addr) {
		pr_err("%s: failed to io remap\n", __func__);
		ret = -EIO;
		goto mem_io_err;
	}

	di_set_reg_base(di_data->base_addr);
	return 0;
mem_io_err:
	free_irq(di_data->irq_number, di_device);
request_irq_err:
	di_clk_uncfg();
clk_cfg_fail:
	return ret;
}

static void sunxi_di_params_exit(void)
{
	di_clk_uncfg();
	free_irq(di_data->irq_number, di_device);
	return;
}


#ifdef CONFIG_PM
static s32 sunxi_di_suspend(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "enter: sunxi_di_suspend.\n");

	del_timer_sync(s_timer);
	if (atomic_read(&di_data->enable)) {
		di_irq_enable(0);
		di_reset();
		di_internal_clk_disable();
		di_clk_disable();
		if (di_data->mem_in_params.v_addr != NULL)
		di_mem_release(&(di_data->mem_in_params));
		if (di_data->mem_out_params.v_addr != NULL)
		di_mem_release(&(di_data->mem_out_params));
	}

	return 0;
}

static s32 sunxi_di_resume(struct device *dev)
{
	s32 ret = 0;
	dprintk(DEBUG_SUSPEND, "enter: sunxi_di_resume.\n");

	if (atomic_read(&di_data->enable)) {
		add_timer(s_timer);
		ret = di_mem_request(&(di_data->mem_in_params));
		if (ret < 0) {
			pr_err("%s: request in_flag mem failed\n", __func__);
			return -1;
		} else {
			di_data->in_flag_phy =
					(void *)di_data->mem_in_params.p_addr;
		}
		ret = di_mem_request(&(di_data->mem_out_params));
		if (ret < 0) {
			pr_err("%s: request out_flag mem failed\n", __func__);
			di_mem_release(&(di_data->mem_in_params));
			return -1;
		} else {
			di_data->out_flag_phy =
					(void *)di_data->mem_out_params.p_addr;
		}
		ret = di_clk_enable();
		if (ret) {
			di_mem_release(&(di_data->mem_in_params));
			di_mem_release(&(di_data->mem_out_params));
			pr_err("%s: enable clk failed.\n", __func__);
			return ret;
		}
		di_internal_clk_enable();
		di_irq_enable(1);
		di_set_init();
	}
	return 0;
}
#endif

static void change_para(struct __di_para_t *di_para,
				struct __di_para_t2 *di_para2)
{
	int i;
	for (i = 0; i < 2; i++) {
		di_para2->input_fb.addr[i] =
			(unsigned long)di_para->input_fb.addr[i];
		di_para2->pre_fb.addr[i] =
			(unsigned long)di_para->pre_fb.addr[i];
		di_para2->output_fb.addr[i] =
			(unsigned long)di_para->output_fb.addr[i];
	}
	di_para2->input_fb.size.width = di_para->input_fb.size.width;
	di_para2->input_fb.size.height = di_para->input_fb.size.height;
	di_para2->input_fb.format = di_para->input_fb.format;

	di_para2->pre_fb.size.width = di_para->pre_fb.size.width;
	di_para2->pre_fb.size.height = di_para->pre_fb.size.height;
	di_para2->pre_fb.format = di_para->pre_fb.format;

	di_para2->source_regn.width = di_para->source_regn.width;
	di_para2->source_regn.height = di_para->source_regn.height;

	di_para2->output_fb.size.width = di_para->output_fb.size.width;
	di_para2->output_fb.size.height = di_para->output_fb.size.height;
	di_para2->output_fb.format = di_para->output_fb.format;
	di_para2->out_regn.width = di_para->out_regn.width;
	di_para2->out_regn.height =  di_para->out_regn.height;

	di_para2->field = di_para->field;
	di_para2->top_field_first = di_para->top_field_first;
}


#ifdef IOMMU_SUPPORT
static int di_dma_map(int fd, struct dmabuf_item *item)
{
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt, *sgt_bak;
	struct scatterlist *sgl, *sgl_bak;
	s32 sg_count = 0;
	int ret = -1;
	int i;
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	if (fd < 0) {
		pr_err("dma_buf_id(%d) is invalid\n", fd);
		goto exit;
	}
	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		pr_err("dma_buf_get failed\n");
		goto exit;
	}

	attachment = dma_buf_attach(dmabuf, iommu_dev);
	if (IS_ERR(attachment)) {
		pr_err("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		pr_err("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	/* create a private sgtable base on the given dmabuf */
	sgt_bak = kmalloc(sizeof(struct sg_table), GFP_KERNEL | __GFP_ZERO);
	if (sgt_bak == NULL) {
		pr_err("alloc sgt fail\n");
		goto err_buf_unmap;
	}
	ret = sg_alloc_table(sgt_bak, sgt->nents, GFP_KERNEL);
	if (ret != 0) {
		pr_err("alloc sgt fail\n");
		goto err_kfree;
	}
	sgl_bak = sgt_bak->sgl;
	for_each_sg(sgt->sgl, sgl, sgt->nents, i)  {
		sg_set_page(sgl_bak, sg_page(sgl), sgl->length, sgl->offset);
		sgl_bak = sg_next(sgl_bak);
	}

	sg_count = dma_map_sg_attrs(iommu_dev, sgt_bak->sgl,
			      sgt_bak->nents, DMA_FROM_DEVICE, &attrs);

	if (sg_count != 1) {
		pr_err("dma_map_sg failed:%d\n", sg_count);
		goto err_sgt_free;
	}

	item->fd = fd;
	item->buf = dmabuf;
	item->sgt = sgt_bak;
	item->attachment = attachment;
	item->dma_addr = sg_dma_address(sgt_bak->sgl);
	ret = 0;

	goto exit;

err_sgt_free:
	sg_free_table(sgt_bak);
err_kfree:
	kfree(sgt_bak);
err_buf_unmap:
	/* unmap attachment sgt, not sgt_bak, cause it's not alloc yet! */
	dma_buf_unmap_attachment(attachment, sgt, DMA_FROM_DEVICE);
err_buf_detach:
	dma_buf_detach(dmabuf, attachment);
err_buf_put:
	dma_buf_put(dmabuf);
exit:
	return ret;
}

static void di_dma_unmap(struct dmabuf_item *item)
{
	DEFINE_DMA_ATTRS(attrs);

	dma_set_attr(DMA_ATTR_SKIP_CPU_SYNC, &attrs);
	dma_unmap_sg_attrs(iommu_dev, item->sgt->sgl,
			      item->sgt->nents, DMA_FROM_DEVICE, &attrs);
	dma_buf_unmap_attachment(item->attachment, item->sgt, DMA_FROM_DEVICE);
	sg_free_table(item->sgt);
	kfree(item->sgt);
	dma_buf_detach(item->buf, item->attachment);
	dma_buf_put(item->buf);
}

static struct di_format_attr fmt_attr_tbl[] = {
	/*
	      format                            bits
						   hor_rsample(u,v)
							  ver_rsample(u,v)
								uvc
								   interleave
								       factor
									  div
	 */
	{ DI_FORMAT_YUV422P                     , 8,  2, 2, 1, 1, 0, 0, 2, 1},
	{ DI_FORMAT_YV12                        , 8,  2, 2, 2, 2, 0, 0, 3, 2},
	{ DI_FORMAT_YUV422_SP_UVUV              , 8,  2, 2, 1, 1, 1, 0, 2, 1},
	{ DI_FORMAT_YUV422_SP_VUVU              , 8,  2, 2, 1, 1, 1, 0, 2, 1},
	{ DI_FORMAT_NV12		        , 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ DI_FORMAT_NV21		        , 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ DI_FORMAT_MB32_12		        , 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ DI_FORMAT_MB32_21		        , 8,  2, 2, 2, 2, 1, 0, 3, 2},
};

s32 di_set_fb_info(struct __di_fb_t2 *di_para, struct dmabuf_item *item)
{
	s32 ret = -1;
	u32 i = 0;
	u32 len = ARRAY_SIZE(fmt_attr_tbl);
	u32 y_width, y_height, u_width, u_height;
	u32 y_pitch, u_pitch;
	u32 y_size, u_size;

	di_para->addr[0] = item->dma_addr;

	if (di_para->format >= DI_FORMAT_MAX) {
		pr_err("%s, format 0x%x is out of range\n", __func__,
			di_para->format);
		goto exit;
	}

	for (i = 0; i < len; ++i) {

		if (fmt_attr_tbl[i].format == di_para->format) {
			y_width = di_para->size.width;
			y_height = di_para->size.height;
			u_width = y_width/fmt_attr_tbl[i].hor_rsample_u;
			u_height = y_height/fmt_attr_tbl[i].ver_rsample_u;

			y_pitch = y_width;
			u_pitch = u_width * (fmt_attr_tbl[i].uvc + 1);

			y_size = y_pitch * y_height;
			u_size = u_pitch * u_height;
			di_para->addr[1] = di_para->addr[0] + y_size;
			di_para->addr[2] = di_para->addr[0] + y_size + u_size;

			ret = 0;
			break;
		}
	}
	if (ret != 0)
		pr_err("%s, format 0x%x is invalid\n", __func__,
			di_para->format);
exit:
	return ret;

}
#endif
static int di_process(struct __di_para_t2 *di_para, struct file *filp)
{
	int ret = 0;
	__u32 field = 0;
#ifdef IOMMU_SUPPORT
	struct dmabuf_item *cur_item = NULL;
	struct dmabuf_item *pre_item = NULL;
	struct dmabuf_item *next_item = NULL;
	struct dmabuf_item *out_item = NULL;
	cur_item = kmalloc(sizeof(struct dmabuf_item),
			      GFP_KERNEL | __GFP_ZERO);
	if (cur_item == NULL) {
		pr_err("malloc memory of size %ld fail!\n",
		       sizeof(struct dmabuf_item));
		goto EXIT;
	}
	pre_item = kmalloc(sizeof(struct dmabuf_item),
			      GFP_KERNEL | __GFP_ZERO);
	if (pre_item == NULL) {
		pr_err("malloc memory of size %ld fail!\n",
		       sizeof(struct dmabuf_item));
		goto FREE_CUR;
	}
	next_item = kmalloc(sizeof(struct dmabuf_item),
			      GFP_KERNEL | __GFP_ZERO);
	if (next_item == NULL) {
		pr_err("malloc memory of size %ld fail!\n",
		       sizeof(struct dmabuf_item));
		goto FREE_PRE;
	}
	out_item = kmalloc(sizeof(struct dmabuf_item),
			      GFP_KERNEL | __GFP_ZERO);
	if (out_item == NULL) {
		pr_err("malloc memory of size %ld fail!\n",
		       sizeof(struct dmabuf_item));
		goto FREE_NEXT;
	}

	ret = di_dma_map(di_para->input_fb.fd, cur_item);
	if (ret != 0) {
		pr_err("map cur_item fail!\n");
		goto FREE_OUT;
	}
	ret = di_dma_map(di_para->pre_fb.fd, pre_item);
	if (ret != 0) {
		pr_err("map pre_item fail!\n");
		goto CUR_DMA_UNMAP;
	}
	ret = di_dma_map(di_para->next_fb.fd, next_item);
	if (ret != 0) {
		pr_err("map next_item fail!\n");
		goto PRE_DMA_UNMAP;
	}
	ret = di_dma_map(di_para->output_fb.fd, out_item);
	if (ret != 0) {
		pr_err("map out_item fail!\n");
		goto NEXT_DMA_UNMAP;
	}

	di_set_fb_info(&di_para->input_fb, cur_item);
	di_set_fb_info(&di_para->pre_fb, pre_item);
	di_set_fb_info(&di_para->next_fb, next_item);
	di_set_fb_info(&di_para->output_fb, out_item);
#endif
	dprintk(DEBUG_DATA_INFO, "%s: input_fb.addr[0] = 0x%lx\n", __func__,
				(unsigned long)(di_para->input_fb.addr[0]));
	dprintk(DEBUG_DATA_INFO, "%s: input_fb.addr[1] = 0x%lx\n", __func__,
				(unsigned long)(di_para->input_fb.addr[1]));
	dprintk(DEBUG_DATA_INFO, "%s: input_fb.addr[2] = 0x%lx\n", __func__,
				(unsigned long)(di_para->input_fb.addr[2]));
	dprintk(DEBUG_DATA_INFO, "%s: input_fb.size.width = %d\n", __func__,
				di_para->input_fb.size.width);
	dprintk(DEBUG_DATA_INFO, "%s: input_fb.size.height = %d\n", __func__,
				di_para->input_fb.size.height);
	dprintk(DEBUG_DATA_INFO, "%s: input_fb.format = %d\n", __func__,
				di_para->input_fb.format);

	dprintk(DEBUG_DATA_INFO, "%s: pre_fb.addr[0] = 0x%lx\n", __func__,
				(unsigned long)(di_para->pre_fb.addr[0]));
	dprintk(DEBUG_DATA_INFO, "%s: pre_fb.addr[1] = 0x%lx\n", __func__,
				(unsigned long)(di_para->pre_fb.addr[1]));
	dprintk(DEBUG_DATA_INFO, "%s: pre_fb.addr[2] = 0x%lx\n", __func__,
				(unsigned long)(di_para->pre_fb.addr[2]));
	dprintk(DEBUG_DATA_INFO, "%s: pre_fb.size.width = %d\n", __func__,
				di_para->pre_fb.size.width);
	dprintk(DEBUG_DATA_INFO, "%s: pre_fb.size.height = %d\n", __func__,
				di_para->pre_fb.size.height);
	dprintk(DEBUG_DATA_INFO, "%s: pre_fb.format = %d\n", __func__,
				di_para->pre_fb.format);

	dprintk(DEBUG_DATA_INFO, "%s: next_fb.addr[0] = 0x%lx\n", __func__,
				(unsigned long)(di_para->next_fb.addr[0]));
	dprintk(DEBUG_DATA_INFO, "%s: next_fb.addr[1] = 0x%lx\n", __func__,
				(unsigned long)(di_para->next_fb.addr[1]));
	dprintk(DEBUG_DATA_INFO, "%s: next_fb.addr[2] = 0x%lx\n", __func__,
				(unsigned long)(di_para->next_fb.addr[2]));
	dprintk(DEBUG_DATA_INFO, "%s: next_fb.size.width = %d\n", __func__,
				di_para->next_fb.size.width);
	dprintk(DEBUG_DATA_INFO, "%s: next_fb.size.height = %d\n", __func__,
				di_para->next_fb.size.height);
	dprintk(DEBUG_DATA_INFO, "%s: next_fb.format = %d\n", __func__,
				di_para->next_fb.format);

	dprintk(DEBUG_DATA_INFO, "%s: source_regn.width = %d\n", __func__,
				di_para->source_regn.width);
	dprintk(DEBUG_DATA_INFO, "%s: source_regn.height = %d\n", __func__,
				di_para->source_regn.height);

	dprintk(DEBUG_DATA_INFO, "%s: output_fb.addr[0] = 0x%lx\n", __func__,
				(unsigned long)(di_para->output_fb.addr[0]));
	dprintk(DEBUG_DATA_INFO, "%s: output_fb.addr[1] = 0x%lx\n", __func__,
				(unsigned long)(di_para->output_fb.addr[1]));
	dprintk(DEBUG_DATA_INFO, "%s: output_fb.addr[2] = 0x%lx\n", __func__,
				(unsigned long)(di_para->output_fb.addr[2]));
	dprintk(DEBUG_DATA_INFO, "%s: output_fb.size.width = %d\n", __func__,
				di_para->output_fb.size.width);
	dprintk(DEBUG_DATA_INFO, "%s: output_fb.size.height = %d\n", __func__,
				di_para->output_fb.size.height);
	dprintk(DEBUG_DATA_INFO, "%s: output_fb.format = %d\n", __func__,
				di_para->output_fb.format);
	dprintk(DEBUG_DATA_INFO, "%s: out_regn.width = %d\n", __func__,
				di_para->out_regn.width);
	dprintk(DEBUG_DATA_INFO, "%s: out_regn.height = %d\n", __func__,
				di_para->out_regn.height);
	dprintk(DEBUG_DATA_INFO, "%s: field = %d\n", __func__, di_para->field);
	dprintk(DEBUG_DATA_INFO, "%s: top_field_first = %d\n", __func__,
				di_para->top_field_first);

	/* when di is in work, wait*/
	ret = di_complete_check_get();
	while (1 == ret) {
		msleep(1);
		ret = di_complete_check_get();
	}
	di_complete_check_set(1);
	field = di_para->top_field_first ? di_para->field : (1-di_para->field);

	dprintk(DEBUG_DATA_INFO, "%s: field = %d\n", __func__, field);
	dprintk(DEBUG_DATA_INFO, "%s: in_flag_phy = 0x%lx\n", __func__,
				(unsigned long)(di_data->in_flag_phy));
	dprintk(DEBUG_DATA_INFO, "%s: out_flag_phy = 0x%lx\n", __func__,
				(unsigned long)(di_data->out_flag_phy));

	/* modify flag buffer update in DI_V2.X */
	ret = di_set_para(di_para, di_data->in_flag_phy,
				di_data->out_flag_phy, field);

	if (ret) {
		pr_err("%s: deinterlace work failed.\n", __func__);
		return -1;
	} else {
		di_irq_enable(1);
		di_start();
		mod_timer(s_timer,
			jiffies + msecs_to_jiffies(di_data->time_value));
	}

	if (!(filp->f_flags & O_NONBLOCK)) {
		ret = wait_event_interruptible(di_data->wait,
					(di_complete_check_get() != 1));
		if (ret)
			return ret;
	}
	ret = di_complete_check_get();
	/* switch flag buffer for next time */
	if (!ret) {/* if success, swap flag buffer */
		void *tmp_ptr;
		tmp_ptr = di_data->in_flag_phy;
		di_data->in_flag_phy = di_data->out_flag_phy;
		di_data->out_flag_phy = tmp_ptr;
	}

#ifdef IOMMU_SUPPORT
	di_dma_unmap(out_item);
NEXT_DMA_UNMAP:
	di_dma_unmap(next_item);
PRE_DMA_UNMAP:
	di_dma_unmap(pre_item);
CUR_DMA_UNMAP:
	di_dma_unmap(cur_item);
FREE_OUT:
	kfree(out_item);
FREE_NEXT:
	kfree(next_item);
FREE_PRE:
	kfree(pre_item);
FREE_CUR:
	kfree(cur_item);
EXIT:
	return ret;
#else
	return ret;
#endif

}

static long sunxi_di_ioctl(struct file *filp, unsigned int cmd,
			unsigned long arg)
{
	struct __di_para_t di_paras;
	struct __di_para_t *di_para = &di_paras;
	struct __di_para_t2 di_paras2;
	struct __di_para_t2 *di_para2 = &di_paras2;
#ifdef DI_V2X_SUPPORT
	struct __di_mode_t di_mode_paras;
	struct __di_mode_t *di_mode = &di_mode_paras;
#endif
	__u32 ret = 0;

	dprintk(DEBUG_TEST, "%s: enter!!\n", __func__);

	switch (cmd) {
	case DI_IOCSTART:
	{
		if (copy_from_user((void *)di_para,
			(void __user *)arg, sizeof(struct __di_para_t))) {
			pr_warn("copy_from_user fail\n");
			return -EFAULT;
		}

		memset(di_para2, 0, sizeof(struct __di_para_t2));
		change_para(di_para, di_para2);
		ret = di_process(di_para2, filp);
	}
	break;

	case DI_IOCSTART2:
	{
		memset(di_para2, 0, sizeof(struct __di_para_t2));

		if (copy_from_user((void *)di_para2,
			(void __user *)arg, sizeof(struct __di_para_t2))) {
			pr_warn("copy_from_user fail\n");
			return -EFAULT;
		}

		ret = di_process(di_para2, filp);
	}
	break;

/* New IO for setting di_mode */
#ifdef DI_V2X_SUPPORT
	case DI_IOCSETMODE:
	{
		if (copy_from_user((void *)di_mode,
			(void __user *)arg, sizeof(struct __di_mode_t))) {
			pr_warn("copy_from_user fail\n");
			return -EFAULT;
		}
		di_set_mode(di_mode);
	}
		break;
#endif
	default:
		break;
	}

	dprintk(DEBUG_TEST, "%s: out!!\n", __func__);
	return ret;
}

#ifdef CONFIG_COMPAT
static long sunxi_di_compat_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	unsigned long translated_arg = (unsigned long)compat_ptr(arg);

	return sunxi_di_ioctl(filp, cmd, translated_arg);
}
#endif

static int sunxi_di_open(struct inode *inode, struct file *file)
{
	s32 ret = 0;

	dprintk(DEBUG_DATA_INFO, "%s: enter!!\n", __func__);

	if (di_data->users >= DI_MAX_USERS) {
		pr_err("%s: users number is out of range(%d)!\n", __func__,
		    DI_MAX_USERS);
		return -EMFILE;
	}

	atomic_set(&di_data->enable, 1);

	di_data->flag_size = (FLAG_WIDTH*FLAG_HIGH)/4;

	di_data->mem_in_params.size = di_data->flag_size;
	ret = di_mem_request(&(di_data->mem_in_params));
	if (ret < 0) {
		printk(KERN_ERR "%s: request in_flag mem failed\n", __func__);
		return -1;
	} else {
		di_data->in_flag_phy = (void *)di_data->mem_in_params.p_addr;
	}

	di_data->mem_out_params.size = di_data->flag_size;
	ret = di_mem_request(&(di_data->mem_out_params));
	if (ret < 0) {
		printk(KERN_ERR "%s: request out_flag mem failed\n", __func__);
		di_mem_release(&(di_data->mem_in_params));
		return -1;
	} else {
		di_data->out_flag_phy = (void *)di_data->mem_out_params.p_addr;
	}

	ret = di_clk_enable();
	if (ret) {
		di_mem_release(&(di_data->mem_in_params));
		di_mem_release(&(di_data->mem_out_params));
		printk(KERN_ERR "%s: enable clk failed.\n", __func__);
		return ret;
	}
	di_internal_clk_enable();
	di_set_init();

	di_data->users++;

	return 0;
}

static int sunxi_di_release(struct inode *inode, struct file *file)
{
	dprintk(DEBUG_DATA_INFO, "%s: enter!!\n", __func__);

	if (di_data->users == 0) {
		pr_err("%s:users number is already Zero, no need to release!\n",
		    __func__);
		return 0;
	}
	di_data->users--;
	atomic_set(&di_data->enable, 0);

	di_irq_enable(0);
	di_reset();
	di_internal_clk_disable();
	di_clk_disable();

	del_timer_sync(s_timer);
	if (di_data->mem_in_params.v_addr != NULL)
		di_mem_release(&(di_data->mem_in_params));
	if (di_data->mem_out_params.v_addr != NULL)
		di_mem_release(&(di_data->mem_out_params));

	return 0;
}

static const struct file_operations sunxi_di_fops = {
	.owner = THIS_MODULE,
	.llseek = noop_llseek,
	.unlocked_ioctl = sunxi_di_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= sunxi_di_compat_ioctl,
#endif
	.open = sunxi_di_open,
	.release = sunxi_di_release,
};

static u64 sunxi_di_dma_mask = DMA_BIT_MASK(32);

static int sunxi_di_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	s32 ret;

	iommu_dev = &pdev->dev;
	dprintk(DEBUG_INIT, "%s: enter!!\n", __func__);

	if (!of_device_is_available(node)) {
		printk("%s: di status disable!!\n", __func__);
		return -EPERM;
	}

	di_data = kzalloc(sizeof(*di_data), GFP_KERNEL);
	if (di_data == NULL) {
		ret = -ENOMEM;
		return ret;
	}

	atomic_set(&di_data->di_complete, 0);
	atomic_set(&di_data->enable, 0);
	di_data->mem_in_params.v_addr = NULL;
	di_data->mem_out_params.v_addr = NULL;

	init_waitqueue_head(&di_data->wait);

	s_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	if (!s_timer) {
		kfree(di_data);
		ret =  -ENOMEM;
		printk(KERN_ERR " %s FAIL TO  Request Time\n", __func__);
		return -1;
	}
	init_timer(s_timer);
	s_timer->function = &di_timer_handle;
	di_data->time_value = DI_TIMEOUT;

	di_wq = create_singlethread_workqueue("di_wq");
	if (!di_wq) {
		printk(KERN_ERR "Creat DE-Interlace workqueue failed.\n");
		ret = -ENOMEM;
		goto create_work_err;
	}

	INIT_WORK(&di_work, di_work_func);

	if (sunxi_di_major == -1) {
		sunxi_di_major = register_chrdev(0, DI_MODULE_NAME,
						&sunxi_di_fops);
		if (sunxi_di_major < 0) {
			pr_err("%s: Failed to register character device\n",
				__func__);
			ret = -1;
			goto register_chrdev_err;
		} else
			dprintk(DEBUG_INIT,
				"%s: sunxi_di_major = %d\n", __func__,
				sunxi_di_major);
	}

	di_dev_class = class_create(THIS_MODULE, DI_MODULE_NAME);
	if (IS_ERR(di_dev_class))
		return -1;
	di_device = device_create(di_dev_class, NULL,  MKDEV(sunxi_di_major, 0),
			NULL, DI_MODULE_NAME);
	di_device->dma_mask = &sunxi_di_dma_mask;
	di_device->coherent_dma_mask = DMA_BIT_MASK(32);
	ret = sunxi_di_params_init(pdev);
	if (ret) {
		pr_err("%s di init params failed!\n", __func__);
		goto init_para_err;
	}

#ifdef CONFIG_PM
	di_data->di_pm_domain.ops.suspend = sunxi_di_suspend;
	di_data->di_pm_domain.ops.resume = sunxi_di_resume;
	di_device->pm_domain = &(di_data->di_pm_domain);
#endif

	ret = sysfs_create_group(&di_device->kobj, &di_attribute_group);
	if (ret) {
		pr_err("%s di_attribute_group create failed!\n", __func__);
		return ret;
	}

	return 0;

init_para_err:
	if (sunxi_di_major > 0) {
		device_destroy(di_dev_class, MKDEV(sunxi_di_major, 0));
		class_destroy(di_dev_class);
		unregister_chrdev(sunxi_di_major, DI_MODULE_NAME);
	}
register_chrdev_err:
	cancel_work_sync(&di_work);
	if (di_wq != NULL) {
		flush_workqueue(di_wq);
		destroy_workqueue(di_wq);
		di_wq = NULL;
	}
create_work_err:
	kfree(s_timer);
	kfree(di_data);

	return ret;
}

static int sunxi_di_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&di_device->kobj, &di_attribute_group);
	sunxi_di_params_exit();
	if (sunxi_di_major > 0) {
		device_destroy(di_dev_class, MKDEV(sunxi_di_major, 0));
		class_destroy(di_dev_class);
		unregister_chrdev(sunxi_di_major, DI_MODULE_NAME);
	}
	cancel_work_sync(&di_work);
	if (di_wq != NULL) {
		flush_workqueue(di_wq);
		destroy_workqueue(di_wq);
		di_wq = NULL;
	}
	kfree(s_timer);
	kfree(di_data);

	printk(KERN_INFO "%s: module unloaded\n", __func__);

	return 0;
}

static const struct of_device_id sunxi_di_match[] = {
	 { .compatible = "allwinner,sunxi-deinterlace", },
	 {},
};
MODULE_DEVICE_TABLE(of, sunxi_di_match);


static struct platform_driver di_platform_driver = {
	.probe  = sunxi_di_probe,
	.remove = sunxi_di_remove,
	.driver = {
		.name	= DI_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_di_match,
	},
};

static int __init sunxi_di_init(void)
{
	return platform_driver_register(&di_platform_driver);
}

static void __exit sunxi_di_exit(void)
{
	platform_driver_unregister(&di_platform_driver);
}

module_init(sunxi_di_init);
module_exit(sunxi_di_exit);
module_param_named(debug_mask, debug_mask, int, 0644);
MODULE_DESCRIPTION("DE-Interlace driver");
MODULE_AUTHOR("Ming Li<liming@allwinnertech.com>");
MODULE_LICENSE("GPL");

