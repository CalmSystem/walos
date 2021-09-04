#ifndef __KERNEL_SIGN_TOOLS_H
#define __KERNEL_SIGN_TOOLS_H

#include "types.h"
#include <string.h>

static inline char w_fn_sign2char(enum w_fn_sign_type s) {
	switch (s)
	{
	case ST_LEN: return 'z';
	case ST_F64: return 'F';
	case ST_F32: return 'f';
	case ST_I64: return 'I';
	case ST_I32: return 'i';
	case ST_VAL: return '.';
	case ST_PTR: return '*';
	case ST_ARR: return 'A';
	case ST_REFV: return 'R';
	case ST_CIO: return 'C';
	case ST_BIO: return 'B';
	default: return '?';
	}
}
static inline enum w_fn_sign_type w_fn_char2sign(char c) {
	switch (c)
	{
	case 'z':
	case 'b':
	case 'c':
	case 'r':
	case 'a':
		return ST_LEN;
	case 'F': return ST_F64;
	case 'f': return ST_F32;
	case 'I': return ST_I64;
	case 'i':
	case 'e':
		return ST_I32;
	case '.': return ST_VAL;
	case '*': return ST_PTR;
	case 'A': return ST_ARR;
	case 'R': return ST_REFV;
	case 'C': return ST_CIO;
	case 'B': return ST_BIO;
	default: return ST_VAL;
	}
}
static inline cstr w_fn_sign2str_r(struct k_fn_decl d, char* io) {
	char* s = io;
	if (d.retc == 0)
		*(s++) = 'v';
	else {
		for (uint32_t i = 0; i < d.retc; i++)
			*(s++) = w_fn_sign2char(ST_VAL);
	}
	*(s++) = '(';
	for (uint32_t i = 0; i < d.argc; i++) {
		*(s++) = d.argv ? w_fn_sign2char(d.argv[i]) : '?';
	}
	*(s++) = ')';
	*(s++) = '\0';
	return io;
}
static inline size_t w_fn_sign2str_len(struct k_fn_decl d) {
	return (d.retc ? d.retc : 1) + 2 + d.argc;
}
static inline cstr w_fn_sign2str(struct k_fn_decl d) {
	static char buf[32];
	return w_fn_sign2str_r(d, buf);
}

static inline size_t w_fn_str2sign(cstr in, enum w_fn_sign_type* out) {
	while (*in && *in != '(') in++;
	if (*in++ != '(') return 0;

	cstr end = in;
	while (*end && *end != ')') end++;
	if (*end != ')' || *(end+1)) return 0;

	const size_t argc = end - in;
	for (size_t i = 0; i < argc; i++) {
		out[i] = w_fn_char2sign(in[i]);
	}
	return argc;
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
