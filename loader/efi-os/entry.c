#include "entry.h"

#include "io.c.h"
#include "initrd.c.h"
#include "acpi.c.h"
#include "memory.h"

void _start(struct loader_info *info) {
	interrupt_disable();

	serial_puts("Serial: Ready\n");

	memory_setup(&info->mmap);
	if (info->initrd.list) initrd = &info->initrd;

	struct os_ctx_t os_ctx = {
		{
			.log=loader_log,
			.wait=loader_wait,
			.take_pages=loader_alloc,
			.release_pages=loader_free,
			.srv_list=loader_srv_list,
			.srv_read=loader_srv_read
		}, {
			NULL, 0
		}
	};
	os_entry(&os_ctx);

	serial_puts("OS Stopped\n");
	acpi_shutdown(info->acpi_rsdp);
}
