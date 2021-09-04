#ifndef __MOD_LOG_H
#define __MOD_LOG_H
#include "types.h"
#include "../shr/log.h"

/** Probably log somewhere */
W_FN(log, write, void, (enum w_log_level lvl, const w_ciovec* ovs, w_size ocnt), {ST_VAL, ST_CIO, ST_LEN})

#endif
