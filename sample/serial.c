#include <w/x86.h>

static inline void serial_putc(char c) {
	const int COM1 = 0x3F8;
	while ((x86_io_read8(COM1 + 5) & 0x20) == 0);
	x86_io_write8(COM1, c);
}

void _start() {
	const char* s = "Hello serial\r\n";
	while (*s) serial_putc(*s++);
}
