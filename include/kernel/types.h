#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

#include <stdlib.h>
typedef const char* cstr;
/** Kernel iovec */
typedef struct k_iovec {
    void* base; /* Pointer to data. */
    size_t len; /* Length of data. */
} k_iovec;
typedef struct k_refvec {
	const void **ptr; /* Array of pointers to argument. */
	size_t len; /* Length of array. */
} k_refvec;

#endif
