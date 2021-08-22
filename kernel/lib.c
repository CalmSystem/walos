#include "./lib.h"
#include <kernel/engine.h>
#include <kernel/log.h>
#include <utils/xxd.h>

#define ENTRY_NAME "entry.wasm"

static void srv_list_cb(void*, const struct loader_srv_file_t*, size_t);
static void entry_run();

static const struct os_ctx_t* s_ctx;
const struct loader_handle *loader_get_handle() { return &s_ctx->handle; }

static bool s_running = true;
static inline void shutdown() { s_running = false; }

static engine* s_engine;

void os_entry(const struct os_ctx_t* ctx) {
	s_ctx = ctx;
	logs(KL_EMERG, "Starting OS");
#if DEBUG
	logs(KL_NOTICE, "DEBUG mode enabled");
#endif
	if (!ctx->handle.wait) {
		logs(KL_EMERG, "Invalid loader");
		return;
	}

	s_engine = engine_load();
	if (!s_engine) {
		logs(KL_EMERG, "Engine load failed");
		return;
	}

	logs(KL_NOTICE, "Loading services");
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
		logf(KL_INFO, "Found service: %s\n", files[i].name);

		if (strcmp(files[i].name, ENTRY_NAME) == 0)
			entry_file = files[i];
		//TODO: create service
		//TODO: register exports
	}

	/* Loading next */
	size_t next = (size_t)offset + n_files;
	loader_get_handle()->srv_list(next, srv_list_cb, (void*)next);
}

static enum w_fn_sign_type __stdout_write_sign[] = {ST_CIO, ST_CLEN};
static cstr stdlib_stdout_write(const void **args, void **rets, struct engine_runtime_ctx* ctx) {
	const k_iovec* iovs = args[0];
	w_size icnt = *(const w_size*)args[1];
	for (w_size i = 0; i < icnt; i++) {
		loader_get_handle()->log(iovs[i].base, iovs[i].len);
	}
	*(w_res*)rets[0] = W_SUCCESS;
	return NULL;
}
static cstr stdlib_stdout_putc(const void **args, void **rets, struct engine_runtime_ctx* ctx) {
	loader_get_handle()->log((const char*)args[0], 1);
	*(w_res*)rets[0] = W_SUCCESS;
	return NULL;
}

static struct engine_signed_call stdlib_signed[] = {
	{stdlib_stdout_write, {"stdout", "write", 1, 2, __stdout_write_sign}},
	{stdlib_stdout_putc,  {"stdout", "putc", 1, 1, NULL}}
};

static engine_signed_call* entry_linker(struct engine_runtime_ctx* self, struct k_fn_decl decl) {
	for (size_t i = 0; i < lengthof(stdlib_signed); i++) {
		int res;
		if ((res = k_fn_decl_match(decl, stdlib_signed[i].decl)) < 0)
			continue;
		if (res == 0)
			logf(KL_NOTICE, "Suppose signature for %s:%s %s\n",
				  decl.mod, decl.name, w_fn_sign2str(stdlib_signed[i].decl));
		return &stdlib_signed[i];
	}
	logf(KL_CRIT, "Cannot link %s:%s %s\n", decl.mod, decl.name, w_fn_sign2str(decl));
	return NULL;
}
static void entry_read_cb(void *offset, size_t part_size) {
	size_t read = (size_t)offset + part_size;
	if (read >= entry_file.size) {
		logs(KL_NOTICE, "Running " ENTRY_NAME);

		engine_module* entry_mod = s_engine->parse(s_engine, (struct engine_code_ref){
			ENTRY_NAME, entry_file.data, entry_file.size });
		if (!entry_mod) {
			logs(KL_CRIT, "Failed to parse entry");
			shutdown();
			return;
		}

		struct engine_runtime_ctx ctx = { entry_linker };
		if (!s_engine->boot(entry_mod, 2048, PG_LINK_FLAG | PG_START_FLAG, &ctx)) {
			logs(KL_CRIT, "Failed to run entry");
		}
		shutdown();

	} else {
		if (read) /* Read more */
			loader_get_handle()->srv_read(entry_file.name, (uint8_t*)entry_file.data + read,
				entry_file.size - read, read, entry_read_cb, (void*)read);
		else {
			logs(KL_CRIT, ENTRY_NAME " not readable");
			*(char *)entry_file.data = '\0';
			shutdown();
		}
	}
}
static void entry_run() {
	if (!entry_file.name) {
		logs(KL_CRIT, ENTRY_NAME " not found");
		shutdown();
		return;
	}

	if (entry_file.data) {
		entry_read_cb((void*)entry_file.size, 0);
	} else {
		logs(KL_DEBUG, "Reading " ENTRY_NAME);
		entry_file.data = calloc(1, entry_file.size+1);
		loader_get_handle()->srv_read(entry_file.name, (void*)entry_file.data,
			entry_file.size, 0, entry_read_cb, (void *)0);
	}
}
