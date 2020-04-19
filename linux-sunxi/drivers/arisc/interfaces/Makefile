#
# Makefile for the linux kernel.
#

obj-y=arisc_dvfs.o arisc_standby.o arisc_rsb.o arisc_axp.o arisc_debug_level.o arisc_loopback.o arisc_dram_crc.o arisc_twi.o arisc_sharemem.o
ifeq ($(CONFIG_SUNXI_ARISC_COM_DIRECTLY),y)
obj-y += arisc_audio.o
endif
