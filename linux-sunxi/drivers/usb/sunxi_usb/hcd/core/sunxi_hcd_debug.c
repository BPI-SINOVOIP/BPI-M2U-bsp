/*
 * drivers/usb/sunxi_usb/hcd/core/sunxi_hcd_debug.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include  "../include/sunxi_hcd_config.h"
#include  "../include/sunxi_hcd_debug.h"

void print_sunxi_hcd_config(struct sunxi_hcd_config *config, char *str)
{
#ifdef SUNXI_HCD_DEBUG
	DMSG_INFO("\n");
	DMSG_INFO("--------------------%s----------------\n", str);

	DMSG_INFO("multipoint       = %d\n", config->multipoint);
	DMSG_INFO("dyn_fifo         = %d\n", config->dyn_fifo);
	DMSG_INFO("soft_con         = %d\n", config->soft_con);
	DMSG_INFO("utm_16           = %d\n", config->utm_16);
	DMSG_INFO("big_endian       = %d\n", config->big_endian);
	DMSG_INFO("mult_bulk_tx     = %d\n", config->mult_bulk_tx);
	DMSG_INFO("mult_bulk_rx     = %d\n", config->mult_bulk_rx);
	DMSG_INFO("high_iso_tx      = %d\n", config->high_iso_tx);
	DMSG_INFO("high_iso_rx      = %d\n", config->high_iso_rx);
	DMSG_INFO("dma              = %d\n", config->dma);
	DMSG_INFO("vendor_req       = %d\n", config->vendor_req);
	DMSG_INFO("num_eps          = %d\n", config->num_eps);
	DMSG_INFO("dma_channels     = %d\n", config->dma_channels);
	DMSG_INFO("dyn_fifo_size    = %d\n", config->dyn_fifo_size);
	DMSG_INFO("vendor_ctrl      = %d\n", config->vendor_ctrl);
	DMSG_INFO("vendor_stat      = %d\n", config->vendor_stat);
	DMSG_INFO("dma_req_chan     = %d\n", config->dma_req_chan);
	DMSG_INFO("ram_size         = %d\n", config->ram_size);

	DMSG_INFO("eps_bits->name   = %s\n", config->eps_bits->name);
	DMSG_INFO("eps_bits->bits   = %d\n", config->eps_bits->bits);

	DMSG_INFO("\n");
#endif
}

void print_sunxi_hcd_list(struct list_head *list_head, char *str)
{
#ifdef SUNXI_HCD_DEBUG

	struct list_head *list_now = NULL;
	struct list_head *list_next = NULL;

	if (list_head == NULL) {
		DMSG_PANIC("ERR: invalid argment\n");
		return;
	}

	if (list_empty(list_head) || list_head->next == NULL) {
		DMSG_PANIC("ERR: list is empty\n");
		return;
	}

	list_now = list_head->next;
	while (list_now != list_head) {
		list_next = list_now->next;

		DMSG_INFO("[%s]: list_now = 0x%p, next = 0x%p, prev = 0x%p, list_head = 0x%p\n",
			str, list_now, list_now->next,
			list_now->prev, list_head);

		list_now = list_next;
	}
#endif
}

void print_urb_list(struct usb_host_endpoint *hep, char *str)
{
	struct urb *urb_temp = NULL;

	list_for_each_entry(urb_temp, &hep->urb_list,  urb_list) {
		DMSG_INFO("[%s]: urb_temp(0x%p, %d, %d, %d)\n",
			str,
			urb_temp, urb_temp->transfer_buffer_length,
			urb_temp->actual_length, urb_temp->status);
	}
}

