/*
 *  drivers/arisc/interfaces/arisc_rsb.c
 *
 * Copyright (c) 2013 Allwinner.
 * 2013-07-01 Written by superm (superm@allwinnertech.com).
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
#define DEBUG_SHAREMEM 0

/**
 * ap read data.
 * @data:    point of data;
 * @length:  length of data;
 *
 * return: result, 0 - read data successed,
 *                !0 - read data failed or the len more then max len;
 */
int arisc_ap_read_data(char *data, int length)
{
	int result;

	if ((data == NULL) || (length == 0)) {
		ARISC_WRN("ap read data paras error\n");
		return -EINVAL;
	}

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_AP_READ_DATA,
				   virt_to_phys(data), length, 0);

	return result;
}
EXPORT_SYMBOL(arisc_ap_read_data);

/**
 * ap write data.
 * @data:    point of data;
 * @length:  length of data;
 *
 * return: result, 0 - write data successed,
 *                !0 - write data failed or the len more then max len;
 */
int arisc_ap_write_data(char *data, int length)
{
	int result;

	if ((data == NULL) || (length == 0)) {
		ARISC_WRN("ap write data paras error\n");
		return -EINVAL;
	}

#if DEBUG_SHAREMEM
	{
		uint8_t *ptr = (uint8_t *)data;
		printk(
		    "ap write: %02x [%02x %02x %02x %02x] [%02x %02x %02x %02x] [%02x] [%02x %02x %02x %02x\n",
		    ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6],
		    ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12],
		    ptr[13]);
	}
#endif

	dmac_flush_range(data, data + length);
	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_AP_WRITE_DATA,
				   virt_to_phys(data), length, 0);

	return result;
}
EXPORT_SYMBOL(arisc_ap_write_data);

/**
 * set msgbox channel receiver interrupt.
 *
 * return: result, 0 - setting successed,
 *                !0 - setting failed;
 */
int arisc_set_msgbox_receiver_int(unsigned int channel, unsigned int user,
				  bool enable)
{
	int result;

	if ((channel < 0) || (channel >= 8)) {
		ARISC_WRN("channel number error\n");
		return -EINVAL;
	}

	if ((user != 0) && (user != 1)) {
		ARISC_WRN("channel user error\n");
		return -EINVAL;
	}

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_SET_MSGBOX_RX_INT,
				   channel, user, enable);

	return result;
}
EXPORT_SYMBOL(arisc_set_msgbox_receiver_int);

/**
 * get msgbox channel receiver pend.
 *
 * return: result 1 - interrupt pending,
 *                !0 - interrupt not pending;
 */
int arisc_get_msgbox_receiver_pend(unsigned int channel, unsigned int user)
{
	int result;

	if ((channel < 0) || (channel >= 8)) {
		ARISC_WRN("channel number error\n");
		return -EINVAL;
	}

	if ((user != 0) && (user != 1)) {
		ARISC_WRN("channel user error\n");
		return -EINVAL;
	}

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_GET_MSGBOX_RX_PEND, channel,
				   user, 0);

	return result;
}
EXPORT_SYMBOL(arisc_get_msgbox_receiver_pend);

/**
 * clear msgbox channel receiver pend.
 *
 * return: result 0 - clear interrupt pending successed,
 *                !0 - clear interrupt pending  failed;;
 */
int arisc_clear_msgbox_receiver_pend(unsigned int channel, unsigned int user)
{
	int result;

	if ((channel < 0) || (channel >= 8)) {
		ARISC_WRN("channel number error\n");
		return -EINVAL;
	}

	if ((user != 0) && (user != 1)) {
		ARISC_WRN("channel user error\n");
		return -EINVAL;
	}

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_CLR_MSGBOX_RX_PEND, channel,
				   user, 0);

	return result;
}
EXPORT_SYMBOL(arisc_clear_msgbox_receiver_pend);

/**
 *  ap wakeup sh interrupt.
 *
 * return: result 0 - send successed,
 *                !0 - send  failed;;
 */
int arisc_ap_wakeup_sh(bool wakeup)
{
	int result;

	/* send message use hwmsgbox */
	result = invoke_scp_fn_smc(ARM_SVC_ARISC_AP_WAKEUP_SH, wakeup, 0, 0);

	return result;
}
EXPORT_SYMBOL(arisc_ap_wakeup_sh);
