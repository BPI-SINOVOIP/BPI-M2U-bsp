/*
 * sound\soc\sunxi\sun3iw1_codec.c
 * (C) Copyright 2014-2017
 * Reuuimlla Technology Co., Ltd. <www.allwinnertech.com>
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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>

#include "sunxi_rw_func.h"
#include "sunxi_cpudai.h"

#define DRV_NAME "sunxi-internal-codec"

void __iomem *codec_digitaladress;
/* void __iomem *codec_analogadress; */
struct spk_gpio_ spk_gpio;

static const DECLARE_TLV_DB_SCALE(dig_vol_tlv, -7424, 0, 0);
static const DECLARE_TLV_DB_SCALE(headphone_vol_tlv, -6300, 100, 0);
static const DECLARE_TLV_DB_SCALE(linein_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic1_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(spkl_vol_tlv, -4800, 150, 0);
static const DECLARE_TLV_DB_SCALE(spkr_vol_tlv, -4800, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic_boost_vol_tlv, 0, 300, 0);
static const DECLARE_TLV_DB_SCALE(adc_input_gain_tlv, -450, 150, 0);

struct codec_sr {
	unsigned int samplerate;
	int srbit;
};

static const struct codec_sr codec_sr_s[] = {
	{44100, 0},
	{48000, 0},
	{8000, 5},
	{11025, 4},
	{12000, 4},
	{16000, 3},
	{22050, 2},
	{24000, 2},
	{32000, 1},
	{96000, 7},
	{192000, 6},
};

static struct label reg_labels[] = {
	LABEL(SUNXI_DAC_DPC),
	LABEL(SUNXI_DAC_FIFOC),
	LABEL(SUNXI_DAC_FIFOS),
	LABEL(SUNXI_ADC_FIFOC),
	LABEL(SUNXI_ADC_FIFOS),
	LABEL(SUNXI_ADC_RXDATA),
	LABEL(SUNXI_DAC_TXDATA),
	LABEL(SUNXI_DAC_CNT),
	LABEL(SUNXI_ADC_CNT),
	LABEL(SUNXI_DAC_DG),
	LABEL(SUNXI_ADC_DG),


	LABEL_END,
};

/*
*enable the codec function which should be enable during system init.
*/
static int codec_init(struct sunxi_codec *sunxi_internal_codec)
{
	struct snd_soc_codec *codec = sunxi_internal_codec->codec;

	/*
	if (sunxi_internal_codec->hp_dirused) {
		snd_soc_update_bits(codec, HP_CTRL,
					(0x3<<HPCOM_FC), (0x3<<HPCOM_FC));
		snd_soc_update_bits(codec, HP_CTRL,
					(0x1<<HPCOM_PT), (0x1<<HPCOM_PT));
	} else {
		snd_soc_update_bits(codec, HP_CTRL,
					(0x3<<HPCOM_FC), (0x0<<HPCOM_FC));
		snd_soc_update_bits(codec, HP_CTRL,
					(0x1<<HPCOM_PT), (0x0<<HPCOM_PT));
	}
	snd_soc_update_bits(codec, DAC_PA_SRC,
				(0x1<<LHPPAMUTE), (0x0<<LHPPAMUTE));
	snd_soc_update_bits(codec, DAC_PA_SRC,
				(0x1<<RHPPAMUTE), (0x0<<RHPPAMUTE));
	*/

	/*when TX FIFO available room less than or equal N,
	* DRQ Requeest will be de-asserted.
	*/
	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x3F<<HP_VOL),
					(0x0<<HP_VOL));   /* set hp volume 0*/
	snd_soc_update_bits(codec, SUNXI_DAC_FIFOC, (0x3<<DAC_DRQ_CLR_CNT),
					(0x3<<DAC_DRQ_CLR_CNT));
	snd_soc_update_bits(codec, SUNXI_DAC_FIFOC, (0x1<<FIFO_FLUSH),
					(0x1<<FIFO_FLUSH));
	/*
	*	0:64-Tap FIR
	*	1:32-Tap FIR
	*/
	snd_soc_update_bits(codec, SUNXI_DAC_FIFOC, (0x1<<FIR_VER),
					(0x0<<FIR_VER));

	snd_soc_update_bits(codec, SUNXI_ADC_FIFOC, (0x1<<ADC_FIFO_FLUSH),
					(0x1<<ADC_FIFO_FLUSH));

	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_L_MUTE),
					(0x0<<PH_L_MUTE));
	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_R_MUTE),
					(0x0<<PH_R_MUTE));
	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_COM_PROTEC),
					(0x0<<PH_COM_PROTEC));
	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x3<<PH_COM_FC),
					(0x0<<PH_COM_FC));
	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<HP_POWER_EN),
					(0x1<<HP_POWER_EN)); /* power hp */
	snd_soc_update_bits(codec, SUNXI_DAC_DPC,  (0x1<<EN_DAC),
					(0x1<<EN_DAC));

	return 0;
}

