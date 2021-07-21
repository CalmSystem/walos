#ifndef __MEMORY_H
#define __MEMORY_H
#include "native.h"
#include "mmap.h"

#define PAGE_SIZE 0x1000
struct memory_state_t {
    uint64_t free;
    uint64_t reserved;
    uint64_t used;
};

/** Initialize page allocator */
void memory_setup(EFI_MEMORY_MAP*);
/** Read informations about memory */
void memory_get_state(struct memory_state_t*);

typedef struct page page;
page* page_alloc(uint32_t count);
page* page_calloc(uint32_t count);
void page_free(page* first, uint32_t count);

#endif
