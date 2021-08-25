#include "entry.h"

#include "io.c.h"
#include "initrd.c.h"
#include "acpi.c.h"
#include "memory.h"
#include "vga.c.h"

void _start(struct loader_info *info) {
	interrupt_disable();

	serial_puts("Serial: Ready\n");

	memory_setup(&info->mmap);
	if (info->initrd.list) initrd = &info->initrd;

	size_t featurecnt = 0;
	if (info->lfb) {
		vga_setup(info->lfb);
		featurecnt += 2;
	}

	k_signed_call features[featurecnt];
	size_t i_feature = 0;
	if (info->lfb) {
		// MAYBE: log to screen
		features[i_feature++] = vga_info_call;
		features[i_feature++] = vga_put_call;
	}

	struct os_ctx_t os_ctx = {
		{
			.log=loader_log,
			.wait=loader_wait,
			.take_pages=loader_alloc,
			.release_pages=loader_free,
			.srv_list=loader_srv_list,
			.srv_read=loader_srv_read
		}, {
			features, featurecnt
		}
	};
	os_entry(&os_ctx);

	serial_puts("OS Stopped\n");
	acpi_shutdown(info->acpi_rsdp);
}
