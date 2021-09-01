#ifndef __KERNEL_LOG_H
#define __KERNEL_LOG_H
#include "types.h"
#include "../shr/log.h"

void klogs(enum w_log_level, cstr ctx, cstr str);
void klogf(enum w_log_level, cstr ctx, cstr fmt, ...) __attribute__((format(printf, 3, 4)));

#define __STR__(x) #x
#define __STR(x) __STR__(x)

#if DEBUG
#undef K_CTX
#define K_CTX __FILE__ ":" __STR(__LINE__)
#else
#ifndef K_CTX
#define K_CTX "kernel"
#endif
#endif
#define logs(level, str) klogs(level, K_CTX, str)
#define logf(level, fmt, ...) klogf(level, K_CTX, fmt, __VA_ARGS__)

#endif
