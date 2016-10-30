/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include  "usbc_i.h"

/*
 ***************************************************************************
 *
 * 选择 usb device 的传输类型。如控制传输、同步传输、中断传输、批量传输。
 *
 ***************************************************************************
 */
static void __USBC_Dev_TsType_default(ulong usbc_base_addr)
{
    //disable所有传输类型
    USBC_REG_clear_bit_b(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(usbc_base_addr));
}

static void __USBC_Dev_TsType_Ctrl(ulong usbc_base_addr)
{
    //--<1>--disable其他传输类型
    USBC_REG_clear_bit_b(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(usbc_base_addr));

	//--<2>--选择 Ctrl 类型
	/* 不需要配置 */
}

static void __USBC_Dev_TsType_Iso(ulong usbc_base_addr)
{
    //--<1>--disable其他传输类型
    /* 不需要配置 */

    //--<2>--选择 Ctrl 类型
    USBC_REG_set_bit_b(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(usbc_base_addr));
}

static void __USBC_Dev_TsType_Int(ulong usbc_base_addr)
{
    //--<1>--disable其他传输类型
    USBC_REG_clear_bit_b(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(usbc_base_addr));

	//--<2>--选择 Ctrl 类型
	/* 不需要配置 */
}

static void __USBC_Dev_TsType_Bulk(ulong usbc_base_addr)
{
    //--<1>--disable其他传输类型
    USBC_REG_clear_bit_b(USBC_BP_POWER_D_ISO_UPDATE_EN, USBC_REG_PCTL(usbc_base_addr));

	//--<2>--选择 Ctrl 类型
	/* 不需要配置 */
}

/*
 ***************************************************************************
 *
 * 选择 usb device 的速度类型。如 高速、全速、低速。
 *
 ***************************************************************************
 */

static void __USBC_Dev_TsMode_default(ulong usbc_base_addr)
{
    USBC_REG_clear_bit_b(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(usbc_base_addr));
}

static void __USBC_Dev_TsMode_Hs(ulong usbc_base_addr)
{
    USBC_REG_set_bit_b(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(usbc_base_addr));
}

static void __USBC_Dev_TsMode_Fs(ulong usbc_base_addr)
{
    USBC_REG_clear_bit_b(USBC_BP_POWER_D_HIGH_SPEED_EN, USBC_REG_PCTL(usbc_base_addr));
}

static void __USBC_Dev_TsMode_Ls(ulong usbc_base_addr)
{
    //硬件不支持ls, 所以默认选择fs
    __USBC_Dev_TsMode_Fs(usbc_base_addr);
}

