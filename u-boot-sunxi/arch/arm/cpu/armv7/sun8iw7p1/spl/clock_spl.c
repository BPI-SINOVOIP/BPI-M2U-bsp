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
#include "asm/armv7.h"
#include "asm/arch/platform.h"
#include "asm/arch/ccmu.h"
#include "asm/arch/timer.h"
#include "asm/arch/archdef.h"

/*
************************************************************************************************************
*
*                                             function
*
*�������� ��
*
*�����б� ��
*
*����ֵ   ��
*
*˵��     ��
*
*
************************************************************************************************************
*/
static int clk_set_divd(void)
{
	unsigned int reg_val;

	/*config axi*/
	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &= ~(0x03 << 8);
	reg_val |=  (0x01 << 8);
	reg_val |=  (0x01 << 0);
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);

	/*config ahb*/
	reg_val = readl(CCMU_AHB1_APB1_CFG_REG);
	reg_val &= ~((0x03 << 12) | (0x03 << 8) | (0x03 << 4));
	reg_val |=  (0x02 << 12);
	reg_val |=  (1 << 4);
	reg_val |=  (1 << 8);

	writel(reg_val, CCMU_AHB1_APB1_CFG_REG);

	return 0;
}
/*******************************************************************************
*��������: set_pll
*����ԭ�ͣ�void set_pll( void )
*��������: ����CPUƵ��
*��ڲ���: void
*�� �� ֵ: void
*��    ע:
*******************************************************************************/
void set_pll(void)
{
    unsigned int reg_val;
    unsigned int i;
    /*cofnig cpu_pll=408M*/

    /*config 24M*/
    __msdelay(300);
    reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
    reg_val &= ~((0x03 << 16) | (0x3 << 0));
    reg_val |=  (0x01 << 16);
	reg_val |=  (0x01 << 0);
    writel(reg_val, CCMU_CPUX_AXI_CFG_REG);

	for (i = 0; i < 0x400; i++) {
		;
	}
    reg_val = (0x10 << 8) | (0x01 << 4) | (0x01 << 31);
    writel(reg_val, CCMU_PLL_C0CPUX_CTRL_REG);

#ifndef CONFIG_FPGA
	do {
		reg_val = readl(CCMU_PLL_C0CPUX_CTRL_REG);
	} while (!(reg_val & (0x1 << 28)));
#endif
    clk_set_divd();
		/* dma reset */
	writel(readl(CCMU_BUS_SOFT_RST_REG0)  | (1 << 6), CCMU_BUS_SOFT_RST_REG0);
	for (i = 0; i < 100; i++) {
		;
	}
	/* gating clock for dma pass */
	writel(readl(CCMU_BUS_CLK_GATING_REG0) | (1 << 6), CCMU_BUS_CLK_GATING_REG0);
	writel(7, (0x01c20000+0x20));
#if 0
	/* ��MBUS,clk src is pll6 */
	writel(0x80000000, CCM_MBUS_RESET_CTRL);       /* Assert mbus domain */
	writel(0x81000002, CCM_MBUS_SCLK_CTRL0);  /* dram>600M, so mbus from 300M->400M */
	/* ʹ��PLL6 */
	writel(readl(CCM_PLL6_MOD_CTRL) | (1U << 31), CCM_PLL6_MOD_CTRL);
#endif
	/*ʹ��PLL6*/
	writel(readl(CCMU_PLL_PERIPH0_CTRL_REG) | (1U << 31), CCMU_PLL_PERIPH0_CTRL_REG);
	__usdelay(100);

	writel(0x00000002, CCMU_MBUS_CLK_REG);
	__usdelay(1);/* ����MBUS�ķ�Ƶ���� */
	writel(0x01000002, CCMU_MBUS_CLK_REG);
	__usdelay(1);/* ѡ��MBUS��Դͷ */
	writel(0x81000002, CCMU_MBUS_CLK_REG);
	__usdelay(1);/* ��MBUSʱ�� */

    reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
    reg_val &= ~(0x03 << 16);
    reg_val |=  (0x02 << 16);
    writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(1000);
	CP15DMB;
	CP15ISB;
	/* ��GPIO */
	writel(readl(CCMU_BUS_CLK_GATING_REG2)		|	(1 << 5), CCMU_BUS_CLK_GATING_REG2);
	writel(readl(SUNXI_RPRCM_BASE + 0x28)   | 		0x01, SUNXI_RPRCM_BASE + 0x28);
    return  ;
}

/*******************************************************************************
*��������: set_pll_in_secure_mode
*����ԭ�ͣ�void set_pll_in_secure_mode( void )
*��������: ����CPUƵ��
*��ڲ���: void
*�� �� ֵ: void
*��    ע:
*******************************************************************************/
void set_pll_in_secure_mode(void)
{
    set_pll();
}

/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
void reset_pll(void)
{
	writel(0x00010000, CCMU_CPUX_AXI_CFG_REG);
	writel(0x00001000, CCMU_PLL_C0CPUX_CTRL_REG);
	writel(0x00001010, CCMU_AHB1_APB1_CFG_REG);

	return ;
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
void set_gpio_gate(void)
{
	writel(readl(CCMU_BUS_CLK_GATING_REG2)		|	(1 << 5), CCMU_BUS_CLK_GATING_REG2);
	writel(readl(SUNXI_RPRCM_BASE + 0x28)	|		0x01, SUNXI_RPRCM_BASE + 0x28);
}
/*
************************************************************************************************************
*
*                                             function
*
*    �������ƣ�
*
*    �����б�
*
*    ����ֵ  ��
*
*    ˵��    ��
*
*
************************************************************************************************************
*/
void set_ccmu_normal(void)
{
	writel(7, CCMU_SEC_SWITCH_REG);
	writel(0xfff, SUNXI_DMA_BASE + 0x20);
}

