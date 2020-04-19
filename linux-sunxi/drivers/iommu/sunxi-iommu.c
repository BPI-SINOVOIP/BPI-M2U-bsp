/*******************************************************************************
 * Copyright (C) 2016-2018, Allwinner Technology CO., LTD.
 * Author: fanqinghua <fanqinghua@allwinnertech.com>
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 ******************************************************************************/
/* #define DEBUG */
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/iommu.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/memblock.h>
#include <linux/export.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_iommu.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/dma-iommu.h>
#include <linux/uaccess.h>
#include "sunxi-iommu.h"

#define _max(x, y) (((u64)(x) > (u64)(y)) ? x : y)

static struct kmem_cache *iopte_cache;
static struct sunxi_iommu *global_iommu_dev;
struct sunxi_iommu_domain *global_sunxi_domain;

#ifdef CONFIG_ARCH_SUN50IW6
static const char *master[10] = {"DE", "VE_R", "DI", "VE", "CSI", "VP9"};
#endif
#ifdef CONFIG_ARCH_SUN50IW3
static const char *master[10] = {"DE0", "DE1", "VE_R", "VE", "CSI", "VP9"};
#endif
/**
 * iopde_offset() - find out the address of page directory entry
 * @iopd:	the base address of Page Directory
 * @iova:	virtual I/O address
 **/
static inline u32 *iopde_offset(u32 *iopd, unsigned int iova)
{
	return iopd + IOPDE_INDEX(iova);
}

/**
 * iopte_offset() - find out the address of page table entry
 * @ent:	the entry of page directory
 * @iova:	virtual I/O address
 **/
static inline u32 *iopte_offset(u32 *ent, unsigned int iova)
{
	return (u32 *)__va(IOPTE_BASE(*ent)) + IOPTE_INDEX(iova);
}

/* TODO: set cache flags per prot IOMMU_CACHE */
static inline u32 sunxi_mk_pte(u32 page, int prot)
{
	u32 flags = 0;

	flags |= (prot & IOMMU_READ) ? SUNXI_PTE_PAGE_READABLE : 0;
	flags |= (prot & IOMMU_WRITE) ? SUNXI_PTE_PAGE_WRITABLE : 0;
	page &= IOMMU_PT_MASK;
	return page | flags | SUNXI_PTE_PAGE_VALID;
}

static inline u32 sunxi_iommu_read(struct sunxi_iommu *iommu, u32 offset)
{
	return readl(iommu->base + offset);
}

static inline void sunxi_iommu_write(struct sunxi_iommu *iommu,
								u32 offset, u32 value)
{
	writel(value, iommu->base + offset);
}

static int sunxi_tlb_flush(struct sunxi_iommu *iommu)
{
	int ret;

	sunxi_iommu_write(iommu, IOMMU_TLB_FLUSH_ENABLE_REG, 0x0003003f);
	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_TLB_FLUSH_ENABLE_REG)), 2);
	if (ret)
		dev_err(iommu->dev, "Enable flush all request timed out\n");
	return ret;
}

static int sunxi_tlb_invalid(dma_addr_t iova, u32 mask)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x0);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_REG, iova);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_MASK_REG, mask);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);
	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG)&0x1), 2);
	if (ret) {
		dev_err(iommu->dev, "TLB Invalid timed out\n");
		goto out;
	}

out:
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x1);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	return ret;
}

static int sunxi_ptw_cache_invalid(dma_addr_t iova)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	int ret = 0;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x0);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ADDR_REG, iova);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);
	ret = sunxi_wait_when(
		(sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG)&0x1), 2);
	if (ret) {
		dev_err(iommu->dev, "PTW cache invalid timed out\n");
		goto out;
	}

out:
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x1);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	return ret;
}

void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	unsigned long mflag;

	BUG_ON(mastor_id > 5);
	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	if (flag)
		iommu->bypass &= ~(mastor_id_bitmap[mastor_id]);
	else
		iommu->bypass |= mastor_id_bitmap[mastor_id];
	sunxi_iommu_write(iommu, IOMMU_BYPASS_REG, iommu->bypass);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
}
EXPORT_SYMBOL(sunxi_enable_device_iommu);

static int sunxi_tlb_init(struct sunxi_iommu_domain *sunxi_domain)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	phys_addr_t dte_addr;
	unsigned long mflag;
	int ret = 0;

	/* iommu init */
	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	dte_addr = __pa(sunxi_domain->pgtable);
	sunxi_iommu_write(iommu, IOMMU_TTB_REG, dte_addr);
	sunxi_iommu_write(iommu, IOMMU_TLB_PREFETCH_REG, 0x3f);
	sunxi_iommu_write(iommu, IOMMU_INT_ENABLE_REG, 0xffffffff);
	sunxi_iommu_write(iommu, IOMMU_BYPASS_REG, iommu->bypass);
	ret = sunxi_tlb_flush(iommu);
	if (ret) {
		dev_err(iommu->dev, "Enable flush all request timed out\n");
		goto out;
	}
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x1);
	sunxi_iommu_write(iommu, IOMMU_ENABLE_REG, IOMMU_ENABLE);
out:
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	return ret;

}

static int sunxi_iova_invalid_helper(unsigned long iova)
{
	struct sunxi_iommu_domain *sunxi_domain = global_sunxi_domain;
	u32 *pte_addr, *dte_addr;

	dte_addr = iopde_offset(sunxi_domain->pgtable, iova);
	if ((*dte_addr & 0x3) != 0x1) {
		pr_err("0x%lx is not mapped!\n", iova);
		return 1;
	}
	pte_addr = iopte_offset(dte_addr, iova);
	if ((*pte_addr & 0x2) == 0) {
		pr_err("0x%lx is not mapped!\n", iova);
		return 1;
	}
	pr_err("0x%lx is mapped!\n", iova);
	return 0;
}

