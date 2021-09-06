#include "lib.h"
#include <stdlib.h>
#include <string.h>
#include "linker.h"
#include "process.h"
#include <kernel/log.h>

#define ENTRY_NAME "entry.wasm"

static const struct loader_handle* s_loader_hdl;
const struct loader_handle *loader_get_handle() { return s_loader_hdl; }

static engine* s_engine;

static bool os_boot(struct process*, bool allow_partial_linking);

struct processes_table {
	struct processes_table* next;
	struct process base[]; /* PROC_TAB_SIZE len */
};
static struct processes_table* s_procs = NULL;
#define PROC_TAB_SIZE ((PAGE_SIZE - offsetof(struct processes_table, base)) / sizeof(struct process))

static struct process* service_read_next();

void os_entry(const struct loader_ctx_t* ctx) {
	s_loader_hdl = &ctx->handle;
	linker_set_features(ctx);
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
	struct process* entry;
	do {
		entry = service_read_next();
	} while (entry && strcasecmp("entry", entry->code.name) != 0);
	if (!entry) {
		logs(WL_EMERG, ENTRY_NAME " not found");
		return;
	}

	entry->imports = linker_get_user_table();
	if (os_boot(entry, true))
		logs(WL_DEBUG, "Entry finished");
}

static inline bool is_process_present(const struct process* p) {
	return p && p->e_ctx.linker == linker_link_proc;
}
static struct process* proc_alloc() {
	struct process* p = NULL;
	{
		struct processes_table** pt = &s_procs;
		for (;!p && *pt; pt = &(*pt)->next) {
			for (size_t i = 0; i < PROC_TAB_SIZE; i++) {
				if (!is_process_present((*pt)->base + i)) {
					p = (*pt)->base + i;
					break;
				}
			}
		}
		if (!p) {
			*pt = (void*)loader_get_handle()->take_pages(1);
			(*pt)->next = NULL;
			p = (*pt)->base;
		}
	}
	memset(p, 0, sizeof(*p));
	p->e_ctx.linker = linker_link_proc;
	return p;
}
static void proc_unload(struct process* p) {
	if (!is_process_present(p)) return;

	p->e_ctx.linker = NULL;
	if (!p->code.name) return;

	logf(WL_NOTICE, "Unloading process %s", p->code.name);
	if (p->runtime) s_engine->free_runtime(p->runtime);
	else if (p->parsed) s_engine->free_module(p->parsed);

	free((void*)p->code.name);
	if (p->code.stream_arg) free(p->code.stream_arg);
	else if (p->kind == PK_USER) free((void*)p->code.static_code);
	p->imports = NULL;
}

static struct process* service_read_next() {
	static size_t s_offset = 0;
	struct loader_srv_file_t file;
	if (!loader_get_handle()->srv_list(&file, 1, s_offset))
		return NULL;

	s_offset++;

	struct process *const p = proc_alloc();

	const size_t file_name_len = strlen(file.name);
	{
		const size_t srv_name_len = file_name_len - (
			strcasecmp(".wasm", file.name + file_name_len - 5) == 0 ? 5 : 0);
		char* srv_name = malloc(srv_name_len + 1);
		memcpy(srv_name, file.name, srv_name_len);
		srv_name[srv_name_len] = '\0';
		p->code.name = srv_name;
	}

	p->imports = linker_get_service_table();
	p->e_ctx.linker = linker_link_proc;
	p->kind = PK_SERVICE;

	p->code.code_size = file.size;
	if (file.data) {
		p->code.static_code = file.data;
	} else {
		p->code.stream_fn = (engine_code_stream_fn)loader_get_handle()->srv_read; // Cast cstr arg to void*
		p->code.stream_arg = malloc(file_name_len + 1);
		memcpy(p->code.stream_arg, file.name, file_name_len + 1);
	}

	logf(WL_INFO, "Found service: %s", p->code.name);
	return p;
}


