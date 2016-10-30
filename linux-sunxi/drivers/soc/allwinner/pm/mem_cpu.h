#ifndef _MEM_CPU_H
#define _MEM_CPU_H

/*
 * Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

/*
 * Image of the saved processor state
 *
 * coprocessor 15 registers(RW)
 */
struct saved_context {
/*
 * FIXME: Only support for Cortex A8 and Cortex A9 now
 */
	/* CR0 */
	__u32 cssr;		/* Cache Size Selection */
	/* CR1 */
#ifdef CORTEX_A8
	__u32 cr;		/* Control */
	__u32 acr;		/* Auxiliary Control Register */
	__u32 cacr;		/* Coprocessor Access Control */
	__u32 sccfgr;		/* Secure Config Register */
	__u32 scdbgenblr;	/* Secure Debug Enable Register */
	__u32 nonscacctrlr;	/* Nonsecure Access Control Register */
#elif defined(CORTEX_A9)
	__u32 cr;
	__u32 actlr;
	__u32 cacr;
	__u32 sder;
	__u32 vcr;
#elif defined(CORTEX_A7)
	__u32 cr;		/* Control */
	__u32 acr;		/* Auxiliary Control Register */
	__u32 cacr;		/* Coprocessor Access Control */
	__u32 sccfgr;		/* Secure Config Register */
	__u32 scdbgenblr;	/* Secure Debug Enable Register */
	__u32 nonscacctrlr;	/* Nonsecure Access Control Register */
#endif

	/* CR2 */
	__u32 ttb_0r;		/* Translation Table Base 0 */
	__u32 ttb_1r;		/* Translation Table Base 1 */
	__u32 ttbcr;		/* Translation Talbe Base Control */
	/* CR3 */
	__u32 dacr;		/* Domain Access Control */
	/* CR5 */
	__u32 d_fsr;		/* Data Fault Status */
	__u32 i_fsr;		/* Instruction Fault Status */
	__u32 d_afsr; /* Data Auxilirary Fault Status */ ;
	__u32 i_afsr; /* Instruction Auxilirary Fault Status */ ;
	/* CR6 */
	__u32 d_far;		/* Data Fault Address */
	__u32 i_far;		/* Instruction Fault Address */
	/* CR7 */
	__u32 par;		/* Physical Address */
	/* CR9 *//* FIXME: Are they necessary? */
	__u32 pmcontrolr;	/* Performance Monitor Control */
	__u32 cesr;		/* Count Enable Set */
	__u32 cecr;		/* Count Enable Clear */
	__u32 ofsr;		/* Overflow Flag Status */
#ifdef CORTEX_A8
	__u32 sir;		/* Software Increment */
#elif defined(CORTEX_A7)
	__u32 sir;		/* Software Increment */
#endif
	__u32 pcsr;		/* Performance Counter Selection */
	__u32 ccr;		/* Cycle Count */
	__u32 esr;		/* Event Selection */
	__u32 pmcountr;		/* Performance Monitor Count */
	__u32 uer;		/* User Enable */
	__u32 iesr;		/* Interrupt Enable Set */
	__u32 iecr;		/* Interrupt Enable Clear */
#ifdef CORTEX_A8
	__u32 l2clr;		/* L2 Cache Lockdown */
	__u32 l2cauxctrlr;	/* L2 Cache Auxiliary Control */
#elif defined(CORTEX_A7)

#endif

	/* CR10 */
#ifdef CORTEX_A8
	__u32 d_tlblr;		/* Data TLB Lockdown Register */
	__u32 i_tlblr;		/* Instruction TLB Lockdown Register */
#elif defined(CORTEX_A7)
#endif
	__u32 prrr;		/* Primary Region Remap Register */
	__u32 nrrr;		/* Normal Memory Remap Register */

	/* CR11 */
#ifdef CORTEX_A8
	__u32 pleuar;		/* PLE User Accessibility */
	__u32 plecnr;		/* PLE Channel Number */
	__u32 plecr;		/* PLE Control */
	__u32 pleisar;		/* PLE Internal Start Address */
	__u32 pleiear;		/* PLE Internal End Address */
	__u32 plecidr;		/* PLE Context ID */
#elif defined(CORTEX_A7)
#endif

	/* CR12 */
#ifdef CORTEX_A8
	__u32 snsvbar;		/* Secure or Nonsecure Vector Base Address */
	__u32 monvecbar;	/*Monitor Vector Base */
#elif defined(CORTEX_A9)
	__u32 vbar;
	__u32 mvbar;
	__u32 vir;
#elif defined(CORTEX_A7)
	__u32 vbar;
	__u32 mvbar;
	__u32 isr;
#endif

	/* CR13 */
	__u32 fcse;		/* FCSE PID */
	__u32 cid;		/* Context ID */
	__u32 urwtpid;		/* User read/write Thread and Process ID */
	__u32 urotpid;		/* User read-only Thread and Process ID */
	__u32 potpid;		/* Privileged only Thread and Process ID */

	/* CR15 */
#ifdef CORTEX_A9
	__u32 mtlbar;
#endif
} __attribute__ ((packed));

void __save_processor_state(struct saved_context *ctxt);
void __restore_processor_state(struct saved_context *ctxt);
void disable_cache_invalidate(void);
void set_copro_default(void);

void save_processor_state(void);
void restore_processor_state(void);
void restore_processor_ttbr0(void);
void set_ttbr0(void);
extern int get_cur_cluster_id(void);

/* Used in mem_cpu_asm.S */
#define SYS_CONTEXT_SIZE (2)
#define SVC_CONTEXT_SIZE (2)	/*do not need backup r14? reason? */
#define FIQ_CONTEXT_SIZE (7)
#define ABT_CONTEXT_SIZE (2)
#define IRQ_CONTEXT_SIZE (2)
#define UND_CONTEXT_SIZE (2)
#define MON_CONTEXT_SIZE (2)

#define EMPTY_CONTEXT_SIZE (11 * sizeof(__u32))

extern unsigned long saved_context_r13_sys[SYS_CONTEXT_SIZE];
extern unsigned long saved_cpsr_svc;
extern unsigned long saved_context_r12_svc[SVC_CONTEXT_SIZE];
extern unsigned long saved_spsr_svc;
extern unsigned long saved_context_r13_fiq[FIQ_CONTEXT_SIZE];
extern unsigned long saved_spsr_fiq;
extern unsigned long saved_context_r13_abt[ABT_CONTEXT_SIZE];
extern unsigned long saved_spsr_abt;
extern unsigned long saved_context_r13_irq[IRQ_CONTEXT_SIZE];
extern unsigned long saved_spsr_irq;
extern unsigned long saved_context_r13_und[UND_CONTEXT_SIZE];
extern unsigned long saved_spsr_und;
extern unsigned long saved_context_r13_mon[MON_CONTEXT_SIZE];
extern unsigned long saved_spsr_mon;
extern unsigned long saved_empty_context_svc[EMPTY_CONTEXT_SIZE];

#endif /*_MEM_CPU_H*/
