obj-$(CONFIG_ARCH_SUN8I) += sun8i.o
obj-$(CONFIG_ARCH_SUN50I) += sun50i.o
obj-$(CONFIG_ARCH_SUN3I) += sun3i.o

ifeq ($(CONFIG_SMP), y)
ifneq ($(CONFIG_ARM_PSCI), y)
ifeq ($(CONFIG_ARCH_SUN8IW17),y)
obj-$(CONFIG_SUN8I_CCI)   += sun8i-cci.o
obj-$(CONFIG_MCPM)        += sun8i-mcpm.o
CFLAGS_sun8i-mcpm.o += -DDEBUG
ifneq ($(CONFIG_SUNXI_TRUSTZONE), y)
obj-$(CONFIG_MCPM)        += sun8i-setup.o
endif
else
obj-y += headsmp.o platsmp.o
obj-$(CONFIG_HOTPLUG_CPU) += hotplug.o
endif
endif
endif
ifeq ($(CONFIG_SUNXI_TRUSTZONE), y)
obj-$(CONFIG_SUNXI_TRUSTZONE) += firmware.o
obj-$(CONFIG_SUNXI_TRUSTZONE) += sunxi-smc.o
endif