static irqreturn_t sunxi_iommu_irq(int irq, void *dev_id)
{
	u32 inter_status_reg, addr_reg, int_masterid_bitmap,
		data_reg, l1_pgint_reg, l2_pgint_reg, master_id;
	struct sunxi_iommu *iommu = dev_id;
	unsigned long mflag;

	spin_lock_irqsave(&iommu->iommu_lock, mflag);
	inter_status_reg = sunxi_iommu_read(iommu, IOMMU_INT_STA_REG) & 0x3ffff;
	l1_pgint_reg = sunxi_iommu_read(iommu, IOMMU_L1PG_INT_REG);
	l2_pgint_reg = sunxi_iommu_read(iommu, IOMMU_L2PG_INT_REG);

	if (inter_status_reg & MICRO_TLB0_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG0);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG0);
	} else if (inter_status_reg & MICRO_TLB1_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG1);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG1);
	} else if (inter_status_reg & MICRO_TLB2_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG2);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG2);
	} else if (inter_status_reg & MICRO_TLB3_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG3);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG3);
	} else if (inter_status_reg & MICRO_TLB4_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG4);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG4);
	} else if (inter_status_reg & MICRO_TLB5_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG5);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG5);
	} else if (inter_status_reg & L1_PAGETABLE_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG6);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG6);
	} else if (inter_status_reg & L2_PAGETABLE_INVALID_INTER_MASK) {
		addr_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_ADDR_REG7);
		data_reg = sunxi_iommu_read(iommu, IOMMU_INT_ERR_DATA_REG7);
	} else {
		pr_err("sunxi iommu int error!!!\n");
	}
	if (sunxi_iova_invalid_helper(addr_reg)) {
		int_masterid_bitmap =
			inter_status_reg | l1_pgint_reg | l2_pgint_reg;
		int_masterid_bitmap &= 0xffff;
		master_id = __ffs(int_masterid_bitmap);
		BUG_ON(master_id > 5);
		pr_err("%s invalid address: 0x%x, data:0x%x, id:0x%x\n",
			master[master_id], addr_reg, data_reg,
				int_masterid_bitmap);
	}
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x0);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_REG, addr_reg);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_MASK_REG,
		(u32)IOMMU_PT_MASK);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG) & 0x1)
		;
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_REG,
		addr_reg + 0x2000);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ADDR_MASK_REG,
		(u32)IOMMU_PT_MASK);
	sunxi_iommu_write(iommu, IOMMU_TLB_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_TLB_IVLD_ENABLE_REG) & 0x1)
		;
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ADDR_REG, addr_reg);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG) & 0x1)
		;
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ADDR_REG,
		addr_reg + 0x200000);
	sunxi_iommu_write(iommu, IOMMU_PC_IVLD_ENABLE_REG, 0x1);
	while (sunxi_iommu_read(iommu, IOMMU_PC_IVLD_ENABLE_REG) & 0x1)
		;
	sunxi_iommu_write(iommu, IOMMU_AUTO_GATING_REG, 0x1);

	sunxi_iommu_write(iommu, IOMMU_INT_CLR_REG, inter_status_reg);
	inter_status_reg |= (l1_pgint_reg | l2_pgint_reg);
	inter_status_reg &= 0xffff;
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, ~inter_status_reg);
	sunxi_iommu_write(iommu, IOMMU_RESET_REG, 0xffffffff);
	spin_unlock_irqrestore(&iommu->iommu_lock, mflag);
	return IRQ_HANDLED;
}

static int sunxi_alloc_iopte(u32 *sent, int prot)
{
	u32 *pent;
	u32 flags = 0;

	flags |= (prot & IOMMU_READ) ? DENT_READABLE : 0;
	flags |= (prot & IOMMU_WRITE) ? DENT_WRITABLE : 0;

	pent = kmem_cache_zalloc(iopte_cache, GFP_ATOMIC);
	BUG_ON((unsigned long)pent & (PT_SIZE - 1));
	if (!pent) {
		pr_err("%s, %d, malloc failed!\n", __func__, __LINE__);
		return 0;
	}
	*sent = __pa(pent) | DENT_VALID;
	__dma_map_area(sent, sizeof(*sent), DMA_TO_DEVICE);

	return 1;
}

static void sunxi_free_iopte(u32 *pent)
{
	kmem_cache_free(iopte_cache, pent);
}

void sunxi_zap_tlb(unsigned long iova, size_t size)
{
	sunxi_tlb_invalid(iova, (u32)IOMMU_PT_MASK);
	sunxi_tlb_invalid(iova + SPAGE_SIZE, (u32)IOMMU_PT_MASK);
	sunxi_tlb_invalid(iova + size, (u32)IOMMU_PT_MASK);
	sunxi_tlb_invalid(iova + size + SPAGE_SIZE, (u32)IOMMU_PT_MASK);
	sunxi_ptw_cache_invalid(iova);
	sunxi_ptw_cache_invalid(iova + SPD_SIZE);
	sunxi_ptw_cache_invalid(iova + size);
	sunxi_ptw_cache_invalid(iova + size + SPD_SIZE);
}

static void sunxi_iommu_domain_destroy(struct iommu_domain *domain)
{
	struct sunxi_iommu_domain *sdm = domain->priv;
	struct sunxi_mdev *mdev, *tmp;
	u32 *dent;
	u32 *pent;
	u32 iova;
	int i;

	WARN_ON(!list_empty(&sdm->mdevs));
	spin_lock(&sdm->lock);
	list_for_each_entry_safe(mdev, tmp, &sdm->mdevs, node) {
		list_del(&mdev->node);
		kfree(mdev);
	}
	spin_unlock(&sdm->lock);

	mutex_lock(&sdm->dt_lock);
	for (i = 0; i < NUM_ENTRIES_PDE; i++) {
		dent = (sdm->pgtable + i);
		iova = i << IOMMU_PD_SHIFT;
		if (IS_VAILD(*dent)) {
			pent = iopte_offset(dent, iova);
			memset(pent, 0, PT_SIZE);
			__dma_map_area(pent, PT_SIZE, DMA_TO_DEVICE);
			sunxi_tlb_invalid(iova, (u32)IOMMU_PD_MASK);
			*dent = 0;
			__dma_map_area(dent, sizeof(*dent), DMA_TO_DEVICE);
			sunxi_ptw_cache_invalid(iova);
			sunxi_free_iopte(pent);
		}
	}
	mutex_unlock(&sdm->dt_lock);
	arm_iommu_release_mapping(sdm->mapping);
	free_pages((unsigned long)sdm->pgtable, get_order(PD_SIZE));
	free_pages((unsigned long)sdm->sg_buffer,
		get_order(MAX_SG_TABLE_SIZE));
	kfree(domain->priv);
	domain->priv = NULL;
	global_sunxi_domain = NULL;
}

