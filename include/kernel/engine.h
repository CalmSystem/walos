#ifndef __KERNEL_ENGINE_H
#define __KERNEL_ENGINE_H

#include "types.h"
#define WASM_ENGINE 1
#define W_FN(mod, name, sign, ret, args) static cstr __ws_##mod##_##name = sign;
#include <mod/types.h>
#include <string.h>

static inline cstr w_fn_sign2str_r(struct k_fn_decl d, char* in) {
	char* s = in;
	if (d.retc == 0)
		*(s++) = 'v';
	else {
		for (uint32_t i = 0; i < d.retc; i++)
			*(s++) = 'i';
	}
	*(s++) = '(';
	for (uint32_t i = 0; i < d.argc; i++) {
		*(s++) = d.argv ? w_fn_sign2char(d.argv[i]) : '?';
	}
	*(s++) = ')';
	*(s++) = '\0';
	return in;
}
static inline cstr w_fn_sign2str(struct k_fn_decl d) {
	static char buf[32];
	return w_fn_sign2str_r(d, buf);
}
static inline bool k_fn_decl_flat(struct k_fn_decl decl) {
	for (uint32_t i = 0; decl.argv && i < decl.argc; i++) {
		if (decl.argv[i] > ST_VAL)
			return false;
	}
	return true;
}
static inline int k_fn_decl_match(struct k_fn_decl decl, struct k_fn_decl into) {
	if (!(decl.retc == into.retc && decl.argc == into.argc &&
		(decl.name == into.name || strcmp(decl.name, into.name) == 0) &&
		(decl.mod == into.mod || strcmp(decl.name, into.name) == 0)))
	return -1;

	if (decl.argv == into.argv)
		return 1;
	if (!decl.argv)
		return 0;
	if (!into.argv)
		return k_fn_decl_flat(decl);

	for (uint32_t i = 0; i < decl.argc; i++) {
		if (decl.argv[i] != into.argv[i])
			return -1;
	}
	return 1;
}

/** Parsed program */
typedef struct engine_module engine_module;
/** Booted program */
typedef struct engine_module engine_runtime;
/** Exported function */
typedef struct engine_module engine_export_fn;

struct engine_code_ref {
	cstr name;
	// MAYBE: stream code
	const uint8_t* code;
	uint64_t code_size;
};

struct engine_runtime_ctx;
typedef cstr (*engine_generic_call)(struct k_refvec args, struct k_refvec rets);
typedef cstr (*engine_signed_call_fn)(const void **args, void **rets, struct engine_runtime_ctx* ctx);
typedef struct engine_signed_call {
	engine_signed_call_fn fn;
	struct k_fn_decl decl;
} engine_signed_call;
struct engine_runtime_ctx {
	engine_signed_call* (*linker)(struct engine_runtime_ctx*, struct k_fn_decl);
};
enum engine_boot_flags {
	PG_NO_FLAG = 0,
	PG_LINK_FLAG = 1 << 0,
	PG_COMPILE_FLAG = 1 << 1,
	PG_START_FLAG = 1 << 2
};

/** Engine class */
typedef struct engine {
	/** Parse program */
	engine_module* (*parse)(struct engine*, struct engine_code_ref);
	/** List imported functions */
	size_t (*list_imports)(engine_module*, struct k_fn_decl* decls, size_t declcnt, size_t offset);
	/** List exported functions */
	size_t (*list_exports)(engine_module*, struct k_fn_decl* decls, size_t declcnt, size_t offset);
	/** Free module */
	void (*free_module)(engine_module*);

	/** Boot program */
	engine_runtime* (*boot)(engine_module*, uint64_t stack_size, enum engine_boot_flags flags, struct engine_runtime_ctx* ctx);
	/** List exported function */
	engine_export_fn *(*get_export)(engine_runtime*, struct k_fn_decl);
	/** Call exported function */
	int (*call)(engine_export_fn*, struct k_refvec args, struct k_refvec rets);
	/** Free runtime */
	void (*free_runtime)(engine_runtime*);
} engine;

/** Setup and return self */
engine* engine_load();

#endif
