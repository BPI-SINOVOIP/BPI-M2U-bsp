/*
 * sound\soc\sunxi\sun8iw10codec.h
 * (C) Copyright 2014-2016
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SUN8IW10_CODEC_H
#define _SUN8IW10_CODEC_H

#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_FIFOC		0x04
#define SUNXI_DAC_FIFOS		0x08
#define SUNXI_ADC_FIFOC		0x10
#define SUNXI_ADC_FIFOS		0x14
#define SUNXI_ADC_RXDATA	0x18
#define SUNXI_DAC_TXDATA	0x20
#define SUNXI_DAC_CNT		0x40
#define SUNXI_ADC_CNT		0x44
#define SUNXI_DAC_DG		0x48
#define SUNXI_ADC_DG		0x4c

#define SUNXI_ADC_DAP_CTR 	0x70
#define SUNXI_ADC_DRC_HHPFC 	0x200
#define SUNXI_ADC_DRC_LHPFC 	0x204

#define AC_PR_CFG		0x400

#define ANALOG_FLAG             0x8000
#define HP_VOL			(0x00 + ANALOG_FLAG)
#define LOMIX_SRC		(0x01 + ANALOG_FLAG)
#define ROMIX_SRC		(0x02 + ANALOG_FLAG)
#define DAC_PA_SRC		(0x03 + ANALOG_FLAG)
#define LINEIN_GCTR		(0x05 + ANALOG_FLAG)
#define MIC_GCTR		(0x06 + ANALOG_FLAG)
#define HP_CTRL			(0x07 + ANALOG_FLAG)
#define SPKL_CTR		(0x08 + ANALOG_FLAG)
#define SPKR_CTR		(0x09 + ANALOG_FLAG)
#define SPK_MBIAS_CTR	(0x0a + ANALOG_FLAG)
#define BIAS_MIC_CTR	(0x0b + ANALOG_FLAG)
#define ADC_MIX_MUTE	(0x0c + ANALOG_FLAG)
#define PA_POP_CTR		(0x0e + ANALOG_FLAG)
#define ADC_A_CTR		(0x0f + ANALOG_FLAG)
#define OPADC_CTR		(0x10 + ANALOG_FLAG)
#define OPMIC_CTR		(0x11 + ANALOG_FLAG)
#define ZERO_CROSS_CTRL	(0x12 + ANALOG_FLAG)
#define ADC_FUN_CTRL	(0x13 + ANALOG_FLAG)
#define BIAS_DA16_CTR	(0x14 + ANALOG_FLAG)
#define DA16_CALI_DATA	(0x15 + ANALOG_FLAG)
#define BIAS_CALI_DATA	(0x17 + ANALOG_FLAG)
#define BIAS_CALI_SET	(0x18 + ANALOG_FLAG)

/*AC_DAC_DPC:0x00*/
#define EN_DAC			31
#define MODQU			25
#define HPF_EN			18
#define DVOL			12
#define HUB_EN			0

/*AC_DAC_FIFOC:0x04*/
#define DAC_FS			29
#define FIR_VER			28
#define SEND_LASAT		26
#define FIFO_MODE		24
#define DAC_DRQ_CLR_CNT	21
#define TX_TRIG_LEVEL	8
#define ADDA_LOOP_EN	7
#define DAC_MONO_EN		6
#define TX_SAMPLE_BITS	5
#define DAC_DRQ_EN		4
#define DAC_IRQ_EN		3
#define FIFO_UNDERRUN_IRQ_EN 2
#define FIFO_OVERRUN_IRQ_EN	1
#define FIFO_FLUSH		0

/*AC_ADC_FIFOC:0x10*/
#define ADFS			29
#define EN_AD			28
#define RX_FIFO_MODE	24
#define ADCDFEN			16
#define RX_FIFO_TRG_LEVEL	8
#define ADC_MONO_EN		7
#define RX_SAMPLE_BITS	6
#define ADC_DRQ_EN		4
#define ADC_IRQ_EN		3
#define ADC_OVERRUN_IRQ_EN	1
#define ADC_FIFO_FLUSH	0

/*AC_ADC_TXDATA:0x20*/
#define TX_DATA			0

/*AC_DAC_CNT:0x40*/
#define TX_CNT			0

/*AC_ADC_CNT:0x44*/
#define RX_CNT			0

/*AC_DAC_DG:0x48*/
/*
*	DAC Modulator Debug
*	0:DAC Modulator Normal Mode
*	1:DAC Modulator Debug Mode
*/
#define DAC_MODU_SELECT	11
/*
* 	DAC Pattern Select
*	00:Normal(Audio sample from TX fifo)
*	01: -6 dB sin wave
*	10: -60 dB sin wave
*	11: silent wave
*/
#define DAC_PATTERN_SELECT 9
/*
* 	CODEC Clock Source Select
*	0:codec clock from PLL
* 	1:codec clock from OSC(for debug)
*/
#define CODEC_CLK_SELECT	8
/*
*	DAC output channel swap enable
*	0:disable
*	1:enable
*/
#define DA_SWP			6

