/*
 * Some macro and struct of SUNXI RTC.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * Mintow <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _RTC_SUNXI_H_
#define _RTC_SUNXI_H_

#define SUNXI_LOSC_CTRL				0x0000
#define SUNXI_LOSC_CTRL_RTC_HMS_ACC		BIT(8)
#define SUNXI_LOSC_CTRL_RTC_YMD_ACC		BIT(7)
#define REG_LOSCCTRL_MAGIC			0x16aa0000
#define REG_CLK32K_AUTO_SWT_EN			BIT(14)
#define RTC_SOURCE_EXTERNAL			0x00000001
#define EXT_LOSC_GSM				0x00000008
#define SUNXI_ALARM_CONFIG                      0x0050
#define SUNXI_ALRM_WAKEUP_OUTPUT_EN             BIT(0)

/* alarm0 which based on seconds can power on system,
 * while alarm1 can't, so alarm1 is not used.
 */
/* #define SUNXI_ALARM1_USED */

#if (defined CONFIG_ARCH_SUN50IW1) \
	|| (defined CONFIG_ARCH_SUN50IW2) \
	|| (defined CONFIG_ARCH_SUN50IW3) \
	|| (defined CONFIG_ARCH_SUN50IW6) \
	|| (defined CONFIG_ARCH_SUN8IW10P1) \
	|| (defined CONFIG_ARCH_SUN8IW11P1)
#define SUNXI_RTC_YMD				0x0010

#define SUNXI_RTC_HMS				0x0014

#ifdef SUNXI_ALARM1_USED

#define SUNXI_ALRM_DHMS				0x0040

#define SUNXI_ALRM_EN				0x0044
#define SUNXI_ALRM_EN_CNT_EN			BIT(8)

#define SUNXI_ALRM_IRQ_EN			0x0048
#define SUNXI_ALRM_IRQ_EN_CNT_IRQ_EN		BIT(0)

#define SUNXI_ALRM_IRQ_STA			0x004c
#define SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND		BIT(0)

#else

#define SUNXI_ALRM_COUNTER                      0x0020
#define SUNXI_ALRM_CURRENT                      0x0024

#define SUNXI_ALRM_EN                           0x0028
#define SUNXI_ALRM_EN_CNT_EN                    BIT(0)

#define SUNXI_ALRM_IRQ_EN                       0x002c
#define SUNXI_ALRM_IRQ_EN_CNT_IRQ_EN            BIT(0)

#define SUNXI_ALRM_IRQ_STA                      0x0030
#define SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND         BIT(0)

#endif /* end of SUNXI_ALARM1_USED */

#else

#define SUNXI_RTC_YMD				0x0004

#define SUNXI_RTC_HMS				0x0008

#define SUNXI_ALRM_DHMS				0x000c

#define SUNXI_ALRM_EN				0x0014
#define SUNXI_ALRM_EN_CNT_EN			BIT(8)

#define SUNXI_ALRM_IRQ_EN			0x0018
#define SUNXI_ALRM_IRQ_EN_CNT_IRQ_EN		BIT(0)

#define SUNXI_ALRM_IRQ_STA			0x001c
#define SUNXI_ALRM_IRQ_STA_CNT_IRQ_PEND		BIT(0)

#endif

#define SUNXI_MASK_DH				0x0000001f
#define SUNXI_MASK_SM				0x0000003f
#define SUNXI_MASK_M				0x0000000f
#define SUNXI_MASK_LY				0x00000001
#define SUNXI_MASK_D				0x00000ffe

#define SUNXI_GET(x, mask, shift)		(((x) & ((mask) << (shift))) \
							>> (shift))

#define SUNXI_SET(x, mask, shift)		(((x) & (mask)) << (shift))

/*
 * Get date values
 */
#define SUNXI_DATE_GET_DAY_VALUE(x)		SUNXI_GET(x, SUNXI_MASK_DH, 0)
#define SUNXI_DATE_GET_MON_VALUE(x)		SUNXI_GET(x, SUNXI_MASK_M, 8)
#define SUNXI_DATE_GET_YEAR_VALUE(x, d)	SUNXI_GET(x, (d)->mask, (d)->yshift)

