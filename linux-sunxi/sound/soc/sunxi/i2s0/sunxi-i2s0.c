/*
 * sound\soc\sunxi\i2s0\sunxi-i2s0.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <linux/gpio.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include "sunxi-i2s0dma.h"
#include "sunxi-i2s0.h"

struct sunxi_i2s0_info sunxi_i2s0;
static unsigned int pin_count;
static struct pinctrl *i2s0_pinctrl;
static struct pinctrl_state *i2s0_pinstate;
static struct pinctrl_state *i2s0_pinstate_sleep;

static int regsave[8];
static int i2s0_used;
static int i2s0_select;
static int over_sample_rate;
static int sample_resolution;
static int word_select_size;
static int pcm_sync_period;
static int msb_lsb_first;
static int sign_extend;
static int slot_index;
static int slot_width;
static int frame_width;
static int tx_data_mode;
static int rx_data_mode;

/*
* sun8iw1,sun8iw3,sun8iw5,sun8iw6.i2s0's pll source is PLL2.
* sun9iw1: i2s0's pll source is PLL3
*/
static struct clk *i2s0_pllclk;
static struct clk *i2s0_pllx8;
static struct clk *i2s0_moduleclk;

static struct sunxi_dma_params sunxi_i2s0_pcm_stereo_out = {
	.name = "i2s0_play",
	.dma_addr = SUNXI_I2S0BASE + SUNXI_I2S0TXFIFO,/*send data address*/
};

static struct sunxi_dma_params sunxi_i2s0_pcm_stereo_in = {
	.name = "i2s0_capture",
	.dma_addr = SUNXI_I2S0BASE + SUNXI_I2S0RXFIFO,/*accept data address*/
};

static void sunxi_snd_txctrl_i2s0(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;
	/*for test*/
	/*
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	reg_val |= SUNXI_I2S0CTL_LOOP;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);
	*/

	reg_val = readl(sunxi_i2s0.regs + SUNXI_TXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUNXI_TXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sunxi_i2s0.regs + SUNXI_TXCHSEL);

	reg_val = readl(sunxi_i2s0.regs + SUNXI_TXCHMAP);
	reg_val = 0;
	if (substream->runtime->channels == 1)
		reg_val = 0x76543200;
	else
		reg_val = 0x76543210;

	writel(reg_val, sunxi_i2s0.regs + SUNXI_TXCHMAP);

	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	reg_val &= ~SUNXI_I2S0CTL_SDO3EN;
	reg_val &= ~SUNXI_I2S0CTL_SDO2EN;
	reg_val &= ~SUNXI_I2S0CTL_SDO1EN;
	reg_val &= ~SUNXI_I2S0CTL_SDO0EN;
	switch (substream->runtime->channels) {
	case 1:
	case 2:
		reg_val |= SUNXI_I2S0CTL_SDO0EN;
		break;
	case 3:
	case 4:
		reg_val |= SUNXI_I2S0CTL_SDO0EN | SUNXI_I2S0CTL_SDO1EN;
		break;
	case 5:
	case 6:
		reg_val |= SUNXI_I2S0CTL_SDO0EN | SUNXI_I2S0CTL_SDO1EN |
			SUNXI_I2S0CTL_SDO2EN;
		break;
	case 7:
	case 8:
		reg_val |= SUNXI_I2S0CTL_SDO0EN | SUNXI_I2S0CTL_SDO1EN |
			SUNXI_I2S0CTL_SDO2EN | SUNXI_I2S0CTL_SDO3EN;
		break;
	default:
		reg_val |= SUNXI_I2S0CTL_SDO0EN;
		break;
	}
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

	/*flush TX FIFO*/
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0FCTL);
	reg_val |= SUNXI_I2S0FCTL_FTX;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FCTL);

	/*clear TX counter*/
	writel(0, sunxi_i2s0.regs + SUNXI_I2S0TXCNT);

	if (on) {
		/* I2S0 TX ENABLE */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
		reg_val |= SUNXI_I2S0CTL_TXEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

		/* enable DMA DRQ mode for play */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0INT);
		reg_val |= SUNXI_I2S0INT_TXDRQEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0INT);
	} else {
		/* I2S0 TX DISABLE */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
		reg_val &= ~SUNXI_I2S0CTL_TXEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0INT);
		reg_val &= ~SUNXI_I2S0INT_TXDRQEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0INT);
	}
}

