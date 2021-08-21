#include <kernel/engine.h>
#define K_CTX "engine"
#include <kernel/log.h>
#include "wasm3.h"

typedef struct engine_m3 {
	engine handle;
	IM3Environment env;
} engine_m3;

static engine_module *engine_m3_parse(struct engine *self, struct engine_code_ref ref) {
	IM3Module mod;
	M3Result res = m3_ParseModule(((struct engine_m3*)self)->env, &mod, ref.code, ref.code_size);
	if (res) {
		logf(KL_ERR, "While parsing %s: %s\n", ref.name, res);
		return NULL;
	}
	m3_SetModuleName(mod, ref.name);
	return (void*)mod;
}
static size_t engine_m3_list_imports(engine_module *mod, struct k_fn_decl *decls, size_t declcnt, size_t offset) {
	for (size_t i = 0; i < declcnt; i++) {
		IM3Function ifn;
		if (m3_FindImportFunction((IM3Module)mod, offset + i, &ifn))
			return i;

		m3_GetFunctionImportName(ifn, &decls[i].mod, &decls[i].name);
		decls[i].retc = m3_GetRetCount(ifn);
		decls[i].argc = m3_GetArgCount(ifn);
		//FIXME: only for stdout:write
		static const enum w_fn_sign_type sign[] = {ST_CIO, ST_CLEN};
		decls[i].argv = sign;
	}
	return declcnt;
}
static size_t engine_m3_list_exports(engine_module *mod, struct k_fn_decl *decls, size_t declcnt, size_t offset) {
	//TODO: impl
	return 0;
}
static void engine_m3_free_module(engine_module *mod) {
	m3_FreeModule((IM3Module)mod);
}

/** Some dark magic...
 *  Read args and rets from _sp.
 *  Convert map offset to pointer.
 *  Convert w_iovec to k_iovec.
 *  Play a lot with stack */
