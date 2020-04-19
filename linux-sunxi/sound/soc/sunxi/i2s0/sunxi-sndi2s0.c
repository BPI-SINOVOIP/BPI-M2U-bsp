/*
 * sound\soc\sunxi\i2s0\sunxi_sndi2s0.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
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
#include <linux/clk.h>
#include <linux/mutex.h>

#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include "sunxi-i2s0.h"
#include "sunxi-i2s0dma.h"

static bool i2s0_pcm_select;
static int i2s0_master;
static int audio_format;
static int signal_inversion;

/*
*	i2s0_pcm_select == 0:--> pcm
*	i2s0_pcm_select == 1:--> i2s
*/
static int sunxi_i2s0_set_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	i2s0_pcm_select = ucontrol->value.integer.value[0];

	if (i2s0_pcm_select) {
		audio_format = 1;
		signal_inversion = 1;
		i2s0_master = 4;
	} else {
		audio_format = 4;
		signal_inversion = 3;
		i2s0_master = 1;
	}

	return 0;
}

static int sunxi_i2s0_get_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = i2s0_pcm_select;
	return 0;
}

/* I2s Or Pcm Audio Mode Select */
static const struct snd_kcontrol_new sunxi_i2s0_controls[] = {
	SOC_SINGLE_BOOL_EXT("I2s Or Pcm Audio Mode Select format", 0,
		sunxi_i2s0_get_audio_mode, sunxi_i2s0_set_audio_mode),
};

static int sunxi_sndi2s0_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq = 22579200;

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned long sample_rate = params_rate(params);

	switch (sample_rate) {
	case 8000:
	case 16000:
	case 32000:
	case 64000:
	case 128000:
	case 12000:
	case 24000:
	case 48000:
	case 96000:
	case 192000:
		freq = 24576000;
		break;
	}

	/*set system clock source freq and set the mode as i2s0 or pcm*/
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, i2s0_pcm_select);
	if (ret < 0)
		return ret;

	/*set system clock source freq and set the mode as i2s0 or pcm*/
	ret = snd_soc_dai_set_sysclk(codec_dai, 0 , freq, i2s0_pcm_select);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
		SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	/*
	* codec clk & FRM master. AP as slave
	*/
	ret = snd_soc_dai_set_fmt(cpu_dai,
		(audio_format | (signal_inversion<<8) | (i2s0_master<<12)));
	if (ret < 0)
		return ret;


	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0)
		return ret;


	ret = snd_soc_dai_set_clkdiv(codec_dai, 0, sample_rate);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}
	/*
	*	audio_format == SND_SOC_DAIFMT_DSP_A
	*	signal_inversion<<8 == SND_SOC_DAIFMT_IB_NF
	*	i2s0_master<<12 == SND_SOC_DAIFMT_CBM_CFM
	*/
	I2S0_DBG("i2s0,audio_format:%d,SND_SOC_DAIFMT_DSP_A:%d\n", audio_format,
		SND_SOC_DAIFMT_DSP_A);

	I2S0_DBG("i2s0,signal_inversion:%d,SND_SOC_DAIFMT_IB_NF:%d\n",
		signal_inversion, SND_SOC_DAIFMT_IB_NF);

	I2S0_DBG("i2s0_master:%d,SND_SOC_DAIFMT_CBM_CFM:%d\n", i2s0_master,
		SND_SOC_DAIFMT_CBM_CFM);

	return 0;
}

/*
 * Card initialization
 */
static int sunxi_i2s0_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	int ret;

	/* Add virtual switch */
	ret = snd_soc_add_codec_controls(codec, sunxi_i2s0_controls,
		ARRAY_SIZE(sunxi_i2s0_controls));
	if (ret) {
		dev_warn(card->dev,
			"Failed to register audio mode control, "
			"will continue without it.\n");
	}
	return 0;
}

static struct snd_soc_ops sunxi_sndi2s0_ops = {
	.hw_params = sunxi_sndi2s0_hw_params,
};

static struct snd_soc_dai_link sunxi_sndi2s0_dai_link = {
	.name = "I2S0",
	.stream_name = "SUNXI-I2S0",
	.cpu_dai_name = "i2s0",
#if defined(CONFIG_ARCH_SUN8IW1) && defined(CONFIG_SND_SOC_AC100_CODEC)
	.codec_dai_name = "sndvir_audio",
	.codec_name = "vir_audio-codec.2-001a",
#else
	.codec_dai_name = "sndi2s0",
	.codec_name = "sunxi-i2s0-codec.0",
#endif
	.init = sunxi_i2s0_init,
	.platform_name = "sunxi-i2s0-pcm-audio.0",
	.ops = &sunxi_sndi2s0_ops,
};

static struct snd_soc_card snd_soc_sunxi_sndi2s0 = {
	.name = "sndi2s0",
	.owner = THIS_MODULE,
	.dai_link = &sunxi_sndi2s0_dai_link,
	.num_links = 1,
};

static int sunxi_sndi2s0_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	int val;
	struct snd_soc_card *card = &snd_soc_sunxi_sndi2s0;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *i2s0_np = of_parse_phandle(np,
		"sunxi,i2s0-controller", 0);

	sunxi_sndi2s0_dai_link.cpu_dai_name = NULL;
	sunxi_sndi2s0_dai_link.cpu_of_node = i2s0_np;

	ret = of_property_read_u32(i2s0_np, "i2s0_select", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "i2s0_select missing or invalid\n");
		i2s0_pcm_select = 0;
	} else {
		i2s0_pcm_select = val;
		dev_dbg(&pdev->dev, "i2s0_select:%d\n", val);
	}

	ret = of_property_read_u32(i2s0_np, "i2s0_master", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "i2s0_master missing or invalid\n");
		i2s0_master = 0;
	} else {
		i2s0_master = val;
		dev_dbg(&pdev->dev, "i2s0_master:%d\n", val);
	}

	ret = of_property_read_u32(i2s0_np, "audio_format", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "audio_format missing or invalid\n");
		audio_format = 0;
	} else {
		audio_format = val;
		dev_dbg(&pdev->dev, "audio_format:%d\n", val);
	}

	ret = of_property_read_u32(i2s0_np, "signal_inversion", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "signal_inversion missing or invalid\n");
		signal_inversion = 0;
	} else {
		signal_inversion = val;
		dev_dbg(&pdev->dev, "signal_inversion:%d\n", val);
	}

	card->dev = &pdev->dev;
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev,
			"snd_soc_register_card() failed: %d\n", ret);
	}
	return ret;
}

static int sunxi_sndi2s0_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	snd_soc_unregister_card(card);
	return 0;
}

static const struct of_device_id sunxi_i2s0_of_match[] = {
	{ .compatible = "allwinner,sunxi-sndi2s0", },
	{},
};

/*method relating*/
static struct platform_driver sunxi_i2s0_driver = {
	.probe = sunxi_sndi2s0_dev_probe,
	.remove = sunxi_sndi2s0_dev_remove,
	.driver = {
		.name = "sndi2s0",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_i2s0_of_match,
		.pm = &snd_soc_pm_ops,
	},
};

static int __init sunxi_sndi2s0_init(void)
{
	int err = 0;

	err = platform_driver_register(&sunxi_i2s0_driver);
	if (err < 0)
		return err;

	return 0;
}
module_init(sunxi_sndi2s0_init);

static void __exit sunxi_sndi2s0_exit(void)
{
	platform_driver_unregister(&sunxi_i2s0_driver);
}
module_exit(sunxi_sndi2s0_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUNXI_sndi2s0 ALSA SoC audio driver");
MODULE_LICENSE("GPL");