static int sunxi_iommu_domain_init(struct iommu_domain *domain)
{
	struct sunxi_iommu_domain *sunxi_domain;

	sunxi_domain = kzalloc(sizeof(*sunxi_domain), GFP_KERNEL);
	if (!sunxi_domain)
		return -ENOMEM;

	/*
	 * The page table size is 16K, and must be aligned 16K.
	 */
	sunxi_domain->pgtable = (unsigned int *)__get_free_pages(
				GFP_KERNEL, get_order(PD_SIZE));
	if (!sunxi_domain->pgtable)
		goto err;

	sunxi_domain->sg_buffer = (unsigned int *)__get_free_pages(
				GFP_KERNEL, get_order(MAX_SG_TABLE_SIZE));
	if (!sunxi_domain->sg_buffer)
		goto err1;

	memset(sunxi_domain->pgtable, 0, PD_SIZE);
	mutex_init(&sunxi_domain->dt_lock);
	spin_lock_init(&sunxi_domain->lock);
	INIT_LIST_HEAD(&sunxi_domain->mdevs);
	global_sunxi_domain = sunxi_domain;
	domain->geometry.aperture_start = 0;
	domain->geometry.aperture_end   = (1ULL << 32) - 1;
	domain->geometry.force_aperture = true;
	domain->priv = sunxi_domain;
	return 0;

err1:
	free_pages((unsigned long)sunxi_domain->pgtable,
		get_order(PD_SIZE));
err:
	kfree(sunxi_domain);
	return -ENOMEM;
}

/*
* Notice: the size may bigger than SPAGE, so we must consider it.
* We should process three conditions:
* 1. start pfn and end pfn in a same PDE
* 2. start pfn in the pre PDE and end pfn in the next PDE
* 3. end pfn is bigger than start pfn plus 1
*/
static int sunxi_iommu_map(struct iommu_domain *domain, unsigned long iova,
		phys_addr_t paddr, size_t size, int prot)
{
	struct sunxi_iommu_domain *priv = domain->priv;
	u32 *dent, *pent;
	size_t iova_start, iova_end, s_iova_start, paddr_start;
	int i, j;
	int ret = 0;

	BUG_ON(priv->pgtable == NULL);
	size = SPAGE_ALIGN(size);
	iova_start = iova & IOMMU_PT_MASK;
	paddr_start = paddr & IOMMU_PT_MASK;
	iova_end = SPAGE_ALIGN(iova + size) - SPAGE_SIZE;
	mutex_lock(&priv->dt_lock);
	if (IOPDE_INDEX(iova_start) == IOPDE_INDEX(iova_end)) {
		/*map in the same pde.*/
		s_iova_start = iova_start;
		dent = iopde_offset(priv->pgtable, iova_start);
		if (!IS_VAILD(*dent))
			sunxi_alloc_iopte(dent, prot);
		for (; iova_start <= iova_end;
			iova_start += SPAGE_SIZE, paddr_start += SPAGE_SIZE) {
			pent = iopte_offset(dent, iova_start);
			WARN_ON(*pent);
			*pent = sunxi_mk_pte(paddr_start, prot);
		}
		__dma_map_area(iopte_offset(dent, s_iova_start),
			(size / SPAGE_SIZE) << 2, DMA_TO_DEVICE);
		goto out;
	} else {
		/*map to the bottom of this pde.*/
		dent = iopde_offset(priv->pgtable, iova_start);
		if (!IS_VAILD(*dent))
			sunxi_alloc_iopte(dent, prot);
		s_iova_start = iova_start;
		for (; iova_start < SPDE_ALIGN(s_iova_start+1);
			iova_start += SPAGE_SIZE, paddr_start += SPAGE_SIZE) {
			pent = iopte_offset(dent, iova_start);
			WARN_ON(*pent);
			*pent = sunxi_mk_pte(paddr_start, prot);
		}
		__dma_map_area(iopte_offset(dent, s_iova_start),
		((SPDE_ALIGN(s_iova_start+1) - s_iova_start) / SPAGE_SIZE) << 2,
			DMA_TO_DEVICE);
	}

	if (IOPDE_INDEX(iova_start) < IOPDE_INDEX(iova_end)) {
		/*map the number of pde*/
		for (i = IOPDE_INDEX(iova_start); i < IOPDE_INDEX(iova_end);
			i++, iova_start += SPD_SIZE, paddr_start += SPD_SIZE) {
			dent = iopde_offset(priv->pgtable, iova_start);
			if (!IS_VAILD(*dent))
				sunxi_alloc_iopte(dent, prot);
			pent = iopte_offset(dent, iova_start);
			for (j = 0; j < NUM_ENTRIES_PTE; j++) {
				WARN_ON(*(pent + j));
				*(pent + j) =
					sunxi_mk_pte(paddr_start + (j * SPAGE_SIZE), prot);
			}
			__dma_map_area(pent, PT_SIZE, DMA_TO_DEVICE);
		}
	}

	/*map the last pde*/
	dent = iopde_offset(priv->pgtable, iova_start);
	if (!IS_VAILD(*dent))
		sunxi_alloc_iopte(dent, prot);
	s_iova_start = iova_start;
	for (; iova_start <= iova_end;
		iova_start += SPAGE_SIZE, paddr_start += SPAGE_SIZE) {
		pent = iopte_offset(dent, iova_start);
		WARN_ON(*pent);
		*pent = sunxi_mk_pte(paddr_start, prot);
	}
	__dma_map_area(iopte_offset(dent, s_iova_start),
		((iova_end + SPAGE_SIZE - s_iova_start) / SPAGE_SIZE) << 2,
			DMA_TO_DEVICE);

out:
	sunxi_zap_tlb(iova, size);
	mutex_unlock(&priv->dt_lock);
	return ret;
}

