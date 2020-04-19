
#ifndef _LINUX_SCENELOCK_DATA_SUN8IW5P1_H
#define _LINUX_SCENELOCK_DATA_SUN8IW5P1_H

#include <linux/power/axp_depend.h>

scene_extended_standby_t extended_standby[8] = {
	{
	 {
	  .id = TALKING_STANDBY_FLAG,
	  .pwr_dm_en = 0xf7,	/* mean cpu is powered off. */
	  .osc_en = 0xf,
	  .init_pll_dis = (~(0x12)),	/* mean pll2 is on. pll5 is shundowned by dram driver. */
	  .exit_pll_en = 0x21,	/* mean enable pll1 and pll6 */
	  .pll_change = 0x0,
	  .bus_change = 0x0,
	  }
	 ,
	 .scene_type = SCENE_TALKING_STANDBY,
	 .name = "talking_standby",
	 }
	,
	{
	 {
	  .id = USB_STANDBY_FLAG,
	  .pwr_dm_en = 0xf7,	/* mean cpu is powered off. */
	  .osc_en = 0xf,	/* mean all osc is powered on. */
	  .init_pll_dis = (~(0x30)),	/* mean pll6 is on.pll5 is shundowned by dram driver. */
	  .exit_pll_en = 0x1,	/* mean enable pll1 and pll6 */
	  .pll_change = 0x20,
	  .pll_factor[5] = {0x1, 0x2, 0, 0}
	  ,			/* pll6 is 24M */
	  .bus_change = 0x14,
	  /* .bus_factor[2]  = {0x2,0x2,0x3,0,0},      //ahb1 src is 1M. */
	  /* .bus_factor[4]  = {0x2,0x0,0x0,0,0},      //axi src is 24M. */
	  .bus_factor[2] = {0, 0, 0, 0, 0}
	  ,			/* switch AHB1 src to IOSC */
	  .bus_factor[4] = {0, 0, 0, 0, 0}
	  ,			/* switch cpu/axi src to IOSC */
	  }
	 ,
	 .scene_type = SCENE_USB_STANDBY,
	 .name = "usb_standby",
	 }
	,
	{
	 {
	  .id = MP3_STANDBY_FLAG,
	  }
	 ,
	 .scene_type = SCENE_MP3_STANDBY,
	 .name = "mp3_standby",
	 }
	,
	{
	 {
	  .id = BOOT_FAST_STANDBY_FLAG,
	  }
	 ,
	 .scene_type = SCENE_BOOT_FAST,
	 .name = "boot_fast",
	 }
	,
	{
	 {
	  .id = SUPER_STANDBY_FLAG,
	  .pwr_dm_en = 0xa1,	/* mean avcc, dram, cpus is on. */
	  .osc_en = 0x4,	/*  mean losc is on. */
	  .init_pll_dis = (~(0x10)),	/* mean pll5 is shundowned by dram driver. */
	  .exit_pll_en = 0x0,
	  .pll_change = 0x0,
	  .bus_change = 0x0,
	  }
	 ,
	 .scene_type = SCENE_SUPER_STANDBY,
	 .name = "super_standby",
	 }
	,
	{
	 {
	  .id = NORMAL_STANDBY_FLAG,
	  .pwr_dm_en = 0xfff,	/* mean all power domain is on. */
	  .osc_en = 0xf,	/*  mean Hosc&Losc&ldo&ldo1 is on. */
	  .init_pll_dis = (~(0x10)),	/* mean pll5 is shundowned by dram driver. */
	  .exit_pll_en = (~(0x10)),
	  .pll_change = 0x1,
	  .pll_factor[0] = {0x10, 0, 0, 0}
	  ,
	  .bus_change = 0x5,	/* ahb1&apb2 is changed. */
	  .bus_factor[0] = {0x2, 0, 0, 0, 0}
	  ,			/* apb2 src is Hosc. */
	  .bus_factor[2] = {0x2, 0, 0, 0, 0}
	  ,			/* ahb1 src is Hosc. */
	  }
	 ,
	 .scene_type = SCENE_NORMAL_STANDBY,
	 .name = "normal_standby",
	 }
	,
	{
	 {
	  .id = GPIO_STANDBY_FLAG,
	  .pwr_dm_en = 0xb3,	/* mean avcc, dram, sys, io, cpus is on. */
	  .osc_en = 0x4,	/*  mean losc is on. */
	  .init_pll_dis = (~(0x10)),	/* mean pll5 is shundowned by dram driver. */
	  .exit_pll_en = 0x0,
	  .pll_change = 0x0,
	  .bus_change = 0x4,
	  .bus_factor[2] = {0x0, 0, 0, 0, 0}
	  ,			/* src is losc. */
	  }
	 ,
	 .scene_type = SCENE_GPIO_STANDBY,
	 .name = "gpio_standby",
	 }
	,
	{
	 {
	  .id = MISC_STANDBY_FLAG,
	  }
	 ,
	 .scene_type = SCENE_MISC_STANDBY,
	 .name = "misc_standby",
	 }
	,
};

#endif
