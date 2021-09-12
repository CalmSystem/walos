#define K_CTX "linker"
#include "linker.h"
#include "process.h"
#include <kernel/sign_tools.h>
#include <stdlib.h>

static inline void table_link(k_signed_call_table* after, const k_signed_call_table* put) {
	while (after->next) after = (k_signed_call_table*)after->next;
	after->next = put;
}
static inline int table_find(const k_signed_call_table* t, struct k_fn_decl decl, const k_signed_call** out) {
	while (t) {
		for (size_t i = 0; i < t->size; i++) {
			int res;
			if ((res = k_fn_decl_match(decl, t->base[i].decl)) < 0)
				continue;

			if (out) *out = &t->base[i];
			return res;
		}
		t = t->next;
	}
	return -1;
}

static const struct loader_ctx_t* s_ctx = NULL;
void linker_set_features(const struct loader_ctx_t* ctx) {
	table_link(ctx->hw_feats, linker_get_bottom_table());
	s_ctx = ctx;
}

#include <kernel/log_fmt.h>
static inline void call_log_(cstr str, unsigned len) {
	loader_get_handle()->log(str, len);
}
static const w_fn_sign_val __sys_log_sign[] = {ST_VAL, ST_CIO, ST_LEN};
static K_SIGNED_HDL(call_sys_log) {
	enum w_log_level lvl = K__GET(uint32_t, 0);
	if (UNLIKELY(lvl < WL_EMERG || lvl > WL_DEBUG)) return "Invalid log level";
	const k_iovec* iovs = _args[1];
	w_size icnt = K__GET(w_size, 2);
	klog_prefix(call_log_, lvl, k_ctx2proc_name(ctx));
	for (w_size i = 0; i < icnt; i++) {
		call_log_(iovs[i].base, iovs[i].len);
	}
	klog_suffix(call_log_, !icnt || *((cstr)iovs[icnt-1].base + iovs[icnt-1].len - 1) != '\n');
	return NULL;
}
static K_SIGNED_HDL(call_sys_tick) {
	loader_get_handle()->wait();
	return NULL;
}
const k_signed_call_table* linker_get_bottom_table() {
	static const k_signed_call_table s_ = {
		NULL, 2, {
			{call_sys_log, NULL, {"sys", "log", 0, 3, __sys_log_sign}},
			{call_sys_tick, NULL, {"sys", "tick", 0, 0, NULL}}
		}
	};
	return &s_;
}

const k_signed_call_table* linker_get_service_table() { return s_ctx->hw_feats; }

static const w_fn_sign_val __stdio_write_sign[] = {ST_CIO, ST_LEN};
static K_SIGNED_HDL(call_stdio_write) {
	const k_iovec* iovs = _args[0];
	w_size icnt = K__GET(w_size, 1);
	for (w_size i = 0; i < icnt; i++) {
		loader_get_handle()->log(iovs[i].base, iovs[i].len);
	}
	K__RET(w_res, 0) = W_SUCCESS;
	return NULL;
}
static K_SIGNED_HDL(call_stdio_putc) {
	loader_get_handle()->log(_args[0], 1);
	K__RET(w_res, 0) = W_SUCCESS;
	return NULL;
}
static const w_fn_sign_val __sys_exec_sign[] = {ST_ARR, ST_LEN, ST_ARR, ST_LEN};
static K_SIGNED_HDL(call_sys_exec) {
	cstr name = _args[0];

	if (strlen(name) >= K__GET(w_size, 1)) {
		K__RET(w_res, 0) = W_EFAULT;
		return NULL;
	}
	k_pid pid = w_proc_exec(name, _args[2], K__GET(w_size, 3), k_ctx2proc(ctx));
	if (pid == NO_PID) {
		K__RET(w_res, 0) = W_EFAIL;
		return NULL;
	}
	w_proc_kill(pid, k_ctx2proc(ctx));
	K__RET(w_res, 0) = W_SUCCESS;
	return NULL;
}
const k_signed_call_table* linker_get_user_table() {
	static k_signed_call_table s_ = {
		NULL, 3, {
			{call_stdio_write, NULL, {"stdio", "write", 1, 2, __stdio_write_sign}},
			{call_stdio_putc, NULL, {"stdio", "putc", 1, 1, NULL}},
			{call_sys_exec, NULL, {"sys", "exec", 1, 4, __sys_exec_sign}}
		}
	};
	if (UNLIKELY(!s_.next)) {
		table_link(&s_, linker_get_bottom_table());
		table_link(s_ctx->usr_feats, &s_);
	}
	return s_ctx->usr_feats;
}

