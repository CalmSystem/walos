#ifndef __MOD_LINK_H
#define __MOD_LINK_H
#include "types.h"

/** Reference to an external w_iovec group. Like file descriptor */
typedef intw_t w_io_ref;
/** Reference to an external w_ciovec group. Like file descriptor */
typedef intw_t w_cio_ref;

/** Get number of elements in w_cio_ref or w_io_ref */
W_FN(link, io_cnt, "e(i*)", w_res, (w_cio_ref ref, w_size* cnt))
/** Get len of a w_cio_ref or w_io_ref element */
W_FN(link, io_len, "e(ii*)", w_res, (w_cio_ref ref, w_size idx, w_size* len))
/** Get lens of lencnt first w_cio_ref or w_io_ref elements */
W_FN(link, io_lens, "e(i*i)", w_res, (w_cio_ref ref, w_size* lens, w_size lencnt))
/** Read a w_cio_ref or w_io_ref element */
W_FN(link, io_read, "e(iiBbi*)", w_res, (w_cio_ref ref, w_size idx, const w_iovec* ivs, w_size ilen, w_size offset, w_size* nread))
/** Write a w_io_ref element */
W_FN(link, io_write, "e(iiCci*)", w_res, (w_io_ref ref, w_size idx, const w_ciovec* ovs, w_size olen, w_size offset, w_size *nwritten))

#endif
