#include "entry.h"
#include <kernel/asm.h>

static inline void serial_putc(char c) {
	const int COM1 = 0x3F8;
	if (c == '\n') serial_putc('\r');
	while ((io_read8(COM1 + 5) & 0x20) == 0);
	io_write8(COM1, c);
}
static void serial_puts(cstr s) {
	while (*s)
		serial_putc(*(s++));
}

static int loader_log(struct w_argvec in, struct w_argvec out) {
	if (in.len != 2 || out.len) return -1;
	cstr str = in.ptr[0];
	const uintw_t len = *(const uintw_t *)in.ptr[1];
	for (uintw_t i = 0; i < len && *str; i++)
		serial_putc(*(str++));

	return 0;
}
static int loader_wait(struct w_argvec in, struct w_argvec out) {
	if (in.len || out.len) return -1;
	interrupt_wait();
	return 0;
}

void _start(struct loader_info *info) {
	interrupt_disable();

	serial_puts("Serial: Ready\n");
	// TODO: setup os

	module_fn mfn[] = {
		DECL_MOD_FN_NATIVE("loader", "log", loader_log, 1),
		DECL_MOD_FN_NATIVE("loader", "wait", loader_wait, 1)
	};
	struct os_ctx_t os_ctx = {mfn, sizeof(mfn) / sizeof(mfn[0])};
	os_entry(&os_ctx);

	serial_puts("OS Stopped\n");
	io_write16(0xB004, 0x2000); /* Bochs */
	io_write16(0x604, 0x2000); /* Qemu */
	io_write16(0x4004, 0x3400); /* Virtualbox */
	//TODO: acpi shutdown
}
