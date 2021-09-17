#include "wasm3.h"
#include <kernel/engine.h>
#define K_CTX "engine"
#include <kernel/log.h>
#include <kernel/sign_tools.h>

/* Read all into static_code */
static inline bool engine_code_unstream(struct engine_code_reader* ref) {
	if (!ref->stream_arg) return true;
	uint8_t* ptr = malloc(ref->code_size+1);
	if (!ptr) return false;
	ptr[ref->code_size] = '\0';

	for (size_t offset = 0; offset < ref->code_size;) {
		size_t read = ref->stream_fn(ref->stream_arg, ptr, ref->code_size - offset, offset);
		if (!read) {
			free(ptr);
			logf(WL_CRIT, "%s not readable", ref->name);
			return false;
		}
		offset += read;
	}
	ref->static_code = ptr;
	ref->stream_arg = NULL;
	return true;
}

typedef struct engine_m3 {
	engine handle;
	IM3Environment env;
} engine_m3;

static engine_module *engine_m3_parse(struct engine *self, struct engine_code_reader ref) {
	IM3Module mod;
	if (!engine_code_unstream(&ref))
		return NULL;
	M3Result res = m3_ParseModule(((struct engine_m3*)self)->env, &mod, ref.static_code, ref.code_size);
	if (res) {
		logf(WL_ERR, "While parsing %s: %s", ref.name, res);
		return NULL;
	}
	m3_SetModuleName(mod, ref.name);
	return (void*)mod;
}
static inline void engine_m3_read_argv(IM3Module mod, struct k_fn_decl *d, bool log) {
	d->argv = NULL;
	IM3Global sign;
	{
		const size_t mod_len = strlen(d->mod);
		const size_t name_len = strlen(d->name);
		//__##mod##_##name##_sign
		char sign_name[2 + mod_len + 1 + name_len + 6];
		memcpy(sign_name, "__", 2);
		memcpy(sign_name + 2, d->mod, mod_len);
		sign_name[2 + mod_len] = '_';
		memcpy(sign_name + 2 + mod_len + 1, d->name, name_len);
		memcpy(sign_name + 2 + mod_len + 1 + name_len, "_sign", 6);
		sign = m3_FindGlobal(mod, sign_name);
	}
	M3TaggedValue sign_val;
	if (sign && m3_GetGlobalType(sign) == c_m3Type_i32 && !m3_ParseGlobal(sign, &sign_val)) {
		uint32_t mem_view_size = 0;
		d->argv = (const void*)m3_ParseMemory(mod, &mem_view_size, 0, sign_val.value.i32);
		if (d->argv && mem_view_size >= d->argc) {
			size_t i_ck = 0;
			while (i_ck < d->argc && (d->argv[i_ck] >= ST_MIN && d->argv[i_ck] <= ST_MAX)) i_ck++;
			if (i_ck < d->argc) {
				if (log) logf(WL_WARN, "Invalid format in global signature for %s->%s:%s %s",
					m3_GetModuleName(mod), d->mod, d->name, w_fn_sign2str(*d));
				d->argv = NULL;
			}
		} else {
			if (log) logf(WL_WARN, "Invalid ptr in global signature for %s->%s:%s %s",
				m3_GetModuleName(mod), d->mod, d->name, w_fn_sign2str(*d));
		}
	}

	//TODO: read from custom section
	if (log && d->argc && !d->argv)
		logf(WL_INFO, "No signature for %s->%s:%s %s",
				m3_GetModuleName(mod), d->mod, d->name, w_fn_sign2str(*d));
}
static size_t engine_m3_list_imports(engine_module *mod, struct k_fn_decl *decls, size_t declcnt, size_t offset) {
	for (size_t i = 0; i < declcnt; i++) {
		IM3Function ifn;
		if (m3_FindFunctionDecl((IM3Module)mod, false, offset + i, &ifn))
			return i;

		struct k_fn_decl* d = decls + i;
		m3_GetFunctionImportName(ifn, &d->mod, &d->name);
		d->retc = m3_GetRetCount(ifn);
		d->argc = m3_GetArgCount(ifn);
		engine_m3_read_argv((IM3Module)mod, d, false);
	}
	return declcnt;
}
static size_t engine_m3_list_exports(engine_module *mod, struct k_fn_decl *decls, size_t declcnt, size_t offset) {
	for (size_t i = 0; i < declcnt; i++) {
		IM3Function ifn;
		if (m3_FindFunctionDecl((IM3Module)mod, true, offset + i, &ifn))
			return i;

		struct k_fn_decl* d = decls + i;
		//TODO: read export mapping globals and custom section
		m3_GetFunctionExportName(ifn, &d->mod, &d->name);
		d->retc = m3_GetRetCount(ifn);
		d->argc = m3_GetArgCount(ifn);
		engine_m3_read_argv((IM3Module)mod, d, false);
	}
	return declcnt;
}
static void engine_m3_free_module(engine_module *mod) {
	m3_FreeModule((IM3Module)mod);
}

