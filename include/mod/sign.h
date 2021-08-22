#ifndef __MOD_SIGN_H
#define __MOD_SIGN_H

/** Function argument type mapper */
enum w_fn_sign_type {
	ST_BLEN = -5, /* w_size (len of ST_BIO) */
	ST_CLEN, /* w_size (len of ST_CIO) */
	ST_F64, /* Float64 */
	ST_F32, /* Float32 */
	ST_I64, /* Int64 */
	ST_I32, /* Int32 */
	ST_VAL = ST_I32, /* sizeof <= 8 standard value */
	ST_OVAL, /* ST_VAL pointer */
	ST_VEC, /* Sized pointer (len in bytes is next) */
	ST_REFV, /* Vector of ST_VAL pointers (len in w_ptr is next) */
	ST_CIO, /* Vector of const sized pointers (len in w_ciovec is next) */
	ST_BIO /* Vector of sized pointers (len in w_iovec is next) */
};
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

#endif
