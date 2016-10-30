/*
 *  OMAP2PLUS DMA channel definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __OMAP2PLUS_DMA_CHANNEL_H
#define __OMAP2PLUS_DMA_CHANNEL_H


/* DMA channels for 24xx */
#define OMAP24XX_DMA_NO_DEVICE		0
#define OMAP24XX_DMA_EXT_DMAREQ0	2	/* S_DMA_1 */
#define OMAP24XX_DMA_EXT_DMAREQ1	3	/* S_DMA_2 */
#define OMAP24XX_DMA_GPMC		4	/* S_DMA_3 */
#define OMAP24XX_DMA_AES_TX		9	/* S_DMA_8 */
#define OMAP24XX_DMA_AES_RX		10	/* S_DMA_9 */
#define OMAP242X_DMA_EXT_DMAREQ2	14	/* S_DMA_13 */
#define OMAP242X_DMA_EXT_DMAREQ3	15	/* S_DMA_14 */
#define OMAP242X_DMA_EXT_DMAREQ4	16	/* S_DMA_15 */
#define OMAP34XX_DMA_I2C3_TX		25	/* S_DMA_24 */
#define OMAP34XX_DMA_I2C3_RX		26	/* S_DMA_25 */
#define OMAP24XX_DMA_I2C1_TX		27	/* S_DMA_26 */
#define OMAP24XX_DMA_I2C1_RX		28	/* S_DMA_27 */
#define OMAP24XX_DMA_I2C2_TX		29	/* S_DMA_28 */
#define OMAP24XX_DMA_I2C2_RX		30	/* S_DMA_29 */
#define OMAP24XX_DMA_MMC2_TX		47	/* S_DMA_46 */
#define OMAP24XX_DMA_MMC2_RX		48	/* S_DMA_47 */
#define OMAP24XX_DMA_UART1_TX		49	/* S_DMA_48 */
#define OMAP24XX_DMA_UART1_RX		50	/* S_DMA_49 */
#define OMAP24XX_DMA_UART2_TX		51	/* S_DMA_50 */
#define OMAP24XX_DMA_UART2_RX		52	/* S_DMA_51 */
#define OMAP24XX_DMA_UART3_TX		53	/* S_DMA_52 */
#define OMAP24XX_DMA_UART3_RX		54	/* S_DMA_53 */
#define OMAP24XX_DMA_MMC1_TX		61	/* S_DMA_60 */
#define OMAP24XX_DMA_MMC1_RX		62	/* S_DMA_61 */
#define OMAP242X_DMA_EXT_DMAREQ5	64	/* S_DMA_63 */
#define OMAP34XX_DMA_AES2_TX		65	/* S_DMA_64 */
#define OMAP34XX_DMA_AES2_RX		66	/* S_DMA_65 */
#define OMAP34XX_DMA_SHA1MD5_RX		69	/* S_DMA_68 */

#define OMAP36XX_DMA_UART4_TX		81	/* S_DMA_80 */
#define OMAP36XX_DMA_UART4_RX		82	/* S_DMA_81 */

/* Only for AM35xx */
#define AM35XX_DMA_UART4_TX		54
#define AM35XX_DMA_UART4_RX		55

#endif /* __OMAP2PLUS_DMA_CHANNEL_H */