static void sunxi_snd_rxctrl_i2s0(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;

	reg_val = readl(sunxi_i2s0.regs + SUNXI_RXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUNXI_RXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sunxi_i2s0.regs + SUNXI_RXCHSEL);

	reg_val = readl(sunxi_i2s0.regs + SUNXI_RXCHMAP);
	reg_val = 0;
	if (substream->runtime->channels == 1)
		reg_val = 0x00003200;
	else
		reg_val = 0x00003210;

	writel(reg_val, sunxi_i2s0.regs + SUNXI_RXCHMAP);

	/*flush RX FIFO*/
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0FCTL);
	reg_val |= SUNXI_I2S0FCTL_FRX;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FCTL);

	/*clear RX counter*/
	writel(0, sunxi_i2s0.regs + SUNXI_I2S0RXCNT);

	if (on) {
		/* I2S0 RX ENABLE */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
		reg_val |= SUNXI_I2S0CTL_RXEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

		/* enable DMA DRQ mode for record */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0INT);
		reg_val |= SUNXI_I2S0INT_RXDRQEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0INT);
	} else {
		/* I2S0 RX DISABLE */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
		reg_val &= ~SUNXI_I2S0CTL_RXEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0INT);
		reg_val &= ~SUNXI_I2S0INT_RXDRQEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0INT);
	}
}

static int sunxi_i2s0_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val = 0;
	u32 reg_val1 = 0;

	/*SDO ON*/
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	reg_val |= (SUNXI_I2S0CTL_SDO0EN | SUNXI_I2S0CTL_SDO1EN |
		SUNXI_I2S0CTL_SDO2EN | SUNXI_I2S0CTL_SDO3EN);

	if (!i2s0_select)
		reg_val |= SUNXI_I2S0CTL_PCM;

	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

	/* master or slave selection */
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:/* codec clk & frm master, ap is slave*/
		pr_debug("%s, line:%d\n", __func__, __LINE__);
		reg_val |= SUNXI_I2S0CTL_MS;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:/* codec clk & frm slave,ap is master*/
		pr_debug("%s, line:%d\n", __func__, __LINE__);
		reg_val &= ~SUNXI_I2S0CTL_MS;
		break;
	default:
		pr_err("unknwon master/slave format\n");
		return -EINVAL;
	}
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

	/* pcm or i2s0 mode selection */
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	reg_val1 = readl(sunxi_i2s0.regs + SUNXI_I2S0FAT0);
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
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);
	writel(reg_val1, sunxi_i2s0.regs + SUNXI_I2S0FAT0);

	/* DAI signal inversions */
	reg_val1 = readl(sunxi_i2s0.regs + SUNXI_I2S0FAT0);
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
	writel(reg_val1, sunxi_i2s0.regs + SUNXI_I2S0FAT0);

	/* set FIFO control register */
	reg_val = 1 & 0x3;
	reg_val |= (1 & 0x1)<<2;
	reg_val |= SUNXI_I2S0FCTL_RXTL(0x1f);/*RX FIFO trigger level*/
	reg_val |= SUNXI_I2S0FCTL_TXTL(0x40);/*TX FIFO empty trigger level*/
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FCTL);
	return 0;
}

