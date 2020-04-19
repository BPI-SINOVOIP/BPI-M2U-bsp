/*
 *  arch/arm/mach-sun6i/arisc/arisc_dram.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "arisc_i.h"
#include <asm/cacheflush.h>

#if defined CONFIG_ARCH_SUN8IW17P1

#else
typedef struct __DRAM_PARA {
	/* normal configuration */
	unsigned int dram_clk;
	/* dram_type DDR2: 2 DDR3: 3 LPDDR2: 6 DDR3L: 31 */
	unsigned int dram_type;
	unsigned int dram_zq;
	unsigned int dram_odt_en;

	/* control configuration */
	unsigned int dram_para1;
	unsigned int dram_para2;

	/* timing configuration */
	unsigned int dram_mr0;
	unsigned int dram_mr1;
	unsigned int dram_mr2;
	unsigned int dram_mr3;
	unsigned int dram_tpr0;
	unsigned int dram_tpr1;
	unsigned int dram_tpr2;
	unsigned int dram_tpr3;
	unsigned int dram_tpr4;
	unsigned int dram_tpr5;
	unsigned int dram_tpr6;

	/* reserved for future use */
	unsigned int dram_tpr7;
	unsigned int dram_tpr8;
	unsigned int dram_tpr9;
	unsigned int dram_tpr10;
	unsigned int dram_tpr11;
	unsigned int dram_tpr12;
	unsigned int dram_tpr13;

} __dram_para_t;
#endif

static __dram_para_t arisc_dram_paras;

static int arisc_get_dram_cfg(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "allwinner,dram");
	if (!np) {
		ARISC_ERR("No allwinner,dram node found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "dram_clk", &arisc_dram_paras.dram_clk))
		ARISC_ERR("get dram_clk failed\n");

	if (of_property_read_u32(np, "dram_type", &arisc_dram_paras.dram_type))
		ARISC_ERR("get dram_type failed\n");

	if (of_property_read_u32(np, "dram_zq", &arisc_dram_paras.dram_zq))
		ARISC_ERR("get dram_zq failed\n");

	if (of_property_read_u32(np, "dram_odt_en", &arisc_dram_paras.dram_odt_en))
		ARISC_ERR("get dram_odt_en failed\n");

	if (of_property_read_u32(np, "dram_para1", &arisc_dram_paras.dram_para1))
		ARISC_ERR("get dram_para1 failed\n");

	if (of_property_read_u32(np, "dram_para2", &arisc_dram_paras.dram_para2))
		ARISC_ERR("get dram_para2 failed\n");

	if (of_property_read_u32(np, "dram_mr0", &arisc_dram_paras.dram_mr0))
		ARISC_ERR("get dram_mr0 failed\n");

	if (of_property_read_u32(np, "dram_mr1", &arisc_dram_paras.dram_mr1))
		ARISC_ERR("get dram_mr1 failed\n");

	if (of_property_read_u32(np, "dram_mr2", &arisc_dram_paras.dram_mr2))
		ARISC_ERR("get dram_mr2 failed\n");

	if (of_property_read_u32(np, "dram_mr3", &arisc_dram_paras.dram_mr3))
		ARISC_ERR("get dram_mr3 failed\n");

	if (of_property_read_u32(np, "dram_tpr0", &arisc_dram_paras.dram_tpr0))
		ARISC_ERR("get dram_tpr0 failed\n");

	if (of_property_read_u32(np, "dram_tpr1", &arisc_dram_paras.dram_tpr1))
		ARISC_ERR("get dram_tpr1 failed\n");

	if (of_property_read_u32(np, "dram_tpr2", &arisc_dram_paras.dram_tpr2))
		ARISC_ERR("get dram_tpr2 failed\n");

	if (of_property_read_u32(np, "dram_tpr3", &arisc_dram_paras.dram_tpr3))
		ARISC_ERR("get dram_tpr3 failed\n");

	if (of_property_read_u32(np, "dram_tpr4", &arisc_dram_paras.dram_tpr4))
		ARISC_ERR("get dram_tpr4 failed\n");

	if (of_property_read_u32(np, "dram_tpr5", &arisc_dram_paras.dram_tpr5))
		ARISC_ERR("get dram_tpr5 failed\n");

	if (of_property_read_u32(np, "dram_tpr6", &arisc_dram_paras.dram_tpr6))
		ARISC_ERR("get dram_tpr6 failed\n");

	if (of_property_read_u32(np, "dram_tpr7", &arisc_dram_paras.dram_tpr7))
		ARISC_ERR("get dram_tpr7 failed\n");

	if (of_property_read_u32(np, "dram_tpr8", &arisc_dram_paras.dram_tpr8))
		ARISC_ERR("get dram_tpr8 failed\n");

	if (of_property_read_u32(np, "dram_tpr9", &arisc_dram_paras.dram_tpr9))
		ARISC_ERR("get dram_tpr9 failed\n");

	if (of_property_read_u32(np, "dram_tpr10", &arisc_dram_paras.dram_tpr10))
		ARISC_ERR("get dram_tpr10 failed\n");

	if (of_property_read_u32(np, "dram_tpr11", &arisc_dram_paras.dram_tpr11))
		ARISC_ERR("get dram_tpr11 failed\n");

	if (of_property_read_u32(np, "dram_tpr12", &arisc_dram_paras.dram_tpr12))
		ARISC_ERR("get dram_tpr12 failed\n");

	if (of_property_read_u32(np, "dram_tpr13", &arisc_dram_paras.dram_tpr13))
		ARISC_ERR("get dram_tpr13 failed\n");

	return 0;
}

int arisc_config_dram_paras(void)
{
	struct arisc_message *pmessage;
	u32 *dram_para;
	u32 index;

	/* parse dram config paras */
	arisc_get_dram_cfg();

	/* update dram config paras to arisc system */
	pmessage = arisc_message_allocate(0);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	dram_para = (u32 *)(&arisc_dram_paras);
	for (index = 0; index < (sizeof(arisc_dram_paras) / 4); index++) {
		/* initialize message */
		pmessage->type       = ARISC_SET_DRAM_PARAS;
		pmessage->attr       = ARISC_MESSAGE_ATTR_HARDSYN;
		pmessage->paras[0]   = index;
		pmessage->paras[1]   = 1;
		pmessage->paras[2]   = dram_para[index];
		pmessage->state      = ARISC_MESSAGE_INITIALIZED;
		pmessage->cb.handler = NULL;
		pmessage->cb.arg     = NULL;

		/* send message */
		arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	}
	/* free message */
	arisc_message_free(pmessage);

	return 0;
}
