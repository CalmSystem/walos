#ifndef __MOD_COMMON_H
#define __MOD_COMMON_H

#include <stdint.h>
#include "../shr/sign.h"
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

#define lengthof(arr) (sizeof(arr)/sizeof(arr[0]))

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
#define W_IOBUF(str) {{str, sizeof(str)}}

/** Result code */
typedef int32_t w_res;

/** No error occurred */
#define W_SUCCESS (0)
/** Error - Undeffined */
#define W_EFAIL (1)
/** Error - Bad address */
#define W_EFAULT (21)
/** Error - Invalid argument */
#define W_EINVAL (28)
/** Error - Function not implemented */
#define W_ENOSYS (52)
/** Error - Not ready retry later */
#define W_ENOTREADY (57)
/** Error - Not supported */
#define W_ENOTSUP (58)
/** Error - Result too large */
#define W_ERANGE (68)

#ifdef __cplusplus
#define W_C_ABI extern "C"
#else
#define W_C_ABI
#endif
#define W_CNAME(mod, name) mod##_##name
#define W_SNAME(mod, name) #mod ":" #name

#define W_IMPORT W_C_ABI
#define W_IMPORT_AS(mod, name) __attribute__((import_module(#mod), import_name(#name))) W_C_ABI
#define W_EXPORT __attribute__((visibility("default"))) W_C_ABI
#define W_EXPORT_NAMED(name) __attribute__((visibility("default"), export_name(name))) W_C_ABI
#define W_EXPORT_AS(mod, name) W_EXPORT_NAMED(W_SNAME(mod, name))

#if W_NO_LINK_SIGN
#define W_LINK_SIGN(mod, name, ...) ;
#else
#define W_LINK_SIGN(mod, name, ...) W_EXPORT const w_fn_sign_val __##mod##_##name##_sign[] = __VA_ARGS__;
#endif

#ifndef W_FN_
/** Imported function (flat) */
#define W_FN_(mod, name, ret, args) W_IMPORT_AS(mod, name) ret W_CNAME(mod, name) args;
#endif
#ifndef W_FN
/** Imported function (signed) */
#define W_FN(mod, name, ret, args, ...) W_FN_(mod, name, ret, args) W_LINK_SIGN(mod, name, __VA_ARGS__)
#endif

/** Exported function handler (flat) */
#define W_FN_HDL_(mod, name, ret, args) W_EXPORT_AS(mod, name) ret W_CNAME(mod, name) args
/** Exported function handler (signed) */
#define W_FN_HDL(mod, name, ret, args, ...) W_LINK_SIGN(mod, name, __VA_ARGS__) W_FN_HDL_(mod, name, ret, args)
#define W_FN_HDL_REWRITE __attribute__((overloadable))

#endif
