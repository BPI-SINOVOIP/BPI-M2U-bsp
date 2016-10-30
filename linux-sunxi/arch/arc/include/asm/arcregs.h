/*
 * Copyright (C) 2004, 2007-2010, 2011-2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ASM_ARC_ARCREGS_H
#define _ASM_ARC_ARCREGS_H

#ifdef __KERNEL__

/* Build Configuration Registers */
#define ARC_REG_DCCMBASE_BCR	0x61	/* DCCM Base Addr */
#define ARC_REG_CRC_BCR		0x62
#define ARC_REG_DVFB_BCR	0x64
#define ARC_REG_EXTARITH_BCR	0x65
#define ARC_REG_VECBASE_BCR	0x68
#define ARC_REG_PERIBASE_BCR	0x69
#define ARC_REG_FP_BCR		0x6B	/* Single-Precision FPU */
#define ARC_REG_DPFP_BCR	0x6C	/* Dbl Precision FPU */
#define ARC_REG_MMU_BCR		0x6f
#define ARC_REG_DCCM_BCR	0x74	/* DCCM Present + SZ */
#define ARC_REG_TIMERS_BCR	0x75
#define ARC_REG_ICCM_BCR	0x78
#define ARC_REG_XY_MEM_BCR	0x79
#define ARC_REG_MAC_BCR		0x7a
#define ARC_REG_MUL_BCR		0x7b
#define ARC_REG_SWAP_BCR	0x7c
#define ARC_REG_NORM_BCR	0x7d
#define ARC_REG_MIXMAX_BCR	0x7e
#define ARC_REG_BARREL_BCR	0x7f
#define ARC_REG_D_UNCACH_BCR	0x6A

/* status32 Bits Positions */
#define STATUS_H_BIT		0	/* CPU Halted */
#define STATUS_E1_BIT		1	/* Int 1 enable */
#define STATUS_E2_BIT		2	/* Int 2 enable */
#define STATUS_A1_BIT		3	/* Int 1 active */
#define STATUS_A2_BIT		4	/* Int 2 active */
#define STATUS_AE_BIT		5	/* Exception active */
#define STATUS_DE_BIT		6	/* PC is in delay slot */
#define STATUS_U_BIT		7	/* User/Kernel mode */
#define STATUS_L_BIT		12	/* Loop inhibit */

/* These masks correspond to the status word(STATUS_32) bits */
#define STATUS_H_MASK		(1<<STATUS_H_BIT)
#define STATUS_E1_MASK		(1<<STATUS_E1_BIT)
#define STATUS_E2_MASK		(1<<STATUS_E2_BIT)
#define STATUS_A1_MASK		(1<<STATUS_A1_BIT)
#define STATUS_A2_MASK		(1<<STATUS_A2_BIT)
#define STATUS_AE_MASK		(1<<STATUS_AE_BIT)
#define STATUS_DE_MASK		(1<<STATUS_DE_BIT)
#define STATUS_U_MASK		(1<<STATUS_U_BIT)
#define STATUS_L_MASK		(1<<STATUS_L_BIT)

/*
 * ECR: Exception Cause Reg bits-n-pieces
 * [23:16] = Exception Vector
 * [15: 8] = Exception Cause Code
 * [ 7: 0] = Exception Parameters (for certain types only)
 */
#define ECR_VEC_MASK			0xff0000
#define ECR_CODE_MASK			0x00ff00
#define ECR_PARAM_MASK			0x0000ff

/* Exception Cause Vector Values */
#define ECR_V_INSN_ERR			0x02
#define ECR_V_MACH_CHK			0x20
#define ECR_V_ITLB_MISS			0x21
#define ECR_V_DTLB_MISS			0x22
#define ECR_V_PROTV			0x23

/* Protection Violation Exception Cause Code Values */
#define ECR_C_PROTV_INST_FETCH		0x00
#define ECR_C_PROTV_LOAD		0x01
#define ECR_C_PROTV_STORE		0x02
#define ECR_C_PROTV_XCHG		0x03
#define ECR_C_PROTV_MISALIG_DATA	0x04

