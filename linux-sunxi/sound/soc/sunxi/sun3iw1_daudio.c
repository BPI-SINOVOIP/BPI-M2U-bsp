/*
 * sound\soc\sunxi\sunxi_daudio.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 * Liu shaohua <liushaohua@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <asm/dma.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/of_gpio.h>
#include "sunxi_dma.h"
#include "sun3iw1_daudio.h"

#define DRV_NAME "sunxi-daudio"

static bool daudio_loop_en;
static int over_sample_rate;
static int word_select_size;
static int regsave[8];

static void sunxi_snd_rxctrl_i2s0(struct snd_pcm_substream *substream,
					void __iomem *regs, int on)
{
	u32 reg_val;

	reg_val = readl(regs + SUNXI_RXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUNXI_RXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, regs + SUNXI_RXCHSEL);

	if (substream->runtime->channels == 1)
		reg_val = 0x00003200;
	else
		reg_val = 0x00003210;

	writel(reg_val, regs + SUNXI_RXCHMAP);

	/*flush RX FIFO*/
	reg_val = readl(regs + SUNXI_I2S0FCTL);
	reg_val |= SUNXI_I2S0FCTL_FRX;
	writel(reg_val, regs + SUNXI_I2S0FCTL);

	/*clear RX counter*/
	writel(0, regs + SUNXI_I2S0RXCNT);

	if (on) {
		/* I2S0 RX ENABLE */
		reg_val = readl(regs + SUNXI_I2S0CTL);
		reg_val |= SUNXI_I2S0CTL_RXEN;
		writel(reg_val, regs + SUNXI_I2S0CTL);

		/* enable DMA DRQ mode for record */
		reg_val = readl(regs + SUNXI_I2S0INT);
		reg_val |= SUNXI_I2S0INT_RXDRQEN;
		writel(reg_val, regs + SUNXI_I2S0INT);
	} else {
		/* I2S0 RX DISABLE */
		reg_val = readl(regs + SUNXI_I2S0CTL);
		reg_val &= ~SUNXI_I2S0CTL_RXEN;
		writel(reg_val, regs + SUNXI_I2S0CTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(regs + SUNXI_I2S0INT);
		reg_val &= ~SUNXI_I2S0INT_RXDRQEN;
		writel(reg_val, regs + SUNXI_I2S0INT);
	}
}

