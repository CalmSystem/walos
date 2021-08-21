#ifndef __KERNEL_OS_H
#define __KERNEL_OS_H

#include "loader.h"

/** Informations given to os_entry */
struct os_ctx_t {
	struct loader_handle handle;
	struct loader_features feats;
};

void os_entry(const struct os_ctx_t*);

#endif
