#include "lib.h"

void linker_set_features(const struct loader_ctx_t*);
const k_signed_call* linker_link_proc(struct k_runtime_ctx* ctx, struct k_fn_decl decl);

const k_signed_call_table* linker_bind_table(
	const k_signed_call_table*, bool* is_partial,
	bool (*i_fn)(void* i_arg, struct k_fn_decl*, size_t offset), void* i_arg,
	void* (*e_fn)(void* e_arg, struct k_fn_decl*), void* e_arg,
	bool (*ef_fn)(void* ef_arg, k_signed_call*));

/** Table with minimal rights (log and wait) */
const k_signed_call_table* linker_get_bottom_table(void);
/** Table with hardware access */
const k_signed_call_table* linker_get_service_table(void);
/** Table with loader drivers */
const k_signed_call_table* linker_get_user_table(void);