#if 0

static int late_enable_dac(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec =
				snd_soc_codec_get_drvdata(codec);
	mutex_lock(&sunxi_internal_codec->dac_mutex);
	pr_debug("..dac power state change.\n");

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->dac_enable == 0) {
			snd_soc_update_bits(codec, SUNXI_DAC_DPC,
						(0x1<<EN_DAC), (0x1<<EN_DAC));
		}
		sunxi_internal_codec->dac_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->dac_enable > 0) {
			sunxi_internal_codec->dac_enable--;
			if (sunxi_internal_codec->dac_enable == 0) {
				snd_soc_update_bits(codec, SUNXI_DAC_DPC,
						(0x1<<EN_DAC), (0x0<<EN_DAC));
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->dac_mutex);
	return 0;
}

static int late_enable_adc(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);
	mutex_lock(&sunxi_internal_codec->adc_mutex);
	pr_debug("..adc power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sunxi_internal_codec->adc_enable == 0) {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
						(0x1<<EN_AD), (0x1<<EN_AD));
		}
		sunxi_internal_codec->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sunxi_internal_codec->adc_enable > 0) {
			sunxi_internal_codec->adc_enable--;
			if (sunxi_internal_codec->adc_enable == 0) {
				snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
						(0x1<<EN_AD), (0x0<<EN_AD));
			}
		}
		break;
	}
	mutex_unlock(&sunxi_internal_codec->adc_mutex);
	return 0;
}

static int ac_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_codec *codec = w->codec;
	pr_debug("..headphone power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*open*/
		snd_soc_update_bits(codec, HP_CTRL, (0x1<<HPPAEN),
					(0x1<<HPPAEN));
		msleep(20);
		snd_soc_update_bits(codec, DAC_PA_SRC, (0x1<<LHPPAMUTE),
					(0x1<<LHPPAMUTE));
		snd_soc_update_bits(codec, DAC_PA_SRC, (0x1<<RHPPAMUTE),
					(0x1<<RHPPAMUTE));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		snd_soc_update_bits(codec, HP_CTRL, (0x1<<HPPAEN),
					(0x0<<HPPAEN));
		snd_soc_update_bits(codec, DAC_PA_SRC, (0x1<<LHPPAMUTE),
					(0x0<<LHPPAMUTE));
		snd_soc_update_bits(codec, DAC_PA_SRC, (0x1<<RHPPAMUTE),
					(0x0<<RHPPAMUTE));
		break;
	}
	return 0;
}

static int ac_speaker_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *k,
				int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);
	pr_debug("..speaker power state change.\n");
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		sunxi_internal_codec->spkenable = true;
		msleep(50);
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKLP_EN), (0x1<<SPKLP_EN));
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKLN_EN), (0x1<<SPKLN_EN));
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKRP_EN), (0x1<<SPKRP_EN));
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKRN_EN), (0x1<<SPKRN_EN));
		if (spk_gpio.cfg)
			gpio_set_value(spk_gpio.gpio, 1);

		break;
	case SND_SOC_DAPM_PRE_PMD:
		sunxi_internal_codec->spkenable = false;
		if (spk_gpio.cfg)
			gpio_set_value(spk_gpio.gpio, 0);
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKLP_EN), (0x0<<SPKLP_EN));
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKLN_EN), (0x0<<SPKLN_EN));
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKRP_EN), (0x0<<SPKRP_EN));
		snd_soc_update_bits(codec, SPK_MBIAS_CTR,
				(0x1<<SPKRN_EN), (0x0<<SPKRN_EN));
	default:
		break;
	}
	return 0;
}

