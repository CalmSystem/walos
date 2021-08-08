#ifndef __ASM_H
#define __ASM_H
#include "stdint.h"

static inline void io_write8(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a" (value), "Nd" (port));
}
static inline uint8_t io_read8(uint16_t port) {
    uint8_t rega;
    __asm__ __volatile__("inb %1,%0" : "=a" (rega) : "Nd" (port));
    return rega;
}
static inline void io_write16(uint16_t port, uint16_t value) {
    __asm__ __volatile__("outw %0, %1" : : "a" (value), "Nd" (port));
}
static inline uint16_t io_read16(uint16_t port) {
    uint16_t rega;
    __asm__ __volatile__("inw %1,%0" : "=a" (rega) : "Nd" (port));
    return rega;
}
static inline void io_write32(uint16_t port, uint32_t value) {
    __asm__ __volatile__("outl %0, %1" : : "a" (value), "Nd" (port));
}
static inline uint32_t io_read32(uint16_t port) {
    uint32_t rega;
    __asm__ __volatile__("inl %1,%0" : "=a" (rega) : "Nd" (port));
    return rega;
}

static inline void flags_set(uint64_t value) {
    __asm__ __volatile__("pushq	%0; popfq" :: "g" (value) : "memory");
}
static inline uint64_t flags_get() {
    uint64_t flags;
    __asm__ __volatile__("pushfq; popq %0" : "=g" (flags) :: "memory");
    return flags;
}
enum rflags_t {
    RF_CPUID = 1 << 21,
    RF_VIRTUAL_INTERRUPT_PENDING = 1 << 20,
    RF_VIRTUAL_INTERRUPT = 1 << 19,
    RF_ALIGNMENT_CHECK = 1 << 18,
    RF_VIRTUAL_8086_MODE = 1 << 17,
    RF_RESUME_FLAG = 1 << 16,
    RF_NESTED_TASK = 1 << 14,
    RF_IOPL_HIGH = 1 << 13,
    RF_IOPL_LOW = 1 << 12,
    RF_OVERFLOW_FLAG = 1 << 11,
    RF_DIRECTION_FLAG = 1 << 10,
    RF_INTERRUPT_FLAG = 1 << 9,
    RF_TRAP_FLAG = 1 << 8,
    RF_SIGN_FLAG = 1 << 7,
    RF_ZERO_FLAG = 1 << 6,
    RF_AUXILIARY_CARRY_FLAG = 1 << 4,
    RF_PARITY_FLAG = 1 << 2,
    RF_CARRY_FLAG = 1,
};

