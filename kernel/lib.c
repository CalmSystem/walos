#include "./lib.h"
#include <string.h>
#include <stdbool.h>

static struct os_ctx_t *CTX;

void os_entry(struct os_ctx_t* ctx) {
	CTX = ctx;
	loader_log("Starting OS\n");
}

static const module_fn* ctx_find(cstr mod, cstr name) {
	const module_fn* ret = NULL;
	unsigned char prio = 0;
	for (size_t i = 0; i < CTX->fncnt; i++) {
		const module_fn* m = CTX->fns + i;
		if ((ret == NULL || m->priority > prio) &&
			strcmp(mod, m->mod) == 0 && strcmp(name, m->name) == 0)
		{
			ret = m;
			prio = m->priority;
		}
	}
	return ret;
}
static inline module_native_fn ctx_find_fn(cstr mod, cstr name) {
	const module_fn *m = ctx_find(mod, name);
	return m && m->type == MOD_FN_NATIVE ? m->fn.native : NULL;
}
#define ctx_with_fn(fn, mod, name) \
static bool __cached_##fn = false; \
static module_native_fn fn; \
{if (!__cached_##fn) { \
	fn = ctx_find_fn(mod, name); \
	__cached_##fn = true; \
} \
if (!fn) return;}

static inline void loader_log(cstr str) {
	ctx_with_fn(fn, "loader", "log");
	const uint32_t len = strlen(str);
	const void *args[] = {str, &len};
	fn((struct w_argvec){args, 2}, (struct w_argvec){NULL, 0});
}