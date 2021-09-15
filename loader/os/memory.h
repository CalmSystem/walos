#ifndef MEMORY_H
#define MEMORY_H
#include "entry.h"

/** Memory usage in pages */
struct memory_state_t {
	uint32_t free, reserved, used;
};

/** Initialize page allocator */
void memory_setup(struct memory_map *);
/** Read informations about memory */
void memory_get_state(struct memory_state_t *);

/** Take pages after addr */
void *pagemap_lock_at(void *start, uint32_t count);
/** Release pages */
void pagemap_unlock_n(void *addr, uint32_t n_pages);

page_t *loader_alloc(size_t n_pages);
void loader_free(page_t*, size_t);

#endif
