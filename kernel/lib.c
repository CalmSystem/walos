#include "./lib.h"
#include <kernel/engine.h>
#include <kernel/log.h>
#include <string.h>
#include <stdlib.h>
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
	loader_get_handle()->srv_list((size_t)offset + n_files, srv_list_cb, offset + n_files);
}

static void engine_stdout_write_call(k_refvec args, k_refvec rets) {
	// assert(args.len == 2 && rets.len == 1);
	const k_iovec* iovs = args.ptr[0];
	w_size icnt = *(const w_size*)args.ptr[1];
	for (w_size i = 0; i < icnt; i++) {
		loader_get_handle()->log(iovs[i].base, iovs[i].len);
	}
	*(w_res*)rets.ptr[0] = W_SUCCESS;
}
static engine_generic_call entry_linker(struct engine_runtime_ctx* self, struct k_fn_decl decl) {
	if (strcmp("stdout", decl.mod) == 0 && strcmp("write", decl.name) == 0) {
		return engine_stdout_write_call;
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

		struct engine_runtime_ctx ctx = {
			NULL, entry_linker
		};
		if (!s_engine->boot(entry_mod, 2048, PG_LINK_FLAG | PG_START_FLAG, &ctx)) {
			logs(KL_CRIT, "Failed to run entry");
		}
		shutdown();

	} else {
		if (read) /* Read more */
			loader_get_handle()->srv_read(entry_file.name, (void*)entry_file.data + read,
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
