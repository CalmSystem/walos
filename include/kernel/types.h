#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../shr/sign.h"

#define lengthof(arr) (sizeof(arr)/sizeof(arr[0]))

typedef const char* cstr;
/** Kernel iovec */
typedef struct k_iovec {
    void* base; /* Pointer to data. */
    size_t len; /* Length of data. */
} k_iovec;

/** 'this' pointer */
typedef void* self_t;
/** Arguments array */
typedef const void** k_argv_t;
/** Return values array */
typedef void** k_retv_t;

/** Function signature */
struct k_fn_decl {
	cstr mod; /* Group name (optional) */
	cstr name; /* Function name */
	uint32_t retc; /* Number of returned values */
	uint32_t argc; /* Number of parameters */
	const w_fn_sign_val* argv; /* Array of argc parameters */
};
/** Opaque program context */
struct k_runtime_ctx;
typedef cstr (*k_signed_call_fn)(self_t self, k_argv_t args, k_retv_t rets, struct k_runtime_ctx* ctx);
#define K_SIGNED_HDL(name) cstr name(self_t self, k_argv_t _args, k_retv_t _rets, struct k_runtime_ctx* ctx)
#define K__GET(type, idx) *(const type*)_args[idx]
#define K__RET(type, idx) *(type*)_rets[idx]
/** Well known function */
typedef struct k_signed_call {
	k_signed_call_fn fn;
	self_t self;
	struct k_fn_decl decl;
} k_signed_call;

#pragma GCC diagnostic ignored "-Wgnu-flexible-array-initializer"
/** Linked arrays of k_signed_call */
typedef struct k_signed_call_table {
	const struct k_signed_call_table* next;
	size_t size;
	k_signed_call base[];
} k_signed_call_table;

#endif
