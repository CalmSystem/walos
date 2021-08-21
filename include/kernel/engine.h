#ifndef __KERNEL_ENGINE_H
#define __KERNEL_ENGINE_H

#include "types.h"
#define WASM_ENGINE 1
#define W_FN(mod, name, sign, ret, args) static cstr __ws_##mod##_##name = sign;
#include <mod/types.h>

/** Function signature */
struct k_fn_decl {
	cstr mod; /* Group name (optional) */
	cstr name; /* Function name */
	uint32_t retc; /* Number of returned values */
	uint32_t argc; /* Number of parameters */
	const enum w_fn_sign_type* argv; /* Array of argc parameters */
};
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
		*(s++) = w_fn_sign2char(d.argv[i]);
	}
	*(s++) = ')';
	*(s++) = '\0';
	return in;
}
static inline cstr w_fn_sign2str(struct k_fn_decl d) {
	static char buf[32];
	return w_fn_sign2str_r(d, buf);
}

/** Parsed program */
typedef struct {} engine_module;
/** Booted program */
typedef struct {} engine_runtime;
/** Exported function */
typedef struct {} engine_export_fn;

struct engine_code_ref {
	cstr name;
	// MAYBE: stream code
	const uint8_t* code;
	uint64_t code_size;
};

typedef void (*engine_generic_call)(struct k_refvec args, struct k_refvec rets);
struct engine_signed_call {
	engine_generic_call fn;
	struct k_fn_decl decl;
};
struct engine_runtime_ctx {
	void *linker_arg;
	engine_generic_call (*linker)(struct engine_runtime_ctx*, struct k_fn_decl);
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
