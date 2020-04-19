	.globl	standby_bin_start
standby_bin_start:
	.incbin	"drivers/soc/allwinner/pm_legacy/standby/standby.code"
	.align	13
	.globl	standby_bin_end
standby_bin_end:
	.align	2
