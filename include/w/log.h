#ifndef __MOD_LOG_H
#define __MOD_LOG_H
#include "types.h"
#include "../shr/log.h"

W_FN(log, write, "v(.Cc)", void, (enum w_log_level lvl, const w_ciovec* ovs, w_size ocnt))

#endif