/*
 * Get time values
 */
#define SUNXI_TIME_GET_SEC_VALUE(x)		SUNXI_GET(x, SUNXI_MASK_SM, 0)
#define SUNXI_TIME_GET_MIN_VALUE(x)		SUNXI_GET(x, SUNXI_MASK_SM, 8)
#define SUNXI_TIME_GET_HOUR_VALUE(x)	SUNXI_GET(x, SUNXI_MASK_DH, 16)

/*
 * Get alarm values
 */
#define SUNXI_ALRM_GET_SEC_VALUE(x)		SUNXI_GET(x, SUNXI_MASK_SM, 0)
#define SUNXI_ALRM_GET_MIN_VALUE(x)		SUNXI_GET(x, SUNXI_MASK_SM, 8)
#define SUNXI_ALRM_GET_HOUR_VALUE(x)	SUNXI_GET(x, SUNXI_MASK_DH, 16)

/*
 * Set date values
 */
#define SUNXI_DATE_SET_DAY_VALUE(x)		SUNXI_DATE_GET_DAY_VALUE(x)
#define SUNXI_DATE_SET_MON_VALUE(x)		SUNXI_SET(x, SUNXI_MASK_M, 8)
#define SUNXI_DATE_SET_YEAR_VALUE(x, d)	SUNXI_SET(x, (d)->mask, (d)->yshift)
#define SUNXI_LEAP_SET_VALUE(x, shift)	SUNXI_SET(x, SUNXI_MASK_LY, shift)

/*
 * Set time values
 */
#define SUNXI_TIME_SET_SEC_VALUE(x)		SUNXI_TIME_GET_SEC_VALUE(x)
#define SUNXI_TIME_SET_MIN_VALUE(x)		SUNXI_SET(x, SUNXI_MASK_SM, 8)
#define SUNXI_TIME_SET_HOUR_VALUE(x)	SUNXI_SET(x, SUNXI_MASK_DH, 16)

/*
 * Set alarm values
 */
#define SUNXI_ALRM_SET_SEC_VALUE(x)		SUNXI_ALRM_GET_SEC_VALUE(x)
#define SUNXI_ALRM_SET_MIN_VALUE(x)		SUNXI_SET(x, SUNXI_MASK_SM, 8)
#define SUNXI_ALRM_SET_HOUR_VALUE(x)	SUNXI_SET(x, SUNXI_MASK_DH, 16)
#define SUNXI_ALRM_SET_DAY_VALUE(x)		SUNXI_SET(x, SUNXI_MASK_D, 21)

/*
 * Time unit conversions
 */
#define SEC_IN_MIN				60
#define SEC_IN_HOUR				(60 * SEC_IN_MIN)
#define SEC_IN_DAY				(24 * SEC_IN_HOUR)

/*
 * The year parameter passed to the driver is usually an offset relative to
 * the year 1900. This macro is used to convert this offset to another one
 * relative to the minimum year allowed by the hardware.
 */
#define SUNXI_YEAR_OFF(x)			((x)->min - 1900)

#if defined(CONFIG_ARCH_SUN50IW2) || defined(CONFIG_ARCH_SUN50IW6)
#define SUNXI_RTC_DYNAMIC_YEAR_RANGE	1
#endif

/*
 * min and max year are arbitrary set considering the limited range of the
 * hardware register field
 */
struct sunxi_rtc_data_year {
	unsigned int min;		/* min year allowed */
	unsigned int max;		/* max year allowed */
	unsigned int mask;		/* mask for the year field */
	unsigned int yshift;		/* bit shift to get the year */
	unsigned char leap_shift;	/* bit shift to get the leap year */
};

struct sunxi_rtc_dev {
	struct rtc_device *rtc;
	struct device *dev;
	struct sunxi_rtc_data_year *data_year;
	void __iomem *base;
	int irq;
};

#endif /* end of _RTC_SUNXI_H_ */

