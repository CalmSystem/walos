#ifndef __MOD_SIGN_H
#define __MOD_SIGN_H

/** Function argument type mapper */
enum w_fn_sign_type {
	ST_LEN = -5, /* Length of previous arg (w_size) */
	ST_F64, /* Float64 */
	ST_F32, /* Float32 */
	ST_I64, /* Int64 */
	ST_I32, /* Int32 */
	ST_VAL, /* sizeof <= 8 unknown value */
	ST_PTR, /* ST_VAL pointer */
	ST_ARR, /* Sized pointer aka array (len in bytes is next) */
	ST_REFV, /* Vector of ST_PTR (len in w_ptr is next) */
	ST_CIO, /* Vector of const sized pointers (len in w_ciovec is next) */
	ST_BIO /* Vector of sized pointers (len in w_iovec is next) */
};
/* Int containing enum w_fn_sign_type.
 * Packed enum are not windows abi compatible */
typedef int8_t w_fn_sign_val;

#endif
