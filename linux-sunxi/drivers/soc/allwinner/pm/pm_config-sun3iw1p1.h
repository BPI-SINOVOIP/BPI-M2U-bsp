#ifndef __PM_CONFIG_SUN3IW1P1_H_
#define __PM_CONFIG_SUN3IW1P1_H_

/*
* Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published by
* the Free Software Foundation.
*/

#include "pm_def_i.h"
#include "asm-generic/sizes.h"
/*#include <generated/autoconf.h>*/

#define SUNXI_SRAM_A1_PBASE		(0x00000000)	/* size 8K */
#define SUNXI_SRAM_C1_PBASE		(0x00004000)	/* size 12K */
#define SUNXI_SRAM_C2_PBASE		(0x00002000)	/* size 8K */

#define SUNXI_RTC_PBASE			(0x01c20400)
#define SUNXI_SRAMCTRL_PBASE		(0x01c00000)
#define SUNXI_DMA_PBASE			(0x01c02000)
#define SUNXI_USB0_PBASE		(0x01c13000)
#define SUNXI_CCM_PBASE			(0x01c20000)
#define SUNXI_INTC_PBASE		(0x01c20400)
#define SUNXI_PIO_PBASE			(0x01c20800)
#define SUNXI_TMR_PBASE			(0x01c20c00)
#define SUNXI_IR_PBASE			(0x01c22c00)
#define SUNXI_LRADC_PBASE		(0x01C23400)
#define SUNXI_UART0_PBASE		(0x01c25000)
#define SUNXI_UART1_PBASE		(0x01c25400)
#define SUNXI_UART2_PBASE		(0x01c25800)
#define SUNXI_TWI0_PBASE		(0x01c27000)
#define SUNXI_TWI1_PBASE		(0x01c27400)
#define SUNXI_TWI2_PBASE		(0x01c27800)

#define SUNXI_IRQ_IR0		(6)
#define SUNXI_IRQ_TIMER0	(13)
#define SUNXI_IRQ_TIMER1	(14)
#define SUNXI_IRQ_TOUCHPANEL	(20)
#define SUNXI_IRQ_LRADC		(22)
#define SUNXI_IRQ_USBOTG	(26)
#define SUNXI_IRQ_EINTD		(38)
#define SUNXI_IRQ_EINTE		(39)
#define SUNXI_IRQ_EINTF		(40)
#define SUNXI_BANK_SIZE 32
#define SUNXI_PA_BASE	0
#define SUNXI_PB_BASE	32
#define SUNXI_PC_BASE	64
#define SUNXI_PD_BASE	96
#define SUNXI_PE_BASE	128
#define SUNXI_PF_BASE	160

/*debug reg, fake addr, this is INTC's addr. INTC base: 0x01c20400, size: 0x6c*/
#define STANDBY_STATUS_REG	(0xf1c20500)
#define STANDBY_STATUS_REG_PA	(0x01c20500)
#define STANDBY_STATUS_REG_NUM	(4)
#define STANDBY_SUPER_FLAG_REG	(0xf1c205f8)
#define STANDBY_SUPER_ADDR_REG	(0xf1c205e4)

/*module base reg*/
#define AW_LRADC01_BASE		(SUNXI_LRADC_PBASE)
#define AW_CCM_BASE		(0x01c20000)
#define AW_CCM_MOD_BASE		(0x01c20000)
/*uart0 gating: bit20, 0: mask, 1: pass */
#define AW_CCU_UART_PA		(0x01c20068)
/*uart0 reset: bit20, 0: reset, 1: de_assert */
#define AW_CCU_UART_RESET_PA	(0x01c202D0)
#define AW_UART1_PBASE		(0x01c25400)

#define AW_UART_PA_GPIO_PA	(0x01c20800 + 0x00)	/*uart1: use pa */
#define AW_UART_GPIO_PA		(AW_UART_PA_GPIO_PA)
#define AW_RTC_BASE		(SUNXI_RTC_PBASE)
#define AW_SRAMCTRL_BASE	(SUNXI_SRAMCTRL_PBASE)
#define GPIO_REG_LENGTH		((0x2C4+0x4)>>2)
#define SRAM_REG_LENGTH		((0xF0+0x4)>>2)
#define CCU_REG_LENGTH		((0x2d0+0x4)>>2)
/*int src no.*/
#define AW_IRQ_TIMER1		(SUNXI_IRQ_TIMER1)
#define AW_IRQ_TOUCHPANEL	(SUNXI_IRQ_TOUCHPANEL)
#define AW_IRQ_LRADC		(SUNXI_IRQ_LRADC)
#define AW_IRQ_NMI		(0)
#define AW_IRQ_MBOX		(0)
#define AW_IRQ_WLAN		(0)

#define AW_IRQ_ALARM		(0)
#define AW_IRQ_IR0		(SUNXI_IRQ_IR0)
#define AW_IRQ_IR1		(0)
#define AW_IRQ_USBOTG		(SUNXI_IRQ_USBOTG)
#define AW_IRQ_USBEHCI0		(0)
#define AW_IRQ_USBEHCI1		(0)
#define AW_IRQ_USBEHCI2		(0)
#define AW_IRQ_USBOHCI0		(0)
#define AW_IRQ_USBOHCI1		(0)
#define AW_IRQ_USBOHCI2		(0)

#define AW_IRQ_GPIOA		(0)
#define AW_IRQ_GPIOB		(0)
#define AW_IRQ_GPIOC		(0)
#define AW_IRQ_GPIOD		(SUNXI_IRQ_EINTD)
#define AW_IRQ_GPIOE		(SUNXI_IRQ_EINTE)
#define AW_IRQ_GPIOF		(SUNXI_IRQ_EINTF)
#define AW_IRQ_GPIOG		(0)
#define AW_IRQ_GPIOH		(0)
#define AW_IRQ_GPIOI		(0)
#define AW_IRQ_GPIOJ		(0)
/**start address for function run in sram*/
#define SRAM_FUNC_START		(0xf0000000)
#define SRAM_FUNC_START_PA	(0x00000000)
/*for mem mapping*/
#define MEM_SW_VA_SRAM_BASE	(0x00000000)
#define MEM_SW_PA_SRAM_BASE	(0x00000000)
#define SP_IN_SRAM		0xf0001ff0	/*end of 8k */
#define SP_IN_SRAM_PA		0x00001ff0	/*end of 8k */
#endif
