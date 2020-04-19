	.align	2
	.globl	resume1_bin_start
resume1_bin_start:
	.incbin	"drivers/soc/allwinner/pm/resume1/resume1.code"
	.align 13
	.globl	resume1_bin_end
resume1_bin_end:
	.align	2
