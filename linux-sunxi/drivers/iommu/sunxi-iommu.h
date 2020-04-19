/*
 * sunxi iommu: main structures
 *
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Written by Hiroshi DOYU <Hiroshi.DOYU@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Register of IOMMU deivce
 */
#define IOMMU_VERSION_REG				0x0000
#define IOMMU_RESET_REG					0x0010
#define IOMMU_ENABLE_REG				0x0020
#define IOMMU_BYPASS_REG				0x0030
#define IOMMU_AUTO_GATING_REG			0x0040
#define IOMMU_WBUF_CTRL_REG				0x0044
#define IOMMU_OOO_CTRL_REG				0x0048
#define IOMMU_4KB_BDY_PRT_CTRL_REG		0x004C
#define IOMMU_TTB_REG					0x0050
#define IOMMU_TLB_ENABLE_REG			0x0060
#define IOMMU_TLB_PREFETCH_REG			0x0070
#define IOMMU_TLB_FLUSH_ENABLE_REG		0x0080
#define IOMMU_TLB_IVLD_ADDR_REG			0x0090
#define IOMMU_TLB_IVLD_ADDR_MASK_REG	0x0094
#define IOMMU_TLB_IVLD_ENABLE_REG		0x0098
#define IOMMU_PC_IVLD_ADDR_REG			0x00A0
#define IOMMU_PC_IVLD_ENABLE_REG		0x00A8
#define IOMMU_DM_AUT_CTRL_REG0			0x00B0
#define IOMMU_DM_AUT_CTRL_REG1			0x00B4
#define IOMMU_DM_AUT_CTRL_REG2			0x00B8
#define IOMMU_DM_AUT_CTRL_REG3			0x00BC
#define IOMMU_DM_AUT_CTRL_REG4			0x00C0
#define IOMMU_DM_AUT_CTRL_REG5			0x00C4
#define IOMMU_DM_AUT_CTRL_REG6			0x00C8
#define IOMMU_DM_AUT_CTRL_REG7			0x00CC
#define IOMMU_DM_AUT_OVWT_REG			0x00D0
#define IOMMU_INT_ENABLE_REG			0x0100
#define IOMMU_INT_CLR_REG				0x0104
#define IOMMU_INT_STA_REG				0x0108
#define IOMMU_INT_ERR_ADDR_REG0			0x0110
#define IOMMU_INT_ERR_ADDR_REG1			0x0114
#define IOMMU_INT_ERR_ADDR_REG2			0x0118
#define IOMMU_INT_ERR_ADDR_REG3			0x011C
#define IOMMU_INT_ERR_ADDR_REG4			0x0120
#define IOMMU_INT_ERR_ADDR_REG5			0x0124
#define IOMMU_INT_ERR_ADDR_REG6			0x0130
#define IOMMU_INT_ERR_ADDR_REG7			0x0134
#define IOMMU_INT_ERR_DATA_REG0			0x0150
#define IOMMU_INT_ERR_DATA_REG1			0x0154
#define IOMMU_INT_ERR_DATA_REG2			0x0158
#define IOMMU_INT_ERR_DATA_REG3			0x015C
#define IOMMU_INT_ERR_DATA_REG4			0x0160
#define IOMMU_INT_ERR_DATA_REG5			0x0164
#define IOMMU_INT_ERR_DATA_REG6			0x0170
#define IOMMU_INT_ERR_DATA_REG7			0x0174
#define IOMMU_L1PG_INT_REG				0x0180
#define IOMMU_L2PG_INT_REG				0x0184
#define IOMMU_VA_REG					0x0190
#define IOMMU_VA_DATA_REG				0x0194
#define IOMMU_VA_CONFIG_REG				0x0198
#define IOMMU_PMU_ENABLE_REG			0x0200
#define IOMMU_PMU_CLR_REG				0x0210
#define IOMMU_PMU_ACCESS_LOW_REG0		0x0230
#define IOMMU_PMU_ACCESS_HIGH_REG0		0x0234
#define IOMMU_PMU_HIT_LOW_REG0			0x0238
#define IOMMU_PMU_HIT_HIGH_REG0			0x023C
#define IOMMU_PMU_ACCESS_LOW_REG1		0x0240
#define IOMMU_PMU_ACCESS_HIGH_REG1		0x0244
#define IOMMU_PMU_HIT_LOW_REG1			0x0248
#define IOMMU_PMU_HIT_HIGH_REG1			0x024C
#define IOMMU_PMU_ACCESS_LOW_REG2		0x0250
#define IOMMU_PMU_ACCESS_HIGH_REG2		0x0254
#define IOMMU_PMU_HIT_LOW_REG2			0x0258
#define IOMMU_PMU_HIT_HIGH_REG2			0x025C
#define IOMMU_PMU_ACCESS_LOW_REG3		0x0260
#define IOMMU_PMU_ACCESS_HIGH_REG3		0x0264
#define IOMMU_PMU_HIT_LOW_REG3			0x0268
#define IOMMU_PMU_HIT_HIGH_REG3			0x026C
#define IOMMU_PMU_ACCESS_LOW_REG4		0x0270
#define IOMMU_PMU_ACCESS_HIGH_REG4		0x0274
#define IOMMU_PMU_HIT_LOW_REG4			0x0278
#define IOMMU_PMU_HIT_HIGH_REG4			0x027C
#define IOMMU_PMU_ACCESS_LOW_REG5		0x0280
#define IOMMU_PMU_ACCESS_HIGH_REG5		0x0284
#define IOMMU_PMU_HIT_LOW_REG5			0x0288
#define IOMMU_PMU_HIT_HIGH_REG5			0x028C
#define IOMMU_PMU_ACCESS_LOW_REG6		0x02D0
#define IOMMU_PMU_ACCESS_HIGH_REG6		0x02D4
#define IOMMU_PMU_HIT_LOW_REG6			0x02D8
#define IOMMU_PMU_HIT_HIGH_REG6			0x02DC
#define IOMMU_PMU_ACCESS_LOW_REG7		0x02E0
#define IOMMU_PMU_ACCESS_HIGH_REG7		0x02E4
#define IOMMU_PMU_HIT_LOW_REG7			0x02E8
#define IOMMU_PMU_HIT_HIGH_REG7			0x02EC
#define IOMMU_PMU_TL_LOW_REG0			0x0300
#define IOMMU_PMU_TL_HIGH_REG0			0x0304
#define IOMMU_PMU_TL_LOW_REG1			0x0310
#define IOMMU_PMU_TL_HIGH_REG1			0x0314
#define IOMMU_PMU_TL_LOW_REG2			0x0320
#define IOMMU_PMU_TL_HIGH_REG2			0x0324
#define IOMMU_PMU_TL_LOW_REG3			0x0330
#define IOMMU_PMU_TL_HIGH_REG3			0x0334
#define IOMMU_PMU_TL_LOW_REG4			0x0340
#define IOMMU_PMU_TL_HIGH_REG4			0x0344
#define IOMMU_PMU_TL_LOW_REG5			0x0350
#define IOMMU_PMU_TL_HIGH_REG5			0x0354

