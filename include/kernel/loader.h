#ifndef __KERNEL_LOADER_H
#define __KERNEL_LOADER_H
#include "types.h"

#define PAGE_SIZE 0x1000
typedef struct page_t page_t;

struct loader_srv_file_t {
	cstr name;
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
	void (*srv_list)(size_t offset, void (*cb)(void* arg, const struct loader_srv_file_t* files, size_t nfiles), void* arg);
	/** Read service content */
	void (*srv_read)(cstr name, void *ptr, size_t len, size_t offset, void (*cb)(void* arg, size_t read), void* arg);
};

struct loader_feature {
	struct k_fn_decl decl;
	union {
		int (*fn)(k_refvec args, k_refvec rets);
		int (*cb)(k_refvec args, size_t retc, void (*cb)(void *arg, const void **retv), void *arg);
	} impl;
	//MAYBE: use uint8_t flags
	bool is_cb;
};
/** Additional functions provided as module functions */
struct loader_features {
	struct loader_feature* ptr; /* Array of features */
	size_t len; /* Number of features */
};

#endif
