#include "types.h"

#undef NULL
#ifdef __cplusplus
#define NULL 0
#else
# define NULL ((void *)0)
#endif

#define offsetof(t,d) __builtin_offsetof(t, d)
#include "stdint.h"