/*
* Notice: the size may bigger than SPAGE, so we must consider it.
* We should process three conditions:
* 1. start pfn and end pfn in a same PDE
* 2. start pfn in the pre PDE and end pfn in the next PDE
* 3. end pfn is bigger than start pfn plus 1
*/
static size_t sunxi_iommu_unmap(struct iommu_domain *domain,
		unsigned long iova, size_t size)
{
	struct sunxi_iommu_domain *priv = domain->priv;
	u32 *dent;
	u32 *pent;
	int i = 0;
	size_t iova_start, iova_end, s_iova_start;

	BUG_ON(priv->pgtable == NULL);
	iova_start = iova & IOMMU_PT_MASK;
	iova_end = SPAGE_ALIGN(iova + size) - SPAGE_SIZE;
	mutex_lock(&priv->dt_lock);
	if (IOPDE_INDEX(iova_start) == IOPDE_INDEX(iova_end)) {
		/*unmap in the same pde.*/
		/*need consider recycling the pte.*/
		dent = iopde_offset(priv->pgtable, iova_start);
		if (IS_VAILD(*dent)) {
			for (; iova_start <= iova_end;
				iova_start += SPAGE_SIZE) {
				pent = iopte_offset(dent, iova_start);
				*pent = 0;
				__dma_map_area(pent, sizeof(*pent),
					DMA_TO_DEVICE);
				sunxi_tlb_invalid(iova_start, (u32)IOMMU_PT_MASK);
			}
		}
		goto out;
	} else {
		/*unmap to the bottom of this pde.*/
		dent = iopde_offset(priv->pgtable, iova_start);
		if (IS_VAILD(*dent)) {
			s_iova_start = iova_start;
			for (; iova_start < SPDE_ALIGN(s_iova_start+1);
				iova_start += SPAGE_SIZE) {
				pent = iopte_offset(dent, iova_start);
				*pent = 0;
				__dma_map_area(pent, sizeof(*pent),
					DMA_TO_DEVICE);
				sunxi_tlb_invalid(iova_start, (u32)IOMMU_PT_MASK);
			}
		}
	}

	if (IOPDE_INDEX(iova_start) < IOPDE_INDEX(iova_end)) {
		/*unmap the number of pde*/
		for (i = IOPDE_INDEX(iova_start); i < IOPDE_INDEX(iova_end);
			i++, iova_start += SPD_SIZE) {
			dent = iopde_offset(priv->pgtable, iova_start);
			if (IS_VAILD(*dent)) {
				pent = iopte_offset(dent, iova_start);
				memset(pent, 0, PT_SIZE);
				__dma_map_area(pent, PT_SIZE, DMA_TO_DEVICE);
				sunxi_tlb_invalid(iova_start, (u32)IOMMU_PD_MASK);
				*dent = 0;
				__dma_map_area(dent, sizeof(*dent),
					DMA_TO_DEVICE);
				sunxi_ptw_cache_invalid(iova_start);
				sunxi_free_iopte(pent);
			}
		}
	}

	/*unmap the last pde*/
	/*need consider recycling the pte.*/
	dent = iopde_offset(priv->pgtable, iova_start);
	if (IS_VAILD(*dent)) {
		for (; iova_start <= iova_end; iova_start += SPAGE_SIZE) {
			pent = iopte_offset(dent, iova_start);
			*pent = 0;
			__dma_map_area(pent, sizeof(*pent), DMA_TO_DEVICE);
			sunxi_tlb_invalid(iova_start, (u32)IOMMU_PT_MASK);
		}
	}

out:
	sunxi_zap_tlb(iova, size);
	mutex_unlock(&priv->dt_lock);
	return size;
}

static size_t sunxi_convert_sg_to_table(struct scatterlist *sg, size_t size)
{
	struct scatterlist *cur = sg;
	u32 *p = global_sunxi_domain->sg_buffer;
	int i;
	size_t total_length = 0;

	BUG_ON(size > MAX_SG_SIZE);
	while (total_length < size) {
		u32 phys = page_to_phys(sg_page(cur));
		unsigned int length = PAGE_ALIGN(cur->offset + cur->length);

		for (i = 0; i < length / SPAGE_SIZE; i++, p++)
			*p = phys + (i * SPAGE_SIZE);
		cur = sg_next(cur);
		total_length += length;
	}
	return total_length;
}

