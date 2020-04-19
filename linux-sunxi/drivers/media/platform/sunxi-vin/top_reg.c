/*
 ******************************************************************************
 *
 * top_reg.c
 *
 * CSIC - top_reg.c module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    	Description
 *
 *   1.0		  Zhao Wei   	2016/06/13	      First Version
 *
 ******************************************************************************
 */

#include <linux/kernel.h>
#include "top_reg_i.h"
#include "top_reg.h"

#include "utility/vin_io.h"

/*isp_id isp_input pasrer_id parser_ch*/
static int isp_input[4][4][4][4] = {
#if defined(CONFIG_ARCH_SUN50IW3P1) || defined(CONFIG_ARCH_SUN50IW6P1)

	/*isp0 input0~3*/
		/*parser0*/     /*parse1*/    /*parser2*/    /*parser3*/
	{
		{{0, 0, 0, 0}, {1, 2, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0} }
	},

	/*isp1 input0~3*/
	{
		{{1, 2, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {0, 0, 0, 0} }
	},
/*
	{
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
	},

	{
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}
	}
*/
#else
	/*isp0 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp1 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp2 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	},

	/*isp3 input0~3*/
	{
		{{0, 4, 0, 0}, {1, 5, 0, 0}, {2, 6, 0, 0}, {3, 7, 0, 0} },
		{{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 2, 0, 0}, {0, 3, 0, 0} },
		{{0, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 2, 0}, {0, 0, 3, 0} },
		{{0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 2}, {0, 0, 0, 3} }
	}
#endif
};

/*vipp_id isp_id isp_ch*/
static int vipp_input[8][4][4] = {
#if defined(CONFIG_ARCH_SUN50IW3P1) || defined(CONFIG_ARCH_SUN50IW6P1)
	/*vipp0*/
	/*isp0*/        /*isp1*/      /*isp2*/       /*isp3*/
	{{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 2, 0, 0}, {1, 3, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 2, 0}, {1, 0, 3, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 2}, {1, 4, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0} },
/*
	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
*/
#else
	{{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 2, 0, 0}, {1, 3, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 2, 0}, {1, 0, 3, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 2}, {1, 4, 0, 3}, {0, 0, 0, 0}, {0, 0, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {1, 0, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 2, 0, 0}, {1, 3, 0, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 2, 0}, {1, 0, 3, 0} },

	{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 2}, {1, 4, 0, 3} }
#endif
};

volatile void __iomem *csic_top_base[MAX_CSIC_TOP_NUM];

int csic_top_set_base_addr(unsigned int sel, unsigned long addr)
{
	if (sel > MAX_CSIC_TOP_NUM - 1)
		return -1;
	csic_top_base[sel] = (volatile void __iomem *)addr;

	return 0;
}

void csic_top_enable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_TOP_EN_MASK, 1 << CSIC_TOP_EN);
}

void csic_top_disable(unsigned int sel)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_TOP_EN_MASK, 0 << CSIC_TOP_EN);
}

void csic_top_wdr_mode(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_WDR_MODE_EN_MASK, en << CSIC_WDR_MODE_EN);
}

void csic_top_sram_pwdn(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_SRAM_PWDN_MASK, en << CSIC_SRAM_PWDN);
}

void csic_top_switch_ncsi(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_NCSI_TOP_SWITCH_MASK, en << CSIC_NCSI_TOP_SWITCH);
}

void csic_top_version_read_en(unsigned int sel, unsigned int en)
{
	vin_reg_clr_set(csic_top_base[sel] + CSIC_TOP_EN_REG_OFF,
			CSIC_VER_EN_MASK, en << CSIC_VER_EN);
}

void csic_isp_input_select(unsigned int sel, unsigned int isp, unsigned int in,
				unsigned int psr, unsigned int ch)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_ISP0_IN0_REG_OFF + isp * 16 + in * 4,
			isp_input[isp][in][psr][ch]);
}

void csic_vipp_input_select(unsigned int sel, unsigned int vipp,
				unsigned int isp, unsigned int ch)
{
	vin_reg_writel(csic_top_base[sel] + CSIC_VIPP0_IN_REG_OFF + vipp * 4,
			vipp_input[vipp][isp][ch]);
}

void csic_feature_list_get(unsigned int sel, struct csic_feature_list *fl)
{
	unsigned int reg_val = 0;
	reg_val = vin_reg_readl(csic_top_base[sel] + CSIC_FEATURE_REG_OFF);
	fl->dma_num = (reg_val & CSIC_DMA_NUM_MASK) >> CSIC_DMA_NUM;
	fl->vipp_num = (reg_val & CSIC_VIPP_NUM_MASK) >> CSIC_VIPP_NUM;
	fl->isp_num = (reg_val & CSIC_ISP_NUM_MASK) >> CSIC_ISP_NUM;
	fl->ncsi_num = (reg_val & CSIC_NCSI_NUM_MASK) >> CSIC_NCSI_NUM;
	fl->mcsi_num = (reg_val & CSIC_MCSI_NUM_MASK) >> CSIC_MCSI_NUM;
	fl->parser_num = (reg_val & CSIC_PARSER_NUM_MASK) >> CSIC_PARSER_NUM;
}

void csic_version_get(unsigned int sel, struct csic_version *v)
{
	unsigned int reg_val = 0;
	reg_val = vin_reg_readl(csic_top_base[sel] + CSIC_VER_REG_OFF);
	v->ver_small = (reg_val & CSIC_VER_SMALL_MASK) >> CSIC_VER_SMALL;
	v->ver_big = (reg_val & CSIC_VER_BIG_MASK) >> CSIC_VER_BIG;
}

