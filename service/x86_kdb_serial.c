#include <w/x86.h>

static inline char serial_getc(void) {
	const int COM1 = 0x3F8;
	while ((x86_io_read8(COM1 + 5) & 1) == 0);
	return x86_io_read8(COM1);
}

W_FN_HDL_(hw, key_read, char, ()) {
	return serial_getc();
}
