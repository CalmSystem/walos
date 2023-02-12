#include "asm.h"

void acpi_shutdown(void *rsdp) {
	io_write16(0xB004, 0x2000); /* Bochs */
	io_write16(0x604, 0x2000); /* Qemu */
	io_write16(0x4004, 0x3400); /* Virtualbox */
	if (rsdp) {
		//TODO: acpi shutdown
	}
	while(1);
}

static void loader_wait(void) {
	interrupt_wait();
}
