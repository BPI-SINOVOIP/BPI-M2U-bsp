/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef INCLUDE_APP_EDID_EDID_H_
#define INCLUDE_APP_EDID_EDID_H_
#include "../config.h"

#if defined(__LINUX_PLAT__)

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/ioctl.h>
#endif

#include "api/hdmitx_dev.h"
#include "api/core/video.h"
#include "api/core/audio.h"
#include "api/general_ops.h"
#include "api/core_api.h"

extern int edid_read(hdmi_tx_dev_t *dev, struct edid *edid);
extern int edid_extension_read(hdmi_tx_dev_t *dev, int block, u8 *edid_ext);

void edid_read_cap(void);

int edid_tx_supports_cea_code(u32 cea_code);

void edid_set_video_prefered_core(void);
int edid_sink_supports_vic_code(u32 vic_code);

void edid_correct_hardware_config(void);

#endif /* INCLUDE_APP_EDID_EDID_H_ */
