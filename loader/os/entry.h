#ifndef ENTRY_H
#define ENTRY_H
#include <kernel/os.h>
#include "../shared/lfb.h"

#if INTPTR_MAX == INT64_MAX
#define PTR_L(type) type*
#else
/** Loader pointer type (64bit) */
typedef uint64_t uintptr_l;
#define PTR_L(type) uintptr_l
#endif

#define MEMORY_MAP_RESERVED 0
#define MEMORY_MAP_USABLE 1
#define MEMORY_MAP_CONVENSIONAL 2
/** Memory map descriptior */
struct memory_map_entry {
	PTR_L(void) base;
	uint64_t num_pages;
	uint32_t type;
	uint32_t unused;
};

/** Memory map list */
struct memory_map {
	PTR_L(struct memory_map_entry) ptr;
	uint64_t count;
};

#define K_PATH "boot\\kernel.elf"
#define ENTRY_PATH "entry.tar.gz"

/** Informations from loader to kernel. Partial Multiboot2 infos */
struct loader_info {
	struct memory_map mmap;
	PTR_L(void) initrd; /* tar */
	PTR_L(void) acpi_rsdp;
	PTR_L(struct linear_frame_buffer) lfb;
};

void _start(struct loader_info *info);

#endif
