/*
 *  drivers/arisc/interfaces/arisc_axp.c
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

#include "../arisc_i.h"

/* nmi isr node, record current nmi interrupt handler and argument */
nmi_isr_t nmi_isr_node[2];
EXPORT_SYMBOL(nmi_isr_node);

/**
 * register call-back function, call-back function is for arisc notify some event to ac327,
 * axp/rtc interrupt for external interrupt NMI.
 * @type:  nmi type, pmu/rtc;
 * @func:  call-back function;
 * @para:  parameter for call-back function;
 *
 * @return: result, 0 - register call-back function successed;
 *                 !0 - register call-back function failed;
 * NOTE: the function is like "int callback(void *para)";
 *       this function will execute in system ISR.
 */
int arisc_nmi_cb_register(u32 type, arisc_cb_t func, void *para)
{
	if (nmi_isr_node[type].handler) {
		if(func == nmi_isr_node[type].handler) {
			ARISC_WRN("nmi interrupt handler register already\n");
			return 0;
		}
		/* just output warning message, overlay handler */
		ARISC_WRN("nmi interrupt handler register already\n");
		return -EINVAL;
	}
	nmi_isr_node[type].handler = func;
	nmi_isr_node[type].arg     = para;

	return 0;
}
EXPORT_SYMBOL(arisc_nmi_cb_register);


/**
 * unregister call-back function.
 * @type:  nmi type, pmu/rtc;
 * @func:  call-back function which need be unregister;
 */
void arisc_nmi_cb_unregister(u32 type, arisc_cb_t func)
{
	if ((nmi_isr_node[type].handler) != (func)) {
		/* invalid handler */
		ARISC_WRN("invalid handler for unreg\n\n");
		return ;
	}
	nmi_isr_node[type].handler = NULL;
	nmi_isr_node[type].arg     = NULL;
}
EXPORT_SYMBOL(arisc_nmi_cb_unregister);

#if defined CONFIG_SUNXI_ARISC_COM_DIRECTLY
int arisc_config_pmu_paras(void)
{
	struct device_node *np;
	u32 pmu_discharge_ltf = 0;
	u32 pmu_discharge_htf = 0;

	int result = 0;
	struct arisc_message *pmessage;

#if defined CONFIG_ARCH_SUN8IW5P1
	np = of_find_node_by_type(NULL, "charger0");
#else
	np = of_find_compatible_node(NULL, NULL, "allwinner,pmu0");
#endif
	if (!np) {
		ARISC_ERR("No allwinner,pmu0 node found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(np, "pmu_discharge_ltf", &pmu_discharge_ltf)) {
		ARISC_ERR("parse pmu discharge ltf fail\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "pmu_discharge_htf", &pmu_discharge_htf)) {
		ARISC_ERR("parse pmu discharge ltf fail\n");
		return -EINVAL;
	}

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_SET_PARAS;
	pmessage->private    = (void *)0x00; /* init pmu paras flag */
	pmessage->paras[0]   = pmu_discharge_ltf;
	pmessage->paras[1]   = pmu_discharge_htf;
	pmessage->paras[2]   = 0;

	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	ARISC_INF("pmu_discharge_ltf:0x%x\n", pmessage->paras[0]);
	ARISC_INF("pmu_discharge_htf:0x%x\n", pmessage->paras[1]);

	/* send request message */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/*check config fail or not*/
	if (pmessage->result) {
		ARISC_WRN("config pmu paras [%d] [%d] fail\n", pmessage->paras[0], pmessage->paras[1]);
		result = -EINVAL;
	}

	/* free allocated message */
	arisc_message_free(pmessage);

	return result;
}

int arisc_disable_nmi_irq(void)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_DISABLE_IRQ;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_disable_nmi_irq);

int arisc_enable_nmi_irq(void)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_ENABLE_IRQ;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_enable_nmi_irq);

int arisc_clear_nmi_status(void)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_CLR_NMI_STATUS;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_clear_nmi_status);

int arisc_set_nmi_trigger(u32 type)
{
	int result;

	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_SET_NMI_TRIGGER;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = type;
	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}