/*AC_ADC_DG:0x4c*/
#define AD_SWP			24

/*AC_ADC_DAP_CTR : 0x70*/
#define ADC_DRC_HPF_EN 		24
#define ENADC_DRC 		26

/*AC_ADC_DRC_HHPFC : 0x200*/
#define ADC_HHPF_CONF 	0

/*AC_ADC_DRC_LHPFC : 0x204*/
#define ADC_LHPF_CONF 	0

/*AC_PR_CFG:0x400*/
#define AC_PR_RST		28
#define AC_PR_RW		24
#define AC_PR_ADDR		16
#define ADDA_PR_WDAT	8
#define ADDA_PR_RDAT	0

/*HP_VOL:0x00*/
#define HPVOL			0

/*LOMIX_SRC:0x01*/
#define LMIXMUTE		0
#define LMIX_MIC1_BST	6
#define LMIX_LINEINL	2
#define LMIX_LDAC		1
#define LMIX_RDAC		0

/*ROMIX_SRC:0x02*/
#define RMIXMUTE		0
#define RMIX_MIC1_BST	6
#define RMIX_LINEINR	2
#define RMIX_RDAC		1
#define RMIX_LDAC		0

/*DAC_PA_SRC:0x03*/
#define DACAREN			7
#define DACALEN			6
#define RMIXEN			5
#define LMIXEN			4
#define RHPPAMUTE		3
#define LHPPAMUTE		2
#define RHPIS			1
#define LHPIS			0

/*LINEIN_GCTR:0x05*/
#define LINEING			4

/*MIC_GCTR:0x06*/
#define MIC_GAIN		4

/*HP_CTRL:0x07*/
#define HPPAEN			7
#define HPCOM_FC		5
#define HPCOM_PT		4
#define COS_SLOPE_CTRL	2
#define LTRNMUTE		1
#define RTLNMUTE		0

/*SPKL_CTR:0x08*/
#define SPKL_SS			6
#define SPKL_VOL		0

/*SPKR_CTR:0x09*/
#define SPKR_SS			6
#define SPKR_VOL		0

/*SPK_MBIAS_CTR:0x0a*/
#define SPKLP_EN		7
#define SPKLN_EN		6
#define SPKRP_EN		5
#define SPKRN_EN		4
#define MBIASSEL		0

/*BIAS_MIC_CTR:0x0b*/
#define MMICBIASEN		6
#define MIC_AMPEN		3
#define MIC_BOOST		0

/*ADC_MIX_MUTE:0x0c*/
#define LADCMIXMUTE		0
#define LADC_MIC_BST	6
#define LADC_LINEINR	3
#define LADC_LINEINL	2
#define LADC_LOUT_MIX	1
#define LADC_ROUT_MIX	0

/*PA_POP_CTR:0x0e*/
#define PA_POP_CTRL		0

/*ADC_A_CTR:0x0f*/
#define ADCEN			6
#define ADCG			0

/*ADC_FUN_CTRL:0x13*/
#define MMIC_BIASCHOPEN	7

/*BIAS_DA16_CTR:0x14*/
#define PA_SPEED_SELECT	7

struct label {
    const char *name;
    int value;
};

#define LABEL(constant) { #constant, constant }
#define LABEL_END { NULL, -1 }

struct spk_gpio_ {
	u32 gpio;
	bool cfg;
};
struct gain_config {
	u32 headphonevol;
	u32 spkervol;
	u32 maingain;
};

struct codec_hw_config {
	u32 adcagc_cfg:1;
	u32 adcdrc_cfg:1;
	u32 dacdrc_cfg:1;
	u32 adchpf_cfg:1;
	u32 dachpf_cfg:1;
};

struct voltage_supply {
	struct regulator *pa_shdn;
};

struct sunxi_codec {
	void __iomem *codec_dbase;
	void __iomem *codec_abase;
	struct clk *srcclk;
	struct pinctrl *pinctrl;

	struct gain_config gain_config;
	struct codec_hw_config hwconfig;

	struct mutex dac_mutex;
	struct mutex adc_mutex;
	struct snd_soc_codec *codec;
	struct snd_soc_dai_driver dai;
	struct voltage_supply vol_supply;
	struct clk *pllclk;
	struct clk *moduleclk;
	u32 dac_enable;
	u32 adc_enable;
	u32 pa_sleep_time;
	bool hp_dirused;
	bool spkenable;
};
#endif
