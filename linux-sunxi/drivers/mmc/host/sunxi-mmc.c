/*
 * Driver for sunxi SD/MMC host controllers
 * (C) Copyright 2007-2011 Reuuimlla Technology Co., Ltd.
 * (C) Copyright 2007-2011 Aaron Maoye <leafy.myeh@reuuimllatech.com>
 * (C) Copyright 2013-2014 O2S GmbH <www.o2s.ch>
 * (C) Copyright 2013-2014 David Lanzend�rfer <david.lanzendoerfer@o2s.ch>
 * (C) Copyright 2013-2014 Hans de Goede <hdegoede@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <linux/clk.h>
#include <linux/clk/sunxi.h>

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>

#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>

#include <linux/sys_config.h>

#include "sunxi-mmc.h"
#include "sunxi-mmc-sun50iw1p1-2.h"
#include "sunxi-mmc-sun50iw1p1-0.h"
#include "sunxi-mmc-sun50iw1p1-1.h"
#include "sunxi-mmc-v4p1x.h"
#include "sunxi-mmc-v4p5x.h"

#include "sunxi-mmc-debug.h"
#include "sunxi-mmc-export.h"

/**default retry times ****/
#define SUNXI_DEF_RETRY_TIMES		6


static void sunxi_mmc_regs_save(struct sunxi_mmc_host *host);
static void sunxi_mmc_regs_restore(struct sunxi_mmc_host *host);
static int sunxi_mmc_bus_clk_en(struct sunxi_mmc_host *host , int enable);
static void sunxi_mmc_parse_cmd(struct mmc_host *mmc, struct mmc_command *cmd , u32 *cval , u32 *im , bool *wdma);
static void sunxi_mmc_set_dat(struct sunxi_mmc_host *host, struct mmc_host *mmc , struct mmc_data *data);
static void sunxi_mmc_exe_cmd(struct sunxi_mmc_host *host, struct mmc_command *cmd , u32 cmd_val , u32 imask);

static int sunxi_mmc_reset_host(struct sunxi_mmc_host *host)
{
	unsigned long expire = jiffies + msecs_to_jiffies(250);
	u32 rval;

	mmc_writel(host, REG_GCTRL, SDXC_HARDWARE_RESET);
	do {
		rval = mmc_readl(host, REG_GCTRL);
	} while (time_before(jiffies, expire) && (rval & SDXC_HARDWARE_RESET));

	if (rval & SDXC_HARDWARE_RESET) {
		dev_err(mmc_dev(host->mmc), "fatal err reset timeout\n");
		return -EIO;
	}

	return 0;
}

static int sunxi_mmc_reset_dmaif(struct sunxi_mmc_host *host)
{
	unsigned long expire = jiffies + msecs_to_jiffies(250);
	u32 rval;

	rval = mmc_readl(host, REG_GCTRL);
	mmc_writel(host, REG_GCTRL, rval | SDXC_DMA_RESET);
	do {
		rval = mmc_readl(host, REG_GCTRL);
	} while (time_before(jiffies, expire) && (rval & SDXC_DMA_RESET));

	if (rval & SDXC_DMA_RESET) {
		dev_err(mmc_dev(host->mmc),
			"fatal err reset dma interface timeout\n");
		return -EIO;
	}

	return 0;
}

static int sunxi_mmc_reset_fifo(struct sunxi_mmc_host *host)
{
	unsigned long expire = jiffies + msecs_to_jiffies(250);
	u32 rval;

	rval = mmc_readl(host, REG_GCTRL);
	mmc_writel(host, REG_GCTRL, rval | SDXC_FIFO_RESET);
	do {
		rval = mmc_readl(host, REG_GCTRL);
	} while (time_before(jiffies, expire) && (rval & SDXC_FIFO_RESET));

	if (rval & SDXC_FIFO_RESET) {
		dev_err(mmc_dev(host->mmc), "fatal err reset fifo timeout\n");
		return -EIO;
	}

	return 0;
}

static int sunxi_mmc_reset_dmactl(struct sunxi_mmc_host *host)
{
	unsigned long expire = jiffies + msecs_to_jiffies(250);
	u32 rval;

	rval = mmc_readl(host, REG_DMAC);
	mmc_writel(host, REG_DMAC, rval | SDXC_IDMAC_SOFT_RESET);
	do {
		rval = mmc_readl(host, REG_DMAC);
	} while (time_before(jiffies, expire)
		 && (rval & SDXC_IDMAC_SOFT_RESET));

	if (rval & SDXC_IDMAC_SOFT_RESET) {
		dev_err(mmc_dev(host->mmc),
			"fatal err reset dma contol timeout\n");
		return -EIO;
	}

	return 0;
}

void sunxi_mmc_set_a12a(struct sunxi_mmc_host *host)
{
	mmc_writel(host, REG_A12A, 0);
}

static int sunxi_mmc_init_host(struct mmc_host *mmc)
{
	u32 rval;
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (sunxi_mmc_reset_host(host))
		return -EIO;

	if (sunxi_mmc_reset_dmactl(host)) {
		return -EIO;
	}

	mmc_writel(host, REG_FTRGL, host->dma_tl ? host->dma_tl : 0x20070008);
	dev_dbg(mmc_dev(host->mmc), "REG_FTRGL %x\n",
		mmc_readl(host, REG_FTRGL));
	mmc_writel(host, REG_TMOUT, 0xffffffff);
	mmc_writel(host, REG_IMASK, host->sdio_imask | host->dat3_imask);
	mmc_writel(host, REG_RINTR, 0xffffffff);
	mmc_writel(host, REG_DBGC, 0xdeb);
	//mmc_writel(host, REG_FUNS, SDXC_CEATA_ON);
	mmc_writel(host, REG_DLBA, host->sg_dma);

	rval = mmc_readl(host, REG_GCTRL);
	rval |= SDXC_INTERRUPT_ENABLE_BIT;
	rval &= ~SDXC_ACCESS_DONE_DIRECT;
	if (host->dat3_imask) {
		rval |= SDXC_DEBOUNCE_ENABLE_BIT;
	}
	mmc_writel(host, REG_GCTRL, rval);

	if (host->sunxi_mmc_set_acmda) {
		host->sunxi_mmc_set_acmda(host);
	}
	return 0;
}

static void sunxi_mmc_init_idma_des(struct sunxi_mmc_host *host,
				    struct mmc_data *data)
{
	struct sunxi_idma_des *pdes = (struct sunxi_idma_des *)host->sg_cpu;
	struct sunxi_idma_des *pdes_pa = (struct sunxi_idma_des *)host->sg_dma;
	int i = 0;

	for (i = 0; i < data->sg_len; i++) {
		pdes[i].config = SDXC_IDMAC_DES0_CH | SDXC_IDMAC_DES0_OWN |
		    SDXC_IDMAC_DES0_DIC;
		pdes[i].buf_size = data->sg[i].length;
		pdes[i].buf_addr_ptr1 = sg_dma_address(&data->sg[i]);
		/*We use size_t only to avoid compile waring*/
		pdes[i].buf_addr_ptr2 = (u32) (size_t)&pdes_pa[i + 1];
	}

	pdes[0].config |= SDXC_IDMAC_DES0_FD;
	pdes[i - 1].config |= SDXC_IDMAC_DES0_LD;
	pdes[i - 1].config &= ~SDXC_IDMAC_DES0_DIC;

	/*
     * **Avoid the io-store starting the idmac hitting io-mem before the
	 * descriptors hit the main-mem.
	 */
	wmb();
}

static enum dma_data_direction sunxi_mmc_get_dma_dir(struct mmc_data *data)
{
	if (data->flags & MMC_DATA_WRITE)
		return DMA_TO_DEVICE;
	else
		return DMA_FROM_DEVICE;
}

static int sunxi_mmc_map_dma(struct sunxi_mmc_host *host, struct mmc_data *data)
{
	u32 i, dma_len;
	struct scatterlist *sg;
	int max_len = (1 << host->idma_des_size_bits);

	dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
			     sunxi_mmc_get_dma_dir(data));
	if (dma_len == 0) {
		dev_err(mmc_dev(host->mmc), "dma_map_sg failed\n");
		return -ENOMEM;
	}

	/*
	*Acorrding DMA-API.txt,dma_len should not be always equal to data->sg_len.
	*Because The dma_map_sg implementation is free to merge several consecutive sglist entries into one
	*But according to dma_map_sg implement in fact,dma_len is always equal to data->sg_len.
	*So we don't change the code,only add a warning on it only for safe
	*/
	BUG_ON(dma_len != data->sg_len);
	BUG_ON(dma_len > host->mmc->max_segs);

	for_each_sg(data->sg, sg, data->sg_len, i) {
		if (sg->offset & 3 || sg->length & 3) {
			dev_err(mmc_dev(host->mmc),
				"unaligned scatterlist: os %x length %d\n",
				sg->offset, sg->length);
			return -EINVAL;
		}
		if (data->sg_len > max_len) {
			dev_err(mmc_dev(host->mmc),
				"sg len is over one dma des transfer len\n");
			BUG();
		}
	}

	return 0;
}

static void sunxi_mmc_start_dma(struct sunxi_mmc_host *host,
				struct mmc_data *data)
{
	u32 rval;

	sunxi_mmc_init_idma_des(host, data);

	sunxi_mmc_reset_fifo(host);
	sunxi_mmc_reset_dmaif(host);
	sunxi_mmc_reset_dmactl(host);

	rval = mmc_readl(host, REG_GCTRL);
	rval |= SDXC_DMA_ENABLE_BIT;
	mmc_writel(host, REG_GCTRL, rval);

	if (!(data->flags & MMC_DATA_WRITE))
		mmc_writel(host, REG_IDIE, SDXC_IDMAC_RECEIVE_INTERRUPT);

	mmc_writel(host, REG_DMAC, SDXC_IDMAC_FIX_BURST | SDXC_IDMAC_IDMA_ON);
}

