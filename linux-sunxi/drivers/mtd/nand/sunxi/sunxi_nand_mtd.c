/*
 * sunxi nand mtd driver
 *
 * Copyright (C) 2017 AllWinnertech Ltd.
 * Author: zhongguizhao <zhongguizhao@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "sunxi_nand_mtd.h"

spinlock_t read_spin_lock;

static int irq;

static unsigned int channel0;
static u64 nand_dma_mask = DMA_BIT_MASK(32);

static u8 *g_nand_page_rd_buf;
static u8 *g_nand_page_wr_buf;

static struct mtd_info *sunxi_nand_mtd;
static struct mtd_info sunxi_mtd[1];

static struct nand_chip sunxi_nand_chip;

static int cur_mtd_info_index;

static char ecc_mode_hw[11] = {16, 24, 28, 32, 40, 48, 56, 60, 64, 72, 80};

static struct mtd_partition sunxi_nand_partition[] = {
	{
		.name = "sys",
		.size = MTDPART_SIZ_FULL,
		.offset = 0,

	}
};

void printf_buf(char *buf, uint len)
{
	uint i;

	for (i = 0; i < len; i++) {
		pr_err("%2x ", buf[i]);
		if (!((i + 1) % 16))
			pr_err("\n");
	}
	pr_err("\n");
}

static irqreturn_t sunxi_nfc_interrupt(int irq, void *dev_id)
{
	unsigned int no;
	unsigned long iflags;

	spin_lock_irqsave(&nand_int_lock, iflags);

	pr_err("%s, %s, %d\n", __FILE__, __func__, __LINE__);
	no = *((unsigned int *)dev_id);
	pr_err("%s, %s, %d, %d\n", __FILE__, __func__, __LINE__, no);
	do_nand_interrupt(no);

	spin_unlock_irqrestore(&nand_int_lock, iflags);

	return IRQ_HANDLED;
}

static int sunxi_nfc_remove(struct platform_device *pdev)
{
	struct page *page;
	int i;

	free_irq(irq, &channel0);
	NandHwExit();
	mtd_device_unregister(sunxi_nand_mtd);

	page = virt_to_page((void *)g_nand_page_wr_buf);
	for (i = 0; i < MAX_NAND_DATA_PAGE_NUM; i++) {
		put_page(page+i);
		put_page(page+i);
	}
	free_pages((unsigned long)(g_nand_page_wr_buf),
		MAX_NAND_DATA_PAGE_SHIFT);
	g_nand_page_wr_buf = NULL;
	pr_err("free nand page write buf ok\n");

	page = virt_to_page((void *)g_nand_page_rd_buf);
	for (i = 0; i < MAX_NAND_DATA_PAGE_NUM; i++) {
		put_page(page+i);
		put_page(page+i);
	}
	free_pages((unsigned long)(g_nand_page_rd_buf),
		MAX_NAND_DATA_PAGE_SHIFT);
	g_nand_page_rd_buf = NULL;
	pr_err("free nand page read buf ok\n");

	return 0;
}

static uint get_ecc_mode(void)
{
	return g_nand_storage_info->EccMode;
}

static uint64_t get_phy_blksize(void)
{
	uint pg_cnt, sec_cnt;

	pg_cnt = g_nand_storage_info->PageCntPerPhyBlk;
	sec_cnt = g_nand_storage_info->SectorCntPerPage;

	return (pg_cnt * sec_cnt) << 9;
}
static uint get_ph_blk_cnt_per_die(void)
{
	return g_nand_storage_info->BlkCntPerDie;
}

static uint get_super_blk_cnt_per_die(void)
{
	return g_nssi->nsci->blk_cnt_per_super_chip;
}

static int check_offs_len(struct mtd_info *mtd,
					loff_t ofs, uint64_t len)
{
	struct nand_chip *chip = mtd->priv;
	int ret = 0;

	/* Start address must align on block boundary */
	if (ofs & ((1 << chip->phys_erase_shift) - 1)) {
		pr_err("%s: Unaligned address\n", __func__);
		ret = -EINVAL;
	}

	/* Length must align on block boundary */
	if (len & ((1 << chip->phys_erase_shift) - 1)) {
		pr_err("%s: Length not block aligned\n",
					__func__);
		ret = -EINVAL;
	}

	return ret;
}

