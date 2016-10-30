/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
/*
 * COM1 NS16550 support
 * originally from linux source (arch/powerpc/boot/ns16550.c)
 * modified to use CONFIG_SYS_ISA_MEM and new defines
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/uart.h>
#include <asm/arch/gpio.h>
#include <asm/arch/ccmu.h>

#define thr rbr
#define dll rbr
#define dlh ier
#define iir fcr


static serial_hw_t *serial_ctrl_base = NULL;



/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_serial_init(int uart_port, void *gpio_cfg, int gpio_max)
{
	u32 reg, i;
	u32 uart_clk;
#ifdef FPGA_PLATFORM
	normal_gpio_set_t fpga_uart_gpio[2] =
	{
	//	{ 1, 5, 2, 1, 1, 0, {0}},//PA5: 2--RX
	//	{ 1, 6, 2, 1, 1, 0, {0}},//PA6: 2--TX

		{ 6, 2, 3, 1, 1, 0, {0}},//PF2: 3--RX
		{ 6, 4, 3, 1, 1, 0, {0}},//PF4: 3--TX
	};
#endif

	if( (uart_port < 0) ||(uart_port > 0) )
	{
		return;
	}
	//reset
	reg = readl(CCMU_UART_BGR_REG);
	reg &= ~(1<<(CCM_UART_RST_OFFSET + uart_port));
	writel(reg, CCMU_UART_BGR_REG);
	for( i = 0; i < 100; i++ );
	reg |=  (1<<(CCM_UART_RST_OFFSET + uart_port));
	writel(reg, CCMU_UART_BGR_REG);
	//gate
	reg = readl(CCMU_UART_BGR_REG);
	reg &= ~(1<<(CCM_UART_GATING_OFFSET + uart_port));
	writel(reg, CCMU_UART_BGR_REG);
	for( i = 0; i < 100; i++ );
	reg |=  (1<<(CCM_UART_GATING_OFFSET + uart_port));
	writel(reg, CCMU_UART_BGR_REG);
	//gpio
#ifdef FPGA_PLATFORM
	boot_set_gpio(fpga_uart_gpio, gpio_max, 1);
#else
	boot_set_gpio(gpio_cfg, gpio_max, 1);
#endif
	//uart init
	serial_ctrl_base = (serial_hw_t *)(SUNXI_UART0_BASE + uart_port * CCM_UART_ADDR_OFFSET);

	serial_ctrl_base->mcr = 0x3;
	uart_clk = (24000000 + 8 * UART_BAUD)/(16 * UART_BAUD);
	serial_ctrl_base->lcr |= 0x80;
	serial_ctrl_base->dlh = uart_clk>>8;
	serial_ctrl_base->dll = uart_clk&0xff;
	serial_ctrl_base->lcr &= ~0x80;
	serial_ctrl_base->lcr = ((PARITY&0x03)<<3) | ((STOP&0x01)<<2) | (DLEN&0x03);
	serial_ctrl_base->fcr = 0x7;

	return;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_serial_putc (char c)
{
	while((serial_ctrl_base->lsr & ( 1 << 6 )) == 0);
	serial_ctrl_base->thr = c;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
char sunxi_serial_getc (void)
{
	while((serial_ctrl_base->lsr & 1) == 0);
	return serial_ctrl_base->rbr;

}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_serial_tstc (void)
{
	return serial_ctrl_base->lsr & 1;
}

