#ifndef __KERNEL_OS_H
#define __KERNEL_OS_H

#include "types.h"

#define PAGE_SIZE 0x1000
typedef struct page_t page_t;

struct loader_srv_file_t {
	const char name[256];
	size_t size;
	/** Optional pointer to stable data.
	 *  Of size+1 with final zero */
	const void *data;
};

/** Functions than any loader should provide */
struct loader_handle {
	/** Process next event */
	void (*wait)();
	/** Probably output log somewhere */
	void (*log)(cstr, size_t len);

	/** Reserve n*PAGE_SIZE memory block */
	page_t* (*take_pages)(size_t n_pages);
	/** Release n*PAGE_SIZE memory block */
	void (*release_pages)(page_t* first, size_t n_pages);

	/** List available services */
	size_t (*srv_list)(struct loader_srv_file_t* files, size_t nfiles, size_t offset);
	/** Read service content */
	size_t (*srv_read)(cstr name, uint8_t *ptr, size_t len, size_t offset);
};

/** Informations given to os_entry */
struct loader_ctx_t {
	struct loader_handle handle;
	k_signed_call_table* hw_feats; /* Functions provided to drivers (hardware io) */
	k_signed_call_table* usr_feats; /* Functions provided to users (full features) */
};

void os_entry(const struct loader_ctx_t*);

#endif