static size_t sunxi_iommu_map_sg(struct iommu_domain *domain,
	unsigned long iova, struct scatterlist *sg, size_t size, int prot)
{
	struct sunxi_iommu_domain *priv = domain->priv;
	u32 *dent, *pent;
	size_t iova_start, iova_end, s_iova_start;
	u32 *paddr_start;
	int i, j;
	size_t total_size = 0;

	BUG_ON(priv->pgtable == NULL);
	size = SPAGE_ALIGN(size);
	iova_start = iova & IOMMU_PT_MASK;
	paddr_start = global_sunxi_domain->sg_buffer;
	iova_end = SPAGE_ALIGN(iova + size) - SPAGE_SIZE;
	mutex_lock(&priv->dt_lock);
	total_size = sunxi_convert_sg_to_table(sg, size);
	if (IOPDE_INDEX(iova_start) == IOPDE_INDEX(iova_end)) {
		/*map in the same pde.*/
		s_iova_start = iova_start;
		dent = iopde_offset(priv->pgtable, iova_start);
		if (!IS_VAILD(*dent))
			sunxi_alloc_iopte(dent, prot);
		for (; iova_start <= iova_end;
			iova_start += SPAGE_SIZE, paddr_start++) {
			pent = iopte_offset(dent, iova_start);
			WARN_ON(*pent);
			*pent = sunxi_mk_pte(*paddr_start, prot);
		}
		__dma_map_area(iopte_offset(dent, s_iova_start),
			(size / SPAGE_SIZE) << 2, DMA_TO_DEVICE);
		goto out;
	} else {
		/*map to the bottom of this pde.*/
		dent = iopde_offset(priv->pgtable, iova_start);
		if (!IS_VAILD(*dent))
			sunxi_alloc_iopte(dent, prot);
		s_iova_start = iova_start;
		for (; iova_start < SPDE_ALIGN(s_iova_start+1);
			iova_start += SPAGE_SIZE, paddr_start++) {
			pent = iopte_offset(dent, iova_start);
			WARN_ON(*pent);
			*pent = sunxi_mk_pte(*paddr_start, prot);
		}
		__dma_map_area(iopte_offset(dent, s_iova_start),
		((SPDE_ALIGN(s_iova_start+1) - s_iova_start) / SPAGE_SIZE) << 2,
			DMA_TO_DEVICE);
	}

	if (IOPDE_INDEX(iova_start) < IOPDE_INDEX(iova_end)) {
		/*map the number of pde*/
		for (i = IOPDE_INDEX(iova_start); i < IOPDE_INDEX(iova_end);
			i++, iova_start += SPD_SIZE) {
			dent = iopde_offset(priv->pgtable, iova_start);
			if (!IS_VAILD(*dent))
				sunxi_alloc_iopte(dent, prot);
			pent = iopte_offset(dent, iova_start);
			for (j = 0; j < NUM_ENTRIES_PTE; j++) {
				WARN_ON(*(pent + j));
				*(pent + j) =
					sunxi_mk_pte(*(paddr_start++), prot);
			}
			__dma_map_area(pent, PT_SIZE, DMA_TO_DEVICE);
		}
	}

	/*map the last pde*/
	dent = iopde_offset(priv->pgtable, iova_start);
	if (!IS_VAILD(*dent))
		sunxi_alloc_iopte(dent, prot);
	s_iova_start = iova_start;
	for (; iova_start <= iova_end;
		iova_start += SPAGE_SIZE, paddr_start++) {
		pent = iopte_offset(dent, iova_start);
		WARN_ON(*pent);
		*pent = sunxi_mk_pte(*paddr_start, prot);
	}
	__dma_map_area(iopte_offset(dent, s_iova_start),
		((iova_end + SPAGE_SIZE - s_iova_start) / SPAGE_SIZE) << 2,
			DMA_TO_DEVICE);

out:
	sunxi_zap_tlb(iova, size);
	mutex_unlock(&priv->dt_lock);
	return total_size;
}

static phys_addr_t sunxi_iommu_iova_to_phys(struct iommu_domain *domain,
		dma_addr_t iova)
{
	struct sunxi_iommu_domain *priv = domain->priv;
	u32 *entry;
	phys_addr_t phys = 0;

	mutex_lock(&priv->dt_lock);
	entry = iopde_offset(priv->pgtable, iova);

	if (IS_VAILD(*entry)) {
		entry = iopte_offset(entry, iova);
		phys = IOPTE_TO_PFN(entry) + IOVA_PAGE_OFT(iova);
	}
	mutex_unlock(&priv->dt_lock);
	return phys;
}

static int sunxi_iommu_attach_device(struct iommu_domain *domain,
		struct device *dev)
{
	int ret = 0;

	pr_err("Attached to iommu domain\n");
	return ret;
}

static void sunxi_iommu_detach_device(struct iommu_domain *domain,
		struct device *dev)
{
	pr_err("Detached to iommu domain\n");
}

static int sunxi_iommu_add_device(struct device *dev)
{
	struct sunxi_mdev *mdev;
	struct of_phandle_args args;
	struct dma_iommu_mapping *mapping;
	struct sunxi_iommu_domain *sunxi_domain = global_sunxi_domain;
	struct sunxi_iommu *iommu = global_iommu_dev;
	int ret = 0;

	/* parse device tree to get master id and pdev */
	ret = of_parse_phandle_with_args(dev->of_node, "iommus",
					"#iommu-cells", 0, &args);
	if (ret < 0) {
		ret = -EINVAL;
		goto out;
	}
	mdev = kzalloc(sizeof(*mdev), GFP_KERNEL);
	if (!mdev) {
		ret = -ENOMEM;
		goto out_put_node;
	}

	mdev->dev = dev;
	mdev->tlbid = args.args[0];
	mdev->flag = args.args[1];

	if  (!sunxi_domain) {
		mapping = arm_iommu_create_mapping(&platform_bus_type, 0,
						   SZ_1G * 4UL);
		if (IS_ERR(mapping)) {
			ret = PTR_ERR(mapping);
			goto out_free_mdev;
		}
		sunxi_domain = global_sunxi_domain;
		sunxi_domain->mapping = mapping;
		sunxi_tlb_init(sunxi_domain);
	} else {
		mapping = sunxi_domain->mapping;
	}
	if (arm_iommu_attach_device(dev, mapping)) {
		pr_err("arm_iommu_attach_device failed\n");
		ret = -EINVAL;
		goto out_free_mdev;
	}
	sunxi_enable_device_iommu(mdev->tlbid, mdev->flag);
	pr_debug("mastor id:%d, bypass:0x%x\n",
			mdev->tlbid, sunxi_iommu_read(iommu, IOMMU_BYPASS_REG));
	spin_lock(&sunxi_domain->lock);
	list_add_tail(&mdev->node, &sunxi_domain->mdevs);
	spin_unlock(&sunxi_domain->lock);
	of_node_put(args.np);
	return ret;

out_free_mdev:
	kfree(mdev);
out_put_node:
	of_node_put(args.np);
out:
	return ret;
}

static void sunxi_iommu_remove_device(struct device *dev)
{
	struct sunxi_mdev *mdev, *tmp;
	struct sunxi_iommu_domain *sunxi_domain = global_sunxi_domain;

	if (!dev->of_node) {
		pr_warn("%s: Unable to get the device tree node\n", __func__);
		return;
	}

	spin_lock(&sunxi_domain->lock);
	list_for_each_entry_safe(mdev, tmp, &sunxi_domain->mdevs, node) {
		if (mdev->dev == dev) {
			arm_iommu_detach_device(dev);
			sunxi_enable_device_iommu(mdev->tlbid, false);
			list_del(&mdev->node);
			kfree(mdev);
			goto out;
		}
	}
out:
	spin_unlock(&sunxi_domain->lock);
}

