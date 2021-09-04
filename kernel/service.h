#include <kernel/engine.h>

struct service {
	struct engine_code_reader code;
	const k_signed_call_table* imports;
	struct engine_runtime_ctx e_ctx;
	engine_module* parsed;
	engine_runtime* runtime;
	k_signed_call_table* exports;
};
static inline struct service* k_ctx2srv(struct k_runtime_ctx* ctx) {
	return (void*)((char*)ctx - offsetof(struct service, e_ctx));
}
static inline cstr k_ctx2srv_name(struct k_runtime_ctx* ctx) {
	return k_ctx2srv(ctx)->code.name;
}
