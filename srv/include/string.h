#ifndef __STRING_H
#define __STRING_H

#include "native.h"

static inline size_t strlen(const char *s) {
	const char *ss = s;
	while (*ss)
		ss++;
	return ss - s;
}

#endif