static void sunxi_mmc_send_manual_stop(struct sunxi_mmc_host *host,
				       struct mmc_request *req)
{
	u32 arg, cmd_val, ri;
	unsigned long expire = jiffies + msecs_to_jiffies(1000);

	cmd_val = SDXC_START | SDXC_RESP_EXPECT |
	    SDXC_STOP_ABORT_CMD | SDXC_CHECK_RESPONSE_CRC;

	if (req->cmd->opcode == SD_IO_RW_EXTENDED) {
		cmd_val |= SD_IO_RW_DIRECT;
		arg = (1 << 31) | (0 << 28) | (SDIO_CCCR_ABORT << 9) |
		    ((req->cmd->arg >> 28) & 0x7);
	} else {
		cmd_val |= MMC_STOP_TRANSMISSION;
		arg = 0;
	}

	mmc_writel(host, REG_CARG, arg);
	mmc_writel(host, REG_CMDR, cmd_val);

	do {
		ri = mmc_readl(host, REG_RINTR);
	} while (!(ri & (SDXC_COMMAND_DONE | SDXC_INTERRUPT_ERROR_BIT)) &&
		 time_before(jiffies, expire));

	if (!(ri & SDXC_COMMAND_DONE) || (ri & SDXC_INTERRUPT_ERROR_BIT)) {
		dev_err(mmc_dev(host->mmc), "send  manual stop command failed\n");
		if (req->stop)
			req->stop->resp[0] = -ETIMEDOUT;
	} else {
		if (req->stop)
			req->stop->resp[0] = mmc_readl(host, REG_RESP0);
		dev_dbg(mmc_dev(host->mmc) , "send manual stop command ok\n");
	}

	mmc_writel(host, REG_RINTR, 0xffff);
}

static void sunxi_mmc_dump_errinfo(struct sunxi_mmc_host *host)
{
	struct mmc_command *cmd = host->mrq->cmd;
	struct mmc_data *data = host->mrq->data;

	/* For some cmds timeout is normal with sd/mmc cards */
   /*
      if ((host->int_sum & SDXC_INTERRUPT_ERROR_BIT) == SDXC_RESP_TIMEOUT && (cmd->opcode == SD_IO_SEND_OP_COND || cmd->opcode == SD_IO_RW_DIRECT))
      return;
    */

	dev_err(mmc_dev(host->mmc),
		"smc %d p%d err, cmd %d,%s%s%s%s%s%s%s%s%s%s !!\n",
		host->mmc->index, host->phy_index, cmd->opcode,
		data ? (data->flags & MMC_DATA_WRITE ? " WR" : " RD") : "",
		host->int_sum & SDXC_RESP_ERROR ? " RE" : "",
		host->int_sum & SDXC_RESP_CRC_ERROR ? " RCE" : "",
		host->int_sum & SDXC_DATA_CRC_ERROR ? " DCE" : "",
		host->int_sum & SDXC_RESP_TIMEOUT ? " RTO" : "",
		host->int_sum & SDXC_DATA_TIMEOUT ? " DTO" : "",
		host->int_sum & SDXC_FIFO_RUN_ERROR ? " FE" : "",
		host->int_sum & SDXC_HARD_WARE_LOCKED ? " HL" : "",
		host->int_sum & SDXC_START_BIT_ERROR ? " SBE" : "",
		host->int_sum & SDXC_END_BIT_ERROR ? " EBE" : "");
	/*sunxi_mmc_dumphex32(host,"sunxi mmc",host->reg_base,0x180);*/
	/*sunxi_mmc_dump_des(host,host->sg_cpu,PAGE_SIZE);*/
}

/* Called in interrupt context! */
static irqreturn_t sunxi_mmc_finalize_request(struct sunxi_mmc_host *host)
{
	struct mmc_request *mrq = host->mrq;
	struct mmc_data *data = mrq->data;
	u32 rval;

	mmc_writel(host, REG_IMASK, host->sdio_imask | host->dat3_imask);
	mmc_writel(host, REG_IDIE, 0);

	if (host->int_sum & SDXC_INTERRUPT_ERROR_BIT) {
		sunxi_mmc_dump_errinfo(host);
		if ((host->ctl_spec_cap & SUNXI_SC_EN_RETRY) &&  data) {
				host->mrq_retry = mrq;
				host->errno_retry = host->int_sum & SDXC_INTERRUPT_ERROR_BIT;
		} else{
			mrq->cmd->error = -ETIMEDOUT;

			if (data) {
				data->error = -ETIMEDOUT;
				host->manual_stop_mrq = mrq;
			}

			if (mrq->stop)
				mrq->stop->error = -ETIMEDOUT;
		}
	} else {
		if (mrq->cmd->flags & MMC_RSP_136) {
			mrq->cmd->resp[0] = mmc_readl(host, REG_RESP3);
			mrq->cmd->resp[1] = mmc_readl(host, REG_RESP2);
			mrq->cmd->resp[2] = mmc_readl(host, REG_RESP1);
			mrq->cmd->resp[3] = mmc_readl(host, REG_RESP0);
		} else {
			mrq->cmd->resp[0] = mmc_readl(host, REG_RESP0);
		}

		if (data)
			data->bytes_xfered = data->blocks * data->blksz;

		//To avoid that "wait busy" and "maual stop" occur at the same time,
		//We wait busy only on not error occur.
		if ((mrq->cmd->flags & MMC_RSP_BUSY)
		    || ((mrq->cmd->data)
			&& (mrq->cmd->data->flags & MMC_DATA_WRITE)
			&& !(host->
			     ctl_spec_cap & NO_MANUAL_WAIT_BUSY_WRITE_END))) {
			if (mmc_readl(host, REG_STAS) & SDXC_CARD_DATA_BUSY)
				host->mrq_busy = host->mrq;
		}
		/**clear retry count if retry ok*/
		host->retry_cnt = 0;
	}

	if (data) {
		mmc_writel(host, REG_IDST, 0x337);
		mmc_writel(host, REG_DMAC, 0);
		rval = mmc_readl(host, REG_GCTRL);
		rval |= SDXC_DMA_RESET;
		mmc_writel(host, REG_GCTRL, rval);
		rval &= ~SDXC_DMA_ENABLE_BIT;
		mmc_writel(host, REG_GCTRL, rval);
		rval |= SDXC_FIFO_RESET;
		mmc_writel(host, REG_GCTRL, rval);
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
			     sunxi_mmc_get_dma_dir(data));
	}

	mmc_writel(host, REG_RINTR, 0xffff);

	if (host->dat3_imask) {
		rval = mmc_readl(host, REG_GCTRL);
		mmc_writel(host, REG_GCTRL, rval | SDXC_DEBOUNCE_ENABLE_BIT);
	}

	host->mrq = NULL;
	host->int_sum = 0;
	host->wait_dma = false;

	return (host->manual_stop_mrq
			|| host->mrq_busy
			|| host->mrq_retry) ? IRQ_WAKE_THREAD : IRQ_HANDLED;
}

static irqreturn_t sunxi_mmc_irq(int irq, void *dev_id)
{
	struct sunxi_mmc_host *host = dev_id;
	struct mmc_request *mrq;
	u32 msk_int, idma_int;
	bool finalize = false;
	bool sdio_int = false;
	irqreturn_t ret = IRQ_HANDLED;

	spin_lock(&host->lock);

	idma_int = mmc_readl(host, REG_IDST);
	msk_int = mmc_readl(host, REG_MISTA);

	dev_dbg(mmc_dev(host->mmc), "irq: rq %p mi %08x idi %08x\n",
		host->mrq, msk_int, idma_int);

	if (host->dat3_imask) {
		if (msk_int & SDXC_CARD_INSERT) {
			mmc_writel(host, REG_RINTR, SDXC_CARD_INSERT);
			mmc_detect_change(host->mmc, msecs_to_jiffies(500));
			goto out;
		}
		if (msk_int & SDXC_CARD_REMOVE) {
			mmc_writel(host, REG_RINTR, SDXC_CARD_REMOVE);
			mmc_detect_change(host->mmc, msecs_to_jiffies(50));
			goto out;
		}
	}

	mrq = host->mrq;
	if (mrq) {
		if (idma_int & SDXC_IDMAC_RECEIVE_INTERRUPT)
			host->wait_dma = false;

		host->int_sum |= msk_int;

		/* Wait for COMMAND_DONE on RESPONSE_TIMEOUT before finalize */
		if ((host->int_sum & SDXC_RESP_TIMEOUT) &&
		    !(host->int_sum & SDXC_COMMAND_DONE))
			mmc_writel(host, REG_IMASK,
				   host->sdio_imask | host->
				   dat3_imask | SDXC_COMMAND_DONE);
		/* Don't wait for dma on error */
		else if (host->int_sum & SDXC_INTERRUPT_ERROR_BIT)
			finalize = true;
		else if ((host->int_sum & SDXC_INTERRUPT_DONE_BIT) &&
			 !host->wait_dma)
			finalize = true;
	}

	if (msk_int & SDXC_SDIO_INTERRUPT)
		sdio_int = true;

	mmc_writel(host, REG_RINTR, msk_int);
	mmc_writel(host, REG_IDST, idma_int);

	if (finalize)
		ret = sunxi_mmc_finalize_request(host);
      out:
	spin_unlock(&host->lock);

	if (finalize && ret == IRQ_HANDLED)
		mmc_request_done(host->mmc, mrq);

	if (sdio_int)
		mmc_signal_sdio_irq(host->mmc);

	return ret;
}

int sunxi_check_r1_ready(struct sunxi_mmc_host *smc_host, unsigned ms)
{
	//struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	unsigned long expire = jiffies + msecs_to_jiffies(ms);
	dev_info(mmc_dev(smc_host->mmc), "wrd\n");
	do {
		if (!(mmc_readl(smc_host, REG_STAS) & SDXC_CARD_DATA_BUSY))
			break;
	} while (time_before(jiffies, expire));

	if ((mmc_readl(smc_host, REG_STAS) & SDXC_CARD_DATA_BUSY)) {
		dev_err(mmc_dev(smc_host->mmc), "wait r1 rdy %d ms timeout\n",
			ms);
		return -1;
	} else {
		return 0;
	}
}

