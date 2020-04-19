/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "pm_types.h"
#include "pm_i.h"
/*
********************************************************************************
*                                       MEM CCU INITIALISE
*
* Description: mem interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
********************************************************************************
*/
__s32 mem_ccu_save(struct ccm_state *ccm_reg)
{
	ccm_reg->ccm_reg = (__ccmu_reg_list_t *) IO_ADDRESS(AW_CCM_BASE);

	/*bias: */
	ccm_reg->ccm_reg_backup.PllAudioBias = ccm_reg->ccm_reg->PllAudioBias;
	ccm_reg->ccm_reg_backup.PllVideo0Bias = ccm_reg->ccm_reg->PllVideo0Bias;

	/*tune & pattern: */
	ccm_reg->ccm_reg_backup.PllAudioPattern =
					ccm_reg->ccm_reg->PllAudioPattern;

	/*cfg: */
	ccm_reg->ccm_reg_backup.Pll1Ctl = ccm_reg->ccm_reg->Pll1Ctl;
	ccm_reg->ccm_reg_backup.Pll2Ctl = ccm_reg->ccm_reg->Pll2Ctl;

	/*notice: be care to cpu, axi restore sequence!. */
	ccm_reg->ccm_reg_backup.SysClkDiv = ccm_reg->ccm_reg->SysClkDiv;
	ccm_reg->ccm_reg_backup.Ahb1Div = ccm_reg->ccm_reg->Ahb1Div;

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	ccm_reg->ccm_reg_backup.AhbGate0 = ccm_reg->ccm_reg->AhbGate0;
	ccm_reg->ccm_reg_backup.AhbGate1 = ccm_reg->ccm_reg->AhbGate1;
	ccm_reg->ccm_reg_backup.Apb1Gate = ccm_reg->ccm_reg->Apb1Gate;

	ccm_reg->ccm_reg_backup.Sd0 = ccm_reg->ccm_reg->Sd0;
	ccm_reg->ccm_reg_backup.Sd1 = ccm_reg->ccm_reg->Sd1;

	ccm_reg->ccm_reg_backup.I2s0 = ccm_reg->ccm_reg->I2s0;

	ccm_reg->ccm_reg_backup.SpdifClk = ccm_reg->ccm_reg->SpdifClk;
	ccm_reg->ccm_reg_backup.Usb = ccm_reg->ccm_reg->Usb;

	ccm_reg->ccm_reg_backup.Be0 = ccm_reg->ccm_reg->Be0;
	ccm_reg->ccm_reg_backup.Fe0 = ccm_reg->ccm_reg->Fe0;
	ccm_reg->ccm_reg_backup.Lcd0Ch0 = ccm_reg->ccm_reg->Lcd0Ch0;

	ccm_reg->ccm_reg_backup.Csi0 = ccm_reg->ccm_reg->Csi0;

	ccm_reg->ccm_reg_backup.PllLock = ccm_reg->ccm_reg->PllLock;
	ccm_reg->ccm_reg_backup.Pll1Lock = ccm_reg->ccm_reg->Pll1Lock;

	ccm_reg->ccm_reg_backup.AhbReset0 = ccm_reg->ccm_reg->AhbReset0;
	ccm_reg->ccm_reg_backup.AhbReset1 = ccm_reg->ccm_reg->AhbReset1;
	ccm_reg->ccm_reg_backup.Apb1Reset = ccm_reg->ccm_reg->Apb1Reset;

	return 0;
}

__s32 mem_ccu_restore(struct ccm_state *ccm_reg)
{
	/*bias: */
	ccm_reg->ccm_reg->PllAudioBias = ccm_reg->ccm_reg_backup.PllAudioBias;

	/*tune & pattern: */
	ccm_reg->ccm_reg->PllAudioPattern =
	    ccm_reg->ccm_reg_backup.PllAudioPattern;

	/*cfg */
	ccm_reg->ccm_reg->Pll2Ctl = ccm_reg->ccm_reg_backup.Pll2Ctl;
	delay_us(10);

	ccm_reg->ccm_reg->Pll1Ctl = ccm_reg->ccm_reg_backup.Pll1Ctl;
	delay_us(1000);

	/*notice: be care to cpu, axi restore sequence!. */
	ccm_reg->ccm_reg->SysClkDiv = ccm_reg->ccm_reg_backup.SysClkDiv;
	ccm_reg->ccm_reg->Ahb1Div = ccm_reg->ccm_reg_backup.Ahb1Div;
	delay_us(1);

	/*actually, the gating & reset ctrl reg
	 ** should not affect corresponding module's recovery.
	 */
	/*first, reset, then, gating. */
	ccm_reg->ccm_reg->AhbReset0 = ccm_reg->ccm_reg_backup.AhbReset0;
	ccm_reg->ccm_reg->AhbReset1 = ccm_reg->ccm_reg_backup.AhbReset1;
	ccm_reg->ccm_reg->Apb1Reset = ccm_reg->ccm_reg_backup.Apb1Reset;

	ccm_reg->ccm_reg->Sd0 = ccm_reg->ccm_reg_backup.Sd0;
	ccm_reg->ccm_reg->Sd1 = ccm_reg->ccm_reg_backup.Sd1;

	ccm_reg->ccm_reg->I2s0 = ccm_reg->ccm_reg_backup.I2s0;

	ccm_reg->ccm_reg->SpdifClk = ccm_reg->ccm_reg_backup.SpdifClk;
	ccm_reg->ccm_reg->Usb = ccm_reg->ccm_reg_backup.Usb;

	ccm_reg->ccm_reg->Be0 = ccm_reg->ccm_reg_backup.Be0;
	ccm_reg->ccm_reg->Fe0 = ccm_reg->ccm_reg_backup.Fe0;
	ccm_reg->ccm_reg->Lcd0Ch0 = ccm_reg->ccm_reg_backup.Lcd0Ch0;

	ccm_reg->ccm_reg->Csi0 = ccm_reg->ccm_reg_backup.Csi0;

	ccm_reg->ccm_reg->PllLock = ccm_reg->ccm_reg_backup.PllLock;
	ccm_reg->ccm_reg->Pll1Lock = ccm_reg->ccm_reg_backup.Pll1Lock;

	delay_us(1);
	ccm_reg->ccm_reg->AhbGate0 = ccm_reg->ccm_reg_backup.AhbGate0;
	ccm_reg->ccm_reg->AhbGate1 = ccm_reg->ccm_reg_backup.AhbGate1;
	ccm_reg->ccm_reg->Apb1Gate = ccm_reg->ccm_reg_backup.Apb1Gate;
	delay_us(10);

	return 0;
}
