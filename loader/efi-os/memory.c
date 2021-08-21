#include "memory.h"
#include "stddef.h"
#include "string.h"
#include <utils/bitmap.h>

static struct {
	bitword_t* ptr;
	uint64_t size;
} pagemap;
static struct memory_state_t state;

extern void _kernel_start;
extern void _kernel_end;
extern void _heap_start;

enum EFI_MEMORY_TYPE {
	EFI_RESERVED_MEMORY = 0x00000000,
	EFI_LOADER_CODE = 0x00000001,
	EFI_LOADER_DATA = 0x00000002,
	EFI_BOOTSERVICES_CODE = 0x00000003,
	EFI_BOOTSERVICES_DATA = 0x00000004,
	EFI_RUNTIMESERVICES_CODE = 0x00000005,
	EFI_RUNTIMESERVICES_DATA = 0x00000006,
	EFI_CONVENTIONAL_MEMORY = 0x00000007,
	EFI_UNUSABLE_MEMORY = 0x00000008,
	EFI_ACPI_RECLAIM_MEMORY = 0x00000009,
	EFI_ACPI_NVS_MEMORY = 0x0000000a,
	EFI_MAPPED_IO_MEMORY = 0x0000000b,
	EFI_MAPPED_IO_PORTSPACE_MEMORY = 0x0000000c,
	EFI_PALCODE_MEMORY = 0x0000000d,
	EFI_PERSISTENT_MEMORY = 0x0000000e
};

static inline void pagemap_set_n(void* addr, uint32_t n_pages, bool lock, uint32_t* from, uint32_t* to) {
	unsigned int start = (uint64_t)addr / PAGE_SIZE;
	for (uint32_t i = 0; i < n_pages; i++) {
		if (bit_get(pagemap.ptr, start+i) != lock) {
			bit_set(pagemap.ptr, start+i, lock);
			--*from;
			++*to;
		}
	}
}
static inline void pagemap_lock_n(void* addr, uint32_t n_pages) {
	pagemap_set_n(addr, n_pages, true, &state.free, &state.used);
}
static inline void pagemap_reserve_n(void* addr, uint32_t n_pages) {
	pagemap_set_n(addr, n_pages, true, &state.free, &state.reserved);
}
void pagemap_unlock_n(void* addr, uint32_t n_pages) {
	pagemap_set_n(addr, n_pages, false, &state.used, &state.free);
}

void* pagemap_lock_at(void* start, uint32_t count) {
	for (uint32_t i = (uint64_t)start / PAGE_SIZE + 1; i < pagemap.size; i++) {
		uint32_t found = 0;
		while (found < count && !bit_get(pagemap.ptr, i+found)) found++;
		if (found >= count) {
			void* p = (void*)((uint64_t)i * PAGE_SIZE);
			pagemap_lock_n(p, count);
			return p;
		}
		i += found;
	}
	return NULL;
}

void memory_setup(struct efi_memory_map* m) {

	void* largest_segment = NULL;
	uint32_t largest_segment_pages = 0;
	state.free = 0;

	for (int i = 0; i < m->size / m->desc_size; i++) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)(m->ptr + i * m->desc_size);
		state.free += desc->numPages;
		if (desc->type == EFI_CONVENTIONAL_MEMORY && desc->numPages > largest_segment_pages) {
			largest_segment = desc->physAddr;
			largest_segment_pages = desc->numPages;
		}
	}

	// Initialize pagemap
	pagemap.size = state.free;
	pagemap.ptr = (bitword_t*)largest_segment;
	memset(pagemap.ptr, 0, pagemap.size / BITS_PER_WORD * sizeof(bitword_t) + 1);
	uint32_t pagemap_pages = pagemap.size / BITS_PER_WORD * sizeof(bitword_t) / PAGE_SIZE + 1;
	pagemap_lock_n(pagemap.ptr, pagemap_pages);

	for (int i = 0; i < m->size / m->desc_size; i++) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)(m->ptr + i * m->desc_size);
		if (desc->type != EFI_CONVENTIONAL_MEMORY &&
			desc->type != EFI_LOADER_CODE &&
			desc->type != EFI_LOADER_DATA &&
			desc->type != EFI_BOOTSERVICES_CODE &&
			desc->type != EFI_BOOTSERVICES_DATA)
		{
			pagemap_reserve_n(desc->physAddr, desc->numPages);
		}
	}

	uint32_t kernel_pages = ((uint64_t)&_kernel_start - (uint64_t)&_kernel_start) / PAGE_SIZE + 1;
	pagemap_lock_n(&_kernel_start, kernel_pages);
}

void memory_get_state(struct memory_state_t* out) { *out = state; }

page_t* loader_alloc(size_t n_pages) {
	return (page_t*)pagemap_lock_at(&_heap_start, n_pages);
}
void loader_free(page_t* first, size_t n_pages) {
	pagemap_unlock_n(first, n_pages);
}