int arisc_axp_get_chip_id(unsigned char *chip_id)
{
	int                   i;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_GET_CHIP_ID;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	memset((void *)pmessage->paras, 0, 16);

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* |paras[0]    |paras[1]    |paras[2]     |paras[3]      |
	 * |chip_id[0~3]|chip_id[4~7]|chip_id[8~11]|chip_id[12~15]|
	 */
	/* copy message readout data to user data buffer */
	for (i = 0; i < 4; i++) {
			chip_id[i] = (pmessage->paras[0] >> (i * 8)) & 0xff;
			chip_id[4 + i] = (pmessage->paras[1] >> (i * 8)) & 0xff;
			chip_id[8 + i] = (pmessage->paras[2] >> (i * 8)) & 0xff;
			chip_id[12 + i] = (pmessage->paras[3] >> (i * 8)) & 0xff;
	}

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_axp_get_chip_id);

int arisc_set_led_bln(u32 led_rgb, u32 led_onms,
			u32 led_offms, u32 led_darkms)
{
	int result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_SET_LED_BLN;
	pmessage->private    = (void *)0; /* set charge magic flag */
	pmessage->paras[0]   = led_rgb;
	pmessage->paras[1]   = led_onms;
	pmessage->paras[2]   = led_offms;
	pmessage->paras[3]   = led_darkms;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;

}
EXPORT_SYMBOL(arisc_set_led_bln);

#if (defined CONFIG_ARCH_SUN8IW5P1)
int arisc_adjust_pmu_chgcur(unsigned int max_chgcur, unsigned int chg_ic_temp)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_AXP_SET_PARAS;
	pmessage->private    = (void *)0x62; /* set charge current flag */
	pmessage->paras[0]   = chg_ic_temp;
	pmessage->paras[1]   = max_chgcur;
	pmessage->paras[2]   = 0;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_adjust_pmu_chgcur);
#endif

int arisc_axp_int_notify(struct arisc_message *pmessage)
{
	u32 type = pmessage->paras[0];
	u32 ret = 0;

	if (type & NMI_INT_TYPE_PMU_OFFSET) {
		/* call pmu interrupt handler */
		if (nmi_isr_node[NMI_INT_TYPE_PMU].handler == NULL) {
			ARISC_WRN("pmu irq handler not install\n");
			return 1;
		}

		ARISC_INF("call pmu interrupt handler\n");
		ret |= (*(nmi_isr_node[NMI_INT_TYPE_PMU].handler))(nmi_isr_node[NMI_INT_TYPE_PMU].arg);
	}
	if (type & NMI_INT_TYPE_RTC_OFFSET) {
		/* call rtc interrupt handler */
		if (nmi_isr_node[NMI_INT_TYPE_RTC].handler == NULL) {
			ARISC_WRN("rtc irq handler not install\n");
			return 1;
		}

		ARISC_INF("call rtc interrupt handler\n");
		ret |= (*(nmi_isr_node[NMI_INT_TYPE_RTC].handler))(nmi_isr_node[NMI_INT_TYPE_RTC].arg);
	}

	return ret;
}

int arisc_set_pwr_tree(u32 *pwr_tree)
{
	int result;
	void *virt_addr;
	phys_addr_t phys_addr;
	struct arisc_message *pmessage;

	virt_addr = dma_alloc_coherent(NULL, PAGE_SIZE, &(phys_addr),
				       GFP_KERNEL);
	if (IS_ERR(virt_addr)) {
		ARISC_ERR("alloc memory failed\n");
		return -ENOMEM;
	}

	memcpy(virt_addr, pwr_tree, sizeof(int) * VCC_MAX_INDEX);

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		dma_free_coherent(NULL, PAGE_SIZE, virt_addr, phys_addr);
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_SET_PWR_TREE;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = (u32)phys_addr;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	dma_free_coherent(NULL, PAGE_SIZE, virt_addr, phys_addr);

	return result;
}
EXPORT_SYMBOL(arisc_set_pwr_tree);

int arisc_pmu_set_voltage(u32 type, u32 voltage)
{
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_SET_PMU_VOLT;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = type;
	pmessage->paras[1]   = voltage;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_pmu_set_voltage);

unsigned int arisc_pmu_get_voltage(u32 type)
{
	u32                   voltage;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_GET_PMU_VOLT;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = type;

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	voltage = pmessage->paras[1];

	/* free message */
	arisc_message_free(pmessage);

	return voltage;
}
EXPORT_SYMBOL(arisc_pmu_get_voltage);

#else
int arisc_disable_nmi_irq(void)
{
	int                   result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_AXP_DISABLE_IRQ, 0, 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_disable_nmi_irq);

int arisc_enable_nmi_irq(void)
{
	int result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_AXP_ENABLE_IRQ, 0, 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_enable_nmi_irq);