const k_signed_call* linker_link_proc(struct k_runtime_ctx* ctx, struct k_fn_decl decl) {
	const k_signed_call* out;
	int res = table_find(k_ctx2proc(ctx)->imports, decl, &out);
	if (res < 0) {
		logf(WL_CRIT, "Cannot link %s->%s:%s %s",
			k_ctx2proc_name(ctx), decl.mod, decl.name, w_fn_sign2str(decl));
		return NULL;
	}
	if (res == 0)
		logf(WL_NOTICE, "Suppose signature for %s->%s:%s %s",
			k_ctx2proc_name(ctx), decl.mod, decl.name, w_fn_sign2str(out->decl));

	return out;
}

const k_signed_call_table* linker_bind_table(
	const k_signed_call_table* bottom, bool* is_partial,
	bool (*i_fn)(void* i_arg, struct k_fn_decl*, size_t offset), void* i_arg,
	void* (*e_fn)(void* e_arg, struct k_fn_decl*), void* e_arg,
	bool (*ef_fn)(void* ef_arg, k_signed_call*)
) {
	k_signed_call_table* table = NULL;
	size_t n_imports = 0;
	for (size_t i = 0;; i++) { /* List imports */
		table = realloc(table, sizeof(k_signed_call_table) + (n_imports + 1) * sizeof(k_signed_call));

		struct k_fn_decl* d = &table->base[n_imports].decl;
		if (!i_fn(i_arg, d, i)) break;
		if (table_find(bottom, *d, NULL) >= 0) continue;

		table->base[n_imports].fn = NULL;
		n_imports++;
		logf(WL_DEBUG, "Must find %s:%s", d->mod, d->name);
	}
	table->size = n_imports;
	table->next = bottom;

	n_imports = 0;
	while (n_imports < table->size) { /* List exports */
		struct k_fn_decl decl;
		void* ef_arg = e_fn(e_arg, &decl);
		if (!ef_arg) break;

		for (size_t j = 0; j < table->size; j++) {
			if (table->base[j].fn || k_fn_decl_match(decl, table->base[j].decl) < 0) continue;

			logf(WL_DEBUG, "Found %s:%s", decl.mod, decl.name);
			table->base[j].decl = decl;
			if (!ef_fn(ef_arg, table->base + j)) break;

			n_imports++;
			logf(WL_DEBUG, "Linked %s:%s", decl.mod, decl.name);
		}
	}

	if ((!is_partial || !*is_partial) && n_imports < table->size)
		n_imports = 0;

	if (is_partial) *is_partial = false;
	for (size_t i = 0; i < table->size; i++) {
		if (table->base[i].fn) continue;
		logf(WL_WARN, "Cannot link %s:%s", table->base[i].decl.mod, table->base[i].decl.name);
		if (is_partial) *is_partial = true;
	}
	if (!n_imports) {
		free(table);
		return bottom;
	}

	for (size_t i = 0; i < n_imports; i++) { /* Pack table */
		if (table->base[i].fn) continue;

		size_t j = i;
		do j++; while (!table->base[j].fn);
		memmove(table->base + i, table->base + j, table->size - j);
		table->size -= (j - i);
	}
	table->size = n_imports;
	return table;
}
