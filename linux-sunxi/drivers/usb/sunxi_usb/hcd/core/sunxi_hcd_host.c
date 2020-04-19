/*
 * drivers/usb/sunxi_usb/hcd/core/sunxi_hcd_host.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>

#include  "../include/sunxi_hcd_config.h"
#include  "../include/sunxi_hcd_core.h"
#include  "../include/sunxi_hcd_virt_hub.h"
#include  "../include/sunxi_hcd_host.h"
#include  "../include/sunxi_hcd_dma.h"

#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
#define DOBULE_EP  1
#define BAND_WIDTH_ISO_EP 4
#define BAND_WIDTH_ISO_EP_EX 5
int is_first_iso = 1;
#endif

/*
 * NOTE on endpoint usage:
 *
 * CONTROL transfers all go through ep0.  BULK ones go through dedicated IN
 * and OUT endpoints ... hardware is dedicated for those "async" queue(s).
 * (Yes, bulk _could_ use more of the endpoints than that, and would even
 * benefit from it.)
 *
 * INTERUPPT and ISOCHRONOUS transfers are scheduled to the other endpoints.
 * So far that scheduling is both dumb and optimistic:  the endpoint will be
 * "claimed" until its software queue is no longer refilled.  No multiplexing
 * of transfers between endpoints, or anything clever.
 */
static void sunxi_hcd_ep_program(struct sunxi_hcd *sunxi_hcd,
						u8 epnum,
						struct urb *urb,
						int is_out,
						u8 *buf,
						u32 offset,
						u32 len);

#if 0
/*
 * @ep: physic ep info
 */
static void sunxi_hcd_h_ep0_flush_fifo(struct sunxi_hcd_hw_ep *ep)
{
	void __iomem *usbc_base = NULL;
	u16 csr = 0;
	int retries = 5;

	if (ep == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return;
	}

	usbc_base = ep->regs;

	/* scrub any data left in the fifo */
	do {
		csr = USBC_Readw(USBC_REG_CSR0(usbc_base));
		if (!(csr & ((1 << USBC_BP_CSR0_H_TxPkRdy)
				| (1 << USBC_BP_CSR0_H_RxPkRdy))))
			break;

		USBC_REG_set_bit_w(USBC_BP_CSR0_H_FlushFIFO,
				USBC_REG_CSR0(usbc_base));

		csr = USBC_Readw(USBC_REG_CSR0(usbc_base));
		udelay(10);
	} while (--retries);

	if (retries == 0)
		DMSG_PANIC("ERR: Could not flush host TX%d fifo: csr: %04x\n",
			ep->epnum, csr);

	/* and reset for the next transfer */
	USBC_Writew(0x00, USBC_REG_CSR0(usbc_base));

	return;
}

/*
 * Clear TX fifo. Needed to avoid BABBLE errors.
 * @ep: physic ep info
 *
 */
static void sunxi_hcd_h_tx_flush_fifo(struct sunxi_hcd_hw_ep *ep)
{
	void __iomem  *usbc_base = ep->regs;
	u16  csr     = 0;
	u16  lastcsr = 0;
	int  retries = 1000;

	/* if has checked that fifo not empty, then flush fifo */
	csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
	while (csr & (1 << USBC_BP_TXCSR_H_FIFO_NOT_EMPTY)) {
		if (csr != lastcsr)
			DMSG_PANIC("WRN: Host TX FIFONOTEMPTY csr: %02x\n",
				csr);

		lastcsr = csr;

		csr |= (1 << USBC_BP_TXCSR_H_FLUSH_FIFO);
		USBC_Writew(csr, USBC_REG_TXCSR(usbc_base));

		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		if (retries-- < 1) {
			DMSG_PANIC("ERR: Could not flush host TX%d fifo: csr: %04x\n",
				 ep->epnum, csr);
			return;
		}

		mdelay(1);
	}

	return;
}

/*
 * @hw_ep: physic ep info
 * @csr:   rx status reg value
 *
 */
static u16 sunxi_hcd_h_rx_flush_fifo(struct sunxi_hcd_hw_ep *hw_ep, u16 csr)
{
	void __iomem  *usbc_base = hw_ep->regs;

	/* we don't want fifo to fill itself again;
	 * ignore dma (various models),
	 * leave toggle alone (may not have been saved yet)
	 */
	csr |= (1 << USBC_BP_RXCSR_H_FLUSH_FIFO)
		| (1 << USBC_BP_RXCSR_H_RX_PKT_READY);
	csr &= ~(1 << USBC_BP_RXCSR_H_REQ_PACKET);
	csr &= ~(1 << USBC_BP_RXCSR_H_AUTO_REQ);
	csr &= ~(1 << USBC_BP_RXCSR_H_AUTO_CLEAR);

	/* write 2x to allow double buffering */
	USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));
	USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));

	/* flush writebuffer */
	return USBC_Readw(USBC_REG_RXCSR(usbc_base));
}
#else
/*
 * @ep: physic ep info
 */
static void sunxi_hcd_h_ep0_flush_fifo(struct sunxi_hcd_hw_ep *ep)
{
	void __iomem *usbc_base = NULL;
	u16 csr = 0;
	int retries = 5;
	__u8 old_ep_index = 0;

	if (ep == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return;
	}

	usbc_base = ep->regs;

	old_ep_index = USBC_GetActiveEp(
				ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_SelectActiveEp(
			ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
			ep->epnum);

	/* scrub any data left in the fifo */
	do {
		csr = USBC_Readw(USBC_REG_CSR0(usbc_base));
		if (!(csr & ((1 << USBC_BP_CSR0_H_TxPkRdy)
				| (1 << USBC_BP_CSR0_H_RxPkRdy))))
			break;

		USBC_Host_FlushFifo(
				ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
				USBC_EP_TYPE_EP0);

		csr = USBC_Readw(USBC_REG_CSR0(usbc_base));

		udelay(10);
	} while (--retries);

	if (retries == 0) {
		DMSG_PANIC("ERR: Could not flush host TX%d fifo: csr: %04x\n",
			ep->epnum, csr);
	}

	/* and reset for the next transfer */
	USBC_Writew(0x00, USBC_REG_CSR0(usbc_base));
	USBC_Host_DisablePing(ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);

	USBC_SelectActiveEp(
			ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
			old_ep_index);

	return;
}

/*
 * Clear TX fifo. Needed to avoid BABBLE errors.
 * @ep: physic ep info
 *
 */
static void sunxi_hcd_h_tx_flush_fifo(struct sunxi_hcd_hw_ep *ep)
{
	void __iomem  *usbc_base = ep->regs;
	u16  csr     = 0;
	u16  lastcsr = 0;
	int  retries = 1000;

	/* if has checked that fifo not empty, then flush fifo */
	csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
	while (csr & (1 << USBC_BP_TXCSR_H_FIFO_NOT_EMPTY)) {
		if (csr != lastcsr)
			DMSG_PANIC("WRN: Host TX FIFONOTEMPTY csr: %02x, %02x\n",
				csr, lastcsr);

		lastcsr = csr;

		csr |= (1 << USBC_BP_TXCSR_H_FLUSH_FIFO);
		csr &= ~(1 << USBC_BP_TXCSR_D_TX_READY);
		USBC_Writew(csr, USBC_REG_TXCSR(usbc_base));

		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		if (retries-- < 1) {
			DMSG_PANIC("ERR: Could not flush host TX%d fifo: csr: %04x\n",
				ep->epnum, csr);
			return;
		}

		mdelay(1);
	}

	return;
}

/*
 * @hw_ep: physic ep info
 * @csr:   rx status reg value
 *
 */
static u16 sunxi_hcd_h_rx_flush_fifo(struct sunxi_hcd_hw_ep *hw_ep, u16 csr)
{
	void __iomem  *usbc_base = hw_ep->regs;

	/* we don't want fifo to fill itself again;
	 * ignore dma (various models),
	 * leave toggle alone (may not have been saved yet)
	 */
	csr |= (1 << USBC_BP_RXCSR_H_FLUSH_FIFO);
	csr &= ~(1 << USBC_BP_RXCSR_H_REQ_PACKET);
	csr &= ~(1 << USBC_BP_RXCSR_H_AUTO_REQ);
	csr &= ~(1 << USBC_BP_RXCSR_H_AUTO_CLEAR);
	csr &= ~(1 << USBC_BP_RXCSR_H_RX_PKT_READY);

	/* write 2x to allow double buffering */
	USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));
	USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));

	/* flush writebuffer */
	return USBC_Readw(USBC_REG_RXCSR(usbc_base));
}
#endif

/*
 * Start transmit. Caller is responsible for locking shared resources.
 *   sunxi_hcd must be locked.
 * @ep: physic ep info
 *
 */
static inline void sunxi_hcd_h_tx_start(struct sunxi_hcd_hw_ep *ep)
{
	void __iomem	*usbc_base	= ep->regs;
	u16		txcsr		= 0;
	__u16		old_ep_index	= 0;

	old_ep_index = USBC_GetActiveEp(
			ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_SelectActiveEp(
			ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
			ep->epnum);

	/* NOTE: no locks here; caller should lock and select EP */
	if (ep->epnum) {
		txcsr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		txcsr |= (1 << USBC_BP_TXCSR_H_TX_READY)
			| USBC_TXCSR_H_WZC_BITS;
		USBC_Writew(txcsr, USBC_REG_TXCSR(usbc_base));
	} else {
		txcsr = (1 << USBC_BP_CSR0_H_SetupPkt)
			| (1 << USBC_BP_CSR0_H_TxPkRdy);
		USBC_Writew(txcsr, USBC_REG_CSR0(usbc_base));
	}

	USBC_SelectActiveEp(
			ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
			old_ep_index);

	return;
}

/*
 * clear ep's all dma status, used when dma exception
 * @qh: input.
 *
 */
static void sunxi_hcd_clean_ep_dma_status(struct sunxi_hcd_qh *qh)
{
	struct sunxi_hcd_hw_ep *hw_ep = NULL;
	u8	old_ep_index	= 0;
	__hdle	usb_bsp_hdle	= 0;
	__u32	is_in		= 0;

	hw_ep        = qh->hw_ep;
	usb_bsp_hdle = qh->hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle;
	is_in        = is_direction_in(qh);

	old_ep_index = USBC_GetActiveEp(usb_bsp_hdle);
	USBC_SelectActiveEp(usb_bsp_hdle, hw_ep->epnum);

	if (is_in) { /* ep in, rx*/
		/* clear ep dma status */
		USBC_Host_ClearEpDma(usb_bsp_hdle, USBC_EP_TYPE_RX);
		USBC_Host_ClearRqPktCount(usb_bsp_hdle, hw_ep->epnum);
		USBC_Host_StopInToken(usb_bsp_hdle, USBC_EP_TYPE_RX);
	} else {     /* ep out, tx*/
		/* clear ep dma status */
		USBC_Host_ClearEpDma(usb_bsp_hdle, USBC_EP_TYPE_TX);
	}

	USBC_SelectActiveEp(usb_bsp_hdle, old_ep_index);

	sunxi_hcd_switch_bus_to_pio(qh, is_in);
	return;
}

/*
 * clear ep's all dma status, used when dma exception
 * @qh: input.
 *
 * note: called with controller irqlocked
 */
static void sunxi_hcd_clean_ep_dma_status_and_flush_fifo(
		struct sunxi_hcd_qh *qh)
{
	struct sunxi_hcd_hw_ep   *hw_ep = NULL;
	u8	old_ep_index	= 0;
	__hdle	usb_bsp_hdle	= 0;
	__u32	is_in		= 0;
	__u16	csr		= 0;

	hw_ep        = qh->hw_ep;
	usb_bsp_hdle = qh->hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle;
	is_in        = is_direction_in(qh);

	old_ep_index = USBC_GetActiveEp(usb_bsp_hdle);
	USBC_SelectActiveEp(usb_bsp_hdle, hw_ep->epnum);

	if (is_in) { /* ep in, rx*/
		/* clear ep dma status */
		USBC_Host_ClearEpDma(usb_bsp_hdle, USBC_EP_TYPE_RX);
		USBC_Host_ClearRqPktCount(usb_bsp_hdle, hw_ep->epnum);
		USBC_Host_StopInToken(usb_bsp_hdle, USBC_EP_TYPE_RX);

		/* flush fifo */
		csr = USBC_Readw(USBC_REG_RXCSR(hw_ep->regs));
		sunxi_hcd_h_rx_flush_fifo(hw_ep, csr);
	} else {    /* ep out, tx*/
		/* clear ep dma status */
		USBC_Host_ClearEpDma(usb_bsp_hdle, USBC_EP_TYPE_TX);

		/* flush fifo */
		csr = USBC_Readw(USBC_REG_TXCSR(hw_ep->regs));
		csr |= USBC_TXCSR_H_WZC_BITS;
		USBC_Writew(csr, USBC_REG_TXCSR(hw_ep->regs));

		sunxi_hcd_h_tx_flush_fifo(hw_ep);
	}

	USBC_SelectActiveEp(usb_bsp_hdle, old_ep_index);

	/* Sunxi_hcd_switch_bus_to_pio(qh, is_in); */
	return;
}

static inline void sunxi_hcd_h_tx_dma_start(
		struct sunxi_hcd_hw_ep *ep,
		struct sunxi_hcd_qh *qh)
{
	void __iomem	*usbc_base	= ep->regs;
	u8		txcsr		= 0;
	u16		old_ep_index	= 0;

	old_ep_index = USBC_GetActiveEp(
			qh->hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	USBC_SelectActiveEp(
			qh->hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
			qh->hw_ep->epnum);

	/* clear timeout, stall error */
	txcsr = USBC_Readb(USBC_REG_TXCSR(usbc_base));
	txcsr |= USBC_TXCSR_H_WZC_BITS;
	USBC_Writeb(txcsr, USBC_REG_TXCSR(usbc_base));

	/* NOTE: no locks here; caller should lock and select EP */
	txcsr = USBC_Readb(USBC_REG_TXCSR(usbc_base) + 1);
	txcsr |= (1 << USBC_BP_TXCSR_H_DMA_REQ_EN) >> 8;
	USBC_Writeb(txcsr, (USBC_REG_TXCSR(usbc_base) + 1));

	DMSG_DBG_DMA("TXCSR = 0x%x\n", USBC_Readw(USBC_REG_TXCSR(usbc_base)));

	USBC_SelectActiveEp(
			qh->hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle,
			old_ep_index);
	return;
}

static bool sunxi_hcd_tx_dma_program(struct sunxi_hcd_hw_ep *hw_ep,
							struct sunxi_hcd_qh *qh,
							struct urb *urb,
							u32 offset,
							u32 length)
{
	void __iomem	*usbc_base	= NULL;
	u16		pkt_size	= 0;
	u8		ep_csr		= 0;
	u16		old_ep_index	= 0;
	__hdle          usb_bsp_hdle	= 0;

	/* check argment */
	if (hw_ep == NULL || qh == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invalid argment, hw_ep=0x%p, qh=0x%p, urb=0x%p\n",
			hw_ep, qh, urb);
		return false;
	}

	/* initialize parameter */
	usbc_base   = hw_ep->regs;
	pkt_size    = qh->maxpacket;
	qh->segsize = length;
	usb_bsp_hdle = qh->hw_ep->sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle;

	/* config tx_csr */
	old_ep_index = USBC_GetActiveEp(usb_bsp_hdle);
	USBC_SelectActiveEp(usb_bsp_hdle, hw_ep->epnum);

	/* auto_set, tx_mode, dma_tx_en, mode1 */
	ep_csr = USBC_Readb(USBC_REG_TXCSR(usbc_base) + 1);
	ep_csr |= (1 << USBC_BP_TXCSR_H_AUTOSET) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_H_MODE) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_H_DMA_REQ_MODE) >> 8;
	USBC_Writeb(ep_csr, (USBC_REG_TXCSR(usbc_base) + 1));

	USBC_SelectActiveEp(usb_bsp_hdle, old_ep_index);

	DMSG_DBG_DMA("line:%d %s transfer_buffer = 0x%x, transfer_dma = 0x%x len = %d\n",
		__LINE__, __func__, (u32)(urb->transfer_buffer + offset),
		(u32)(urb->transfer_dma + offset), length);

	/* config dma */
	sunxi_hcd_dma_set_config(qh,
			(dma_addr_t)urb->transfer_buffer + offset, length);
	sunxi_hcd_dma_start(qh,
			(u32 __force)hw_ep->fifo,
			(dma_addr_t)urb->transfer_buffer + offset,
			length);

	return true;
}