int arisc_clear_nmi_status(void)
{
	int result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_CLR_NMI_STATUS, 0, 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_clear_nmi_status);

int arisc_set_nmi_trigger(u32 type)
{
	int result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_SET_NMI_TRIGGER, type, 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_set_nmi_trigger);

int arisc_axp_get_chip_id(unsigned char *chip_id)
{
	int result;

	/* FIXME: if the runtime sever enable the mmu & dcache,
	 * should not use flush cache here.
	 */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_AXP_GET_CHIP_ID,
			virt_to_phys(chip_id), 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_axp_get_chip_id);

int arisc_set_led_bln(u32 led_rgb, u32 led_onms, u32 led_offms, u32 led_darkms)
{
	int result;
	u32 paras[22];

	paras[0] = led_rgb;
	paras[1] = led_onms;
	paras[2] = led_offms;
	paras[3] = led_darkms;

	/* FIXME: if the runtime sever enable the mmu & dcache,
	 * should not use flush cache here.
	 */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_SET_LED_BLN,
			virt_to_phys(paras), 0, 0);

	return result;

}
EXPORT_SYMBOL(arisc_set_led_bln);

#if (defined CONFIG_ARCH_SUN50IW1P1) || \
	(defined CONFIG_ARCH_SUN50IW2P1) || \
	(defined CONFIG_ARCH_SUN50IW3P1) || \
	(defined CONFIG_ARCH_SUN50IW6P1)
int arisc_adjust_pmu_chgcur(unsigned int max_chgcur, unsigned int chg_ic_temp)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_ARISC_AXP_SET_PARAS, max_chgcur,
			chg_ic_temp, 1);

	return result;
}
EXPORT_SYMBOL(arisc_adjust_pmu_chgcur);
#endif

int arisc_set_pwr_tree(u32 *pwr_tree)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_ARISC_SET_PWR_TREE,
			virt_to_phys(pwr_tree), 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_set_pwr_tree);


int arisc_axp_int_notify(struct arisc_message *pmessage)
{
	u32 type = pmessage->paras[0];
	u32 ret = 0;

	if (type & NMI_INT_TYPE_PMU_OFFSET) {
		/* call pmu interrupt handler */
		if (nmi_isr_node[NMI_INT_TYPE_PMU].handler == NULL) {
			ARISC_WRN("pmu irq handler not install\n");
			return 1;
		}

		ARISC_INF("call pmu interrupt handler\n");
		ret |= (*(nmi_isr_node[NMI_INT_TYPE_PMU].handler))(nmi_isr_node[NMI_INT_TYPE_PMU].arg);
	}
	if (type & NMI_INT_TYPE_RTC_OFFSET)
	{
		/* call rtc interrupt handler */
		if (nmi_isr_node[NMI_INT_TYPE_RTC].handler == NULL) {
			ARISC_WRN("rtc irq handler not install\n");
			return 1;
		}

		ARISC_INF("call rtc interrupt handler\n");
		ret |= (*(nmi_isr_node[NMI_INT_TYPE_RTC].handler))(nmi_isr_node[NMI_INT_TYPE_RTC].arg);
	}

	return ret;
}

int arisc_pmu_set_voltage(u32 type, u32 voltage)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_ARISC_SET_PMU_VOLT, type, voltage, 0);

	return result;
}
EXPORT_SYMBOL(arisc_pmu_set_voltage);

unsigned int arisc_pmu_get_voltage(u32 type)
{
	u32 voltage;

	invoke_scp_fn_smc(ARM_SVC_ARISC_GET_PMU_VOLT, type,
			virt_to_phys(&voltage), 0);

	return voltage;
}
EXPORT_SYMBOL(arisc_pmu_get_voltage);

int arisc_pmu_set_voltage_state(u32 type, u32 state)
{
	int result;

	result = invoke_scp_fn_smc(ARM_SVC_ARISC_SET_PMU_VOLT_STA, type, state, 0);

	return result;
}
EXPORT_SYMBOL(arisc_pmu_set_voltage_state);

unsigned int arisc_pmu_get_voltage_state(u32 type)
{
	u32 state;

	invoke_scp_fn_smc(ARM_SVC_ARISC_GET_PMU_VOLT_STA, type,
			virt_to_phys(&state), 0);

	return state;
}
EXPORT_SYMBOL(arisc_pmu_get_voltage_state);


#endif