static int sunxi_i2s0_hw_params(struct snd_pcm_substream *substream,
struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_data;
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
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0FAT0);
	sunxi_i2s0.samp_res = sample_resolution;
	reg_val &= ~SUNXI_I2S0FAT0_SR_RVD;
	if (sunxi_i2s0.samp_res == 16)
		reg_val |= SUNXI_I2S0FAT0_SR_16BIT;
	else if (sunxi_i2s0.samp_res == 20)
		reg_val |= SUNXI_I2S0FAT0_SR_20BIT;
	else
		reg_val |= SUNXI_I2S0FAT0_SR_24BIT;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FAT0);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0FCTL);
		reg_val &= ~SUNXI_I2S0FCTL_TXIM_MOD1;
		if (sample_resolution == 24)
			reg_val &= ~SUNXI_I2S0FCTL_TXIM_MOD1;
		else
			reg_val |= SUNXI_I2S0FCTL_TXIM_MOD1;

		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FCTL);
	} else {
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0FCTL);
		reg_val &= ~SUNXI_I2S0FCTL_RXOM_MOD3;
		if (sample_resolution == 24)
			reg_val &= ~SUNXI_I2S0FCTL_RXOM_MOD3;
		else
			reg_val |= SUNXI_I2S0FCTL_RXOM_MOD0;

		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FCTL);
	}
	/* play or record */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sunxi_i2s0_pcm_stereo_out;
	else
		dma_data = &sunxi_i2s0_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}

static int sunxi_i2s0_trigger(struct snd_pcm_substream *substream,
	int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	u32 reg_val;
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			sunxi_snd_rxctrl_i2s0(substream, 1);
		else
			sunxi_snd_txctrl_i2s0(substream, 1);

		/*Global Enable Digital Audio Interface*/
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
		reg_val |= SUNXI_I2S0CTL_GEN;
		/*reg_val |= SUNXI_I2S0CTL_LOOP;*/
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			sunxi_snd_rxctrl_i2s0(substream, 0);
		else
			sunxi_snd_txctrl_i2s0(substream, 0);

		/*Global disable Digital Audio Interface*/
		/*
		reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
		reg_val &= ~SUNXI_I2S0CTL_GEN;
		writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);
		*/
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int sunxi_i2s0_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
	unsigned int freq, int i2s0_pcm_select)
{
	if (clk_set_rate(i2s0_pllclk, freq))
		pr_err("try to set the i2s0_pllclk failed!\n");

	i2s0_select = i2s0_pcm_select;

	return 0;
}

static int sunxi_i2s0_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int sample_rate)
{
	u32 reg_val;
	u32 mclk;
	u32 mclk_div = 0;
	u32 bclk_div = 0;

	mclk = over_sample_rate;

	if (i2s0_select) {
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
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CLKD);

	/* word select size */
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0FAT0);
	sunxi_i2s0.ws_size = word_select_size;
	reg_val &= ~SUNXI_I2S0FAT0_WSS_32BCLK;
	if (sunxi_i2s0.ws_size == 16)
		reg_val |= SUNXI_I2S0FAT0_WSS_16BCLK;
	else if (sunxi_i2s0.ws_size == 20)
		reg_val |= SUNXI_I2S0FAT0_WSS_20BCLK;
	else if (sunxi_i2s0.ws_size == 24)
		reg_val |= SUNXI_I2S0FAT0_WSS_24BCLK;
	else
		reg_val |= SUNXI_I2S0FAT0_WSS_32BCLK;

	sunxi_i2s0.samp_res = sample_resolution;
	reg_val &= ~SUNXI_I2S0FAT0_SR_RVD;
	if (sunxi_i2s0.samp_res == 16)
		reg_val |= SUNXI_I2S0FAT0_SR_16BIT;
	else if (sunxi_i2s0.samp_res == 20)
		reg_val |= SUNXI_I2S0FAT0_SR_20BIT;
	else
		reg_val |= SUNXI_I2S0FAT0_SR_24BIT;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FAT0);

	/* PCM REGISTER setup */
	sunxi_i2s0.pcm_txtype = tx_data_mode;
	sunxi_i2s0.pcm_rxtype = rx_data_mode;
	reg_val = sunxi_i2s0.pcm_txtype&0x3;
	reg_val |= sunxi_i2s0.pcm_rxtype<<2;

	sunxi_i2s0.pcm_sync_type = frame_width;
	if (sunxi_i2s0.pcm_sync_type)
		reg_val |= SUNXI_I2S0FAT1_SSYNC;

	sunxi_i2s0.pcm_sw = slot_width;
	if (sunxi_i2s0.pcm_sw == 16)
		reg_val |= SUNXI_I2S0FAT1_SW;

	sunxi_i2s0.pcm_start_slot = slot_index;
	reg_val |= (sunxi_i2s0.pcm_start_slot & 0x3)<<6;

	sunxi_i2s0.pcm_lsb_first = msb_lsb_first;
	reg_val |= sunxi_i2s0.pcm_lsb_first<<9;

	sunxi_i2s0.pcm_sync_period = pcm_sync_period;
	if (sunxi_i2s0.pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sunxi_i2s0.pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sunxi_i2s0.pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sunxi_i2s0.pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0FAT1);

	I2S0_DBG("%s, line:%d, slot_index:%d\n",
		__func__, __LINE__, slot_index);
	I2S0_DBG("%s, line:%d, slot_width:%d\n",
		__func__, __LINE__, slot_width);
	I2S0_DBG("%s, line:%d, i2s0_select:%d\n",
		__func__, __LINE__, i2s0_select);
	I2S0_DBG("%s, line:%d, frame_width:%d\n",
		__func__, __LINE__, frame_width);
	I2S0_DBG("%s, line:%d, sign_extend:%d\n",
		__func__, __LINE__, sign_extend);
	I2S0_DBG("%s, line:%d, tx_data_mode:%d\n",
		__func__, __LINE__, tx_data_mode);
	I2S0_DBG("%s, line:%d, rx_data_mode:%d\n",
		__func__, __LINE__, rx_data_mode);
	I2S0_DBG("%s, line:%d, msb_lsb_first:%d\n",
		__func__, __LINE__, msb_lsb_first);
	I2S0_DBG("%s, line:%d, pcm_sync_period:%d\n",
		__func__, __LINE__, pcm_sync_period);
	I2S0_DBG("%s, line:%d, word_select_size:%d\n",
		__func__, __LINE__, word_select_size);
	I2S0_DBG("%s, line:%d, over_sample_rate:%d\n",
		__func__, __LINE__, over_sample_rate);
	I2S0_DBG("%s, line:%d, sample_resolution:%d\n",
		__func__, __LINE__, sample_resolution);

	return 0;
}