/* DTLB Miss Exception Cause Code Values */
#define ECR_C_BIT_DTLB_LD_MISS		8
#define ECR_C_BIT_DTLB_ST_MISS		9


/* Auxiliary registers */
#define AUX_IDENTITY		4
#define AUX_INTR_VEC_BASE	0x25
#define AUX_IRQ_LEV		0x200	/* IRQ Priority: L1 or L2 */
#define AUX_IRQ_HINT		0x201	/* For generating Soft Interrupts */
#define AUX_IRQ_LV12		0x43	/* interrupt level register */

#define AUX_IENABLE		0x40c
#define AUX_ITRIGGER		0x40d
#define AUX_IPULSE		0x415

/* Timer related Aux registers */
#define ARC_REG_TIMER0_LIMIT	0x23	/* timer 0 limit */
#define ARC_REG_TIMER0_CTRL	0x22	/* timer 0 control */
#define ARC_REG_TIMER0_CNT	0x21	/* timer 0 count */
#define ARC_REG_TIMER1_LIMIT	0x102	/* timer 1 limit */
#define ARC_REG_TIMER1_CTRL	0x101	/* timer 1 control */
#define ARC_REG_TIMER1_CNT	0x100	/* timer 1 count */

#define TIMER_CTRL_IE		(1 << 0) /* Interupt when Count reachs limit */
#define TIMER_CTRL_NH		(1 << 1) /* Count only when CPU NOT halted */

/* MMU Management regs */
#define ARC_REG_TLBPD0		0x405
#define ARC_REG_TLBPD1		0x406
#define ARC_REG_TLBINDEX	0x407
#define ARC_REG_TLBCOMMAND	0x408
#define ARC_REG_PID		0x409
#define ARC_REG_SCRATCH_DATA0	0x418

/* Bits in MMU PID register */
#define MMU_ENABLE		(1 << 31)	/* Enable MMU for process */

/* Error code if probe fails */
#define TLB_LKUP_ERR		0x80000000

/* TLB Commands */
#define TLBWrite    0x1
#define TLBRead     0x2
#define TLBGetIndex 0x3
#define TLBProbe    0x4

#if (CONFIG_ARC_MMU_VER >= 2)
#define TLBWriteNI  0x5		/* write JTLB without inv uTLBs */
#define TLBIVUTLB   0x6		/* explicitly inv uTLBs */
#else
#undef TLBWriteNI		/* These cmds don't exist on older MMU */
#undef TLBIVUTLB
#endif

/* Instruction cache related Auxiliary registers */
#define ARC_REG_IC_BCR		0x77	/* Build Config reg */
#define ARC_REG_IC_IVIC		0x10
#define ARC_REG_IC_CTRL		0x11
#define ARC_REG_IC_IVIL		0x19
#if (CONFIG_ARC_MMU_VER > 2)
#define ARC_REG_IC_PTAG		0x1E
#endif

/* Bit val in IC_CTRL */
#define IC_CTRL_CACHE_DISABLE   0x1

/* Data cache related Auxiliary registers */
#define ARC_REG_DC_BCR		0x72
#define ARC_REG_DC_IVDC		0x47
#define ARC_REG_DC_CTRL		0x48
#define ARC_REG_DC_IVDL		0x4A
#define ARC_REG_DC_FLSH		0x4B
#define ARC_REG_DC_FLDL		0x4C
#if (CONFIG_ARC_MMU_VER > 2)
#define ARC_REG_DC_PTAG		0x5C
#endif

/* Bit val in DC_CTRL */
#define DC_CTRL_INV_MODE_FLUSH  0x40
#define DC_CTRL_FLUSH_STATUS    0x100

/* MMU Management regs */
#define ARC_REG_PID		0x409
#define ARC_REG_SCRATCH_DATA0	0x418

