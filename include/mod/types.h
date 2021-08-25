#ifndef __MOD_COMMON_H
#define __MOD_COMMON_H

#include <stdint.h>
#include "sign.h"
#if WASM_64
typedef int64_t intw_t;
typedef uint64_t uintw_t;
#else
typedef int32_t intw_t;
typedef uint32_t uintw_t;
#endif
#if WASM_ENGINE
/** Runtime pointer type */
typedef uintw_t w_ptr;
/** Runtime constant pointer type */
typedef uintw_t w_cptr;
#else
/** Pointer type */
typedef void* w_ptr;
/** Constant pointer type */
typedef const void* w_cptr;
#endif
/** Size type */
typedef uintw_t w_size;

/** Sized data vector */
typedef struct w_iovec {
	w_ptr base; /* Pointer to data. */
	w_size len; /* Length of data. */
} w_iovec;
/** Sized constant data vector */
typedef struct w_ciovec {
	w_cptr base; /* Pointer to data. */
	w_size len; /* Length of data. */
} w_ciovec;

/** Result code */
typedef int32_t w_res;

/** No error occurred */
#define W_SUCCESS (0)
/** Error - Bad address */
#define W_EFAULT (-21)
/** Error - Invalid argument */
#define W_EINVAL (-28)
/** Error - Function not implemented */
#define W_ENOSYS (-52)
/** Error - Not supported */
#define W_ENOTSUP (-58)
/** Error - Result too large */
#define W_ERANGE (-68)

/** Is w_res ok */
#define W_IS_OK(res) ((res) >= W_SUCCESS)
/** Is w_res an error */
#define W_IS_ERR(res) ((res) < W_SUCCESS)

#ifndef W_FN
#define W_FN(mod, name, sign, ret, args) \
ret mod##_##name args \
	__attribute__((__import_module__(#mod), __import_name__(#name)));
#endif

#endif
