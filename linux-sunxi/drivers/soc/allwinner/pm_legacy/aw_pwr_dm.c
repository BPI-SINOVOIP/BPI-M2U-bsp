
/* arch/arm/mach-sunxi/pm/aw_pwr_dm.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *
 *  By      : yanggq
 *  Version : v1.0
 *  Date    : 2014-8-6 09:08
 */

#include <linux/ctype.h>
#include <linux/module.h>
#include "linux/power/aw_pm.h"
#include "pm.h"
#include "linux/power/axp_depend.h"
#include <linux/power/scenelock.h>

#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)) && defined(CONFIG_AW_AXP)
ssize_t parse_pwr_dm_map(char *s, unsigned int size, unsigned int bitmap)
{
	int i = 0;
	char *start = s;
	char *end = NULL;
	unsigned int bit_event = 0;
	int count = 0;
	int counted = 0;

	if (NULL == s || 0 == size) {
		if (!(parse_bitmap_en & DEBUG_PWR_DM_MAP))
			return 0;
		s = NULL;
	} else {
		end = s + size;
	}

	for (i = 0; i < (pwr_dm_bitmap_name_mapping_cnt); i++) {
		bit_event =
		    (1 << pwr_dm_bitmap_name_mapping[i].mask_bit & bitmap);
		if (bit_event) {
			uk_printf(s, end - s, "\t\t%s bit 0x%x\t",
				  pwr_dm_bitmap_name_mapping[i].id_name,
				  (1 << pwr_dm_bitmap_name_mapping[i].
				   mask_bit));
			count++;
		}

		if (counted != count && 0 == count % 2) {
			counted = count;
			uk_printf(s, end - s, "\n");
		}

	}

	uk_printf(s, end - s, "\n");

	return s - start;

}
#else
ssize_t parse_pwr_dm_map(char *s, unsigned int size, unsigned int bitmap)
{
	return -1;
}
#endif