/*
 * Start the URB at the front of an endpoint's queue.
 *   end must be claimed from the caller.
 * @sw_hcd: usb controller
 * @is_in:  flag, if ep is IN.
 * @qh:     ep schedule info
 *
 */
static void sunxi_hcd_start_urb(
		struct sunxi_hcd *sunxi_hcd,
		int is_in, struct sunxi_hcd_qh *qh)
{
	u16                 frame       = 0;
	u32                 len         = 0;
	void __iomem        *usbc_base  = NULL;
	struct urb          *urb        = NULL;
	void                *buf        = NULL;
	u32                 offset      = 0;
	struct sunxi_hcd_hw_ep   *hw_ep      = NULL;
	unsigned            pipe        = 0;
	u8                  address     = 0;
	int                 epnum       = 0;

	/* check argment */
	if (sunxi_hcd == NULL || qh == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, qh=0x%p\n",
			sunxi_hcd, qh);
		return;
	}

	/* initalize parameter */
	usbc_base   = sunxi_hcd->mregs;
	urb         = next_urb(qh);
	if (urb == NULL) {
		DMSG_WRN("ERR: sunxi_hcd_start_urb, urb is NULL\n");
		return;
	}

	buf         = urb->transfer_buffer;
	hw_ep       = qh->hw_ep;
	pipe        = urb->pipe;
	address     = usb_pipedevice(pipe);
	epnum       = hw_ep->epnum;

	/* initialize software qh state */
	qh->offset  = 0;
	qh->segsize = 0;

	DMSG_DBG_HCD("start_urb: ep%d%s, qh(0x%p, 0x%x, 0x%x), urb(0x%p, 0x%p, %d, %d)\n",
		epnum, (is_in ? "rx" : "tx"),
		qh, qh->epnum, qh->type,
		urb, urb->transfer_buffer,
		urb->transfer_buffer_length,
		urb->actual_length);

	/* gather right source of data */
	switch (qh->type) {
	case USB_ENDPOINT_XFER_CONTROL:
		{
			/* control transfers always start with SETUP */
			is_in = 0;
			hw_ep->out_qh = qh;
			sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_START;
			buf = urb->setup_packet;
			len = 8;
		}
		break;

	case USB_ENDPOINT_XFER_ISOC:
		{
			qh->iso_idx = 0;
			qh->frame = 0;
			offset = urb->iso_frame_desc[0].offset;
			len = urb->iso_frame_desc[0].length;
		}
		break;

	default:  /* bulk, interrupt */
		/* actual_length may be nonzero on retry paths */
		buf = urb->transfer_buffer + urb->actual_length;
		len = urb->transfer_buffer_length - urb->actual_length;
	}

	DMSG_DBG_HCD("sunxi_hcd_start_urb: qh 0x%p urb %p dev%d ep%d%s%s, epnum %d, %p/%d\n",
		qh, urb, address, qh->epnum, is_in ? "in" : "out",
		({char *s; switch (qh->type) {
				case USB_ENDPOINT_XFER_CONTROL:
					s = "";
					break;
				case USB_ENDPOINT_XFER_BULK:
					s = "-bulk";
					break;
				case USB_ENDPOINT_XFER_ISOC:
					s = "-iso";
					break;
				default:
					s = "-intr";
					break;
			}; s; }),
		epnum, buf + offset, len);

	/* Configure endpoint */
	if (is_in || hw_ep->is_shared_fifo)
		hw_ep->in_qh = qh;
	else
		hw_ep->out_qh = qh;

	sunxi_hcd_ep_program(sunxi_hcd, epnum, urb, !is_in, buf, offset, len);

	/* transmit may have more work: start it when it is time */
	if (is_in) {
		DMSG_WRN("ERR: transmit may have more work: start it when it is time\n");
		return;
	}

	/* determine if the time is right for a periodic transfer */
	switch (qh->type) {
	case USB_ENDPOINT_XFER_ISOC:
	case USB_ENDPOINT_XFER_INT:
		{
			DMSG_DBG_HCD("check whether there's still time for periodic Tx\n");

			frame = USBC_Readw(USBC_REG_FRNUM(usbc_base));

			/* FIXME this doesn't implement that
			 * scheduling policy ...
			 * or handle framecounter wrapping
			 */
			if ((urb->transfer_flags & URB_ISO_ASAP)
				|| (frame >= urb->start_frame)) {
				/* REVISIT the SOF irq handler
				 * shouldn't duplicate this code;
				 * and we don't init urb->start_frame...
				 */
				qh->frame = 0;
				goto start;
			} else {
				qh->frame = urb->start_frame;

				/* enable SOF interrupt so we can count down */
				DMSG_DBG_HCD("SOF for %d\n", epnum);

				USBC_Writeb(0xff, USBC_REG_INTUSBE(usbc_base));
			}
		}
		break;

	default:
start:
		if (is_sunxi_hcd_dma_capable(
				sunxi_hcd->usbc_no, len,
				qh->maxpacket, epnum)) {
			DMSG_DBG_HCD("Start TX%d %s\n", epnum, "dma");

			sunxi_hcd_h_tx_dma_start(hw_ep, qh);
		} else {
			DMSG_DBG_HCD("Start TX%d %s\n", epnum, "pio");

			sunxi_hcd_h_tx_start(hw_ep);
		}
	}

	return;
}

/*
 * caller owns controller lock, irqs are blocked.
 * @sw_hcd: usb controller
 * @urb:    usb request block
 * @status: urb status
 *
 */
static void
__sunxi_hcd_giveback(struct sunxi_hcd *sunxi_hcd, struct urb *urb, int status)
__releases(sunxi_hcd->lock)
__acquires(sunxi_hcd->lock)
{
	if (sunxi_hcd == NULL || urb == NULL) {
		DMSG_PANIC("ERR: __sunxi_hcd_giveback, invalid argment, sunxi_hcd=0x%p, urb=0x%p\n",
		sunxi_hcd, urb);
		return;
	}

	DMSG_DBG_HCD("g: urb(0x%p, %pF, %d, %d, %d), dev(%d), ep%d%s\n",
		urb, urb->complete, status,
		urb->actual_length, urb->transfer_buffer_length,
		usb_pipedevice(urb->pipe),
		usb_pipeendpoint(urb->pipe),
		usb_pipein(urb->pipe) ? "in" : "out"
		);

	usb_hcd_unlink_urb_from_ep(sunxi_hcd_to_hcd(sunxi_hcd), urb);

	spin_unlock(&sunxi_hcd->lock);
	usb_hcd_giveback_urb(sunxi_hcd_to_hcd(sunxi_hcd), urb, status);
	spin_lock(&sunxi_hcd->lock);

	return;
}

/*
 * for bulk/interrupt endpoints only
 * @ep:    usb controller
 * @is_in: flag. if ep is IN.
 * @urb:   usb request block
 *
 */
static inline void sunxi_hcd_save_toggle(
		struct sunxi_hcd_hw_ep *ep,
		int is_in, struct urb *urb)
{
	struct usb_device	*udev       = NULL;
	u16			csr         = 0;
	void __iomem		*usbc_base  = NULL;
	struct sunxi_hcd_qh	*qh         = NULL;

	/* check argment */
	if (ep == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invaild argment\n");
		return;
	}
	if (urb->dev == NULL) {
		DMSG_PANIC("ERR: invalid argment, urb->dev=0x%p\n", urb->dev);
		return;
	}

	/* initialize parameter */
	udev = urb->dev;
	usbc_base = ep->regs;

	/* FIXME:  the current Mentor DMA code seems to have
	 * problems getting toggle correct.
	 */
	if (is_in || ep->is_shared_fifo)
		qh = ep->in_qh;
	else
		qh = ep->out_qh;

	if (!is_in) {
		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		usb_settoggle(udev, qh->epnum, 1,
			((csr & (1 << USBC_BP_TXCSR_H_DATA_TOGGLE)) ? 1 : 0));
	} else {
		csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));
		usb_settoggle(udev, qh->epnum, 0,
			((csr & (1 << USBC_BP_RXCSR_H_DATA_TOGGLE)) ? 1 : 0));
	}

	return;
}

/*
 * caller owns controller lock, irqs are blocked.
 * @qh:     ep schedule info
 * @urb:    usb request block
 * @status: urb status
 *
 */
static struct sunxi_hcd_qh *sunxi_hcd_giveback(
		struct sunxi_hcd_qh *qh,
		struct urb *urb, int status)
{
	struct sunxi_hcd_hw_ep	*ep     = NULL;
	struct sunxi_hcd	*sunxi_hcd = NULL;
	int			is_in   = 0;
	int			ready   = 0;

	/* check argment */
	if (qh == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invalid argment, qh=0x%p, urb=0x%p\n",
			qh, urb);
		return NULL;
	}

	DMSG_DBG_HCD("sunxi_hcd_giveback: qh(0x%p, 0x%x, 0x%x), urb(0x%p, %d, %d), status(0x%x, 0x%x)\n",
		qh, qh->epnum, qh->type,
		urb, urb->transfer_buffer_length, urb->actual_length,
		urb->status, status);

	/* initialize parameter */
	ep    = qh->hw_ep;
	sunxi_hcd  = ep->sunxi_hcd;
	is_in = usb_pipein(urb->pipe);
	ready = qh->is_ready;

	/* save toggle eagerly, for paranoia */
	switch (qh->type) {
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		sunxi_hcd_save_toggle(ep, is_in, urb);
		break;

	case USB_ENDPOINT_XFER_ISOC:
		if (status == 0 && urb->error_count)
			status = -EXDEV;
		break;

	default:
		DMSG_WRN("wrn: unkown ep type(%d)\n", qh->type);
		break;
	}

	qh->is_ready = 0;
	__sunxi_hcd_giveback(sunxi_hcd, urb, status);
	qh->is_ready = ready;

	/* reclaim resources (and bandwidth) ASAP; deschedule it, and
	 * invalidate qh as soon as list_empty(&hep->urb_list)
	 */
	if (list_empty(&qh->hep->urb_list)) {
		struct list_head *head = NULL;

		if (is_in)
			ep->rx_reinit = 1;
		else
			ep->tx_reinit = 1;

		/* clobber old pointers to this qh */
		if (is_in || ep->is_shared_fifo)
			ep->in_qh = NULL;
		else
			ep->out_qh = NULL;

		qh->hep->hcpriv = NULL;

		switch (qh->type) {
		case USB_ENDPOINT_XFER_CONTROL:
		case USB_ENDPOINT_XFER_BULK:
			/* fifo policy for these lists, except that NAKing
			 * should rotate a qh to the end (for fairness).
			 */
			if (qh->mux == 1) {
				head = qh->ring.prev;
				list_del(&qh->ring);
				kfree(qh);
				qh = first_qh(head);
			} else {
				kfree(qh);
				qh = NULL;
			}
			break;

		case USB_ENDPOINT_XFER_ISOC:
		case USB_ENDPOINT_XFER_INT:
			/* this is where periodic bandwidth should be
			 * de-allocated if it's tracked and allocated;
			 * and where we'd update the schedule tree...
			 */
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
			if (qh->type == USB_ENDPOINT_XFER_ISOC) {

				int old_ep_index_ex = 0;
				old_ep_index_ex = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
				sunxi_hcd_ep_select(sunxi_hcd->mregs,
						qh->hw_ep->epnum);

				USBC_Writel(0x00,
					USBC_REG_RXMAXP(sunxi_hcd->mregs));
				USBC_Writel(0x00,
					USBC_REG_EX_USB_EPATTR(sunxi_hcd->mregs));

				sunxi_hcd_ep_select(
					sunxi_hcd->mregs,
					old_ep_index_ex);

				urb->ep->hcpriv = NULL;
			}
#endif
			kfree(qh);
			qh = NULL;
			break;
		}
	}

	return qh;
}

/*
 * @sw_hcd: usb controller
 * @urb:    usb request block
 * @hw_ep:  ep physical info
 * @is_in:  flag. if ep is IN.
 *
 */
static void sunxi_hcd_advance_schedule(struct sunxi_hcd *sunxi_hcd,
		struct urb *urb,
		struct sunxi_hcd_hw_ep *hw_ep,
		int is_in)
{
	struct sunxi_hcd_qh *qh = NULL;

	/* check argment */
	if (sunxi_hcd == NULL || urb == NULL || hw_ep == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, urb=0x%p, hw_ep=0x%p\n",
			sunxi_hcd, urb, hw_ep);
		return;
	}

	if (is_in || hw_ep->is_shared_fifo)
		qh = hw_ep->in_qh;
	else
		qh = hw_ep->out_qh;

	if (urb->status == -EINPROGRESS)
		qh = sunxi_hcd_giveback(qh, urb, 0);
	else
		qh = sunxi_hcd_giveback(qh, urb, urb->status);

	if (!is_host_active(sunxi_hcd) || !sunxi_hcd->is_active) {
		DMSG_PANIC("sunxi_hcd_advance_schedule, host is not active\n");
		return;
	}

	if (qh != NULL) {
		if (qh->is_ready) {
			DMSG_DBG_HCD("... next ep%d %cX urb %p\n",
				hw_ep->epnum,
				(is_in ? 'R' : 'T'),
				next_urb(qh));
			sunxi_hcd_start_urb(sunxi_hcd, is_in, qh);
		} else {
			DMSG_WRN("WRN: qh is not ready\n");
		}
	}

	return;
}

