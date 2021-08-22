#ifndef __MOD_STDOUT_H
#define __MOD_STDOUT_H
#include "./types.h"

W_FN(stdout, write, "e(Cc)", w_res, (const w_ciovec* ovs, w_size ocnt))
W_FN(stdout, putc, "e(i)", w_res, (char c))

#endif
