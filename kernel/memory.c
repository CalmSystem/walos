#include "memory.h"
#include "stddef.h"
#include "bitmap.h"
#include "string.h"

struct {
    bitword_t* ptr;
    size_t size;
} pagemap;
struct memory_state_t state;

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

enum EFI_MEMORY_ATTR {
    EFI_MEMORY_UC = 0x0000000000000001,
    EFI_MEMORY_WC = 0x0000000000000002,
    EFI_MEMORY_WT = 0x0000000000000004,
    EFI_MEMORY_WB = 0x0000000000000008,
    EFI_MEMORY_UCE = 0x0000000000000010,
    EFI_MEMORY_WP = 0x0000000000001000,
    EFI_MEMORY_RP = 0x0000000000002000,
    EFI_MEMORY_XP = 0x0000000000004000,
    EFI_MEMORY_NV = 0x0000000000008000,
    EFI_MEMORY_MORE_RELIABLE = 0x0000000000010000,
    EFI_MEMORY_RO = 0x0000000000020000,
    EFI_MEMORY_RUNTIME = 0x8000000000000000
};

static inline void page_set_n(void* addr, uint32_t n_pages, bool lock, uint64_t* from, uint64_t* to) {
    unsigned int start = (uint64_t)addr / PAGE_SIZE;
    for (size_t i = 0; i < n_pages; i++) {
        if (bit_get(pagemap.ptr, start+i) != lock) {
            bit_set(pagemap.ptr, start+i, lock);
            *from -= PAGE_SIZE;
            *to += PAGE_SIZE;
        }
    }
}
static inline void page_lock_n(void* addr, uint32_t n_pages) {
    page_set_n(addr, n_pages, true, &state.free, &state.used);
}
static inline void page_reserve_n(void* addr, uint32_t n_pages) {
    page_set_n(addr, n_pages, true, &state.free, &state.reserved);
}
static inline void page_unlock_n(void* addr, uint32_t n_pages){
    page_set_n(addr, n_pages, false, &state.used, &state.free);
}

static inline page* page_alloc_at(uint32_t count, void* start) {
    for (uint32_t i = (uint64_t)start / PAGE_SIZE + 1; i < pagemap.size; i++) {
        uint32_t found = 0;
        while (found < count && !bit_get(pagemap.ptr, i+found)) found++;
        if (found >= count) {
            page* p = (page*)((uint64_t)i * PAGE_SIZE);
            page_lock_n(p, count);
            return p;
        }
        i += found;
    }
    return (page*)0;
}
page* page_alloc(uint32_t count) {
    return page_alloc_at(count, &_heap_start);
}
page* page_calloc(uint32_t count) {
    page* p = page_alloc(count);
    memset(p, 0, PAGE_SIZE * count);
    return p;
}
void page_free(page* first, uint32_t count) {
    page_unlock_n(first, count);
}

void memory_setup(EFI_MEMORY_MAP* m) {

    void* largest_segment = NULL;
    uint32_t largest_segment_pages = 0;
    state.free = 0;

    for (int i = 0; i < m->size / m->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)(m->ptr + i * m->desc_size);
        state.free += desc->numPages * PAGE_SIZE;
        if (desc->type == EFI_CONVENTIONAL_MEMORY && desc->numPages > largest_segment_pages) {
            largest_segment = desc->physAddr;
            largest_segment_pages = desc->numPages;
        }
    }

    // Initialize pagemap
    pagemap.size = state.free / PAGE_SIZE;
    pagemap.ptr = (bitword_t*)largest_segment;
    memset(pagemap.ptr, 0, pagemap.size / BITS_PER_WORD * sizeof(bitword_t) + 1);
    uint32_t pagemap_pages = pagemap.size / BITS_PER_WORD * sizeof(bitword_t) / PAGE_SIZE + 1;
    page_lock_n(pagemap.ptr, pagemap_pages);

    for (int i = 0; i < m->size / m->desc_size; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)(m->ptr + i * m->desc_size);
        if (desc->type != EFI_CONVENTIONAL_MEMORY &&
            desc->type != EFI_LOADER_CODE &&
            desc->type != EFI_LOADER_DATA &&
            desc->type != EFI_BOOTSERVICES_CODE &&
            desc->type != EFI_BOOTSERVICES_DATA)
        {
            page_reserve_n(desc->physAddr, desc->numPages);
        }
    }

    uint32_t kernel_pages = ((uint64_t)&_kernel_start - (uint64_t)&_kernel_start) / PAGE_SIZE + 1;
    page_lock_n(&_kernel_start, kernel_pages);

}

void memory_get_state(struct memory_state_t* out) { *out = state; }


