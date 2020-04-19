/*
 * Allwinner SoCs(sun3iw1p1) pinctrl driver.
 *
 * Copyright (C) 2014 Jackie Hwang
 *
 * Jackie Hwang <huangshr@allwinnertech.com>
 *
 * Copyright (C) 2014 Chen-Yu Tsai
 *
 * Chen-Yu Tsai <wens@csie.org>
 *
 * Copyright (C) 2014 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-sunxi.h"
static const struct sunxi_desc_pin sun3iw1p1_pins[] = {
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "tp0"),		/* X1 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* BCLK */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* RTS */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* CS */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "tp0"),		/* X2 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* LRCK */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* CTS */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* MOSI */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "tp0"),		/* Y1 */
		  SUNXI_FUNCTION(0x3, "pwm0"),		/* PWM0 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* IN */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* RX */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* MOSI */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "tp0"),		/* Y2 */
		  SUNXI_FUNCTION(0x3, "ir0"),		/* RX */
		  SUNXI_FUNCTION(0x4, "da0"),		/* OUT */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* TX */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* MISO */
		  SUNXI_FUNCTION(0x7, "io_disabled")),

	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "dqs0"),		/* DQS0 */
		  SUNXI_FUNCTION(0x3, "twi1"),		/* SCK */
		  SUNXI_FUNCTION(0x4, "da0"),		/* BCLK */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* RTS */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* CS */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "dqs1"),		/* DQS1 */
		  SUNXI_FUNCTION(0x3, "twi1"),		/* SDA */
		  SUNXI_FUNCTION(0x4, "da0"),		/* LRCK */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* CTS */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* MOSI */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "ckb"),		/* CKB */
		  SUNXI_FUNCTION(0x3, "pwm0"),		/* PWM0 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* IN */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* RX */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* CLK */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "ddr_ref_d"),	/* DDR_REF_D */
		  SUNXI_FUNCTION(0x3, "ir0"),		/* RX */
		  SUNXI_FUNCTION(0x4, "da0"),		/* OUT */
		  SUNXI_FUNCTION(0x5, "uart1"),		/* TX */
		  SUNXI_FUNCTION(0x6, "spi1"),		/* MISO */
		  SUNXI_FUNCTION(0x7, "io_disabled")),

	/*Hole*/
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "spi0"),		/* CLK */
		  SUNXI_FUNCTION(0x3, "sdc1"),		/* CLK */
		  SUNXI_FUNCTION(0x4, "vdevice"),	/* vdevice */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "spi0"),		/* CS */
		  SUNXI_FUNCTION(0x3, "sdc1"),		/* CMD */
		  SUNXI_FUNCTION(0x4, "vdevice"),	/* vdevice */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "spi0"),		/* MISO */
		  SUNXI_FUNCTION(0x3, "sdc1"),		/* D0 */
		  SUNXI_FUNCTION(0x7, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "spi0"),		/* MOSI */
		  SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		  SUNXI_FUNCTION(0x7, "io_disabled")),

	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D2 */
		  SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		  SUNXI_FUNCTION(0x4, "rsb0"),		/* SDA */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D3 */
		  SUNXI_FUNCTION(0x3, "uart1"),		/* RTS */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D4*/
		  SUNXI_FUNCTION(0x3, "uart1"),		/* CTS */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D5 */
		  SUNXI_FUNCTION(0x3, "uart1"),		/* RX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D6 */
		  SUNXI_FUNCTION(0x3, "uart1"),		/* TX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D7 */
		  SUNXI_FUNCTION(0x3, "twi1"),		/* SCK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D10 */
		  SUNXI_FUNCTION(0x3, "twi1"),		/* SDA */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D11 */
		  SUNXI_FUNCTION(0x3, "da0"),		/* MCLK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D12 */
		  SUNXI_FUNCTION(0x3, "da0"),		/* BCLK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D13 */
		  SUNXI_FUNCTION(0x3, "da0"),		/* LRCK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D14 */
		  SUNXI_FUNCTION(0x3, "da0"),		/* IN */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D15 */
		  SUNXI_FUNCTION(0x3, "da0"),		/* OUT */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D18 */
		  SUNXI_FUNCTION(0x3, "twi0"),		/* SCK */
		  SUNXI_FUNCTION(0x4, "rsb0"),		/* SCK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 12)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D19 */
		  SUNXI_FUNCTION(0x3, "uart2"),		/* TX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 13)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D20 */
		  SUNXI_FUNCTION(0x3, "lvds1"),		/* RX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 14)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D21 */
		  SUNXI_FUNCTION(0x3, "uart2"),		/* RTS */
		  SUNXI_FUNCTION(0x4, "twi2"),		/* SCK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 15)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D22 */
		  SUNXI_FUNCTION(0x3, "uart2"),		/* CTS */
		  SUNXI_FUNCTION(0x4, "twi2"),		/* SDA */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 16)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* D23 */
		  SUNXI_FUNCTION(0x3, "spdif0"),	/* OUT */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 17)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* CLK */
		  SUNXI_FUNCTION(0x3, "spi0"),		/* CS */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 18)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* DE */
		  SUNXI_FUNCTION(0x3, "spi0"),		/* MOSI */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 19)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* HYSNC */
		  SUNXI_FUNCTION(0x3, "spi0"),		/* CLK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 20)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "lcd0"),		/* VSYNC */
		  SUNXI_FUNCTION(0x3, "spi0"),		/* MISO */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 0, 21)),

	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* HSYNC */
		  SUNXI_FUNCTION(0x3, "lcd0"),		/* D0 */
		  SUNXI_FUNCTION(0x4, "twi2"),		/* SCK */
		  SUNXI_FUNCTION(0x5, "uart0"),		/* RX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* VSYNC */
		  SUNXI_FUNCTION(0x3, "lcd0"),		/* D1 */
		  SUNXI_FUNCTION(0x4, "twi2"),		/* SDA */
		  SUNXI_FUNCTION(0x5, "uart0"),		/* TX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* PCLK */
		  SUNXI_FUNCTION(0x3, "lcd0"),		/* D8 */
		  SUNXI_FUNCTION(0x4, "clk0"),		/* OUT */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D0 */
		  SUNXI_FUNCTION(0x3, "lcd0"),		/* D9 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* BCLK */
		  SUNXI_FUNCTION(0x5, "rsb0"),		/* SCK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D1 */
		  SUNXI_FUNCTION(0x3, "lcd0"),		/* D16 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* LRCK */
		  SUNXI_FUNCTION(0x5, "rsb0"),		/* SDA */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D2 */
		  SUNXI_FUNCTION(0x3, "lcd0"),		/* D17 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* IN */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 5)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D3 */
		  SUNXI_FUNCTION(0x3, "pwm1"),		/* PWM1 */
		  SUNXI_FUNCTION(0x4, "da0"),		/* OUT */
		  SUNXI_FUNCTION(0x5, "spdif0"),	/* OUT */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 6)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D4 */
		  SUNXI_FUNCTION(0x3, "uart2"),		/* TX */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* CS */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 7)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D5 */
		  SUNXI_FUNCTION(0x3, "uart2"),		/* RX */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* MOSI */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 8)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D6 */
		  SUNXI_FUNCTION(0x3, "uart2"),		/* RTS */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* CLK */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 9)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "csi0"),		/* D7 */
		  SUNXI_FUNCTION(0x3, "uart2"),		/* CTS */
		  SUNXI_FUNCTION(0x4, "spi1"),		/* MISO */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 10)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "clk0"),		/* OUT */
		  SUNXI_FUNCTION(0x3, "twi0"),		/* SCK */
		  SUNXI_FUNCTION(0x4, "ir0"),		/* RX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 11)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 12),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "da0"),		/* MCLK */
		  SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		  SUNXI_FUNCTION(0x4, "pwm0"),		/* PWM0 */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 1, 12)),

	/* Hole */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "sdc0"),		/* D1 */
		  SUNXI_FUNCTION(0x3, "dbg0"),		/* MS */
		  SUNXI_FUNCTION(0x4, "ir0"),		/* MS */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 0)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "sdc0"),		/* D0 */
		  SUNXI_FUNCTION(0x3, "dgb0"),		/* DI */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 1)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "sdc0"),		/* CLK */
		  SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 2)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "sdc0"),		/* CMD */
		  SUNXI_FUNCTION(0x3, "dbg0"),		/* DO */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 3)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "sdc0"),		/* D3 */
		  SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 4)),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		  SUNXI_FUNCTION(0x0, "gpio_in"),
		  SUNXI_FUNCTION(0x1, "gpio_out"),
		  SUNXI_FUNCTION(0x2, "sdc0"),		/* D2 */
		  SUNXI_FUNCTION(0x3, "dbg0"),		/* CK */
		  SUNXI_FUNCTION(0x4, "pmw1"),		/* PMW1 */
		  SUNXI_FUNCTION(0x7, "io_disabled"),
		  SUNXI_FUNCTION_IRQ_BANK(0x6, 2, 5)),
};

