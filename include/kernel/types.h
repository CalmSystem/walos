#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../mod/sign.h"

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

/** Function signature */
struct k_fn_decl {
	cstr mod; /* Group name (optional) */
	cstr name; /* Function name */
	uint32_t retc; /* Number of returned values */
	uint32_t argc; /* Number of parameters */
	const enum w_fn_sign_type* argv; /* Array of argc parameters */
};

#endif
