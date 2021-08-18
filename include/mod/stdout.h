#ifndef __MOD_STDOUT_H
#define __MOD_STDOUT_H
#include "./common.h"

W_FN(stdout, write, "i(*i)", int, (struct w_iovec *iov, uintw_t icnt))

#endif
