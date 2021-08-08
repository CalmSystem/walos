#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H
#include "../asm.h"
#include "stdlib.h"

#define LWIP_PLATFORM_ASSERT(x) do {printf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); abort();} while(0)

typedef preempt_lock_t sys_prot_t;

#define LW_SYS_ARCH_DECL_PROTECT(lev) sys_prot_t lev
#define LW_SYS_ARCH_PROTECT(lev)      preempt_disable(&lev)
#define LW_SYS_ARCH_UNPROTECT(lev)    preempt_release(lev)

#define LWIP_RAND() ((u32_t)rand())

#endif
