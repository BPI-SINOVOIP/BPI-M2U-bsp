/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_intc.h
* By      : xulu
* Version : v1.0
* Date    : 2012-11-3 20:13
* Descript: intterupt bsp for platform mem.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __MEM_INT_H__
#define __MEM_INT_H__
#include "pm.h"

/* define interrupt source */
enum interrupt_source_e {
	INT_SOURCE_TIMER0 = AW_IRQ_TIMER0,
	INT_SOURCE_TIMER1 = AW_IRQ_TIMER1,
	INT_SOURCE_TOUCHPNL = AW_IRQ_TOUCHPANEL,
	INT_SOURCE_LRADC = AW_IRQ_LRADC,
	INT_SOURCE_EXTNMI = AW_IRQ_NMI,
	INT_SOURCE_MSG_BOX = AW_IRQ_MBOX,
	INT_SOURCE_ALARM = AW_IRQ_ALARM,
	INT_SOURCE_IR0 = AW_IRQ_IR0,
	INT_SOURCE_IR1 = AW_IRQ_IR1,
	INT_SOURCE_WLAN = AW_IRQ_WLAN,
	INT_SOURCE_USBOTG = AW_IRQ_USBOTG,
	INT_SOURCE_USBEHCI0 = AW_IRQ_USBEHCI0,
	INT_SOURCE_USBEHCI1 = AW_IRQ_USBEHCI1,
	INT_SOURCE_USBEHCI2 = AW_IRQ_USBEHCI2,
	INT_SOURCE_USBOHCI0 = AW_IRQ_USBOHCI0,
	INT_SOURCE_USBOHCI1 = AW_IRQ_USBOHCI1,
	INT_SOURCE_USBOHCI2 = AW_IRQ_USBOHCI2,
	INT_SOURCE_GPIOA = AW_IRQ_GPIOA,
	INT_SOURCE_GPIOB = AW_IRQ_GPIOB,
	INT_SOURCE_GPIOC = AW_IRQ_GPIOC,
	INT_SOURCE_GPIOD = AW_IRQ_GPIOD,
	INT_SOURCE_GPIOE = AW_IRQ_GPIOE,
	INT_SOURCE_GPIOF = AW_IRQ_GPIOF,
	INT_SOURCE_GPIOG = AW_IRQ_GPIOG,
	INT_SOURCE_GPIOH = AW_IRQ_GPIOH,
	INT_SOURCE_GPIOI = AW_IRQ_GPIOI,
	INT_SOURCE_GPIOJ = AW_IRQ_GPIOJ,
};

#define SW_INT_VECTOR_REG		(0x00)
#define SW_INT_BASE_ADR_REG	(0x04)
#define SW_NMI_INT_CTRL_REG	(0x0C)

#define SW_INT_PENDING_REG0	(0x10)
#define SW_INT_PENDING_REG1	(0x14)

#define SW_INT_ENABLE_REG0	(0x20)
#define SW_INT_ENABLE_REG1	(0x24)

#define SW_INT_MASK_REG0		(0x30)
#define SW_INT_MASK_REG1		(0x34)

#define SW_INT_RESP_REG0		(0x40)
#define SW_INT_RESP_REG1		(0x44)

#define SW_INT_FORCE_REG0		(0x50)
#define SW_INT_FORCE_REG1		(0x54)

#define SW_INT_PRIO_REG0		(0x60)
#define SW_INT_PRIO_REG1		(0x64)
#define SW_INT_PRIO_REG2		(0x68)
#define SW_INT_PRIO_REG3		(0x6C)

extern __s32 mem_int_init(void);
extern __s32 mem_int_save(void);
extern __s32 mem_int_restore(void);
extern __s32 mem_enable_int(enum interrupt_source_e src);
extern __s32 mem_query_int(enum interrupt_source_e src);

#endif	/*__MEM_INT_H__*/