/* Bits in MMU PID register */
#define MMU_ENABLE		(1 << 31)	/* Enable MMU for process */

/*
 * Floating Pt Registers
 * Status regs are read-only (build-time) so need not be saved/restored
 */
#define ARC_AUX_FP_STAT         0x300
#define ARC_AUX_DPFP_1L         0x301
#define ARC_AUX_DPFP_1H         0x302
#define ARC_AUX_DPFP_2L         0x303
#define ARC_AUX_DPFP_2H         0x304
#define ARC_AUX_DPFP_STAT       0x305

#ifndef __ASSEMBLY__

/*
 ******************************************************************
 *      Inline ASM macros to read/write AUX Regs
 *      Essentially invocation of lr/sr insns from "C"
 */

#if 1

#define read_aux_reg(reg)	__builtin_arc_lr(reg)

/* gcc builtin sr needs reg param to be long immediate */
#define write_aux_reg(reg_immed, val)		\
		__builtin_arc_sr((unsigned int)val, reg_immed)

#else

#define read_aux_reg(reg)		\
({					\
	unsigned int __ret;		\
	__asm__ __volatile__(		\
	"	lr    %0, [%1]"		\
	: "=r"(__ret)			\
	: "i"(reg));			\
	__ret;				\
})

/*
 * Aux Reg address is specified as long immediate by caller
 * e.g.
 *    write_aux_reg(0x69, some_val);
 * This generates tightest code.
 */
#define write_aux_reg(reg_imm, val)	\
({					\
	__asm__ __volatile__(		\
	"	sr   %0, [%1]	\n"	\
	:				\
	: "ir"(val), "i"(reg_imm));	\
})

/*
 * Aux Reg address is specified in a variable
 *  * e.g.
 *      reg_num = 0x69
 *      write_aux_reg2(reg_num, some_val);
 * This has to generate glue code to load the reg num from
 *  memory to a reg hence not recommended.
 */
#define write_aux_reg2(reg_in_var, val)		\
({						\
	unsigned int tmp;			\
						\
	__asm__ __volatile__(			\
	"	ld   %0, [%2]	\n\t"		\
	"	sr   %1, [%0]	\n\t"		\
	: "=&r"(tmp)				\
	: "r"(val), "memory"(&reg_in_var));	\
})

#endif

#define READ_BCR(reg, into)				\
{							\
	unsigned int tmp;				\
	tmp = read_aux_reg(reg);			\
	if (sizeof(tmp) == sizeof(into)) {		\
		into = *((typeof(into) *)&tmp);		\
	} else {					\
		extern void bogus_undefined(void);	\
		bogus_undefined();			\
	}						\
}

#define WRITE_BCR(reg, into)				\
{							\
	unsigned int tmp;				\
	if (sizeof(tmp) == sizeof(into)) {		\
		tmp = (*(unsigned int *)(into));	\
		write_aux_reg(reg, tmp);		\
	} else  {					\
		extern void bogus_undefined(void);	\
		bogus_undefined();			\
	}						\
}

/* Helpers */
#define TO_KB(bytes)		((bytes) >> 10)
#define TO_MB(bytes)		(TO_KB(bytes) >> 10)
#define PAGES_TO_KB(n_pages)	((n_pages) << (PAGE_SHIFT - 10))
#define PAGES_TO_MB(n_pages)	(PAGES_TO_KB(n_pages) >> 10)

#ifdef CONFIG_ARC_FPU_SAVE_RESTORE
/* These DPFP regs need to be saved/restored across ctx-sw */
struct arc_fpu {
	struct {
		unsigned int l, h;
	} aux_dpfp[2];
};
#endif

/*
 ***************************************************************
 * Build Configuration Registers, with encoded hardware config
 */
struct bcr_identity {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int chip_id:16, cpu_id:8, family:8;
#else
	unsigned int family:8, cpu_id:8, chip_id:16;
#endif
};