/** Some dark magic...
 *  Read args and rets from _sp.
 *  Convert map offset to pointer.
 *  Convert w_iovec to k_iovec.
 *  Play a lot with stack */
static m3ApiRawFunction(engine_m3_link_signed) {
	const uint32_t ret_cnt = m3_GetRetCount(_ctx->function);
	const uint32_t arg_cnt = m3_GetArgCount(_ctx->function);
	uint64_t *const ret_p = _sp;
	_sp += ret_cnt;
	uint64_t *const arg_p = _sp;
	_sp += arg_cnt;

	void *rets[ret_cnt];
	for (uint32_t i = 0; i < ret_cnt; i++) {
		rets[i] = ret_p + i;
	}

	/*
		struct k_signed_call *s_call = _ctx->userdata;
		assert(s_call->decl.retc == ret_cnt && s_call->decl.argc == arg_cnt);
	*/
	const w_fn_sign_val *const sign = ((k_signed_call*)_ctx->userdata)->decl.argv;

	size_t ind_cnt = 0; /* Indirection buffer size */
	const void *args[arg_cnt];
	for (uint32_t i = 0; i < arg_cnt; i++) {
		/* Map arguments */
		if (sign[i] > ST_VAL) {
			args[i] = m3ApiOffsetToPtr(m3ApiReadMem32(arg_p + i));
			uint32_t len = m3ApiReadMem32(arg_p + i + 1); //MAYBE: sign[i].len_idx
			switch (sign[i]) {
			case ST_BIO:
			case ST_CIO:
				ind_cnt += len * sizeof(k_iovec);
				len *= sizeof(w_iovec);
				break;
			case ST_REFV:
				ind_cnt += len * sizeof(void*);
				len *= sizeof(w_ptr);
				break;
			case ST_ARR:
				len *= sizeof(uint8_t);
				break;
			case ST_PTR:
				len = sizeof(uint64_t);
				break;
			default:
				break;
			}
			if (LIKELY(len > 0))
				m3ApiCheckMem(args[i], len);
		} else
			args[i] = arg_p + i;
	}

	uint8_t indv[ind_cnt];
	ind_cnt = 0;
	for (uint32_t i = 0; i < arg_cnt; i++) {
		switch (sign[i]) {
		case ST_BIO:
		case ST_CIO: {
			const w_ciovec *const w_iovs = args[i];
			k_iovec *const k_iovs = (void*)(indv + ind_cnt);
			args[i] = k_iovs;
			const uint32_t len = m3ApiReadMem32(arg_p + i + 1); //MAYBE: sign[i].len_idx
			for (uint32_t j = 0; j < len; j++) {
				k_iovs[j].base = m3ApiOffsetToPtr(w_iovs[j].base);
				k_iovs[j].len = w_iovs[j].len;
				m3ApiCheckMem(k_iovs[j].base, k_iovs[j].len);
			}
			ind_cnt += len * sizeof(k_iovec);
			break;
		}
		case ST_REFV: {
			const w_ptr *const w_refs = args[i];
			void* *const k_refs = (void*)(indv + ind_cnt);
			args[i] = k_refs;
			const uint32_t len = m3ApiReadMem32(arg_p + i + 1); //MAYBE: sign[i].len_idx
			for (uint32_t j = 0; j < len; j++) {
				k_refs[j] = m3ApiOffsetToPtr(w_refs[j]);
				m3ApiCheckMem(k_refs[j], 1);
			}
			ind_cnt += len * sizeof(void*);
			break;
		}
		default:
			break;
		}
	}
	// assert(ind_cnt == sizeof(indv))

	const struct k_signed_call *const call = _ctx->userdata;
	return call->fn(call->self, args, rets, m3_GetUserData(runtime));
	// big stack free...
}
/** Striped engine_m3_link_signed only for ST_VAL */
static m3ApiRawFunction(engine_m3_link_flat) {
	const uint32_t ret_cnt = m3_GetRetCount(_ctx->function);
	const uint32_t arg_cnt = m3_GetArgCount(_ctx->function);
	uint64_t *const ret_p = _sp;
	_sp += ret_cnt;
	uint64_t *const arg_p = _sp;
	_sp += arg_cnt;

	void *rets[ret_cnt];
	for (uint32_t i = 0; i < ret_cnt; i++) {
		rets[i] = ret_p + i;
	}
	const void *args[arg_cnt];
	for (uint32_t i = 0; i < arg_cnt; i++) {
		args[i] = arg_p + i;
	}

	const struct k_signed_call *const call = _ctx->userdata;
	return call->fn(call->self, args, rets, m3_GetUserData(runtime));
	// some stack free...
}
/** Striped engine_m3_link_flat return one ST_VAL */
static m3ApiRawFunction(engine_m3_link_flat_i) {
	const uint32_t arg_cnt = m3_GetArgCount(_ctx->function);
	uint64_t* ret_p = _sp++;
	uint64_t *const arg_p = _sp;
	_sp += arg_cnt;

	const void *args[arg_cnt];
	for (uint32_t i = 0; i < arg_cnt; i++) {
		args[i] = arg_p + i;
	}

	const struct k_signed_call *const call = _ctx->userdata;
	return call->fn(call->self, args, (k_retv_t)&ret_p, m3_GetUserData(runtime));
	// small stack free...
}
/** Striped engine_m3_link_flat return void */
static m3ApiRawFunction(engine_m3_link_flat_v) {
	const uint32_t arg_cnt = m3_GetArgCount(_ctx->function);
	uint64_t *const arg_p = _sp;
	_sp += arg_cnt;

	const void *args[arg_cnt];
	for (uint32_t i = 0; i < arg_cnt; i++) {
		args[i] = arg_p + i;
	}

	const struct k_signed_call *const call = _ctx->userdata;
	return call->fn(call->self, args, NULL, m3_GetUserData(runtime));
	// small stack free...
}

