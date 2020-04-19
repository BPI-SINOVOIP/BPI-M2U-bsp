/*
 * drivers/usb/sunxi_usb/usbc/usbc_host.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * daniel, 2009.09.21
 *
 * usb register ops.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include  "usbc_i.h"

/* select usb speed type, high/full/low */

/* not config speed type */
static void __USBC_Host_TsMode_Default(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_b(USBC_BP_POWER_H_HIGH_SPEED_EN,
			USBC_REG_PCTL(usbc_base_addr));
}

/* config high speed */
static void __USBC_Host_TsMode_Hs(void __iomem *usbc_base_addr)
{
	USBC_REG_set_bit_b(USBC_BP_POWER_H_HIGH_SPEED_EN,
			USBC_REG_PCTL(usbc_base_addr));
}

/* config full speed */
static void __USBC_Host_TsMode_Fs(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_b(USBC_BP_POWER_H_HIGH_SPEED_EN,
			USBC_REG_PCTL(usbc_base_addr));
}

/* config low speed */
static void __USBC_Host_TsMode_Ls(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_b(USBC_BP_POWER_H_HIGH_SPEED_EN,
			USBC_REG_PCTL(usbc_base_addr));
}

static void __USBC_Host_ep0_EnablePing(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_CSR0_H_DisPing,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_DisablePing(void __iomem *usbc_base_addr)
{
	u16 csr0 = 0;
	csr0 = USBC_Readw(USBC_REG_CSR0(usbc_base_addr));
	csr0 |= (1 << USBC_BP_CSR0_H_DisPing);
	USBC_Writew(csr0, USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Host_ep0_IsNakTimeOut(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_CSR0_H_NAK_Timeout,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_ClearNakTimeOut(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_CSR0_H_NAK_Timeout,
			USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Host_ep0_IsError(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_CSR0_H_Error,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_ClearError(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_CSR0_H_Error,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_EpType(void __iomem *usbc_base_addr, __u32 ts_mode)
{
	__u32 reg_val = 0;

	/* config transfer speed */
	switch (ts_mode) {
	case USBC_TS_MODE_HS:
		reg_val |= 0x01 << USBC_BP_RXTYPE_SPEED;
		break;

	case USBC_TS_MODE_FS:
		reg_val |= 0x02 << USBC_BP_RXTYPE_SPEED;
		break;

	case USBC_TS_MODE_LS:
		reg_val |= 0x03 << USBC_BP_RXTYPE_SPEED;
		break;

	default:
		reg_val = 0;
	}

	USBC_Writeb(reg_val, USBC_REG_EP0TYPE(usbc_base_addr));
}

static void __USBC_Host_ep0_FlushFifo(void __iomem *usbc_base_addr)
{
	USBC_Writew(1 << USBC_BP_CSR0_H_FlushFIFO,
			USBC_REG_CSR0(usbc_base_addr));
	__USBC_Host_ep0_DisablePing(usbc_base_addr);
}

static void __USBC_Host_ep0_ConfigEp_Default(void __iomem *usbc_base_addr)
{
	/* --<1>--config ep0 csr */
	USBC_Writew(1<<USBC_BP_CSR0_H_FlushFIFO, USBC_REG_CSR0(usbc_base_addr));
	__USBC_Host_ep0_DisablePing(usbc_base_addr);

	/* --<2>--config polling interval */
	USBC_Writeb(0x00, USBC_REG_TXINTERVAL(usbc_base_addr));

	/* config ep transfer type */
	USBC_Writeb(0x00, USBC_REG_EP0TYPE(usbc_base_addr));
}

static void __USBC_Host_ep0_ConfigEp(void __iomem *usbc_base_addr,
			__u32 ts_mode, __u32 interval)
{
	/* --<1>--config ep0 csr */
	USBC_Writew(1<<USBC_BP_CSR0_H_FlushFIFO, USBC_REG_CSR0(usbc_base_addr));
	__USBC_Host_ep0_DisablePing(usbc_base_addr);

	/* --<2>--config polling interval */
	USBC_Writeb(interval, USBC_REG_NAKLIMIT0(usbc_base_addr));

	/* config ep0 transfer type */
	__USBC_Host_ep0_EpType(usbc_base_addr, ts_mode);
}

static __u32 __USBC_Host_ep0_IsReadDataReady(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_CSR0_H_RxPkRdy,
			USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Host_ep0_IsWriteDataReady(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_CSR0_H_TxPkRdy,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_ReadDataHalf(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_CSR0_H_RxPkRdy,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_ReadDataComplete(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_CSR0_H_RxPkRdy,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_WriteDataHalf(void __iomem *usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_CSR0_H_TxPkRdy,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_WriteDataComplete(void __iomem *usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_CSR0_H_TxPkRdy,
			USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Host_ep0_IsStall(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_CSR0_H_RxStall,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_ClearStall(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_CSR0_H_RxStall,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_ClearCSR(void __iomem *usbc_base_addr)
{
	USBC_Writew(0x00, USBC_REG_CSR0(usbc_base_addr));
	__USBC_Host_ep0_DisablePing(usbc_base_addr);
}

static __u32 __USBC_Host_ep0_IsReqPktSet(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_CSR0_H_ReqPkt,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_StartInToken(void __iomem *usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_CSR0_H_ReqPkt,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_StopInToken(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_CSR0_H_ReqPkt,
			USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_StatusAfterIn(void __iomem *usbc_base_addr)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readw(USBC_REG_CSR0(usbc_base_addr));
	reg_val |= 1 << USBC_BP_CSR0_H_TxPkRdy;
	reg_val |= 1 << USBC_BP_CSR0_H_StatusPkt;
	USBC_Writew(reg_val, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_StatusAfterOut(void __iomem *usbc_base_addr)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readw(USBC_REG_CSR0(usbc_base_addr));
	reg_val |= 1 << USBC_BP_CSR0_H_ReqPkt;
	reg_val |= 1 << USBC_BP_CSR0_H_StatusPkt;
	USBC_Writew(reg_val, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_ep0_SendSetupPkt(void __iomem *usbc_base_addr)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readw(USBC_REG_CSR0(usbc_base_addr));
	reg_val |= 1 << USBC_BP_CSR0_H_SetupPkt;
	reg_val |= 1 << USBC_BP_CSR0_H_TxPkRdy;
	USBC_Writew(reg_val, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Host_Tx_EpType(void __iomem *usbc_base_addr,
			__u32 ep_index, __u32 ts_mode, __u32 ts_type)
{
	__u32 reg_val = 0;

	/* config transfer speed */
	switch (ts_mode) {
	case USBC_TS_MODE_HS:
		reg_val |= 0x01 << USBC_BP_TXTYPE_SPEED;
		break;

	case USBC_TS_MODE_FS:
		reg_val |= 0x02 << USBC_BP_TXTYPE_SPEED;
		break;

	case USBC_TS_MODE_LS:
		reg_val |= 0x03 << USBC_BP_TXTYPE_SPEED;
		break;

	default:
		reg_val = 0;
	}

	/* --<1>--config protocol */
	switch (ts_type) {
	case USBC_TS_TYPE_ISO:
		reg_val |= 0x1 << USBC_BP_TXTYPE_PROROCOL;
		break;

	case USBC_TS_TYPE_BULK:
		reg_val |= 0x2 << USBC_BP_TXTYPE_PROROCOL;
		break;

	case USBC_TS_TYPE_INT:
		reg_val |= 0x3 << USBC_BP_TXTYPE_PROROCOL;
		break;

	default:
		break;
	}

	/* --<2>--config target ep number */
	reg_val |= ep_index;

	USBC_Writeb(reg_val, USBC_REG_TXTYPE(usbc_base_addr));
}

static void __USBC_Host_Tx_FlushFifo(void __iomem *usbc_base_addr)
{
	USBC_Writew((1 << USBC_BP_TXCSR_H_CLEAR_DATA_TOGGLE)
			| (1 << USBC_BP_TXCSR_H_FLUSH_FIFO),
	USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Tx_ConfigEp_Default(void __iomem *usbc_base_addr)
{
	/* --<1>--config tx_csr, flush fifo, clear all to 0 */
	USBC_Writew((1 << USBC_BP_TXCSR_H_CLEAR_DATA_TOGGLE)
			| (1 << USBC_BP_TXCSR_H_FLUSH_FIFO),
	USBC_REG_TXCSR(usbc_base_addr));

	USBC_Writew(0x00, USBC_REG_TXCSR(usbc_base_addr));

	/* --<2>--config tx ep max packet */
	USBC_Writew(0x00, USBC_REG_TXMAXP(usbc_base_addr));

	/* --<3>--config ep transfer type */
	USBC_Writeb(0x00, USBC_REG_TXTYPE(usbc_base_addr));

	/* --<4>--config polling interval */
	USBC_Writeb(0x00, USBC_REG_TXINTERVAL(usbc_base_addr));
}

static void __USBC_Host_Tx_ConfigEp(void __iomem *usbc_base_addr,
		__u32 ep_index, __u32 ts_mode, __u32 ts_type,
		__u32 is_double_fifo, __u32 ep_MaxPkt, __u32 interval)
{
	__u16 reg_val = 0;
	__u16 temp = 0;

	/* --<1>--config tx_csr */
	USBC_Writew((1 << USBC_BP_TXCSR_H_MODE)
		| (1 << USBC_BP_TXCSR_H_CLEAR_DATA_TOGGLE)
		| (1 << USBC_BP_TXCSR_H_FLUSH_FIFO),
	USBC_REG_TXCSR(usbc_base_addr));

	if (is_double_fifo) {
		USBC_Writew((1 << USBC_BP_TXCSR_H_MODE)
			| (1 << USBC_BP_TXCSR_H_CLEAR_DATA_TOGGLE)
			| (1 << USBC_BP_TXCSR_H_FLUSH_FIFO),
		USBC_REG_TXCSR(usbc_base_addr));
	}

	/* --<2>--config tx ep max packet */
	reg_val = USBC_Readw(USBC_REG_TXMAXP(usbc_base_addr));
	temp    = ep_MaxPkt & ((1 << USBC_BP_TXMAXP_PACKET_COUNT) - 1);
	reg_val |= temp;
	USBC_Writew(reg_val, USBC_REG_TXMAXP(usbc_base_addr));

	/* --<3>--config ep transfer type */
	__USBC_Host_Tx_EpType(usbc_base_addr, ep_index, ts_mode, ts_type);

	/* --<4>--config polling interval */
	USBC_Writeb(interval, USBC_REG_TXINTERVAL(usbc_base_addr));
}

static void __USBC_Host_Tx_ConfigEpDma(void __iomem *usbc_base_addr)
{
	__u16 ep_csr = 0;

	/* auto_set, tx_mode, dma_tx_en, mode1 */
	ep_csr = USBC_Readb(USBC_REG_TXCSR(usbc_base_addr) + 1);
	ep_csr |= (1 << USBC_BP_TXCSR_H_AUTOSET) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_H_MODE) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_H_DMA_REQ_EN) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_H_DMA_REQ_MODE) >> 8;
	USBC_Writeb(ep_csr, (USBC_REG_TXCSR(usbc_base_addr) + 1));
}

static void __USBC_Host_Tx_ClearEpDma(void __iomem *usbc_base_addr)
{
	__u16 ep_csr = 0;

	/* auto_set, dma_tx_en, mode1 */
	ep_csr = USBC_Readb(USBC_REG_TXCSR(usbc_base_addr) + 1);
	ep_csr &= ~((1 << USBC_BP_TXCSR_H_AUTOSET) >> 8);
	ep_csr &= ~((1 << USBC_BP_TXCSR_H_DMA_REQ_EN) >> 8);
	USBC_Writeb(ep_csr, (USBC_REG_TXCSR(usbc_base_addr) + 1));

	/* DMA_REQ_EN and DMA_REQ_MODE cannot be cleared in the same cycle */
	ep_csr = USBC_Readb(USBC_REG_TXCSR(usbc_base_addr) + 1);
	ep_csr &= ~((1 << USBC_BP_TXCSR_H_DMA_REQ_MODE) >> 8);
	USBC_Writeb(ep_csr, (USBC_REG_TXCSR(usbc_base_addr) + 1));
}

static __u32 __USBC_Host_Tx_IsWriteDataReady(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_TXCSR_H_TX_READY,
			USBC_REG_TXCSR(usbc_base_addr))
		| USBC_REG_test_bit_w(USBC_BP_TXCSR_H_FIFO_NOT_EMPTY,
			USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Tx_WriteDataHalf(void __iomem *usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_TXCSR_H_TX_READY,
			USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Tx_WriteDataComplete(void __iomem *usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_TXCSR_H_TX_READY,
			USBC_REG_TXCSR(usbc_base_addr));
}

static __u32 __USBC_Host_Tx_IsNakTimeOut(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_TXCSR_H_NAK_TIMEOUT,
			USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Tx_ClearNakTimeOut(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_TXCSR_H_NAK_TIMEOUT,
			USBC_REG_TXCSR(usbc_base_addr));
}

static __u32 __USBC_Host_Tx_IsError(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_TXCSR_H_ERROR,
			USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Tx_ClearError(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_TXCSR_H_ERROR,
			USBC_REG_TXCSR(usbc_base_addr));
}

static __u32 __USBC_Host_Tx_IsStall(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_TXCSR_H_TX_STALL,
			USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Tx_ClearStall(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_TXCSR_H_TX_STALL,
			USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Tx_ClearCSR(void __iomem *usbc_base_addr)
{
	USBC_Writew(0x00, USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_EpType(void __iomem *usbc_base_addr,
		__u32 ep_index, __u32 ts_mode, __u32 ts_type)
{
	__u32 reg_val = 0;

	/* config transfer speed */
	switch (ts_mode) {
	case USBC_TS_MODE_HS:
		reg_val |= 0x01 << USBC_BP_RXTYPE_SPEED;
		break;

	case USBC_TS_MODE_FS:
		reg_val |= 0x02 << USBC_BP_RXTYPE_SPEED;
		break;

	case USBC_TS_MODE_LS:
		reg_val |= 0x03 << USBC_BP_RXTYPE_SPEED;
		break;

	default:
		reg_val = 0;
	}

	/* --<1>--config protocol */
	switch (ts_type) {
	case USBC_TS_TYPE_ISO:
		reg_val |= 0x1 << USBC_BP_RXTYPE_PROROCOL;
		break;

	case USBC_TS_TYPE_BULK:
		reg_val |= 0x2 << USBC_BP_RXTYPE_PROROCOL;
		break;

	case USBC_TS_TYPE_INT:
		reg_val |= 0x3 << USBC_BP_RXTYPE_PROROCOL;
		break;

	default:
		break;
	}

	/* --<2>--config target ep number */
	reg_val |= ep_index;

	USBC_Writeb(reg_val, USBC_REG_RXTYPE(usbc_base_addr));
}

static void __USBC_Host_Rx_FlushFifo(void __iomem *usbc_base_addr)
{
	USBC_Writew((1 << USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE)
			| (1 << USBC_BP_RXCSR_H_FLUSH_FIFO),
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_ConfigEp_Default(void __iomem *usbc_base_addr)
{
	/* --<1>--config rx_csr, flush fifo */
	USBC_Writew((1 << USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE)
			| (1 << USBC_BP_RXCSR_H_FLUSH_FIFO),
			USBC_REG_RXCSR(usbc_base_addr));

	USBC_Writew(0x00, USBC_REG_RXCSR(usbc_base_addr));

	/* --<2>--config rx ep max packet */
	USBC_Writew(0x00, USBC_REG_RXMAXP(usbc_base_addr));

	/* --<3>--config ep transfer type */
	USBC_Writeb(0x00, USBC_REG_RXTYPE(usbc_base_addr));

	/* --<4>--config polling interval */
	USBC_Writeb(0x00, USBC_REG_RXINTERVAL(usbc_base_addr));
}

static void __USBC_Host_Rx_ConfigEp(void __iomem *usbc_base_addr,
		__u32 ep_index, __u32 ts_mode, __u32 ts_type,
		__u32 is_double_fifo, __u32 ep_MaxPkt, __u32 interval)
{
	__u16 reg_val = 0;
	__u16 temp = 0;

	/* --<1>--config rx_csr */
	USBC_Writew((1 << USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE)
			| (1 << USBC_BP_RXCSR_H_FLUSH_FIFO),
			USBC_REG_RXCSR(usbc_base_addr));

	if (is_double_fifo) {
		USBC_Writew((1 << USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE)
				| (1 << USBC_BP_RXCSR_H_FLUSH_FIFO),
				USBC_REG_RXCSR(usbc_base_addr));
	}

	/* --<2>--config tx ep max packet */
	reg_val = USBC_Readw(USBC_REG_RXMAXP(usbc_base_addr));
	temp    = ep_MaxPkt & ((1 << USBC_BP_RXMAXP_PACKET_COUNT) - 1);
	reg_val |= temp;
	USBC_Writew(reg_val, USBC_REG_RXMAXP(usbc_base_addr));

	/* --<3>--config ep transfer type */
	__USBC_Host_Rx_EpType(usbc_base_addr, ep_index, ts_mode, ts_type);

	/* --<4>--config polling interval */
	USBC_Writeb(interval, USBC_REG_RXINTERVAL(usbc_base_addr));
}

static void __USBC_Host_Rx_ConfigEpDma(void __iomem *usbc_base_addr)
{
	__u16 ep_csr = 0;

	/* config dma, auto_clear, dma_rx_en, mode1, */
	ep_csr = USBC_Readb(USBC_REG_RXCSR(usbc_base_addr) + 1);
	ep_csr |= (1 << USBC_BP_RXCSR_H_AUTO_CLEAR) >> 8;
	ep_csr |= (1 << USBC_BP_RXCSR_H_AUTO_REQ) >> 8;
	/* ep_csr &= ~((1 << USBC_BP_RXCSR_H_DMA_REQ_MODE) >> 8); */
	ep_csr |= ((1 << USBC_BP_RXCSR_H_DMA_REQ_MODE) >> 8);
	ep_csr |= (1 << USBC_BP_RXCSR_H_DMA_REQ_EN) >> 8;
	USBC_Writeb(ep_csr, (USBC_REG_RXCSR(usbc_base_addr) + 1));
}

static void __USBC_Host_Rx_ClearEpDma(void __iomem *usbc_base_addr)
{
	__u16 ep_csr = 0;

	/* auto_clear, dma_rx_en, mode1, */
	ep_csr = USBC_Readb(USBC_REG_RXCSR(usbc_base_addr) + 1);
	ep_csr &= ~((1 << USBC_BP_RXCSR_H_AUTO_CLEAR) >> 8);
	ep_csr &= ~((1 << USBC_BP_RXCSR_H_AUTO_REQ) >> 8);
	ep_csr &= ~((1 << USBC_BP_RXCSR_H_DMA_REQ_MODE) >> 8);
	ep_csr &= ~((1 << USBC_BP_RXCSR_H_DMA_REQ_EN) >> 8);
	USBC_Writeb(ep_csr, (USBC_REG_RXCSR(usbc_base_addr) + 1));
}

static __u32 __USBC_Host_Rx_IsReadDataReady(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_RXCSR_H_RX_PKT_READY,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_ReadDataHalf(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_H_RX_PKT_READY,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_ReadDataComplete(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_H_RX_PKT_READY,
			USBC_REG_RXCSR(usbc_base_addr));
}

static __u32 __USBC_Host_Rx_IsNakTimeOut(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_RXCSR_H_NAK_TIMEOUT,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_ClearNakTimeOut(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_H_NAK_TIMEOUT,
			USBC_REG_RXCSR(usbc_base_addr));
}

static __u32 __USBC_Host_Rx_IsError(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_RXCSR_H_ERROR,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_ClearError(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_H_ERROR,
			USBC_REG_RXCSR(usbc_base_addr));
}

static __u32 __USBC_Host_Rx_IsStall(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_RXCSR_H_RX_STALL,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_ClearStall(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_H_RX_STALL,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_ClearCSR(void __iomem *usbc_base_addr)
{
	USBC_Writew(0x00, USBC_REG_RXCSR(usbc_base_addr));
}

static __u32 __USBC_Host_Rx_IsReqPktSet(void __iomem *usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_RXCSR_H_REQ_PACKET,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_StartInToken(void __iomem *usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_RXCSR_H_REQ_PACKET,
			USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Host_Rx_StopInToken(void __iomem *usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_H_REQ_PACKET,
			USBC_REG_RXCSR(usbc_base_addr));
}

void USBC_Host_SetFunctionAddress_Deafult(__hdle hUSB,
		__u32 ep_type, __u32 ep_index)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_TX:
		USBC_Writeb(0x00, USBC_REG_TXFADDRx(usbc_otg->base_addr,
				ep_index));
		USBC_Writeb(0x00, USBC_REG_TXHADDRx(usbc_otg->base_addr,
				ep_index));
		USBC_Writeb(0x00, USBC_REG_TXHPORTx(usbc_otg->base_addr,
				ep_index));
		break;

	case USBC_EP_TYPE_RX:
		USBC_Writeb(0x00, USBC_REG_RXFADDRx(usbc_otg->base_addr,
				ep_index));
		USBC_Writeb(0x00, USBC_REG_RXHADDRx(usbc_otg->base_addr,
				ep_index));
		USBC_Writeb(0x00, USBC_REG_RXHPORTx(usbc_otg->base_addr,
				ep_index));
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_SetFunctionAddress_Deafult);

void USBC_Host_SetFunctionAddress(__hdle hUSB,
						__u32 EpType,
						__u32 EpIndex,
						__u32 FunctionAdress,
						__u32 MultiTT,
						__u32 HubAddress,
						__u32 HubPortNumber)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;
	__u8 temp_8 = 0;

	if (usbc_otg == NULL)
		return;

	if (MultiTT)
		temp_8 = 1 << USBC_BP_HADDR_MULTI_TT;

	temp_8 |= HubAddress;

	switch (EpType) {
	case USBC_EP_TYPE_TX:
		USBC_Writeb(FunctionAdress,
			USBC_REG_TXFADDRx(usbc_otg->base_addr, EpIndex));
		USBC_Writeb(temp_8,
			USBC_REG_TXHADDRx(usbc_otg->base_addr, EpIndex));
		USBC_Writeb(HubPortNumber,
			USBC_REG_TXHPORTx(usbc_otg->base_addr, EpIndex));
		break;

	case USBC_EP_TYPE_RX:
		USBC_Writeb(FunctionAdress,
			USBC_REG_RXFADDRx(usbc_otg->base_addr, EpIndex));
		USBC_Writeb(temp_8,
			USBC_REG_RXHADDRx(usbc_otg->base_addr, EpIndex));
		USBC_Writeb(HubPortNumber,
			USBC_REG_RXHPORTx(usbc_otg->base_addr, EpIndex));
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_SetFunctionAddress);

void USBC_Host_SetHubAddress_Deafult(__hdle hUSB, __u32 ep_type, __u32 ep_index)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_TX:
		USBC_Writeb(0x00,
			USBC_REG_TXHADDRx(usbc_otg->base_addr, ep_index));
		break;

	case USBC_EP_TYPE_RX:
		USBC_Writeb(0x00,
			USBC_REG_RXHADDRx(usbc_otg->base_addr, ep_index));
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_SetHubAddress_Deafult);

void USBC_Host_SetHubAddress(__hdle hUSB, __u32 ep_type,
		__u32 ep_index, __u32 is_mutli_tt, __u8 address)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_TX:
		USBC_Writeb(((is_mutli_tt << USBC_BP_HADDR_MULTI_TT) | address),
			USBC_REG_TXHADDRx(usbc_otg->base_addr, ep_index));
		break;

	case USBC_EP_TYPE_RX:
		USBC_Writeb(((is_mutli_tt << USBC_BP_HADDR_MULTI_TT) | address),
			USBC_REG_RXHADDRx(usbc_otg->base_addr, ep_index));
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_SetHubAddress);

void USBC_Host_SetHPortAddress_Deafult(__hdle hUSB,
		__u32 ep_type, __u32 ep_index)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_TX:
		USBC_Writeb(0x00,
			USBC_REG_TXHPORTx(usbc_otg->base_addr, ep_index));
		break;

	case USBC_EP_TYPE_RX:
		USBC_Writeb(0x00,
			USBC_REG_RXHPORTx(usbc_otg->base_addr, ep_index));
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_SetHPortAddress_Deafult);

void USBC_Host_SetHPortAddress(__hdle hUSB, __u32 ep_type,
		__u32 ep_index, __u8 address)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_TX:
		USBC_Writeb(address,
			USBC_REG_TXHPORTx(usbc_otg->base_addr, ep_index));
		break;

	case USBC_EP_TYPE_RX:
		USBC_Writeb(address,
			USBC_REG_RXHPORTx(usbc_otg->base_addr, ep_index));
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_SetHPortAddress);

__u32 USBC_Host_QueryTransferMode(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return USBC_TS_MODE_UNKOWN;

	if (USBC_REG_test_bit_b(USBC_BP_POWER_H_HIGH_SPEED_FLAG,
			USBC_REG_PCTL(usbc_otg->base_addr))) {
		return USBC_TS_MODE_HS;
	} else {
		return USBC_TS_MODE_FS;
	}
}
EXPORT_SYMBOL(USBC_Host_QueryTransferMode);

void USBC_Host_ConfigTransferMode(__hdle hUSB, __u32 speed_mode)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	/* select transfer speed */
	switch (speed_mode) {
	case USBC_TS_MODE_HS:
		__USBC_Host_TsMode_Hs(usbc_otg->base_addr);
		break;

	case USBC_TS_MODE_FS:
		__USBC_Host_TsMode_Fs(usbc_otg->base_addr);
		break;

	case USBC_TS_MODE_LS:
		__USBC_Host_TsMode_Ls(usbc_otg->base_addr);
		break;

	default:
		__USBC_Host_TsMode_Default(usbc_otg->base_addr);
	}
}
EXPORT_SYMBOL(USBC_Host_ConfigTransferMode);

/* reset the device on usb port, recommended reset gap is 100ms */
void USBC_Host_ResetPort(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_REG_set_bit_b(USBC_BP_POWER_H_RESET,
			USBC_REG_PCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_ResetPort);

/*
 * USBC_Host_ResetPort and USBC_Host_ClearResetPortFlag
 * should combine, but may influent efficiency in bsp delay
 */
void USBC_Host_ClearResetPortFlag(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_REG_clear_bit_b(USBC_BP_POWER_H_RESET,
			USBC_REG_PCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_ClearResetPortFlag);

/* resume the device on usb port, recommended resume gap is 10ms */
void USBC_Host_RusumePort(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_REG_set_bit_b(USBC_BP_POWER_H_RESUME,
			USBC_REG_PCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_RusumePort);

/*
 * USBC_Host_RusumePort and USBC_Host_ClearRusumePortFlag
 * should combine, but may influent efficiency in bsp delay
 */
void USBC_Host_ClearRusumePortFlag(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_REG_clear_bit_b(USBC_BP_POWER_H_RESUME,
			USBC_REG_PCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_ClearRusumePortFlag);

/* usb port suspend */
void USBC_Host_SuspendPort(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_REG_set_bit_b(USBC_BP_POWER_H_SUSPEND,
			USBC_REG_PCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_SuspendPort);

__u32 USBC_Host_QueryPowerStatus(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return 0;

	return USBC_Readb(USBC_REG_PCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_QueryPowerStatus);

void USBC_Host_StartSession(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_REG_set_bit_b(USBC_BP_DEVCTL_SESSION,
			USBC_REG_DEVCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_StartSession);

void USBC_Host_EndSession(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_REG_clear_bit_b(USBC_BP_DEVCTL_SESSION,
			USBC_REG_DEVCTL(usbc_otg->base_addr));
}
EXPORT_SYMBOL(USBC_Host_EndSession);

/*
 * peripheral device type
 * @hUSB: handle return by USBC_open_otg,
 * include the key data which USBC need
 */
__u32 USBC_Host_PeripheralType(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;
	__u8 reg_val = 0;

	if (usbc_otg == NULL)
		return 0;

	reg_val = USBC_Readb(USBC_REG_DEVCTL(usbc_otg->base_addr));
	if (reg_val & (1 << USBC_BP_DEVCTL_FS_DEV))
		return USBC_DEVICE_FSDEV;
	else if (reg_val & (1 << USBC_BP_DEVCTL_LS_DEV))
		return USBC_DEVICE_LSDEV;
	else
		return USBC_DEVICE_LSDEV;
}
EXPORT_SYMBOL(USBC_Host_PeripheralType);

void USBC_Host_FlushFifo(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_FlushFifo(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_FlushFifo(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_FlushFifo(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_FlushFifo);

/*
 * release all resource of Ep, except irq.
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 */
void USBC_Host_ConfigEp_Default(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ConfigEp_Default(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ConfigEp_Default(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ConfigEp_Default(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ConfigEp_Default);

/*
 * config Ep, including double fifo, max packet, etc.
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type:    ep type
 * @ep_index:   ep index for dest device
 * @ts_type:    transfer type
 * @is_double_fifo: if is double fifo
 * @ep_MaxPkt:  ep max packet
 * @interval:   time gap
 *
 */
void USBC_Host_ConfigEp(__hdle hUSB, __u32 ep_type,
		__u32 ep_index, __u32 ts_mode, __u32 ts_type,
		__u32 is_double_fifo, __u32 ep_MaxPkt, __u32 interval)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ConfigEp(usbc_otg->base_addr,
				ts_mode, interval);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ConfigEp(usbc_otg->base_addr,
				ep_index, ts_mode, ts_type,
				is_double_fifo, ep_MaxPkt, interval);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ConfigEp(usbc_otg->base_addr,
				ep_index, ts_mode, ts_type,
				is_double_fifo, ep_MaxPkt, interval);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ConfigEp);

/*
 * config Ep dma
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
void USBC_Host_ConfigEpDma(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		/* not support */
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ConfigEpDma(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ConfigEpDma(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ConfigEpDma);

/*
 * clear Ep dma setting
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
void USBC_Host_ClearEpDma(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		/* not support */
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ClearEpDma(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ClearEpDma(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ClearEpDma);

/*
 * check if Ep is NakTimeOut
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 * return: 0 - NAK not timeout, 1 - NAK timeout
 */
__u32 USBC_Host_IsEpNakTimeOut(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return 0;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		return __USBC_Host_ep0_IsNakTimeOut(usbc_otg->base_addr);

	case USBC_EP_TYPE_TX:
		return __USBC_Host_Tx_IsNakTimeOut(usbc_otg->base_addr);

	case USBC_EP_TYPE_RX:
		return __USBC_Host_Rx_IsNakTimeOut(usbc_otg->base_addr);

	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(USBC_Host_IsEpNakTimeOut);

/*
 * clear Ep NakTimeOut status
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
void USBC_Host_ClearEpNakTimeOut(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ClearNakTimeOut(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ClearNakTimeOut(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ClearNakTimeOut(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ClearEpNakTimeOut);

/*
 * check if Ep is error
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 * return: 0 - ep is not error, 1 - ep is error
 */
__u32 USBC_Host_IsEpError(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return 0;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		return __USBC_Host_ep0_IsError(usbc_otg->base_addr);

	case USBC_EP_TYPE_TX:
		return __USBC_Host_Tx_IsError(usbc_otg->base_addr);

	case USBC_EP_TYPE_RX:
		return __USBC_Host_Rx_IsError(usbc_otg->base_addr);

	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(USBC_Host_IsEpError);

/*
 * clear Ep error status
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
void USBC_Host_ClearEpError(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ClearError(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ClearError(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ClearError(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ClearEpError);

/*
 * check if Ep is stalled
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 * return: 0 - ep is not stall, 1 - ep is stall
 */
__u32 USBC_Host_IsEpStall(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return 0;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		return __USBC_Host_ep0_IsStall(usbc_otg->base_addr);

	case USBC_EP_TYPE_TX:
		return __USBC_Host_Tx_IsStall(usbc_otg->base_addr);

	case USBC_EP_TYPE_RX:
		return __USBC_Host_Rx_IsStall(usbc_otg->base_addr);

	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(USBC_Host_IsEpStall);

/*
 * clear Ep stall status
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
void USBC_Host_ClearEpStall(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ClearStall(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ClearStall(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ClearStall(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ClearEpStall);

void USBC_Host_ClearEpCSR(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ClearCSR(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_ClearCSR(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ClearCSR(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_ClearEpCSR);

static __s32 __USBC_Host_ReadDataHalf(void __iomem *usbc_base_addr,
		__u32 ep_type)
{
	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ReadDataHalf(usbc_base_addr);
		break;

	case USBC_EP_TYPE_TX:
		/* not support */
		return -1;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ReadDataHalf(usbc_base_addr);
		break;

	default:
		return -1;
	}

	return 0;
}

static __s32 __USBC_Host_ReadDataComplete(void __iomem *usbc_base_addr,
		__u32 ep_type)
{
	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_ReadDataComplete(usbc_base_addr);
		break;

	case USBC_EP_TYPE_TX:
		/* not support */
		return -1;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_ReadDataComplete(usbc_base_addr);
		break;

	default:
		return -1;
	}

	return 0;
}

static __s32 __USBC_Host_WriteDataHalf(void __iomem *usbc_base_addr,
		__u32 ep_type)
{
	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_WriteDataHalf(usbc_base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_WriteDataHalf(usbc_base_addr);
		break;

	case USBC_EP_TYPE_RX:
		/* not support */
		return -1;

	default:
		return -1;
	}

	return 0;
}

static __s32 __USBC_Host_WriteDataComplete(void __iomem *usbc_base_addr,
		__u32 ep_type)
{
	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_WriteDataComplete(usbc_base_addr);
		break;

	case USBC_EP_TYPE_TX:
		__USBC_Host_Tx_WriteDataComplete(usbc_base_addr);
		break;

	case USBC_EP_TYPE_RX:
		/* not support */
		return -1;

	default:
		return -1;
	}

	return 0;
}

/*
 * check if the data ready for read
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
__u32 USBC_Host_IsReadDataReady(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return 0;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		return __USBC_Host_ep0_IsReadDataReady(usbc_otg->base_addr);

	case USBC_EP_TYPE_TX:
		/* not support */
		break;

	case USBC_EP_TYPE_RX:
		return __USBC_Host_Rx_IsReadDataReady(usbc_otg->base_addr);

	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(USBC_Host_IsReadDataReady);

/*
 * check if fifo empty
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
__u32 USBC_Host_IsWriteDataReady(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return 0;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		return __USBC_Host_ep0_IsWriteDataReady(usbc_otg->base_addr);

	case USBC_EP_TYPE_TX:
		return __USBC_Host_Tx_IsWriteDataReady(usbc_otg->base_addr);

	case USBC_EP_TYPE_RX:
		/* not support */
		break;

	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(USBC_Host_IsWriteDataReady);

/*
 * get read status. if read over or not
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 * @complete: if all data has been written
 *
 * return 0 on success, !0 on fail.
 */
__s32 USBC_Host_ReadDataStatus(__hdle hUSB, __u32 ep_type, __u32 complete)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return -1;

	if (complete)
		__USBC_Host_ReadDataComplete(usbc_otg->base_addr, ep_type);
	else
		__USBC_Host_ReadDataHalf(usbc_otg->base_addr, ep_type);

	return 0;
}
EXPORT_SYMBOL(USBC_Host_ReadDataStatus);

/*
 * get write status. if write over or not
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 * @complete: if all data has been written
 *
 * return 0 on success, !0 on fail.
 */
__s32 USBC_Host_WriteDataStatus(__hdle hUSB, __u32 ep_type, __u32 complete)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return -1;

	if (complete)
		return __USBC_Host_WriteDataComplete(
				usbc_otg->base_addr, ep_type);
	else
		return __USBC_Host_WriteDataHalf(
				usbc_otg->base_addr, ep_type);
}
EXPORT_SYMBOL(USBC_Host_WriteDataStatus);

/*
 * check if ReqPkt bit is set
 * @hUSB: handle return by USBC_open_otg,
 *        include the key data which USBC need
 * @ep_type: ep type
 *
 */
__u32 USBC_Host_IsReqPktSet(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return 0;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		return __USBC_Host_ep0_IsReqPktSet(usbc_otg->base_addr);

	case USBC_EP_TYPE_TX:
		/* not support */
		break;

	case USBC_EP_TYPE_RX:
		return __USBC_Host_Rx_IsReqPktSet(usbc_otg->base_addr);

	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(USBC_Host_IsReqPktSet);

/*
 * start sending in token to device
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
void USBC_Host_StartInToken(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_StartInToken(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		/* not support */
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_StartInToken(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_StartInToken);

/*
 * stop sending in token to device
 * @hUSB:    handle return by USBC_open_otg,
 *           include the key data which USBC need
 * @ep_type: ep type
 *
 */
void USBC_Host_StopInToken(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	switch (ep_type) {
	case USBC_EP_TYPE_EP0:
		__USBC_Host_ep0_StopInToken(usbc_otg->base_addr);
		break;

	case USBC_EP_TYPE_TX:
		/* not support */
		break;

	case USBC_EP_TYPE_RX:
		__USBC_Host_Rx_StopInToken(usbc_otg->base_addr);
		break;

	default:
		break;
	}
}
EXPORT_SYMBOL(USBC_Host_StopInToken);

void USBC_Host_ConfigRqPktCount(__hdle hUSB, __u32 ep_index, __u32 RqPktCount)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;
	void __iomem *temp = 0;

	if (usbc_otg == NULL)
		return;

	temp = USBC_REG_RPCOUNTx(usbc_otg->base_addr, ep_index);

	USBC_Writew(RqPktCount, temp);
}
EXPORT_SYMBOL(USBC_Host_ConfigRqPktCount);

void USBC_Host_ClearRqPktCount(__hdle hUSB, __u32 ep_index)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	USBC_Writew(0x00, USBC_REG_RPCOUNTx(usbc_otg->base_addr, ep_index));
}
EXPORT_SYMBOL(USBC_Host_ClearRqPktCount);

void USBC_Host_EnablePing(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	__USBC_Host_ep0_EnablePing(usbc_otg->base_addr);
}
EXPORT_SYMBOL(USBC_Host_EnablePing);

void USBC_Host_DisablePing(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	__USBC_Host_ep0_DisablePing(usbc_otg->base_addr);
}
EXPORT_SYMBOL(USBC_Host_DisablePing);

void USBC_Host_SendCtrlStatus(__hdle hUSB, __u32 is_after_in)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	if (is_after_in)
		__USBC_Host_ep0_StatusAfterIn(usbc_otg->base_addr);
	else
		__USBC_Host_ep0_StatusAfterOut(usbc_otg->base_addr);
}
EXPORT_SYMBOL(USBC_Host_SendCtrlStatus);

void USBC_Host_SendSetupPkt(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL)
		return;

	__USBC_Host_ep0_SendSetupPkt(usbc_otg->base_addr);
}
EXPORT_SYMBOL(USBC_Host_SendSetupPkt);