#endif

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	SOC_SINGLE_TLV("dac digital volume",
			SUNXI_DAC_DPC,     DVOL,                0x3f, 0, NULL),
	SOC_SINGLE_TLV("dac: right chanel en",
			DAC_MIXER_CTRL, DAC_AG_R_EN,         1,    0, NULL),
	SOC_SINGLE_TLV("dac: left chanle en",
			DAC_MIXER_CTRL, DAC_AG_L_EN,         1,    0, NULL),
	SOC_SINGLE_TLV("dac: right analog output mixer en",
			DAC_MIXER_CTRL, R_MIXER_EN,          1,    0, NULL),
	SOC_SINGLE_TLV("dac: left analog output mixer en",
			DAC_MIXER_CTRL, L_MIXER_EN,          1,    0, NULL),
	SOC_SINGLE_TLV("dac: right mute",
			DAC_MIXER_CTRL, PH_R_MUTE,           1,    0, NULL),
	SOC_SINGLE_TLV("dac: left mute",
			DAC_MIXER_CTRL, PH_L_MUTE,           1,    0, NULL),
	SOC_SINGLE_TLV("hp right source select: 0-dac, 1-mixer",
			DAC_MIXER_CTRL, PH_R_PWR_SLT,        1,    0, NULL),
	SOC_SINGLE_TLV("hp left source select: 0-dac, 1-mixer",
			DAC_MIXER_CTRL, PH_L_PWR_SLT,        1,    0, NULL),
	SOC_SINGLE_TLV("dac: right mixer mute: mic",
			DAC_MIXER_CTRL, R_MIXER_MUTE_MIC,    1,    0, NULL),
	SOC_SINGLE_TLV("dac: right mixer mute: linein",
			DAC_MIXER_CTRL, R_MIXER_MUTE_LINEIN, 1,    0, NULL),
	SOC_SINGLE_TLV("dac: right mixer mute: FM",
			DAC_MIXER_CTRL, R_MIXER_MUTE_FM,     1,    0, NULL),
	SOC_SINGLE_TLV("dac: right mixer mute: right dac",
			DAC_MIXER_CTRL, R_MIXER_MUTE_R_DAC,  1,    0, NULL),
	SOC_SINGLE_TLV("dac: right mixer mute: left dac",
			DAC_MIXER_CTRL, R_MIXER_MUTE_L_DAC,  1,    0, NULL),
	SOC_SINGLE_TLV("head phone power",
			DAC_MIXER_CTRL, HP_POWER_EN,         1,    0, NULL),
	SOC_SINGLE_TLV("dac: left mixer mute: mic",
			DAC_MIXER_CTRL, L_MIXER_MUTE_MIC,    1,    0, NULL),
	SOC_SINGLE_TLV("dac: left mixer mute: linein",
			DAC_MIXER_CTRL, L_MIXER_MUTE_LINEIN, 1,    0, NULL),
	SOC_SINGLE_TLV("dac: left mixer mute: FM",
			DAC_MIXER_CTRL, L_MIXER_MUTE_FM,     1,    0, NULL),
	SOC_SINGLE_TLV("dac: left mixer mute: right dac",
			DAC_MIXER_CTRL, L_MIXER_MUTE_R_DAC,  1,    0, NULL),
	SOC_SINGLE_TLV("dac: left mixer mute: left dac",
			DAC_MIXER_CTRL, L_MIXER_MUTE_L_DAC,  1,    0, NULL),
	SOC_SINGLE_TLV("dac: left hpout to right hpout",
			DAC_MIXER_CTRL, L_HP_TO_R_HP_MUTE,   1,    0, NULL),
	SOC_SINGLE_TLV("dac: right hpout to left hpout",
			DAC_MIXER_CTRL, R_HP_TO_L_HP_MUTE,   1,    0, NULL),
	SOC_SINGLE_TLV("head phone volume",
			DAC_MIXER_CTRL, HP_VOL,              0x3f, 0, NULL),
	SOC_SINGLE_TLV("MICIN GAIN control",
			ADC_MIXER_CTRL, MIC_GAIN_CTL,        0x7,  0, NULL),
	SOC_SINGLE_TLV("LINEIN GAIN control",
			ADC_MIXER_CTRL, LINEIN_VOL,          0x7,  0, NULL),
	SOC_SINGLE_TLV("ADC INPUT GAIN control",
			ADC_MIXER_CTRL, ADC_IN_GAIN_CTL,     0x7,  0, NULL),
	SOC_SINGLE_TLV("COS slop time control for Anti-pop",
			ADC_MIXER_CTRL, COS_SLOP_TM,         0x3,  0, NULL),
	SOC_SINGLE_TLV("ADC mixer mute for mic",
			ADC_MIXER_CTRL, ADC_MIX_MUTE_MIC,    0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC mixer mute for FML",
			ADC_MIXER_CTRL, ADC_MIX_MUTE_FML,    0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC mixer mute for FMR",
			ADC_MIXER_CTRL, ADC_MIX_MUTE_FMR,    0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC mixer mute for linein",
			ADC_MIXER_CTRL, ADC_MIX_MUTE_LINEIN, 0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC mixer mute for left ouput",
			ADC_MIXER_CTRL, ADC_MIX_MUTE_L,      0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC mixer mute for right ouput",
			ADC_MIXER_CTRL, ADC_MIX_MUTE_R,      0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC PA speed select",
			ADC_MIXER_CTRL, PA_SPEED_SLT,        0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC FM volume",
			ADC_MIXER_CTRL, FM_TO_MIX_GAIN,      0x7,  0, NULL),
	SOC_SINGLE_TLV("ADC MIC Boost AMP en",
			ADC_MIXER_CTRL, MIC_BST_AMP_EN,      0x1,  0, NULL),
	SOC_SINGLE_TLV("ADC MIC Boost AMP gain control",
			ADC_MIXER_CTRL, MIC_BOST_GAIN,      0x7,  0, NULL),

	SOC_SINGLE_TLV("SPK_L Mux Left Mixer en",
			DAC_MIXER_CTRL, 24,        0x1,  0, NULL),

	SOC_SINGLE_TLV("SPK_R Mux Right Mixer en",
			DAC_MIXER_CTRL, 25,        0x1,  0, NULL),
};

