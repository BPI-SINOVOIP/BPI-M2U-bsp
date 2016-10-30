/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 * Copyright 2012 Linaro Ltd.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/clk.h>
#include <linux/clk/mxs.h>
#include <linux/clkdev.h>
#include <linux/clocksource.h>
#include <linux/can/platform/flexcan.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/irqchip/mxs.h>
#include <linux/reboot.h>
#include <linux/micrel_phy.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/pinctrl/consumer.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/system_misc.h>

#include "pm.h"

/* MXS DIGCTL SAIF CLKMUX */
#define MXS_DIGCTL_SAIF_CLKMUX_DIRECT		0x0
#define MXS_DIGCTL_SAIF_CLKMUX_CROSSINPUT	0x1
#define MXS_DIGCTL_SAIF_CLKMUX_EXTMSTR0		0x2
#define MXS_DIGCTL_SAIF_CLKMUX_EXTMSTR1		0x3

#define MXS_GPIO_NR(bank, nr)	((bank) * 32 + (nr))

#define MXS_SET_ADDR		0x4
#define MXS_CLR_ADDR		0x8
#define MXS_TOG_ADDR		0xc

static inline void __mxs_setl(u32 mask, void __iomem *reg)
{
	__raw_writel(mask, reg + MXS_SET_ADDR);
}

static inline void __mxs_clrl(u32 mask, void __iomem *reg)
{
	__raw_writel(mask, reg + MXS_CLR_ADDR);
}

static inline void __mxs_togl(u32 mask, void __iomem *reg)
{
	__raw_writel(mask, reg + MXS_TOG_ADDR);
}

/*
 * MX28EVK_FLEXCAN_SWITCH is shared between both flexcan controllers
 */
#define MX28EVK_FLEXCAN_SWITCH	MXS_GPIO_NR(2, 13)

static int flexcan0_en, flexcan1_en;

static void mx28evk_flexcan_switch(void)
{
	if (flexcan0_en || flexcan1_en)
		gpio_set_value(MX28EVK_FLEXCAN_SWITCH, 1);
	else
		gpio_set_value(MX28EVK_FLEXCAN_SWITCH, 0);
}

static void mx28evk_flexcan0_switch(int enable)
{
	flexcan0_en = enable;
	mx28evk_flexcan_switch();
}

static void mx28evk_flexcan1_switch(int enable)
{
	flexcan1_en = enable;
	mx28evk_flexcan_switch();
}

static struct flexcan_platform_data flexcan_pdata[2];

static struct of_dev_auxdata mxs_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("fsl,imx28-flexcan", 0x80032000, NULL, &flexcan_pdata[0]),
	OF_DEV_AUXDATA("fsl,imx28-flexcan", 0x80034000, NULL, &flexcan_pdata[1]),
	{ /* sentinel */ }
};

#define OCOTP_WORD_OFFSET		0x20
#define OCOTP_WORD_COUNT		0x20

#define BM_OCOTP_CTRL_BUSY		(1 << 8)
#define BM_OCOTP_CTRL_ERROR		(1 << 9)
#define BM_OCOTP_CTRL_RD_BANK_OPEN	(1 << 12)

static DEFINE_MUTEX(ocotp_mutex);
static u32 ocotp_words[OCOTP_WORD_COUNT];

