#include "asm.h"

static inline void serial_putc(char c) {
	const int COM1 = 0x3F8;
	if (c == '\n') serial_putc('\r');
	while ((io_read8(COM1 + 5) & 0x20) == 0);
	io_write8(COM1, c);
}

void loader_log(cstr str, size_t len) {
	for (size_t i = 0; i < len; i++)
		serial_putc(*(str++));
}

static inline void llog_out(cstr str, unsigned len) { loader_log(str, len); }
#include <llog.h>