static inline struct process* proc_by_pid(k_pid id) {
	// MAYBE: add generation counter
	if (__builtin_expect(id >= NO_PID, 0)) return NULL;
	for (struct processes_table* pt = s_procs; pt; pt = pt->next) {
		if (id < PROC_TAB_SIZE) {
			if (!is_process_present(pt->base + id)) break;
			return pt->base + id;
		} else
			id -= PROC_TAB_SIZE;
	}
	return NULL;
}
static inline k_pid proc_get_pid(const struct process* p) {
	k_pid id = 0;
	for (struct processes_table* pt = s_procs; pt; pt = pt->next) {
		for (size_t i = 0; i < PROC_TAB_SIZE; i++) {
			if (p == pt->base + i) return id;
			id++;
		}
	}
	return NO_PID;
}
static inline bool is_process_owning(const struct process* parent, const struct process* child) {
	if (!is_process_present(parent)) return false;
	while (is_process_present(child) && child->kind == PK_USER) {
		if (child->user.parent == parent) return true;
		child = child->user.parent;
	}
	return false;
}

k_pid w_proc_exec(cstr name, const uint8_t* code, size_t code_len, const struct process* parent) {
	struct process *const p = proc_alloc();

	size_t name_len = strlen(name);
	char* name_buf = malloc(name_len + 1);
	memcpy(name_buf, name, name_len + 1);
	p->code.name = name_buf;

	p->imports = linker_get_user_table();
	p->kind = PK_USER;
	p->user.parent = parent;

	p->code.code_size = code_len;
	uint8_t* code_buf = malloc(code_len);
	memcpy(code_buf, code, code_len);
	p->code.static_code = code_buf;

	logf(WL_INFO, "Created process: %s", p->code.name);
	k_pid pid = proc_get_pid(p);
	os_boot(p, true);
	return pid;
}
bool w_proc_kill(k_pid id, struct process* parent) {
	struct process* child = proc_by_pid(id);
	if (!child || !is_process_owning(parent, child)) return false;
	proc_unload(child);
	return true;
}

static bool os_parse(struct process* p) {
	if (p->parsed) return true;
	logf(WL_DEBUG, "Parsing %s", p->code.name);
	p->parsed = s_engine->parse(s_engine, p->code);
	if (!p->parsed)
		logf(WL_CRIT, "Failed to parse %s", p->code.name);
	return p->parsed;
}
static inline bool lk_import(void* arg, struct k_fn_decl* out, size_t offset) {
	return s_engine->list_imports(arg, out, 1, offset);
}
struct lk_export_t {
	struct process *const caller;
	struct process *cur;
	size_t n_e;
	struct processes_table* table;
	size_t n_s;
};
static inline void* lk_export(void* arg, struct k_fn_decl* out) {
	struct lk_export_t* self = arg;
	while (1) {
		// Find next export
		if (is_process_present(self->cur) && self->cur->kind == PK_SERVICE && self->cur != self->caller) {
			if (os_parse(self->cur)) {
				while (1) {
					if (!s_engine->list_exports(self->cur->parsed, out, 1, self->n_e))
						break;

					self->n_e++;
					return self->cur;
				}
			} else
				proc_unload(self->cur);
		}
		// Find next service
		self->n_e = 0;
		if (self->table) {
			self->cur = self->table->base + self->n_s;
			self->n_s++;
			if (self->n_s >= PROC_TAB_SIZE) {
				self->n_s = 0;
				self->table = self->table->next;
			}
		} else {
			self->cur = service_read_next();
			if (!self->cur) return NULL;
		}
	}
}
static inline bool lk_fn(void* arg, k_signed_call* out) {
	struct process *const p = arg;
	if (!(is_process_present(p) && p->kind == PK_SERVICE && os_boot(p, false))) return false;
	return s_engine->get_export(p->runtime, out->decl, out);
}
static bool os_boot(struct process* p, bool allow_partial_linking) {
	if (p->runtime) return true;
	if (!os_parse(p)) return false;

	logf(WL_DEBUG, "Linking %s", p->code.name);
	struct lk_export_t ex = {p, NULL, 0, s_procs, 0};
	bool partial = allow_partial_linking;
	p->imports = linker_bind_table(p->imports, &partial, lk_import, p->parsed, lk_export, &ex, lk_fn);
	if (partial) {
		logf(WL_ERR, "Failed to fully link %s", p->code.name);
		if (!allow_partial_linking) return false;
	}

	logf(WL_DEBUG, "Booting %s", p->code.name);
	p->runtime = s_engine->boot(p->parsed, 2048, PG_LINK_FLAG | PG_START_FLAG, &p->e_ctx);
	if (!p->runtime)
		logf(WL_CRIT, "Failed to boot %s", p->code.name);
	return p->runtime;
}