/*
 *
 *return :0 -is not bank page; 1-bank page.
 */
static int sunxi_nand_chk_bank_page(uint8_t *buf, uint len)
{

	uint i;

	if ((buf == NULL) || !len)
		return 0;

	for (i = 0; i < len; i++)
		if (buf[i] != 0xff)
			return 0;

	return 1;
}

/*
 * nand_do_read_ops - [INTERN] Read data with ECC
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob ops structure
 *
 * Internal function. Called with chip held.
 */
static int nand_do_read_ops(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd->priv;
	uint page, blk_num, chipnr, page_size, pages_per_blk;
	uint col, copy_len, copy_oob_len;
	uint64_t sec_bitmap;
	uint8_t *mbuf;
	uint8_t sbuf[SUNXI_MAX_SECS_PER_PAGE * 4];
	uint8_t *oob, *buf;
	int ret = -ENOTSUPP;
	int ret_tmp;
	uint64_t addr;
	uint readlen = ops->len;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

	addr = from;
	ops->oobretlen = 0;
	ops->retlen = 0;

	if (!readlen)
		return 0;

	if (from >= mtd->size) {
		pr_err("addr(0x%llx) overflow,mtdsize: 0x%llx,line:%d,%s\n",
			from, mtd->size, __LINE__,  __func__);
		return	-EINVAL;
	}

	mbuf = g_nand_page_rd_buf;
	if (!mbuf) {
		pr_err("Err: malloc memory fail, line: %d, %s\n",
			__LINE__,  __func__);
		return -ENOMEM;
	}

	if ((1 << chip->page_shift) > SUNXI_MAX_PAGE_SIZE) {
		pr_err("pagesize > mbuf_size.pagesize=0x%0x, line:%d, %s\n",
			(1 << (uint)chip->page_shift), __LINE__, __func__);
		return -ENOMEM;
	}
	if (mtd->oobsize > sizeof(sbuf)) {
		pr_err("oobsize>sbuf_size,oob=0x%x,sbuf=0x%0x,line:%d,%s\n",
			mtd->oobsize, (uint)sizeof(sbuf), __LINE__, __func__);
		return -ENOMEM;
	}

	spin_lock(&read_spin_lock);

	buf = ops->datbuf;
	oob = ops->oobbuf;
	if (oob)
		copy_oob_len = MIN(ops->ooblen, mtd->oobsize);
	else
		copy_oob_len = mtd->oobsize;

	chipnr = (uint)(addr >> chip->chip_shift);
	blk_num = (uint)((addr & (chip->chipsize - 1))
		>> chip->phys_erase_shift);
	page_size = 1 << chip->page_shift;
	pages_per_blk = 1 << (chip->phys_erase_shift - chip->page_shift);
	page = ((int)(addr >> chip->page_shift) & chip->pagemask)
		% pages_per_blk;
	sec_bitmap = page_size >> 9;

	col = addr % page_size;

	SUNXI_DBG("addr: 0x%08x,%08x\n", (uint32)(addr >> 32),
		(uint32)addr);
	SUNXI_DBG("readlen: 0x%0x\n", readlen);
	SUNXI_DBG("chipnum: %0d, blknum: %0d, page: %0d, col: %0d\n",
		chipnr, blk_num, page, col);

	while (readlen) {
		if (chipnr >= chip->numchips) {
			pr_err("Err: chipnr overflow\n");
			pr_err("chipnr=%d, total_chips=%d, line:%d, %s\n",
				chipnr, chip->numchips, __LINE__,  __func__);
			ret = -EINVAL;
			break;
		}
		if (readlen >= (page_size - col))
			copy_len = page_size - col;
		else
			copy_len = readlen;

		ret_tmp = PHY_VirtualPageRead(chipnr, blk_num, page, sec_bitmap,
				mbuf, sbuf);
		if (ret_tmp && (ret_tmp != ECC_LIMIT)) {
			ret = -EBADMSG;

			pr_err("Err: Read data abort\n");
			pr_err("chipnr=%d, blk_num=%d, page=%d, col=%d\n",
				chipnr, blk_num, page, col);
			pr_err("Err data is(print first 16 bytes or less):\n");
			/* pr_buf(mbuf + col, MIN(copy_len, 16)); */
			break;
		}

		if (ret_tmp == ECC_LIMIT) {
			ret = mtd->bitflip_threshold;
			pr_err("Warning: Read data ECC_LIMIT.\n");
			pr_err("chipnr=%d, blk_num=%d, page=%d\n",
				chipnr, blk_num, page);
		}

		if (ret != mtd->bitflip_threshold)
			ret = ret_tmp;

		if (sunxi_nand_chk_bank_page(sbuf, copy_oob_len)) {
			memset(mbuf, 0xff, page_size);
			pr_err("Warning: Bank page\n");
			pr_err("chipnr=%d, blk_num=%d, page=%d\n",
				chipnr, blk_num, page);
		}

		if (buf) {
			memcpy(buf, mbuf + col, copy_len);
			buf += copy_len;
		}
		if (oob) {
			memcpy(oob, sbuf, copy_oob_len);
			ops->oobretlen = copy_oob_len;
		}
		readlen -= copy_len;
		col = 0;
		page++;
	    if (page >= pages_per_blk) {
			blk_num++;
			page = 0;
			if (blk_num >= (int)(chip->chipsize
					>> chip->phys_erase_shift)) {
				blk_num = 0;
				chipnr++;
			}
		}
	}
	ops->retlen = ops->len - readlen;

	SUNXI_DBG("exit: ret = 0x%0x, line: %d, %s\n",
		ret, __LINE__,  __func__);

	spin_unlock(&read_spin_lock);

	return ret;
}

