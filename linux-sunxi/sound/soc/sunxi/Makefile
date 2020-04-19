# SUNXI platform Support

snd-sunxi-soc-objs := sunxi_dma.o
snd-sunxi-soc-objs += codec_utils.o
ifneq ($(CONFIG_SND_SUNXI_SOC_RW),)
snd-sunxi-soc-objs += sunxi_rw_func.o
endif
ifneq ($(CONFIG_SND_SUNXI_SOC_SPDIF_UTILS),)
snd-sunxi-soc-objs += spdif-utils.o
endif
obj-y += snd-sunxi-soc.o

obj-$(CONFIG_SND_SOC) += audiocodec/
obj-$(CONFIG_SND_SOC) += i2s0/
obj-$(CONFIG_SND_SOC) += i2s1/
obj-$(CONFIG_SND_CODEC_NAU85L20) += nau8520.o
#audiocodec
#sun50i
obj-$(CONFIG_SND_SUNXI_SOC_INTERNAL_AUDIOCODEC) += sunxi_codec.o
obj-$(CONFIG_SND_SUNXI_SOC_INTERNAL_I2S) += sunxi_i2s.o

#sun3iw1
obj-$(CONFIG_SND_SUNXI_SOC_INTERNAL_SUN3IW1_AUDIOCODEC) += sun3iw1_codec.o
#sun8iw10
obj-$(CONFIG_SND_SUNXI_SOC_INTERNAL_SUN8IW10_AUDIOCODEC) += sun8iw10_codec.o
#sun8iw11
obj-$(CONFIG_SND_SUNXI_SOC_INTERNAL_SUN8IW11_AUDIOCODEC) += sun8iw11_codec.o
#sun50iw2
obj-$(CONFIG_SND_SUNXI_SOC_INTERNAL_SUN50IW2_AUDIOCODEC) += sun50iw2_codec.o
#sun50iw3
obj-$(CONFIG_SND_SUNXI_SOC_INTERNAL_SUN50IW3_AUDIOCODEC) += sun50iw3_codec.o

#sun3iw1 sun8iw10 sun8iw11 sun50iw2
obj-$(CONFIG_SND_SUNXI_SOC_CODEC_CPU_DAI) += sunxi_cpudai.o

#sun3iw1
obj-$(CONFIG_SND_SOC_SUN3IW1_CPU_DAUDIO) += sun3iw1_daudio.o

#daudio
obj-$(CONFIG_SND_SUNXI_SOC_VIRCODEC) += snddaudio.o
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO_PLATFORM) += sunxi_daudio.o

#dmic
obj-$(CONFIG_SND_SUNXI_SOC_DMIC) += sunxi_dmic.o

#dsd
obj-$(CONFIG_SND_SUNXI_SOC_DSD) += sunxi_dsd.o
obj-$(CONFIG_SND_SOC_CS4385) += cs4385.o

#hdmi
obj-$(CONFIG_SND_SUNXI_SOC_HDMIAUDIO) += sndhdmi.o

#spdif
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += sunxi_spdif.o

#for sound machine
#sun50i
obj-$(CONFIG_SND_SUNXI_SOC_AUDIO_CODEC_MACHINE) += sunxi_sndcodec.o
#sun3iw1
obj-$(CONFIG_SND_SUNXI_SOC_SUN3IW1_AUDIO_CODEC_MACHINE) += sun3iw1_sndcodec.o
#sun8iw10
obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW10_AUDIO_CODEC_MACHINE) += sun8iw10_sndcodec.o
#sun8iw11
obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW11_AUDIO_CODEC_MACHINE) += sun8iw11_sndcodec.o
#sun8iw11
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW2_AUDIO_CODEC_MACHINE) += sun50iw2_sndcodec.o
#sun8iw11
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW3_AUDIO_CODEC_MACHINE) += sun50iw3_sndcodec.o

# audio hub
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_ahub.o
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_ahub_cpudai.o
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_ahub_daudio.o
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_netlink.o

# ahub machine
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi-sndahub.o

#daudio machine
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO_MACHINE) += sunxi-snddaudio.o
#daudio0
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO0_MACHINE) += sunxi-snddaudio.o

#daudio1
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO1_MACHINE) += sunxi-snddaudio.o

#dmic
obj-$(CONFIG_SND_SUNXI_SOC_DMIC) += sunxi-snddmic.o

#dsd
obj-$(CONFIG_SND_SUNXI_SOC_DSD) += sunxi-snddsd.o

#hdmi
obj-$(CONFIG_SND_SUNXI_SOC_HDMIAUDIO) += sunxi-sndhdmi.o

#spdif
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += sunxi-sndspdif.o

##DEBUG
ifeq ($(CONFIG_SUNXI_AUDIO_DEBUG), y)
    EXTRA_CFLAGS += -DDEBUG
endif