/*
 * @sw_hcd: usb controller
 * @urb:    usb request block
 * @epnum:  ep index
 * @iso_err: flag. is iso error?
 *
 */
static bool sunxi_hcd_host_packet_rx(
		struct sunxi_hcd *sunxi_hcd,
		struct urb *urb, u8 epnum, u8 iso_err)
{
	u16			rx_count    = 0;
	u8			*buf        = NULL;
	u16			csr         = 0;
	bool			done        = false;
	u32			length      = 0;
	int			do_flush    = 0;
	struct sunxi_hcd_hw_ep	*hw_ep      = NULL;
	void __iomem		*usbc_base  = NULL;
	struct sunxi_hcd_qh	*qh         = NULL;
	int			pipe        = 0;
	void			*buffer     = NULL;

	/* check argment */
	if (sunxi_hcd == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, urb=0x%p\n",
			sunxi_hcd, urb);
		return false;
	}

	/* initialize parameter */
	done        = false;
	hw_ep       = sunxi_hcd->endpoints + epnum;
	usbc_base   = hw_ep->regs;
	qh          = hw_ep->in_qh;
	pipe        = urb->pipe;
	buffer      = urb->transfer_buffer;

	/* sunxi_hcd_ep_select(usbc_base, epnum); */
	rx_count = USBC_Readw(USBC_REG_RXCOUNT(usbc_base));

	DMSG_DBG_HCD("RX%d count %d, buffer %p len %d/%d\n", epnum, rx_count,
		urb->transfer_buffer, qh->offset,
		urb->transfer_buffer_length);

	/* unload FIFO */
	if (usb_pipeisoc(pipe)) {
		int status = 0;
		struct usb_iso_packet_descriptor *d = NULL;

		if (iso_err) {
			status = -EILSEQ;
			urb->error_count++;
		}

		d = urb->iso_frame_desc + qh->iso_idx;
		buf = buffer + d->offset;
		length = d->length;
		if (rx_count > length) {
			if (status == 0) {
				status = -EOVERFLOW;
				urb->error_count++;
			}

			DMSG_DBG_HCD("** OVERFLOW %d into %d\n",
				rx_count, length);

			do_flush = 1;
		} else {
			length = rx_count;
		}

		urb->actual_length += length;
		d->actual_length = length;
		d->status = status;

		/* see if we are done */
		done = (++qh->iso_idx >= urb->number_of_packets);
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
		if (DOBULE_EP) {
			int other_epnum = 0;
			int other_csr = 0;
			int old_ep_index = 0;


			if (hw_ep->epnum == BAND_WIDTH_ISO_EP)
				other_epnum = BAND_WIDTH_ISO_EP_EX;
			else if (hw_ep->epnum == BAND_WIDTH_ISO_EP_EX)
				other_epnum = BAND_WIDTH_ISO_EP;

			if ((hw_ep->epnum == BAND_WIDTH_ISO_EP)
					|| (hw_ep->epnum == BAND_WIDTH_ISO_EP_EX)) {
				old_ep_index = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
				sunxi_hcd_ep_select(usbc_base, other_epnum);

				other_csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));

				other_csr |= USBC_RXCSR_H_WZC_BITS;
				other_csr &= ~((1 << USBC_BP_RXCSR_H_RX_PKT_READY)
						| (1 << USBC_BP_RXCSR_H_REQ_PACKET));

				other_csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);

				USBC_Writew(other_csr, USBC_REG_RXCSR(usbc_base));

				sunxi_hcd_ep_select(usbc_base, old_ep_index);
			}
		}
#endif
	} else {
		/* non-isoch */
		buf = buffer + qh->offset;
		length = urb->transfer_buffer_length - qh->offset;
		if (rx_count > length) {
			if (urb->status == -EINPROGRESS)
				urb->status = -EOVERFLOW;

			DMSG_DBG_HCD("** OVERFLOW %d into %d\n",
				rx_count, length);

			do_flush = 1;
		} else{
			length = rx_count;
		}

		urb->actual_length += length;
		qh->offset += length;

		/* see if we are done */
		done = (urb->actual_length == urb->transfer_buffer_length)
			|| (rx_count < qh->maxpacket)
			|| (urb->status != -EINPROGRESS);
		if (done && (urb->status == -EINPROGRESS)
			&& (urb->transfer_flags & URB_SHORT_NOT_OK)
			&& (urb->actual_length
			< urb->transfer_buffer_length)) {
			urb->status = -EREMOTEIO;
		}
	}

	sunxi_hcd_read_fifo(hw_ep, length, buf);

	csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));
	csr |= USBC_RXCSR_H_WZC_BITS;
	if (unlikely(do_flush)) {
		sunxi_hcd_h_rx_flush_fifo(hw_ep, csr);
	} else {
		/* REVISIT this assumes AUTOCLEAR is never set */
		csr &= ~((1 << USBC_BP_RXCSR_H_RX_PKT_READY)
			| (1 << USBC_BP_RXCSR_H_REQ_PACKET));

		if (!done) {
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
			if (qh->type == USB_ENDPOINT_XFER_ISOC && DOBULE_EP) {
				if ((hw_ep->epnum != BAND_WIDTH_ISO_EP)
						&& (hw_ep->epnum != BAND_WIDTH_ISO_EP_EX)) {
					csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);
				}
			} else {
				csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);
			}
#else

			csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);
#endif
		}

		USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));

	}

	return done;
}

/*
 * @sw_hcd: usb controller
 * @qh:     ep schedule info
 * @ep:     ep physical info
 *
 * note:
 *    we don't always need to reinit a given side of an endpoint...
 * when we do, use tx/rx reinit routine and then construct a new CSR
 * to address data toggle, NYET, and DMA or PIO.
 *
 *    it's possible that driver bugs (especially for DMA) or aborting a
 * transfer might have left the endpoint busier than it should be.
 * the busy/not-empty tests are basically paranoia.
 */
static void sunxi_hcd_rx_reinit(
		struct sunxi_hcd *sunxi_hcd,
		struct sunxi_hcd_qh *qh,
		struct sunxi_hcd_hw_ep *ep)
{
	u16	csr = 0;
	void __iomem *usbc_base  = NULL;

	/* check argment */
	if (sunxi_hcd == NULL || qh == NULL || ep == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, qh=0x%p, ep=0x%p\n",
			sunxi_hcd, qh, ep);
		return;
	}

	/* initialize parameter */
	usbc_base = ep->regs;

	/* NOTE:  we know the "rx" fifo reinit never triggers for ep0.
	 * That always uses tx_reinit since ep0 repurposes TX register
	 * offsets; the initial SETUP packet is also a kind of OUT.
	 */

	/* if programmed for Tx, put it in RX mode */
	if (ep->is_shared_fifo) {
		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		if (csr & (1 << USBC_BP_TXCSR_H_MODE)) {
			sunxi_hcd_h_tx_flush_fifo(ep);

			csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
			csr |= (1 << USBC_BP_TXCSR_H_FORCE_DATA_TOGGLE);
			USBC_Writew(csr, USBC_REG_TXCSR(usbc_base));
		}

		/*
		 * Clear the MODE bit (and everything else) to enable Rx.
		 * NOTE: we mustn't clear the DMAMODE bit before DMAENAB.
		 */
		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		if (csr & (1 << USBC_BP_TXCSR_H_DMA_REQ_MODE))
			USBC_Writew(USBC_BP_TXCSR_H_DMA_REQ_MODE,
				USBC_REG_TXCSR(usbc_base));

		USBC_Writew(0, USBC_REG_TXCSR(usbc_base));

		/* scrub all previous state, clearing toggle */
	} else {
		csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));
		if (csr & (1 << USBC_BP_RXCSR_H_RX_PKT_READY))
			DMSG_PANIC("WRN: rx%d, packet/%d ready?\n",
				ep->epnum,
				USBC_Readw(USBC_REG_RXCOUNT(usbc_base)));

		sunxi_hcd_h_rx_flush_fifo(ep,
			(1 << USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE));
	}

	/* target addr and (for multipoint) hub addr/port */
	if (sunxi_hcd->is_multipoint) {
		sunxi_hcd_write_rxfunaddr(ep->target_regs, qh->addr_reg);
		sunxi_hcd_write_rxhubaddr(ep->target_regs, qh->h_addr_reg);
		sunxi_hcd_write_rxhubport(ep->target_regs, qh->h_port_reg);
	} else{
		USBC_Writeb(qh->addr_reg, USBC_REG_FADDR(usbc_base));
	}

	/* protocol/endpoint, interval/NAKlimit, i/o size */
	USBC_Writeb(qh->type_reg, USBC_REG_RXTYPE(usbc_base));
	USBC_Writeb(qh->intv_reg, USBC_REG_RXINTERVAL(usbc_base));


	/* NOTE: bulk combining rewrites high bits of maxpacket */
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
	USBC_Writew((qh->maxpacket|((qh->PacketCount)<<11)),
			USBC_REG_RXMAXP(usbc_base));
	if (qh->type == USB_ENDPOINT_XFER_ISOC) {
		if ((qh->hw_ep->epnum == BAND_WIDTH_ISO_EP) && DOBULE_EP)
			is_first_iso = 1;
	}
#else
	USBC_Writew(qh->maxpacket, USBC_REG_RXMAXP(usbc_base));
#endif
	ep->rx_reinit = 0;

	return;
}

/*
 * Program an HDRC endpoint as per the given URB
 *
 * Context: irqs blocked, controller lock held
 */
