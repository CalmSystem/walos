#ifndef __MOD_STDIO_H
#define __MOD_STDIO_H
#include "types.h"

W_FN(stdio, write, w_res, (const w_ciovec* ovs, w_size ocnt), {ST_CIO, ST_LEN})
W_FN_(stdio, putc, w_res, (char c))

// W_FN(stdio, read, "e(Bb)", w_res, (const w_iovec* ivs, w_size icnt))
W_FN_(stdio, getc, char, ())

#endif
