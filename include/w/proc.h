#ifndef __MOD_PROC_H
#define __MOD_PROC_H
#include "types.h"

/** Run WASM with system runtime */
W_FN(proc, exec, w_res, (const char* name, w_size name_len, const uint8_t* code, w_size code_len), {ST_ARR, ST_LEN, ST_ARR, ST_LEN})

#endif
