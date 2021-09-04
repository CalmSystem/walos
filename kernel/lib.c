#include "lib.h"
#include <stdlib.h>
#include <string.h>
#include "linker.h"
#include "service.h"
#include <kernel/log.h>

#define ENTRY_NAME "entry.wasm"

static const struct loader_handle* s_loader_hdl;
const struct loader_handle *loader_get_handle() { return s_loader_hdl; }

static engine* s_engine;

static bool os_boot(struct service*, bool allow_partial_linking);

struct services_page {
	struct services_page* next;
	struct service base[]; /* SRV_PAGE_SIZE len */
};
static struct services_page* s_services = NULL;
#define SRV_PAGE_SIZE ((PAGE_SIZE - offsetof(struct services_page, base)) / sizeof(struct service))

static struct service* service_read_next();

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
	struct service* entry;
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

static inline bool is_service_present(struct service* p) {
	return p && p->e_ctx.linker == linker_link_srv;
}
static struct service* service_read_next() {
	static size_t s_offset = 0;
	struct loader_srv_file_t file;
	if (!loader_get_handle()->srv_list(&file, 1, s_offset))
		return NULL;

	s_offset++;

	struct service* s = NULL;
	{
		struct services_page** pp = &s_services;
		for (;!s && *pp; pp = &(*pp)->next) {
			for (size_t i = 0; i < SRV_PAGE_SIZE; i++) {
				if (!is_service_present((*pp)->base + i)) {
					s = (*pp)->base + i;
					break;
				}
			}
		}
		if (!s) {
			*pp = (void*)loader_get_handle()->take_pages(1);
			(*pp)->next = NULL;
			s = (*pp)->base;
		}
	}
	memset(s, 0, sizeof(*s));

	const size_t file_name_len = strlen(file.name);
	{
		const size_t srv_name_len = file_name_len - (
			strcasecmp(".wasm", file.name + file_name_len - 5) == 0 ? 5 : 0);
		char* srv_name = malloc(srv_name_len + 1);
		memcpy(srv_name, file.name, srv_name_len);
		srv_name[srv_name_len] = '\0';
		s->code.name = srv_name;
	}

	s->imports = linker_get_service_table();
	s->e_ctx.linker = linker_link_srv;

	s->code.code_size = file.size;
	if (file.data) {
		s->code.static_code = file.data;
	} else {
		s->code.stream_fn = (engine_code_stream_fn)loader_get_handle()->srv_read; // Cast cstr arg to void*
		s->code.stream_arg = malloc(file_name_len + 1);
		memcpy(s->code.stream_arg, file.name, file_name_len + 1);
	}

	logf(WL_INFO, "Found service: %s", s->code.name);
	return s;
}
static inline void service_unload(struct service* s) {
	if (!is_service_present(s)) return;

	logf(WL_NOTICE, "Unloading service %s", s->code.name);
	if (s->runtime) s_engine->free_runtime(s->runtime);
	else if (s->parsed) s_engine->free_module(s->parsed);

	free((void*)s->code.name);
	if (s->code.stream_arg) free(s->code.stream_arg);
	s->imports = NULL;
	s->e_ctx.linker = NULL;
	const k_signed_call_table* exp = s->exports;
	while (exp) {
		void* prev = (void*)exp;
		exp = (k_signed_call_table*)exp->next;
		free(prev);
	}
}

static bool os_parse(struct service* s) {
	if (s->parsed) return true;
	logf(WL_DEBUG, "Parsing %s", s->code.name);
	s->parsed = s_engine->parse(s_engine, s->code);
	if (!s->parsed)
		logf(WL_CRIT, "Failed to parse %s", s->code.name);
	return s->parsed;
}
static inline bool lk_import(void* arg, struct k_fn_decl* out, size_t offset) {
	return s_engine->list_imports(arg, out, 1, offset);
}
struct lk_export_t {
	struct service *const caller;
	struct service *cur;
	size_t n_e;
	struct services_page* page;
	size_t n_s;
};
static inline void* lk_export(void* arg, struct k_fn_decl* out) {
	struct lk_export_t* self = arg;
	while (1) {
		// Find next export
		if (is_service_present(self->cur) && self->cur != self->caller) {
			if (os_parse(self->cur)) {
				while (1) {
					if (!s_engine->list_exports(self->cur->parsed, out, 1, self->n_e))
						break;

					self->n_e++;
					return self->cur;
				}
			} else
				service_unload(self->cur);
		}
		// Find next service
		self->n_e = 0;
		if (self->page) {
			self->cur = self->page->base + self->n_s;
			self->n_s++;
			if (self->n_s >= SRV_PAGE_SIZE) {
				self->n_s = 0;
				self->page = self->page->next;
			}
		} else {
			self->cur = service_read_next();
			if (!self->cur) return NULL;
		}
	}
}
static inline bool lk_fn(void* arg, k_signed_call* out) {
	struct service *const s = arg;
	if (!(is_service_present(s) && os_boot(s, false))) return false;
	return s_engine->get_export(s->runtime, out->decl, out);
}
static bool os_boot(struct service* s, bool allow_partial_linking) {
	if (s->runtime) return true;
	if (!os_parse(s)) return false;

	logf(WL_DEBUG, "Linking %s", s->code.name);
	struct lk_export_t ex = {s, NULL, 0, s_services, 0};
	bool partial = allow_partial_linking;
	s->imports = linker_bind_table(s->imports, &partial, lk_import, s->parsed, lk_export, &ex, lk_fn);
	if (partial) {
		logf(WL_ERR, "Failed to fully link %s", s->code.name);
		if (!allow_partial_linking) return false;
	}

	logf(WL_DEBUG, "Booting %s", s->code.name);
	s->runtime = s_engine->boot(s->parsed, 2048, PG_LINK_FLAG | PG_START_FLAG, &s->e_ctx);
	if (!s->runtime)
		logf(WL_CRIT, "Failed to boot %s", s->code.name);
	return s->runtime;
}