static unsigned long sunxi_iommu_reg_read(struct iommu_domain *domain,
				       unsigned long offset)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	u32 retval;

	if ((offset >= SZ_1K) || (offset & 0x3)) {
		pr_err("Invalid offset: 0x%lx\n", offset);
		return 0;
	}
	spin_lock(&iommu->iommu_lock);
	retval = sunxi_iommu_read(iommu, offset);
	spin_unlock(&iommu->iommu_lock);
	return retval;
}

static void sunxi_iommu_reg_write(struct iommu_domain *domain,
			       unsigned long offset, unsigned long val)
{
	struct sunxi_iommu *iommu = global_iommu_dev;

	if ((offset >= SZ_1K) || (offset & 0x3)) {
		pr_err("Invalid offset: 0x%lx\n", offset);
		return;
	}
	spin_lock(&iommu->iommu_lock);
	sunxi_iommu_write(iommu, offset, val);
	spin_unlock(&iommu->iommu_lock);
}

int sunxi_iova_test_write(dma_addr_t iova, u32 val)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	int retval;

	spin_lock(&iommu->iommu_lock);
	sunxi_iommu_write(iommu, IOMMU_VA_REG, iova);
	sunxi_iommu_write(iommu, IOMMU_VA_DATA_REG, val);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x00000100);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x00000101);
	retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_VA_CONFIG_REG) & 0x1), 1);
	if (retval)
		dev_err(iommu->dev,
			"write VA address request timed out\n");
	spin_unlock(&iommu->iommu_lock);
	return retval;
}

unsigned long sunxi_iova_test_read(dma_addr_t iova)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	unsigned long retval;

	spin_lock(&iommu->iommu_lock);
	sunxi_iommu_write(iommu, IOMMU_VA_REG, iova);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x00000000);
	sunxi_iommu_write(iommu,
			IOMMU_VA_CONFIG_REG, 0x00000001);
	retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_VA_CONFIG_REG) & 0x1), 1);
	if (retval) {
		dev_err(iommu->dev,
			"read VA address request timed out\n");
		retval = 0;
		goto out;
	}
	retval = sunxi_iommu_read(iommu,
			IOMMU_VA_DATA_REG);
out:
	spin_unlock(&iommu->iommu_lock);
	return retval;
}

static struct iommu_ops sunxi_iommu_ops = {
	.domain_init = &sunxi_iommu_domain_init,
	.domain_destroy = &sunxi_iommu_domain_destroy,
	.attach_dev = &sunxi_iommu_attach_device,
	.detach_dev = &sunxi_iommu_detach_device,
	.map = &sunxi_iommu_map,
	.unmap = &sunxi_iommu_unmap,
	.map_sg = sunxi_iommu_map_sg,
	.iova_to_phys = &sunxi_iommu_iova_to_phys,
	.pgsize_bitmap = SZ_4K,

	.domain_has_cap = NULL,
	.add_device = sunxi_iommu_add_device,
	.remove_device = sunxi_iommu_remove_device,
	.device_group = NULL,
	.domain_get_attr = NULL,
	.domain_set_attr = NULL,
	.domain_window_enable = NULL,
	.domain_window_disable = NULL,
	.reg_read = sunxi_iommu_reg_read,
	.reg_write = sunxi_iommu_reg_write,
};

static ssize_t sunxi_iommu_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	u32 data;

	spin_lock(&iommu->iommu_lock);
	data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
	spin_unlock(&iommu->iommu_lock);
	return snprintf(buf, PAGE_SIZE,
		"enable = %d\n", data & 0x1 ? 1 : 0);
}

static ssize_t sunxi_iommu_enable_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	unsigned long val;
	u32 data;
	int retval;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val) {
		spin_lock(&iommu->iommu_lock);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_ENABLE_REG, data | 0x1);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_PMU_CLR_REG) & 0x1), 1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		spin_unlock(&iommu->iommu_lock);
	} else {
		spin_lock(&iommu->iommu_lock);
		data = sunxi_iommu_read(iommu, IOMMU_PMU_CLR_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_CLR_REG, data | 0x1);
		retval = sunxi_wait_when((sunxi_iommu_read(iommu,
				IOMMU_PMU_CLR_REG) & 0x1), 1);
		if (retval)
			dev_err(iommu->dev, "Clear PMU Count timed out\n");
		data = sunxi_iommu_read(iommu, IOMMU_PMU_ENABLE_REG);
		sunxi_iommu_write(iommu, IOMMU_PMU_ENABLE_REG, data & ~0x1);
		spin_unlock(&iommu->iommu_lock);
	}

	return count;
}