static engine_runtime *engine_m3_boot(engine_module *em, uint64_t stack_size, enum engine_boot_flags flags, struct engine_runtime_ctx* ctx) {

	IM3Module mod = (IM3Module)em;
	IM3Runtime runtime = m3_NewRuntime(m3_GetModuleEnvironment(mod), stack_size, ctx);
	if (!runtime) {
		logf(WL_CRIT, "Cannot allocate runtime with %zu stack", stack_size);
		return NULL;
	}

	m3_FixStart(mod, "_start");
	m3_FixStart(mod, "_initialize");
	M3Result res;
	res = m3_LoadModule(runtime, mod);
	if (res) {
		m3_FreeModule(mod);
		m3_FreeRuntime(runtime);
		logf(WL_ERR, "While loading %s: %s", m3_GetModuleName(mod), res);
		return NULL;
	}

	if (flags & PG_LINK_FLAG) {
		for (size_t i = 0;; i++) {
			struct k_fn_decl decl;
			{
				IM3Function ifn;
				if (m3_FindFunctionDecl(mod, false, i, &ifn)) break;

				m3_GetFunctionImportName(ifn, &decl.mod, &decl.name);
				decl.retc = m3_GetRetCount(ifn);
				decl.argc = m3_GetArgCount(ifn);
				engine_m3_read_argv(mod, &decl, true);
			}

			const k_signed_call* call;
			if (LIKELY(ctx) && (call = ctx->linker((void*)ctx, decl))) {
				M3RawCall link = engine_m3_link_signed;
				if (k_fn_decl_flat(call->decl)) {
					switch (call->decl.retc)
					{
					case 0:
						link = engine_m3_link_flat_v;
						break;
					case 1:
						link = engine_m3_link_flat_i;
						break;

					default:
						link = engine_m3_link_flat;
						break;
					}
				}
				res = m3_LinkRawFunctionEx(mod, decl.mod, decl.name, NULL, link, call);
				if (LIKELY(!res)) continue;
			} else
				res = "no handler";

			logf(WL_WARN, "While linking %s->%s:%s %s: %s", m3_GetModuleName(mod),
					decl.mod, decl.name, w_fn_sign2str(decl), res);
		}
	}
	if ((flags & PG_COMPILE_FLAG) && (res = m3_CompileModule(mod))) {
		m3_FreeRuntime(runtime);
		logf(WL_WARN, "While compiling %s: %s", m3_GetModuleName(mod), res);
		return NULL;
	}
	if ((flags & PG_START_FLAG) && (res = m3_RunStart(mod))) {
		m3_FreeRuntime(runtime);
		logf(WL_WARN, "While starting %s: %s", m3_GetModuleName(mod), res);
		return NULL;
	}

	return (engine_runtime*)runtime;
}
static K_SIGNED_HDL(engine_m3_call) {
	IM3Function fn = self;
	// Important outofbound danger
	M3Result err = m3_Call(fn, m3_GetArgCount(fn), _args);
	if (err) return err;

	return m3_GetResults(fn, m3_GetRetCount(fn), (k_argv_t)_rets);
}
static bool engine_m3_get_export(engine_runtime *r, struct k_fn_decl decl, k_signed_call* out) {
	IM3Function fn;
	{
		size_t mod_len = strlen(decl.mod);
		size_t name_len = strlen(decl.name);
		char sname[mod_len + name_len + 2];
		memcpy(sname, decl.mod, mod_len);
		sname[mod_len] = ':';
		memcpy(sname + mod_len + 1, decl.name, name_len + 1);
		if (m3_FindFunction(&fn, (IM3Runtime)r, sname)) return false;
	}
	if (decl.argc != m3_GetArgCount(fn) || decl.retc != m3_GetRetCount(fn)) return false;

	out->fn = engine_m3_call;
	out->self = fn;
	out->decl = decl;
	return true;
}
static void engine_m3_free_runtime(engine_runtime *r) {
	m3_FreeRuntime((IM3Runtime)r);
}

static engine_m3 s_m3 = {0};
engine* engine_load(void) {
	if (!s_m3.env) {
		s_m3.env = m3_NewEnvironment();
		if (!s_m3.env) return NULL;

		s_m3.handle.parse = engine_m3_parse;
		s_m3.handle.list_imports = engine_m3_list_imports;
		s_m3.handle.list_exports = engine_m3_list_exports;
		s_m3.handle.free_module = engine_m3_free_module;
		s_m3.handle.boot = engine_m3_boot;
		s_m3.handle.get_export = engine_m3_get_export;
		s_m3.handle.free_runtime = engine_m3_free_runtime;
	}
	return &s_m3.handle;
}