static int sunxi_check_r1_ready_may_sleep(struct sunxi_mmc_host *smc_host)
{
	unsigned int cnt = 0;
	const unsigned int delay_max_cnt[] = { 1000, 0x7fffffff };
	int i = 0;
	unsigned long expire = jiffies + msecs_to_jiffies(10);

	/*****dead wait******/
	do {
		if (!(mmc_readl(smc_host, REG_STAS) & SDXC_CARD_DATA_BUSY))
			break;
	} while (time_before(jiffies, expire));

	if (!(mmc_readl(smc_host, REG_STAS) & SDXC_CARD_DATA_BUSY)) {
		dev_dbg(mmc_dev(smc_host->mmc), "dead Wait r1 rdy ok\n");
		return 0;
	}

	/*****no dead wait*****/
	for (i = 0; i < 2; i++, cnt = 0) {
		do {
			if (!
			    (mmc_readl(smc_host, REG_STAS) &
			     SDXC_CARD_DATA_BUSY)) {
				dev_dbg(mmc_dev(smc_host->mmc),
					"Wait r1 rdy ok c%d i%d \n", cnt, i);
				return 0;
			}
			if (i ? cnt / 5000 : cnt / 500000) {
				//print to tell that we are waiting busy
				dev_info(mmc_dev(smc_host->mmc),
					 "Has wait r1 rdy c%d i%d\n", cnt, i);
			}
			i ? usleep_range(1000, 1200) : usleep_range(10, 20);
		} while ((cnt++) < delay_max_cnt[i]);
	}

	dev_err(mmc_dev(smc_host->mmc), "Wait r1 rdy timeout\n");
	return -1;
}

static irqreturn_t sunxi_mmc_handle_bottom_half(int irq, void *dev_id)
{
	struct sunxi_mmc_host *host = dev_id;
	struct mmc_request *mrq_stop;
	struct mmc_request *mrq_busy = NULL;
	struct mmc_request *mrq_retry = NULL;
	struct mmc_host	*mmc = host->mmc;
	int rval = 0;
	unsigned long iflags;

	spin_lock_irqsave(&host->lock, iflags);
	mrq_stop = host->manual_stop_mrq;
	mrq_busy = host->mrq_busy;
	mrq_retry = host->mrq_retry;
	spin_unlock_irqrestore(&host->lock, iflags);

	if (mrq_busy) {
		/*
		*Here,we don't use the timeout value in mrq_busy->busy_timeout
		*Because this value may not right for example when useing TRIM
		*So we use max wait time and print time value every 1 second
		*sunxi_check_r1_ready_may_sleep(host,0x7ffffff);
		*/
		sunxi_check_r1_ready_may_sleep(host);
		spin_lock_irqsave(&host->lock, iflags);
		host->mrq_busy = NULL;
		spin_unlock_irqrestore(&host->lock, iflags);
		mmc_request_done(mmc, mrq_busy);
		return IRQ_HANDLED;
	}
	dev_dbg(mmc_dev(mmc), "no request for busy\n");


	if (mrq_stop) {
		dev_err(mmc_dev(mmc), "data error, sending stop command\n");

		/*
		 * We will never have more than one outstanding request,
		 * and we do not complete the request until after
		* we've cleared host->manual_stop_mrq so we do not need to
		* spin lock this function.
		* Additionally we have wait states within this function
		* so having it in a lock is a very bad idea.
		*/
		sunxi_mmc_send_manual_stop(host, mrq_stop);
		if (gpio_is_valid(host->card_pwr_gpio))
			gpio_set_value(host->card_pwr_gpio,
					(host->
						ctl_spec_cap & CARD_PWR_GPIO_HIGH_ACTIVE) ? 0 :
						1);

		/***reset host***/
		sunxi_mmc_regs_save(host);
		sunxi_mmc_bus_clk_en(host , 0);
		sunxi_mmc_bus_clk_en(host , 1);
		sunxi_mmc_regs_restore(host);
		dev_info(mmc_dev(host->mmc) , "reset:host reset and recover finish\n");
		/***update clk***/
		rval = host->sunxi_mmc_oclk_en(host , 1);
		if (rval) {
			dev_err(mmc_dev(mmc) , "reset:update clk failed %s %d\n",
							__FUNCTION__ , __LINE__);
		}

		spin_lock_irqsave(&host->lock, iflags);
		host->manual_stop_mrq = NULL;
		spin_unlock_irqrestore(&host->lock, iflags);

		mmc_request_done(mmc, mrq_stop);
		return IRQ_HANDLED;
	}
	dev_dbg(mmc_dev(mmc), "no request for manual stop\n");


	if (mrq_retry) {
		bool wait_dma = false;
		u32 imask = 0;
		u32 cmd_val = 0;
		struct mmc_command *cmd = mrq_retry->cmd;
		struct mmc_data *data = mrq_retry->data;

		dev_info(mmc_dev(host->mmc) , "*****retry:start*****\n");

		/***Recover device state and stop host state machine****/
		sunxi_mmc_send_manual_stop(host , mrq_retry);

		/*****If device not exit,no need to retry*****/
		/**to do:how to deal with data3 detect better here**/
		if (!mmc_gpio_get_cd(mmc)) {
			dev_err(mmc_dev(mmc) , "retry:no device\n");
			goto retry_giveup;
		}

		/***wait device busy over***/
		rval = sunxi_mmc_check_r1_ready(mmc , 1000);
		if (rval) {
			dev_err(mmc_dev(host->mmc) , "retry:busy timeout\n");
			goto retry_giveup;
		}

		/***reset host***/
		spin_lock_irqsave(&host->lock , iflags);
		sunxi_mmc_regs_save(host);
		spin_unlock_irqrestore(&host->lock , iflags);
		/**if gating/reset protect itself,so no lock use host->lock**/
		sunxi_mmc_bus_clk_en(host , 0);
		sunxi_mmc_bus_clk_en(host , 1);
		spin_lock_irqsave(&host->lock , iflags);
		sunxi_mmc_regs_restore(host);
		spin_unlock_irqrestore(&host->lock , iflags);
		dev_dbg(mmc_dev(host->mmc) , "retry:host reset and reg recover ok\n");

		/***set phase/delay not lock***/
		if (host->sunxi_mmc_judge_retry) {
			rval = host->sunxi_mmc_judge_retry(host , NULL , host->retry_cnt , host->errno_retry , NULL);
			if (rval) {
				dev_err(mmc_dev(mmc) , "retry:set phase failed or over retry times\n");
				goto reupdate_clk;
			}
		} else if (host->retry_cnt > SUNXI_DEF_RETRY_TIMES) {
				dev_err(mmc_dev(mmc) , "retry:over default retry times\n");
				goto reupdate_clk;
		}

		/***use sunxi_mmc_oclk_en  to update clk***/
		rval = host->sunxi_mmc_oclk_en(host , 1);
		if (rval) {
			dev_err(mmc_dev(mmc) , "retry:update clk failed %s %d\n",
							__FUNCTION__ , __LINE__);
			goto retry_giveup;
		}

		rval = sunxi_mmc_map_dma(host , data);
		if (rval < 0) {
			dev_err(mmc_dev(mmc) , "map DMA failed\n");
			goto retry_giveup;
		}

		sunxi_mmc_parse_cmd(mmc , cmd , &cmd_val , &imask , &wait_dma);
		sunxi_mmc_set_dat(host , mmc , data);
		spin_lock_irqsave(&host->lock, iflags);
		host->mrq = mrq_retry;
		host->mrq_retry = NULL;
		host->wait_dma = wait_dma;
		host->retry_cnt++;
		sunxi_mmc_exe_cmd(host, cmd , cmd_val , imask);
		dev_info(mmc_dev(host->mmc) , "*****retry:re-send cmd*****\n");
		spin_unlock_irqrestore(&host->lock, iflags);
		return IRQ_HANDLED;
reupdate_clk:
		/**update clk for other cmd from upper layer to be sent****/
		rval = host->sunxi_mmc_oclk_en(host , 1);
		if (rval)
			dev_err(mmc_dev(mmc) , "retry:update clk failed %s %d\n",
						__FUNCTION__ , __LINE__);
retry_giveup:
		dev_err(mmc_dev(host->mmc) , "retry:give up\n");
		spin_lock_irqsave(&host->lock , iflags);
		host->mrq_retry = NULL;
		cmd->error = -ETIMEDOUT;
		data->error = -ETIMEDOUT;
		if (mrq_retry->stop)
			mrq_retry->stop->error = -ETIMEDOUT;
		spin_unlock_irqrestore(&host->lock , iflags);
		mmc_request_done(host->mmc , mrq_retry);
		return IRQ_HANDLED;

	}
	dev_dbg(mmc_dev(host->mmc) , "no request for data retry\n");

	dev_err(mmc_dev(host->mmc) , "no request in bottom halfhalf\n");

	return IRQ_HANDLED;
}



s32 sunxi_mmc_update_clk(struct sunxi_mmc_host *host)
{
	u32 rval;
	unsigned long expire = jiffies + msecs_to_jiffies(1000);	//1000ms timeout
	s32 ret = 0;

	rval = SDXC_START | SDXC_UPCLK_ONLY | SDXC_WAIT_PRE_OVER;
	/*
    if (smc_host->voltage_switching)
	      rval |= SDXC_VolSwitch;
    */
    mmc_writel(host, REG_CMDR, rval);

	do {
		rval = mmc_readl(host, REG_CMDR);
	} while (time_before(jiffies, expire) && (rval & SDXC_START));

	if (rval & SDXC_START) {
		dev_err(mmc_dev(host->mmc),
			"update clock timeout, fatal error!!!\n");
		ret = -EIO;
	}

	return ret;
}