#define IRQ_BANK_NUM 3
static const unsigned sun3iw1p1_irq_bank_base[IRQ_BANK_NUM] = {0};

static const struct sunxi_pinctrl_desc sun3iw1p1_pinctrl_data = {
	.pins = sun3iw1p1_pins,
	.npins = ARRAY_SIZE(sun3iw1p1_pins),
	.pin_base = 0,
	.irq_banks = IRQ_BANK_NUM,
	.irq_bank_base = sun3iw1p1_irq_bank_base,
};

static int sun3iw1p1_pinctrl_probe(struct platform_device *pdev)
{

	return sunxi_pinctrl_init(pdev,
				  &sun3iw1p1_pinctrl_data);
}

static struct of_device_id sun3iw1p1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun3iw1p1-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun3iw1p1_pinctrl_match);

static struct platform_driver sun3iw1p1_pinctrl_driver = {
	.probe	= sun3iw1p1_pinctrl_probe,
	.driver	= {
		.name		= "sun3iw1p1-pinctrl",
		.owner		= THIS_MODULE,
		.of_match_table	= sun3iw1p1_pinctrl_match,
	},
};
static int __init sun3iw1p1_pio_init(void)
{
	int ret;
	ret = platform_driver_register(&sun3iw1p1_pinctrl_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_debug("register sun3iw1p1 pio controller failed\n");
		return -EINVAL;
	}
	return 0;
}
postcore_initcall(sun3iw1p1_pio_init);

MODULE_AUTHOR("Jackie Hwang <huangshr@allwinnertech.com>");
MODULE_AUTHOR("Chen-Yu Tsai <wens@csie.org>");
MODULE_AUTHOR("Boris Brezillon <boris.brezillon@free-electrons.com");
MODULE_AUTHOR("Maxime Ripard <maxime.ripard@free-electrons.com");
MODULE_DESCRIPTION("Allwinner sun3iw1p1 PIO pinctrl driver");
MODULE_LICENSE("GPL");