static void sunxi_snd_txctrl_i2s0(struct snd_pcm_substream *substream,
					void __iomem *regs, int on)
{
	u32 reg_val;

	reg_val = readl(regs + SUNXI_TXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUNXI_TXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, regs + SUNXI_TXCHSEL);

	reg_val = readl(regs + SUNXI_TXCHMAP);
	reg_val = 0;
	if (substream->runtime->channels == 1)
		reg_val = 0x76543200;
	else
		reg_val = 0x76543210;

	writel(reg_val, regs + SUNXI_TXCHMAP);

	reg_val = readl(regs + SUNXI_I2S0CTL);
	reg_val &= ~SUNXI_I2S0CTL_SDO0EN;
	switch (substream->runtime->channels) {
	case 1:
	case 2:
		reg_val |= SUNXI_I2S0CTL_SDO0EN;
		break;
	case 3:
	case 4:
		reg_val |= SUNXI_I2S0CTL_SDO0EN;
		break;
	case 5:
	case 6:
		reg_val |= SUNXI_I2S0CTL_SDO0EN;
		break;
	case 7:
	case 8:
		reg_val |= SUNXI_I2S0CTL_SDO0EN;
		break;
	default:
		reg_val |= SUNXI_I2S0CTL_SDO0EN;
		break;
	}
	writel(reg_val, regs + SUNXI_I2S0CTL);

	/*flush TX FIFO*/
	reg_val = readl(regs + SUNXI_I2S0FCTL);
	reg_val |= SUNXI_I2S0FCTL_FTX;
	writel(reg_val, regs + SUNXI_I2S0FCTL);

	/*clear TX counter*/
	writel(0, regs + SUNXI_I2S0TXCNT);

	if (on) {
		/* I2S0 TX ENABLE */
		reg_val = readl(regs + SUNXI_I2S0CTL);
		reg_val |= SUNXI_I2S0CTL_TXEN;
		writel(reg_val, regs + SUNXI_I2S0CTL);

		/* enable DMA DRQ mode for play */
		reg_val = readl(regs + SUNXI_I2S0INT);
		reg_val |= SUNXI_I2S0INT_TXDRQEN;
		writel(reg_val, regs + SUNXI_I2S0INT);
	} else {
		/* I2S0 TX DISABLE */
		reg_val = readl(regs + SUNXI_I2S0CTL);
		reg_val &= ~SUNXI_I2S0CTL_TXEN;
		writel(reg_val, regs + SUNXI_I2S0CTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(regs + SUNXI_I2S0INT);
		reg_val &= ~SUNXI_I2S0INT_TXDRQEN;
		writel(reg_val, regs + SUNXI_I2S0INT);
	}
}



static int sunxi_daudio_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;
	struct sunxi_i2s0_info *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);

	/*SDO ON*/
	reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0CTL);
	/*reg_val |= SUNXI_I2S0CTL_SDO0EN;*/
	if (!sunxi_daudio->tdm_config)
		reg_val &= ~SUNXI_I2S0CTL_PCM;  /* select i2s mode */
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CTL);

	/* master or slave selection */
	reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0CTL);
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		/* codec clk & frm master, ap is slave*/
		pr_debug("%s, line:%d\n", __func__, __LINE__);
		reg_val |= SUNXI_I2S0CTL_MS;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		/* codec clk & frm slave,ap is master*/
		pr_debug("%s, line:%d\n", __func__, __LINE__);
		reg_val &= ~SUNXI_I2S0CTL_MS;
		break;
	default:
		pr_err("unknwon master/slave format\n");
		return -EINVAL;
	}
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CTL);

	/* pcm or i2s0 mode selection */
	reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0CTL);
	reg_val1 = readl(sunxi_daudio->regs + SUNXI_I2S0FAT0);
	reg_val1 &= ~SUNXI_I2S0FAT0_FMT_RVD;
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:        /* I2S0 mode */
		reg_val &= ~SUNXI_I2S0CTL_PCM;
		reg_val1 |= SUNXI_I2S0FAT0_FMT_I2S0;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
		reg_val &= ~SUNXI_I2S0CTL_PCM;
		reg_val1 |= SUNXI_I2S0FAT0_FMT_RGT;
		break;
	case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
		reg_val &= ~SUNXI_I2S0CTL_PCM;
		reg_val1 |= SUNXI_I2S0FAT0_FMT_LFT;
		break;
	case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
		reg_val |= SUNXI_I2S0CTL_PCM;
		reg_val1 &= ~SUNXI_I2S0FAT0_LRCP;
		break;
	case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
		reg_val |= SUNXI_I2S0CTL_PCM;
		reg_val1 |= SUNXI_I2S0FAT0_LRCP;
		break;
	default:
		return -EINVAL;
	}
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CTL);
	writel(reg_val1, sunxi_daudio->regs + SUNXI_I2S0FAT0);

	/* DAI signal inversions */
	reg_val1 = readl(sunxi_daudio->regs + SUNXI_I2S0FAT0);
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
		reg_val1 &= ~SUNXI_I2S0FAT0_LRCP;
		reg_val1 &= ~SUNXI_I2S0FAT0_BCP;
		break;
	case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
		reg_val1 |= SUNXI_I2S0FAT0_LRCP;
		reg_val1 &= ~SUNXI_I2S0FAT0_BCP;
		break;
	case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
		reg_val1 &= ~SUNXI_I2S0FAT0_LRCP;
		reg_val1 |= SUNXI_I2S0FAT0_BCP;
		break;
	case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
		reg_val1 |= SUNXI_I2S0FAT0_LRCP;
		reg_val1 |= SUNXI_I2S0FAT0_BCP;
		break;
	}
	writel(reg_val1, sunxi_daudio->regs + SUNXI_I2S0FAT0);

	/* set FIFO control register */
	reg_val = 1 & 0x3;
	reg_val |= (1 & 0x1)<<2;
	reg_val |= SUNXI_I2S0FCTL_RXTL(0x1f);	/*RX FIFO trigger level*/
	reg_val |= SUNXI_I2S0FCTL_TXTL(0x40);	/*TX FIFO empty trigger level*/
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0FCTL);

	return 0;
}