static ssize_t sunxi_iommu_profilling_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_iommu *iommu = global_iommu_dev;
	u64 micro_tlb0_access_count;
	u64 micro_tlb0_hit_count;
	u16 micro_tlb0_hitrate;
	u16 macro_tlb0_hitrate;
	u64 micro_tlb1_access_count;
	u64 micro_tlb1_hit_count;
	u16 micro_tlb1_hitrate;
	u16 macro_tlb1_hitrate;
	u64 micro_tlb2_access_count;
	u64 micro_tlb2_hit_count;
	u16 micro_tlb2_hitrate;
	u16 macro_tlb2_hitrate;
	u64 micro_tlb3_access_count;
	u64 micro_tlb3_hit_count;
	u16 micro_tlb3_hitrate;
	u16 macro_tlb3_hitrate;
	u64 micro_tlb4_access_count;
	u64 micro_tlb4_hit_count;
	u16 micro_tlb4_hitrate;
	u16 macro_tlb4_hitrate;
	u64 micro_tlb5_access_count;
	u64 micro_tlb5_hit_count;
	u16 micro_tlb5_hitrate;
	u16 macro_tlb5_hitrate;
	u64 macrotlb_access_count;
	u64 macrotlb_hit_count;
	u16 macrotlb_hitrate;
	u64 ptwcache_access_count;
	u64 ptwcache_hit_count;
	u16 ptwcache_hitrate;
	u64 micro_tlb0_latency;
	u64 micro_tlb1_latency;
	u64 micro_tlb2_latency;
	u64 micro_tlb3_latency;
	u64 micro_tlb4_latency;
	u64 micro_tlb5_latency;

	spin_lock(&iommu->iommu_lock);
	macrotlb_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG6) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG6);
	macrotlb_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG6) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG6);
	macrotlb_hitrate =
		(macrotlb_hit_count * 1000) /
		_max(1, macrotlb_access_count);

	micro_tlb0_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG0) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG0);
	micro_tlb0_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG0) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG0);
	micro_tlb0_hitrate =
		(micro_tlb0_hit_count * 1000) /
		_max(1, micro_tlb0_access_count);
	macro_tlb0_hitrate = micro_tlb0_access_count == 0 ? 0 :
		((1000 - micro_tlb0_hitrate) * macrotlb_hitrate) / 1000;

	micro_tlb1_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG1) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG1);
	micro_tlb1_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG1) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG1);
	micro_tlb1_hitrate =
		(micro_tlb1_hit_count * 1000) /
		_max(1, micro_tlb1_access_count);
	macro_tlb1_hitrate = micro_tlb1_access_count == 0 ? 0 :
		((1000 - micro_tlb1_hitrate) * macrotlb_hitrate) / 1000;

	micro_tlb2_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG2) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG2);
	micro_tlb2_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG2) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG2);
	micro_tlb2_hitrate =
		(micro_tlb2_hit_count * 1000) /
		_max(1, micro_tlb2_access_count);
	macro_tlb2_hitrate = micro_tlb2_access_count == 0 ? 0 :
		((1000 - micro_tlb2_hitrate) * macrotlb_hitrate) / 1000;

	micro_tlb3_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG3) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG3);
	micro_tlb3_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG3) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG3);
	micro_tlb3_hitrate =
		(micro_tlb3_hit_count * 1000) /
		_max(1, micro_tlb3_access_count);
	macro_tlb3_hitrate = micro_tlb3_access_count == 0 ? 0 :
		((1000 - micro_tlb3_hitrate) * macrotlb_hitrate) / 1000;

	micro_tlb4_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG4) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG4);
	micro_tlb4_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG4) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG4);
	micro_tlb4_hitrate =
		(micro_tlb4_hit_count * 1000) /
		_max(1, micro_tlb4_access_count);
	macro_tlb4_hitrate = micro_tlb4_access_count == 0 ? 0 :
		((1000 - micro_tlb4_hitrate) * macrotlb_hitrate) / 1000;

	micro_tlb5_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG5) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG5);
	micro_tlb5_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG5) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG5);
	micro_tlb5_hitrate =
		(micro_tlb5_hit_count * 1000) /
		_max(1, micro_tlb5_access_count);
	macro_tlb5_hitrate = micro_tlb5_access_count == 0 ? 0 :
		((1000 - micro_tlb5_hitrate) * macrotlb_hitrate) / 1000;

	ptwcache_access_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_HIGH_REG7) &&
		0x7ff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_ACCESS_LOW_REG7);
	ptwcache_hit_count =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_HIT_HIGH_REG7) &&
		0x7ff) << 32) | sunxi_iommu_read(iommu, IOMMU_PMU_HIT_LOW_REG7);
	ptwcache_hitrate =
		(ptwcache_hit_count * 1000) /
		_max(1, ptwcache_access_count);

	micro_tlb0_latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG0) &&
		0x3ffff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG0);
	micro_tlb1_latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG1) &&
		0x3ffff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG1);
	micro_tlb2_latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG2) &&
		0x3ffff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG2);
	micro_tlb3_latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG3) &&
		0x3ffff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG3);
	micro_tlb4_latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG4) &&
		0x3ffff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG4);
	micro_tlb5_latency =
		((u64)(sunxi_iommu_read(iommu, IOMMU_PMU_TL_HIGH_REG5) &&
		0x3ffff) << 32) |
		sunxi_iommu_read(iommu, IOMMU_PMU_TL_LOW_REG5);
	spin_unlock(&iommu->iommu_lock);

	return snprintf(buf, PAGE_SIZE,
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_hitrate = %d\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_hitrate = %d\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_hitrate = %d\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_hitrate = %d\n"
		"%s_access_count = 0x%llu\n"
		"%s_hit_count = 0x%llu\n"
		"%s_hitrate = %d\n"
		"%s_access_count = 0x%llx\n"
		"%s_hit_count = 0x%llx\n"
		"%s_hitrate = %d\n"
		"macrotlb_access_count = 0x%llx\n"
		"macrotlb_hit_count = 0x%llx\n"
		"macrotlb_hitrate = %d\n"
		"ptwcache_access_count = 0x%llx\n"
		"ptwcache_hit_count = 0x%llx\n"
		"ptwcache_hitrate = %d\n"
		"%s_latency = 0x%llx\n"
		"%s_latency = 0x%llx\n"
		"%s_latency = 0x%llx\n"
		"%s_latency = 0x%llx\n"
		"%s_latency = 0x%llx\n"
		"%s_latency = 0x%llx\n",
		master[0], micro_tlb0_access_count,
		master[0], micro_tlb0_hit_count,
		master[0], micro_tlb0_hitrate + macro_tlb0_hitrate,
		master[1], micro_tlb1_access_count,
		master[1], micro_tlb1_hit_count,
		master[1], micro_tlb1_hitrate + macro_tlb1_hitrate,
		master[2], micro_tlb2_access_count,
		master[2], micro_tlb2_hit_count,
		master[2], micro_tlb2_hitrate + macro_tlb2_hitrate,
		master[3], micro_tlb3_access_count,
		master[3], micro_tlb3_hit_count,
		master[3], micro_tlb3_hitrate + macro_tlb3_hitrate,
		master[4], micro_tlb4_access_count,
		master[4], micro_tlb4_hit_count,
		master[4], micro_tlb4_hitrate + macro_tlb4_hitrate,
		master[5], micro_tlb5_access_count,
		master[5], micro_tlb5_hit_count,
		master[5], micro_tlb5_hitrate + macro_tlb5_hitrate,
		macrotlb_access_count,
		macrotlb_hit_count,
		macrotlb_hitrate,
		ptwcache_access_count,
		ptwcache_hit_count,
		ptwcache_hitrate,
		master[0], micro_tlb0_latency,
		master[1], micro_tlb1_latency,
		master[2], micro_tlb2_latency,
		master[3], micro_tlb3_latency,
		master[4], micro_tlb4_latency,
		master[5], micro_tlb5_latency);
}