static void sunxi_hcd_ep_program(struct sunxi_hcd *sunxi_hcd,
						u8 epnum,
						struct urb *urb,
						int is_out,
						u8 *buf,
						u32 offset,
						u32 len)
{
	void __iomem            *usbc_base      = NULL;
	struct sunxi_hcd_hw_ep  *hw_ep          = NULL;
	struct sunxi_hcd_qh     *qh             = NULL;
	u16                     packet_sz       = 0;
	u16			old_ep_index    = 0;

	/* check argment */
	if (sunxi_hcd == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, urb=0x%p\n",
			sunxi_hcd, urb);
		return;
	}

	/* initialize parameter */
	usbc_base = sunxi_hcd->mregs;
	hw_ep = sunxi_hcd->endpoints + epnum;

	if (!is_out || hw_ep->is_shared_fifo)
		qh = hw_ep->in_qh;
	else
		qh = hw_ep->out_qh;

	packet_sz = qh->maxpacket;

	DMSG_DBG_HCD("sunxi_hcd_ep_program: %s hw%d urb %p spd%d dev%d ep%d%s h_addr%02x h_port%02x bytes %d\n",
		is_out ? "-->" : "<--",
		epnum, urb, urb->dev->speed,
		qh->addr_reg, qh->epnum, is_out ? "out" : "in",
		qh->h_addr_reg, qh->h_port_reg,
		len);

	old_ep_index = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	sunxi_hcd_ep_select(usbc_base, epnum);

	/* make sure we clear DMAEnab, autoSet bits from previous run */

	/* OUT/transmit/EP0 or IN/receive? */
	if (is_out) { /* tx */
		u16	csr = 0;
		u16	int_txe = 0;
		u16	load_count = 0;

		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));

		/* disable interrupt in case we flush */
		int_txe = USBC_Readw(USBC_REG_INTTxE(usbc_base));
		USBC_Writew(int_txe & ~(1 << epnum),
				USBC_REG_INTTxE(usbc_base));

		/* general endpoint setup */
		if (epnum) {
			/* flush all old state, set default */
			sunxi_hcd_h_tx_flush_fifo(hw_ep);

			/*
			 * We must not clear the DMAMODE bit before or in
			 * the same cycle with the DMAENAB bit, so we clear
			 * the latter first...
			 */
			csr &= ~((1 << USBC_BP_TXCSR_H_NAK_TIMEOUT)
				| (1 << USBC_BP_TXCSR_H_AUTOSET)
				| (1 << USBC_BP_TXCSR_H_DMA_REQ_EN)
				| (1 << USBC_BP_TXCSR_H_FORCE_DATA_TOGGLE)
				| (1 << USBC_BP_TXCSR_H_TX_STALL)
				| (1 << USBC_BP_TXCSR_H_ERROR)
				| (1 << USBC_BP_TXCSR_H_TX_READY)
				);
			csr |= (1 << USBC_BP_TXCSR_H_MODE);

			if (usb_gettoggle(urb->dev, qh->epnum, 1)) {
				csr |= (1 << USBC_BP_TXCSR_H_DATA_TOGGLE_WR_EN)
					| (1 << USBC_BP_TXCSR_H_DATA_TOGGLE);
			} else {
				csr |= (1 << USBC_BP_TXCSR_H_CLEAR_DATA_TOGGLE);
			}

			USBC_Writew(csr, USBC_REG_TXCSR(usbc_base));

			/* REVISIT may need to clear FLUSHFIFO ... */
			csr &= ~(1 << USBC_BP_TXCSR_H_DMA_REQ_MODE);
			USBC_Writew(csr, USBC_REG_TXCSR(usbc_base));

			csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		} else {
			/* endpoint 0: just flush */
			sunxi_hcd_h_ep0_flush_fifo(hw_ep);
		}

		/* target addr and (for multipoint) hub addr/port */
		if (sunxi_hcd->is_multipoint) {
			sunxi_hcd_write_txfunaddr(usbc_base,
					epnum, qh->addr_reg);
			sunxi_hcd_write_txhubaddr(usbc_base,
					epnum, qh->h_addr_reg);
			sunxi_hcd_write_txhubport(usbc_base,
					epnum, qh->h_port_reg);

			/* FIXME if !epnum, do the same for RX ... */
		} else {
			USBC_Writeb(qh->addr_reg, USBC_REG_FADDR(usbc_base));
		}

		/* protocol/endpoint/interval/NAKlimit */
		if (epnum) {
			USBC_Writeb(qh->type_reg, USBC_REG_TXTYPE(usbc_base));

			if (can_bulk_split(sunxi_hcd, qh->type)) {
				u32 tx_pkt_size = packet_sz
					| (((hw_ep->max_packet_sz_tx / packet_sz) - 1) << 11);

				USBC_Writew(tx_pkt_size,
					USBC_REG_TXMAXP(usbc_base));
			} else {
				USBC_Writew(packet_sz,
					USBC_REG_TXMAXP(usbc_base));
			}

			USBC_Writeb(qh->intv_reg,
				USBC_REG_TXINTERVAL(usbc_base));
		} else {
			USBC_Writeb(qh->intv_reg,
				USBC_REG_NAKLIMIT0(usbc_base));

			if (sunxi_hcd->is_multipoint) {
				USBC_Writeb(qh->type_reg,
					USBC_REG_EP0TYPE(usbc_base));
			}
		}

		if (can_bulk_split(sunxi_hcd, qh->type))
			load_count = min_t(u32, hw_ep->max_packet_sz_tx, len);
		else
			load_count = min_t(u32, packet_sz, len);

		/* dma transmit */
		if (is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no,
				len, packet_sz, epnum)) {
			if (sunxi_hcd_tx_dma_program(
					hw_ep, qh, urb, offset, len))
				load_count = 0;
		}

		if (load_count) {
			/* PIO to load FIFO */
			qh->segsize = load_count;
			sunxi_hcd_write_fifo(hw_ep, load_count, buf);
		}

		/* re-enable interrupt */
		USBC_Writew(int_txe, USBC_REG_INTTxE(usbc_base));

	} else {  /* IN/receive */    /* rx */
		u16	csr = 0;

		if (hw_ep->rx_reinit) {
			sunxi_hcd_rx_reinit(sunxi_hcd, qh, hw_ep);
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
			if (qh->type == USB_ENDPOINT_XFER_ISOC && DOBULE_EP) {
				int rxcsr = USBC_Readl(USBC_REG_RXMAXP(usbc_base));
				int epattr = USBC_Readl(USBC_REG_EX_USB_EPATTR(usbc_base));
				int fifo_add = USBC_Readl(USBC_REG_RXFIFOSZ(usbc_base));
				int ep_add = USBC_Readl(USBC_REG_RXFADDRx_Ex(usbc_base));
				int other_epnum = 0;
				int old_ep_index_ex =  0;
				int reg_val = 0;

				if (hw_ep->epnum == BAND_WIDTH_ISO_EP)
					other_epnum = BAND_WIDTH_ISO_EP_EX;
				else if (hw_ep->epnum == BAND_WIDTH_ISO_EP_EX)
					other_epnum = BAND_WIDTH_ISO_EP;

				if ((hw_ep->epnum == BAND_WIDTH_ISO_EP)
						|| (hw_ep->epnum == BAND_WIDTH_ISO_EP_EX)) {

					memcpy((struct sunxi_hcd_hw_ep *)(sunxi_hcd->endpoints + BAND_WIDTH_ISO_EP_EX), (struct sunxi_hcd_hw_ep *)(sunxi_hcd->endpoints + BAND_WIDTH_ISO_EP), sizeof(struct sunxi_hcd_hw_ep));

					((struct sunxi_hcd_hw_ep *)(sunxi_hcd->endpoints + BAND_WIDTH_ISO_EP_EX))->fifo = (((struct sunxi_hcd_hw_ep *)(sunxi_hcd->endpoints + BAND_WIDTH_ISO_EP_EX))->fifo  + 4 * (BAND_WIDTH_ISO_EP_EX - BAND_WIDTH_ISO_EP));

					((struct sunxi_hcd_hw_ep *)(sunxi_hcd->endpoints + BAND_WIDTH_ISO_EP_EX))->epnum = BAND_WIDTH_ISO_EP_EX;

					old_ep_index_ex = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
					sunxi_hcd_ep_select(usbc_base, other_epnum);

					USBC_Writel(rxcsr, USBC_REG_RXMAXP(usbc_base));
					USBC_Writel(epattr, USBC_REG_EX_USB_EPATTR(usbc_base));
					USBC_Writel(fifo_add, USBC_REG_RXFIFOSZ(usbc_base));
					USBC_Writel(ep_add, USBC_REG_RXFADDRx_Ex(usbc_base));

					reg_val = USBC_Readl(USBC_REG_RXFIFOSZ(usbc_base));
					reg_val &= ~(0x1fff << 16);
					reg_val |= (0x280 << 16);
					USBC_Writel(reg_val, USBC_REG_RXFIFOSZ(usbc_base));

					sunxi_hcd_ep_select(usbc_base, old_ep_index_ex);
				}
			}
#endif

			/* init new state: toggle and NYET, maybe DMA later */
			if (usb_gettoggle(urb->dev, qh->epnum, 0)) {
				csr = (1 << USBC_BP_RXCSR_H_DATA_TOGGLE_WR_EN)
					| (1 << USBC_BP_RXCSR_H_DATA_TOGGLE);
			} else {
				csr = 0;
			}

			if (qh->type == USB_ENDPOINT_XFER_INT)
				csr |= (1 << USBC_BP_RXCSR_H_DISNYET);
		} else {
			csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));

			/* scrub any stale state, leaving toggle alone */
			csr &= (1 << USBC_BP_RXCSR_H_DISNYET);
		}

		/* kick things off */
		if (is_sunxi_hcd_dma_capable(
				sunxi_hcd->usbc_no, len, packet_sz, epnum)) {
			qh->segsize = len;

			/* AUTOREQ is in a DMA register */
			USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));

			csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));

			csr |= (1 << USBC_BP_RXCSR_H_AUTO_CLEAR);
			csr |= (1 << USBC_BP_RXCSR_H_AUTO_REQ);
			csr |= (1 << USBC_BP_RXCSR_H_DMA_REQ_MODE);

			USBC_Host_ConfigRqPktCount(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, qh->hw_ep->epnum, (len / qh->maxpacket));

			DMSG_DBG_DMA("line:%d %s transfer_buffer = 0x%x,transfer_dma = 0x%x, len= %d\n", __LINE__, __func__, (u32)(urb->transfer_buffer + offset), (u32)(urb->transfer_dma + offset), len);

			/* config dma */
			sunxi_hcd_dma_set_config(qh, (dma_addr_t)(urb->transfer_buffer + offset), len);
			sunxi_hcd_dma_start(qh, (u32 __force)hw_ep->fifo, (dma_addr_t)(urb->transfer_buffer + offset), len);

			csr |= (1 << USBC_BP_RXCSR_H_DMA_REQ_EN);
		}
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
		if (qh->type == USB_ENDPOINT_XFER_ISOC && DOBULE_EP) {

			if ((hw_ep->epnum  == BAND_WIDTH_ISO_EP) && is_first_iso) {
				is_first_iso  = 0;
				 csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);
				 USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));
			}

			if ((hw_ep->epnum != BAND_WIDTH_ISO_EP) && (hw_ep->epnum != BAND_WIDTH_ISO_EP_EX)) {
				csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);
				USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));
			}
		} else {

			csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);
			USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));

		}
#else
		csr |= (1 << USBC_BP_RXCSR_H_REQ_PACKET);

		USBC_Writew(csr, USBC_REG_RXCSR(usbc_base));
		csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));
#endif
		DMSG_DBG_DMA("line:%d %s 1RXCSR%d := %04x\n",
			__LINE__, __func__, epnum, csr);
	}

	sunxi_hcd_ep_select(usbc_base, old_ep_index);
	return;
}

/*
 * Service the default endpoint (ep0) as host.
 *
 * Return true until it's time to start the status stage.
 */
static bool sunxi_hcd_h_ep0_continue(
		struct sunxi_hcd *sunxi_hcd,
		u16 len, struct urb *urb)
{
	bool                    more        = false;
	u8                      *fifo_dest  = NULL;
	u16                     fifo_count  = 0;
	struct sunxi_hcd_hw_ep  *hw_ep      = NULL;
	struct sunxi_hcd_qh     *qh         = NULL;
	struct usb_ctrlrequest  *request    = NULL;

	/* check argment */
	if (sunxi_hcd == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, urb=0x%p\n",
			sunxi_hcd, urb);
		return false;
	}

	/* initialize parameter */
	hw_ep = sunxi_hcd->control_ep;
	qh = hw_ep->in_qh;

	switch (sunxi_hcd->ep0_stage) {
	case SUNXI_HCD_EP0_IN:
		{
			fifo_dest  = urb->transfer_buffer + urb->actual_length;
			fifo_count = min_t(size_t, len, urb->transfer_buffer_length - urb->actual_length);
			if (fifo_count < len)
				urb->status = -EOVERFLOW;

			sunxi_hcd_read_fifo(hw_ep, fifo_count, fifo_dest);

			urb->actual_length += fifo_count;
			if (len < qh->maxpacket) {
				/* always terminate on short read; it's
				 * rarely reported as an error.
				 */
				USBC_Host_ReadDataStatus(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_EP0, 1);
			} else if (urb->actual_length < urb->transfer_buffer_length) {
				more = true;
				USBC_Host_ReadDataStatus(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_EP0, 0);
			}
		}
		break;

	case SUNXI_HCD_EP0_START:
		request = (struct usb_ctrlrequest *) urb->setup_packet;

		if (!request->wLength) {
			DMSG_DBG_HCD("start no-DATA\n");

			break;
		} else if (request->bRequestType & USB_DIR_IN) {
			DMSG_DBG_HCD("start IN-DATA\n");

			sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_IN;
			more = true;
			break;
		} else {
			DMSG_DBG_HCD("start OUT-DATA\n");

			sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_OUT;
			more = true;
		}
		/* FALLTHROUGH */
	case SUNXI_HCD_EP0_OUT:
		fifo_count = min_t(size_t, qh->maxpacket,
				urb->transfer_buffer_length - urb->actual_length);

		DMSG_DBG_HCD("Sending %d byte to ep0 fifo, urb(0x%p, %d, %d)\n",
			fifo_count,
			urb, urb->transfer_buffer_length,
			urb->actual_length);

		if (fifo_count) {
			fifo_dest = (u8 *)(urb->transfer_buffer + urb->actual_length);

			sunxi_hcd_write_fifo(hw_ep, fifo_count, fifo_dest);

			urb->actual_length += fifo_count;
			more = true;
			break;
		}
		break;

	default:
		DMSG_PANIC("ERR: bogus ep0 stage %d\n", sunxi_hcd->ep0_stage);
		break;
	}

	return more;
}

/*
 * Handle default endpoint interrupt as host. Only called in IRQ time
 *    from sunxi_hcd_interrupt().
 * @sw_hcd: usb controller
 *
 * note: called with controller irqlocked
 */
