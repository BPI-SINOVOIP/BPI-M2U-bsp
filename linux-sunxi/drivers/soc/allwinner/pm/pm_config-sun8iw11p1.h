#ifndef _PM_CONFIG_SUN8IW11P1_H
#define _PM_CONFIG_SUN8IW11P1_H

/*
* Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published by
* the Free Software Foundation.
*/

#include "pm_def_i.h"
#include "asm-generic/sizes.h"


#define SUNXI_SRAM_A1_PBASE			(0x00000000)
#define SUNXI_SRAM_A2_PBASE			(0x00004000)
#define SUNXI_SRAM_A3_PBASE			(0x00008000)
#define SUNXI_PIO_PBASE				(0x01c20800)
#define SUNXI_R_PRCM_PBASE			(0)
#define SUNXI_MSGBOX_PBASE			(0)
#define SUNXI_SPINLOCK_PBASE		(0)
#define SUNXI_R_PIO_PBASE			(0)
#define SUNXI_R_CPUCFG_PBASE		(0)
#define SUNXI_UART0_PBASE			(0x01c28000)
#define SUNXI_TWI0_PBASE			(0x01c2ac00)
#define SUNXI_TWI1_PBASE			(0x01c2b000)
#define SUNXI_TWI2_PBASE			(0x01c2b400)
#define SUNXI_CPUCFG_P_REG0			(0)
#define SUNXI_CPUCFG_GENCTL			(0)
#define SUNXI_CPUX_PWR_CLAMP(x)			(0)
#define SUNXI_CPUX_PWR_CLAMP_STATUS(x)	(0)
#define SUNXI_CPU_PWROFF_REG			(0)
#define SUNXI_RTC_PBASE				(0x01c20400)
#define SUNXI_SRAMCTRL_PBASE        (0x01c00000)
#define SUNXI_LRADC_PBASE           (0x01c24400)
#define SUNXI_GIC_DIST_PBASE		(0)
#define SUNXI_GIC_CPU_PBASE			(0)
#define SUNXI_TIMER_PBASE			(0)
#define SUNXI_CCM_PBASE				(0)

#ifdef CONFIG_FPGA_V4_PLATFORM
#define SUNXI_IRQ_TIMER0      (38)
#else
#define SUNXI_IRQ_TIMER0      (54)
#endif
#define SUNXI_IRQ_TIMER1      (55)
#define SUNXI_IRQ_LRADC       (62)
#define SUNXI_IRQ_NMI         (32)
#define SUNXI_IRQ_ALARM0      (56)
#define SUNXI_IRQ_USBOTG      (70)
#define SUNXI_IRQ_USBEHCI0    (71)
#define SUNXI_IRQ_USBOHCI0    (72)
#define SUNXI_IRQ_USBEHCI1    (96)
#define SUNXI_IRQ_USBOHCI1    (108)
#define SUNXI_IRQ_EINTA       (0)
#define SUNXI_IRQ_EINTB       (0)
#define SUNXI_IRQ_EINTD       (0)
#define SUNXI_IRQ_EINTE       (0)
#define SUNXI_IRQ_EINTF       (0)
#define SUNXI_IRQ_EINTG       (0)
#define SUNXI_IRQ_EINTH       (60)
#define SUNXI_IRQ_EINTI       (60)
#define SUNXI_IRQ_MBOX        (0)
#define SUNXI_IRQ_WLAN        (0)

#define SUNXI_BANK_SIZE 32
#define SUNXI_PA_BASE	0
#define SUNXI_PB_BASE	32
#define SUNXI_PC_BASE	64
#define SUNXI_PD_BASE	96
#define SUNXI_PE_BASE	128
#define SUNXI_PF_BASE	160
#define SUNXI_PG_BASE	192
#define SUNXI_PH_BASE	224
#define SUNXI_PI_BASE	256
#define SUNXI_PJ_BASE	288
#define SUNXI_PK_BASE	320
#define SUNXI_PL_BASE	352
#define SUNXI_PM_BASE	384
#define SUNXI_PN_BASE	416
#define SUNXI_PO_BASE	448
#define AXP_PIN_BASE	1024

/*debug reg*/
#define STANDBY_STATUS_REG		(0xf1c20500)
#define STANDBY_STATUS_REG_PA	(0x01c20500)
#define STANDBY_STATUS_REG_NUM	(4)
#define STANDBY_SUPER_FLAG_REG  (0xf1c205f8)
#define STANDBY_SUPER_ADDR_REG  (0xf1c205fc)

/*module base reg*/
#define AW_LRADC01_BASE						(SUNXI_LRADC_PBASE)
#define AW_CCM_BASE							(0x01c20000)
#define AW_CCM_MOD_BASE						(0x01c20000)
#define AW_CCM_PIO_BUS_GATE_REG_OFFSET		(0x68)
#define AW_CCU_UART_PA						(0x01c2006C)	/*uart0 gating: bit16, 0: mask, 1: pass */
#define AW_CCU_UART_RESET_PA				(0x01c202D8)	/*uart0 reset: bit16, 0: reset, 1: de_assert */
#define AW_UART0_PBASE					(0x01c28000)

