#ifndef __MOD_COMMON_H
#define __MOD_COMMON_H

#include <stdint.h>
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

/** Function argument type mapper */
enum w_fn_sign_type {
	ST_BLEN = -5, /* w_size (len of ST_BIO) */
	ST_CLEN, /* w_size (len of ST_CIO) */
	ST_F64, /* Float64 */
	ST_F32, /* Float32 */
	ST_I64, /* Int64 */
	ST_I32, /* Int32 */
	ST_VAL = ST_I32, /* sizeof <= 8 standard value */
	ST_OVAL, /* ST_VAL pointer */
	ST_VEC, /* Sized pointer (len in bytes is next) */
	ST_REFV, /* Vector of ST_VAL pointers (len in w_ptr is next) */
	ST_CIO, /* Vector of const sized pointers (len in w_ciovec is next) */
	ST_BIO /* Vector of sized pointers (len in w_iovec is next) */
};
static inline char w_fn_sign2char(enum w_fn_sign_type s) {
	switch (s)
	{
	case ST_BLEN: return 'b';
	case ST_CLEN: return 'c';
	case ST_F64: return 'F';
	case ST_F32: return 'f';
	case ST_I64: return 'I';
	case ST_I32: return 'i';
	case ST_OVAL: return '*';
	case ST_VEC: return 'V';
	case ST_REFV: return 'R';
	case ST_CIO: return 'C';
	case ST_BIO: return 'B';
	}
}

#ifndef W_FN
#define W_FN(mod, name, sign, ret, args) \
ret mod##_##name args \
	__attribute__((__import_module__(#mod), __import_name__(#name)));
#endif

#endif
