#ccflags-y += -DDEBUG

ifneq ($(strip $(CONFIG_ARCH_SUN50I) $(CONFIG_ARCH_SUN8IW10) $(CONFIG_ARCH_SUN8IW11) $(CONFIG_ARCH_SUN8IW17) $(CONFIG_ARCH_SUN3IW1)),)
obj-$(CONFIG_PM_SLEEP) += pm/
else ifneq ($(strip $(CONFIG_ARCH_SUN8IW6) $(CONFIG_ARCH_SUN8IW5)),)
obj-$(CONFIG_PM_SLEEP) += pm_legacy/
endif

obj-$(CONFIG_SUNXI_SRAM) += sunxi_sram.o

obj-$(CONFIG_ARCH_SUNXI) += sunxi-sid.o
