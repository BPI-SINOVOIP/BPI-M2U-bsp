/*
 * Allwinner SoCs bootGUI.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __VIDEO_MISC_HAL_H__
#define __VIDEO_MISC_HAL_H__

int get_disp_fdt_node(void);
void disp_getprop_by_name(int node, const char *name,
	unsigned int *value, unsigned int defval);
int hal_save_int_to_kernel(char *name, int value);
int hal_save_string_to_kernel(char *name, char *str);

int hal_fat_fsload(char *part_name, char *file_name,
	char *load_addr, unsigned long length);

#endif /* #ifndef __VIDEO_MISC_HAL_H__ */