irqreturn_t sunxi_hcd_h_ep0_irq(struct sunxi_hcd *sunxi_hcd)
{
	struct urb          *urb        = NULL;
	u16                 csr         = 0;
	u16                 len         = 0;
	int                 status      = 0;
	void __iomem        *usbc_base  = NULL;
	struct sunxi_hcd_hw_ep	*hw_ep	= NULL;
	struct sunxi_hcd_qh	*qh	= NULL;
	bool                complete    = false;
	irqreturn_t         retval      = IRQ_NONE;

	/* check argment */
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return IRQ_HANDLED;
	}

	/* initialize parameter */
	usbc_base   = sunxi_hcd->mregs;
	hw_ep       = sunxi_hcd->control_ep;
	qh          = hw_ep->in_qh;

	DMSG_DBG_HCD("sunxi_hcd_h_ep0_irq:qh = 0x%p, in_qh = 0x%p, out_qh = 0x%p\n", qh, hw_ep->in_qh, hw_ep->out_qh);

	/* ep0 only has one queue, "in" */
	urb = next_urb(qh);
	if (urb == NULL)
		return IRQ_HANDLED;

	sunxi_hcd_ep_select(usbc_base, 0);
	csr = USBC_Readw(USBC_REG_CSR0(usbc_base));

	if (csr & (1 << USBC_BP_CSR0_H_RxPkRdy))
		len = USBC_Readw(USBC_REG_COUNT0(usbc_base));
	else
		len = 0;

	DMSG_DBG_HCD("sunxi_hcd_h_ep0_irq: csr0 %04x, qh %p, count %d, urb(0x%p, %d, %d), stage %d\n",
		csr, qh, len, urb, urb->transfer_buffer_length, urb->actual_length, sunxi_hcd->ep0_stage);

	/* if we just did status stage, we are done */
	if (SUNXI_HCD_EP0_STATUS == sunxi_hcd->ep0_stage) {
		retval = IRQ_HANDLED;
		complete = true;
	}

	/* prepare status */
	if (csr & (1 << USBC_BP_CSR0_H_RxStall)) {
		USBC_Host_ClearEpStall(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_EP0);

		status = -EPIPE;
	} else if (csr & (1 << USBC_BP_CSR0_H_Error)) {
		DMSG_PANIC("ERR: sunxi_hcd_h_ep0_irq, no response, csr0 %04x\n", csr);
		USBC_Host_ClearEpError(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_EP0);

		status = -EPROTO;
	} else if (csr & (1 << USBC_BP_CSR0_H_NAK_Timeout)) {
		DMSG_PANIC("ERR: sunxi_hcd_h_ep0_irq, control NAK timeout\n");

		USBC_Writew(0x00, USBC_REG_CSR0(usbc_base));

		USBC_Host_DisablePing(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
		USBC_Writeb(0x00, USBC_REG_NAKLIMIT0(usbc_base));

		retval = IRQ_HANDLED;
	}

	if (status) {
		retval = IRQ_HANDLED;
		if (urb)
			urb->status = status;

		complete = true;

		/* use the proper sequence to abort the transfer */
		if (csr & (1 << USBC_BP_CSR0_H_ReqPkt)) {
			csr &= ~(1 << USBC_BP_CSR0_H_ReqPkt);

			USBC_Writew(csr, USBC_REG_CSR0(usbc_base));

			csr &= ~(1 << USBC_BP_CSR0_H_NAK_Timeout);
			USBC_Writew(csr, USBC_REG_CSR0(usbc_base));
			USBC_Host_DisablePing(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);

		} else {
			sunxi_hcd_h_ep0_flush_fifo(hw_ep);
		}

		USBC_Writeb(0x00, USBC_REG_NAKLIMIT0(usbc_base));

		/* clear it */
		USBC_Writew(0x00, USBC_REG_CSR0(usbc_base));
		USBC_Host_DisablePing(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	}

	if (unlikely(!urb)) {
		/* stop endpoint since we have no place for its data, this
		 * SHOULD NEVER HAPPEN! */
		DMSG_PANIC("ERR: sunxi_hcd_h_ep0_irq, no URB for ep 0\n");

		sunxi_hcd_h_ep0_flush_fifo(hw_ep);

		goto done;
	}

	if (!complete) {
		/* call common logic and prepare response */
		if (sunxi_hcd_h_ep0_continue(sunxi_hcd, len, urb)) {
			/* more packets required */
			csr = (SUNXI_HCD_EP0_IN == sunxi_hcd->ep0_stage)
				?  (1 << USBC_BP_CSR0_H_ReqPkt) : (1 << USBC_BP_CSR0_H_TxPkRdy);
		} else {
			/* data transfer complete; perform status phase */
			if (usb_pipeout(urb->pipe) || !urb->transfer_buffer_length) {
				csr = (1 << USBC_BP_CSR0_H_StatusPkt)
					| (1 << USBC_BP_CSR0_H_ReqPkt);
			} else {
				csr = (1 << USBC_BP_CSR0_H_StatusPkt)
					| (1 << USBC_BP_CSR0_H_TxPkRdy);
			}

			/* flag status stage */
			sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_STATUS;
		}

		USBC_Writew(csr, USBC_REG_CSR0(usbc_base));
		USBC_Host_DisablePing(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);

		retval = IRQ_HANDLED;
	} else {
		sunxi_hcd->ep0_stage = SUNXI_HCD_EP0_IDLE;
		USBC_Writew(0x00, USBC_REG_CSR0(usbc_base));
		USBC_Host_DisablePing(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
	}

	/* call completion handler if done */
	if (complete)
		sunxi_hcd_advance_schedule(sunxi_hcd, urb, hw_ep, 1);

done:
	return retval;
}
EXPORT_SYMBOL(sunxi_hcd_h_ep0_irq);

/*
 * Service a Tx-Available or dma completion irq for the endpoint
 * @sw_hcd: usb controller
 * @epnum:  ep index
 *
 * note:
 *   Host side TX (OUT) using Mentor DMA works as follows:
 *	submit_urb ->
 *		- if queue was empty, Program Endpoint
 *		- ... which starts DMA to fifo in mode 1 or 0
 *
 *   DMA Isr (transfer complete) -> TxAvail()
 *		- Stop DMA (~DmaEnab)	(<--- Alert ... currently happens
 *					only in sunxi_hcd_cleanup_urb)
 *		- TxPktRdy has to be set in mode 0 or for
 *			short packets in mode 1.
 */
void sunxi_hcd_host_tx(struct sunxi_hcd *sunxi_hcd, u8 epnum)
{
	int                 pipe        = 0;
	bool                done        = false;
	u16                 tx_csr      = 0;
	size_t              length      = 0;
	size_t              offset      = 0;
	struct urb          *urb        = NULL;
	struct sunxi_hcd_hw_ep	*hw_ep      = NULL;
	struct sunxi_hcd_qh	*qh         = NULL;
	u32			status      = 0;
	void __iomem		*usbc_base  = NULL;

	/* check argment */
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return;
	}

	/* initialize parameter */
	hw_ep = sunxi_hcd->endpoints + epnum;

	if (hw_ep == NULL)
		return;

	qh = hw_ep->is_shared_fifo ? hw_ep->in_qh : hw_ep->out_qh;
	usbc_base = sunxi_hcd->mregs;

	if (hw_ep == NULL || qh == NULL) {
		DMSG_PANIC("ERR: invalid hw_ep or qh\n");
		return;
	}
	urb = next_urb(qh);
	if (urb == NULL) {
		DMSG_PANIC("ERR: sunxi_hcd_host_tx, urb is NULL\n");
		return;
	}

	sunxi_hcd_ep_select(usbc_base, epnum);

	tx_csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));

	/* with CPPI, DMA sometimes triggers "extra" irqs */
	if (!urb) {
		DMSG_PANIC("ERR: sunxi_hcd_host_tx, extra TX%d ready, csr %04x\n", epnum, tx_csr);
		return;
	}

	pipe = urb->pipe;

	DMSG_DBG_HCD("tx: ep(0x%p, %d, 0x%x), qh(0x%p, 0x%x, 0x%x), urb(0x%p, 0x%p, %d, %d)\n",
		hw_ep, hw_ep->epnum, USBC_Readw(USBC_REG_TXCSR(usbc_base)),
		qh, qh->epnum, qh->type,
		urb, urb->transfer_buffer, urb->transfer_buffer_length, urb->actual_length);

	/* check for errors */
	if (tx_csr & (1 << USBC_BP_TXCSR_H_TX_STALL)) {
		/* dma was disabled, fifo flushed */
		DMSG_PANIC("ERR: sunxi_hcd_host_tx, TX end %d stall\n", epnum);

		/* stall; record URB status */
		status = -EPIPE;

	} else if (tx_csr & (1 << USBC_BP_TXCSR_H_ERROR)) {
		/* (NON-ISO) dma was disabled, fifo flushed */
		DMSG_PANIC("ERR: sunxi_hcd_host_tx, TX 3strikes on ep=%d\n", epnum);

		status = -ETIMEDOUT;

	} else if (tx_csr & (1 << USBC_BP_TXCSR_H_NAK_TIMEOUT)) {
		DMSG_PANIC("ERR: sunxi_hcd_host_tx, TX end=%d device not responding\n", epnum);

		/* NOTE:  this code path would be a good place to PAUSE a
		 * transfer, if there's some other (nonperiodic) tx urb
		 * that could use this fifo.  (dma complicates it...)
		 * That's already done for bulk RX transfers.
		 *
		 * if (bulk && qh->ring.next != &sunxi_hcd->out_bulk), then
		 * we have a candidate... NAKing is *NOT* an error
		 */
		sunxi_hcd_ep_select(usbc_base, epnum);

		USBC_Writew((USBC_TXCSR_H_WZC_BITS | USBC_BP_TXCSR_H_TX_READY), USBC_REG_TXCSR(usbc_base));

		return;
	}

	if (status) {
		/* dma is working */
		if (sunxi_hcd_dma_is_busy(qh)) {
			DMSG_PANIC("ERR: tx ep error during dma phase\n");

			/* stop dma */
			sunxi_hcd_dma_stop(qh);
			sunxi_hcd_clean_ep_dma_status_and_flush_fifo(qh);
		}

		/* do the proper sequence to abort the transfer in the
		 * usb core; the dma engine should already be stopped.
		 */
		sunxi_hcd_h_tx_flush_fifo(hw_ep);
		tx_csr &= ~((1 << USBC_BP_TXCSR_H_AUTOSET)
			| (1 << USBC_BP_TXCSR_H_DMA_REQ_EN)
			| (1 << USBC_BP_TXCSR_H_ERROR)
			| (1 << USBC_BP_TXCSR_H_TX_STALL)
			| (1 << USBC_BP_TXCSR_H_NAK_TIMEOUT)
			);

		sunxi_hcd_ep_select(usbc_base, epnum);

		USBC_Writew(tx_csr, USBC_REG_TXCSR(usbc_base));
		/* REVISIT may need to clear FLUSHFIFO ... */
		USBC_Writew(tx_csr, USBC_REG_TXCSR(usbc_base));

		USBC_Writew(0x00, USBC_REG_TXINTERVAL(usbc_base));

		done = true;
	}

	if (is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no, qh->segsize, qh->maxpacket, epnum)
		&& !status) {
		/* when dma interrupt occur, USB donnot immediately take away the data in FIFO,
		 * so here check TX_READY, until data been token away
		 *
		 * in normal, after dma complete, will callback, not return
		 */
		if (tx_csr & ((1 << USBC_BP_TXCSR_H_FIFO_NOT_EMPTY) | (1 << USBC_BP_TXCSR_H_TX_READY))) {
			__u32 cmt = 0xfff;

			while (cmt && (tx_csr & ((1 << USBC_BP_TXCSR_H_FIFO_NOT_EMPTY) | (1 << USBC_BP_TXCSR_H_TX_READY)))) {
				tx_csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
				cmt--;
			}

			if (cmt == 0) {
				DMSG_PANIC("ERR: TX ep dma transmit timeout, tx_csr(0x%x), ep(%d), len(%d)\n",
					tx_csr, hw_ep->epnum, urb->transfer_buffer_length);
			}
		}

		/*
		 * DMA has completed.  But if we're using DMA mode 1 (multi
		 * packet DMA), we need a terminal TXPKTRDY interrupt before
		 * we can consider this transfer completed, lest we trash
		 * its last packet when writing the next URB's data.  So we
		 * switch back to mode 0 to get that interrupt; we'll come
		 * back here once it happens.
		 */
		USBC_Host_ClearEpDma(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_TX);

		/*
		 * There is no guarantee that we'll get an interrupt
		 * after clearing DMAMODE as we might have done this
		 * too late (after TXPKTRDY was cleared by controller).
		 * Re-read TXCSR as we have spoiled its previous value.
		 */
		tx_csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));

		/*
		 * We may get here from a DMA completion or TXPKTRDY interrupt.
		 * In any case, we must check the FIFO status here and bail out
		 * only if the FIFO still has data -- that should prevent the
		 * "missed" TXPKTRDY interrupts and deal with double-buffered
		 * FIFO mode too...
		 */
		if (tx_csr & ((1 << USBC_BP_TXCSR_H_FIFO_NOT_EMPTY) | (1 << USBC_BP_TXCSR_H_TX_READY))) {
			DMSG_PANIC("ERR: sunxi_hcd_host_tx, DMA complete but packet still in FIFO, CSR(%04x), RP_Count(%d), qh(0x%p), urb(0x%p, %d, %d)\n",
				tx_csr, USBC_Readw(USBC_REG_RPCOUNTx(usbc_base, hw_ep->epnum)),
				qh, urb, urb->transfer_buffer_length, urb->actual_length);

			return;
		}
	}

	if (!status
		|| is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no, qh->segsize, qh->maxpacket, epnum)
		|| usb_pipeisoc(pipe)) {
		length = qh->segsize;
		qh->offset += length;

		if (usb_pipeisoc(pipe)) {
			struct usb_iso_packet_descriptor *d = NULL;

			d = urb->iso_frame_desc + qh->iso_idx;
			d->actual_length = length;
			d->status = status;
			if (++qh->iso_idx >= urb->number_of_packets) {
				done = true;
			} else {
				d++;
				offset = d->offset;
				length = d->length;
			}
		} else if (is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no, qh->segsize, qh->maxpacket, epnum)) {
			done = true;
		} else {
			/* see if we need to send more data, or ZLP */
			if (qh->segsize < qh->maxpacket)
				done = true;
			else if (qh->offset == urb->transfer_buffer_length
				&& !(urb->transfer_flags & URB_ZERO_PACKET))
				done = true;
			if (!done) {
				offset = qh->offset;
				length = urb->transfer_buffer_length - offset;
			}
		}
	}

	/* urb->status != -EINPROGRESS means request has been faulted,
	 * so we must abort this transfer after cleanup
	 */
	if (urb->status != -EINPROGRESS) {
		done = true;

		if (status == 0)
			status = urb->status;
	}

	if (done) {
		/* set status */
		urb->status = status;
		urb->actual_length = qh->offset;
		sunxi_hcd_advance_schedule(sunxi_hcd, urb, hw_ep, USB_DIR_OUT);

		return;
	} else	if (usb_pipeisoc(pipe) && is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no, qh->segsize, qh->maxpacket, epnum)) {
		if (sunxi_hcd_tx_dma_program(hw_ep, qh, urb, offset, length))
			return;
	} else	if (tx_csr & (1 << USBC_BP_TXCSR_H_DMA_REQ_EN)) {
		DMSG_PANIC("ERR: sunxi_hcd_host_tx, not complete, but DMA enabled?\n");

		return;
	}

	/*
	 * PIO: start next packet in this URB.
	 *
	 * REVISIT: some docs say that when hw_ep->tx_double_buffered,
	 * (and presumably, FIFO is not half-full) we should write *two*
	 * packets before updating TXCSR; other docs disagree...
	 */
	if (length > qh->maxpacket)
		length = qh->maxpacket;

	sunxi_hcd_write_fifo(hw_ep, length, urb->transfer_buffer + offset);
	qh->segsize = length;

	sunxi_hcd_ep_select(usbc_base, epnum);
	USBC_Writew(USBC_TXCSR_H_WZC_BITS | (1 << USBC_BP_TXCSR_H_TX_READY), USBC_REG_TXCSR(usbc_base));

	return;
}
EXPORT_SYMBOL(sunxi_hcd_host_tx);

/* Host side RX (IN) using Mentor DMA works as follows:
 *	submit_urb ->
 *	- if queue was empty, ProgramEndpoint
 *	- first IN token is sent out (by setting ReqPkt)
 *	LinuxIsr -> RxReady()
 *	/\	=> first packet is received
 *	|	- Set in mode 0 (DmaEnab, ~ReqPkt)
 *	|		-> DMA Isr (transfer complete) -> RxReady()
 *	|		    - Ack receive (~RxPktRdy), turn off DMA (~DmaEnab)
 *	|		    - if urb not complete, send next IN token (ReqPkt)
 *	|			   |		else complete urb.
 *	|			   |
 *	---------------------------
 *
 * Nuances of mode 1:
 *	For short packets, no ack (+RxPktRdy) is sent automatically
 *	(even if AutoClear is ON)
 *	For full packets, ack (~RxPktRdy) and next IN token (+ReqPkt) is sent
 *	automatically => major problem, as collecting the next packet becomes
 *	difficult. Hence mode 1 is not used.
 *
 * REVISIT
 *	All we care about at this driver level is that
 *       (a) all URBs terminate with REQPKT cleared and fifo(s) empty;
 *       (b) termination conditions are: short RX, or buffer full;
 *       (c) fault modes include
 *           - iff URB_SHORT_NOT_OK, short RX status is -EREMOTEIO.
 *             (and that endpoint's dma queue stops immediately)
 *           - overflow (full, PLUS more bytes in the terminal packet)
 *
 *	So for example, usb-storage sets URB_SHORT_NOT_OK, and would
 *	thus be a great candidate for using mode 1 ... for all but the
 *	last packet of one URB's transfer.
 */

/*
 * Schedule next QH from sunxi_hcd->in_bulk and move the current qh to
 *   the end; avoids starvation for other endpoints.
 */
static void sunxi_hcd_bulk_rx_nak_timeout(struct sunxi_hcd *sunxi_hcd, struct sunxi_hcd_hw_ep *ep)
{
	struct urb          *urb        = NULL;
	void __iomem        *usbc_base  = NULL;
	struct sunxi_hcd_qh *cur_qh     = NULL;
	struct sunxi_hcd_qh *next_qh    = NULL;
	u16                 rx_csr      = 0;

	/* check argment */
	if (sunxi_hcd == NULL || ep == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, ep=0x%p\n", sunxi_hcd, ep);
		return;
	}

	/* initialize parameter */
	usbc_base = sunxi_hcd->mregs;

	sunxi_hcd_ep_select(usbc_base, ep->epnum);

	/* clear nak timeout bit */
	rx_csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));
	rx_csr |= USBC_RXCSR_H_WZC_BITS;
	rx_csr &= ~(1 << USBC_BP_RXCSR_H_DATA_ERR);
	USBC_Writew(rx_csr, USBC_REG_RXCSR(usbc_base));

	cur_qh = first_qh(&sunxi_hcd->in_bulk);
	if (cur_qh) {
		urb = next_urb(cur_qh);
		if (urb == NULL) {
			DMSG_PANIC("ERR: sunxi_hcd_bulk_rx_nak_timeout, urb is NULL\n");
			return;
		}

		if (sunxi_hcd_dma_is_busy(cur_qh))
			urb->actual_length += sunxi_hcd_dma_transmit_length(cur_qh, is_direction_in(cur_qh), (dma_addr_t)urb->transfer_buffer + cur_qh->offset);
		sunxi_hcd_save_toggle(ep, 1, urb);

		/* move cur_qh to end of queue */
		list_move_tail(&cur_qh->ring, &sunxi_hcd->in_bulk);

		/* get the next qh from sunxi_hcd->in_bulk */
		next_qh = first_qh(&sunxi_hcd->in_bulk);

		/* set rx_reinit and schedule the next qh */
		ep->rx_reinit = 1;
		sunxi_hcd_start_urb(sunxi_hcd, 1, next_qh);
	}

	return;
}