static int sunxi_mmc_bus_clk_en(struct sunxi_mmc_host *host , int enable)
{
	int rval = 0;
	struct mmc_host	*mmc = host->mmc;
	if (enable) {
		if (!IS_ERR(host->clk_rst)) {
			rval = clk_prepare_enable(host->clk_rst);
			if (rval) {
				dev_err(mmc_dev(mmc), "reset err %d\n", rval);
				return -1;
			}
		}

		rval = clk_prepare_enable(host->clk_ahb);
		if (rval) {
			dev_err(mmc_dev(mmc), "Enable ahb clk err %d\n", rval);
			return -1;
		}
		rval = clk_prepare_enable(host->clk_mmc);
		if (rval) {
			dev_err(mmc_dev(mmc), "Enable mmc clk err %d\n", rval);
			return -1;
		}
	} else{
		clk_disable_unprepare(host->clk_mmc);
		clk_disable_unprepare(host->clk_ahb);
		if (!IS_ERR(host->clk_rst))
			clk_disable_unprepare(host->clk_rst);
	}
	return 0;
}



static void sunxi_mmc_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	u32 rval;
	char *bus_mode[] = { "", "OD", "PP" };
	char *pwr_mode[] = { "OFF", "UP", "ON" };
    char *timing[] = {
				"LEGACY(SDR12)", "MMC-HS(SDR20)", "SD-HS(SDR25)", "UHS-SDR12",\
				"UHS-SDR25",\
				"UHS-SDR50", "UHS-SDR104", "UHS-DDR50", "MMC-HS200", "MMC-HS400"\
    };
    char *drv_type[] = { "B", "A", "C", "D" };

	BUG_ON(ios->bus_mode >= sizeof(bus_mode) / sizeof(bus_mode[0]));
	BUG_ON(ios->power_mode >= sizeof(pwr_mode) / sizeof(pwr_mode[0]));
	BUG_ON(ios->timing >= sizeof(timing) / sizeof(timing[0]));
	dev_info(mmc_dev(mmc), "sdc set ios: "
		 "clk %dHz bm %s pm %s vdd %d width %d timing %s dt %s\n",
		 ios->clock, bus_mode[ios->bus_mode],
		 pwr_mode[ios->power_mode], ios->vdd,
		 1 << ios->bus_width, timing[ios->timing],
		 drv_type[ios->drv_type]);

	/* Set the power state */
	switch (ios->power_mode) {
	case MMC_POWER_ON:
		break;

	case MMC_POWER_UP:
		if (host->power_on) {
			break;
		}
		if (!IS_ERR(mmc->supply.vmmc)) {
			rval =
			    mmc_regulator_set_ocr(mmc, mmc->supply.vmmc,
						  ios->vdd);
			if (rval) {
				return;
			}
		}
		if (!IS_ERR(mmc->supply.vqmmc)) {
			rval = regulator_enable(mmc->supply.vqmmc);
			if (rval < 0) {
				dev_err(mmc_dev(mmc),
					"failed to enable vqmmc regulator\n");
				return;
			}
		}

		if (gpio_is_valid(host->card_pwr_gpio))
			gpio_set_value(host->card_pwr_gpio,
				       (host->
					ctl_spec_cap &
					CARD_PWR_GPIO_HIGH_ACTIVE) ? 1 : 0);

		if (!IS_ERR(host->pins_default)) {
			rval = pinctrl_select_state(host->pinctrl, host->pins_default);
			if (rval) {
				dev_err(mmc_dev(mmc), "could not set default pins\n");
				return;
			}
		}

		if (!IS_ERR(host->clk_rst)) {
			rval = clk_prepare_enable(host->clk_rst);
			if (rval) {
				dev_err(mmc_dev(mmc), "reset err %d\n", rval);
				return;
			}
		}

		rval = clk_prepare_enable(host->clk_ahb);
		if (rval) {
			dev_err(mmc_dev(mmc), "Enable ahb clk err %d\n", rval);
			return;
		}
		rval = clk_prepare_enable(host->clk_mmc);
		if (rval) {
			dev_err(mmc_dev(mmc), "Enable mmc clk err %d\n", rval);
			return;
		}

		host->ferror = sunxi_mmc_init_host(mmc);
		if (host->ferror)
			return;

		enable_irq(host->irq);

		host->power_on = 1;
		dev_dbg(mmc_dev(mmc), "power on!\n");
		break;

	case MMC_POWER_OFF:
		if (!host->power_on || host->dat3_imask) {
			break;
		}

		disable_irq(host->irq);
		sunxi_mmc_reset_host(host);

		clk_disable_unprepare(host->clk_mmc);
		clk_disable_unprepare(host->clk_ahb);

		if (!IS_ERR(host->clk_rst))
			clk_disable_unprepare(host->clk_rst);

		if (!IS_ERR(host->pins_sleep)) {
			rval = pinctrl_select_state(host->pinctrl, host->pins_sleep);
			if (rval) {
			dev_err(mmc_dev(mmc), "could not set sleep pins\n");
				return;
			}
		}

		if (gpio_is_valid(host->card_pwr_gpio))
			gpio_set_value(host->card_pwr_gpio,
				       (host->
					ctl_spec_cap &
					CARD_PWR_GPIO_HIGH_ACTIVE) ? 0 : 1);

		if (!IS_ERR(mmc->supply.vqmmc)) {
			rval = regulator_disable(mmc->supply.vqmmc);
			if (rval) {
				dev_err(mmc_dev(mmc),
					"Could not disable vqmmc\n");
				return;
			}
		}

		if (!IS_ERR(mmc->supply.vmmc)) {
			rval = mmc_regulator_set_ocr(mmc, mmc->supply.vmmc, 0);
			if (rval)
				return;
		}

		host->power_on = 0;
		dev_dbg(mmc_dev(mmc), "power off!\n");
		break;
	}

	/* set bus width */
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_1:
		mmc_writel(host, REG_WIDTH, SDXC_WIDTH1);
		break;
	case MMC_BUS_WIDTH_4:
		mmc_writel(host, REG_WIDTH, SDXC_WIDTH4);
		break;
	case MMC_BUS_WIDTH_8:
		mmc_writel(host, REG_WIDTH, SDXC_WIDTH8);
		break;
	}

	dev_dbg(mmc_dev(host->mmc), "REG_WIDTH: 0x%08x \n",
		mmc_readl(host, REG_WIDTH));

	/* set ddr mode */
	rval = mmc_readl(host, REG_GCTRL);
	if (ios->timing == MMC_TIMING_UHS_DDR50)
		rval |= SDXC_DDR_MODE;
	else
		rval &= ~SDXC_DDR_MODE;
	mmc_writel(host, REG_GCTRL, rval);
	dev_dbg(mmc_dev(host->mmc), "REG_GCTRL: 0x%08x \n",
		mmc_readl(host, REG_GCTRL));

	/* set up clock */
	//if (ios->clock && ios->power_mode&& host->sunxi_mmc_clk_set_rate) {
	if (ios->power_mode && host->sunxi_mmc_clk_set_rate) {
		host->ferror = host->sunxi_mmc_clk_set_rate(host, ios);
		/* Android code had a usleep_range(50000, 55000); here */
	}
}

static void sunxi_mmc_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	unsigned long flags;
	u32 imask;

	spin_lock_irqsave(&host->lock, flags);

	imask = mmc_readl(host, REG_IMASK);
	if (enable) {
		host->sdio_imask = SDXC_SDIO_INTERRUPT;
		imask |= SDXC_SDIO_INTERRUPT;
	} else {
		host->sdio_imask = 0;
		imask &= ~SDXC_SDIO_INTERRUPT;
	}
	mmc_writel(host, REG_IMASK, imask);
	spin_unlock_irqrestore(&host->lock, flags);
}

static void sunxi_mmc_hw_reset(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	mmc_writel(host, REG_HWRST, 0);
	udelay(10);
	mmc_writel(host, REG_HWRST, 1);
	udelay(300);
}

static int sunxi_mmc_signal_voltage_switch(struct mmc_host *mmc,
					   struct mmc_ios *ios)
{
#ifdef CONFIG_REGULATOR
	int ret = 0;
	struct regulator *vqmmc = mmc->supply.vqmmc;
	struct device_node *np = NULL;
	bool disable_vol_switch = false;

	if (!mmc->parent || !mmc->parent->of_node) {
		dev_err(mmc_dev(mmc),
			"no dts to parse signal switch fun,use default\n");
		return 0;
	}

	np = mmc->parent->of_node;
	disable_vol_switch =
	    of_property_read_bool(np, "sunxi-dis-signal-vol-sw");

	/*For some emmc,io voltage will be fixed at 1.8v or other voltage,so we can not switch io voltage */
	/*Because mmc core will change the io voltage to 3.3v when power up,so will must disable voltage switch */
	if (disable_vol_switch) {
		dev_dbg(mmc_dev(mmc), "disable signal voltage-switch\n");
		return 0;
	}

	switch (ios->signal_voltage) {
	case MMC_SIGNAL_VOLTAGE_330:
		if (!IS_ERR(vqmmc)) {
			ret = regulator_set_voltage(vqmmc, 2700000, 3600000);
			if (ret) {
				dev_err(mmc_dev(mmc),
					"Switching to 3.3V signalling voltage "
					" failed\n");
				return -EIO;
			}
		} else {
			dev_info(mmc_dev(mmc),
				 "no vqmmc,Check if there is regulator\n");
			return 0;
		}
		/* Wait for 5ms */
		//usleep_range(5000, 5500);
		return 0;
	case MMC_SIGNAL_VOLTAGE_180:
		if (!IS_ERR(vqmmc)) {
			ret = regulator_set_voltage(vqmmc, 1700000, 1950000);
			if (ret) {
				dev_err(mmc_dev(mmc),
					"Switching to 1.8V signalling voltage "
					" failed\n");
				return -EIO;
			}
		} else {
			dev_info(mmc_dev(mmc),
				 "no vqmmc,Check if there is regulator\n");
			return 0;
		}

		/* Wait for 5ms */
		//usleep_range(5000, 5500);
		return 0;
	case MMC_SIGNAL_VOLTAGE_120:
		if (!IS_ERR(vqmmc)) {
			ret = regulator_set_voltage(vqmmc, 1100000, 1300000);
			if (ret) {
				dev_err(mmc_dev(mmc),
					"Switching to 1.2V signalling voltage "
					" failed\n");
				return -EIO;
			}
		} else {
			dev_info(mmc_dev(mmc),
				 "no vqmmc,Check if there is regulator\n");
			return 0;
		}

		return 0;
	default:
		/* No signal voltage switch required */
		dev_err(mmc_dev(mmc),
			"unknow signal voltage switch request %x\n",
			ios->signal_voltage);
		return -ENOSYS;
	}
#else
	return 0;
#endif
}

