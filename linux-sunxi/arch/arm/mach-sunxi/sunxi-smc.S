/*
 * sunxi platform smc functions
 */
#include <linux/linkage.h>
#include <linux/init.h>

ENTRY(__sunxi_fast_smc_call)
    push	{r4-r12, lr}
	mov	r4, r0
	ldm	r4, {r0-r3}
    .arch_extension sec
	smc	#0
	stm	r4, {r0-r3}
	pop	{r4-r12, pc}
ENDPROC(__sunxi_fast_smc_call)
