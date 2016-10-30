/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
/*
**********************************************************************************************************************
*
*						           the Embedded Secure Bootloader System
*
*
*						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include "common.h"
#include "asm/io.h"
#include "asm/arch/ccmu.h"
#include "asm/arch/timer.h"
#include "asm/arch/archdef.h"

extern int set_rtc_voltage(int set_vol);

void enable_pll_lock_bit(__u32 lock_bit)
{
	__u32 reg_val;
	reg_val = readl(CCMU_PLL_LOCK_CTRL_REG);
	reg_val |= lock_bit;
	writel(reg_val, CCMU_PLL_LOCK_CTRL_REG);

}

void disable_pll_lock_bit(__u32 lock_bit)
{
	__u32 reg_val;
	reg_val = readl(CCMU_PLL_LOCK_CTRL_REG);
	reg_val &= (~lock_bit);
	writel(reg_val, CCMU_PLL_LOCK_CTRL_REG);
}

void set_pll_cpux_axi(void)
{
	__u32 reg_val;
	//select CPUX  clock src: OSC24M,AXI divide ratio is 3, system apb clk ratio is 4
	//cpu/axi /sys apb  clock ratio
	writel((1<<16) | (3<<8) | (2<<0), CCMU_CPUX_AXI_CFG_REG);
	__usdelay(20);
    
	//set PLL_CPUX, the  default  clk is 408M  ,PLL_OUTPUT= 24M*N*K/( M*P)
	disable_pll_lock_bit(LOCK_EN_PLL_CPUX);
	writel((0x1000), CCMU_PLL_CPUX_CTRL_REG);
	enable_pll_lock_bit(LOCK_EN_PLL_CPUX);
	writel((1<<31) | readl(CCMU_PLL_CPUX_CTRL_REG), CCMU_PLL_CPUX_CTRL_REG);
	//wait PLL_CPUX stable
#ifndef FPGA_PLATFORM
	while(!(readl(CCMU_PLL_CPUX_CTRL_REG) & (0x1<<28)));
	__usdelay(20);
#endif
	disable_pll_lock_bit(LOCK_EN_PLL_CPUX);

	//set and change cpu clk src to PLL_CPUX,  PLL_CPUX:AXI0 = 408M:136M
	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &=  ~(3 << 16);
	reg_val |=  (2 << 16);    
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(1000);
}



void set_pll_periph0_ahb_apb(void)
{
	//change ahb src before set pll6
	writel((0x01 << 12) | (readl(CCMU_AHB1_APB1_CFG_REG)&(~(0x3<<12))), CCMU_AHB1_APB1_CFG_REG);

	//enable PLL6:  600M(1X)  1200M(2x)
	disable_pll_lock_bit(LOCK_EN_PLL_PERIPH0);
	writel( 0x41811, CCMU_PLL_PERIPH0_CTRL_REG);
	enable_pll_lock_bit(LOCK_EN_PLL_PERIPH0);
	writel( (1U << 31)|readl(CCMU_PLL_PERIPH0_CTRL_REG), CCMU_PLL_PERIPH0_CTRL_REG);
#ifndef FPGA_PLATFORM
	while(!(readl(CCMU_PLL_PERIPH0_CTRL_REG) & (0x1<<28)));
	__usdelay(20);
#endif
	disable_pll_lock_bit(LOCK_EN_PLL_PERIPH0);

	//set AHB1/APB1 clock  divide ratio
	//ahb1 clock src is PLL6,                           (0x03<< 12)
	//apb1 clk src is ahb1 clk src, divide  ratio is 2  (1<<8)
	//ahb1 pre divide  ratio is 2:    0:1  , 1:2,  2:3,   3:4 (2<<6)
	//PLL6:AHB1:APB1 = 600M:200M:100M ,
	writel((1<<8) | (2<<6) | (0<<4), CCMU_AHB1_APB1_CFG_REG);
	writel((0x03 << 12)|readl(CCMU_AHB1_APB1_CFG_REG), CCMU_AHB1_APB1_CFG_REG);
}
void set_pll_dma(void)
{
	//----DMA function--------
	//dma reset
	writel(readl(CCMU_BUS_SOFT_RST_REG0)  | (1 << 6), CCMU_BUS_SOFT_RST_REG0);
	__usdelay(20);
	//gating clock for dma pass
	writel(readl(CCMU_BUS_CLK_GATING_REG0) | (1 << 6), CCMU_BUS_CLK_GATING_REG0);
	//auto gating disable ---auto gating function on1680&1689 is ok,so not need disable
	//writel(7, (SUNXI_DMA_BASE+0x28));
}

void set_pll_mbus(void)
{
	//reset mbus domain
	writel(0x80000000, CCMU_MBUS_RST_REG);
	//open MBUS,clk src is pll6(2x) , pll6/(m+1) = 400M
	//writel((1<<31) | (1<<24) | (2<<0), CCMU_MBUS_CLK_REG);
	writel((2<<0), CCMU_MBUS_CLK_REG); __usdelay(1);//set MBUS div
	writel((1<<24) | (2<<0), CCMU_MBUS_CLK_REG); __usdelay(1);//set MBUS clock source
	writel((1<<31) | (1<<24) | (2<<0), CCMU_MBUS_CLK_REG); __usdelay(1);//open MBUS clock
}
void set_ce_gating(void)
{
	//note: this function for ddr read ss id

	//reset
	writel( readl(CCMU_BUS_SOFT_RST_REG0)  | (1<<5), CCMU_BUS_SOFT_RST_REG0);
	//open ce gating
	writel( readl(CCMU_BUS_CLK_GATING_REG0)  | (1<<5), CCMU_BUS_CLK_GATING_REG0);
}

void set_pll( void )
{
	__u32 reg_val;

	printf("set pll start\n");
	//pll bias select,to fix pll error
	//use PLL_CTRL_REG0
	//bit28 0: pll bias from ADDA
	//      1: pll bias from PLL inside
	reg_val = readl(SUNXI_SYSCRL_BASE+0xF0);
	reg_val |= (1<<28);
	writel(reg_val, SUNXI_SYSCRL_BASE+0xF0);
	__usdelay(10);

	//use new mode
	enable_pll_lock_bit(LOCK_EN_NEW_MODE);

	set_pll_cpux_axi();
	set_pll_periph0_ahb_apb();
	set_pll_dma();
	set_pll_mbus();
	set_ce_gating();

	disable_pll_lock_bit(LOCK_EN_NEW_MODE);
	printf("set pll end\n");

	if((readl(0x01c15000+0x4)&0x7) != 0x1)
	{
		//0x1:V40 other:A20E
		set_rtc_voltage(0);
	}
	return ;
}


void reset_pll( void )
{
	printf("reset pll start\n");
	writel(0x10300, CCMU_CPUX_AXI_CFG_REG);
	__msdelay(10);

	writel(0x01010, CCMU_AHB1_APB1_CFG_REG);
	__msdelay(10);

	writel(0x00001000, CCMU_PLL_CPUX_CTRL_REG);
	__msdelay(10);

	writel(0x00041811, CCMU_PLL_PERIPH0_CTRL_REG);
	__msdelay(10);
	printf("reset pll end\n");

	return ;
}

void set_gpio_gate(void)
{

}