static int sunxi_daudio_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_i2s0_info  *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	int sample_resolution;
	u32 reg_val = 0;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_resolution = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		sample_resolution = 24;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		sample_resolution = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		sample_resolution = 24;
		break;
	default:
		return -EINVAL;
	}

	reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0FAT0);
	reg_val &= ~SUNXI_I2S0FAT0_SR_RVD;
	if (sample_resolution == 16)
		reg_val |= SUNXI_I2S0FAT0_SR_16BIT;
	else if (sample_resolution == 20)
		reg_val |= SUNXI_I2S0FAT0_SR_20BIT;
	else
		reg_val |= SUNXI_I2S0FAT0_SR_24BIT;
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0FAT0);
	sunxi_daudio->samp_res = sample_resolution;

	return 0;
}

static int sunxi_daudio_trigger(struct snd_pcm_substream *substream,
					int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	u32 reg_val;
	struct sunxi_i2s0_info  *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/*Global Enable Digital Audio Interface*/
		reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0CTL);
		reg_val |= SUNXI_I2S0CTL_GEN;
		writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CTL);

		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			sunxi_snd_rxctrl_i2s0(substream,
						sunxi_daudio->regs, 1);
		else
			sunxi_snd_txctrl_i2s0(substream,
						sunxi_daudio->regs, 1);

		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/*Global disable Digital Audio Interface*/
		reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0CTL);
	    reg_val &= ~SUNXI_I2S0CTL_GEN;
		writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CTL);

		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			sunxi_snd_rxctrl_i2s0(substream,
						sunxi_daudio->regs, 0);
		else
			sunxi_snd_txctrl_i2s0(substream,
						sunxi_daudio->regs, 0);

		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int sunxi_daudio_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
				unsigned int freq, int daudio_pcm_select)
{
	struct sunxi_i2s0_info *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);


	if (clk_set_rate(sunxi_daudio->tdm_pllclk, freq))
		pr_err("try to set the i2s0_pllclk failed!\n");

	return 0;
}