static int sunxi_i2s0_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}

static int sunxi_i2s0_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static void i2s0regsave(void)
{
	regsave[0] = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	regsave[1] = readl(sunxi_i2s0.regs + SUNXI_I2S0FAT0);
	regsave[2] = readl(sunxi_i2s0.regs + SUNXI_I2S0FAT1);
	regsave[3] = readl(sunxi_i2s0.regs + SUNXI_I2S0FCTL) | (0x3<<24);
	regsave[4] = readl(sunxi_i2s0.regs + SUNXI_I2S0INT);
	regsave[5] = readl(sunxi_i2s0.regs + SUNXI_I2S0CLKD);
	regsave[6] = readl(sunxi_i2s0.regs + SUNXI_TXCHSEL);
	regsave[7] = readl(sunxi_i2s0.regs + SUNXI_TXCHMAP);
}

static void i2s0regrestore(void)
{
	writel(regsave[0], sunxi_i2s0.regs + SUNXI_I2S0CTL);
	writel(regsave[1], sunxi_i2s0.regs + SUNXI_I2S0FAT0);
	writel(regsave[2], sunxi_i2s0.regs + SUNXI_I2S0FAT1);
	writel(regsave[3], sunxi_i2s0.regs + SUNXI_I2S0FCTL);
	writel(regsave[4], sunxi_i2s0.regs + SUNXI_I2S0INT);
	writel(regsave[5], sunxi_i2s0.regs + SUNXI_I2S0CLKD);
	writel(regsave[6], sunxi_i2s0.regs + SUNXI_TXCHSEL);
	writel(regsave[7], sunxi_i2s0.regs + SUNXI_TXCHMAP);
}