/*built widget*/
static const struct snd_soc_dapm_widget ac_dapm_widgets[] = {
	SND_SOC_DAPM_PGA("SPK_LR Adder", SND_SOC_NOPM, 0, 0, NULL, 0),

	/*output widget*/
	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),

	SND_SOC_DAPM_OUTPUT("SPKL"),
	SND_SOC_DAPM_OUTPUT("SPKR"),

	SND_SOC_DAPM_AIF_OUT("ADC_L", "Capture", 0, SND_SOC_NOPM, 0, 0),

	/*INPUT widget*/
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),

	SND_SOC_DAPM_INPUT("LINEINR"),
	SND_SOC_DAPM_INPUT("LINEINL"),
};

static const struct snd_soc_dapm_route ac_dapm_routes[] = {
	/*PLAYBACK*/
	{"Left Output Mixer", "DACL Switch",		"DAC_L"},
	{"Left Output Mixer", "DACR Switch",		"DAC_R"},
	{"Left Output Mixer", "LINEINL Switch",		"LINEINL"},
	{"Left Output Mixer", "MIC1Booststage Switch",	"MIC1 PGA"},

	{"Right Output Mixer", "DACR Switch",		"DAC_R"},
	{"Right Output Mixer", "DACL Switch",		"DAC_L"},
	{"Right Output Mixer", "LINEINR Switch",	"LINEINR"},
	{"Right Output Mixer", "MIC1Booststage Switch",	"MIC1 PGA"},

	/*hp mux*/
	{"HP_R Mux", "DACR HPR Switch",			"DAC_R"},
	{"HP_R Mux", "MIXER_R Switch",			"Right Output Mixer"},

	{"HP_L Mux", "DACL HPL Switch",			"DAC_L"},
	{"HP_L Mux", "MIXER_L Switch",			"Left Output Mixer"},

	/*hp endpoint*/
	{"HPOUTR", NULL,				"HP_R Mux"},
	{"HPOUTL", NULL,				"HP_L Mux"},

	{"Headphone", NULL,				"HPOUTR"},
	{"Headphone", NULL,				"HPOUTL"},

	/*spk mux*/
	{"SPK_LR Adder", NULL,				"Right Output Mixer"},
	{"SPK_LR Adder", NULL,				"Left Output Mixer"},

	{"SPK_L Mux", "MIXR+MIXL",			"SPK_LR Adder"},
	{"SPK_L Mux", "MIXER_L Switch",			"Left Output Mixer"},
	{"SPK_L Mux", "MIXER_R Switch",			"Right Output Mixer"},

	{"SPK_R Mux", "MIXR+MIXL",			"SPK_LR Adder"},
	{"SPK_R Mux", "MIXER_R Switch",			"Right Output Mixer"},
	{"SPK_R Mux", "MIXER_L Switch",			"Left Output Mixer"},

	{"SPKR", NULL,					"SPK_R Mux"},
	{"SPKL", NULL,					"SPK_L Mux"},

	{"External Speaker", NULL, "SPKL"},
	{"External Speaker", NULL, "SPKR"},

	{"MIC1 PGA", NULL,				"MIC1P"},
	{"MIC1 PGA", NULL,				"MIC1N"},

	/*LADC SOURCE mixer*/
	{"LADC input Mixer", "MIC1 boost Switch",	"MIC1 PGA"},
	{"LADC input Mixer", "LINEINR Switch",		"LINEINR"},
	{"LADC input Mixer", "LINEINL Switch",		"LINEINL"},
	{"LADC input Mixer", "l_output mixer Switch",	"Left Output Mixer"},
	{"LADC input Mixer", "r_output mixer Switch",	"Right Output Mixer"},

	/*ADC--ADCMUX*/
	{"ADC_L", NULL, "LADC input Mixer"},
};

