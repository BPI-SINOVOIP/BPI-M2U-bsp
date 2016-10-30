#ifndef __DMA_sun50iw3__
#define __DMA_sun50iw3__

/*
 * The source DRQ type and port corresponding relation
 *
 */
#define DRQSRC_SRAM		0
#define DRQSRC_SDRAM		DRQSRC_SRAM
#define DRQSRC_DRAM		1
/* #define DRQSRC_RESEVER	2 */
#define DRQSRC_DAUDIO_0_RX	3
#define DRQSRC_DAI0_RX		DRQSRC_DAUDIO_0_RX
#define DRQSRC_DAUDIO_1_RX	4
#define DRQSRC_DAI1_RX		DRQSRC_DAUDIO_1_RX
#define DRQSRC_DAUDIO_2_RX	5
#define DRQSRC_DAI2_RX		DRQSRC_DAUDIO_2_RX
#define DRQSRC_AUDIO_CODEC	6
#define DRQSRC_CODEC		DRQSRC_AUDIO_CODEC
#define DRQSRC_DMIC		7
/* #define DRQSRC_RESEVER	8 */
#define DRQSRC_SS		9
#define DRQSRC_CE_RX		DRQSRC_SS
#define DRQSRC_NAND0		10
/* #define DRQSRC_RESEVER	11 */
#define DRQSRC_GPADC		12
/* #define DRQSRC_RESEVER	13 */
#define DRQSRC_UART0RX		14
#define DRQSRC_UART1RX		15
#define DRQSRC_UART2RX		16
#define DRQSRC_UART3RX		17
/* #define DRQSRC_RESEVER	18 */
/* #define DRQSRC_RESEVER	19 */
/* #define DRQSRC_RESEVER	20 */
/* #define DRQSRC_RESEVER	21 */
#define DRQSRC_SPI0RX		22
#define DRQSRC_SPI1RX		23
/* #define DRQSRC_RESEVER	24 */
/* #define DRQSRC_RESEVER	25 */
/* #define DRQSRC_RESEVER	26 */
/* #define DRQSRC_RESEVER	27 */
/* #define DRQSRC_RESEVER	28 */
/* #define DRQSRC_RESEVER	29 */
#define DRQSRC_OTG_EP1		30
#define DRQSRC_OTG_EP2		31
#define DRQSRC_OTG_EP3		32
#define DRQSRC_OTG_EP4		33
#define DRQSRC_OTG_EP5		34

/*
 * The destination DRQ type and port corresponding relation
 *
 */
#define DRQDST_SRAM		0
#define DRQDST_SDRAM		DRQDST_SRAM
#define DRQDST_DRAM		1
/* #define DRQDST_RESEVER	2 */
#define DRQDST_DAUDIO_0_TX	3
#define DRQDST_DAI0_TX		DRQDST_DAUDIO_0_TX
#define DRQDST_DAUDIO_1_TX	4
#define DRQDST_DAI1_TX		DRQDST_DAUDIO_1_TX
#define DRQDST_DAUDIO_2_TX	5
#define DRQDST_DAI2_TX		DRQDST_DAUDIO_2_TX
#define DRQDST_AUDIO_CODEC	6
#define DRQDST_CODEC		DRQDST_AUDIO_CODEC
/* #define DRQDST_RESEVER	7 */
/* #define DRQDST_RESEVER	8 */
#define DRQDST_SS		9
#define DRQDST_CE_TX		DRQDST_SS
#define DRQDST_NAND0		10
/* #define DRQDST_RESEVER	11 */
/* #define DRQDST_RESEVER	12 */
/* #define DRQDST_RESEVER	13 */
#define DRQDST_UART0TX		14
#define DRQDST_UART1TX 		15
#define DRQDST_UART2TX		16
#define DRQDST_UART3TX		17
/* #define DRQDST_RESEVER	18 */
/* #define DRQDST_RESEVER	19 */
/* #define DRQDST_RESEVER	20 */
/* #define DRQDST_RESEVER	21 */
#define DRQDST_SPI0TX		22
#define DRQDST_SPI1TX		23
/* #define DRQDST_RESEVER	24 */
/* #define DRQDST_RESEVER	25 */
/* #define DRQDST_RESEVER	26 */
/* #define DRQDST_RESEVER	27 */
/* #define DRQDST_RESEVER	28 */
/* #define DRQDST_RESEVER	29 */
#define DRQDST_OTG_EP1		30
#define DRQDST_OTG_EP2		31
#define DRQDST_OTG_EP3		32
#define DRQDST_OTG_EP4		33
#define DRQDST_OTG_EP5		34

#endif /*__DMA_sun50iw3__  */