static int sunxi_i2s0_suspend(struct snd_soc_dai *cpu_dai)
{
	int ret;
	u32 reg_val;
	u32 pin_index = 0;
	u32 config;
	struct gpio_config *pin_i2s0_cfg;
	char pin_name[8];
	pr_debug("[I2S0]Entered %s\n", __func__);

	/*Global disable Digital Audio Interface*/
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	reg_val &= ~SUNXI_I2S0CTL_GEN;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

	i2s0regsave();
	if ((NULL == i2s0_moduleclk) || (IS_ERR(i2s0_moduleclk))) {
		pr_err("i2s0_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable(i2s0_moduleclk);
	}

	ret = pinctrl_select_state(i2s0_pinctrl, i2s0_pinstate_sleep);
	if (ret) {
		pr_warn("[i2s0]select pin sleep state failed\n");
		return ret;
	}
	devm_pinctrl_put(i2s0_pinctrl);

	return 0;
}

static int sunxi_i2s0_resume(struct snd_soc_dai *cpu_dai)
{
	int ret;
	u32 reg_val;
	pr_debug("[I2S0]Entered %s\n", __func__);

	/*release the module clock*/
	if (clk_enable(i2s0_moduleclk))
		pr_err("try to enable i2s0_moduleclk output failed!\n");

	i2s0regrestore();

	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	reg_val |= SUNXI_I2S0CTL_GEN;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

	i2s0_pinctrl = devm_pinctrl_get(cpu_dai->dev);
	if (IS_ERR_OR_NULL(i2s0_pinctrl)) {
		dev_err(cpu_dai->dev, "pinctrl resume get failed\n");
		ret = -ENOMEM;
		return ret;
	}

	i2s0_pinstate = pinctrl_lookup_state(i2s0_pinctrl,
		PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(i2s0_pinstate)) {
		dev_err(cpu_dai->dev, "pinctrl default state get failed\n");
		ret = -EINVAL;
		return ret;
	}

	i2s0_pinstate_sleep = pinctrl_lookup_state(i2s0_pinctrl,
		PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(i2s0_pinstate_sleep)) {
		dev_err(cpu_dai->dev, "pinctrl sleep state get failed\n");
		ret = -EINVAL;
		return ret;
	}

	ret = pinctrl_select_state(i2s0_pinctrl, i2s0_pinstate);
	if (ret)
		dev_warn(cpu_dai->dev, "set pinctrl default state failed\n");

	return 0;
}

#define SUNXI_I2S0_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_i2s0_dai_ops = {
	.trigger = sunxi_i2s0_trigger,
	.hw_params = sunxi_i2s0_hw_params,
	.set_fmt = sunxi_i2s0_set_fmt,
	.set_clkdiv = sunxi_i2s0_set_clkdiv,
	.set_sysclk = sunxi_i2s0_set_sysclk,
};

static struct snd_soc_dai_driver sunxi_i2s0_dai = {
	.probe = sunxi_i2s0_dai_probe,
	.suspend = sunxi_i2s0_suspend,
	.resume = sunxi_i2s0_resume,
	.remove = sunxi_i2s0_dai_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_I2S0_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_I2S0_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops = &sunxi_i2s0_dai_ops,
};

static const struct snd_soc_component_driver sunxi_i2s0_dai_component = {
	.name = "sunxi-i2s0-dai",
};


static int __init sunxi_i2s0_dev_probe(struct platform_device *pdev)
{
	int err;
	int ret = 0;
	int reg_val = 0;
	int val;
	struct device_node *np = pdev->dev.of_node;

	sunxi_i2s0.regs = ioremap(SUNXI_I2S0BASE, 0x100);
	if (sunxi_i2s0.regs == NULL)
		return -ENXIO;

		i2s0_pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR_OR_NULL(i2s0_pinctrl)) {
			dev_err(&pdev->dev, "pinctrl get failed\n");
			ret = -EINVAL;
			return ret;
		}

		i2s0_pinstate = pinctrl_lookup_state(i2s0_pinctrl,
			PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(i2s0_pinstate)) {
			dev_err(&pdev->dev, "i2s0_pinstate get failed\n");
			ret = -EINVAL;
			return ret;
		}

		i2s0_pinstate_sleep = pinctrl_lookup_state(i2s0_pinctrl,
			PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(i2s0_pinstate_sleep)) {
			dev_err(&pdev->dev,
				"i2s0_pinstate_sleep get failed\n");
			ret = -EINVAL;
			return ret;
		}

