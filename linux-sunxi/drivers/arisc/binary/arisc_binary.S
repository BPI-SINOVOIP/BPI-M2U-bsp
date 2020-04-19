	.globl  arisc_binary_start
	.globl  arisc_binary_end
arisc_binary_start:
#if (defined CONFIG_ARCH_SUN8IW5P1)
	.incbin "drivers/arisc/binary/arisc_sun8iw5p1.bin"
#elif (defined CONFIG_ARCH_SUN8IW6P1)
	.incbin "drivers/arisc/binary/arisc_sun8iw6p1.bin"
#endif
arisc_binary_end:
	.align  2