static const u32 *mxs_get_ocotp(void)
{
	struct device_node *np;
	void __iomem *ocotp_base;
	int timeout = 0x400;
	size_t i;
	static int once;

	if (once)
		return ocotp_words;

	np = of_find_compatible_node(NULL, NULL, "fsl,ocotp");
	ocotp_base = of_iomap(np, 0);
	WARN_ON(!ocotp_base);

	mutex_lock(&ocotp_mutex);

	/*
	 * clk_enable(hbus_clk) for ocotp can be skipped
	 * as it must be on when system is running.
	 */

	/* try to clear ERROR bit */
	__mxs_clrl(BM_OCOTP_CTRL_ERROR, ocotp_base);

	/* check both BUSY and ERROR cleared */
	while ((__raw_readl(ocotp_base) &
		(BM_OCOTP_CTRL_BUSY | BM_OCOTP_CTRL_ERROR)) && --timeout)
		cpu_relax();

	if (unlikely(!timeout))
		goto error_unlock;

	/* open OCOTP banks for read */
	__mxs_setl(BM_OCOTP_CTRL_RD_BANK_OPEN, ocotp_base);

	/* approximately wait 32 hclk cycles */
	udelay(1);

	/* poll BUSY bit becoming cleared */
	timeout = 0x400;
	while ((__raw_readl(ocotp_base) & BM_OCOTP_CTRL_BUSY) && --timeout)
		cpu_relax();

	if (unlikely(!timeout))
		goto error_unlock;

	for (i = 0; i < OCOTP_WORD_COUNT; i++)
		ocotp_words[i] = __raw_readl(ocotp_base + OCOTP_WORD_OFFSET +
						i * 0x10);

	/* close banks for power saving */
	__mxs_clrl(BM_OCOTP_CTRL_RD_BANK_OPEN, ocotp_base);

	once = 1;

	mutex_unlock(&ocotp_mutex);

	return ocotp_words;

error_unlock:
	mutex_unlock(&ocotp_mutex);
	pr_err("%s: timeout in reading OCOTP\n", __func__);
	return NULL;
}

enum mac_oui {
	OUI_FSL,
	OUI_DENX,
	OUI_CRYSTALFONTZ,
};

static void __init update_fec_mac_prop(enum mac_oui oui)
{
	struct device_node *np, *from = NULL;
	struct property *newmac;
	const u32 *ocotp = mxs_get_ocotp();
	u8 *macaddr;
	u32 val;
	int i;

	for (i = 0; i < 2; i++) {
		np = of_find_compatible_node(from, NULL, "fsl,imx28-fec");
		if (!np)
			return;

		from = np;

		if (of_get_property(np, "local-mac-address", NULL))
			continue;

		newmac = kzalloc(sizeof(*newmac) + 6, GFP_KERNEL);
		if (!newmac)
			return;
		newmac->value = newmac + 1;
		newmac->length = 6;

		newmac->name = kstrdup("local-mac-address", GFP_KERNEL);
		if (!newmac->name) {
			kfree(newmac);
			return;
		}

		/*
		 * OCOTP only stores the last 4 octets for each mac address,
		 * so hard-code OUI here.
		 */
		macaddr = newmac->value;
		switch (oui) {
		case OUI_FSL:
			macaddr[0] = 0x00;
			macaddr[1] = 0x04;
			macaddr[2] = 0x9f;
			break;
		case OUI_DENX:
			macaddr[0] = 0xc0;
			macaddr[1] = 0xe5;
			macaddr[2] = 0x4e;
			break;
		case OUI_CRYSTALFONTZ:
			macaddr[0] = 0x58;
			macaddr[1] = 0xb9;
			macaddr[2] = 0xe1;
			break;
		}
		val = ocotp[i];
		macaddr[3] = (val >> 16) & 0xff;
		macaddr[4] = (val >> 8) & 0xff;
		macaddr[5] = (val >> 0) & 0xff;

		of_update_property(np, newmac);
	}
}

static inline void enable_clk_enet_out(void)
{
	struct clk *clk = clk_get_sys("enet_out", NULL);

	if (!IS_ERR(clk))
		clk_prepare_enable(clk);
}

static void __init imx28_evk_init(void)
{
	update_fec_mac_prop(OUI_FSL);

	mxs_saif_clkmux_select(MXS_DIGCTL_SAIF_CLKMUX_EXTMSTR0);
}