static int sunxi_mmc_card_busy(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	return mmc_readl(host, REG_STAS) & SDXC_CARD_DATA_BUSY;
}


static void sunxi_mmc_parse_cmd(struct mmc_host *mmc, struct mmc_command *cmd , u32 *cval , u32 *im , bool *wdma)
{
	bool wait_dma	= false;
	u32 imask = SDXC_INTERRUPT_ERROR_BIT;
	u32 cmd_val = SDXC_START | (cmd->opcode & 0x3f);

	if (cmd->opcode == MMC_GO_IDLE_STATE) {
		cmd_val |= SDXC_SEND_INIT_SEQUENCE;
		imask |= SDXC_COMMAND_DONE;
	}

	if (cmd->flags & MMC_RSP_PRESENT) {
		cmd_val |= SDXC_RESP_EXPECT;
		if (cmd->flags & MMC_RSP_136)
			cmd_val |= SDXC_LONG_RESPONSE;
		if (cmd->flags & MMC_RSP_CRC)
			cmd_val |= SDXC_CHECK_RESPONSE_CRC;

		if ((cmd->flags & MMC_CMD_MASK) == MMC_CMD_ADTC) {
			cmd_val |= SDXC_DATA_EXPECT | SDXC_WAIT_PRE_OVER;
			if (cmd->data->flags & MMC_DATA_STREAM) {
				imask |= SDXC_AUTO_COMMAND_DONE;
				cmd_val |= SDXC_SEQUENCE_MODE |
				    SDXC_SEND_AUTO_STOP;
			}

			if (cmd->data->stop) {
				imask |= SDXC_AUTO_COMMAND_DONE;
				cmd_val |= SDXC_SEND_AUTO_STOP;
			} else {
				imask |= SDXC_DATA_OVER;
			}

			if (cmd->data->flags & MMC_DATA_WRITE)
				cmd_val |= SDXC_WRITE;
			else if (cmd->data->flags & MMC_DATA_READ)
				wait_dma = true;
			else
				dev_err(mmc_dev(mmc),
					"!!!!!!!Not support cmd->data->flags %x !!!!!!!\n",
					cmd->data->flags);
		} else {
			imask |= SDXC_COMMAND_DONE;
		}
	} else {
		imask |= SDXC_COMMAND_DONE;
	}
	*im			= imask;
	*cval		= cmd_val;
	*wdma		= wait_dma;
}


static void sunxi_mmc_set_dat(struct sunxi_mmc_host *host, struct mmc_host *mmc , struct mmc_data *data)
{
		mmc_writel(host, REG_BLKSZ, data->blksz);
		mmc_writel(host, REG_BCNTR, data->blksz * data->blocks);
		if (host->sunxi_mmc_thld_ctl) {
			host->sunxi_mmc_thld_ctl(host, &mmc->ios, data);
		}
		sunxi_mmc_start_dma(host, data);
}

static void sunxi_mmc_exe_cmd(struct sunxi_mmc_host *host, struct mmc_command *cmd , u32 cmd_val , u32 imask)
{
	u32 rval = 0;
	if (host->dat3_imask) {
		rval = mmc_readl(host, REG_GCTRL);
		rval &= ~SDXC_DEBOUNCE_ENABLE_BIT;
		mmc_writel(host, REG_GCTRL, rval);
	}
	mmc_writel(host, REG_IMASK,
		   host->sdio_imask | host->dat3_imask | imask);
	mmc_writel(host, REG_CARG, cmd->arg);
	mmc_writel(host, REG_CMDR, cmd_val);
}


static void sunxi_mmc_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	struct mmc_command *cmd = mrq->cmd;
	struct mmc_data *data = mrq->data;
	unsigned long iflags;
	bool wait_dma = false;
	u32 imask = 0;
	u32 cmd_val = 0;
	int ret;

	/* Check for set_ios errors (should never happen) */
	if (host->ferror) {
		mrq->cmd->error = host->ferror;
		mmc_request_done(mmc, mrq);
		return;
	}

	if (data) {
		ret = sunxi_mmc_map_dma(host, data);
		if (ret < 0) {
			dev_err(mmc_dev(mmc), "map DMA failed\n");
			cmd->error = ret;
			data->error = ret;
			mmc_request_done(mmc, mrq);
			return;
		}
	}

	sunxi_mmc_parse_cmd(mmc , cmd , &cmd_val , &imask, &wait_dma);

	dev_dbg(mmc_dev(mmc), "cmd %d(%08x) arg %x ie 0x%08x len %d\n",
		cmd_val & 0x3f, cmd_val, cmd->arg, imask,
		mrq->data ? mrq->data->blksz * mrq->data->blocks : 0);

	spin_lock_irqsave(&host->lock, iflags);

	if (host->mrq || host->manual_stop_mrq
		|| host->mrq_busy || host->mrq_retry) {
		spin_unlock_irqrestore(&host->lock, iflags);

		if (data)
			dma_unmap_sg(mmc_dev(mmc), data->sg, data->sg_len,
				     sunxi_mmc_get_dma_dir(data));

		dev_err(mmc_dev(mmc), "request already pending\n");
		mrq->cmd->error = -EBUSY;
		mmc_request_done(mmc, mrq);
		return;
	}

	if (data) {
		spin_unlock_irqrestore(&host->lock, iflags);
		sunxi_mmc_set_dat(host , mmc , data);
		spin_lock_irqsave(&host->lock, iflags);
	}

	host->mrq = mrq;
	host->wait_dma = wait_dma;
	sunxi_mmc_exe_cmd(host, cmd , cmd_val , imask);

	spin_unlock_irqrestore(&host->lock, iflags);
}

/*we use our own mmc_regulator_get_supply because our platform regulator not support supply name,*/
/*only support regulator ID,but linux mmc' own mmc_regulator_get_supply use supply name*/
static int sunxi_mmc_regulator_get_supply(struct mmc_host *mmc)
{
	struct device *dev = mmc_dev(mmc);
	int ret = 0;
	int i = 0;
	struct device_node *np = NULL;
	struct property *prop = NULL;
	char *pwr_prop_str[] = { "vmmc", "vqmmc", "vdmmc" };
	const char *reg_str[sizeof(pwr_prop_str) / sizeof(pwr_prop_str[0])] = { NULL };

	if (!mmc->parent || !mmc->parent->of_node) {
		dev_err(mmc_dev(mmc), "not dts found\n");
		return -EINVAL;
	}

	np = mmc->parent->of_node;
	for (i = 0; i < sizeof(pwr_prop_str) / sizeof(pwr_prop_str[0]); i++) {
		prop = of_find_property(np, pwr_prop_str[i], NULL);
		if (!prop) {
			dev_info(dev, "Can't get %s regulator string\n",
				 pwr_prop_str[i]);
			continue;
		}
		reg_str[i] = devm_kzalloc(&mmc->class_dev, prop->length,
					  GFP_KERNEL);
		if (!reg_str[i]) {
			dev_err(dev, "Can't get mem for %s regulator string \n",
				pwr_prop_str[i]);
			return -EINVAL;
		}

		ret = of_property_read_string(np, pwr_prop_str[i], &reg_str[i]);
		if (ret) {
			dev_err(dev, "read regulator prop %s failed\n",
				pwr_prop_str[i]);
			return ret;
		}
		dev_info(dev, "regulator prop %s,str %s\n", pwr_prop_str[i],
			 reg_str[i]);
	}

	/*if there is not regulator,we should make supply to ERR_PTR to make sure that other code know there is not regulator */
	/*Because our regulator driver does not support binding to device tree,so we can not binding it to our dev(for example regulator_get(dev, reg_str[0]) or devm_regulator_get(dev, reg_str[0]) ) */
		/*
		mmc->supply.vmmc = (reg_str[0]== NULL)?(ERR_PTR(-ENODEV)):devm_regulator_get(NULL, reg_str[0]);
		mmc->supply.vqmmc = (reg_str[1]== NULL)?(ERR_PTR(-ENODEV)):devm_regulator_get(NULL, reg_str[1]);
		mmc->supply.vdmmc = (reg_str[2]== NULL)?(ERR_PTR(-ENODEV)):devm_regulator_get(NULL, reg_str[2]);
		*/
	mmc->supply.vmmc = regulator_get(NULL, reg_str[0]);
	mmc->supply.vqmmc = regulator_get(NULL, reg_str[1]);
	mmc->supply.vdmmc = regulator_get(NULL, reg_str[2]);

	if (IS_ERR(mmc->supply.vmmc)) {
		dev_info(dev, "No vmmc regulator found\n");
	} else {
		ret = mmc_regulator_get_ocrmask(mmc->supply.vmmc);
		if (ret > 0)
			mmc->ocr_avail = ret;
		else
			dev_warn(dev, "Failed getting OCR mask: %d\n", ret);
	}

	if (IS_ERR(mmc->supply.vqmmc)) {
		dev_info(dev, "No vqmmc regulator found\n");
	}

	if (IS_ERR(mmc->supply.vdmmc)) {
		dev_info(dev, "No vdmmc regulator found\n");
	}

	return 0;
}

/*Because our regulator driver does not support binding to device tree,so we can not binding it to our dev(for example regulator_get(dev, reg_str[0]) or devm_regulator_get(dev, reg_str[0]) )*/
/*so we must release it manully*/
static void sunxi_mmc_regulator_release_supply(struct mmc_host *mmc)
{
	if (!IS_ERR(mmc->supply.vdmmc)) {
		regulator_put(mmc->supply.vdmmc);
	}

	if (!IS_ERR(mmc->supply.vqmmc)) {
		regulator_put(mmc->supply.vqmmc);
	}

	if (!IS_ERR(mmc->supply.vmmc)) {
		regulator_put(mmc->supply.vmmc);
	}
}

