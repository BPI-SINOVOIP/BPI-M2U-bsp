/*
 * sound\soc\sunxi\audiocodec\sunxi_sndcodec.c
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
#include <linux/of.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>

#if defined CONFIG_ARCH_SUN8IW5 || defined CONFIG_ARCH_SUN8IW9
static int sunxi_sndpcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq = 22579200;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
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

	/*set system clock source freq */
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, 0);
	if (ret < 0)
		return ret;

	/*set system fmt:api2s:master aif1:slave*/
	ret = snd_soc_dai_set_fmt(cpu_dai, 0);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		pr_err("%s, line:%d\n", __func__, __LINE__);
		return ret;
	}

	return 0;
}


static struct snd_soc_ops sunxi_sndpcm_ops = {
	.hw_params = sunxi_sndpcm_hw_params,
};
#endif
static struct snd_soc_dai_link sunxi_sndpcm_dai_link = {
	.name = "audiocodec",
	.stream_name = "SUNXI-CODEC",
	.cpu_dai_name = "sunxi-codec",
	.codec_dai_name = "sndcodec",
	.platform_name = "sunxi-pcm-codec-audio",
	.codec_name = "sunxi-pcm-codec",

#if defined CONFIG_ARCH_SUN8IW5 || defined CONFIG_ARCH_SUN8IW9
	.ops = &sunxi_sndpcm_ops,
#endif
};

static struct snd_soc_card snd_soc_sunxi_sndpcm = {
	.name = "audiocodec",
	.owner = THIS_MODULE,
	.dai_link = &sunxi_sndpcm_dai_link,
	.num_links = 1,
};

static int sunxi_sndpcm_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_soc_card *card = &snd_soc_sunxi_sndpcm;
	card->dev = &pdev->dev;

	/* codec_of_node */
	sunxi_sndpcm_dai_link.codec_name = NULL;
	sunxi_sndpcm_dai_link.codec_of_node =
		of_find_compatible_node(NULL, NULL,
		"allwinner,sunxi-internal-codec");
	if (!sunxi_sndpcm_dai_link.codec_of_node) {
		dev_err(&pdev->dev, "codec_of_node missing or invalid\n");
		ret = -EINVAL;
	}


	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n",
			ret);
	}
	return ret;
}

static int __exit sunxi_sndpcm_dev_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	snd_soc_unregister_card(card);
	return 0;
}

/*data relating*/
static struct platform_device sunxi_sndpcm_device = {
	.name = "audiocodec",
	.id = PLATFORM_DEVID_NONE,
};

/*method relating*/
static struct platform_driver sunxi_sndpcm_driver = {
	.probe = sunxi_sndpcm_dev_probe,
	.remove = sunxi_sndpcm_dev_remove,
	.driver = {
		.name = "audiocodec",
		.owner = THIS_MODULE,
		.pm = &snd_soc_pm_ops,
	},
};

static int __init sunxi_sndpcm_init(void)
{
	int err = 0;

	err = platform_device_register(&sunxi_sndpcm_device);
	if (err < 0)
		return err;

	err = platform_driver_register(&sunxi_sndpcm_driver);
	if (err < 0)
		return err;

	return 0;
}
module_init(sunxi_sndpcm_init);

static void __exit sunxi_sndpcm_exit(void)
{
	platform_driver_unregister(&sunxi_sndpcm_driver);
}
module_exit(sunxi_sndpcm_exit);
MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUNXI_sndpcm ALSA SoC audio driver");
MODULE_LICENSE("GPL");