/*
 * Service an RX interrupt for the given IN endpoint; docs cover bulk, iso,
 *   and high-bandwidth IN transfer cases.
 *
 */
void sunxi_hcd_host_rx(struct sunxi_hcd *sunxi_hcd, u8 epnum)
{
	struct urb          *urb        = NULL;
	struct sunxi_hcd_hw_ep *hw_ep   = NULL;
	struct sunxi_hcd_qh *qh         = NULL;
	size_t              xfer_len    = 0;
	void __iomem        *usbc_base  = NULL;
	int                 pipe        = 0;
	u16                 rx_csr      = 0;
	u16                 val         = 0;
	bool                iso_err     = false;
	bool                done        = false;
	u32                 status      = 0;
	u32                 dma_exceptional = 0;

	/* check argment */
	if (sunxi_hcd == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return;
	}

	/* initialize parameter */
	hw_ep       = sunxi_hcd->endpoints + epnum;
	qh          = hw_ep->in_qh;
	usbc_base   = sunxi_hcd->mregs;

	sunxi_hcd_ep_select(usbc_base, epnum);

	urb = next_urb(qh);
	if ((urb == NULL) || (u32)urb < 0x7fffffff) {
		DMSG_PANIC("sunxi_hcd_host_rx, urb is %p\n", urb);
		return;
	}

	status      = 0;
	xfer_len    = 0;

	rx_csr = USBC_Readw(USBC_REG_RXCSR(usbc_base));
	val = rx_csr;

	if (unlikely(!urb)) {
		/* REVISIT -- THIS SHOULD NEVER HAPPEN ... but, at least
		 * usbtest #11 (unlinks) triggers it regularly, sometimes
		 * with fifo full.  (Only with DMA??)
		 */
		DMSG_PANIC("ERR: sunxi_hcd_host_rx, BOGUS RX%d ready, csr %04x, count %d\n",
			epnum, val, USBC_Readw(USBC_REG_RXCOUNT(usbc_base)));

		sunxi_hcd_h_rx_flush_fifo(hw_ep, (1 << USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE));

		return;
	}

	pipe = urb->pipe;

	DMSG_DBG_HCD("rx: ep(0x%p, %d, 0x%x, %d), qh(0x%p, 0x%x, 0x%x), urb(0x%p, 0x%p, %d, %d)\n",
		hw_ep, hw_ep->epnum, USBC_Readw(USBC_REG_RXCSR(usbc_base)), USBC_Readw(USBC_REG_RXCOUNT(usbc_base)),
		qh, qh->epnum, qh->type,
		urb, urb->transfer_buffer, urb->transfer_buffer_length, urb->actual_length);

	/* check for errors, concurrent stall & unlink is not really
	 * handled yet! */
	if (rx_csr & (1 << USBC_BP_RXCSR_H_RX_STALL)) {
		DMSG_PANIC("ERR: sunxi_hcd_host_rx, RX end %d STALL(0x%x)\n", epnum, rx_csr);
		USBC_Host_ClearEpStall(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_RX);

		/* stall; record URB status */
		status = -EPIPE;
	} else if (rx_csr & (1 << USBC_BP_RXCSR_H_ERROR)) {
		if ((qh->type == USB_ENDPOINT_XFER_INT) || (qh->type == USB_ENDPOINT_XFER_BULK)) {
			DMSG_PANIC("ERR: sunxi_hcd_host_rx, end %d RX proto error(0x%x)\n", epnum, rx_csr);

			USBC_Host_ClearEpError(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_RX);

			/* if the device is usb hub, then the hub may be disconnect */
			if (qh->type == USB_ENDPOINT_XFER_INT)
				status = -ECONNRESET;
			else
				status = -EPROTO;

			USBC_Writeb(0x00, USBC_REG_RXINTERVAL(usbc_base));
		}
	} else if (rx_csr & (1 << USBC_BP_RXCSR_H_DATA_ERR)) {
		if (USB_ENDPOINT_XFER_ISOC != qh->type) {
			DMSG_PANIC("ERR: sunxi_hcd_host_rx, RX end %d NAK timeout(0x%x)\n", epnum, rx_csr);

			/* NOTE: NAKing is *NOT* an error, so we want to
			 * continue.  Except ... if there's a request for
			 * another QH, use that instead of starving it.
			 *
			 * Devices like Ethernet and serial adapters keep
			 * reads posted at all times, which will starve
			 * other devices without this logic.
			 */
			if (usb_pipebulk(urb->pipe)
				&& qh->mux == 1
				&& !list_is_singular(&sunxi_hcd->in_bulk)) {
				sunxi_hcd_bulk_rx_nak_timeout(sunxi_hcd, hw_ep);

				return;
			}

			sunxi_hcd_ep_select(usbc_base, epnum);
			rx_csr |= USBC_RXCSR_H_WZC_BITS;
			rx_csr &= ~(1 << USBC_BP_RXCSR_H_DATA_ERR);
			USBC_Writew(rx_csr, USBC_REG_RXCSR(usbc_base));

			goto finish;
		} else {
			DMSG_PANIC("ERR: sunxi_hcd_host_rx, RX end %d ISO data error(0x%x)\n", epnum, rx_csr);

			/* packet error reported later */
			iso_err = true;
		}
	}

	/* faults abort the transfer */
	if (status) {
		/* clean up dma and collect transfer count */
		if (sunxi_hcd_dma_is_busy(qh)) {
			DMSG_PANIC("ERR: rx ep error during dma phase\n");

			/* stop dma */
			sunxi_hcd_dma_stop(qh);
			sunxi_hcd_clean_ep_dma_status_and_flush_fifo(qh);
			xfer_len = sunxi_hcd_dma_transmit_length(qh, is_direction_in(qh), (dma_addr_t)urb->transfer_buffer + qh->offset);
		}

		sunxi_hcd_h_rx_flush_fifo(hw_ep, 1 << USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE);

		USBC_Writeb(0x00, USBC_REG_RXINTERVAL(usbc_base));

		done = true;

		goto finish;
	}

	/* thorough shutdown for now ... given more precise fault handling
	 * and better queueing support, we might keep a DMA pipeline going
	 * while processing this irq for earlier completions.
	 */

	/* FIXME this is _way_ too much in-line logic for Mentor DMA */
	if (rx_csr & (1 << USBC_BP_RXCSR_H_REQ_PACKET))  {
		/* REVISIT this happened for a while on some short reads...
		 * the cleanup still needs investigation... looks bad...
		 * and also duplicates dma cleanup code above ... plus,
		 * shouldn't this be the "half full" double buffer case?
		 */
		if (sunxi_hcd_dma_is_busy(qh)) {
			DMSG_PANIC("ERR: rx ep error during dma phase\n");

			/* stop dma */
			sunxi_hcd_dma_stop(qh);
			sunxi_hcd_clean_ep_dma_status_and_flush_fifo(qh);
			xfer_len = sunxi_hcd_dma_transmit_length(qh, is_direction_in(qh), (dma_addr_t)urb->transfer_buffer + qh->offset);

			done = true;
		}

		DMSG_DBG_HCD("sunxi_hcd_host_rx, epnum, = %d, RXCSR = 0x%x, len = %d\n",
			epnum, rx_csr, xfer_len);

		rx_csr &= ~(1 << USBC_BP_RXCSR_H_REQ_PACKET);

		sunxi_hcd_ep_select(usbc_base, epnum);
		USBC_Writew((USBC_RXCSR_H_WZC_BITS | rx_csr), USBC_REG_RXCSR(usbc_base));
	}

	if (is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no, qh->segsize, qh->maxpacket, epnum) && (rx_csr & (1 << USBC_BP_RXCSR_H_DMA_REQ_EN))) {
		/* during dma, if usb receive short packet, then rx irq come */

		/* query DMA current transfer byte count */
		xfer_len = sunxi_hcd_dma_transmit_length(qh, is_direction_in(qh), (dma_addr_t)(urb->transfer_buffer + qh->offset));

		DMSG_DBG_DMA("line:%d %s, xfer_len = %d, butffer = %d, len = %d\n", __LINE__, __func__, xfer_len, (urb->transfer_buffer + qh->offset), qh->dma_transfer_len);

		/* stop DMA transfer */
		if (sunxi_hcd_dma_is_busy(qh)) {
			DMSG_WRN("WRN: during dma phase, rx irq come\n");
			sunxi_hcd_dma_stop(qh);
			dma_exceptional = 1;
		}

		/* clear dma status */
		if (usb_pipeisoc(pipe)) {
			struct usb_iso_packet_descriptor *d = NULL;

			d = urb->iso_frame_desc + qh->iso_idx;
			d->actual_length = xfer_len;

			/* even if there was an error, we did the dma
			 * for iso_frame_desc->length
			 */
			if (d->status != EILSEQ && d->status != -EOVERFLOW)
				d->status = 0;

			if (++qh->iso_idx >= urb->number_of_packets)
				done = true;
			else
				done = false;
		} else {
			if ((urb->actual_length + xfer_len) >= urb->transfer_buffer_length) {  /* urb buffer is full */
				DMSG_DBG_DMA("line:%d %s\n", __LINE__, __func__);

				done = 1;
			} else if (USBC_Host_IsReadDataReady(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, USBC_EP_TYPE_RX)) {  /* short packet */
				if (dma_exceptional) { /* receive short packet */
					urb->actual_length += xfer_len;
					qh->offset += xfer_len;
					done = sunxi_hcd_host_packet_rx(sunxi_hcd, urb, epnum, iso_err);
					DMSG_DBG_DMA("line:%d %s\n", __LINE__, __func__);
				} else {
					DMSG_PANIC("ERR: have more packet\n");
					done = 0;
				}
			} else {
				DMSG_PANIC("ERR: unkow rx irq come\n");
				done = 1;
			}
		}

		sunxi_hcd_clean_ep_dma_status(qh);

	} else if (urb->status == -EINPROGRESS) {

		DMSG_DBG_DMA("line:%d %s\n", __LINE__, __func__);
		/* if no errors, be sure a packet is ready for unloading */
		if (unlikely(!(rx_csr & (1 << USBC_BP_RXCSR_H_RX_PKT_READY)))) {
			status = -EPROTO;

			DMSG_PANIC("Rx interrupt with no errors or packet!\n");

			/* FIXME this is another "SHOULD NEVER HAPPEN" */

			/* SCRUB (RX) */
			/* do the proper sequence to abort the transfer */
			sunxi_hcd_ep_select(usbc_base, epnum);

			val &= ~(1 << USBC_BP_RXCSR_H_REQ_PACKET);
			USBC_Writew(val, USBC_REG_RXCSR(usbc_base));

			goto finish;
		}

		/* we are expecting IN packets */
		if (is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no, qh->segsize, qh->maxpacket, epnum)) {

			u16 rx_count = 0;
			int length = 0, desired_mode = 0;
			dma_addr_t buf = 0;

			rx_count = USBC_Readw(USBC_REG_RXCOUNT(usbc_base));

			DMSG_DBG_DMA("ERR: sunxi_hcd_host_rx, RX%d count %d, buffer 0x%x len %d/%d\n",
				epnum, rx_count,
				urb->transfer_dma + urb->actual_length,
				qh->offset,
				urb->transfer_buffer_length);

			if (usb_pipeisoc(pipe)) {
				int status = 0;
				struct usb_iso_packet_descriptor *d = NULL;

				d = urb->iso_frame_desc + qh->iso_idx;

				if (iso_err) {
					status = -EILSEQ;
					urb->error_count++;
				}

				if (rx_count > d->length) {
					if (status == 0) {
						status = -EOVERFLOW;
						urb->error_count++;
					}

					DMSG_DBG_HCD("ERR: sunxi_hcd_host_rx, OVERFLOW %d into %d\n", rx_count, d->length);

					length = d->length;
				} else{
					length = rx_count;
				}

				d->status = status;
				buf = urb->transfer_dma + d->offset;
			} else {
				length = rx_count;
				buf = urb->transfer_dma + urb->actual_length;
			}

			desired_mode = 0;

			/* because of the issue below, mode 1 will
			 * only rarely behave with correct semantics.
			 */
			if ((urb->transfer_flags & URB_SHORT_NOT_OK)
				&& ((urb->transfer_buffer_length - urb->actual_length) > qh->maxpacket)) {
				desired_mode = 1;
			}

			if (rx_count < hw_ep->max_packet_sz_rx) {
				length = rx_count;
				desired_mode = 0;
			} else {
				length = urb->transfer_buffer_length;
			}

			/* Disadvantage of using mode 1:
			 *	It's basically usable only for mass storage class; essentially all
			 *	other protocols also terminate transfers on short packets.
			 *
			 * Details:
			 *	An extra IN token is sent at the end of the transfer (due to AUTOREQ)
			 *	If you try to use mode 1 for (transfer_buffer_length - 512), and try
			 *	to use the extra IN token to grab the last packet using mode 0, then
			 *	the problem is that you cannot be sure when the device will send the
			 *	last packet and RxPktRdy set. Sometimes the packet is recd too soon
			 *	such that it gets lost when RxCSR is re-set at the end of the mode 1
			 *	transfer, while sometimes it is recd just a little late so that if you
			 *	try to configure for mode 0 soon after the mode 1 transfer is
			 *	completed, you will find rxcount 0. Okay, so you might think why not
			 *	wait for an interrupt when the pkt is recd. Well, you won't get any!
			 */

			val = USBC_Readw(USBC_REG_RXCSR(usbc_base));
			val &= ~(1 << USBC_BP_RXCSR_H_REQ_PACKET);

			if (desired_mode == 0)
				val &= ~(1 << USBC_BP_RXCSR_H_AUTO_REQ);
			else
				val |= (1 << USBC_BP_RXCSR_H_AUTO_REQ);

			val |= (1 << USBC_BP_RXCSR_H_AUTO_CLEAR) | (1 << USBC_BP_RXCSR_H_DMA_REQ_EN);
			val |= USBC_RXCSR_H_WZC_BITS;
			USBC_Writew(val, USBC_REG_RXCSR(usbc_base));

			USBC_Host_ConfigRqPktCount(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle, qh->hw_ep->epnum, (length / qh->maxpacket));

			/* REVISIT if when actual_length != 0,
			 * transfer_buffer_length needs to be
			 * adjusted first...
			 */
			sunxi_hcd_dma_set_config(qh, buf, length);
			sunxi_hcd_dma_start(qh, (u32 __force)hw_ep->fifo, buf, length);

			DMSG_DBG_DMA("line:%d %s,RXCSR%d := %04x\n", __LINE__, __func__, epnum, val);
		}

		if (!is_sunxi_hcd_dma_capable(sunxi_hcd->usbc_no, qh->segsize, qh->maxpacket, epnum)) {
			done = sunxi_hcd_host_packet_rx(sunxi_hcd, urb, epnum, iso_err);
			DMSG_DBG_HCD("ERR: sunxi_hcd_host_rx, read %spacket\n", done ? "last " : "");
		}
	}