/*
 * nand_do_read_oob - [INTERN] NAND read out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operations description structure
 *
 * NAND read out-of-band data from the spare area.
 */
static int nand_do_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd->priv;

	if (!ops->datbuf)
		ops->len = 1 << chip->page_shift;

	return nand_do_read_ops(mtd, from, ops);
}

/*
 * nand_read - [MTD Interface] MTD compatibility function for nand_do_read_ecc
 * @mtd: MTD device structure
 * @from: offset to read from
 * @len: number of bytes to read
 * @retlen: pointer to variable to store the number of read bytes
 * @buf: the databuffer to put data
 *
 * Get hold of the chip and call nand_do_read.
 */
static int sunxi_nand_part_read(struct mtd_info *mtd,
	loff_t from, size_t len, size_t *retlen, uint8_t *buf)
{
	struct mtd_oob_ops ops;
	int ret;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

	ops.len = len;
	ops.datbuf = buf;
	ops.oobbuf = NULL;
	ret = nand_do_read_ops(mtd, from, &ops);
	*retlen = ops.retlen;
	return ret;
}

/*
 * nand_do_write_ops - [INTERN] NAND write with ECC
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operations description structure
 *
 * NAND write with ECC.
 */
static int nand_do_write_ops(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd->priv;
	uint page, blk_num, chipnr, page_size, pages_per_blk;
	uint copy_oob_len = 0;
	uint64_t sec_bitmap;
	uint8_t *mbuf;
	uint8_t sbuf[SUNXI_MAX_SECS_PER_PAGE * 4];
	uint8_t *oob, *buf;
	int ret = -ENOTSUPP;
	uint64_t addr;
	uint writelen = ops->len;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

	addr = to;
	ops->retlen = 0;
	ops->oobretlen = 0;

	if (!writelen)
		return 0;

	if (writelen % (1 << chip->page_shift))
		return	-EINVAL;

	if (to >= mtd->size) {
		pr_err("Err: mtd addr overflow\n");
		pr_err("addr:0x%llx, mtdsize:0x%llx, line: %d, %s\n",
			to, mtd->size, __LINE__,  __func__);
		return	-EINVAL;
	}

	mbuf = g_nand_page_wr_buf;
	if (!mbuf) {
		pr_err("Err: malloc memory fail, line: %d, %s\n",
			__LINE__,  __func__);
		return -ENOMEM;
	}

	if ((1 << chip->page_shift) > SUNXI_MAX_PAGE_SIZE) {
		pr_err("Err: pagesize > mbuf_size.\n");
		pr_err("pagesize = 0x%0x, line: %d, %s\n",
			1 << chip->page_shift, __LINE__,  __func__);
		return -ENOMEM;
	}
	if (mtd->oobsize > sizeof(sbuf)) {
		pr_err("Err: oobsize > sbuf_size\n");
		pr_err("oobsize=0x%0x, sbuf_size=0x%0x, line:%d, %s\n",
			mtd->oobsize, (uint)sizeof(sbuf), __LINE__,  __func__);
		return -ENOMEM;
	}

	spin_lock(&read_spin_lock);

	memset(sbuf, 0xff, sizeof(sbuf));

	buf = ops->datbuf;
	oob = ops->oobbuf;
	if (oob) {
		copy_oob_len = ops->ooblen < sizeof(sbuf) ?
			ops->ooblen : sizeof(sbuf);
		memcpy(sbuf, oob, copy_oob_len);
	}

	chipnr = (uint)(addr >> chip->chip_shift);
	blk_num = (uint)((addr & (chip->chipsize - 1))
		>> chip->phys_erase_shift);
	page_size = 1 << chip->page_shift;
	pages_per_blk = (1 << chip->phys_erase_shift) / page_size;
	page = ((int)(addr >> chip->page_shift) & chip->pagemask)
		% pages_per_blk;
	sec_bitmap = page_size >> 9;

	memset(mbuf, 0xff, page_size);

	SUNXI_DBG("addr: 0x%08x,%08x\n",
		(uint32)((uint64_t)addr >> 32), (uint32)addr);
	SUNXI_DBG("writelen: 0x%0x\n", writelen);
	SUNXI_DBG("chipnum: %0d, blknum: %0d, page: %0d\n",
		chipnr, blk_num, page);

	while (writelen) {
		if (chipnr >= chip->numchips) {
			pr_err("Err: chipnr overflow\n");
			pr_err("chipnr=%d, total_chips=%d, line:%d, %s\n",
				chipnr, chip->numchips, __LINE__,  __func__);
			ret = -EINVAL;
			break;
		}
		if (buf) {
			memcpy(mbuf, buf, page_size);
			buf += page_size;
		}

		ret = PHY_VirtualPageWrite(chipnr, blk_num, page, sec_bitmap,
			mbuf, sbuf);
		if (ret) {
			pr_err("Err: write data abort\n");
			pr_err("chipnr=%d, blk_num=%d, page=%d\n",
				chipnr, blk_num, page);
			break;
		}
		ops->oobretlen = copy_oob_len;
		writelen -= page_size;
		page++;
	    if (page >= pages_per_blk) {
			blk_num++;
			page = 0;
			if (blk_num >= (int)(chip->chipsize
					>> chip->phys_erase_shift)) {
				blk_num = 0;
				chipnr++;
			}
		}
	}
	ops->retlen = ops->len - writelen;

	SUNXI_DBG("exit: ret = 0x%0x, line = %d, %s\n",
		ret, __LINE__,  __func__);

	spin_unlock(&read_spin_lock);

	return ret;
}