static int codec_start(struct snd_pcm_substream *substream,
			struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec;

	sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
			(0x1<<FIFO_FLUSH), (0x1<<FIFO_FLUSH));/*flush tx fifo */
		snd_soc_update_bits(codec, DAC_MIXER_CTRL,
			(0x1<<L_MIXER_MUTE_R_DAC), (0x1<<L_MIXER_MUTE_R_DAC));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL,
			(0x1<<R_MIXER_MUTE_R_DAC), (0x1<<R_MIXER_MUTE_R_DAC));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL,
			(0x1<<PH_L_PWR_SLT),       (0x0<<PH_L_PWR_SLT));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_R_PWR_SLT),
			(0x0<<PH_R_PWR_SLT));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_L_MUTE),
			(0x1<<PH_L_MUTE));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_R_MUTE),
			(0x1<<PH_R_MUTE));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<DAC_AG_L_EN),
			(0x1<<DAC_AG_L_EN));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<DAC_AG_R_EN),
			(0x1<<DAC_AG_R_EN));
	} else {
		snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,   (0x1<<EN_AD),
			(0x1<<EN_AD));
		snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
			(0x1<<ADC_FIFO_FLUSH), (0x1<<ADC_FIFO_FLUSH));
		snd_soc_update_bits(codec, ADC_MIXER_CTRL, (0x1<<ADC_EN),
			(0x1<<ADC_EN));
	}

	return 0;
}

static int codec_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec =
				snd_soc_codec_get_drvdata(codec);

	if (sunxi_internal_codec->spkenable == true)
		msleep(sunxi_internal_codec->pa_sleep_time);
	msleep(150);

	return 0;
}

static void codec_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec;

	sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,   (0x1<<FIFO_FLUSH),
			(0x1<<FIFO_FLUSH));/* flush tx fifo */
		snd_soc_update_bits(codec, DAC_MIXER_CTRL,
			(0x1<<L_MIXER_MUTE_R_DAC), (0x0<<L_MIXER_MUTE_R_DAC));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL,
			(0x1<<R_MIXER_MUTE_R_DAC), (0x0<<R_MIXER_MUTE_R_DAC));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_L_MUTE),
			(0x0<<PH_L_MUTE));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_R_MUTE),
			(0x0<<PH_R_MUTE));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<DAC_AG_L_EN),
			(0x0<<DAC_AG_L_EN));
		snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<DAC_AG_R_EN),
			(0x0<<DAC_AG_R_EN));
	} else {
		snd_soc_update_bits(codec, SUNXI_ADC_FIFOC, (0x1<<EN_AD),
			(0x0<<EN_AD));
		snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
			(0x1<<ADC_FIFO_FLUSH), (0x1<<ADC_FIFO_FLUSH));
		snd_soc_update_bits(codec, ADC_MIXER_CTRL, (0x1<<ADC_EN),
			(0x0<<ADC_EN));
	}
}

static int codec_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			/*enable dac drq*/
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x1<<DAC_DRQ_EN), (0x1<<DAC_DRQ_EN));
			return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x1<<DAC_DRQ_EN), (0x0<<DAC_DRQ_EN));
			return 0;
		default:
			return -EINVAL;
		}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x1<<ADC_DRQ_EN), (0x1<<ADC_DRQ_EN));
			return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x1<<ADC_DRQ_EN), (0x0<<ADC_DRQ_EN));
			return 0;
		default:
			pr_err("error:%s,%d\n", __func__, __LINE__);
			return -EINVAL;
		}
	}
	return 0;
}