finish:
	if (!dma_exceptional) {
		urb->actual_length += xfer_len;
		qh->offset += xfer_len;
		DMSG_DBG_DMA("line:%d %s\n", __LINE__, __func__);
	}

	if (done) {
		if (urb->status == -EINPROGRESS)
			urb->status = status;

		DMSG_DBG_DMA("line:%d %s\n", __LINE__, __func__);

		sunxi_hcd_advance_schedule(sunxi_hcd, urb, hw_ep, USB_DIR_IN);
	}

	return;
}
EXPORT_SYMBOL(sunxi_hcd_host_rx);

/*
 * note:
 *    schedule nodes correspond to peripheral endpoints, like an OHCI QH.
 * the software schedule associates multiple such nodes with a given
 * host side hardware endpoint + direction; scheduling may activate
 * that hardware endpoint.
 */
static int sunxi_hcd_schedule(struct sunxi_hcd *sunxi_hcd,
				struct sunxi_hcd_qh *qh,
				int is_in)
{
	int                 idle        = 0;
	int                 best_diff   = 0;
	int                 best_end    = 0;
	int                 epnum       = 0;
	struct sunxi_hcd_hw_ep   *hw_ep      = NULL;
	struct list_head    *head       = NULL;

	/* use fixed hardware for control and bulk */
	if (qh->type == USB_ENDPOINT_XFER_CONTROL) {
		head  = &sunxi_hcd->control;
		hw_ep = sunxi_hcd->control_ep;

		goto success;
	}

	/* else, periodic transfers get muxed to other endpoints */

	/*
	 * We know this qh hasn't been scheduled, so all we need to do
	 * is choose which hardware endpoint to put it on ...
	 *
	 * REVISIT what we really want here is a regular schedule tree
	 * like e.g. OHCI uses.
	 */
	best_diff = 4096;
	best_end = -1;

	for (epnum = 1, hw_ep = sunxi_hcd->endpoints + 1;
			epnum < sunxi_hcd->nr_endpoints;
			epnum++, hw_ep++) {
		int	diff = 0;

#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
		if (epnum == BAND_WIDTH_ISO_EP && qh->type == USB_ENDPOINT_XFER_ISOC) {
			if (qh->maxpacket > 512)
				DMSG_WRN("HIGH BAND_WIDTH_ISO\n");
			else
				continue;
		}

		if (epnum == BAND_WIDTH_ISO_EP_EX)
			continue;
#endif

		if (is_in || hw_ep->is_shared_fifo) {
			if (hw_ep->in_qh  != NULL) {
				DMSG_WRN("hw_ep->in_qh = 0x%p, epnum = %d, sunxi_hcd->nr_endpoints = %d\n",
		hw_ep->in_qh, epnum, sunxi_hcd->nr_endpoints);
				continue;
			}
		} else if (hw_ep->out_qh != NULL) {
			DMSG_WRN("hw_ep->out_qh = 0x%p, epnum = %d, sunxi_hcd->nr_endpoints = %d\n",
				hw_ep->out_qh, epnum, sunxi_hcd->nr_endpoints);
			continue;
		}
#ifndef CONFIG_UDC_HIGH_BANDWITH_ISO

		if (hw_ep == sunxi_hcd->bulk_ep) {
			DMSG_WRN("hw_ep == sunxi_hcd->bulk_ep, epnum = %d, sunxi_hcd->nr_endpoints = %d\n",
				epnum, sunxi_hcd->nr_endpoints);
			continue;
		}

		if (hw_ep == sunxi_hcd->bulk_ep) {
			DMSG_WRN("hw_ep == sunxi_hcd->bulk_ep, epnum = %d, sunxi_hcd->nr_endpoints = %d\n",
				epnum, sunxi_hcd->nr_endpoints);
			continue;
		}
#endif

		if (is_in)
			diff = hw_ep->max_packet_sz_rx - qh->maxpacket;
		else
			diff = hw_ep->max_packet_sz_tx - qh->maxpacket;

		/* find the least diff */
		if (diff >= 0 && best_diff > diff) {
			best_diff = diff;
			best_end = epnum;
		}
	}

	/* use bulk reserved ep1 if no other ep is free */
	if (best_end < 0 && qh->type == USB_ENDPOINT_XFER_BULK) {
		DMSG_WRN("wrn: no other ep is free, must use ep1\n");

		hw_ep = sunxi_hcd->bulk_ep;
		if (is_in)
			head = &sunxi_hcd->in_bulk;
		else
			head = &sunxi_hcd->out_bulk;

		/* Enable bulk RX NAK timeout scheme when bulk requests are
		 * multiplexed.  This scheme doen't work in high speed to full
		 * speed scenario as NAK interrupts are not coming from a
		 * full speed device connected to a high speed device.
		 * NAK timeout interval is 8 (128 uframe or 16ms) for HS and
		 * 4 (8 frame or 8ms) for FS device.
		 */
		/*
		if (is_in && qh->dev) {
			qh->intv_reg = (USB_SPEED_HIGH == qh->dev->speed) ? 8 : 4;
		}
		*/
		goto success;
	} else if (best_end < 0) {
		DMSG_PANIC("ERR: sunxi_hcd_schedule, best_end\n");

		return -ENOSPC;
	}

	idle = 1;
	qh->mux = 0;
	hw_ep = sunxi_hcd->endpoints + best_end;

#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
	if (qh->type == USB_ENDPOINT_XFER_ISOC) {
		int old_ep_index_ex = 0;

		old_ep_index_ex = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);
		sunxi_hcd_ep_select(sunxi_hcd->mregs, best_end);
		USBC_Writel(0x0, USBC_REG_RXMAXP(sunxi_hcd->mregs));

		USBC_Writel(0x0, USBC_REG_EX_USB_EPATTR(sunxi_hcd->mregs));
		sunxi_hcd_ep_select(sunxi_hcd->mregs, old_ep_index_ex);

		hw_ep->rx_reinit = 1;
	}

#endif

success:
	if (head) {
		idle = list_empty(head);
		list_add_tail(&qh->ring, head);
		qh->mux = 1;
	}

	qh->hw_ep = hw_ep;
	qh->hep->hcpriv = qh;
	if (idle)
		sunxi_hcd_start_urb(sunxi_hcd, is_in, qh);

	return 0;
}

int sunxi_hcd_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags)
{
	unsigned long                   flags       = 0;
	struct sunxi_hcd		*sunxi_hcd  = NULL;
	struct usb_host_endpoint        *hep        = NULL;
	struct sunxi_hcd_qh		*qh         = NULL;
	struct sunxi_hcd_qh		*qh_temp    = NULL;
	struct usb_endpoint_descriptor  *epd        = NULL;
	int                             ret         = 0;
	unsigned                        type_reg    = 0;
	unsigned                        interval    = 0;

	/* check argment */
	if (hcd == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invalid argment, sunxi_hcd=0x%p, ep=0x%p\n", hcd, urb);
		return -ENODEV;
	}

	/* initialize parameter */
	sunxi_hcd = hcd_to_sunxi_hcd(hcd);
	hep  = urb->ep;
	epd  = &hep->desc;

	/* host role must be active */
	if (!is_host_active(sunxi_hcd) || !sunxi_hcd->is_active) {
		DMSG_PANIC("ERR: sunxi_hcd_urb_enqueue, host is not active\n");
		return -ENODEV;
	}

	/* avoid all allocations within spinlocks */
	qh_temp = kzalloc(sizeof(*qh_temp), mem_flags);
	if (!qh_temp) {
		DMSG_PANIC("ERR: kzalloc failed\n");
		return -ENOMEM;
	}

	spin_lock_irqsave(&sunxi_hcd->lock, flags);

	ret = usb_hcd_link_urb_to_ep(hcd, urb);
	qh = ret ? NULL : hep->hcpriv;
	if (qh)
		urb->hcpriv = qh;

	/* DMA mapping was already done, if needed, and this urb is on
	 * hep->urb_list now ... so we're done, unless hep wasn't yet
	 * scheduled onto a live qh.
	 *
	 * REVISIT best to keep hep->hcpriv valid until the endpoint gets
	 * disabled, testing for empty qh->ring and avoiding qh setup costs
	 * except for the first urb queued after a config change.
	 */

	/* when urb had disposed by sunxi_hcd_giveback, urb will callback by usb core.
	 * program not leave sunxi_hcd_giveback, than new urb is coming, while the qh is busy,
	 * so we must return it.
	 *
	 * the urb will be done by sunxi_hcd_advance_schedule ---> sunxi_hcd_start_urb
	 */
	if (qh || ret) {
		DMSG_WRN("ERR: sunxi_hcd_urb_enqueue, qh(0x%p) is not null, ret(%d)\n", qh, ret);

		if (qh) {
			DMSG_WRN("ERR: sunxi_hcd_urb_enqueue, qh(0x%p, %d, %d) is not null\n",
				qh, qh->epnum, qh->type);
		}

		kfree(qh_temp);
		qh_temp = NULL;

		spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

		return ret;
	}

	/* Allocate and initialize qh, minimizing the work done each time
	 * hw_ep gets reprogrammed, or with irqs blocked.  Then schedule it.
	 *
	 * REVISIT consider a dedicated qh kmem_cache, so it's harder
	 * for bugs in other kernel code to break this driver...
	 */
	qh = qh_temp;
	memset(qh, 0, sizeof(struct sunxi_hcd_qh));

	qh->hep = hep;
	qh->dev = urb->dev;
	INIT_LIST_HEAD(&qh->ring);
	qh->is_ready = 1;


	qh->maxpacket = le16_to_cpu(epd->wMaxPacketSize);
	qh->PacketCount = (qh->maxpacket>>11)&0x3;
	qh->maxpacket &= 0x7ff;

	qh->epnum = usb_endpoint_num(epd);
	qh->type = usb_endpoint_type(epd);

	/* NOTE: urb->dev->devnum is wrong during SET_ADDRESS */
	qh->addr_reg = (u8) usb_pipedevice(urb->pipe);

	/* precompute rxtype/txtype/type0 register */
	type_reg = (qh->type << 4) | qh->epnum;
	switch (urb->dev->speed) {
	case USB_SPEED_LOW:
		type_reg |= 0xc0;
		break;

	case USB_SPEED_FULL:
		type_reg |= 0x80;
		break;

	default:
		type_reg |= 0x40;
	}
	qh->type_reg = type_reg;

	/* Precompute RXINTERVAL/TXINTERVAL register */
	switch (qh->type) {
	case USB_ENDPOINT_XFER_INT:
		/*
		 * Full/low speeds use the  linear encoding,
		 * high speed uses the logarithmic encoding.
		 */
		if (urb->dev->speed <= USB_SPEED_FULL) {
			interval = max_t(u8, epd->bInterval, 1);
			break;
		}
		/* FALLTHROUGH */
	case USB_ENDPOINT_XFER_ISOC:
		/* ISO always uses logarithmic encoding */
		interval = min_t(u8, epd->bInterval, 16);
		break;

	default:
		/* REVISIT we actually want to use NAK limits, hinting to the
		 * transfer scheduling logic to try some other qh, e.g. try
		 * for 2 msec first:
		 *
		 * interval = (USB_SPEED_HIGH == urb->dev->speed) ? 16 : 2;
		 *
		 * The downside of disabling this is that transfer scheduling
		 * gets VERY unfair for nonperiodic transfers; a misbehaving
		 * peripheral could make that hurt.  That's perfectly normal
		 * for reads from network or serial adapters ... so we have
		 * partial NAKlimit support for bulk RX.
		 *
		 * The upside of disabling it is simpler transfer scheduling.
		 */
		interval = 0;
	}
	qh->intv_reg = interval;

	/* precompute addressing for external hub/tt ports */
	if (sunxi_hcd->is_multipoint) {
		struct usb_device *parent = urb->dev->parent;

		if (parent != hcd->self.root_hub) {
			qh->h_addr_reg = (u8) parent->devnum;

			/* set up tt info if needed */
			if (urb->dev->tt) {
				qh->h_port_reg = (u8) urb->dev->ttport;
				if (urb->dev->tt->hub)
					qh->h_addr_reg = (u8) urb->dev->tt->hub->devnum;

				if (urb->dev->tt->multi)
					qh->h_addr_reg |= 0x80;
			}
		}
	}

	DMSG_DBG_HCD("\nq: qh(0x%p, 0x%x, 0x%x), urb(0x%p, 0x%p, %d, %d), dma(0x%x, 0x%x, 0x%x)\n",
		qh, qh->epnum, qh->type,
			urb, urb->transfer_buffer, urb->transfer_buffer_length, urb->actual_length,
			urb->transfer_dma, urb->setup_dma, urb->transfer_flags);
	DMSG_DBG_HCD("q: qh(0x%p, %d, %d, %d), addr(0x%x, 0x%x, 0x%x, 0x%x, 0x%x), urb(0x%p, %d, %d, %d)\n",
		qh, qh->epnum, qh->maxpacket, qh->type,
			qh->type_reg, qh->intv_reg, qh->addr_reg, qh->h_addr_reg, qh->h_port_reg,
			urb, urb->transfer_buffer_length, urb->ep->desc.bEndpointAddress, urb->ep->desc.wMaxPacketSize);

	/* invariant: hep->hcpriv is null OR the qh that's already scheduled.
	 * until we get real dma queues (with an entry for each urb/buffer),
	 * we only have work to do in the former case.
	 */
	if (hep->hcpriv) {
		DMSG_PANIC("ERR: some concurrent activity submitted another urb to hep\n");

		kfree(qh);
		qh = NULL;
		ret = 0;
	} else {
		ret = sunxi_hcd_schedule(sunxi_hcd, qh, epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK);
	}

	if (ret == 0)
		urb->hcpriv = qh;
	else
		DMSG_PANIC("ERR: sunxi_hcd_schedule failed\n");



	if (ret != 0) {
		usb_hcd_unlink_urb_from_ep(hcd, urb);
		kfree(qh);
		qh = NULL;
	}

	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	return ret;
}
EXPORT_SYMBOL(sunxi_hcd_urb_enqueue);