static int sunxi_daudio_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id,
					int sample_rate)
{
	u32 reg_val;
	u32 mclk;
	u32 mclk_div = 0;
	u32 bclk_div = 0;
	struct sunxi_i2s0_info *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);
	mclk = over_sample_rate;
	if (sunxi_daudio->tdm_config) {  /* i2s config */
		/*mclk div calculate*/
		switch (sample_rate) {
		case 8000:
		{
			switch (mclk) {
			case 128:
				mclk_div = 24;
				break;
			case 192:
				mclk_div = 16;
				break;
			case 256:
				mclk_div = 12;
				break;
			case 384:
				mclk_div = 8;
				break;
			case 512:
				mclk_div = 6;
				break;
			case 768:
				mclk_div = 4;
				break;
			}
			break;
		}

		case 16000:
		{
			switch (mclk) {
			case 128:
				mclk_div = 12;
				break;
			case 192:
				mclk_div = 8;
				break;
			case 256:
				mclk_div = 6;
				break;
			case 384:
				mclk_div = 4;
				break;
			case 768:
				mclk_div = 2;
				break;
			}
			break;
		}

		case 32000:
		{
			switch (mclk) {
			case 128:
				mclk_div = 6;
				break;
			case 192:
				mclk_div = 4;
				break;
			case 384:
				mclk_div = 2;
				break;
			case 768:
				mclk_div = 1;
				break;
			}
			break;
		}

		case 64000:
		{
			switch (mclk) {
			case 192:
				mclk_div = 2;
				break;
			case 384:
				mclk_div = 1;
				break;
			}
			break;
		}

		case 128000:
		{
			switch (mclk) {
			case 192:
				mclk_div = 1;
				break;
			}
			break;
		}

		case 11025:
		case 12000:
		{
			switch (mclk) {
			case 128:
				mclk_div = 16;
				break;
			case 256:
				mclk_div = 8;
				break;
			case 512:
				mclk_div = 4;
				break;
			}
			break;
		}

		case 22050:
		case 24000:
		{
			switch (mclk) {
			case 128:
				mclk_div = 8;
				break;
			case 256:
				mclk_div = 4;
				break;
			case 512:
				mclk_div = 2;
				break;
			}
			break;
		}

		case 44100:
		case 48000:
		{
			switch (mclk) {
			case 128:
				mclk_div = 4;
				break;
			case 256:
				mclk_div = 2;
				break;
			case 512:
				mclk_div = 1;
				break;
			}
			break;
		}

		case 88200:
		case 96000:
		{
			switch (mclk) {
			case 128:
				mclk_div = 2;
				break;
			case 256:
				mclk_div = 1;
				break;
			}
			break;
		}

		case 176400:
		case 192000:
		{
			mclk_div = 1;
			break;
		}

		}

		/*bclk div caculate*/
		bclk_div = mclk/(2*word_select_size);
	} else {
		mclk_div = 2;
		bclk_div = 6;
	}
	/*calculate MCLK Divide Ratio*/
	switch (mclk_div) {
	case 1:
		mclk_div = 0;
		break;
	case 2:
		mclk_div = 1;
		break;
	case 4:
		mclk_div = 2;
		break;
	case 6:
		mclk_div = 3;
		break;
	case 8:
		mclk_div = 4;
		break;
	case 12:
		mclk_div = 5;
		break;
	case 16:
		mclk_div = 6;
		break;
	case 24:
		mclk_div = 7;
		break;
	case 32:
		mclk_div = 8;
		break;
	case 48:
		mclk_div = 9;
		break;
	case 64:
		mclk_div = 0xA;
		break;
	}
	mclk_div &= 0xf;

	/*calculate BCLK Divide Ratio*/
	switch (bclk_div) {
	case 2:
		bclk_div = 0;
		break;
	case 4:
		bclk_div = 1;
		break;
	case 6:
		bclk_div = 2;
		break;
	case 8:
		bclk_div = 3;
		break;
	case 12:
		bclk_div = 4;
		break;
	case 16:
		bclk_div = 5;
		break;
	case 32:
		bclk_div = 6;
		break;
	case 64:
		bclk_div = 7;
		break;
	}
	bclk_div &= 0x7;

	/*set mclk and bclk dividor register*/
	reg_val = mclk_div;
	reg_val |= (bclk_div<<4);
	reg_val |= (0x1<<7);
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CLKD);

	/* word select size */
	reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0FAT0);
	reg_val &= ~SUNXI_I2S0FAT0_WSS_32BCLK;
	if (word_select_size == 16)
		reg_val |= SUNXI_I2S0FAT0_WSS_16BCLK;
	else if (word_select_size == 20)
		reg_val |= SUNXI_I2S0FAT0_WSS_20BCLK;
	else if (word_select_size == 24)
		reg_val |= SUNXI_I2S0FAT0_WSS_24BCLK;
	else
		reg_val |= SUNXI_I2S0FAT0_WSS_32BCLK;

	reg_val &= ~SUNXI_I2S0FAT0_SR_RVD;
	if (sunxi_daudio->samp_res == 16)
		reg_val |= SUNXI_I2S0FAT0_SR_16BIT;
	else if (sunxi_daudio->samp_res == 20)
		reg_val |= SUNXI_I2S0FAT0_SR_20BIT;
	else
		reg_val |= SUNXI_I2S0FAT0_SR_24BIT;
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0FAT0);

	/* PCM REGISTER setup */
	reg_val = sunxi_daudio->pcm_txtype&0x3;
	reg_val |= sunxi_daudio->pcm_rxtype<<2;

	if (sunxi_daudio->pcm_sync_type)
		reg_val |= SUNXI_I2S0FAT1_SSYNC;

	if (sunxi_daudio->slot_width_select == 16)
		reg_val |= SUNXI_I2S0FAT1_SW;

	reg_val |= (sunxi_daudio->pcm_start_slot & 0x3)<<6;

	reg_val |= sunxi_daudio->pcm_lsb_first<<9;

	if (sunxi_daudio->pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sunxi_daudio->pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sunxi_daudio->pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sunxi_daudio->pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0FAT1);

	return 0;
}

static int sunxi_daudio_dai_probe(struct snd_soc_dai *dai)
{
	struct sunxi_i2s0_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	dai->capture_dma_data = &sunxi_daudio->capture_dma_param;
	dai->playback_dma_data = &sunxi_daudio->play_dma_param;
	return 0;
}

