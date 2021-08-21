#ifndef ASM_H
#define ASM_H

#include <stdint.h>

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

static inline void interrupt_disable() {
	__asm__ __volatile__("cli":::"memory");
}
static inline void interrupt_wait() {
	__asm__ __volatile__("sti; hlt; cli":::"memory");
}

#endif
