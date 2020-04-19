/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "include/ESMHost.h"

void boot_fail_check(esm_instance_t *esm)
{
	u32 reg = 0;

	esm->driver->hpi_read(esm->driver->instance,
				ESM_REG_AE_ERR_STAT0(HPI_HOST_OFF),
				&reg);

	pr_info("boot_fail_check 0x60:%x", reg);
}


ESM_STATUS ESM_GetStatusRegister(esm_instance_t *esm, esm_status_t *status,
							uint8_t clear)
{
	ESM_STATUS err;
	uint32_t reg0 = 0;
	uint32_t clr = 0, clr1 = 0;

	/*LOG_TRACE();*/
	if (esm == 0)
		return ESM_HL_NO_INSTANCE;

	/* Read the status bitmask */
	if (esm->driver->hpi_read(esm->driver->instance,
					ESM_REG_HP_IRQ_STAT_MSG(HPI_HOST_OFF),
					&reg0) != ESM_HL_SUCCESS) {
		pr_info("HPI Read Failed %d", err);
		return ESM_HL_DRIVER_HPI_READ_FAILED;
	}

	if (ESM_REG_HP_IRQ_BIT_SET(reg0, HP_IRQ_STAT_UPDATED_STAT_BIT)) {
		if (esm->driver->hpi_read(esm->driver->instance,
					ESM_REG_AE_ERR_STAT0(HPI_HOST_OFF),
					&status->esm_exception) != ESM_HL_DRIVER_SUCCESS) {
			pr_info("HPI Read Failed %d", err);
			return ESM_HL_DRIVER_HPI_READ_FAILED;
		}
		/* Populate exceptions FIFO buffer */
		esm_hostlib_put_exceptions(esm);
		/* Save the last exception */
		esm->esm_exception = status->esm_exception;
		clr |= (1 << HP_IRQ_STAT_UPDATED_STAT_BIT);
		clr1 |= (1 << HP_IRQ_STAT_UPDATED_STAT_BIT);
	}

	if (ESM_REG_HP_IRQ_BIT_SET(reg0, HP_IRQ_SYNC_LOST_STAT_BIT)) {
		esm->esm_sync_lost = 1;
		status->esm_sync_lost = 1;
		clr |= (1 << HP_IRQ_SYNC_LOST_STAT_BIT);
	} else {
		esm->esm_sync_lost = 0;
		status->esm_sync_lost = 0;
	}

	if (ESM_REG_HP_IRQ_BIT_SET(reg0, HP_IRQ_AUTH_PASS_STAT_BIT)) {
		esm->esm_auth_pass = 1;
		status->esm_auth_pass = 1;
		clr |= (1 << HP_IRQ_AUTH_PASS_STAT_BIT);
	} else {
		esm->esm_auth_pass = 0;
		status->esm_auth_pass = 0;
	}

	if (ESM_REG_HP_IRQ_BIT_SET(reg0, HP_IRQ_AUTH_FAIL_STAT_BIT)) {
		esm->esm_auth_fail = 1;
		status->esm_auth_fail = 1;
		clr |= (1 << HP_IRQ_AUTH_FAIL_STAT_BIT);
	} else {
		esm->esm_auth_fail = 0;
		status->esm_auth_fail = 0;
	}


	if (clear == 1) {
		if (clr != 0) {
			if (esm->driver->hpi_write(esm->driver->instance,
					ESM_REG_HP_IRQ_STAT_MSG(HPI_HOST_OFF), clr) != ESM_HL_SUCCESS) {
				pr_info("HPI Write Failed %d", err);
				return ESM_HL_DRIVER_HPI_WRITE_FAILED;
			}
		}
	} else {
		if (clr1 != 0) {
			if (esm->driver->hpi_write(esm->driver->instance,
					ESM_REG_HP_IRQ_STAT_MSG(HPI_HOST_OFF), clr1) != ESM_HL_SUCCESS) {
				pr_info("HPI Write Failed %d", err);
				return ESM_HL_DRIVER_HPI_WRITE_FAILED;
			}
		}
	}

	return ESM_HL_SUCCESS;
}
