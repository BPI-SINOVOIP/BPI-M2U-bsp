#
# Makefile for the linux kernel.
#

obj-y=arisc.o arisc-notifier.o scp-call.o
obj-y += interfaces/

ifeq ($(CONFIG_SUNXI_ARISC_COM_DIRECTLY),y)
obj-y += hwmsgbox/
obj-y += message_manager/
obj-y += arisc_dram.o
endif

ifeq ($(CONFIG_ARCH_SUN8IW5P1),y)
obj-y += binary/
else ifeq ($(CONFIG_ARCH_SUN8IW6P1),y)
obj-y += binary/
endif