static void __init imx28_evk_post_init(void)
{
	if (!gpio_request_one(MX28EVK_FLEXCAN_SWITCH, GPIOF_DIR_OUT,
			      "flexcan-switch")) {
		flexcan_pdata[0].transceiver_switch = mx28evk_flexcan0_switch;
		flexcan_pdata[1].transceiver_switch = mx28evk_flexcan1_switch;
	}
}

static int apx4devkit_phy_fixup(struct phy_device *phy)
{
	phy->dev_flags |= MICREL_PHY_50MHZ_CLK;
	return 0;
}

static void __init apx4devkit_init(void)
{
	enable_clk_enet_out();

	if (IS_BUILTIN(CONFIG_PHYLIB))
		phy_register_fixup_for_uid(PHY_ID_KSZ8051, MICREL_PHY_ID_MASK,
					   apx4devkit_phy_fixup);
}

#define ENET0_MDC__GPIO_4_0	MXS_GPIO_NR(4, 0)
#define ENET0_MDIO__GPIO_4_1	MXS_GPIO_NR(4, 1)
#define ENET0_RX_EN__GPIO_4_2	MXS_GPIO_NR(4, 2)
#define ENET0_RXD0__GPIO_4_3	MXS_GPIO_NR(4, 3)
#define ENET0_RXD1__GPIO_4_4	MXS_GPIO_NR(4, 4)
#define ENET0_TX_EN__GPIO_4_6	MXS_GPIO_NR(4, 6)
#define ENET0_TXD0__GPIO_4_7	MXS_GPIO_NR(4, 7)
#define ENET0_TXD1__GPIO_4_8	MXS_GPIO_NR(4, 8)
#define ENET_CLK__GPIO_4_16	MXS_GPIO_NR(4, 16)

#define TX28_FEC_PHY_POWER	MXS_GPIO_NR(3, 29)
#define TX28_FEC_PHY_RESET	MXS_GPIO_NR(4, 13)
#define TX28_FEC_nINT		MXS_GPIO_NR(4, 5)

static const struct gpio tx28_gpios[] __initconst = {
	{ ENET0_MDC__GPIO_4_0, GPIOF_OUT_INIT_LOW, "GPIO_4_0" },
	{ ENET0_MDIO__GPIO_4_1, GPIOF_OUT_INIT_LOW, "GPIO_4_1" },
	{ ENET0_RX_EN__GPIO_4_2, GPIOF_OUT_INIT_LOW, "GPIO_4_2" },
	{ ENET0_RXD0__GPIO_4_3, GPIOF_OUT_INIT_LOW, "GPIO_4_3" },
	{ ENET0_RXD1__GPIO_4_4, GPIOF_OUT_INIT_LOW, "GPIO_4_4" },
	{ ENET0_TX_EN__GPIO_4_6, GPIOF_OUT_INIT_LOW, "GPIO_4_6" },
	{ ENET0_TXD0__GPIO_4_7, GPIOF_OUT_INIT_LOW, "GPIO_4_7" },
	{ ENET0_TXD1__GPIO_4_8, GPIOF_OUT_INIT_LOW, "GPIO_4_8" },
	{ ENET_CLK__GPIO_4_16, GPIOF_OUT_INIT_LOW, "GPIO_4_16" },
	{ TX28_FEC_PHY_POWER, GPIOF_OUT_INIT_LOW, "fec-phy-power" },
	{ TX28_FEC_PHY_RESET, GPIOF_OUT_INIT_LOW, "fec-phy-reset" },
	{ TX28_FEC_nINT, GPIOF_DIR_IN, "fec-int" },
};