static inline int cpuid_get(int code, uint32_t where[4]) {
    __asm__ __volatile__("cpuid":"=a"(*where),"=b"(*(where+1)),
        "=c"(*(where+2)),"=d"(*(where+3)):"a"(code));
    return (int)where[0];
}
enum cpuid_feat_c {
    CPU_FEAT_C_SSE3         = 1 << 0,
    CPU_FEAT_C_PCLMUL       = 1 << 1,
    CPU_FEAT_C_DTES64       = 1 << 2,
    CPU_FEAT_C_MONITOR      = 1 << 3,
    CPU_FEAT_C_DS_CPL       = 1 << 4,
    CPU_FEAT_C_VMX          = 1 << 5,
    CPU_FEAT_C_SMX          = 1 << 6,
    CPU_FEAT_C_EST          = 1 << 7,
    CPU_FEAT_C_TM2          = 1 << 8,
    CPU_FEAT_C_SSSE3        = 1 << 9,
    CPU_FEAT_C_CID          = 1 << 10,
    CPU_FEAT_C_FMA          = 1 << 12,
    CPU_FEAT_C_CX16         = 1 << 13,
    CPU_FEAT_C_ETPRD        = 1 << 14,
    CPU_FEAT_C_PDCM         = 1 << 15,
    CPU_FEAT_C_PCIDE        = 1 << 17,
    CPU_FEAT_C_DCA          = 1 << 18,
    CPU_FEAT_C_SSE4_1       = 1 << 19,
    CPU_FEAT_C_SSE4_2       = 1 << 20,
    CPU_FEAT_C_x2APIC       = 1 << 21,
    CPU_FEAT_C_MOVBE        = 1 << 22,
    CPU_FEAT_C_POPCNT       = 1 << 23,
    CPU_FEAT_C_AES          = 1 << 25,
    CPU_FEAT_C_XSAVE        = 1 << 26,
    CPU_FEAT_C_OSXSAVE      = 1 << 27,
    CPU_FEAT_C_AVX          = 1 << 28,
    CPU_FEAT_C_RDRAND       = 1 << 30
};
enum cpuid_feat_d {
    CPU_FEAT_D_FPU          = 1 << 0,
    CPU_FEAT_D_VME          = 1 << 1,
    CPU_FEAT_D_DE           = 1 << 2,
    CPU_FEAT_D_PSE          = 1 << 3,
    CPU_FEAT_D_TSC          = 1 << 4,
    CPU_FEAT_D_MSR          = 1 << 5,
    CPU_FEAT_D_PAE          = 1 << 6,
    CPU_FEAT_D_MCE          = 1 << 7,
    CPU_FEAT_D_CX8          = 1 << 8,
    CPU_FEAT_D_APIC         = 1 << 9,
    CPU_FEAT_D_SEP          = 1 << 11,
    CPU_FEAT_D_MTRR         = 1 << 12,
    CPU_FEAT_D_PGE          = 1 << 13,
    CPU_FEAT_D_MCA          = 1 << 14,
    CPU_FEAT_D_CMOV         = 1 << 15,
    CPU_FEAT_D_PAT          = 1 << 16,
    CPU_FEAT_D_PSE36        = 1 << 17,
    CPU_FEAT_D_PSN          = 1 << 18,
    CPU_FEAT_D_CLF          = 1 << 19,
    CPU_FEAT_D_DTES         = 1 << 21,
    CPU_FEAT_D_ACPI         = 1 << 22,
    CPU_FEAT_D_MMX          = 1 << 23,
    CPU_FEAT_D_FXSR         = 1 << 24,
    CPU_FEAT_D_SSE          = 1 << 25,
    CPU_FEAT_D_SSE2         = 1 << 26,
    CPU_FEAT_D_SS           = 1 << 27,
    CPU_FEAT_D_HTT          = 1 << 28,
    CPU_FEAT_D_TM1          = 1 << 29,
    CPU_FEAT_D_IA64         = 1 << 30,
    CPU_FEAT_D_PBE          = 1 << 31
};
static inline int cpuid_has_feat_c(enum cpuid_feat_c feat) {
    uint32_t enx[4];
    return cpuid_get(1, enx) && (enx[2] & feat);
}
static inline int cpuid_has_feat_d(enum cpuid_feat_d feat) {
    uint32_t enx[4];
    return cpuid_get(1, enx) && (enx[3] & feat);
}

struct idtr_t {
    uint16_t size;
    uintptr_t offset;
} __attribute__((packed));
static inline void idt_load(struct idtr_t* pidtr) {
    __asm__ volatile ("lidt (%0)" : : "r" (pidtr));
}

static inline void cli() {
    __asm__ __volatile__("cli":::"memory");
}
static inline void sti() {
    __asm__ __volatile__("sti":::"memory");
}

typedef int preempt_lock_t;
static inline void preempt_disable(preempt_lock_t* lock) {
    if (lock) *lock = flags_get() & RF_INTERRUPT_FLAG;
    cli();
}
static inline void preempt_release(preempt_lock_t lock) {
    if (lock) sti();
}
static inline void preempt_force() {
    __asm__ __volatile__("sti; hlt; cli":::"memory");
}

#endif
