#ifndef ENTRY_H
#define ENTRY_H
#include <kernel/os.h>
#include "../shared/lfb.h"

typedef uint64_t uintptr_l;

#ifndef __EFI_H
#define EFI_MEMORY_RESERVED 0
#define EFI_MEMORY_CONVENSIONAL 7
/** UEFI memory block descriptor */
typedef struct {
	uint32_t type;
	uint32_t padding;
	uintptr_l physAddr;
	uintptr_l virtAddr;
	uint64_t numPages;
	uint64_t attribs;
} EFI_MEMORY_DESCRIPTOR;
#endif

/** UEFI memory pages list */
struct efi_memory_map {
	union {
		void* ptr;
		uintptr_l addr;
	};
	uint64_t size;
	uint64_t desc_size;
};

#define K_PATH "boot\\kernel.elf"
#define ENTRY_PATH "entry.tar.gz"

/** Informations from loader to kernel. Partial Multiboot2 infos */
struct loader_info {
	struct efi_memory_map mmap;
	uintptr_l initrd; /* tar */
	uintptr_l acpi_rsdp;
	union {
		struct linear_frame_buffer* ptr;
		uintptr_l addr;
	} lfb;
};

void _start(struct loader_info *info);

#endif