static m3ApiRawFunction(engine_m3_generic_link) {
	const uint32_t ret_cnt = m3_GetRetCount(_ctx->function);
	const uint32_t arg_cnt = m3_GetArgCount(_ctx->function);
	uint64_t *const ret_p = _sp;
	_sp += ret_cnt;
	uint64_t *const arg_p = _sp;
	_sp += arg_cnt;

	const void *retv[ret_cnt];
	for (uint32_t i = 0; i < ret_cnt; i++) {
		retv[i] = ret_p + i;
	}

	/*
		struct engine_signed_call *s_call = _ctx->userdata;
		assert(s_call->decl.retc == ret_cnt && s_call->decl.argc == arg_cnt);
	*/
	const enum w_fn_sign_type *const sign = ((struct engine_signed_call *)_ctx->userdata)->decl.argv;

	size_t ind_cnt = 0; /* Indirection buffer size */
	const void *argv[arg_cnt];
	for (uint32_t i = 0; i < arg_cnt; i++) {
		/* Map arguments */
		if (sign[i] > ST_VAL) {
			argv[i] = m3ApiOffsetToPtr(m3ApiReadMem32(arg_p + i));
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
			case ST_VEC:
				len *= sizeof(uint8_t);
				break;
			case ST_OVAL:
				len = sizeof(uint64_t);
				break;
			default:
				break;
			}
			m3ApiCheckMem(argv[i], len);
		} else
			argv[i] = arg_p + i;
	}

	uint8_t indv[ind_cnt];
	ind_cnt = 0;
	for (uint32_t i = 0; i < arg_cnt; i++) {
		switch (sign[i]) {
		case ST_BIO:
		case ST_CIO: {
			const w_ciovec *const w_iovs = argv[i];
			k_iovec *const k_iovs = (void*)(indv + ind_cnt);
			argv[i] = k_iovs;
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
			const w_ptr *const w_refs = argv[i];
			void* *const k_refs = (void*)(indv + ind_cnt);
			argv[i] = k_refs;
			const uint32_t len = m3ApiReadMem32(arg_p + i + 1); //MAYBE: sign[i].len_idx
			for (uint32_t j = 0; j < len; j++) {
				k_refs[j] = m3ApiOffsetToPtr(w_refs[j]);
				m3ApiCheckMem(k_refs[j], 1);
			}
			ind_cnt += len * sizeof(k_iovec);
			break;
		}
		default:
			break;
		}
	}
	// assert(ind_cnt == sizeof(indv))

	((struct engine_signed_call *)_ctx->userdata)->fn(
		(k_refvec){argv, arg_cnt}, (k_refvec){retv, ret_cnt});

	// big stack free...

	return m3Err_none;
}
static engine_runtime *engine_m3_boot(engine_module *em, uint64_t stack_size, enum engine_boot_flags flags, struct engine_runtime_ctx* ctx) {

	IM3Module mod = (IM3Module)em;
	IM3Runtime runtime = m3_NewRuntime(m3_GetModuleEnvironment(mod), stack_size, ctx);
	if (!runtime) {
		logf(KL_CRIT, "Cannot allocate runtime with %zu stack\n", stack_size);
		return NULL;
	}

	m3_FixStart(mod, "_start");
	M3Result res;
	res = m3_LoadModule(runtime, mod);
	if (res) {
		m3_FreeModule(mod);
		m3_FreeRuntime(runtime);
		logf(KL_ERR, "While loading %s: %s\n", m3_GetModuleName(mod), res);
		return NULL;
	}

	if (flags & PG_LINK_FLAG) {
		for (size_t i = 0;; i++) {
			struct k_fn_decl decl;
			if (!engine_m3_list_imports(em, &decl, 1, i))
				break;

			engine_generic_call call;
			if (UNLIKELY(!ctx) || !(call = ctx->linker(ctx->linker_arg, decl))) {
				res = "no handler";
				goto link_warn;
			}

			struct engine_signed_call *s_call = malloc(sizeof(*s_call));
			if (UNLIKELY(!s_call)) {
				res = m3Err_mallocFailed;
				goto link_warn;
			}

			s_call->decl = decl;
			s_call->fn = call;
			res = m3_LinkRawFunctionEx(mod, decl.mod, decl.name, NULL, engine_m3_generic_link, s_call);
			if (UNLIKELY(res)) {
				free(s_call);
				goto link_warn;
			}

			continue;
			link_warn:
				logf(KL_WARN, "While linking %s->%s:%s %s: %s\n", m3_GetModuleName(mod),
					 decl.mod, decl.name, w_fn_sign2str(decl), res);
		}
	}
	if ((flags & PG_COMPILE_FLAG) && (res = m3_CompileModule(mod))) {
		m3_FreeRuntime(runtime);
		logf(KL_WARN, "While compiling %s: %s\n", m3_GetModuleName(mod), res);
		return NULL;
	}
	if ((flags & PG_START_FLAG) && (res = m3_RunStart(mod))) {
		m3_FreeRuntime(runtime);
		logf(KL_WARN, "While starting %s: %s\n", m3_GetModuleName(mod), res);
		return NULL;
	}

	return (engine_runtime*)runtime;
}
static engine_export_fn *engine_m3_get_export(engine_runtime *r, struct k_fn_decl decl) {
	IM3Function fn;
	if (m3_FindFunction(&fn, (IM3Runtime)r, decl.name))
		return NULL;

	//TODO: check mod in table
	//TODO: check sign
	return (engine_export_fn*)fn;
}
static int engine_m3_call(engine_export_fn *f, struct k_refvec args, struct k_refvec rets) {
	IM3Function fn = (IM3Function)f;
	if (m3_GetArgCount(fn) != args.len || m3_GetRetCount(fn) != rets.len)
		return W_EINVAL;

	M3Result res;
	res = m3_Call(fn, args.len, args.ptr);
	if (res) {
		logf(KL_WARN, "While calling %s->%s: %s\n", m3_GetModuleName(m3_GetFunctionModule(fn)),
			m3_GetFunctionName(fn), res);
		return -1;
	}

	if (m3_GetResults(fn, rets.len, rets.ptr))
		return W_ERANGE;

	return 1;
}
static void engine_m3_free_runtime(engine_runtime *r) {
	m3_FreeRuntime((IM3Runtime)r);
}

static engine_m3 s_m3 = {0};
engine* engine_load() {
	if (!s_m3.env) {
		s_m3.env = m3_NewEnvironment();
		if (!s_m3.env) return NULL;

		s_m3.handle.parse = engine_m3_parse;
		s_m3.handle.list_imports = engine_m3_list_imports;
		s_m3.handle.list_exports = engine_m3_list_exports;
		s_m3.handle.free_module = engine_m3_free_module;
		s_m3.handle.boot = engine_m3_boot;
		s_m3.handle.get_export = engine_m3_get_export;
		s_m3.handle.call = engine_m3_call;
		s_m3.handle.free_runtime = engine_m3_free_runtime;
	}
	return &s_m3.handle;
}