struct bcr_mmu_1_2 {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int ver:8, ways:4, sets:4, u_itlb:8, u_dtlb:8;
#else
	unsigned int u_dtlb:8, u_itlb:8, sets:4, ways:4, ver:8;
#endif
};

struct bcr_mmu_3 {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int ver:8, ways:4, sets:4, osm:1, reserv:3, pg_sz:4,
		     u_itlb:4, u_dtlb:4;
#else
	unsigned int u_dtlb:4, u_itlb:4, pg_sz:4, reserv:3, osm:1, sets:4,
		     ways:4, ver:8;
#endif
};

#define EXTN_SWAP_VALID     0x1
#define EXTN_NORM_VALID     0x2
#define EXTN_MINMAX_VALID   0x2
#define EXTN_BARREL_VALID   0x2

struct bcr_extn {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int pad:20, crc:1, ext_arith:2, mul:2, barrel:2, minmax:2,
		     norm:2, swap:1;
#else
	unsigned int swap:1, norm:2, minmax:2, barrel:2, mul:2, ext_arith:2,
		     crc:1, pad:20;
#endif
};

/* DSP Options Ref Manual */
struct bcr_extn_mac_mul {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int pad:16, type:8, ver:8;
#else
	unsigned int ver:8, type:8, pad:16;
#endif
};

struct bcr_extn_xymem {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int ram_org:2, num_banks:4, bank_sz:4, ver:8;
#else
	unsigned int ver:8, bank_sz:4, num_banks:4, ram_org:2;
#endif
};

struct bcr_cache {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int pad:12, line_len:4, sz:4, config:4, ver:8;
#else
	unsigned int ver:8, config:4, sz:4, line_len:4, pad:12;
#endif
};

struct bcr_perip {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int start:8, pad2:8, sz:8, pad:8;
#else
	unsigned int pad:8, sz:8, pad2:8, start:8;
#endif
};
struct bcr_iccm {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int base:16, pad:5, sz:3, ver:8;
#else
	unsigned int ver:8, sz:3, pad:5, base:16;
#endif
};

/* DCCM Base Address Register: ARC_REG_DCCMBASE_BCR */
struct bcr_dccm_base {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int addr:24, ver:8;
#else
	unsigned int ver:8, addr:24;
#endif
};

/* DCCM RAM Configuration Register: ARC_REG_DCCM_BCR */
struct bcr_dccm {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int res:21, sz:3, ver:8;
#else
	unsigned int ver:8, sz:3, res:21;
#endif
};

/* Both SP and DP FPU BCRs have same format */
struct bcr_fp {
#ifdef CONFIG_CPU_BIG_ENDIAN
	unsigned int fast:1, ver:8;
#else
	unsigned int ver:8, fast:1;
#endif
};

/*
 *******************************************************************
 * Generic structures to hold build configuration used at runtime
 */

struct cpuinfo_arc_mmu {
	unsigned int ver, pg_sz, sets, ways, u_dtlb, u_itlb, num_tlb;
};

struct cpuinfo_arc_cache {
	unsigned int has_aliasing, sz, line_len, assoc, ver;
};

struct cpuinfo_arc_ccm {
	unsigned int base_addr, sz;
};

struct cpuinfo_arc {
	struct cpuinfo_arc_cache icache, dcache;
	struct cpuinfo_arc_mmu mmu;
	struct bcr_identity core;
	unsigned int timers;
	unsigned int vec_base;
	unsigned int uncached_base;
	struct cpuinfo_arc_ccm iccm, dccm;
	struct bcr_extn extn;
	struct bcr_extn_xymem extn_xymem;
	struct bcr_extn_mac_mul extn_mac_mul;
	struct bcr_fp fp, dpfp;
};

extern struct cpuinfo_arc cpuinfo_arc700[];

#endif /* __ASEMBLY__ */

#endif /* __KERNEL__ */

#endif /* _ASM_ARC_ARCREGS_H */