/*
 * nand_do_write_oob - [MTD Interface] NAND write out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 *
 * NAND write out-of-band.
 */
static int nand_do_write_oob(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd->priv;

	if (!ops->datbuf)
		ops->len = 1 << chip->page_shift;

	return nand_do_write_ops(mtd, to, ops);
}

static int sunxi_nand_part_write(struct mtd_info *mtd, loff_t to,
	size_t len, size_t *retlen, const u_char *buf)
{
		struct mtd_oob_ops ops;
		int ret;
		uint8_t sbuf[SUNXI_MAX_SECS_PER_PAGE * 4];

		SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

		memset(sbuf, 0xff, sizeof(sbuf));
		sbuf[1] = SUNXI_NAND_W_FLAG;

		ops.len = len;
		ops.datbuf = (uint8_t *)buf;
		ops.oobbuf = sbuf;
		ops.ooblen = mtd->oobsize;
		ret = nand_do_write_ops(mtd, to, &ops);
		*retlen = ops.retlen;
		return ret;
}

/*
 * nand_read_oob - [MTD Interface] NAND read data and/or out-of-band
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operation description structure
 *
 * NAND read data and/or out-of-band data.
 */
static int sunxi_nand_part_read_oob(struct mtd_info *mtd, loff_t from,
			 struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

	ops->retlen = 0;

	/* Do not allow reads past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->size) {
		pr_err("%s: Attempt read beyond end of device\n", __func__);
		return -EINVAL;
	}

	if (!ops->datbuf)
		ret = nand_do_read_oob(mtd, from, ops);
	else
		ret = nand_do_read_ops(mtd, from, ops);
	return ret;
}


/*
 * nand_write_oob - [MTD Interface] NAND write data and/or out-of-band
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation description structure
 */
static int sunxi_nand_part_write_oob(struct mtd_info *mtd, loff_t to,
			struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

	ops->retlen = 0;

	/* Do not allow writes past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->size) {
		pr_err("%s: Attempt write beyond end of device\n", __func__);
		return -EINVAL;
	}

	if (!ops->datbuf)
		ret = nand_do_write_oob(mtd, to, ops);
	else
		ret = nand_do_write_ops(mtd, to, ops);

	return ret;
}

static void sunxi_nand_sync(struct mtd_info *mtd)
{
	pr_err("%s: called\n", __func__);
}

/*
 * nand_suspend - [MTD Interface] Suspend the NAND flash
 * @mtd: MTD device structure
 */
static int sunxi_nand_suspend(struct mtd_info *mtd)
{
	pr_err("%s,%d\n", __func__, __LINE__);
	NandHwSuperStandby();
	return 0;
}

/*
 * nand_resume - [MTD Interface] Resume the NAND flash
 * @mtd: MTD device structure
 */
static void sunxi_nand_resume(struct mtd_info *mtd)
{
	pr_err("-----CHENEY :%s,%d\n", __func__, __LINE__);
	NandHwSuperResume();
}

/*
 * return: 0 --valid blk; -1 --reserved for boot0 or boot1
 */
static int chk_boot0_boot1_reserved_blks(struct mtd_info *mtd, int blk_num)
{
	int mul;
	int boot_num;

	boot_num = BOOT0_1_USED_PHY_NAND_BLK_CNT;
	mul = get_ph_blk_cnt_per_die() / get_super_blk_cnt_per_die();

	if (mul * blk_num < boot_num) {
		pr_err("%d is in boot0 or boot1 (0 - %d), mtd ignore it.\n",
			blk_num, boot_num - 1);
		return 1;
	}

	return 0;
}


/**
 * nand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd: MTD device structure
 * @offs: offset relative to mtd start
 */
static int sunxi_nand_part_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct nand_chip *chip = mtd->priv;
	uint chipnr, blk_num;
	uint64_t addr;
	int ret;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

	if (mtd == NULL)
		pr_err("MTD is null, line: %d, %s\n",
			__LINE__,  __func__);
	if (mtd->priv == NULL)
		pr_err("mtd->priv is null, line: %d, %s\n",
			__LINE__,  __func__);

	addr = offs;

	chipnr = (uint)(addr >> chip->chip_shift);
	blk_num = (uint)((addr & (chip->chipsize - 1))
		>> chip->phys_erase_shift);

	if (chk_boot0_boot1_reserved_blks(mtd, blk_num))
		return 1;

	spin_lock(&read_spin_lock);
	ret = PHY_VirtualBadBlockCheck(chipnr, blk_num);
	spin_unlock(&read_spin_lock);
	if (ret) {
		pr_err("Warning: find bad blk.chipnr = %d, blk_nr = %0d XXX\n",
			chipnr, blk_num);
		return 1;
	} else {
		SUNXI_DBG("exit: line: %d, %s\n\n", __LINE__,  __func__);
		return 0;
	}
}


static int _sunxi_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	uint chipnr, blk_num;
	uint64_t addr;
	int ret;

	addr = ofs;
	chipnr = (uint)(addr >> chip->chip_shift);
	blk_num = (uint)((addr & (chip->chipsize - 1))
		>> chip->phys_erase_shift);

	if (ofs >= mtd->size) {
		pr_err("warning: offs is overflow, fix bad blk.");
		pr_err("%s, offs:0x%llx, blk_num: %d\n",
			__func__, ofs, blk_num);
		return 0;
	}

	spin_lock(&read_spin_lock);
	ret = PHY_VirtualBadBlockMark(chipnr, blk_num);
	spin_unlock(&read_spin_lock);

	return ret;
}

