#ifndef __MOD_SYS_H
#define __MOD_SYS_H
#include "types.h"
#include "../shr/log.h"

/** Probably log somewhere */
W_FN(sys, log, void, (enum w_log_level lvl, const w_ciovec* ovs, w_size ocnt), {ST_VAL, ST_CIO, ST_LEN})
/** Wait next event */
W_FN_(sys, tick, void, ())

/** Run WASM with system runtime */
W_FN(sys, exec, w_res, (const char* name, w_size name_len, const uint8_t* code, w_size code_len), {ST_ARR, ST_LEN, ST_ARR, ST_LEN})

#endif