static const struct of_device_id sunxi_mmc_of_match[] = {
	{.compatible = "allwinner,sun4i-a10-mmc",},
	{.compatible = "allwinner,sun5i-a13-mmc",},
	{.compatible = "allwinner,sun8iw10p1-sdmmc3",},
	{.compatible = "allwinner,sun8iw10p1-sdmmc1",},
	{.compatible = "allwinner,sun8iw10p1-sdmmc0",},
	{.compatible = "allwinner,sun50i-sdmmc2",},
	{.compatible = "allwinner,sun50i-sdmmc1",},
	{.compatible = "allwinner,sun50i-sdmmc0",},
	{.compatible = "allwinner,sunxi-mmc-v4p1x",},
	{.compatible = "allwinner,sunxi-mmc-v4p5x",},
		{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sunxi_mmc_of_match);

static struct mmc_host_ops sunxi_mmc_ops = {
	.request = sunxi_mmc_request,
	.set_ios = sunxi_mmc_set_ios,
	.get_ro = mmc_gpio_get_ro,
	.get_cd = mmc_gpio_get_cd,
	.enable_sdio_irq = sunxi_mmc_enable_sdio_irq,
	.hw_reset = sunxi_mmc_hw_reset,
	.start_signal_voltage_switch = sunxi_mmc_signal_voltage_switch,
	.card_busy = sunxi_mmc_card_busy,
};

#if defined(MMC_FPGA) && defined(CONFIG_ARCH_SUN8IW10P1)
void disable_card2_dat_det(void)
{
	void __iomem *card2_int_sg_en =
	    ioremap(0x1c0f000 + 0x1000 * 2 + 0x38, 0x100);
	writel(0, card2_int_sg_en);
	iounmap(card2_int_sg_en);
}

void enable_card3(void)
{
	void __iomem *card3_en = ioremap(0x1c20800 + 0xB4, 0x100);
	//void __iomem *card3_en =  ioremap(0x1c20800 + 0x48, 0x100);//
	writel(0x55555555, card3_en);
	writel(0x55555555, card3_en + 4);
	writel(0x55555555, card3_en + 8);
	writel(0x55555555, card3_en + 12);
	iounmap(card3_en);
}

#endif


//The following shutdown only use for sdmmc2 to be compatible with a20
extern int mmc_go_idle(struct mmc_host *host);
extern int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr);
extern int mmc_send_status(struct mmc_card *card, u32 *status);
extern void mmc_set_clock(struct mmc_host *host, unsigned int hz);
extern void mmc_set_timing(struct mmc_host *host, unsigned int timing);
extern void mmc_set_bus_width(struct mmc_host *host, unsigned int width);
void sunxi_mmc_do_shutdown_com(struct platform_device *pdev)
{
	u32 ocr = 0;
	u32 err = 0;
	struct mmc_host *mmc = NULL;
	struct sunxi_mmc_host *host = NULL;
	u32 status = 0;

	mmc = platform_get_drvdata(pdev);
	if (mmc == NULL) {
		dev_err(&pdev->dev, "%s: mmc is NULL\n", __FUNCTION__);
		goto out;
	}

	host = mmc_priv(mmc);
	if (host == NULL) {
		dev_err(&pdev->dev, "%s: host is NULL\n", __FUNCTION__);
		goto out;
	}

	dev_info(mmc_dev(mmc), "try to disable cache\n");
	mmc_claim_host(mmc);
	err = mmc_cache_ctrl(mmc, 0);
	mmc_release_host(mmc);
	if (err) {
		dev_err(mmc_dev(mmc), "disable cache failed\n");
		mmc_claim_host(mmc);	/*not release host to not allow android to read/write after shutdown*/
		goto out;
	}
	//claim host to not allow androd read/write during shutdown
	dev_dbg(mmc_dev(mmc), "%s: claim host\n", __FUNCTION__);
	mmc_claim_host(mmc);

	do {
		if (mmc_send_status(mmc->card, &status) != 0) {
			dev_err(mmc_dev(mmc), "%s: send status failed\n",
				__FUNCTION__);
			goto out;	/*err_out; //not release host to not allow android to read/write after shutdown*/
		}
	} while (status != 0x00000900);

	//mmc_card_set_ddr_mode(card);
	mmc_set_timing(mmc, MMC_TIMING_LEGACY);
	mmc_set_bus_width(mmc, MMC_BUS_WIDTH_1);
	mmc_set_clock(mmc, 400000);
	err = mmc_go_idle(mmc);
	if (err) {
		dev_err(mmc_dev(mmc), "%s: mmc_go_idle err\n", __FUNCTION__);
		goto out;	/*err_out; //not release host to not allow android to read/write after shutdown*/
	}

	if (mmc->card->type != MMC_TYPE_MMC) {	/*sd can support cmd1,so not send cmd1*/
		goto out;	/*not release host to not allow android to read/write after shutdown*/
	}

	err = mmc_send_op_cond(mmc, 0, &ocr);
	if (err) {
		dev_err(mmc_dev(mmc), "%s: first mmc_send_op_cond err\n",
			__FUNCTION__);
		goto out;	/*err_out; //not release host to not allow android to read/write after shutdown*/
	}

	err = mmc_send_op_cond(mmc, ocr | (1 << 30), &ocr);
	if (err) {
		dev_err(mmc_dev(mmc), "%s: mmc_send_op_cond err\n",
			__FUNCTION__);
		goto out;	/*err_out; //not release host to not allow android to read/write after shutdown*/
	}
	/*do not release host to not allow android to read/write after shutdown*/
	goto out;

      out:
	dev_info(mmc_dev(mmc), "%s: mmc shutdown exit..ok\n", __FUNCTION__);

	return;
}


