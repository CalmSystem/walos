#ifndef __MOD_X86_H
#define __MOD_X86_H
#include "types.h"

W_FN_(x86, io_write8, void, (uint16_t port, uint8_t val)) /* outb */
W_FN_(x86, io_read8, uint8_t, (uint16_t port)) /* inb */
W_FN_(x86, io_write16, void, (uint16_t port, uint16_t val)) /* outw */
W_FN_(x86, io_read16, uint16_t, (uint16_t port)) /* inw */
W_FN_(x86, io_write32, void, (uint16_t port, uint32_t val)) /* outl */
W_FN_(x86, io_read32, uint32_t, (uint16_t port)) /* inl */

#endif
