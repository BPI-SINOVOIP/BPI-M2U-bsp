#
# Makefile for the SUNXI SS controller
#

obj-$(CONFIG_CRYPTO_SUNXI) += ss.o

ss-y += sunxi_ss.o

ifdef CONFIG_ARCH_SUN8IW5
	SUNXI_SS_VER = v1
endif
ifdef CONFIG_ARCH_SUN8IW6
	SUNXI_SS_VER = v2
endif
ifdef CONFIG_ARCH_SUN8IW11
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN8IW17
	SUNXI_SS_VER = v3
endif
ifdef CONFIG_ARCH_SUN50I
	SUNXI_SS_VER = v3
endif

ss-y += sunxi_ss_proc_comm.o $(SUNXI_SS_VER)/sunxi_ss_reg.o $(SUNXI_SS_VER)/sunxi_ss_proc.o
ss-$(CONFIG_CRYPTO_SUNXI_SS_KL) += sunxi_ss_kl.o

EXTRA_CFLAGS += -Idrivers/crypto/sunxi/$(SUNXI_SS_VER)

#ccflags-y += -DDEBUG
