#ifndef __MOD_LINK_H
#define __MOD_LINK_H
#include "types.h"

/** Reference to an external w_iovec group. Like file descriptor */
typedef intw_t w_io_ref;
/** Reference to an external w_ciovec group. Like file descriptor */
typedef intw_t w_cio_ref;

/** Get number of elements in w_cio_ref or w_io_ref */
W_FN(link, io_cnt, w_res, (w_cio_ref ref, w_size* cnt), {ST_VAL, ST_PTR})
/** Get len of a w_cio_ref or w_io_ref element */
W_FN(link, io_len, w_res, (w_cio_ref ref, w_size idx, w_size* len), {ST_VAL, ST_LEN, ST_PTR})
/** Get lens of lencnt first w_cio_ref or w_io_ref elements */
W_FN(link, io_lens, w_res, (w_cio_ref ref, w_size* lens, w_size lencnt), {ST_VAL, ST_PTR, ST_LEN})
/** Read a w_cio_ref or w_io_ref element */
W_FN(link, io_read, w_res,
	(w_cio_ref ref, w_size idx, const w_iovec* ivs, w_size ilen, w_size offset, w_size* nread),
	{ST_VAL, ST_LEN, ST_BIO, ST_LEN, ST_LEN, ST_PTR})
/** Write a w_io_ref element */
W_FN(link, io_write, w_res,
	(w_io_ref ref, w_size idx, const w_ciovec* ovs, w_size olen, w_size offset, w_size *nwritten),
	{ST_VAL, ST_LEN, ST_BIO, ST_LEN, ST_LEN, ST_PTR})

#endif