#define IOMMU_RESET_SHIFT   31
#define IOMMU_RESET_MASK (1 << IOMMU_RESET_SHIFT)
#define IOMMU_RESET_SET (0 << 31)
#define IOMMU_RESET_RELEASE (1 << 31)

/*
 * IOMMU enable register field
 */
#define IOMMU_ENABLE	0x1

/*
 * IOMMU interrupt id mask
 */
#define MICRO_TLB0_INVALID_INTER_MASK   0x1
#define MICRO_TLB1_INVALID_INTER_MASK   0x2
#define MICRO_TLB2_INVALID_INTER_MASK   0x4
#define MICRO_TLB3_INVALID_INTER_MASK   0x8
#define MICRO_TLB4_INVALID_INTER_MASK   0x10
#define MICRO_TLB5_INVALID_INTER_MASK   0x20
#define L1_PAGETABLE_INVALID_INTER_MASK   0x10000
#define L2_PAGETABLE_INVALID_INTER_MASK   0x20000

/*
 * This version Hardware just only support 4KB page. It have
 * a two level page table structure, where the first level has
 * 4096 entries, and the second level has 256 entries. And, the
 * first level is "Page Directory(PG)", every entry include a
 * Page Table base address and a few of control bits. Second
 * level is "Page Table(PT)", every entry include a physical
 * page address and a few of control bits. Each entry is one
 * 32-bit word. Most of the bits in the second level entry are
 * used by hardware.
 *
 * Virtual Address Format:
 *     31              20|19        12|11     0
 *     +-----------------+------------+--------+
 *     |    PDE Index    |  PTE Index | offset |
 *     +-----------------+------------+--------+
 *
 * Table Layout:
 *
 *      First Level         Second Level
 *   (Page Directory)       (Page Table)
 *   ----+---------+0
 *    ∧  |  PDE   |   ---> -+--------+----
 *    |  ----------+1       |  PTE   |  ∧
 *    |  |        |         +--------+  |
 *       ----------+2       |        |  1K
 *   16K |        |         +--------+  |
 *       ----------+3       |        |  ∨
 *    |  |        |         +--------+----
 *    |  ----------
 *    |  |        |
 *    ∨  |        |
 *   ----+--------+
 *
 * IOPDE:
 * 31                     10|9       0
 * +------------------------+--------+
 * |   PTE Base Address     |CTRL BIT|
 * +------------------------+--------+
 *
 * IOPTE:
 * 31                  12|11         0
 * +---------------------+-----------+
 * |  Phy Page Address   |  CTRL BIT |
 * +---------------------+-----------+
 *
 */

#define NUM_ENTRIES_PDE 4096
#define NUM_ENTRIES_PTE 256
#define PD_SIZE (NUM_ENTRIES_PDE * sizeof(u32))
#define PT_SIZE	(NUM_ENTRIES_PTE * sizeof(u32))