#if defined(CONFIG_ARCH_SUN8IW5) \
	|| defined(CONFIG_ARCH_SUN8IW6)
	i2s0_pllx8 = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(i2s0_pllx8)) {
		dev_err(&pdev->dev, "i2s0_pllx8 get failed\n");
		err = PTR_ERR(i2s0_pllx8);
		return err;
	}
#else
#ifndef CONFIG_ARCH_SUN9I
		i2s0_pllx8 = clk_get(NULL, "pll2x8");
#else
		i2s0_pllx8 = clk_get(NULL, "pll3");
#endif
#endif

#ifndef CONFIG_ARCH_SUN9I
	if ((!i2s0_pllx8) || (IS_ERR(i2s0_pllx8)))
		pr_err("try to get i2s0_pllx8 failed\n");

	if (clk_prepare_enable(i2s0_pllx8))
		pr_err("enable i2s0_pllclk failed;\n");

	/*i2s0 pll2clk*/
	i2s0_pllclk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(i2s0_pllclk)) {
		dev_err(&pdev->dev, "i2s0_pllclk get failed\n");
		err = PTR_ERR(i2s0_pllclk);
		return err;
	}
	if (clk_prepare_enable(i2s0_pllclk))
		pr_err("enable i2s0_pllclk failed;\n");

#else
	/*i2s0 pll3clk*/
	i2s0_pllclk = clk_get(NULL, "pll3");
	if ((!i2s0_pllclk) || (IS_ERR(i2s0_pllclk)))
		pr_err("try to get i2s0_pllclk failed\n");

	if (clk_prepare_enable(i2s0_pllclk))
		pr_err("enable i2s0_pllclk failed;\n");

#endif

	/*i2s0 module clk*/
	i2s0_moduleclk = of_clk_get(np, 2);
	if (IS_ERR_OR_NULL(i2s0_moduleclk)) {
		dev_err(&pdev->dev, "i2s0_moduleclk get failed\n");
		err = PTR_ERR(i2s0_moduleclk);
		return err;
	}
	if (clk_set_parent(i2s0_moduleclk, i2s0_pllclk))
		pr_err("try to set parent clk failed! line = %d\n", __LINE__);

#ifndef CONFIG_ARCH_SUN9I
	if (clk_set_rate(i2s0_moduleclk, 24576000/8))
		pr_err("set i2s0_moduleclk rate failed! line=%d\n", __LINE__);

#else
	if (clk_set_rate(i2s0_moduleclk, 24576000))
		pr_err("set i2s0_moduleclk rate failed! line=%d\n", __LINE__);

