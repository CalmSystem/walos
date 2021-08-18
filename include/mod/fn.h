#ifndef __MOD_MOD_H
#define __MOD_MOD_H

#include "./common.h"

typedef void (*module_fn_cb)(void *arg, int ret, struct w_argvec out);

/** Loader provided sync function pointer */
typedef int (*module_native_fn)(struct w_argvec in, struct w_argvec out);
/** Loader provided async function pointer */
typedef int (*module_native_async_fn)(struct w_argvec in, module_fn_cb cb, void *arg);

/** Engine internal function pointer */
typedef struct module_engine_fn {
	void *cb, *arg;
} module_engine_fn;

/** Module function declaration */
typedef struct module_fn {
	/** Owning module */
	cstr mod;
	/** Function name */
	cstr name;
	union {
		module_native_fn native;
		module_native_async_fn native_async;
		/** Service name */
		cstr promise;
		module_engine_fn engine;
	} fn;
	unsigned char priority;
	enum module_fn_type {
		MOD_FN_NATIVE, MOD_FN_NATIVE_ASYNC,
		MOD_FN_PROMISE, MOD_FN_ENGINE
	} type;
} module_fn;

#define DECL_MOD_FN_NATIVE(mod, name, ptr, priority) {(mod), (name), {.native=(ptr)}, (priority), MOD_FN_NATIVE}
#define DECL_MOD_FN_NATIVE_ASYNC(mod, name, ptr, priority) {(mod), (name), {.native_async=(ptr)}, (priority), MOD_FN_NATIVE_ASYNC}

#endif