#define IOMMU_PD_SHIFT 20
#define IOMMU_PD_MASK  (~((1UL << IOMMU_PD_SHIFT) - 1))

#define IOMMU_PT_SHIFT 12
#define IOMMU_PT_MASK  (~((1UL << IOMMU_PT_SHIFT) - 1))

#define PAGE_OFFSET_MASK  ((1UL << IOMMU_PT_SHIFT) - 1)
#define IOPTE_BASE_MASK	 (~(PT_SIZE - 1))

/*
 * Page Directory Entry Control Bits
 */
#define DENT_VALID	0x01
#define DENT_PTE_SHFIT	10
#define DENT_WRITABLE	  BIT(3)
#define DENT_READABLE	  BIT(2)

/*
 * Page Table Entry Control Bits
 */
#define SUNXI_PTE_PAGE_WRITABLE	  BIT(3)
#define SUNXI_PTE_PAGE_READABLE	  BIT(2)
#define SUNXI_PTE_PAGE_VALID		  BIT(1)

#define IS_VAILD(x)	(((x) & 0x03) == DENT_VALID)

#define IOPDE_INDEX(va)	(((va) >> IOMMU_PD_SHIFT) & (NUM_ENTRIES_PDE - 1))
#define IOPTE_INDEX(va)	(((va) >> IOMMU_PT_SHIFT) & (NUM_ENTRIES_PTE - 1))

#define IOPTE_BASE(ent) ((ent) & IOPTE_BASE_MASK)

#define IOPTE_TO_PFN(ent) ((*ent) & IOMMU_PT_MASK)
#define IOVA_PAGE_OFT(va) ((va) & PAGE_OFFSET_MASK)

#define SPAGE_SIZE (1 << IOMMU_PT_SHIFT)
#define SPD_SIZE (1 << IOMMU_PD_SHIFT)
#define SPAGE_ALIGN(addr) ALIGN(addr, SPAGE_SIZE)
#define SPDE_ALIGN(addr) ALIGN(addr, SPD_SIZE)
#define MAX_SG_SIZE  (128 << 20)
#define MAX_SG_TABLE_SIZE ((MAX_SG_SIZE / SPAGE_SIZE) * sizeof(u32))

#ifdef CONFIG_ARCH_SUN50IW3
static const u32 mastor_id_bitmap[] = {0x3, 0x0, 0x0, 0xc, 0x10, 0x20};
#endif
#ifdef CONFIG_ARCH_SUN50IW6
static const u32 mastor_id_bitmap[] = {0x1, 0x0, 0x4, 0xa, 0x10, 0x20};
#endif

#define sunxi_wait_when(COND, MS) ({ \
	unsigned long timeout__ = jiffies + msecs_to_jiffies(MS) + 1;	\
	int ret__ = 0;							\
	while ((COND)) {						\
		if (time_after(jiffies, timeout__)) {			\
			ret__ = (!COND) ? 0 : -ETIMEDOUT;		\
			break;						\
		}							\
		udelay(1);					\
	}								\
	ret__;								\
})

/*
 * The format of device tree, and client device how to use it.
 *
 * /{
 *	....
 *	smmu: iommu@xxxxx {
 *		compatible = "allwinner,iommu";
 *		reg = <xxx xxx xxx xxx>;
 *		interrupts = <GIC_SPI xxx IRQ_TYPE_LEVEL_HIGH>;
 *		interrupt-names = "iommu-irq";
 *		clocks = <&iommu_clk>;
 *		clock-name = "iommu-clk";
 *		#iommu-cells = <1>;
 *		status = "enabled";
 *	};
 *
 *	de@xxxxx {
 *		.....
 *		iommus = <&smmu ID>;
 *	};
 *
 * }
 *
 * Here, ID number is 0 ~ 5, every client device have a unique id.
 * Every id represent a micro TLB, also represent a master device.
 *
 */
struct sunxi_iommu {
	struct device *dev;
	void __iomem *base;
	struct clk *clk;
	int irq;
	u32 bypass;
	spinlock_t iommu_lock;
};

struct sunxi_iommu_domain {
	unsigned int *pgtable;		/* first page directory, size is 16KB */
	u32 *sg_buffer;
	struct mutex  dt_lock;	/* lock for modifying page table @ pgtable */
	struct dma_iommu_mapping *mapping;
	/* list of master device, it represent a micro TLB */
	struct list_head mdevs;
	spinlock_t lock;
};

/*
 * sunxi master device which use iommu.
 */
struct sunxi_mdev {
	struct list_head node;	/* for sunxi_iommu mdevs list */
	struct device *dev;	/* the master device */
	unsigned int tlbid;	/* micro TLB id, distinguish device by it */
	bool flag;
};

extern unsigned long sunxi_iova_test_read(dma_addr_t iova);
extern int sunxi_iova_test_write(dma_addr_t iova, u32 val);
extern void sunxi_zap_tlb(unsigned long iova, size_t size);

