#ifndef _MEM_DIVLIBC_H
#define _MEM_DIVLIBC_H

void __mem_div0(void);
__u32 raw_lib_udiv(__u32 dividend, __u32 divisior);
extern void __aeabi_idiv(void);
extern void __aeabi_idivmod(void);
extern void __aeabi_uidiv(void);
extern void __aeabi_uidivmod(void);

#endif /*_MEM_DIVLIBC_H*/
