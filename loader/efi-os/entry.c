#include "entry.h"

#include "io.c.h"
#include "initrd.c.h"
#include "acpi.c.h"
#include "memory.h"
#include "vga.c.h"
#include "x86.c.h"

void _start(struct loader_info *info) {
	interrupt_disable();

	llogs(WL_DEBUG, "Serial: Ready");

	memory_setup(&info->mmap);
	if (info->initrd.list) initrd = &info->initrd;

	size_t i_feature = lengthof(x86_features);
	if (info->lfb) {
		vga_setup(info->lfb);
		i_feature += lengthof(vga_features);
		// MAYBE: log to screen
	}

	const size_t featurecnt = i_feature;
	k_signed_call features[featurecnt];
	i_feature = 0;
	for (size_t i = 0; i < lengthof(x86_features); i++) {
		features[i_feature++] = x86_features[i];
	}
	for (size_t i = 0; info->lfb && i < lengthof(vga_features); i++) {
		features[i_feature++] = vga_features[i];
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

	llogs(WL_INFO, "OS Stopped");
	acpi_shutdown(info->acpi_rsdp);
}