static int codec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int i = 0;
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec;

	sunxi_internal_codec = snd_soc_codec_get_drvdata(codec);
	for (i = 0; i < ARRAY_SIZE(codec_sr_s); i++) {
		if (codec_sr_s[i].samplerate ==  params_rate(params)) {
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x7<<DAC_FS), (codec_sr_s[i].srbit<<DAC_FS));
			} else {
				snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x7<<ADFS), (codec_sr_s[i].srbit<<ADFS));
			}
			if (codec_sr_s[i].samplerate%1000) { /*22.5792MHz*/
				if (clk_set_rate(sunxi_internal_codec->pllclk,
								22579200)) {
					pr_err("[audio-codec]try to set the");
					pr_err(" pll clk rate failed!\n");
				}
			} else {/*24.576MHz*/
				if (clk_set_rate(sunxi_internal_codec->pllclk,
								24576000)) {
					pr_err("[audio-codec]try to set the");
					pr_err(" pll clk rate failed!\n");
				}
			}
			break;
		}
	}

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			/*set TX FIFO MODE:24bit*/
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x3<<FIFO_MODE), (0x0<<FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
			(0x1<<TX_SAMPLE_BITS), (0x1<<TX_SAMPLE_BITS));
		} else {
			/*set RX FIFO MODE:24bit*/
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
			(0x1<<RX_FIFO_MODE), (0x0<<RX_FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
			(0x1<<RX_SAMPLE_BITS), (0x1<<RX_SAMPLE_BITS));
		}
	break;
	case SNDRV_PCM_FORMAT_S16_LE:
	default:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			/*set TX FIFO MODE:16bit*/
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x3<<FIFO_MODE), (0x0<<FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
			(0x1<<TX_SAMPLE_BITS), (0x0<<TX_SAMPLE_BITS));
		} else {
			/*set RX FIFO MODE:16bit*/
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
			(0x1<<RX_FIFO_MODE), (0x1<<RX_FIFO_MODE));
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
			(0x1<<RX_SAMPLE_BITS), (0x0<<RX_SAMPLE_BITS));
		}
	break;
	}

	if (params_channels(params) == 1) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x1<<DAC_MONO_EN), (0x1<<DAC_MONO_EN));
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x1<<ADC_MONO_EN), (0x1<<ADC_MONO_EN));
		}
	} else {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x1<<DAC_MONO_EN), (0x0<<DAC_MONO_EN));
		} else {
			snd_soc_update_bits(codec, SUNXI_ADC_FIFOC,
				(0x1<<ADC_MONO_EN), (0x0<<ADC_MONO_EN));
		}
	}

	return 0;
}

static int codec_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

static int codec_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);

	if (clk_set_rate(sunxi_internal_codec->pllclk, freq))
		pr_err("[audio-cpudai]try to set the pll clk rate failed!\n");

	return 0;
}

static int codec_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	codec->dapm.bias_level = level;
	return 0;
}

static const struct snd_soc_dai_ops codec_dai_ops = {
	.startup		= codec_start,
	.set_fmt		= codec_set_dai_fmt,
	.hw_params		= codec_hw_params,
	.shutdown		= codec_shutdown,
	.digital_mute		= codec_mute,
	.set_sysclk		= codec_set_dai_sysclk,
	.trigger		= codec_trigger,
};

static struct snd_soc_dai_driver codec_dai[] = {
	{
		.name = "sun8iw10codec",
		.id = 1,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
					SNDRV_PCM_FMTBIT_S24_LE,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
					SNDRV_PCM_FMTBIT_S24_LE,
		 },
		.ops = &codec_dai_ops,
	},
};

static const struct snd_soc_component_driver sunxi_i2s_component = {
	.name		= DRV_NAME,
};

static int codec_soc_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);

	sunxi_internal_codec->codec = codec;
	sunxi_internal_codec->dac_enable = 0;
	sunxi_internal_codec->adc_enable = 0;
	mutex_init(&sunxi_internal_codec->dac_mutex);
	mutex_init(&sunxi_internal_codec->adc_mutex);

#if 1
	/* Add virtual switch */
	ret = snd_soc_add_codec_controls(codec, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret) {
		pr_err("[audio-codec] Failed to register audio mode control, ");
		pr_err("will continue without it.\n");
	}
#endif

	codec_init(sunxi_internal_codec);

	return 0;
}

int audio_gpio_iodisable(u32 gpio)
{
	char pin_name[8];
	u32 config, ret;
	sunxi_gpio_to_name(gpio, pin_name);
	config = (((7) << 16) | (0 & 0xFFFF));
	ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
	return ret;
}

static int codec_suspend(struct snd_soc_codec *codec)
{
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);
	pr_debug("[audio codec]:suspend start.\n");

	if (spk_gpio.cfg)
		audio_gpio_iodisable(spk_gpio.gpio);

	if (sunxi_internal_codec->vol_supply.cpvdd)
		regulator_disable(sunxi_internal_codec->vol_supply.cpvdd);

	if (sunxi_internal_codec->vol_supply.avcc)
		regulator_disable(sunxi_internal_codec->vol_supply.avcc);

	pr_debug("[audio codec]:suspend end..\n");

	return 0;
}

