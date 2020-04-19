/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_i.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 17:21
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_I_H__
#define __STANDBY_I_H__

#include "../pm_types.h"
#include "../pm_i.h"

#include <linux/power/aw_pm.h>
#include <linux/power/axp_depend.h>
#include <mach/platform.h>

#include "standby_cfg.h"
#include "common.h"
#include "standby_clock.h"
#include "standby_key.h"
#include "standby_power.h"
#include "standby_usb.h"
#include "standby_twi.h"
#include "standby_ir.h"
#include "standby_arisc.h"

#ifndef CONFIG_SUNXI_ARISC
#include "dram/dram.h"
#endif

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned short *)(addr)))
#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

extern struct aw_pm_info pm_info;

#ifdef CHECK_CACHE_TLB_MISS
extern int d_cache_miss_start;
extern int d_tlb_miss_start;
extern int i_tlb_miss_start;
extern int i_cache_miss_start;
extern int d_cache_miss_end;
extern int d_tlb_miss_end;
extern int i_tlb_miss_end;
extern int i_cache_miss_end;
#endif

/* parameter for standby, it will be transfered from sys_pwm module */
extern struct aw_pm_info pm_info;
extern struct normal_standby_para normal_standby_para_info;

int standby_main(struct aw_pm_info *arg);

#endif /* __STANDBY_I_H__ */
