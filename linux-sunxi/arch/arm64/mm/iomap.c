/*
 *  linux/arch/arm64/mm/iomap.c
 * Copyright (C) 2017 Allwinner Co., Ltd.
 *
 * Author: ysn <yangshounan@allwinnertech.com>
 *
 * Map IO port and PCI memory spaces so that {read,write}[bwl] can
 * be used to access this memory.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/io.h>


#ifdef CONFIG_PCI
unsigned long pcibios_min_io = 0x1000;
EXPORT_SYMBOL(pcibios_min_io);

unsigned long pcibios_min_mem = 0x01000000;
EXPORT_SYMBOL(pcibios_min_mem);

#endif
