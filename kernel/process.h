#include <kernel/engine.h>

struct process {
	struct engine_code_reader code;
	const k_signed_call_table* imports;
	struct engine_runtime_ctx e_ctx;
	engine_module* parsed;
	engine_runtime* runtime;
	enum process_kind {
		PK_SERVICE, PK_USER
	}kind;
	union {
		// struct { } service;
		struct {
			const struct process* parent;
		} user;
	};
};
static inline struct process* k_ctx2proc(struct k_runtime_ctx* ctx) {
	return (void*)((char*)ctx - offsetof(struct process, e_ctx));
}
static inline cstr k_ctx2proc_name(struct k_runtime_ctx* ctx) {
	return k_ctx2proc(ctx)->code.name;
}

typedef uint32_t k_pid;
#define NO_PID UINT32_MAX

k_pid w_proc_exec(cstr name, const uint8_t* code, size_t code_len, const struct process* child);
bool w_proc_kill(k_pid, struct process* parent);
