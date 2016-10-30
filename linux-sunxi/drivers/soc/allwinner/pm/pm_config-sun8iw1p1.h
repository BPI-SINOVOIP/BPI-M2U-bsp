#ifndef _PM_CONFIG_SUN8IW1P1_H
#define _PM_CONFIG_SUN8IW1P1_H

/*
* Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 2 as published by
* the Free Software Foundation.
*/

#include "pm_def_i.h"
#include "mach/platform.h"
#include "mach/memory.h"
#include "asm-generic/sizes.h"
/*#include <generated/autoconf.h>*/
#include "mach/irqs.h"

/*debug reg*/
#define STANDBY_STATUS_REG	(0xf1f00100)
#define STANDBY_STATUS_REG_PA	(0x01f00100)
#define STANDBY_STATUS_REG_NUM	(6)

/*module base reg*/
#define AW_LRADC01_BASE		(SUNXI_LRADC01_PBASE)
#define AW_CCM_BASE		(SUNXI_CCM_PBASE)
#define AW_CCU_UART_PA		(0x01c2006C)
#define AW_CCU_UART_RESET_PA	(0x01c202D8)

/*uart & jtag para*/
#define AW_JTAG_PH_GPIO_PA              (0x01c20800 + 0x100)	/*jtag0: PH9-PH12,      bitmap: 0x3, 3330 */
#define AW_JTAG_PF_GPIO_PA              (0x01c20800 + 0xB4)	/*jtag0: PF0, PF1, PF3, PF5        bitmap: 0x40, 4044; */

#define AW_UART_PH_GPIO_PA              (0x01c20800 + 0x104)	/*uart0: PH20, PH21,     bitmap: 0x22, 0000 */
#define AW_UART_PF_GPIO_PA              (0x01c20800 + 0xB4)	/*uart0: PF2, PF4,               bitmap: 0x04, 0400; */

#define AW_JTAG_PH_CONFIG_VAL_MASK      (0x000ffff0)
#define AW_JTAG_PH_CONFIG_VAL           (0x00033330)
#define AW_JTAG_PF_CONFIG_VAL_MASK      (0x00f0f0ff)
#define AW_JTAG_PF_CONFIG_VAL           (0x00404044)

#define AW_UART_PH_CONFIG_VAL_MASK      (0x00ff0000)
#define AW_UART_PH_CONFIG_VAL           (0x00220000)
#define AW_UART_PF_CONFIG_VAL_MASK      (0x000F0F00)
#define AW_UART_PF_CONFIG_VAL           (0x00040400)

#define AW_JTAG_GPIO_PA                 (AW_JTAG_PF_GPIO_PA)
#define AW_UART_GPIO_PA                 (AW_UART_PF_GPIO_PA)
#define AW_JTAG_CONFIG_VAL_MASK         AW_JTAG_PF_CONFIG_VAL_MASK
#define AW_JTAG_CONFIG_VAL              AW_JTAG_PF_CONFIG_VAL

#define AW_RTC_BASE		(SUNXI_RTC_PBASE)
#define AW_SRAMCTRL_BASE	(SUNXI_SRAMCTRL_PBASE)
#define GPIO_REG_LENGTH		((0x278+0x4)>>2)
#define CPUS_GPIO_REG_LENGTH	((0x238+0x4)>>2)
#define SRAM_REG_LENGTH		((0x94+0x4)>>2)
#define CCU_REG_LENGTH		((0x308+0x4)>>2)

/*int src*/
#define AW_IRQ_TIMER1		(SUNXI_IRQ_TIMER1)
#define AW_IRQ_TOUCHPANEL	(SUNXI_IRQ_TOUCHPANEL)
#define AW_IRQ_LRADC		(SUNXI_IRQ_LRADC)
#define AW_IRQ_NMI			(SUNXI_IRQ_NMI)
#define AW_IRQ_MBOX		(SUNXI_IRQ_MBOX)
#define AW_IRQ_WLAN             (0)

#define AW_IRQ_ALARM		(0)
#define AW_IRQ_IR0		(0)
#define AW_IRQ_IR1		(0)

#define AW_IRQ_USBOTG		(SUNXI_IRQ_USB_OTG)
#define AW_IRQ_USBEHCI0		(SUNXI_IRQ_USB_EHCI0)
#define AW_IRQ_USBOHCI0		(SUNXI_IRQ_USB_OHCI0)
#define AW_IRQ_USBEHCI1		(SUNXI_IRQ_USB_EHCI1)
#define AW_IRQ_USBOHCI1		(SUNXI_IRQ_USB_OHCI1)
#define AW_IRQ_USBOHCI2		(SUNXI_IRQ_USB_OHCI2)
#define AW_IRQ_USBEHCI2		(0)

#define AW_IRQ_GPIOA		(SUNXI_IRQ_EINTA)
#define AW_IRQ_GPIOB		(SUNXI_IRQ_EINTB)
#define AW_IRQ_GPIOC		(0)
#define AW_IRQ_GPIOD		(0)
#define AW_IRQ_GPIOE		(SUNXI_IRQ_EINTE)
#define AW_IRQ_GPIOF		(0)
#define AW_IRQ_GPIOG		(SUNXI_IRQ_EINTG)
#define AW_IRQ_GPIOH		(0)
#define AW_IRQ_GPIOI		(0)
#define AW_IRQ_GPIOJ		(0)

/**---stack point address in sram-------------*/
/*
 * notice: try not to use last 0xc bytes,
 * considering the ds-5 debugger will access (last + 0xc) bytes with unknown reason,
 * which will hangup the soc.
 **/
#define SP_IN_SRAM		0xf0003ff0	/*end of 16k */
#define SP_IN_SRAM_PA		0x00003ff0	/*end of 16k */
#define SP_IN_SRAM_START	(SRAM_FUNC_START_PA | 0x3c00)	/*15k */
#endif /*_PM_CONFIG_SUN8IW1P1_H*/