#endif

	if (clk_prepare_enable(i2s0_moduleclk))
		pr_err("open i2s0_moduleclk failed! line = %d\n", __LINE__);

	reg_val = readl(sunxi_i2s0.regs + SUNXI_I2S0CTL);
	reg_val |= SUNXI_I2S0CTL_GEN;
	writel(reg_val, sunxi_i2s0.regs + SUNXI_I2S0CTL);

	ret = of_property_read_u32(np, "i2s0_select", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "i2s0_select missing or invalid\n");
		i2s0_select = 0;
	} else {
		i2s0_select = val;
		dev_dbg(&pdev->dev, "i2s0_select:%d\n", val);
	}

	ret = of_property_read_u32(np, "over_sample_rate", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "over_sample_rate missing or invalid\n");
		over_sample_rate = 0;
	} else {
		over_sample_rate = val;
		dev_dbg(&pdev->dev, "over_sample_rate:%d\n", val);
	}

	ret = of_property_read_u32(np, "sample_resolution", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "sample_resolution missing or invalid\n");
		sample_resolution = 0;
	} else {
		sample_resolution = val;
		dev_dbg(&pdev->dev, "sample_resolution:%d\n", val);
	}

	ret = of_property_read_u32(np, "word_select_size", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "word_select_size missing or invalid\n");
		word_select_size = 0;
	} else {
		word_select_size = val;
		dev_dbg(&pdev->dev, "word_select_size:%d\n", val);
	}

	ret = of_property_read_u32(np, "pcm_sync_period", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pcm_sync_period missing or invalid\n");
		pcm_sync_period = 0;
	} else {
		pcm_sync_period = val;
		dev_dbg(&pdev->dev, "pcm_sync_period:%d\n", val);
	}

	ret = of_property_read_u32(np, "msb_lsb_first", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "msb_lsb_first missing or invalid\n");
		msb_lsb_first = 0;
	} else {
		msb_lsb_first = val;
		dev_dbg(&pdev->dev, "msb_lsb_first:%d\n", val);
	}

	ret = of_property_read_u32(np, "sign_extend", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "sign_extend missing or invalid\n");
		sign_extend = 0;
	} else {
		sign_extend = val;
		dev_dbg(&pdev->dev, "sign_extend:%d\n", val);
	}

	ret = of_property_read_u32(np, "slot_index", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "slot_index missing or invalid\n");
		slot_index = 0;
	} else {
		slot_index = val;
		dev_dbg(&pdev->dev, "slot_index:%d\n", val);
	}

	ret = of_property_read_u32(np, "slot_width", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "slot_width missing or invalid\n");
		slot_width = 0;
	} else {
		slot_width = val;
		dev_dbg(&pdev->dev, "slot_width:%d\n", val);
	}

	ret = of_property_read_u32(np, "frame_width", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "frame_width missing or invalid\n");
		frame_width = 0;
	} else {
		frame_width = val;
		dev_dbg(&pdev->dev, "frame_width:%d\n", val);
	}

	ret = of_property_read_u32(np, "tx_data_mode", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "tx_data_mode missing or invalid\n");
		tx_data_mode = 0;
	} else {
		tx_data_mode = val;
		dev_dbg(&pdev->dev, "tx_data_mode:%d\n", val);
	}

	ret = of_property_read_u32(np, "rx_data_mode", &val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "rx_data_mode missing or invalid\n");
		rx_data_mode = 0;
	} else {
		rx_data_mode = val;
		dev_dbg(&pdev->dev, "rx_data_mode:%d\n", val);
	}

	ret = snd_soc_register_component(&pdev->dev,
		&sunxi_i2s0_dai_component, &sunxi_i2s0_dai, 1);
	if (ret)
		dev_err(&pdev->dev, "Failed to register DAI\n");


	return 0;
}

static int __exit sunxi_i2s0_dev_remove(struct platform_device *pdev)
{

	if ((NULL == i2s0_moduleclk) || (IS_ERR(i2s0_moduleclk))) {
		pr_err("i2s0_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable(i2s0_moduleclk);
	}
	if ((NULL == i2s0_pllx8) || (IS_ERR(i2s0_pllx8))) {
		pr_err("i2s0_pllx8 handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*reease pllx8clk*/
		clk_put(i2s0_pllx8);
	}
	if ((NULL == i2s0_pllclk) || (IS_ERR(i2s0_pllclk))) {
		pr_err("i2s0_pllclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release pll2clk*/
		clk_put(i2s0_pllclk);
	}

	devm_pinctrl_put(i2s0_pinctrl);

	snd_soc_unregister_component(&pdev->dev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id sunxi_i2s0_of_match[] = {
	{ .compatible = "allwinner,sunxi-i2s0", },
	{},
};

/*method relating*/
static struct platform_driver sunxi_i2s0_driver = {
	.probe = sunxi_i2s0_dev_probe,
	.remove = sunxi_i2s0_dev_remove,
	.driver = {
		.name = "i2s0",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_i2s0_of_match,
	},
};

static int __init sunxi_i2s0_init(void)
{
	int err = 0;

	err = platform_driver_register(&sunxi_i2s0_driver);
	if (err < 0)
		return err;

	return 0;
}
module_init(sunxi_i2s0_init);

static void __exit sunxi_i2s0_exit(void)
{
	platform_driver_unregister(&sunxi_i2s0_driver);
}
module_exit(sunxi_i2s0_exit);

/* Module information */
MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("sunxi I2S0 SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-i2s0");

