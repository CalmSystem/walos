#ifndef K_LIB_H
#define K_LIB_H

#define W_FN(mod, name, sign, ret, args) \
	static cstr __ws_##mod##_##name = sign;
#include <kernel/os.h>

static const module_fn *ctx_find(cstr mod, cstr name);
static inline module_native_fn ctx_find_fn(cstr mod, cstr name);
static inline void loader_log(cstr str);

#endif