/**
 * nand_block_markbad - [MTD Interface] Mark block at the given offset as bad
 * @mtd: MTD device structure
 * @ofs: offset relative to mtd start
 */
static int sunxi_nand_part_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);

	ret = sunxi_nand_part_block_isbad(mtd, ofs);
	if (ret) {
		/* If it was bad already, return success and do nothing */
		if (ret > 0)
			return 0;
		return ret;
	}

	return _sunxi_nand_block_markbad(mtd, ofs);
}

static int sunxi_nand_part_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int blk_num, ret, chipnr;
	loff_t len;
	uint64_t addr;
	struct nand_chip *chip = mtd->priv;

	SUNXI_DBG("line: %d, %s\n", __LINE__,  __func__);
	SUNXI_DBG("%s: start = 0x%012llx, len = %llu\n",
				__func__, (unsigned long long)instr->addr,
				(unsigned long long)instr->len);

	addr = instr->addr;

	if (check_offs_len(mtd, addr, instr->len))
		return -EINVAL;

	chipnr = (uint)(addr >> chip->chip_shift);
	blk_num = (uint)((addr & (chip->chipsize - 1))
		>> chip->phys_erase_shift);
	len = instr->len;

	SUNXI_DBG("erase chipnum: %0d, blknum: %0d , len:0x%08x,%08x\n",
			chipnr, blk_num, (uint32)(len >> 32), (uint32)len);

	while (len) {
		if (chipnr >= chip->numchips) {
			pr_err("Err: chipnr overflow.\n");
			pr_err("chipnr = %d, total_chips = %d, line: %d, %s\n",
				chipnr, chip->numchips, __LINE__,  __func__);

			ret = -EINVAL;
			break;
		}

		spin_lock(&read_spin_lock);
		ret = PHY_VirtualBlockErase(chipnr, blk_num);
		spin_unlock(&read_spin_lock);
		if (ret) {
			instr->state = MTD_ERASE_FAILED;
			pr_err("Erase blk fail, block num : 0x%x\n",
				blk_num);
			goto erase_exit;
		}

		len -= (1 << chip->phys_erase_shift);
		blk_num++;
		if (len && !((blk_num << chip->page_shift) & chip->pagemask)) {
			chipnr++;
			blk_num = 0;
		}
	}
	instr->state = MTD_ERASE_DONE;

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;

	/* Do call back function */
