#include "lib.h"
#include <stdlib.h>
#include <kernel/engine.h>
#include <kernel/log.h>
#include <kernel/sign_tools.h>
#include <utils/xxd.h>

#define ENTRY_NAME "entry.wasm"

static void srv_list_cb(void*, const struct loader_srv_file_t*, size_t);
static void entry_run();

static const struct os_ctx_t* s_ctx;
const struct loader_handle *loader_get_handle() { return &s_ctx->handle; }
const struct loader_features *loader_get_feats() { return &s_ctx->feats; }

static bool s_running = true;
static inline void shutdown() { s_running = false; }

static engine* s_engine;

void os_entry(const struct os_ctx_t* ctx) {
	s_ctx = ctx;
	logs(WL_EMERG, "Starting OS");
#if DEBUG
	logs(WL_NOTICE, "DEBUG mode enabled");
#endif
	if (!ctx->handle.wait) {
		logs(WL_EMERG, "Invalid loader");
		return;
	}

	s_engine = engine_load();
	if (!s_engine) {
		logs(WL_EMERG, "Engine load failed");
		return;
	}

	logs(WL_NOTICE, "Loading services");
	loader_get_handle()->srv_list(0, srv_list_cb, NULL);

	while (s_running)
		ctx->handle.wait();
}

static struct loader_srv_file_t entry_file = {0};
static void srv_list_cb(void* offset, const struct loader_srv_file_t* files, size_t n_files) {
	if (!n_files) {
		entry_run();
		return;
	}

	for (size_t i = 0; i < n_files; i++) {
		logf(WL_INFO, "Found service: %s\n", files[i].name);

		if (strcmp(files[i].name, ENTRY_NAME) == 0)
			entry_file = files[i];
		//TODO: create service
		//TODO: register exports
	}

	/* Loading next */
	size_t next = (size_t)offset + n_files;
	loader_get_handle()->srv_list(next, srv_list_cb, (void*)next);
}

struct k_runtime_ctx {
	struct engine_runtime_ctx engine;
	cstr name;
};
static enum w_fn_sign_type __stdio_write_sign[] = {ST_CIO, ST_LEN};
static cstr stdlib_stdio_write(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	const k_iovec* iovs = args[0];
	w_size icnt = *(const w_size*)args[1];
	for (w_size i = 0; i < icnt; i++) {
		loader_get_handle()->log(iovs[i].base, iovs[i].len);
	}
	*(w_res*)rets[0] = W_SUCCESS;
	return NULL;
}
static cstr stdlib_stdio_putc(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	loader_get_handle()->log((const char*)args[0], 1);
	*(w_res*)rets[0] = W_SUCCESS;
	return NULL;
}
static cstr stdlib_stdio_none(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	return "No stdin";
}

#include <kernel/log_fmt.h>
static inline void stdlib_log_(cstr str, unsigned len) {
	loader_get_handle()->log(str, len);
}
static enum w_fn_sign_type __log_write_sign[] = {ST_VAL, ST_CIO, ST_LEN};
static cstr stdlib_log_write(const void **args, void **rets, struct k_runtime_ctx* ctx) {
	enum w_log_level lvl = *(const uint32_t*)args[0];
	if (__builtin_expect(lvl < WL_EMERG || lvl > WL_DEBUG, 0)) return "Invalid log level";
	const k_iovec* iovs = args[1];
	w_size icnt = *(const w_size*)args[2];
	klog_prefix(stdlib_log_, lvl, ctx->name);
	for (w_size i = 0; i < icnt; i++) {
		stdlib_log_(iovs[i].base, iovs[i].len);
	}
	klog_suffix(stdlib_log_, !icnt || *((cstr)iovs[icnt-1].base + iovs[icnt-1].len - 1) != '\n');
	return NULL;
}

static struct k_signed_call stdlib[] = {
	{stdlib_stdio_write, {"stdio", "write", 1, 2, __stdio_write_sign}},
	{stdlib_stdio_putc,  {"stdio", "putc", 1, 1, NULL}},
	{stdlib_stdio_none,  {"stdio", "getc", 1, 0, NULL}},
	{stdlib_log_write,  {"log", "write", 0, 3, __log_write_sign}}
};

static k_signed_call* entry_linker(struct k_runtime_ctx* self, struct k_fn_decl decl) {
	struct loader_features libs[] = { //TODO: by process ctx
		{stdlib, lengthof(stdlib)},
		*loader_get_feats()
	};
	if (stdlib[2].fn == stdlib_stdio_none) {
		struct k_fn_decl kw_key_read_decl = {"hw", "key_read", 1, 0, NULL};
		for (size_t i = 0; i < loader_get_feats()->len; i++) {
			if (k_fn_decl_match(kw_key_read_decl, loader_get_feats()->ptr[i].decl) < 0)
				continue;

			stdlib[2].fn = loader_get_feats()->ptr[i].fn;
			break;
		}
	}

	for (size_t j = 0; j < lengthof(libs); j++) {
		for (size_t i = 0; i < libs[j].len; i++) {
			int res;
			if ((res = k_fn_decl_match(decl, libs[j].ptr[i].decl)) < 0)
				continue;
			if (res == 0)
				logf(WL_NOTICE, "Suppose signature for %s:%s %s\n",
					decl.mod, decl.name, w_fn_sign2str(libs[j].ptr[i].decl));
			return &libs[j].ptr[i];
		}
	}
	logf(WL_CRIT, "Cannot link %s:%s %s\n", decl.mod, decl.name, w_fn_sign2str(decl));
	return NULL;
}
static void entry_read_cb(void *offset, size_t part_size) {
	size_t read = (size_t)offset + part_size;
	if (read >= entry_file.size) {
		engine_module* entry_mod = s_engine->parse(s_engine, (struct engine_code_ref){
			ENTRY_NAME, entry_file.data, entry_file.size });
		if (!entry_mod) {
			logs(WL_CRIT, "Failed to parse entry");
			shutdown();
			return;
		}

		logs(WL_NOTICE, "Running " ENTRY_NAME);
		struct k_runtime_ctx ctx = {{entry_linker}, entry_file.name};
		if (!s_engine->boot(entry_mod, 2048, PG_LINK_FLAG | PG_START_FLAG, &ctx)) {
			logs(WL_CRIT, "Failed to run entry");
		}
		shutdown();

	} else {
		if (read) /* Read more */
			loader_get_handle()->srv_read(entry_file.name, (uint8_t*)entry_file.data + read,
				entry_file.size - read, read, entry_read_cb, (void*)read);
		else {
			logs(WL_CRIT, ENTRY_NAME " not readable");
			*(char *)entry_file.data = '\0';
			shutdown();
		}
	}
}
static void entry_run() {
	if (!entry_file.name) {
		logs(WL_CRIT, ENTRY_NAME " not found");
		shutdown();
		return;
	}

	if (entry_file.data) {
		entry_read_cb((void*)entry_file.size, 0);
	} else {
		logs(WL_DEBUG, "Reading " ENTRY_NAME);
		entry_file.data = calloc(1, entry_file.size+1);
		loader_get_handle()->srv_read(entry_file.name, (void*)entry_file.data,
			entry_file.size, 0, entry_read_cb, (void *)0);
	}
}
