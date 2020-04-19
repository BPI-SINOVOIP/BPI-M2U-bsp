/****************************************************************************
 * the file for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
****************************************************************************/

#include "platform.h"
#include "../nand_blk.h"
#include "../nand_dev.h"


/*****************************************************************************

*****************************************************************************/
int NAND_IS_Secure_sys(void)
{
if (PLATFORM_CLASS == 0) {
	if (sunxi_soc_is_secure()) {
		NAND_Print_DBG("secure system\n");
		return 1;
	    } else {
		    NAND_Print_DBG("non secure\n");
		    return 0;
	    }
	} else if (PLATFORM_CLASS == 1) {
		return 1;
	} else {
		return -1;
	}
}

int NAND_boot0_img_IS_Secure_package(void)
{
	if (sunxi_soc_is_secure()) {
		NAND_Print_DBG("secure system\n");
		return 1;
	    } else {
		    NAND_Print_DBG("non secure\n");
		    return 0;
	    }
}

/*****************************************************************************

*****************************************************************************/
int NAND_Get_Boot0_Acess_Pagetable_Mode(void)
{
	return PLATFORM_BOOT0_ACESS_PAGE_TABLE_MODE;
}

/*****************************************************************************

*****************************************************************************/
int NAND_Get_Platform_NO(void)
{
	return PLATFORM_NO;
}
/*****************************************************************************

*****************************************************************************/
int nand_print_platform(void)
{
	NAND_Print(PLATFORM_STRINGS);
	NAND_Print("\n");
	return 0;
}

