#ifndef __NATIVE_H
#define __NATIVE_H

#include "stdint.h"
#if defined(__i386__)
typedef int32_t intn_t;
typedef uint32_t uintn_t;
#elif defined(__x86_64__)
typedef int64_t intn_t;
typedef uint64_t uintn_t;
#else
typedef int_t intn_t;
typedef uint_t uintn_t;
#endif
typedef uintn_t size_t;

#endif