static void __USBC_Dev_ep0_ConfigEp0_Default(ulong usbc_base_addr)
{
    USBC_Writew(1<<USBC_BP_CSR0_D_FLUSH_FIFO, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_ConfigEp0(ulong usbc_base_addr)
{
    USBC_Writew(1<<USBC_BP_CSR0_D_FLUSH_FIFO, USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Dev_ep0_IsReadDataReady(ulong usbc_base_addr)
{
    return USBC_REG_test_bit_w(USBC_BP_CSR0_D_RX_PKT_READY, USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Dev_ep0_IsWriteDataReady(ulong usbc_base_addr)
{
    return USBC_REG_test_bit_w(USBC_BP_CSR0_D_TX_PKT_READY, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_ReadDataHalf(ulong usbc_base_addr)
{
	USBC_Writew(1<<USBC_BP_CSR0_D_SERVICED_RX_PKT_READY, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_ReadDataComplete(ulong usbc_base_addr)
{
    USBC_Writew((1<<USBC_BP_CSR0_D_SERVICED_RX_PKT_READY) | (1<<USBC_BP_CSR0_D_DATA_END),
	            USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_WriteDataHalf(ulong usbc_base_addr)
{
	USBC_Writew(1<<USBC_BP_CSR0_D_TX_PKT_READY, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_WriteDataComplete(ulong usbc_base_addr)
{
    USBC_Writew((1<<USBC_BP_CSR0_D_TX_PKT_READY) | (1<<USBC_BP_CSR0_D_DATA_END),
	            USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Dev_ep0_IsEpStall(ulong usbc_base_addr)
{
    return USBC_REG_test_bit_w(USBC_BP_CSR0_D_SENT_STALL, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_SendStall(ulong usbc_base_addr)
{
    USBC_REG_set_bit_w(USBC_BP_CSR0_D_SEND_STALL,
		               USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_ClearStall(ulong usbc_base_addr)
{
    USBC_REG_clear_bit_w(USBC_BP_CSR0_D_SEND_STALL, USBC_REG_CSR0(usbc_base_addr));
    USBC_REG_clear_bit_w(USBC_BP_CSR0_D_SENT_STALL, USBC_REG_CSR0(usbc_base_addr));
}

static __u32 __USBC_Dev_ep0_IsSetupEnd(ulong usbc_base_addr)
{
    return USBC_REG_test_bit_w(USBC_BP_CSR0_D_SETUP_END, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_ep0_ClearSetupEnd(ulong usbc_base_addr)
{
    USBC_REG_set_bit_w(USBC_BP_CSR0_D_SERVICED_SETUP_END, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_Tx_EnableIsoEp(ulong usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_TXCSR_D_ISO, USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Dev_Tx_EnableIntEp(ulong usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_TXCSR_D_ISO, USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Dev_Tx_EnableBulkEp(ulong usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_TXCSR_D_ISO, USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Dev_Tx_ConfigEp_Default(ulong usbc_base_addr)
{
    //--<1>--clear tx csr
	USBC_Writew(0x00, USBC_REG_TXCSR(usbc_base_addr));

	//--<2>--clear tx ep max packet
	USBC_Writew(0x00, USBC_REG_TXMAXP(usbc_base_addr));

	//--<3>--config ep transfer type
}

static void __USBC_Dev_Tx_ConfigEp(ulong usbc_base_addr, __u32 ts_type, __u32 is_double_fifo, __u32 ep_MaxPkt)
{
    __u16 reg_val = 0;
	__u16 temp = 0;

    //--<1>--config tx csr
	reg_val = (1 << USBC_BP_TXCSR_D_MODE);
	reg_val |= (1 << USBC_BP_TXCSR_D_CLEAR_DATA_TOGGLE);
	reg_val |= (1 << USBC_BP_TXCSR_D_FLUSH_FIFO);
	USBC_Writew(reg_val, USBC_REG_TXCSR(usbc_base_addr));

	if(is_double_fifo){
		USBC_Writew(reg_val, USBC_REG_TXCSR(usbc_base_addr));
	}

	//--<2>--config tx ep max packet
	reg_val = USBC_Readw(USBC_REG_TXMAXP(usbc_base_addr));
	temp    = ep_MaxPkt & ((1 << USBC_BP_TXMAXP_PACKET_COUNT) - 1);
	reg_val &= ~(0x1fff);		//added by jerry
	reg_val |= temp;
	USBC_Writew(reg_val, USBC_REG_TXMAXP(usbc_base_addr));

	//--<3>--config ep transfer type
	switch(ts_type){
		case USBC_TS_TYPE_ISO:
			__USBC_Dev_Tx_EnableIsoEp(usbc_base_addr);
		break;

		case USBC_TS_TYPE_INT:
			__USBC_Dev_Tx_EnableIntEp(usbc_base_addr);
		break;

		case USBC_TS_TYPE_BULK:
			__USBC_Dev_Tx_EnableBulkEp(usbc_base_addr);
		break;

		default:
			__USBC_Dev_Tx_EnableBulkEp(usbc_base_addr);
	}
}

static void __USBC_Dev_Tx_ConfigEpDma(ulong usbc_base_addr)
{
    __u16 ep_csr = 0;

	//auto_set, tx_mode, dma_tx_en, mode1
	ep_csr = USBC_Readb(USBC_REG_TXCSR(usbc_base_addr) + 1);
	ep_csr |= (1 << USBC_BP_TXCSR_D_AUTOSET) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_D_MODE) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_D_DMA_REQ_EN) >> 8;
	ep_csr |= (1 << USBC_BP_TXCSR_D_DMA_REQ_MODE) >> 8;
	USBC_Writeb(ep_csr, (USBC_REG_TXCSR(usbc_base_addr) + 1));
}

static void __USBC_Dev_Tx_ClearEpDma(ulong usbc_base_addr)
{
    __u16 ep_csr = 0;

	//auto_set, dma_tx_en, mode1
	ep_csr = USBC_Readb(USBC_REG_TXCSR(usbc_base_addr) + 1);
	ep_csr &= ~((1 << USBC_BP_TXCSR_D_AUTOSET) >> 8);
	ep_csr &= ~((1 << USBC_BP_TXCSR_D_DMA_REQ_EN) >> 8);
	USBC_Writeb(ep_csr, (USBC_REG_TXCSR(usbc_base_addr) + 1));

	//DMA_REQ_EN和DMA_REQ_MODE不能在同一个cycle中清除
	ep_csr = USBC_Readb(USBC_REG_TXCSR(usbc_base_addr) + 1);
	ep_csr &= ~((1 << USBC_BP_TXCSR_D_DMA_REQ_MODE) >> 8);
	USBC_Writeb(ep_csr, (USBC_REG_TXCSR(usbc_base_addr) + 1));
}

static __u32 __USBC_Dev_Tx_IsWriteDataReady(ulong usbc_base_addr)
{
    __u32 temp = 0;

    temp = USBC_Readw(USBC_REG_TXCSR(usbc_base_addr));
	temp &= (1 << USBC_BP_TXCSR_D_TX_READY) | (1 << USBC_BP_TXCSR_D_FIFO_NOT_EMPTY);

	return temp;
}

static __u32 __USBC_Dev_Tx_IsWriteDataReady_FifoEmpty(ulong usbc_base_addr)
{
	__u32 temp = 0;

	temp = USBC_Readw(USBC_REG_TXCSR(usbc_base_addr));
	temp &= (1 << USBC_BP_TXCSR_D_TX_READY) | (1 << USBC_BP_TXCSR_D_FIFO_NOT_EMPTY);

	return temp;
}

static void __USBC_Dev_Tx_WriteDataHalf(ulong usbc_base_addr)
{
    __u16 ep_csr = 0;

	ep_csr = USBC_Readw(USBC_REG_TXCSR(usbc_base_addr));
	ep_csr |= 1 << USBC_BP_TXCSR_D_TX_READY;
	ep_csr &= ~(1 << USBC_BP_TXCSR_D_UNDER_RUN);
	USBC_Writew(ep_csr, USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Dev_Tx_WriteDataComplete(ulong usbc_base_addr)
{
    __u16 ep_csr = 0;

	ep_csr = USBC_Readw(USBC_REG_TXCSR(usbc_base_addr));
	ep_csr |= 1 << USBC_BP_TXCSR_D_TX_READY;
	ep_csr &= ~(1 << USBC_BP_TXCSR_D_UNDER_RUN);
	USBC_Writew(ep_csr, USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Dev_Tx_SendStall(ulong usbc_base_addr)
{
    //send stall, and fifo is flushed automaticly
	USBC_REG_set_bit_w(USBC_BP_TXCSR_D_SEND_STALL, USBC_REG_TXCSR(usbc_base_addr));
}

static __u32 __USBC_Dev_Tx_IsEpStall(ulong usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_TXCSR_D_SENT_STALL, USBC_REG_TXCSR(usbc_base_addr));
}


static void __USBC_Dev_Tx_ClearStall(ulong usbc_base_addr)
{
	__u32 reg_val;

	reg_val = USBC_Readw(USBC_REG_TXCSR(usbc_base_addr));
	reg_val &= ~((1 << USBC_BP_TXCSR_D_SENT_STALL)|(1 << USBC_BP_TXCSR_D_SEND_STALL));
	USBC_Writew(reg_val, USBC_REG_TXCSR(usbc_base_addr));
}

//这里为了去掉编译时的warning, 而把这里屏蔽掉的
/*
static __u32 __USBC_Dev_Tx_IsEpIncomp(ulong usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_TXCSR_D_INCOMPLETE, USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Dev_Tx_ClearIncomp(ulong usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_TXCSR_D_INCOMPLETE, USBC_REG_TXCSR(usbc_base_addr));
}
*/
static void __USBC_Dev_Rx_EnableIsoEp(ulong usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_RXCSR_D_ISO, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_EnableIntEp(ulong usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_D_ISO, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_EnableBulkEp(ulong usbc_base_addr)
{
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_D_ISO, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_ConfigEp_Default(ulong usbc_base_addr)
{
    //--<1>--clear tx csr
	USBC_Writew(0x00, USBC_REG_RXCSR(usbc_base_addr));

	//--<2>--clear tx ep max packet
	USBC_Writew(0x00, USBC_REG_RXMAXP(usbc_base_addr));

	//--<3>--config ep transfer type
}

static void __USBC_Dev_Rx_ConfigEp(ulong usbc_base_addr, __u32 ts_type, __u32 is_double_fifo, __u32 ep_MaxPkt)
{
    __u16 reg_val = 0;
	__u16 temp = 0;

    //--<1>--config tx csr
    USBC_Writew((1 << USBC_BP_RXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_RXCSR_D_FLUSH_FIFO),
                USBC_REG_RXCSR(usbc_base_addr));

	if(is_double_fifo){
		USBC_Writew((1 << USBC_BP_RXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_RXCSR_D_FLUSH_FIFO),
                    USBC_REG_RXCSR(usbc_base_addr));
	}

	//--<2>--config tx ep max packet
	reg_val = USBC_Readw(USBC_REG_RXMAXP(usbc_base_addr));
	temp    = ep_MaxPkt & ((1 << USBC_BP_RXMAXP_PACKET_COUNT) - 1);
	reg_val &= ~(0x1fff);		//added by jerry
	reg_val |= temp;
	USBC_Writew(reg_val, USBC_REG_RXMAXP(usbc_base_addr));

	//--<3>--config ep transfer type
	switch(ts_type){
		case USBC_TS_TYPE_ISO:
			__USBC_Dev_Rx_EnableIsoEp(usbc_base_addr);
		break;

		case USBC_TS_TYPE_INT:
			__USBC_Dev_Rx_EnableIntEp(usbc_base_addr);
		break;

		case USBC_TS_TYPE_BULK:
			__USBC_Dev_Rx_EnableBulkEp(usbc_base_addr);
		break;

		default:
			__USBC_Dev_Rx_EnableBulkEp(usbc_base_addr);
	}
}

static void __USBC_Dev_Rx_ConfigEpDma(ulong usbc_base_addr)
{
    __u16 ep_csr = 0;

    //auto_clear, dma_rx_en, mode0
	ep_csr = USBC_Readb(USBC_REG_RXCSR(usbc_base_addr) + 1);
	ep_csr |= ((1 << USBC_BP_RXCSR_D_AUTO_CLEAR) >> 8);
	ep_csr &= ~((1 << USBC_BP_RXCSR_D_DMA_REQ_MODE) >> 8);
	ep_csr |= ((1 << USBC_BP_RXCSR_D_DMA_REQ_EN) >> 8);
	USBC_Writeb(ep_csr, (USBC_REG_RXCSR(usbc_base_addr) + 1));
}

static void __USBC_Dev_Rx_ClearEpDma(ulong usbc_base_addr)
{
    __u16 ep_csr = 0;

    //auto_clear, dma_rx_en, mode0
	ep_csr = USBC_Readb(USBC_REG_RXCSR(usbc_base_addr) + 1);
	ep_csr &= ~((1 << USBC_BP_RXCSR_D_AUTO_CLEAR) >> 8);
	ep_csr &= ~((1 << USBC_BP_RXCSR_D_DMA_REQ_MODE) >> 8);
	ep_csr &= ~((1 << USBC_BP_RXCSR_D_DMA_REQ_EN) >> 8);
	USBC_Writeb(ep_csr, (USBC_REG_RXCSR(usbc_base_addr) + 1));
}

static __u32 __USBC_Dev_Rx_IsReadDataReady(ulong usbc_base_addr)
{
    return USBC_REG_test_bit_w(USBC_BP_RXCSR_D_RX_PKT_READY, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_ReadDataHalf(ulong usbc_base_addr)
{
	__u32 reg_val = 0;

	//overrun, dataerr is used in iso transfer
    reg_val = USBC_Readw(USBC_REG_RXCSR(usbc_base_addr));
	reg_val &= ~(1 << USBC_BP_RXCSR_D_RX_PKT_READY);
	reg_val &= ~(1 << USBC_BP_RXCSR_D_OVERRUN);
	reg_val &= ~(1 << USBC_BP_RXCSR_D_DATA_ERROR);
	USBC_Writew(reg_val, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_ReadDataComplete(ulong usbc_base_addr)
{
    __u32 reg_val = 0;

	//overrun, dataerr is used in iso transfer
	reg_val = USBC_Readw(USBC_REG_RXCSR(usbc_base_addr));
	reg_val &= ~(1 << USBC_BP_RXCSR_D_RX_PKT_READY);
	reg_val &= ~(1 << USBC_BP_RXCSR_D_OVERRUN);
	reg_val &= ~(1 << USBC_BP_RXCSR_D_DATA_ERROR);
	USBC_Writew(reg_val, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_SendStall(ulong usbc_base_addr)
{
	USBC_REG_set_bit_w(USBC_BP_RXCSR_D_SEND_STALL, USBC_REG_RXCSR(usbc_base_addr));
}

static __u32 __USBC_Dev_Rx_IsEpStall(ulong usbc_base_addr)
{
	return USBC_REG_test_bit_w(USBC_BP_RXCSR_D_SENT_STALL, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_ClearStall(ulong usbc_base_addr)
{
    USBC_REG_clear_bit_w(USBC_BP_RXCSR_D_SEND_STALL, USBC_REG_RXCSR(usbc_base_addr));
	USBC_REG_clear_bit_w(USBC_BP_RXCSR_D_SENT_STALL, USBC_REG_RXCSR(usbc_base_addr));
}

static void __USBC_Dev_ClearDma_Trans(ulong usbc_base_addr)
{
	__u32 reg_val;

	reg_val  = readl(usbc_base_addr + USBC_REG_o_PCTL);
	reg_val &= ~(1 << 24);
	writel(reg_val, usbc_base_addr + USBC_REG_o_PCTL);
}

static void __USBC_Dev_ConfigDma_Trans(ulong usbc_base_addr)
{
	__u32 reg_val;

	reg_val  = readl(usbc_base_addr + USBC_REG_o_PCTL);
	reg_val |= (1 << 24);
	writel(reg_val, usbc_base_addr + USBC_REG_o_PCTL);
}
/*
***********************************************************************************
*                     USBC_Dev_SetAddress_default
*
* Description:
*    清除host给device分配的地址
*
* Arguments:
*    hUSB       :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*
* Returns:
*
*
* note:
*    无
*
***********************************************************************************
*/
void USBC_Dev_SetAddress_default(__hdle hUSB)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return;
	}

    USBC_Writeb(0x00, USBC_REG_FADDR(usbc_otg->base_addr));
}

/*
***********************************************************************************
*                     USBC_Dev_SetAddress
*
* Description:
*    设置地址
*
* Arguments:
*    hUSB       :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    address    :  input.  host分配的地址
*
* Returns:
*
*
* note:
*    无
*
***********************************************************************************
*/
void USBC_Dev_SetAddress(__hdle hUSB, __u8 address)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return;
	}

    USBC_Writeb(address, USBC_REG_FADDR(usbc_otg->base_addr));
}

__u32 USBC_Dev_QueryTransferMode(__hdle hUSB)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return USBC_TS_MODE_UNKOWN;
	}

	if(USBC_REG_test_bit_b(USBC_BP_POWER_D_HIGH_SPEED_FLAG, USBC_REG_PCTL(usbc_otg->base_addr))){
		return USBC_TS_MODE_HS;
	}else{
	    return USBC_TS_MODE_FS;
	}
}

/*
***********************************************************************************
*                     USBC_Dev_ConfigTransferMode
*
* Description:
*    配置device的传输类型和速度模式
*
* Arguments:
*    hUSB       :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    type       :  input.  传输类型
*    speed_mode :  input.  速度模式
*
* Returns:
*
*
* note:
*    无
*
***********************************************************************************
*/
void USBC_Dev_ConfigTransferMode(__hdle hUSB, __u8 ts_type, __u8 speed_mode)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return;
	}

    //--<1>--选择传输类型
    //默认是bulk传输
    switch(ts_type){
		case USBC_TS_TYPE_CTRL:
			__USBC_Dev_TsType_Ctrl(usbc_otg->base_addr);
		break;

		case USBC_TS_TYPE_ISO:
			__USBC_Dev_TsType_Iso(usbc_otg->base_addr);
		break;

		case USBC_TS_TYPE_INT:
			__USBC_Dev_TsType_Int(usbc_otg->base_addr);
		break;

		case USBC_TS_TYPE_BULK:
			__USBC_Dev_TsType_Bulk(usbc_otg->base_addr);
		break;

		default:
			__USBC_Dev_TsType_default(usbc_otg->base_addr);
	}

    //--<2>--选择传输速度
    switch(speed_mode){
		case USBC_TS_MODE_HS:
			__USBC_Dev_TsMode_Hs(usbc_otg->base_addr);
		break;

		case USBC_TS_MODE_FS:
			__USBC_Dev_TsMode_Fs(usbc_otg->base_addr);
		break;

		case USBC_TS_MODE_LS:
			__USBC_Dev_TsMode_Ls(usbc_otg->base_addr);
		break;

		default:
			__USBC_Dev_TsMode_default(usbc_otg->base_addr);
	}
}

/*
***********************************************************************************
*                     USBC_Dev_ConectSwitch
*
* Description:
*    和PC通信的开关
*
* Arguments:
*    hUSB   :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    is_on  :  input.  1: 打开和PC通信开关. 0: 关闭和PC通信通道
*
* Returns:
*
*
* note:
*    无
*
***********************************************************************************
*/
void USBC_Dev_ConectSwitch(__hdle hUSB, __u32 is_on)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return ;
	}

	if(is_on == USBC_DEVICE_SWITCH_ON){
		USBC_REG_set_bit_b(USBC_BP_POWER_D_SOFT_CONNECT, USBC_REG_PCTL(usbc_otg->base_addr));
	}else{
		USBC_REG_clear_bit_b(USBC_BP_POWER_D_SOFT_CONNECT, USBC_REG_PCTL(usbc_otg->base_addr));
	}
}

/*
***********************************************************************************
*                     USBC_Dev_PowerStatus
*
* Description:
*    查询当前device所处的状态, 如reset、resume、suspend等状态。
*
* Arguments:
*    hUSB   :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*
* Returns:
*
*
* note:
*    无
*
***********************************************************************************
*/
__u32 USBC_Dev_QueryPowerStatus(__hdle hUSB)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return 0;
	}

    return (USBC_Readb(USBC_REG_PCTL(usbc_otg->base_addr)) & 0x0f);
}

/*
***********************************************************************************
*                     USBC_Dev_ConfigEp
*
* Description:
*    配置ep, 包括双FIFO、最大传输包等
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type        :  input.  传输类型
*    is_double_fifo :  input.  速度模式
*    ep_MaxPkt      :  input.  最大包
*
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_ConfigEp(__hdle hUSB, __u32 ts_type, __u32 ep_type, __u32 is_double_fifo, __u32 ep_MaxPkt)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

	switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_ConfigEp0(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_ConfigEp(usbc_otg->base_addr, ts_type, is_double_fifo,  ep_MaxPkt);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_ConfigEp(usbc_otg->base_addr, ts_type, is_double_fifo, ep_MaxPkt);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_ConfigEp
*
* Description:
*    释放ep所有的资源, 中断除外
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_ConfigEp_Default(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

	switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_ConfigEp0_Default(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_ConfigEp_Default(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_ConfigEp_Default(usbc_otg->base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_ConfigEpDma
*
* Description:
*    配置ep的dma设置
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type        :  input.  传输类型
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_ConfigEpDma(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

	switch(ep_type){
		case USBC_EP_TYPE_EP0:
			//not support

			return -1;
		//break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_ConfigEpDma(usbc_otg->base_addr);
			__USBC_Dev_ConfigDma_Trans(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_ConfigEpDma(usbc_otg->base_addr);
			__USBC_Dev_ConfigDma_Trans(usbc_otg->base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_ClearEpDma
*
* Description:
*    清除ep的dma设置
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type        :  input.  传输类型
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_ClearEpDma(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

	switch(ep_type){
		case USBC_EP_TYPE_EP0:
			//not support

			return -1;
		//break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_ClearEpDma(usbc_otg->base_addr);
			__USBC_Dev_ClearDma_Trans(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_ClearEpDma(usbc_otg->base_addr);
			__USBC_Dev_ClearDma_Trans(usbc_otg->base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_IsEpStall
*
* Description:
*    查询ep是否stall
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type        :  input.  传输类型
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_IsEpStall(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_IsEpStall(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_IsEpStall(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_IsEpStall(usbc_otg->base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_EpEnterStall
*
* Description:
*    使ep进入stall状态
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type        :  input.  传输类型
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_EpSendStall(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_SendStall(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_SendStall(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_SendStall(usbc_otg->base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_EpClearStall
*
* Description:
*    清除ep的stall状态
*
* Arguments:
*    hUSB           :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type        :  input.  传输类型
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_EpClearStall(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_ClearStall(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_ClearStall(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_ClearStall(usbc_otg->base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_Ctrl_IsSetupEnd
*
* Description:
*    查询ep0是否SetupEnd
*
* Arguments:
*    hUSB  :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*
* Returns:
*
* note:
*    无
*
***********************************************************************************
*/
__u32 USBC_Dev_Ctrl_IsSetupEnd(__hdle hUSB)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return 0;
	}

    return __USBC_Dev_ep0_IsSetupEnd(usbc_otg->base_addr);
}

/*
***********************************************************************************
*                     USBC_Dev_Ctrl_ClearSetupEnd
*
* Description:
*    清除ep0的SetupEnd状态
*
* Arguments:
*    hUSB  :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*
* Returns:
*
* note:
*    无
*
***********************************************************************************
*/
void USBC_Dev_Ctrl_ClearSetupEnd(__hdle hUSB)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return ;
	}

	__USBC_Dev_ep0_ClearSetupEnd(usbc_otg->base_addr);
}


static __s32 __USBC_Dev_WriteDataHalf(ulong usbc_base_addr, __u32 ep_type)
{
    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_WriteDataHalf(usbc_base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_WriteDataHalf(usbc_base_addr);
		break;

		case USBC_EP_TYPE_RX:
			//not support
			return -1;
		//break;

		default:
			return -1;
	}

    return 0;
}

static __s32 __USBC_Dev_WriteDataComplete(ulong usbc_base_addr, __u32 ep_type)
{
    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_WriteDataComplete(usbc_base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_WriteDataComplete(usbc_base_addr);
		break;

		case USBC_EP_TYPE_RX:
			//not support
			return -1;
		//break;

		default:
			return -1;
	}

    return 0;
}

static __s32 __USBC_Dev_ReadDataHalf(ulong usbc_base_addr, __u32 ep_type)
{
    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_ReadDataHalf(usbc_base_addr);
		break;

		case USBC_EP_TYPE_TX:
			//not support
			return -1;
		//break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_ReadDataHalf(usbc_base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

static __s32 __USBC_Dev_ReadDataComplete(ulong usbc_base_addr, __u32 ep_type)
{
    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_ReadDataComplete(usbc_base_addr);
		break;

		case USBC_EP_TYPE_TX:
			//not support
			return -1;
		//break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_ReadDataComplete(usbc_base_addr);
		break;

		default:
			return -1;
	}

    return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_WriteDataStatus
*
* Description:
*    写数据的状况, 是写了一部分, 还是完全写完了
*
* Arguments:
*    hUSB      :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type   :  input.  传输类型
*    complete  :  input.  是否所有的数据都写完了
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_WriteDataStatus(__hdle hUSB, __u32 ep_type, __u32 complete)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

	if(complete){
	    return __USBC_Dev_WriteDataComplete(usbc_otg->base_addr, ep_type);
	}else{
 		return __USBC_Dev_WriteDataHalf(usbc_otg->base_addr, ep_type);
   }
}

/*
***********************************************************************************
*                     USBC_Dev_ReadDataStatus
*
* Description:
*    写数据的状况, 是写了一部分, 还是完全写完了
*
* Arguments:
*    hUSB      :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type   :  input.  传输类型
*    complete  :  input.  是否所有的数据都写完了
* Returns:
*    0  :  成功
*   !0  :  失败
*
* note:
*    无
*
***********************************************************************************
*/
__s32 USBC_Dev_ReadDataStatus(__hdle hUSB, __u32 ep_type, __u32 complete)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

	if(complete){
	    return __USBC_Dev_ReadDataComplete(usbc_otg->base_addr, ep_type);
	}else{
		return __USBC_Dev_ReadDataHalf(usbc_otg->base_addr, ep_type);
    }
}

/*
***********************************************************************************
*                     USBC_Dev_IsReadDataReady
*
* Description:
*    查询usb准备读取的数据是否准备好了
*
* Arguments:
*    hUSB     :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type  :  input.  传输类型
*
* Returns:
*
* note:
*    无
*
***********************************************************************************
*/
__u32 USBC_Dev_IsReadDataReady(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return 0;
	}

    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			return __USBC_Dev_ep0_IsReadDataReady(usbc_otg->base_addr);

		case USBC_EP_TYPE_TX:
			//not support
		break;

		case USBC_EP_TYPE_RX:
			return __USBC_Dev_Rx_IsReadDataReady(usbc_otg->base_addr);

		default:
		break;
	}

	return 0;
}

/*
***********************************************************************************
*                     USBC_Dev_IsWriteDataReady
*
* Description:
*    查询fifo是否为空
*
* Arguments:
*    hUSB    :  input.  USBC_open_otg获得的句柄, 记录了USBC所需要的一些关键数据
*    ep_type :  input.  传输类型
*
* Returns:
*
* note:
*    无
*
***********************************************************************************
*/
__u32 USBC_Dev_IsWriteDataReady(__hdle hUSB, __u32 ep_type)
{
    __usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return 0;
	}

    switch(ep_type){
		case USBC_EP_TYPE_EP0:
			return __USBC_Dev_ep0_IsWriteDataReady(usbc_otg->base_addr);

		case USBC_EP_TYPE_TX:
			return __USBC_Dev_Tx_IsWriteDataReady(usbc_otg->base_addr);

		case USBC_EP_TYPE_RX:
			//not support
		break;

		default:
		break;
	}

	return 0;
}

__u32 USBC_Dev_IsWriteDataReady_FifoEmpty(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if (usbc_otg == NULL) {
		return 0;
	}

	switch(ep_type) {
	case USBC_EP_TYPE_EP0:
		return __USBC_Dev_ep0_IsWriteDataReady(usbc_otg->base_addr);

	case USBC_EP_TYPE_TX:
		return __USBC_Dev_Tx_IsWriteDataReady_FifoEmpty(usbc_otg->base_addr);

	case USBC_EP_TYPE_RX:
		//not support
		break;

	default:
		break;
	}

	return 0;
}

/*
 * configure the device's transfer type and speed mode.
 * @hUSB:       handle return by USBC_open_otg, include the key data which USBC need
 *
 */
__s32 USBC_Dev_IsoUpdateEnable(__hdle hUSB)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return -1;
	}

	__USBC_Dev_TsType_Iso(usbc_otg->base_addr);
	return 0;
}

static void __USBC_Dev_ep0_FlushFifo(ulong usbc_base_addr)
{
	USBC_Writew(1 << USBC_BP_CSR0_D_FLUSH_FIFO, USBC_REG_CSR0(usbc_base_addr));
}

static void __USBC_Dev_Tx_FlushFifo(ulong usbc_base_addr)
{
	USBC_Writew((1 << USBC_BP_TXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_TXCSR_D_FLUSH_FIFO),
	USBC_REG_TXCSR(usbc_base_addr));
}

static void __USBC_Dev_Rx_FlushFifo(ulong usbc_base_addr)
{
	USBC_Writew((1 << USBC_BP_RXCSR_D_CLEAR_DATA_TOGGLE) | (1 << USBC_BP_RXCSR_D_FLUSH_FIFO),
	USBC_REG_RXCSR(usbc_base_addr));
}

void USBC_Dev_FlushFifo(__hdle hUSB, __u32 ep_type)
{
	__usbc_otg_t *usbc_otg = (__usbc_otg_t *)hUSB;

	if(usbc_otg == NULL){
		return ;
	}

	switch(ep_type){
		case USBC_EP_TYPE_EP0:
			__USBC_Dev_ep0_FlushFifo(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_TX:
			__USBC_Dev_Tx_FlushFifo(usbc_otg->base_addr);
		break;

		case USBC_EP_TYPE_RX:
			__USBC_Dev_Rx_FlushFifo(usbc_otg->base_addr);
		break;

		default:
		break;
	}
}
//
//EXPORT_SYMBOL(USBC_Dev_SetAddress_default);
//EXPORT_SYMBOL(USBC_Dev_SetAddress);
//
//EXPORT_SYMBOL(USBC_Dev_QueryTransferMode);
//EXPORT_SYMBOL(USBC_Dev_ConfigTransferMode);
//EXPORT_SYMBOL(USBC_Dev_ConectSwitch);
//EXPORT_SYMBOL(USBC_Dev_QueryPowerStatus);
//
//EXPORT_SYMBOL(USBC_Dev_ConfigEp);
//EXPORT_SYMBOL(USBC_Dev_ConfigEp_Default);
//EXPORT_SYMBOL(USBC_Dev_ConfigEpDma);
//EXPORT_SYMBOL(USBC_Dev_ClearEpDma);
//
//EXPORT_SYMBOL(USBC_Dev_IsEpStall);
//EXPORT_SYMBOL(USBC_Dev_EpSendStall);
//EXPORT_SYMBOL(USBC_Dev_EpClearStall);
//
//EXPORT_SYMBOL(USBC_Dev_Ctrl_IsSetupEnd);
//EXPORT_SYMBOL(USBC_Dev_Ctrl_ClearSetupEnd);
//
//EXPORT_SYMBOL(USBC_Dev_IsReadDataReady);
//EXPORT_SYMBOL(USBC_Dev_IsWriteDataReady);
//EXPORT_SYMBOL(USBC_Dev_WriteDataStatus);
//EXPORT_SYMBOL(USBC_Dev_ReadDataStatus);
//

