/*
 * (C) Copyright 2017-2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 * Configuration settings for the Allwinner sunxi series of boards.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <sys_partition.h>
#include <sys_config.h>
#include <sys_config_old.h>
#include <smc.h>
#include <securestorage.h>
#include <sunxi_board.h>

#if defined(CONFIG_SUNXI_HDCP_IN_SECURESTORAGE)

int sunxi_get_hdcp_cfg(void)
{
	int ret;
	int hdcpkey_enable = 0;

	ret = script_parser_fetch("hdmi", "hdmi_hdcp_enable",
		&hdcpkey_enable, 1);
	if ((ret) || (hdcpkey_enable != 1)) {
		pr_warning("hdmi hdcp not enable!\n");
		return 0;
	}

	return hdcpkey_enable;
}

int sunxi_hdcp_keybox_install(void)
{
	int ret = 0;
	int workmode;
	sunxi_secure_storage_info_t secure_object;

	workmode = get_boot_work_mode();
	if ((workmode == WORK_MODE_USB_PRODUCT)
		|| (workmode == WORK_MODE_USB_UPDATE)
		|| (workmode == WORK_MODE_USB_DEBUG))
		return 0;

	if (!sunxi_get_hdcp_cfg())
		return 0;

	memset(&secure_object, 0, sizeof(secure_object));
	ret = sunxi_secure_storage_init();
	if (ret) {
		pr_error("sunxi init secure storage failed\n");
		return -1;
	}

	ret = sunxi_secure_object_up("hdcpkey", (void *)&secure_object,
			sizeof(secure_object));
	if (ret) {
		pr_error("probe hdcp key failed\n");
		return -1;
	}

	ret = smc_tee_keybox_store("hdcpkey", (void *)&secure_object,
			sizeof(secure_object));
	if (ret) {
		pr_error("ssk encrypt failed\n");
		return -1;
	}

	ret = smc_aes_rssk_decrypt_to_keysram();
	if (ret) {
		pr_error("push hdcp key failed\n");
		return -1;
	}
	pr_msg("push hdcp key finish!\n");

	return 0;
}
#endif

#ifdef CONFIG_WIDEVINE_KEY_INSTALL
DECLARE_GLOBAL_DATA_PTR;

int sunxi_widevine_keybox_install(void)
{
	int ret = -1;
	int workmode;
	sunxi_secure_storage_info_t secure_object;

	workmode =  get_boot_work_mode();
	if (workmode != WORK_MODE_BOOT)
		return 0;

	if (gd->securemode != SUNXI_SECURE_MODE_WITH_SECUREOS)
		return 0;

	memset(&secure_object, 0, sizeof(secure_object));
	if (sunxi_secure_storage_init() < 0) {
		pr_error("secure storage init fail\n");
		ret = -1;
		goto out;
	}

	ret = sunxi_secure_object_up("widevine",
		(void *)&secure_object, sizeof(secure_object));
	if (ret) {
		pr_error("secure storage read fail\n");
		ret = -1;
		goto out;
	}


	ret = smc_tee_keybox_store("widevine",
		(void *)&secure_object, sizeof(secure_object));
	if (ret) {
		pr_error("key install fail\n");
		ret = -1;
		goto out;
	}

	pr_msg("key install finish\n");
	return 0;
out:
	pr_error("Widevine key install fail !!!\n");
	return -1;
}

#endif