/*
 * note:
 *    abort a transfer that's at the head of a hardware queue.
 * called with controller locked, irqs blocked
 * that hardware queue advances to the next transfer, unless prevented
 */
static int sunxi_hcd_cleanup_urb(struct urb *urb, struct sunxi_hcd_qh *qh, int is_in)
{
	struct sunxi_hcd_hw_ep   *ep    = NULL;
	unsigned            hw_end      = 0;
	void __iomem        *usbc_base  = NULL;
	u16                 csr         = 0;
	int                 status      = 0;

	/* check argment */
	if (urb == NULL || qh == NULL) {
		DMSG_PANIC("ERR: invalid argment, urb=0x%p, qh=0x%p\n", urb, qh);
		return -1;
	}

	/* initialize parameter */
	ep = qh->hw_ep;
	hw_end = ep->epnum;
	usbc_base = ep->sunxi_hcd->mregs;

	DMSG_INFO_HCD0("sunxi_hcd_cleanup_urb: qh(0x%p,0x%x,0x%x), urb(0x%p,%d,%d), ep(0x%p,%d,0x%p,0x%p)\n",
		qh, qh->epnum, qh->type,
		urb, urb->transfer_buffer_length, urb->actual_length,
		ep, ep->epnum, ep->in_qh, ep->out_qh);

	sunxi_hcd_ep_select(usbc_base, hw_end);

	if (is_sunxi_hcd_dma_capable(ep->sunxi_hcd->usbc_no, urb->transfer_buffer_length, qh->maxpacket, ep->epnum)) {
		DMSG_DBG_HCD("HCD: sunxi_hcd_cleanup_urb, abort %cX%d DMA for urb %p --> %d\n",
			(is_in ? 'R' : 'T'), ep->epnum, urb, status);

		/* stop dma */
		sunxi_hcd_dma_stop(qh);
		sunxi_hcd_clean_ep_dma_status_and_flush_fifo(qh);
		urb->actual_length += sunxi_hcd_dma_transmit_length(qh, is_direction_in(qh), (dma_addr_t)urb->transfer_buffer + qh->offset);
	}

	/* turn off DMA requests, discard state, stop polling ... */
	if (is_in) {
		/* giveback saves bulk toggle */
		csr = sunxi_hcd_h_rx_flush_fifo(ep, 0);

		/* REVISIT we still get an irq; should likely clear the
		 * endpoint's irq status here to avoid bogus irqs.
		 * clearing that status is platform-specific...
		 */
	} else if (ep->epnum) {
		sunxi_hcd_h_tx_flush_fifo(ep);

		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
		csr &= ~((1 << USBC_BP_TXCSR_H_AUTOSET)
			| (1 << USBC_BP_TXCSR_H_DMA_REQ_EN)
			| (1 << USBC_BP_TXCSR_H_TX_STALL)
			| (1 << USBC_BP_TXCSR_H_NAK_TIMEOUT)
			| (1 << USBC_BP_TXCSR_H_ERROR)
			| (1 << USBC_BP_TXCSR_H_TX_READY));
		USBC_Writew(csr, USBC_REG_TXCSR(usbc_base));
		/* REVISIT may need to clear FLUSHFIFO ... */
		USBC_Writew(csr, USBC_REG_TXCSR(usbc_base));

		/* flush cpu writebuffer */
		csr = USBC_Readw(USBC_REG_TXCSR(usbc_base));
	} else  {
		sunxi_hcd_h_ep0_flush_fifo(ep);
	}

	if (status == 0)
		sunxi_hcd_advance_schedule(ep->sunxi_hcd, urb, ep, is_in);

	return status;
}

int sunxi_hcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
	struct sunxi_hcd	*sunxi_hcd	= NULL;
	struct sunxi_hcd_qh	*qh     = NULL;
	struct list_head    *sched  = NULL;
	unsigned long       flags   = 0;
	int                 ret     = 0;

#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
	int hep_epnum = 0;
#endif
	/* check argment */
	if (hcd == NULL || urb == NULL) {
		DMSG_PANIC("ERR: invalid argment, hcd=0x%p, urb=0x%p\n", hcd, urb);
		return -1;
	}

	/* initialize parameter */
	sunxi_hcd = hcd_to_sunxi_hcd(hcd);

	spin_lock_irqsave(&sunxi_hcd->lock, flags);

	DMSG_INFO_HCD0("[sunxi_hcd]: sunxi_hcd_urb_dequeue, sunxi_hcd(%p, 0x%d, 0x%x), urb(%p, %d, %d), dev = %d, ep = %d, dir = %s\n",
			sunxi_hcd, sunxi_hcd->ep0_stage, sunxi_hcd->epmask,
			urb, urb->transfer_buffer_length, urb->actual_length,
			usb_pipedevice(urb->pipe),
			usb_pipeendpoint(urb->pipe),
			usb_pipein(urb->pipe) ? "in" : "out");

	ret = usb_hcd_check_unlink_urb(hcd, urb, status);
	if (ret) {
		DMSG_PANIC("ERR: usb_hcd_check_unlink_urb faield\n");
		goto done;
	}

	qh = urb->hcpriv;
	if (!qh) {
		DMSG_PANIC("ERR: urb->hcpriv is null\n");
		goto done;
	}

#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
	if (qh->hw_ep)
		hep_epnum = qh->hw_ep->epnum;
#endif

	/* Any URB not actively programmed into endpoint hardware can be
	 * immediately given back; that's any URB not at the head of an
	 * endpoint queue, unless someday we get real DMA queues.  And even
	 * if it's at the head, it might not be known to the hardware...
	 *
	 * Otherwise abort current transfer, pending dma, etc.; urb->status
	 * has already been updated.  This is a synchronous abort; it'd be
	 * OK to hold off until after some IRQ, though.
	 */
	if (!qh->is_ready || urb->urb_list.prev != &qh->hep->urb_list) {
		ret = -EINPROGRESS;
	} else {
		switch (qh->type) {
		case USB_ENDPOINT_XFER_CONTROL:
			sched = &sunxi_hcd->control;
			break;

		case USB_ENDPOINT_XFER_BULK:
			if (qh->mux == 1) {
				if (usb_pipein(urb->pipe))
					sched = &sunxi_hcd->in_bulk;
				else
					sched = &sunxi_hcd->out_bulk;
			}
break;

		default:
			/* REVISIT when we get a schedule tree, periodic
			 * transfers won't always be at the head of a
			 * singleton queue...
			 */
			sched = NULL;

			break;
		}
	}

	/* NOTE:  qh is invalid unless !list_empty(&hep->urb_list) */
	if (ret < 0 || (sched && qh != first_qh(sched))) {
		int	ready = qh->is_ready;

		ret = 0;
		qh->is_ready = 0;

		__sunxi_hcd_giveback(sunxi_hcd, urb, 0);
		qh->is_ready = ready;

		/* If nothing else (usually sunxi_hcd_giveback) is using it
		 * and its URB list has emptied, recycle this qh.
		 */
#ifdef CONFIG_UDC_HIGH_BANDWITH_ISO
		 if ((qh->type == USB_ENDPOINT_XFER_ISOC &&  list_empty(&qh->hep->urb_list))) {

			qh->hep->hcpriv = NULL;
			urb->ep->hcpriv = NULL;

			{
				struct sunxi_hcd_hw_ep *hw_ep		= NULL;
				int old_ep_index_ex = 0;

				old_ep_index_ex = USBC_GetActiveEp(sunxi_hcd->sunxi_hcd_io->usb_bsp_hdle);

				sunxi_hcd_ep_select(sunxi_hcd->mregs, hep_epnum);
				USBC_Writel(0x00, USBC_REG_RXMAXP(sunxi_hcd->mregs));
				USBC_Writel(0x00, USBC_REG_EX_USB_EPATTR(sunxi_hcd->mregs));

				sunxi_hcd_ep_select(sunxi_hcd->mregs, old_ep_index_ex);

				hw_ep  = sunxi_hcd->endpoints + hep_epnum;

				if (hw_ep) {
					hw_ep->in_qh = NULL;
					hw_ep->out_qh = NULL;
					hw_ep->rx_reinit = 1;
					hw_ep->tx_reinit = 1;
				}
			}
			list_del(&qh->ring);
			kfree(qh);
		} else if (ready && list_empty(&qh->hep->urb_list)) {
			qh->hep->hcpriv = NULL;
			list_del(&qh->ring);
			kfree(qh);
		}
#else
		if (ready && list_empty(&qh->hep->urb_list)) {
			qh->hep->hcpriv = NULL;
			list_del(&qh->ring);
			kfree(qh);
		}
#endif
	} else {
		ret = sunxi_hcd_cleanup_urb(urb, qh, urb->pipe & USB_DIR_IN);
	}

done:
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	return ret;
}
EXPORT_SYMBOL(sunxi_hcd_urb_dequeue);

/*
 * disable an endpoint
 */
void sunxi_hcd_h_disable(struct usb_hcd *hcd, struct usb_host_endpoint *hep)
{
	u8                  epnum   = 0;
	unsigned long		flags   = 0;
	struct sunxi_hcd         *sunxi_hcd   = NULL;
	u8                  is_in   = 0;
	struct sunxi_hcd_qh      *qh     = NULL;
	struct urb          *urb    = NULL;
	struct list_head    *sched  = NULL;

	/* check argment */
	if (hcd == NULL || hep == NULL) {
		DMSG_PANIC("ERR: invalid argment, hcd=0x%p, hep=0x%p\n", hcd, hep);
		return;
	}

	/* initialize parameter */
	epnum = hep->desc.bEndpointAddress;
	sunxi_hcd = hcd_to_sunxi_hcd(hcd);
	is_in = epnum & USB_DIR_IN;

	spin_lock_irqsave(&sunxi_hcd->lock, flags);

	DMSG_INFO_HCD0("[sunxi_hcd]: sunxi_hcd_h_disable, epnum = %x\n", epnum);

	qh = hep->hcpriv;
	if (qh == NULL)
		goto exit;

	switch (qh->type) {
	case USB_ENDPOINT_XFER_CONTROL:
		sched = &sunxi_hcd->control;
		break;

	case USB_ENDPOINT_XFER_BULK:
		if (qh->mux == 1) {
			if (is_in)
				sched = &sunxi_hcd->in_bulk;
			else
				sched = &sunxi_hcd->out_bulk;

		}
		break;

	default:
		DMSG_PANIC("ERR: not support type(%d)\n", qh->type);

		/* REVISIT when we get a schedule tree, periodic transfers
		 * won't always be at the head of a singleton queue...
		 */
		sched = NULL;
		break;
	}

	/* NOTE:  qh is invalid unless !list_empty(&hep->urb_list) */

	/* kick first urb off the hardware, if needed */
	qh->is_ready = 0;
	if (!sched || qh == first_qh(sched)) {
		urb = next_urb(qh);
		if (urb == NULL) {
			DMSG_PANIC("ERR: sunxi_hcd_h_disable, urb is NULL\n");
			goto exit;
		}

		/* make software (then hardware) stop ASAP */
		if (!urb->unlinked)
			urb->status = -ESHUTDOWN;

		/* cleanup */
		sunxi_hcd_cleanup_urb(urb, qh, urb->pipe & USB_DIR_IN);

		/* Then nuke all the others ... and advance the
		 * queue on hw_ep (e.g. bulk ring) when we're done.
		 */
		while (!list_empty(&hep->urb_list)) {
			urb = next_urb(qh);
			if (urb == NULL) {
				DMSG_PANIC("ERR: sunxi_hcd_h_disable, urb is NULL\n");
				goto exit;
			}

			urb->status = -ESHUTDOWN;
			sunxi_hcd_advance_schedule(sunxi_hcd, urb, qh->hw_ep, is_in);
		}
	} else {
		/* Just empty the queue; the hardware is busy with
		 * other transfers, and since !qh->is_ready nothing
		 * will activate any of these as it advances.
		 */
		while (!list_empty(&hep->urb_list))
			__sunxi_hcd_giveback(sunxi_hcd, next_urb(qh), -ESHUTDOWN);

		hep->hcpriv = NULL;
		list_del(&qh->ring);
		kfree(qh);
	}

exit:
	spin_unlock_irqrestore(&sunxi_hcd->lock, flags);

	return;
}
EXPORT_SYMBOL(sunxi_hcd_h_disable);

int sunxi_hcd_h_get_frame_number(struct usb_hcd *hcd)
{
	struct sunxi_hcd	*sunxi_hcd = hcd_to_sunxi_hcd(hcd);

	return USBC_Readw(USBC_REG_FRNUM(sunxi_hcd->mregs));
}
EXPORT_SYMBOL(sunxi_hcd_h_get_frame_number);

int sunxi_hcd_h_start(struct usb_hcd *hcd)
{
	struct sunxi_hcd	*sunxi_hcd = hcd_to_sunxi_hcd(hcd);

	/* NOTE: sunxi_hcd_start() is called when the hub driver turns
	 * on port power, or when (OTG) peripheral starts.
	 */
	hcd->state = HC_STATE_RUNNING;
	sunxi_hcd->port1_status = 0;

	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_h_start);

void sunxi_hcd_h_stop(struct usb_hcd *hcd)
{
	sunxi_hcd_stop(hcd_to_sunxi_hcd(hcd));
	hcd->state = HC_STATE_HALT;
}
EXPORT_SYMBOL(sunxi_hcd_h_stop);

#if 0
int sunxi_hcd_bus_suspend(struct usb_hcd *hcd)
{
	struct sunxi_hcd	*sunxi_hcd = hcd_to_sunxi_hcd(hcd);

	if (is_host_active(sunxi_hcd) && sunxi_hcd->is_active) {
		DMSG_PANIC("ERR: sunxi_hcd_bus_suspend, trying to suspend as host is_active=%i\n",
			sunxi_hcd->is_active);

		return -EBUSY;
	} else {
		return 0;
	}

	return 0;
}
#else
int sunxi_hcd_bus_suspend(struct usb_hcd *hcd)
{
	return 0;
}
#endif
EXPORT_SYMBOL(sunxi_hcd_bus_suspend);

int sunxi_hcd_bus_resume(struct usb_hcd *hcd)
{
	/* resuming child port does the work */
	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_bus_resume);