/*	if (!ret)
 *		mtd_erase_callback(instr);
 */

	SUNXI_DBG("exit: ret = 0x%0x, line: %d, %s\n\n",
		ret, __LINE__,  __func__);

	return ret;

}

struct mtd_info *get_next_mtd_info(void)
{
	if (cur_mtd_info_index < MAX_PARTITION - 1) {
		cur_mtd_info_index++;
		return &sunxi_mtd[cur_mtd_info_index];
	} else {
		return NULL;
	}
}

/*
 * return data is  2^n.
 */
static uint64_t align_data(uint64_t dat)
{
	int i;
	int no = 0;
	uint64_t data = dat;

	for (i = 0; i < 64; i++) {
		if (data & 0x01)
			no = i;
		data >>= 1;
	}
	return dat & ((uint64_t)1 << no);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
static struct _nand_info *sunxi_nand_hw_init(void)
{
	int ret;

	pr_err("NandHwInit\n");

	nand_cfg_setting();

	ret = nand_physic_init();
	if (ret != 0) {
		PHY_ERR("nand_physic_init error %d\n", ret);
		return NULL;
	}

	aw_nand_info.type = 0;
	aw_nand_info.SectorNumsPerPage =
		g_nssi->nsci->sector_cnt_per_super_page;
	aw_nand_info.BytesUserData =
		g_nssi->nsci->spare_bytes;
	aw_nand_info.BlkPerChip =
		g_nssi->nsci->blk_cnt_per_super_chip;
	aw_nand_info.ChipNum =
		g_nssi->super_chip_cnt;
	aw_nand_info.PageNumsPerBlk =
		g_nssi->nsci->page_cnt_per_super_blk;
	aw_nand_info.MaxBlkEraseTimes =
		g_nssi->nsci->nci_first->max_erase_times;
	aw_nand_info.EnableReadReclaim = 1;
	aw_nand_info.boot = phyinfo_buf;

	pr_err("NandHwInit end\n\n");

	return &aw_nand_info;
}

static int sunxi_nand_mtd_init(struct _nand_info *nand_info)
{
	struct nand_chip *chip;
	struct _nand_info *nand;
	struct mtd_info *mtd;

	uint ecc_mode;
	uint chipcnt, blkcnt_perchip, pgcnt_perblk;
	uint seccnt_perpg, oobsize, pgsize, blksize;
	uint64_t chipsize, align_chipsize;
	int ret;

	pr_err("%s, %d\n", __func__, __LINE__);

	nand = nand_info;

	mtd = &sunxi_mtd[0];
	chip = &sunxi_nand_chip;

	memset(mtd, 0x00, sizeof(struct mtd_info));
	memset(chip, 0x00, sizeof(struct nand_chip));

	mtd->priv = chip;
	sunxi_nand_mtd = mtd;

	chipcnt = nand->ChipNum;
	blkcnt_perchip = nand->BlkPerChip;
	pgcnt_perblk = nand->PageNumsPerBlk;
	seccnt_perpg = nand->SectorNumsPerPage;
	oobsize = nand->BytesUserData;

	pgsize = seccnt_perpg << 9;
	blksize = pgsize * pgcnt_perblk;
	chipsize = ((uint64)blksize) * blkcnt_perchip;

	chip->numchips = chipcnt;
	align_chipsize = align_data(chipsize);
	chip->chipsize = chipsize > align_chipsize ?
		align_chipsize << 1 : align_chipsize;

	chip->page_shift = ffs(pgsize) - 1;
	chip->phys_erase_shift = ffs(blksize) - 1;

	if (chip->chipsize & 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else {
		chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32));
		chip->chip_shift += 32 - 1;
	}
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;
	chip->pagebuf = -1;

	mtd->size = chipsize * chip->numchips;

	mtd->erasesize = blksize;
	mtd->writesize = pgsize;
	mtd->oobsize = oobsize;
	mtd->oobavail = mtd->oobsize;

	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->owner = THIS_MODULE;

	ecc_mode = get_ecc_mode();
	mtd->ecc_strength = ecc_mode < sizeof(ecc_mode_hw) ?
		ecc_mode_hw[ecc_mode] : ecc_mode_hw[sizeof(ecc_mode_hw)-1];
	mtd->bitflip_threshold = mtd->ecc_strength;

	mtd->writebufsize =  pgsize;
	mtd->numeraseregions = 0;
	mtd->subpage_sft = 0;

	mtd->name = "sunxi_nand_mtd";

	mtd->_erase = sunxi_nand_part_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = sunxi_nand_part_read;
	mtd->_write = sunxi_nand_part_write;
	mtd->_panic_write = sunxi_nand_part_write;
	mtd->_read_oob = sunxi_nand_part_read_oob;
	mtd->_write_oob = sunxi_nand_part_write_oob;
	mtd->_sync = sunxi_nand_sync;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_suspend = sunxi_nand_suspend;
	mtd->_resume = sunxi_nand_resume;
	mtd->_block_isbad = sunxi_nand_part_block_isbad;
	mtd->_block_markbad = sunxi_nand_part_block_markbad;

	ret = mtd_device_register(mtd, sunxi_nand_partition,
		sizeof(sunxi_nand_partition) / sizeof(sunxi_nand_partition[0]));
	if (ret)
		pr_err("sunxi mtd register err\n");

	SUNXI_DBG("pgsize: 0x%x\n", pgsize);
	SUNXI_DBG("mtd->erasesize: 0x%x\n", mtd->erasesize);
	SUNXI_DBG("mtd->oobsize: 0x%x\n", mtd->oobsize);
	SUNXI_DBG("mtd->size: 0x%llx\n", mtd->size);

	SUNXI_DBG("ecc_mode: %d, mtd->ecc_strength: %d\n",
		ecc_mode, mtd->ecc_strength);

	SUNXI_DBG("ph blk size : 0x%llx\n", get_phy_blksize());

	SUNXI_DBG("%s, %d\n\n", __func__, __LINE__);

	return 0;
}