static int sunxi_daudio_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static void i2s0regsave(void __iomem   *regs)
{
	regsave[0] = readl(regs + SUNXI_I2S0CTL);
	regsave[1] = readl(regs + SUNXI_I2S0FAT0);
	regsave[2] = readl(regs + SUNXI_I2S0FAT1);
	regsave[3] = readl(regs + SUNXI_I2S0FCTL) | (0x3<<24);
	regsave[4] = readl(regs + SUNXI_I2S0INT);
	regsave[5] = readl(regs + SUNXI_I2S0CLKD);
	regsave[6] = readl(regs + SUNXI_TXCHSEL);
	regsave[7] = readl(regs + SUNXI_TXCHMAP);
}
static void i2s0regrestore(void __iomem   *regs)
{
	writel(regsave[0], regs + SUNXI_I2S0CTL);
	writel(regsave[1], regs + SUNXI_I2S0FAT0);
	writel(regsave[2], regs + SUNXI_I2S0FAT1);
	writel(regsave[3], regs + SUNXI_I2S0FCTL);
	writel(regsave[4], regs + SUNXI_I2S0INT);
	writel(regsave[5], regs + SUNXI_I2S0CLKD);
	writel(regsave[6], regs + SUNXI_TXCHSEL);
	writel(regsave[7], regs + SUNXI_TXCHMAP);
}

static int sunxi_daudio_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	int ret = 0;
	struct sunxi_i2s0_info *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);

	pr_debug("[I2S0]Entered %s\n", __func__);

	/*Global disable Digital Audio Interface*/
	reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0CTL);
	reg_val &= ~SUNXI_I2S0CTL_GEN;
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CTL);

	i2s0regsave(sunxi_daudio->regs);
	if ((NULL == sunxi_daudio->tdm_moduleclk) ||
		(IS_ERR(sunxi_daudio->tdm_moduleclk))) {
		pr_err("i2s0_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable_unprepare(sunxi_daudio->tdm_moduleclk);
	}

	if (sunxi_daudio->pinstate_sleep) {
		ret = pinctrl_select_state(sunxi_daudio->pinctrl,
						sunxi_daudio->pinstate_sleep);
		if (ret) {
			pr_warn("[daudio]select pin sleep state failed\n");
			return ret;
		}
	}
	/*release gpio resource*/
	if (sunxi_daudio->pinctrl != NULL)
		devm_pinctrl_put(sunxi_daudio->pinctrl);
	sunxi_daudio->pinctrl = NULL;
	sunxi_daudio->pinstate = NULL;
	sunxi_daudio->pinstate_sleep = NULL;

	return 0;
}

static int sunxi_daudio_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	int ret = 0;
	struct sunxi_i2s0_info *sunxi_daudio = snd_soc_dai_get_drvdata(cpu_dai);

	pr_debug("[I2S0]Entered %s\n", __func__);

	if (!sunxi_daudio->pinctrl) {
		sunxi_daudio->pinctrl = devm_pinctrl_get(cpu_dai->dev);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinctrl)) {
			pr_warn(
			"[daudio]request pinctrl handle for audio failed\n");
			return -EINVAL;
		}
	}
	if (!sunxi_daudio->pinstate) {
		sunxi_daudio->pinstate = pinctrl_lookup_state(
				sunxi_daudio->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate)) {
			pr_warn("[daudio]lookup pin default state failed\n");
			return -EINVAL;
		}
	}
	if (!sunxi_daudio->pinstate_sleep) {
		sunxi_daudio->pinstate_sleep = pinctrl_lookup_state(
				sunxi_daudio->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate_sleep)) {
			pr_warn("[daudio]lookup pin default state failed\n");
			return -EINVAL;
		}
	}
	ret = pinctrl_select_state(sunxi_daudio->pinctrl,
					sunxi_daudio->pinstate);
	if (ret) {
		pr_warn("[daudio]select pin default state failed\n");
		return ret;
	}

	/*release the module clock*/
	if (clk_prepare_enable(sunxi_daudio->tdm_moduleclk))
		pr_err("try to enable i2s0_moduleclk output failed!\n");

	i2s0regrestore(sunxi_daudio->regs);

	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sunxi_daudio->regs + SUNXI_I2S0CTL);
	reg_val |= SUNXI_I2S0CTL_GEN;
	writel(reg_val, sunxi_daudio->regs + SUNXI_I2S0CTL);

	return 0;
}