static int codec_resume(struct snd_soc_codec *codec)
{
	int ret;
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);
	pr_debug("[audio codec]:resume start\n");

	if (sunxi_internal_codec->vol_supply.cpvdd) {
		ret = regulator_enable(sunxi_internal_codec->vol_supply.cpvdd);
		if (ret) {
			pr_err("[%s]: cpvdd:regulator_enable() failed!\n",
				__func__);
		}
	}

	if (sunxi_internal_codec->vol_supply.avcc) {
		ret = regulator_enable(sunxi_internal_codec->vol_supply.avcc);
		if (ret) {
			pr_err("[%s]: avcc:regulator_enable() failed!\n",
				__func__);
		}
	}

	codec_init(sunxi_internal_codec);

	if (spk_gpio.cfg) {
		gpio_direction_output(spk_gpio.gpio, 1);
		gpio_set_value(spk_gpio.gpio, 0);
	}

	pr_debug("[audio codec]:resume end..\n");
	return 0;
}

/* power down chip */
static int codec_soc_remove(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x1<<PH_COM_PROTEC),
				(0x0<<PH_COM_PROTEC));
	snd_soc_update_bits(codec, DAC_MIXER_CTRL, (0x3<<PH_COM_FC),
				(0x0<<PH_COM_FC));

	return 0;
}

static unsigned int codec_read(struct snd_soc_codec *codec,
					  unsigned int reg)
{
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);

	return codec_rdreg(sunxi_internal_codec->codec_dbase + reg);
}