int set_nand_dev(struct device *dev)
{
	ndfc_dev = dev;
	return 0;
}

struct device *get_nand_dev(void)
{
	return ndfc_dev;
}

int NAND_SetIOBaseAddrCH0(void *addr)
{
	SUNXI_DBG("addr=0x%llx, %s, %d\n", (u64)addr, __func__, __LINE__);

	NDFC0_BASE_ADDR = addr;

	return 0;
}

static int sunxi_nfc_probe(struct platform_device *pdev)
{
	struct _nand_info *p_nand_info;
	char *dev_name = "nand_dev";
	struct page *page;
	int i;
	int ret;

	page = alloc_pages(GFP_KERNEL, MAX_NAND_DATA_PAGE_SHIFT);
	if (!page) {
		pr_err("alloc nand page read  buf err.\n");
		ret = -ENOMEM;
		goto err;
	}

	for (i = 0; i < MAX_NAND_DATA_PAGE_NUM; i++) {
		get_page(page+i);
		get_page(page+i);
	}

	g_nand_page_rd_buf = (void *)page_address(page);
	if (!g_nand_page_rd_buf) {
		ret = -ENOMEM;
		goto free_nand_rd_buf;
	}
	pr_err("alloc nand page read buf ok, buf size(KB) = 0x%0x\n",
		i = MAX_NAND_DATA_LEN / 1024);

	page = alloc_pages(GFP_KERNEL, MAX_NAND_DATA_PAGE_SHIFT);
	if (!page) {
		pr_err("alloc nand page write buf err.\n");
		ret = -ENOMEM;
		goto free_nand_rd_buf;
	}

	for (i = 0; i < MAX_NAND_DATA_PAGE_NUM; i++) {
		get_page(page + i);
		get_page(page + i);
	}
	g_nand_page_wr_buf = (void *)page_address(page);
	if (!g_nand_page_wr_buf) {
		ret = -ENOMEM;
		goto free_nand_wr_buf;
	}

	pr_err("alloc nand page write buf ok, buf size(KB) = 0x%0x\n",
		i = MAX_NAND_DATA_LEN / 1024);

	plat_dev_nand = pdev;
	set_nand_dev(&pdev->dev);

	pdev->dev.dma_mask = &nand_dma_mask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	spin_lock_init(&nand_int_lock);
	spin_lock_init(&read_spin_lock);

	pr_err("sunxi_nfc_probe !!!\n");

	NAND_SetIOBaseAddrCH0(of_iomap(get_nand_dev()->of_node, 0));

	if ((nand_wait_rb_mode() != 0) || (nand_wait_dma_mode() != 0)) {
		pr_err("nand interrupt request\n\n");
		irq = irq_of_parse_and_map(get_nand_dev()->of_node, 0);
		if (request_irq(irq, sunxi_nfc_interrupt, IRQF_DISABLED,
			dev_name, &channel0)) {
			pr_err("nand interrupte ch0 irqno: %d register err\n",
				irq);
			return -11;
		}
	}

	p_nand_info = sunxi_nand_hw_init();

	if (!p_nand_info) {
		pr_err("p_nand_info == NULL, exit.\n\n");
		goto out_mod_clk_unprepare;
	}

	sunxi_nand_mtd_init(p_nand_info);

	pr_err("Exit %s\n", __func__);

out_mod_clk_unprepare:
	return 0;

free_nand_wr_buf:
	page = virt_to_page((void *)g_nand_page_wr_buf);
	for (i = 0; i < MAX_NAND_DATA_PAGE_NUM; i++) {
		put_page(page + i);
		put_page(page + i);
	}
	free_pages((unsigned long)(g_nand_page_wr_buf),
		MAX_NAND_DATA_PAGE_SHIFT);

free_nand_rd_buf:
	page = virt_to_page((void *)g_nand_page_rd_buf);
	for (i = 0; i < MAX_NAND_DATA_PAGE_NUM; i++) {
		put_page(page + i);
		put_page(page + i);
	}
	free_pages((unsigned long)(g_nand_page_rd_buf),
		MAX_NAND_DATA_PAGE_SHIFT);
err:
	return ret;
}

static const struct of_device_id sunxi_nfc_ids[] = {
	{ .compatible = "allwinner,sun50i-nand", },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sunxi_nfc_ids);

static struct platform_driver sunxi_nfc_driver = {
	.driver = {
		.name = "sunxi-nand",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_nfc_ids,
	},
	.probe = sunxi_nfc_probe,
	.remove = sunxi_nfc_remove,
};

module_platform_driver(sunxi_nfc_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Zhong Guizhao");
MODULE_DESCRIPTION("Allwinner NAND Flash mtd driver");
MODULE_ALIAS("platform:sunxi-nand");
