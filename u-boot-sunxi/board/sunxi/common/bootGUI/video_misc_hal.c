/*
 * Allwinner SoCs bootGUI.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <malloc.h>
#include <sys_config.h>
#include <fdt_support.h>
#include "video_misc_hal.h"

#if defined(CONFIG_BOOT_PARAMETER)
#include <sunxi_bootparam.h>
#endif

#define DISP_FDT_NODE "disp"

int get_disp_fdt_node(void)
{
	static int fdt_node = -1;

	if (0 <= fdt_node)
		return fdt_node;
	/* notice: make sure we use the only one nodt "disp". */
	fdt_node = fdt_path_offset(working_fdt, DISP_FDT_NODE);
	assert(fdt_node >= 0);
	return fdt_node;
}

void disp_getprop_by_name(int node, const char *name,
	unsigned int *value, unsigned int defval)
{
	if (fdt_getprop_u32(working_fdt, node, name, value) < 0) {
		printf("set disp.%s fail. using defval=%d\n", name, defval);
		*value = defval;
	}
}

int hal_save_int_to_kernel(char *name, int value)
{
	int ret = -1;

#ifndef CONFIG_SUNXI_MULITCORE_BOOT
	int node = get_disp_fdt_node();
	ret = fdt_setprop_u32(working_fdt, node, name, (uint32_t)value);
#else
	ret = sunxi_fdt_getprop_store(working_fdt,
		DISP_FDT_NODE, name, (uint32_t)value);
#endif
	printf("save_int_to_kernel %s.%s(0x%x) code:%s\n",
		DISP_FDT_NODE, name, value, fdt_strerror(ret));
	return ret;
}

int hal_save_string_to_kernel(char *name, char *str)
{
	int ret = -1;

#ifndef CONFIG_SUNXI_MULITCORE_BOOT
	int node = get_disp_fdt_node();
	ret = fdt_setprop_string(working_fdt, node, name, str);
#else
	ret = sunxi_fdt_getprop_store_string(working_fdt,
		DISP_FDT_NODE, name, str);
#endif
	printf("save_string_to_kernel %s.%s(%s). ret-code:%s\n",
		DISP_FDT_NODE, name, str, fdt_strerror(ret));
	return ret;
}

/*---------------------------------------------------------*/

int hal_fat_fsload(char *part_name, char *file_name,
	char *load_addr, ulong length)
{
	int readed_len = -1;

#ifdef CONFIG_BOOT_PARAMETER
	int read_from_fs = 1;
	read_from_fs = get_disp_para_mode();
	if (!read_from_fs) {
		readed_len = bootparam_get_display_region_by_name(
			file_name, load_addr, length);
	}
#ifdef HAS_FAT_FSLOAD
	if (read_from_fs && (0 >= readed_len))
		readed_len = aw_fat_fsload(part_name, file_name, load_addr, length);
#endif /* #ifdef HAS_FAT_FSLOAD */
#endif /* #ifdef CONFIG_BOOT_PARAMETER */

	return readed_len;
}