static void __init tx28_post_init(void)
{
	struct device_node *np;
	struct platform_device *pdev;
	struct pinctrl *pctl;
	int ret;

	enable_clk_enet_out();

	np = of_find_compatible_node(NULL, NULL, "fsl,imx28-fec");
	pdev = of_find_device_by_node(np);
	if (!pdev) {
		pr_err("%s: failed to find fec device\n", __func__);
		return;
	}

	pctl = pinctrl_get_select(&pdev->dev, "gpio_mode");
	if (IS_ERR(pctl)) {
		pr_err("%s: failed to get pinctrl state\n", __func__);
		return;
	}

	ret = gpio_request_array(tx28_gpios, ARRAY_SIZE(tx28_gpios));
	if (ret) {
		pr_err("%s: failed to request gpios: %d\n", __func__, ret);
		return;
	}

	/* Power up fec phy */
	gpio_set_value(TX28_FEC_PHY_POWER, 1);
	msleep(26); /* 25ms according to data sheet */

	/* Mode strap pins */
	gpio_set_value(ENET0_RX_EN__GPIO_4_2, 1);
	gpio_set_value(ENET0_RXD0__GPIO_4_3, 1);
	gpio_set_value(ENET0_RXD1__GPIO_4_4, 1);

	udelay(100); /* minimum assertion time for nRST */

	/* Deasserting FEC PHY RESET */
	gpio_set_value(TX28_FEC_PHY_RESET, 1);

	pinctrl_put(pctl);
}

static void __init cfa10049_init(void)
{
	update_fec_mac_prop(OUI_CRYSTALFONTZ);
}

static void __init cfa10037_init(void)
{
	update_fec_mac_prop(OUI_CRYSTALFONTZ);
}

static void __init mxs_machine_init(void)
{
	if (of_machine_is_compatible("fsl,imx28-evk"))
		imx28_evk_init();
	else if (of_machine_is_compatible("bluegiga,apx4devkit"))
		apx4devkit_init();
	else if (of_machine_is_compatible("crystalfontz,cfa10037"))
		cfa10037_init();
	else if (of_machine_is_compatible("crystalfontz,cfa10049"))
		cfa10049_init();

	of_platform_populate(NULL, of_default_bus_match_table,
			     mxs_auxdata_lookup, NULL);

	if (of_machine_is_compatible("karo,tx28"))
		tx28_post_init();

	if (of_machine_is_compatible("fsl,imx28-evk"))
		imx28_evk_post_init();
}

#define MX23_CLKCTRL_RESET_OFFSET	0x120
#define MX28_CLKCTRL_RESET_OFFSET	0x1e0
#define MXS_CLKCTRL_RESET_CHIP		(1 << 1)

/*
 * Reset the system. It is called by machine_restart().
 */
static void mxs_restart(enum reboot_mode mode, const char *cmd)
{
	struct device_node *np;
	void __iomem *reset_addr;

	np = of_find_compatible_node(NULL, NULL, "fsl,clkctrl");
	reset_addr = of_iomap(np, 0);
	if (!reset_addr)
		goto soft;

	if (of_device_is_compatible(np, "fsl,imx23-clkctrl"))
		reset_addr += MX23_CLKCTRL_RESET_OFFSET;
	else
		reset_addr += MX28_CLKCTRL_RESET_OFFSET;

	/* reset the chip */
	__mxs_setl(MXS_CLKCTRL_RESET_CHIP, reset_addr);

	pr_err("Failed to assert the chip reset\n");

	/* Delay to allow the serial port to show the message */
	mdelay(50);

soft:
	/* We'll take a jump through zero as a poor second */
	soft_restart(0);
}

static void __init mxs_timer_init(void)
{
	if (of_machine_is_compatible("fsl,imx23"))
		mx23_clocks_init();
	else
		mx28_clocks_init();
	clocksource_of_init();
}

static const char *mxs_dt_compat[] __initdata = {
	"fsl,imx28",
	"fsl,imx23",
	NULL,
};

DT_MACHINE_START(MXS, "Freescale MXS (Device Tree)")
	.map_io		= debug_ll_io_init,
	.init_irq	= irqchip_init,
	.handle_irq	= icoll_handle_irq,
	.init_time	= mxs_timer_init,
	.init_machine	= mxs_machine_init,
	.init_late      = mxs_pm_init,
	.dt_compat	= mxs_dt_compat,
	.restart	= mxs_restart,
MACHINE_END
