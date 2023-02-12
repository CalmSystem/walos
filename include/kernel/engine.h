#ifndef __KERNEL_ENGINE_H
#define __KERNEL_ENGINE_H

#include "types.h"
#define WASM_ENGINE 1
#include <w/types.h>

/** Parsed program */
typedef struct engine_module engine_module;
/** Booted program */
typedef struct engine_runtime engine_runtime;

typedef size_t (*engine_code_stream_fn)(void* stream_arg, uint8_t* ptr, size_t len, size_t offset);
struct engine_code_reader {
	cstr name;
	union {
		const uint8_t* static_code;
		engine_code_stream_fn stream_fn;
	};
	void* stream_arg;
	uint64_t code_size;
};

struct engine_runtime_ctx {
	const k_signed_call* (*linker)(struct k_runtime_ctx*, struct k_fn_decl);
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
	engine_module* (*parse)(struct engine*, struct engine_code_reader);
	/** List imported functions */
	size_t (*list_imports)(engine_module*, struct k_fn_decl* decls, size_t declcnt, size_t offset);
	/** List exported functions */
	size_t (*list_exports)(engine_module*, struct k_fn_decl* decls, size_t declcnt, size_t offset);
	/** Free module */
	void (*free_module)(engine_module*);

	/** Boot program */
	engine_runtime* (*boot)(engine_module*, uint64_t stack_size, enum engine_boot_flags flags, struct engine_runtime_ctx* ctx);
	/** Callable exported function */
	bool (*get_export)(engine_runtime*, struct k_fn_decl, k_signed_call* out);
	/** Free runtime */
	void (*free_runtime)(engine_runtime*);
} engine;

/** Setup and return self */
engine* engine_load(void);

#endif
