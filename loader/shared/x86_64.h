#include <stdint.h>

static inline int cpuid_get(int code, uint32_t where[4]) {
    __asm__ __volatile__("cpuid":"=a"(*where),"=b"(*(where+1)),
        "=c"(*(where+2)),"=d"(*(where+3)):"a"(code));
    return (int)where[0];
}
static inline bool cpuid_has_feat(uint8_t idx, bool edx) {
	uint32_t enx[4];
	return cpuid_get(1, enx) && (enx[edx ? 3 : 2] & (1ul << idx));
}
static char simd_fxsave_region[512] __attribute__((aligned(16)));
static inline void x86_64_enable_feats(void) {
	// SSE2 is required for long mode compatible CPU
	// if (!cpuid_has_feat(25, true)) return;
	uint64_t cr0, cr4;
	__asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
	cr0 &= ~(1 << 2);
	cr0 |= (1 << 1);
	__asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0));
	__asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= (1 << 9);
	cr4 |= (1 << 10);
	__asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4));

	// AVX
	if (!(cpuid_has_feat(27, false) && cpuid_has_feat(28, false))) return;
	// Need OSXSAVE
	__asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
	cr4 |= (1 << 18);
	__asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4));
	__asm__ __volatile__("fxsave %0;" :: "m"(simd_fxsave_region));

	__asm__ __volatile__(
		"pushq %rax;"
		"pushq %rcx;"
		"pushq %rdx;"
		"xorq %rcx, %rcx;"
		"xgetbv;"
		"orq 0x7, %rax;"
		"xsetbv;"
		"popq %rdx;"
		"popq %rcx;"
		"popq %rax"
	);
}
