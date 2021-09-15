#include "entry.h"

#include "io.c.h"
#include "initrd.c.h"
#include "acpi.c.h"
#include "memory.h"
#include "vga.c.h"
#include "x86.c.h"

void _start(struct loader_info *info) {
	interrupt_disable();

	memory_setup(&info->mmap);
	if (info->initrd) initrd = info->initrd;
	if (info->lfb) vga_setup(info->lfb);

	const struct loader_ctx_t ctx = {
		{
			.log=loader_log,
			.wait=loader_wait,
			.take_pages=loader_alloc,
			.release_pages=loader_free,
			.srv_list=loader_srv_list,
			.srv_read=loader_srv_read
		},
		.hw_feats=&x86_feats,
		.usr_feats=info->lfb ? &vga_feats : &no_vga_feats
	};
	os_entry(&ctx);

	llogs(WL_INFO, "OS Stopped");
	acpi_shutdown((void*)info->acpi_rsdp);
}
