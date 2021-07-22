#ifndef __STRING_H
#define __STRING_H

#include "stdint.h"
typedef uint32_t size_t;

static inline size_t strlen(const char *s) {
	const char *ss = s;
	while (*ss)
		ss++;
	return ss - s;
}

#endif