#define SUNXI_DAUDIO_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_daudio_dai_ops = {
	.trigger	= sunxi_daudio_trigger,
	.hw_params	= sunxi_daudio_hw_params,
	.set_fmt	= sunxi_daudio_set_fmt,
	.set_clkdiv = sunxi_daudio_set_clkdiv,
	.set_sysclk = sunxi_daudio_set_sysclk,
};

static struct snd_soc_dai_driver sunxi_daudio_dai = {
	.probe		= sunxi_daudio_dai_probe,
	.suspend	= sunxi_daudio_suspend,
	.resume		= sunxi_daudio_resume,
	.remove		= sunxi_daudio_dai_remove,
	.playback	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE
				| SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops		= &sunxi_daudio_dai_ops,
};
static const struct of_device_id sunxi_daudio_of_match[] = {
	{ .compatible = "allwinner,sunxi-daudio", },
	{},
};
static const struct snd_soc_component_driver sunxi_daudio_component = {
	.name		= DRV_NAME,
};
static int sunxi_daudio_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource res;
	struct device_node *node = pdev->dev.of_node;
	const struct of_device_id *device;
	void __iomem  *sunxi_daudio_membase = NULL;
	struct sunxi_i2s0_info  *sunxi_daudio;
	u32 temp_val;
	sunxi_daudio = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_i2s0_info),
					GFP_KERNEL);
	if (!sunxi_daudio) {
		dev_err(&pdev->dev, "Can't allocate sunxi_daudio\n");
		ret = -ENOMEM;
		goto err0;
	}
	dev_set_drvdata(&pdev->dev, sunxi_daudio);
	sunxi_daudio->dai = sunxi_daudio_dai;
	sunxi_daudio->dai.name = dev_name(&pdev->dev);

	device = of_match_device(sunxi_daudio_of_match, &pdev->dev);
	if (!device)
		return -ENODEV;

	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse device node resource\n");
		return -ENODEV;
	}
	sunxi_daudio_membase = ioremap(res.start, resource_size(&res));

	if (NULL == sunxi_daudio_membase)
		pr_err("[audio-daudio]Can't map i2s registers\n");
	else
		sunxi_daudio->regs = sunxi_daudio_membase;

	sunxi_daudio->tdm_pllclk = of_clk_get(node, 0);
	sunxi_daudio->tdm_moduleclk = of_clk_get(node, 1);
	if (IS_ERR(sunxi_daudio->tdm_pllclk) ||
		IS_ERR(sunxi_daudio->tdm_moduleclk)) {
		dev_err(&pdev->dev, "[audio-daudio]Can't get daudio clocks\n");
		if (IS_ERR(sunxi_daudio->tdm_pllclk))
			ret = PTR_ERR(sunxi_daudio->tdm_pllclk);
		else
			ret = PTR_ERR(sunxi_daudio->tdm_moduleclk);
		goto err1;
	} else {
		clk_prepare_enable(sunxi_daudio->tdm_pllclk);
		if (clk_set_parent(sunxi_daudio->tdm_moduleclk,
					sunxi_daudio->tdm_pllclk)) {
			pr_err("try to set parent of ");
			pr_err("sunxi_daudio->tdm_moduleclk to ");
			pr_err("sunxi_daudio->tdm_pllclk failed! line = %d\n",
				__LINE__);
		}
		if (clk_set_rate(sunxi_daudio->tdm_moduleclk, 24576000/8)) {
			pr_err("set i2s0_moduleclk clock freq to ");
			pr_err("24576000 failed! line = %d\n", __LINE__);
		}
		clk_prepare_enable(sunxi_daudio->tdm_moduleclk);
	}
	ret = of_property_read_u32(node, "tdm_num", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]tdm_num config missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->tdm_num = temp_val;
	}

	sunxi_daudio->play_dma_param.dma_addr = res.start + SUNXI_I2S0TXFIFO;
	if (0 == sunxi_daudio->tdm_num)
		sunxi_daudio->play_dma_param.dma_drq_type_num =
						DRQDST_DAUDIO_0_TX;

	sunxi_daudio->play_dma_param.dst_maxburst = 4;
	sunxi_daudio->play_dma_param.src_maxburst = 4;

	sunxi_daudio->capture_dma_param.dma_addr = res.start + SUNXI_I2S0RXFIFO;
	if (0 == sunxi_daudio->tdm_num)
		sunxi_daudio->capture_dma_param.dma_drq_type_num =
							DRQSRC_DAUDIO_0_RX;

	sunxi_daudio->capture_dma_param.src_maxburst = 4;
	sunxi_daudio->capture_dma_param.dst_maxburst = 4;
	sunxi_daudio->pinctrl = NULL;

	if (!sunxi_daudio->pinctrl) {
		sunxi_daudio->pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinctrl)) {
			pr_warn("[daudio]request pinctrl handle ");
			pr_warn("for audio failed\n");
			return -EINVAL;
		}
	}
	if (!sunxi_daudio->pinstate) {
		sunxi_daudio->pinstate = pinctrl_lookup_state(
				sunxi_daudio->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate)) {
			pr_warn("[daudio]lookup pin default state failed\n");
			return -EINVAL;
		}
	}
	if (!sunxi_daudio->pinstate_sleep) {
		sunxi_daudio->pinstate_sleep = pinctrl_lookup_state(
				sunxi_daudio->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_daudio->pinstate_sleep)) {
			pr_warn("[daudio]lookup pin default state failed\n");
			return -EINVAL;
		}
	}
	pinctrl_select_state(sunxi_daudio->pinctrl, sunxi_daudio->pinstate);

	ret = of_property_read_u32(node, "word_select_size", &word_select_size);
	if (ret < 0) {
		pr_err("[audio-daudio]word_select_size configurations ");
		pr_err("missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	}

	ret = of_property_read_u32(node, "over_sample_rate", &over_sample_rate);
	if (ret < 0) {
		pr_err("[audio-daudio]over_sample_rate configurations ");
		pr_err("missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	}

	ret = of_property_read_u32(node, "pcm_sync_period", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]pcm_sync_period configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->pcm_sync_period = temp_val;
	}

	ret = of_property_read_u32(node, "pcm_lsb_first", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]pcm_lsb_first configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->pcm_lsb_first = temp_val;
	}

	ret = of_property_read_u32(node, "pcm_start_slot", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]pcm_start_slot configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->pcm_start_slot = temp_val;
	}

	ret = of_property_read_u32(node, "slot_width_select", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]slot_width_select configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->slot_width_select = temp_val;
	}

	ret = of_property_read_u32(node, "pcm_sync_type", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]pcm_sync_type configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->pcm_sync_type = temp_val;
	}

	ret = of_property_read_u32(node, "tx_data_mode", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]tx_data_mode configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->tx_data_mode = temp_val;
	}

	ret = of_property_read_u32(node, "rx_data_mode", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]rx_data_mode configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->rx_data_mode = temp_val;
	}

	ret = of_property_read_u32(node, "tdm_config", &temp_val);
	if (ret < 0) {
		pr_err("[audio-daudio]tdm_config configurations");
		pr_err(" missing or invalid.\n");
		ret = -EINVAL;
		goto err1;
	} else {
		sunxi_daudio->tdm_config = temp_val;
	}


	ret = snd_soc_register_component(&pdev->dev, &sunxi_daudio_component,
					&sunxi_daudio->dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err1;
	}
	ret = asoc_dma_platform_register(&pdev->dev, 0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		goto err2;
	}

	return 0;
err2:
	snd_soc_unregister_component(&pdev->dev);
err1:
	iounmap(sunxi_daudio->regs);
err0:
	return ret;
}

static int sunxi_daudio_platform_remove(struct platform_device *pdev)
{
	struct sunxi_i2s0_info  *sunxi_daudio;

	sunxi_daudio = dev_get_drvdata(&pdev->dev);
	clk_disable_unprepare(sunxi_daudio->tdm_moduleclk);
	clk_disable_unprepare(sunxi_daudio->tdm_pllclk);
	snd_soc_unregister_component(&pdev->dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver sunxi_daudio_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_daudio_of_match,
	},
	.probe = sunxi_daudio_platform_probe,
	.remove = sunxi_daudio_platform_remove,
};
module_platform_driver(sunxi_daudio_driver);
module_param_named(daudio_loop_en, daudio_loop_en, bool, S_IRUGO | S_IWUSR);
/* Module information */
MODULE_AUTHOR("huangxin,liushaohua");
MODULE_DESCRIPTION("sunxi DAUDIO SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);