static int codec_write(struct snd_soc_codec *codec,
				  unsigned int reg, unsigned int value)
{
	struct sunxi_codec *sunxi_internal_codec =
					snd_soc_codec_get_drvdata(codec);

	codec_wrreg(sunxi_internal_codec->codec_dbase + reg, value);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_codec = {
	.probe   = codec_soc_probe,
	.remove  = codec_soc_remove,
	.suspend = codec_suspend,
	.resume  = codec_resume,
	.set_bias_level = codec_set_bias_level,
	.read    = codec_read,
	.write   = codec_write,
	.ignore_pmdown_time = 1,
};

static ssize_t show_audio_reg(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int count = 0;
	int i = 0;
	int reg_group = 0;

	count += sprintf(buf, "dump audio reg:\n");

	while (reg_labels[i].name != NULL) {
		if (reg_labels[i].value == 0)
			reg_group++;

		if (reg_group == 1) {
			count += sprintf(buf + count, "%s 0x%p: 0x%x\n",
				reg_labels[i].name, (codec_digitaladress +
				reg_labels[i].value), readl(codec_digitaladress
				+ reg_labels[i].value));
		}
		i++;
	}

	return count;
}

/* ex:
*param 1: 0 read;1 write
*param 2: 1 digital reg; 2 analog reg
*param 3: reg value;
*param 4: write value;
	read:
		echo 0,1,0x00 > audio_reg
		echo 0,2,0x00 > audio_reg
	write:
		echo 1,1,0x00,0xa > audio_reg
		echo 1,2,0x00,0xff > audio_reg
*/
static ssize_t store_audio_reg(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int reg_val_read;
	int input_reg_val = 0;
	int input_reg_group = 0;
	int input_reg_offset = 0;

	ret = sscanf(buf, "%d,%d,0x%x,0x%x", &rw_flag, &input_reg_group,
			&input_reg_offset, &input_reg_val);
	pr_info("ret:%d, reg_group:%d, reg_offset:%d, reg_val:0x%x\n",
			ret, input_reg_group, input_reg_offset, input_reg_val);

	if (!(input_reg_group == 1 || input_reg_group == 2)) {
		pr_err("not exist reg group\n");
		ret = count;
		goto out;
	}
	if (!(rw_flag == 1 || rw_flag == 0)) {
		pr_err("not rw_flag\n");
		ret = count;
		goto out;
	}
	if (input_reg_group == 1) {
		if (rw_flag) {
			writel(input_reg_val, codec_digitaladress +
				input_reg_offset);
		} else {
			reg_val_read = readl(codec_digitaladress +
				input_reg_offset);
		}
	}

	ret = count;

out:
	return ret;
}

static DEVICE_ATTR(audio_reg, 0644, show_audio_reg, store_audio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "audio_reg_debug",
	.attrs  = audio_debug_attrs,
};
static const struct of_device_id sunxi_codec_of_match[] = {
	{ .compatible = "allwinner,sunxi-internal-codec", },
	{},
};

static int __init sunxi_internal_codec_probe(struct platform_device *pdev)
{
	s32 ret = 0;
	u32 temp_val = 0;
	u32 pa_shdn = 0;
	const struct of_device_id *device;
	struct sunxi_codec *sunxi_internal_codec;
	struct device_node *node = pdev->dev.of_node;

	if (!node) {
		dev_err(&pdev->dev,
			"can not get dt node for this device.\n");
		ret = -EINVAL;
		goto err0;
	}
	sunxi_internal_codec = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_codec), GFP_KERNEL);
	if (!sunxi_internal_codec) {
		dev_err(&pdev->dev, "Can't allocate sunxi_codec\n");
		ret = -ENOMEM;
		goto err0;
	}
	dev_set_drvdata(&pdev->dev, sunxi_internal_codec);
	device = of_match_device(sunxi_codec_of_match, &pdev->dev);
	if (!device) {
		ret = -ENODEV;
		goto err1;
	}
	sunxi_internal_codec->pllclk = of_clk_get(node, 0);
	sunxi_internal_codec->codecclk = of_clk_get(node, 1);

	if (IS_ERR(sunxi_internal_codec->pllclk)) {
		dev_err(&pdev->dev,
				"[audio-cpudai]Can't get audio pll clock\n");
		ret = PTR_ERR(sunxi_internal_codec->pllclk);
		goto err1;
	}
	if (IS_ERR(sunxi_internal_codec->codecclk)) {
		dev_err(&pdev->dev,
				"[audio-cpudai]Can't get audio codec clock\n");
		ret = PTR_ERR(sunxi_internal_codec->codecclk);
		goto err1;
	}
	clk_prepare_enable(sunxi_internal_codec->pllclk);
	clk_prepare_enable(sunxi_internal_codec->codecclk);

	sunxi_internal_codec->codec_dbase = NULL;
	sunxi_internal_codec->codec_dbase = of_iomap(node, 0);
	if (NULL == sunxi_internal_codec->codec_dbase)
		pr_err("[audio-codec]Can't map codec digital registers\n");
	else
		codec_digitaladress = sunxi_internal_codec->codec_dbase;

	ret = of_property_read_u32(node, "headphonevol", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]headphonevol configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.headphonevol = temp_val;
	}

	ret = of_property_read_u32(node, "gpio_shdn", &pa_shdn);
	if (!gpio_is_valid(pa_shdn)) {
		pr_err("failed to get gpio\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(node, "gpio_num", &temp_val);
	if (!gpio_is_valid(temp_val)) {
		pr_err("failed to get gpio\n");
		return -EINVAL;
	}
	/*printk("gpio number:%d\n",temp_val);*/
	ret = devm_gpio_request(&pdev->dev, temp_val, "DA_POWER");
	if (ret) {
		pr_err("failed to request pwr-key gpio\n");
		return -EINVAL;
	} else {
		ret = gpio_direction_output(temp_val, pa_shdn);
		/*printk("gpio_direction_output DA_POWER return %d\n", ret);*/
	}
	ret = of_property_read_u32(node, "spkervol", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]spkervol configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.spkervol = temp_val;
	}
	ret = of_property_read_u32(node, "maingain", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]maingain configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->gain_config.maingain = temp_val;
	}
	ret = of_property_read_u32(node, "hp_dirused", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]hp_dirused configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->hp_dirused = temp_val;
	}
	ret = of_property_read_u32(node, "pa_sleep_time", &temp_val);
	if (ret < 0) {
		pr_err("[audio-codec]pa_sleep_time configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_internal_codec->pa_sleep_time = temp_val;
	}

	pr_debug("headphonevol:%d,spkervol:%d,maingain:%d,pa_sleep_time:%d\n",
		sunxi_internal_codec->gain_config.headphonevol,
		sunxi_internal_codec->gain_config.spkervol,
		sunxi_internal_codec->gain_config.maingain,
		sunxi_internal_codec->pa_sleep_time
	);

	snd_soc_register_codec(&pdev->dev, &soc_codec_dev_codec, codec_dai,
				ARRAY_SIZE(codec_dai));

	ret  = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (ret)
		pr_err("[audio-codec]failed to create attr group\n");

	return 0;
err1:
	kfree(sunxi_internal_codec);
err0:
	return ret;
}

static int __exit sunxi_internal_codec_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static void sunxi_internal_codec_shutdown(struct platform_device *pdev)
{
	if (spk_gpio.cfg)
		gpio_set_value(spk_gpio.gpio, 0);
}

static struct platform_driver sunxi_internal_codec_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_codec_of_match,
	},
	.probe = sunxi_internal_codec_probe,
	.remove = __exit_p(sunxi_internal_codec_remove),
	.shutdown = sunxi_internal_codec_shutdown,
};

module_platform_driver(sunxi_internal_codec_driver);

MODULE_DESCRIPTION("codec ALSA soc codec driver");
MODULE_AUTHOR("huanxin<huanxin@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm-codec");

