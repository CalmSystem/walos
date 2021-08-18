#ifndef __MOD_COMMON_H
#define __MOD_COMMON_H

#include <stdint.h>
#if W_64
typedef int64_t intw_t;
typedef uint64_t uintw_t;
#else
typedef int32_t intw_t;
typedef uint32_t uintw_t;
#endif

struct w_iovec {
	void *base; /* Pointer to data. */
	uintw_t len; /* Length of data. */
};
struct w_argvec {
	const void **ptr; /* Pointer to argument. */
	uintw_t len; /* Number of arguments. */
};
typedef const char* cstr;

#ifndef W_FN
#define W_FN(mod, name, sign, ret, args) \
ret mod##_##name args \
	__attribute__((__import_module__(#mod), __import_name__(#name)));
#endif

#endif
