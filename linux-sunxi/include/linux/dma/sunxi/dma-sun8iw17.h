#ifndef __DMA_SUN8IW17__
#define __DMA_SUN8IW17__

/*
 * The source DRQ type and port corresponding relation
 */
#define DRQSRC_SRAM		0
#define DRQSRC_SDRAM		0
#define DRQSRC_S_PDIF_RX        2
#define DRQSRC_DAUDIO_0_RX	3
#define DRQSRC_DAUDIO_1_RX      4
#define DRQSRC_DAUDIO_2_RX      5
#define DRQSRC_AUDIO_CODEC	6
#define DRQSRC_DMIC		7
/* #define DRQSRC_RESEVER       8 */
#define DRQSRC_CE_RX            9
#define DRQSRC_NAND0		10
/* #define DRQSRC_RESEVER       11 */
#define DRQSRC_GPADC            9
/* #define DRQSRC_RESEVER       13 */
#define DRQSRC_UART0_RX		14
#define DRQSRC_UART1_RX		15
#define DRQSRC_UART2_RX         16
#define DRQSRC_UART3_RX		17
#define DRQSRC_UART4_RX         18
/* #define DRQSRC_RESEVER       19 */
/* #define DRQSRC_RESEVER       20 */
/* #define DRQSRC_RESEVER       21 */
#define DRQSRC_SPI0_RX		22
#define DRQSRC_SPI1_RX		23
/* #define DRQSRC_RESEVER       24 */
/* #define DRQSRC_RESEVER       25 */
/* #define DRQSRC_RESEVER       26 */
/* #define DRQSRC_RESEVER       27 */
/* #define DRQSRC_RESEVER       28 */
/* #define DRQSRC_RESEVER       29 */
#define DRQSRC_OTG_EP1		30
#define DRQSRC_OTG_EP2		31
#define DRQSRC_OTG_EP3          32
#define DRQSRC_OTG_EP4          33
#define DRQSRC_OTG_EP5          34

/*
 * The destination DRQ type and port corresponding relation
 */
#define DRQDST_SRAM		0
#define DRQDST_SDRAM		0
#define DRQDST_S_PDIF_TX        2
#define DRQDST_DAUDIO_0_TX	3
#define DRQDST_DAUDIO_1_TX      4
#define DRQDST_DAUDIO_2_TX	5
#define DRQDST_AUDIO_CODEC	6
/* #define DRQSRC_RESEVER       7 */
/* #define DRQSRC_RESEVER       8 */
#define DRQDST_CE_TX            9
#define DRQDST_NAND0		10
/* #define DRQSRC_RESEVER       11*/
/* #define DRQSRC_RESEVER       12 */
#define DRQDST_IR_TX		13
#define DRQDST_UART0_TX		14
#define DRQDST_UART1_TX		15
#define DRQDST_UART2_TX         16
#define DRQDST_UART3_TX         17
#define DRQDST_UART4_TX         18
/* #define DRQSRC_RESEVER       19 */
/* #define DRQSRC_RESEVER       20 */
/* #define DRQSRC_RESEVER       21 */
#define DRQDST_SPI0_TX		22
#define DRQDST_SPI1_TX          23
/* #define DRQSRC_RESEVER       24 */
/* #define DRQSRC_RESEVER       25 */
/* #define DRQSRC_RESEVER       26 */
/* #define DRQSRC_RESEVER       27 */
/* #define DRQSRC_RESEVER       28 */
/* #define DRQSRC_RESEVER       29 */
#define DRQDST_OTG_EP1		30
#define DRQDST_OTG_EP2		31
#define DRQDST_OTG_EP3          32
#define DRQDST_OTG_EP4          33
#define DRQDST_OTG_EP5          34


#endif /*__DMA_SUN8IW17__  */
