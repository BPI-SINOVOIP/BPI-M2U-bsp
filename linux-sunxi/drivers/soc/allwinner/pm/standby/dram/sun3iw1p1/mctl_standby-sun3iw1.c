#include "mctl_reg-sun3iw1.h"
#include "mctl_hal-sun3iw1.h"
#include "mctl_para-sun3iw1.h"

void  dram_delay(unsigned n)
{
	while (n)
		n--;
}

void MASTER_CLOSE(void)
{
	DRAM_REG_MAE &= ~(0xfffc);
}

void MASTER_OPEN(void)
{
	DRAM_REG_MAE |= 0xffff;
}

void gate_dram_CLK(unsigned int set)
{
	if (set) {
		DRAM_CCM_DRAM_GATING_REG &= ~(0x1u << 31);
		DRAM_CCM_DRAM_GATING_REG |= 0x1u << 31;
	} else {
		DRAM_CCM_DRAM_GATING_REG &= ~(0x1u << 31);
	}
}


void CSP_DRAMC_enter_selfrefresh(void)
{
	/* enter dram self fresh mode */
	DRAM_REG_SCTLR |= 0x1<<1;

	/* wait dram enter self-refresh status */
	while (!(DRAM_REG_SCTLR & (0x1 << 11)))
		continue;
}

void CSP_DRAMC_exit_selfrefresh(void)
{
	/* exit selfrefresh */
	DRAM_REG_SCTLR &= ~(0x1 << 1);

	/* wait dram exit self-refresh status */
	while (DRAM_REG_SCTLR & (0x1 << 11))
		continue;
}

void dram_enable_all_master(void)
{
	MASTER_OPEN();
	dram_delay(0x1000);
}

void dram_disable_all_master(void)
{
	MASTER_CLOSE();
	dram_delay(0x1000);
}

unsigned int dram_power_save_process(void)
{
	CSP_DRAMC_enter_selfrefresh();
	dram_delay(0x1000);
	gate_dram_CLK(1);/*close dram clk*/

	return 0;
}


unsigned int dram_power_up_process(__dram_para_t *para)
{
	gate_dram_CLK(0);/*open dram clk*/
	dram_delay(0x100);
	/*exit self refesh*/
	CSP_DRAMC_exit_selfrefresh();

	return 0;
}