/*uart & jtag para*/
#define AW_JTAG_PH_GPIO_PA              (0x01c20800 + 0x28)	/*jtag0: Pb14-Pb17, */
#define AW_JTAG_PF_GPIO_PA              (0x01c20800 + 0xB4)	/*jtag0: PF0, PF1, PF3, PF5        bitmap: 0x40, 4044; */

#define AW_UART_PH_GPIO_PA              (0x01c20800 + 0x28)	/*uart0: use PB */
#define AW_UART_PF_GPIO_PA              (0x01c20800 + 0xB4)	/*uart0: PF2, PF4,               bitmap: 0x04, 0400; */

#define AW_JTAG_PH_CONFIG_VAL_MASK      (0x0000ffff)
#define AW_JTAG_PH_CONFIG_VAL           (0x00003333)
#define AW_JTAG_PF_CONFIG_VAL_MASK      (0x00f0f0ff)
#define AW_JTAG_PF_CONFIG_VAL           (0x00303033)

#define AW_UART_PH_CONFIG_VAL_MASK      (0x000F0F00)
#define AW_UART_PH_CONFIG_VAL           (0x00030300)
#define AW_UART_PF_CONFIG_VAL_MASK      (0x000F0F00)
#define AW_UART_PF_CONFIG_VAL           (0x00030300)

#define AW_JTAG_GPIO_PA                 (AW_JTAG_PF_GPIO_PA)
#define AW_UART_GPIO_PA                 (AW_UART_PF_GPIO_PA)
#define AW_JTAG_CONFIG_VAL_MASK         AW_JTAG_PF_CONFIG_VAL_MASK
#define AW_JTAG_CONFIG_VAL              AW_JTAG_PF_CONFIG_VAL

#define AW_RTC_BASE		(SUNXI_RTC_PBASE)
#define AW_SRAMCTRL_BASE	(SUNXI_SRAMCTRL_PBASE)
#define GPIO_REG_LENGTH		((0x278+0x4)>>2)
#define CPUS_GPIO_REG_LENGTH	((0x218+0x4)>>2)
#define SRAM_REG_LENGTH		((0xF4+0x4)>>2)
#define CCU_REG_LENGTH		((0x2d8+0x4)>>2)

/*int src no.*/
#define AW_IRQ_TIMER1		(SUNXI_IRQ_TIMER1)
#define AW_IRQ_TOUCHPANEL	(0)
#define AW_IRQ_LRADC		(SUNXI_IRQ_LRADC)
#define AW_IRQ_NMI			(SUNXI_IRQ_NMI)
#define AW_IRQ_MBOX			(SUNXI_IRQ_MBOX)
#define AW_IRQ_WLAN			(SUNXI_IRQ_WLAN)

#define AW_IRQ_ALARM		(SUNXI_IRQ_ALARM0)
#define AW_IRQ_IR0			(0)
#define AW_IRQ_IR1			(0)
#define AW_IRQ_USBOTG		(SUNXI_IRQ_USBOTG)
#define AW_IRQ_USBEHCI0		(SUNXI_IRQ_USBEHCI0)
#define AW_IRQ_USBEHCI1		(SUNXI_IRQ_USBEHCI1)
#define AW_IRQ_USBEHCI2		(0)
#define AW_IRQ_USBOHCI0		(SUNXI_IRQ_USBOHCI0)
#define AW_IRQ_USBOHCI1		(SUNXI_IRQ_USBOHCI1)
#define AW_IRQ_USBOHCI2		(0)

#define AW_IRQ_GPIOA		(0)
#define AW_IRQ_GPIOB		(0)
#define AW_IRQ_GPIOC		(0)
#define AW_IRQ_GPIOD		(0)
#define AW_IRQ_GPIOE		(0)
#define AW_IRQ_GPIOF		(0)
#define AW_IRQ_GPIOG		(0)
#define AW_IRQ_GPIOH		(SUNXI_IRQ_EINTH)
#define AW_IRQ_GPIOI		(SUNXI_IRQ_EINTI)
#define AW_IRQ_GPIOJ		(0)

/**start address for function run in sram*/
#define SRAM_FUNC_START		(0xf0000000)
#define SRAM_FUNC_START_PA	(0x00000000)
/*for mem mapping*/
#define MEM_SW_VA_SRAM_BASE	(0x00000000)
#define MEM_SW_PA_SRAM_BASE	(0x00000000)
/*dram area*/
#define DRAM_BASE_ADDR_PA	(0x40000000)
#define DRAM_TRANING_SIZE	(16)

#define CPU_CLK_REST_DEFAULT_VAL	(0x00010000)	/*SRC is HOSC. */

/**---stack point address in sram-------------*/
/*
 * notice: try not to use last 0xc bytes,
 * considering the ds-5 debugger will access (last + 0xc) bytes with unknown reason,
 * which will hangup the soc.
 **/
#define SP_IN_SRAM             0xf000bff0	/*end of 48k */
#define SP_IN_SRAM_PA          0x0000bff0	/*end of 48k */
#define SP_IN_SRAM_START	(SRAM_FUNC_START_PA | 0x3c00)	/*15k */
#endif /*_PM_CONFIG_SUN8IW11P1_H*/
