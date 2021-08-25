#ifndef __KERNEL_SIGN_TOOLS_H
#define __KERNEL_SIGN_TOOLS_H

#include "types.h"
#include <string.h>

static inline char w_fn_sign2char(enum w_fn_sign_type s) {
	switch (s)
	{
	case ST_BLEN: return 'b';
	case ST_CLEN: return 'c';
	case ST_F64: return 'F';
	case ST_F32: return 'f';
	case ST_I64: return 'I';
	case ST_I32: return 'i';
	case ST_OVAL: return '*';
	case ST_VEC: return 'V';
	case ST_REFV: return 'R';
	case ST_CIO: return 'C';
	case ST_BIO: return 'B';
	}
}
static inline cstr w_fn_sign2str_r(struct k_fn_decl d, char* in) {
	char* s = in;
	if (d.retc == 0)
		*(s++) = 'v';
	else {
		for (uint32_t i = 0; i < d.retc; i++)
			*(s++) = 'i';
	}
	*(s++) = '(';
	for (uint32_t i = 0; i < d.argc; i++) {
		*(s++) = d.argv ? w_fn_sign2char(d.argv[i]) : '?';
	}
	*(s++) = ')';
	*(s++) = '\0';
	return in;
}
static inline cstr w_fn_sign2str(struct k_fn_decl d) {
	static char buf[32];
	return w_fn_sign2str_r(d, buf);
}

static inline bool k_fn_decl_flat(struct k_fn_decl decl) {
	for (uint32_t i = 0; decl.argv && i < decl.argc; i++) {
		if (decl.argv[i] > ST_VAL)
			return false;
	}
	return true;
}
static inline int k_fn_decl_match(struct k_fn_decl decl, struct k_fn_decl into) {
	if (!(decl.retc == into.retc && decl.argc == into.argc &&
		(decl.name == into.name || strcmp(decl.name, into.name) == 0) &&
		(decl.mod == into.mod || strcmp(decl.name, into.name) == 0)))
	return -1;

	if (decl.argv == into.argv)
		return 1;
	if (!decl.argv)
		return 0;
	if (!into.argv)
		return k_fn_decl_flat(decl);

	for (uint32_t i = 0; i < decl.argc; i++) {
		if (decl.argv[i] != into.argv[i])
			return -1;
	}
	return 1;
}

#endif