static int sunxi_mmc_resource_request(struct sunxi_mmc_host *host,
				      struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;
	u32 caps_val = 0;
	struct gpio_config flags;

	ret = of_property_read_u32(np, "ctl-spec-caps", &caps_val);
	if (!ret) {
		host->ctl_spec_cap |= caps_val;
		dev_info(&pdev->dev, "***ctl-spec-caps*** %x\n", host->ctl_spec_cap);
	}
#ifdef SUNXI_SDMMC3
	if (of_device_is_compatible(np, "allwinner,sun8iw10p1-sdmmc3")) {
		host->sunxi_mmc_clk_set_rate =
		    sunxi_mmc_clk_set_rate_for_sdmmc3;
		//host->dma_tl = (0x3<<28)|(15<<16)|240;
		host->dma_tl = SUNXI_DMA_TL_SDMMC3;
		//host->idma_des_size_bits = 12;
		host->idma_des_size_bits = SUNXI_DES_SIZE_SDMMC3;
		host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_for_sdmmc3;
		host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg3;
		host->sunxi_mmc_restore_spec_reg = sunxi_mmc_restore_spec_reg3;
		host->sunxi_mmc_dump_dly_table = sunxi_mmc_dump_dly3;
		sunxi_mmc_reg_ex_res_inter(host, 3);
		host->sunxi_mmc_set_acmda = sunxi_mmc_set_a12a;
		host->sunxi_mmc_shutdown = sunxi_mmc_do_shutdown3;
		host->phy_index = 3;	/*2;*/
	}

#if defined(MMC_FPGA) && defined(CONFIG_ARCH_SUN8IW10P1)
	enable_card3();
#endif				/*defined(MMC_FPGA) && defined(CONFIG_ARCH_SUN8IW10P1) */

#endif

#ifdef SUNXI_SDMMC2
	if (of_device_is_compatible(np, "allwinner,sun50i-sdmmc2")) {
		host->sunxi_mmc_clk_set_rate =
		    sunxi_mmc_clk_set_rate_for_sdmmc2;
		//host->dma_tl = (0x3<<28)|(15<<16)|240;
		host->dma_tl = SUNXI_DMA_TL_SDMMC2;
		//host->idma_des_size_bits = 12;
		host->idma_des_size_bits = SUNXI_DES_SIZE_SDMMC2;
		host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_for_sdmmc2;
		host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg2;
		host->sunxi_mmc_restore_spec_reg = sunxi_mmc_restore_spec_reg2;
		host->sunxi_mmc_dump_dly_table = sunxi_mmc_dump_dly2;
		sunxi_mmc_reg_ex_res_inter(host, 2);
		host->sunxi_mmc_set_acmda = sunxi_mmc_set_a12a;
		if (host->ctl_spec_cap & NO_REINIT_SHUTDOWN) {
			host->sunxi_mmc_shutdown = sunxi_mmc_do_shutdown2;
		} else {
			/*only be compatible with a20 */
			host->sunxi_mmc_shutdown = sunxi_mmc_do_shutdown_com;
		}
		host->phy_index = 2;
	}
#endif

#ifdef SUNXI_SDMMC0
	if (of_device_is_compatible(np, "allwinner,sun50i-sdmmc0")
	    || of_device_is_compatible(np, "allwinner,sun8iw10p1-sdmmc0")) {
		host->sunxi_mmc_clk_set_rate =
		    sunxi_mmc_clk_set_rate_for_sdmmc0;
		//host->dma_tl = (0x2<<28)|(7<<16)|248;
		host->dma_tl = SUNXI_DMA_TL_SDMMC0;
		//host->idma_des_size_bits = 15;
		host->idma_des_size_bits = SUNXI_DES_SIZE_SDMMC0;
		host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_for_sdmmc0;
		host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg0;
		host->sunxi_mmc_restore_spec_reg = sunxi_mmc_restore_spec_reg0;
		sunxi_mmc_reg_ex_res_inter(host, 0);
		host->sunxi_mmc_set_acmda = sunxi_mmc_set_a12a;
		host->phy_index = 0;
	}
#endif


#ifdef SUNXI_SDMMC1
	if (of_device_is_compatible(np, "allwinner,sun50i-sdmmc1")
	    || of_device_is_compatible(np, "allwinner,sun8iw10p1-sdmmc1")) {
		host->sunxi_mmc_clk_set_rate =
		    sunxi_mmc_clk_set_rate_for_sdmmc1;
		//host->dma_tl = (0x3<<28)|(15<<16)|240;
		host->dma_tl = SUNXI_DMA_TL_SDMMC1;
		//host->idma_des_size_bits = 15;
		host->idma_des_size_bits = SUNXI_DES_SIZE_SDMMC1;
		host->sunxi_mmc_thld_ctl = sunxi_mmc_thld_ctl_for_sdmmc1;
		host->sunxi_mmc_save_spec_reg = sunxi_mmc_save_spec_reg1;
		host->sunxi_mmc_restore_spec_reg = sunxi_mmc_restore_spec_reg1;
		sunxi_mmc_reg_ex_res_inter(host, 1);
		host->sunxi_mmc_set_acmda = sunxi_mmc_set_a12a;
		host->phy_index = 1;
	}
#endif
	if (of_device_is_compatible(np, "allwinner,sunxi-mmc-v4p1x"))	{
		int phy_index = 0;
		if (of_property_match_string(np , "device_type" , "sdc0") == 0) {
			phy_index = 0;
		} else if (of_property_match_string(np , "device_type" , "sdc1") == 0) {
			phy_index = 1;
		} else if (of_property_match_string(np , "device_type" , "sdc2") == 0) {
			phy_index = 2;
		} else if (of_property_match_string(np , "device_type" , "sdc3") == 0) {
			phy_index = 3;
		} else {
			dev_err(&pdev->dev , "No sdmmc device,check dts\n");
		}
		sunxi_mmc_init_priv_v4p1x(host , pdev , phy_index);
	}

	if (of_device_is_compatible(np, "allwinner,sunxi-mmc-v4p5x"))	{
		int phy_index = 0;
		if (of_property_match_string(np , "device_type" , "sdc0") == 0) {
			phy_index = 0;
		} else if (of_property_match_string(np , "device_type" , "sdc1") == 0) {
			phy_index = 1;
		} else if (of_property_match_string(np , "device_type" , "sdc2") == 0) {
			phy_index = 2;
		} else if (of_property_match_string(np , "device_type" , "sdc3") == 0) {
			phy_index = 3;
		} else {
			dev_err(&pdev->dev , "No sdmmc device,check dts\n");
		}
		sunxi_mmc_init_priv_v4p5x(host , pdev , phy_index);
	}


	//ret = mmc_regulator_get_supply(host->mmc);
	ret = sunxi_mmc_regulator_get_supply(host->mmc);
	if (ret) {
		return ret;
	}
	/*Maybe in some platform,no regulator,so we set ocr_avail manully */
	if (!host->mmc->ocr_avail)
		host->mmc->ocr_avail =
		    MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 |
		    MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34;

	//enable card detect pin power
	if (!IS_ERR(host->mmc->supply.vdmmc)) {
		ret = regulator_enable(host->mmc->supply.vdmmc);
		if (ret < 0) {
			dev_err(mmc_dev(host->mmc),
				"failed to enable vdmmc regulator\n");
			return ret;
		}
	}

	host->card_pwr_gpio =
	    of_get_named_gpio_flags(np, "card-pwr-gpios", 0,
				    (enum of_gpio_flags *)&flags);
	if (gpio_is_valid(host->card_pwr_gpio)) {
		ret =
		    devm_gpio_request_one(&pdev->dev, host->card_pwr_gpio,
					  GPIOF_DIR_OUT, "card-pwr-gpios");
		if (ret < 0)
			dev_err(&pdev->dev,
				"could not get  card-pwr-gpios gpio\n");
	}

	host->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(host->pinctrl)) {
		dev_warn(&pdev->dev, "Could not get pinctrl,check if pin is needed\n");
	}

	host->pins_default = pinctrl_lookup_state(host->pinctrl,
					  PINCTRL_STATE_DEFAULT);
	if (IS_ERR(host->pins_default)) {
		dev_warn(&pdev->dev, "could not get default pinstate,check if pin is needed\n");
	}

	host->pins_sleep = pinctrl_lookup_state(host->pinctrl,
					PINCTRL_STATE_SLEEP);
	if (IS_ERR(host->pins_sleep)) {
		dev_warn(&pdev->dev, "could not get sleep pinstate,check if pin is needed\n");
	}

	host->reg_base = devm_ioremap_resource(&pdev->dev,
					       platform_get_resource(pdev,
								     IORESOURCE_MEM,
								     0));
	if (IS_ERR(host->reg_base)) {
		ret = PTR_ERR(host->reg_base);
		goto error_disable_regulator;
	}

	host->clk_ahb = devm_clk_get(&pdev->dev, "ahb");
	if (IS_ERR(host->clk_ahb)) {
		dev_err(&pdev->dev, "Could not get ahb clock\n");
		ret = PTR_ERR(host->clk_ahb);
		goto error_disable_regulator;
	}

	host->clk_mmc = devm_clk_get(&pdev->dev, "mmc");
	if (IS_ERR(host->clk_mmc)) {
		dev_err(&pdev->dev, "Could not get mmc clock\n");
		ret = PTR_ERR(host->clk_mmc);
		goto error_disable_regulator;
	}

	host->clk_rst = devm_clk_get(&pdev->dev, "rst");
	if (IS_ERR(host->clk_rst)) {
		dev_warn(&pdev->dev, "Could not get mmc rst\n");
	}

	if (!IS_ERR(host->clk_rst)) {
		ret = clk_prepare_enable(host->clk_rst);
		if (ret) {
			dev_err(&pdev->dev, "reset err %d\n", ret);
			goto error_disable_regulator;
		}
	}

	ret = clk_prepare_enable(host->clk_ahb);
	if (ret) {
		dev_err(&pdev->dev, "Enable ahb clk err %d\n", ret);
		goto error_assert_reset;
	}

	ret = clk_prepare_enable(host->clk_mmc);
	if (ret) {
		dev_err(&pdev->dev, "Enable mmc clk err %d\n", ret);
		goto error_disable_clk_ahb;
	}
#if defined(MMC_FPGA) && defined(CONFIG_ARCH_SUN8IW10P1)
	disable_card2_dat_det();
#endif
	/*
	 * Sometimes the controller asserts the irq on boot for some reason,
	 * make sure the controller is in a sane state before enabling irqs.
	 */
	ret = sunxi_mmc_reset_host(host);
	if (ret)
		goto error_disable_clk_mmc;

	host->irq = platform_get_irq(pdev, 0);
	ret = devm_request_threaded_irq(&pdev->dev, host->irq, sunxi_mmc_irq,
					sunxi_mmc_handle_bottom_half, 0,
					"sunxi-mmc", host);
	if (ret) {
		dev_err(&pdev->dev, "faild to request irq %d\n", ret);
		goto error_disable_clk_mmc;
	}

	disable_irq(host->irq);

	clk_disable_unprepare(host->clk_mmc);
	clk_disable_unprepare(host->clk_ahb);

	if (!IS_ERR(host->clk_rst))
		clk_disable_unprepare(host->clk_rst);

	return ret;

      error_disable_clk_mmc:
	clk_disable_unprepare(host->clk_mmc);
      error_disable_clk_ahb:
	clk_disable_unprepare(host->clk_ahb);
      error_assert_reset:
#if 0
	if (!IS_ERR(host->reset))
		reset_control_assert(host->reset);
#else
	if (!IS_ERR(host->clk_rst))
		clk_disable_unprepare(host->clk_rst);
#endif
      error_disable_regulator:
	if (!IS_ERR(host->mmc->supply.vdmmc))
		regulator_disable(host->mmc->supply.vdmmc);
	sunxi_mmc_regulator_release_supply(host->mmc);

	return ret;
}

static int sunxi_mmc_probe(struct platform_device *pdev)
{
	struct sunxi_mmc_host *host;
	struct mmc_host *mmc;
	int ret;

	dev_info(&pdev->dev, "%s\n", DRIVER_VERSION);

	mmc = mmc_alloc_host(sizeof(struct sunxi_mmc_host), &pdev->dev);
	if (!mmc) {
		dev_err(&pdev->dev, "mmc alloc host failed\n");
		return -ENOMEM;
	}

	host = mmc_priv(mmc);
	host->mmc = mmc;
	spin_lock_init(&host->lock);

	ret = sunxi_mmc_resource_request(host, pdev);
	if (ret)
		goto error_free_host;

	host->dma_mask = DMA_BIT_MASK(32);
	pdev->dev.dma_mask = &host->dma_mask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	host->sg_cpu = dma_alloc_coherent(&pdev->dev, SUNXI_REQ_PAGE_SIZE,
					  &host->sg_dma, GFP_KERNEL);
	if (!host->sg_cpu) {
		dev_err(&pdev->dev, "Failed to allocate DMA descriptor mem\n");
		ret = -ENOMEM;
		goto error_free_host;
	}

	mmc->ops = &sunxi_mmc_ops;
	mmc->max_blk_count = 8192;
	mmc->max_blk_size = 4096;
	mmc->max_segs = SUNXI_REQ_PAGE_SIZE / sizeof(struct sunxi_idma_des);
	mmc->max_seg_size = (1 << host->idma_des_size_bits);
	mmc->max_req_size = mmc->max_seg_size * mmc->max_segs;
	/* 400kHz ~ 50MHz */
	mmc->f_min = 400000;
	mmc->f_max = 50000000;
	mmc->caps |=
	    MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED | MMC_CAP_ERASE |
	    MMC_CAP_WAIT_WHILE_BUSY;
	mmc->max_busy_timeout = 0x7ffffff;	/*ms*/

#ifndef CONFIG_REGULATOR
	//Because fpga has no regulator,so we add it manully
	mmc->ocr_avail =
	    MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 |
	    MMC_VDD_32_33 | MMC_VDD_33_34;
	dev_info(&pdev->dev,
		 "***set host ocr***\n");

#endif

	mmc_of_parse(mmc);
	if (mmc->sunxi_caps3 & MMC_SUNXI_CAP3_DAT3_DET) {
		host->dat3_imask = SDXC_CARD_INSERT | SDXC_CARD_REMOVE;
		dev_info(mmc_dev(host->mmc),
			 "***enable data3 detect***\n");
	}

	if (mmc_gpio_get_cd(mmc)) {
		if (gpio_is_valid(host->card_pwr_gpio))
			gpio_set_value(host->card_pwr_gpio,
				       (host->
					ctl_spec_cap &
					CARD_PWR_GPIO_HIGH_ACTIVE) ? 1 : 0);
	} else {
		if (gpio_is_valid(host->card_pwr_gpio))
			gpio_set_value(host->card_pwr_gpio,
				       (host->
					ctl_spec_cap &
					CARD_PWR_GPIO_HIGH_ACTIVE) ? 0 : 1);
	}

	ret = mmc_add_host(mmc);
	if (ret)
		goto error_free_dma;

	ret = mmc_create_sys_fs(host, pdev);
	if (ret) {
		dev_err(&pdev->dev, "create sys fs failed\n");
		goto error_free_dma;
	}

	dev_info(&pdev->dev, "base:0x%p irq:%u\n", host->reg_base, host->irq);
	platform_set_drvdata(pdev, mmc);
	return 0;

      error_free_dma:
	dma_free_coherent(&pdev->dev, SUNXI_REQ_PAGE_SIZE, host->sg_cpu,
			  host->sg_dma);
      error_free_host:
	mmc_free_host(mmc);
	return ret;
}