static struct device_attribute sunxi_iommu_enable_attr =
	__ATTR(enable, S_IWUSR | S_IRUGO, sunxi_iommu_enable_show,
	sunxi_iommu_enable_store);
static struct device_attribute sunxi_iommu_profilling_attr =
	__ATTR(profilling, S_IRUGO, sunxi_iommu_profilling_show, NULL);

static void sunxi_iommu_sysfs_create(struct platform_device *_pdev)
{
	device_create_file(&_pdev->dev, &sunxi_iommu_enable_attr);
	device_create_file(&_pdev->dev, &sunxi_iommu_profilling_attr);
}

static void sunxi_iommu_sysfs_remove(struct platform_device *_pdev)
{
	device_remove_file(&_pdev->dev, &sunxi_iommu_enable_attr);
	device_remove_file(&_pdev->dev, &sunxi_iommu_profilling_attr);
}

static int sunxi_iommu_probe(struct platform_device *pdev)
{
	int ret, irq;
	struct device *dev = &pdev->dev;
	struct sunxi_iommu *iommu_dev;
	struct resource *res;

	iommu_dev = kzalloc(sizeof(*iommu_dev), GFP_KERNEL);
	if (!iommu_dev) {
		dev_dbg(dev, "Not enough memory\n");
		return  -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_dbg(dev, "Unable to find resource region\n");
		ret = -ENOENT;
		goto err_res;
	}

	iommu_dev->base = devm_ioremap_resource(&pdev->dev, res);
	if (!iommu_dev->base) {
		dev_dbg(dev, "Unable to map IOMEM @ PA:%#x\n",
				(unsigned int)res->start);
		ret = -ENOENT;
		goto err_res;
	}

	iommu_dev->bypass = 0x3f;
	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_dbg(dev, "Unable to find IRQ resource\n");
		goto err_irq;
	}
	pr_info(" irq = %d\n", irq);

	ret = devm_request_irq(dev, irq, sunxi_iommu_irq, IRQF_DISABLED,
			dev_name(dev), (void *)iommu_dev);
	if (ret < 0) {
		dev_dbg(dev, "Unabled to register interrupt handler\n");
		goto err_irq;
	}
	iommu_dev->irq = irq;
	iommu_dev->clk = of_clk_get_by_name(dev->of_node, "iommu");
	if (IS_ERR(iommu_dev->clk)) {
		iommu_dev->clk = NULL;
		dev_dbg(dev, "Unable to find clock\n");
		goto err_clk;
	}
	clk_prepare_enable(iommu_dev->clk);
	/*
	 * TODO: initialize hardware configuration, such as clock, bypass bit.
	 */

	platform_set_drvdata(pdev, iommu_dev);
	iommu_dev->dev = dev;
	spin_lock_init(&iommu_dev->iommu_lock);
	global_iommu_dev = iommu_dev;
	if (dev->parent)
		pm_runtime_enable(dev);
	bus_set_iommu(&platform_bus_type, &sunxi_iommu_ops);
	sunxi_iommu_sysfs_create(pdev);
	return 0;

err_clk:
	free_irq(irq, iommu_dev);
err_irq:
	devm_iounmap(dev, iommu_dev->base);
err_res:
	kfree(iommu_dev);
	dev_err(dev, "Failed to initialize\n");

	return ret;
}

static int sunxi_iommu_remove(struct platform_device *pdev)
{
	struct sunxi_iommu *mmu_dev = platform_get_drvdata(pdev);

	sunxi_iommu_sysfs_remove(pdev);
	free_irq(mmu_dev->irq, mmu_dev);
	devm_iounmap(mmu_dev->dev, mmu_dev->base);
	kfree(mmu_dev);
	global_iommu_dev = NULL;

	return 0;
}

static int sunxi_iommu_suspend(struct device *dev)
{
	return 0;
}

static int sunxi_iommu_resume(struct device *dev)
{
	struct sunxi_iommu_domain *sunxi_domain = global_sunxi_domain;
	int err;

	err = sunxi_tlb_init(sunxi_domain);
	return err;
}

const struct dev_pm_ops sunxi_iommu_pm_ops = {
	.suspend	= sunxi_iommu_suspend,
	.resume	= sunxi_iommu_resume,
};

static const struct of_device_id sunxi_iommu_dt[] = {
	{ .compatible = "allwinner,sunxi-iommu", },
};
MODULE_DEVICE_TABLE(of, sunxi_iommu_dt);

static struct platform_driver sunxi_iommu_driver = {
	.probe		= sunxi_iommu_probe,
	.remove		= sunxi_iommu_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "sunxi-iommu",
		.pm	= &sunxi_iommu_pm_ops,
		.of_match_table = sunxi_iommu_dt,
	}
};

static int __init sunxi_iommu_init(void)
{
	struct device_node *np;
	int ret;

	np = of_find_matching_node(NULL, sunxi_iommu_dt);
	if (!np)
		return 0;

	of_node_put(np);

	iopte_cache = kmem_cache_create("sunxi-iopte-cache", PT_SIZE,
				PT_SIZE, SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);
	if (!iopte_cache)
		return -ENOMEM;

	ret = platform_driver_register(&sunxi_iommu_driver);
	if (ret)
		kmem_cache_destroy(iopte_cache);

	return ret;
}

static void __exit sunxi_iommu_exit(void)
{
	kmem_cache_destroy(iopte_cache);
	platform_driver_unregister(&sunxi_iommu_driver);
}

subsys_initcall(sunxi_iommu_init);
module_exit(sunxi_iommu_exit);

MODULE_DESCRIPTION("IOMMU Driver for Allwinner");
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_ALIAS("platform:allwinner");
MODULE_LICENSE("GPL v2");
