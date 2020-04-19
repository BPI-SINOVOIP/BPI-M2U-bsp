/*
 * (C) Copyright 2017-2020
 *Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/gpio.h>
#include <private_boot0.h>
#include <asm/io.h>


int SPINAND_Print(const char * str, ...);

#define get_wvalue(addr)	(*((volatile unsigned long  *)(addr)))
#define put_wvalue(addr, v)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

#define SPIC0_BASE_ADDR				(0x01c05000)
#define CCMU_BASE_ADDR				(0x01c20000)
#define GPIO_BASE_ADDR				(0x01c20800)

__u32 SPINAND_GetIOBaseAddr(void)
{
	return SPIC0_BASE_ADDR;
}

__u32 Getpll6Clk(void)
{
	return 600;
}

int SPINAND_ClkRequest(__u32 nand_index)
{
	__u32 cfg;

	/*reset*/
	cfg = readl(CCMU_BASE_ADDR + 0x2c0);
	cfg &= (~((0x1<<20)|(0x1<<6)));
	writel(cfg, CCMU_BASE_ADDR + 0x2c0);

	cfg = readl(CCMU_BASE_ADDR + 0x2c0);
	cfg |= ((0x1<<20)|(0x1<<6));
	writel(cfg, CCMU_BASE_ADDR + 0x2c0);

	/*open ahb*/
	cfg = readl(CCMU_BASE_ADDR + 0x60);
	cfg |= (0x1<<20);
	writel(cfg, CCMU_BASE_ADDR + 0x60);
	printf("CCMU_BASE_ADDR + 0x2c0 0x%x\n",readl(CCMU_BASE_ADDR + 0x2c0));
	printf("CCMU_BASE_ADDR + 0x60 0x%x\n",readl(CCMU_BASE_ADDR + 0x60));

	return 0;
}

void SPINAND_ClkRelease(__u32 nand_index)
{
	return ;
}

/*
**********************************************************************************************************************
*
*             NAND_GetCmuClk
*
*  Description:
*
*
*  Parameters:
*
*
*  Return value:
*
*
**********************************************************************************************************************
*/
int SPINAND_SetClk(__u32 nand_index, __u32 nand_clock)
{
	__u32 sclk0;
	__u32 mclk = Getpll6Clk();
	__u32 div;
	__u32 cdr1 = 0;
	__u32 cdr2 = 0;
	__u32 cdr_sel = 0;

	sclk0 = nand_clock*2;

	div = mclk / (sclk0 << 1);

	if (div == 0) {
		cdr1 = 0;

		cdr2 = 0;
		cdr_sel = 0;
	} else if (div <= 0x100) {
		cdr1 = 0;

		cdr2 = div - 1;
		cdr_sel = 1;
	} else {
		div = 0;
		while (mclk > sclk0) {
			div++;
			mclk >>= 1;
		}
		cdr1 = div;

		cdr2 = 0;
		cdr_sel = 0;
	}

	writel((cdr_sel << 12) | (cdr1 << 8) | cdr2, SPIC0_BASE_ADDR+0x24);

	printf("SPIC0_BASE_ADDR+0x24: 0x%x\n", readl(SPIC0_BASE_ADDR+0x24));

	return 0;
}

int SPINAND_GetClk(__u32 nand_index)
{
	__u32 mclk;
	__u32 cfg;
	__u32 nand_max_clock;
	__u32 cdr_sel, cdr1, cdr2;

	/*set nand clock*/
	mclk = Getpll6Clk();

    /*set nand clock gate on*/
	cfg = readl(SPIC0_BASE_ADDR + 0x24);
	cdr_sel = (cfg>>12) & 0x1;
	cdr1 = (cfg>>8) & 0xf;
	cdr2 = (cfg) & 0xff;

	if(cdr_sel == 0)
		nand_max_clock = (mclk >> cdr1);
	else
		nand_max_clock = (mclk  / (2*(cdr2+1)));

	printf("SPINAND_GetClk: nand_max_clock=0x%x\n", nand_max_clock);

	return nand_max_clock;
}

void SPINAND_PIORequest(__u32 nand_index)
{
	writel(0x2222, GPIO_BASE_ADDR + 0x48);
	writel(0x55, GPIO_BASE_ADDR + 0x5c);
	writel(0x04, GPIO_BASE_ADDR + 0x64);

	printf("(GPIO_BASE_ADDR + 0x48): 0x%x\n", *(volatile __u32 *)(GPIO_BASE_ADDR + 0x48));
	printf("(GPIO_BASE_ADDR + 0x5c): 0x%x\n", *(volatile __u32 *)(GPIO_BASE_ADDR + 0x5c));
	printf("(GPIO_BASE_ADDR + 0x64): 0x%x\n", *(volatile __u32 *)(GPIO_BASE_ADDR + 0x64));


}

void SPINAND_PIORelease(__u32 nand_index)
{
	return;
}


/*
************************************************************************************************************
*
*                                             OSAL_malloc
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ： 这是一个虚假的malloc函数，目的只是提供这样一个函数，避免编译不通过
*               本身不提供任何的函数功能
*
************************************************************************************************************
*/
void* SPINAND_Malloc(unsigned int Size)
{
	return (void *)CONFIG_SYS_SDRAM_BASE;
}
#if 0
__s32 SPINAND_Print(const char * str, ...)
{
	printf(str);
	return 0;
}
#endif