static int sunxi_mmc_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	mmc_remove_host(mmc);
	disable_irq(host->irq);
	sunxi_mmc_reset_host(host);

	mmc_remove_sys_fs(host, pdev);

	if (!IS_ERR(mmc->supply.vdmmc))
		regulator_disable(mmc->supply.vdmmc);

	sunxi_mmc_regulator_release_supply(mmc);

	dma_free_coherent(&pdev->dev, SUNXI_REQ_PAGE_SIZE, host->sg_cpu,
			  host->sg_dma);
	mmc_free_host(mmc);

	return 0;
}



static void sunxi_mmc_regs_save(struct sunxi_mmc_host *host)
{
	struct sunxi_mmc_ctrl_regs *bak_regs = &host->bak_regs;

	/*save public register */
	bak_regs->gctrl = mmc_readl(host, REG_GCTRL);
	bak_regs->clkc = mmc_readl(host, REG_CLKCR);
	bak_regs->timeout = mmc_readl(host, REG_TMOUT);
	bak_regs->buswid = mmc_readl(host, REG_WIDTH);
	bak_regs->waterlvl = mmc_readl(host, REG_FTRGL);
	bak_regs->funcsel = mmc_readl(host, REG_FUNS);
	bak_regs->debugc = mmc_readl(host, REG_DBGC);
	bak_regs->idmacc = mmc_readl(host, REG_DMAC);
	bak_regs->dlba = mmc_readl(host, REG_DLBA);
	bak_regs->imask = mmc_readl(host, REG_IMASK);

	if (host->sunxi_mmc_save_spec_reg) {
		host->sunxi_mmc_save_spec_reg(host);
	} else {
		dev_warn(mmc_dev(host->mmc), "no spec reg save\n");
	}
}

static void sunxi_mmc_regs_restore(struct sunxi_mmc_host *host)
{
	struct sunxi_mmc_ctrl_regs *bak_regs = &host->bak_regs;

	/*restore public register */
	mmc_writel(host, REG_GCTRL, bak_regs->gctrl);
	mmc_writel(host, REG_CLKCR, bak_regs->clkc);
	mmc_writel(host, REG_TMOUT, bak_regs->timeout);
	mmc_writel(host, REG_WIDTH, bak_regs->buswid);
	mmc_writel(host, REG_FTRGL, bak_regs->waterlvl);
	mmc_writel(host, REG_FUNS, bak_regs->funcsel);
	mmc_writel(host, REG_DBGC, bak_regs->debugc);
	mmc_writel(host, REG_DMAC, bak_regs->idmacc);
	mmc_writel(host, REG_DLBA, bak_regs->dlba);
	mmc_writel(host, REG_IMASK, bak_regs->imask);

	if (host->sunxi_mmc_restore_spec_reg) {
		host->sunxi_mmc_restore_spec_reg(host);
	} else {
		dev_warn(mmc_dev(host->mmc), "no spec reg restore\n");
	}
	if (host->sunxi_mmc_set_acmda) {
		host->sunxi_mmc_set_acmda(host);
	}
}
#ifdef CONFIG_PM

static int sunxi_mmc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int ret = 0;

	if (mmc) {
		ret = mmc_suspend_host(mmc);
		if (!ret) {
			if (!IS_ERR(mmc->supply.vdmmc)) {
				ret = regulator_disable(mmc->supply.vdmmc);
				if (ret) {
					dev_err(mmc_dev(mmc),
						"disable vdmmc failed in suspend\n");
					return ret;
				}
			}

			if (mmc_card_keep_power(mmc) || host->dat3_imask) {
				disable_irq(host->irq);
				sunxi_mmc_regs_save(host);

				clk_disable_unprepare(host->clk_mmc);
				clk_disable_unprepare(host->clk_ahb);

				if (!IS_ERR(host->clk_rst))
					clk_disable_unprepare(host->clk_rst);

				if (!IS_ERR(host->pins_sleep)) {
					ret = pinctrl_select_state(host->pinctrl , host->pins_sleep);
					if (ret) {
						dev_err(mmc_dev(mmc),
							"could not set sleep pins in suspend\n");
						return ret;
					}
				}
				if (!IS_ERR(mmc->supply.vqmmc))
					regulator_disable(mmc->supply.vqmmc);

				if (!IS_ERR(mmc->supply.vmmc)) {
					ret =
					    mmc_regulator_set_ocr(mmc,
								  mmc->supply.
								  vmmc, 0);
					return ret;
				}
				dev_dbg(mmc_dev(mmc), "dat3_imask %x\n",
					 host->dat3_imask);
					/*dump_reg(host);*/
			}
			sunxi_mmc_gpio_suspend_cd(mmc);
			/*sunxi_dump_reg(mmc);*/
		}
	}

	return ret;
}

static int sunxi_mmc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);
	int ret = 0;

	if (mmc) {
		sunxi_mmc_gpio_resume_cd(mmc);
		if (mmc_card_keep_power(mmc) || host->dat3_imask) {
			if (!IS_ERR(mmc->supply.vmmc)) {
				ret =
				    mmc_regulator_set_ocr(mmc, mmc->supply.vmmc,
							  mmc->ios.vdd);
				if (ret)
					return ret;
			}

			if (!IS_ERR(mmc->supply.vqmmc)) {
				ret = regulator_enable(mmc->supply.vqmmc);
				if (ret < 0) {
					dev_err(mmc_dev(mmc),
						"failed to enable vqmmc regulator\n");
					return ret;
				}
			}

			if (!IS_ERR(host->pins_default)) {
				ret = pinctrl_select_state(host->pinctrl , host->pins_default);
				if (ret) {
					dev_err(mmc_dev(mmc),
						"could not set default pins in resume\n");
					return ret;
				}
			}

			if (!IS_ERR(host->clk_rst)) {
				ret = clk_prepare_enable(host->clk_rst);
				if (ret) {
					dev_err(mmc_dev(mmc), "reset err %d\n",
						ret);
					return ret;
				}
			}

			ret = clk_prepare_enable(host->clk_ahb);
			if (ret) {
				dev_err(mmc_dev(mmc), "Enable ahb clk err %d\n",
					ret);
				return ret;
			}
			ret = clk_prepare_enable(host->clk_mmc);
			if (ret) {
				dev_err(mmc_dev(mmc), "Enable mmc clk err %d\n",
					ret);
				return ret;
			}

			host->ferror = sunxi_mmc_init_host(mmc);
			if (host->ferror)
				return -1;

			sunxi_mmc_regs_restore(host);
			host->ferror = sunxi_mmc_update_clk(host);
			if (host->ferror)
				return -1;

			enable_irq(host->irq);
			dev_info(mmc_dev(mmc), "dat3_imask %x\n",
				 host->dat3_imask);
				/*dump_reg(host);*/
		}
		//enable card detect pin power
		if (!IS_ERR(mmc->supply.vdmmc)) {
			ret = regulator_enable(mmc->supply.vdmmc);
			if (ret < 0) {
				dev_err(mmc_dev(mmc),
					"failed to enable vdmmc regulator\n");
				return ret;
			}
		}
		/*sunxi_dump_reg(mmc);*/
		ret = mmc_resume_host(mmc);
	}

	return ret;
}

static const struct dev_pm_ops sunxi_mmc_pm = {
	.suspend = sunxi_mmc_suspend,
	.resume = sunxi_mmc_resume,
};

#define sunxi_mmc_pm_ops &sunxi_mmc_pm

#else				/* CONFIG_PM */

#define sunxi_mmc_pm_ops NULL

#endif				/* CONFIG_PM */

void sunxi_shutdown_mmc(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct sunxi_mmc_host *host = mmc_priv(mmc);

	if (host->sunxi_mmc_shutdown) {
		host->sunxi_mmc_shutdown(pdev);
	}
}


static struct platform_driver sunxi_mmc_driver = {
	.driver = {
		   .name = "sunxi-mmc",
		   .of_match_table = of_match_ptr(sunxi_mmc_of_match),
		   .pm = sunxi_mmc_pm_ops,
		   },
	.probe = sunxi_mmc_probe,
	.remove = sunxi_mmc_remove,
	.shutdown = sunxi_shutdown_mmc,
};

module_platform_driver(sunxi_mmc_driver);

MODULE_DESCRIPTION("Allwinner's SD/MMC Card Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("David Lanzend�rfer <david.lanzendoerfer@o2s.ch>");
MODULE_ALIAS("platform:sunxi-mmc");
