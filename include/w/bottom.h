#ifndef __MOD_BOTTOM_H
#define __MOD_BOTTOM_H
#include "types.h"
#include "../shr/log.h"

/** Probably log somewhere */
W_FN(sys, log, void, (enum w_log_level lvl, const w_ciovec* ovs, w_size ocnt), {ST_VAL, ST_CIO, ST_LEN})
/** Wait next event */
W_FN_(sys, yield, void, ())

#endif
