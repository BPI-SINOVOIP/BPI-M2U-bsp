#
# Makefile for the kernel mixer device drivers.
#

obj-$(CONFIG_SUNXI_G2D) += g2d_sunxi.o

ifneq ($(CONFIG_ARCH_SUN8IW12P1)$(CONFIG_ARCH_SUN8IW17P1),)
g2d_sunxi-objs				:= g2d_driver.o g2d_bsp_v2.o
else
g2d_sunxi-objs				:= g2d_driver.o g2d_bsp_sun8iw11.o
endif